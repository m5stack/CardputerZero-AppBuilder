use std::fs;
use std::path::PathBuf;
use std::thread;
use std::time::{Duration, Instant};

use embedded_graphics::{
    mono_font::{ascii::{FONT_10X20, FONT_6X10}, MonoTextStyle},
    pixelcolor::Rgb565,
    prelude::*,
    primitives::{PrimitiveStyle, PrimitiveStyleBuilder, Rectangle},
    text::{Baseline, Text},
};
use evdev::{Device, Key};
use framebuffer::Framebuffer;

fn rgb565(r: u8, g: u8, b: u8) -> u16 {
    ((r as u16 >> 3) << 11) | ((g as u16 >> 2) << 5) | (b as u16 >> 3)
}

struct FbTarget<'a> {
    buf: &'a mut [u8],
    width: u32,
    height: u32,
    line_length: u32,
    bpp: u32,
}

impl<'a> FbTarget<'a> {
    fn new(buf: &'a mut [u8], width: u32, height: u32, line_length: u32, bpp: u32) -> Self {
        Self { buf, width, height, line_length, bpp }
    }

    fn clear_color(&mut self, color: u16) {
        let bytes_per_pixel = (self.bpp / 8) as usize;
        if bytes_per_pixel == 2 {
            let lo = (color & 0xff) as u8;
            let hi = (color >> 8) as u8;
            for y in 0..self.height as usize {
                let row = &mut self.buf[y * self.line_length as usize..][..self.width as usize * 2];
                for px in row.chunks_exact_mut(2) {
                    px[0] = lo;
                    px[1] = hi;
                }
            }
        }
    }

    fn put_pixel(&mut self, x: i32, y: i32, color: u16) {
        if x < 0 || y < 0 || (x as u32) >= self.width || (y as u32) >= self.height {
            return;
        }
        let bytes_per_pixel = (self.bpp / 8) as usize;
        let offset = (y as u32 * self.line_length) as usize + (x as u32 as usize) * bytes_per_pixel;
        if bytes_per_pixel == 2 {
            self.buf[offset] = (color & 0xff) as u8;
            self.buf[offset + 1] = (color >> 8) as u8;
        } else if bytes_per_pixel == 4 {
            let r = ((color >> 11) & 0x1f) as u8;
            let g = ((color >> 5) & 0x3f) as u8;
            let b = (color & 0x1f) as u8;
            let r8 = (r << 3) | (r >> 2);
            let g8 = (g << 2) | (g >> 4);
            let b8 = (b << 3) | (b >> 2);
            self.buf[offset] = b8;
            self.buf[offset + 1] = g8;
            self.buf[offset + 2] = r8;
            self.buf[offset + 3] = 0xff;
        }
    }
}

impl<'a> OriginDimensions for FbTarget<'a> {
    fn size(&self) -> Size {
        Size::new(self.width, self.height)
    }
}

impl<'a> DrawTarget for FbTarget<'a> {
    type Color = Rgb565;
    type Error = core::convert::Infallible;

    fn draw_iter<I>(&mut self, pixels: I) -> Result<(), Self::Error>
    where
        I: IntoIterator<Item = Pixel<Self::Color>>,
    {
        for Pixel(p, c) in pixels {
            let raw: u16 = RawU16::from(c).into_inner();
            self.put_pixel(p.x, p.y, raw);
        }
        Ok(())
    }
}

use embedded_graphics::pixelcolor::raw::RawU16;

fn find_keyboard() -> Option<Device> {
    let dir = fs::read_dir("/dev/input").ok()?;
    let mut paths: Vec<PathBuf> = dir
        .filter_map(|e| e.ok())
        .map(|e| e.path())
        .filter(|p| {
            p.file_name()
                .and_then(|n| n.to_str())
                .map(|n| n.starts_with("event"))
                .unwrap_or(false)
        })
        .collect();
    paths.sort();
    for p in paths {
        if let Ok(dev) = Device::open(&p) {
            if let Some(keys) = dev.supported_keys() {
                if keys.contains(Key::KEY_ESC) || keys.contains(Key::KEY_Q) {
                    return Some(dev);
                }
            }
        }
    }
    None
}

fn main() {
    let mut fb = Framebuffer::new("/dev/fb0").expect("open /dev/fb0 failed");
    let xres = fb.var_screen_info.xres;
    let yres = fb.var_screen_info.yres;
    let bpp = fb.var_screen_info.bits_per_pixel;
    let line_length = fb.fix_screen_info.line_length;
    println!("fb0: {}x{} bpp={} line_length={}", xres, yres, bpp, line_length);

    let fb_size = (line_length * yres) as usize;
    let mut frame = vec![0u8; fb_size];

    let bg = rgb565(10, 14, 30);
    let fg = rgb565(240, 240, 240);
    let accent = rgb565(0, 180, 255);
    let track = rgb565(60, 60, 80);
    let border_col = rgb565(180, 180, 180);

    let mut kb = find_keyboard();

    let start = Instant::now();
    let duration_ms: u128 = 3000;

    loop {
        let elapsed = start.elapsed().as_millis();
        let progress = (elapsed.min(duration_ms) as f32 / duration_ms as f32).min(1.0);

        {
            let mut target = FbTarget::new(&mut frame, xres, yres, line_length, bpp);
            target.clear_color(bg);

            let border_style: PrimitiveStyle<Rgb565> = PrimitiveStyleBuilder::new()
                .stroke_color(Rgb565::new(
                    ((border_col >> 11) & 0x1f) as u8,
                    ((border_col >> 5) & 0x3f) as u8,
                    (border_col & 0x1f) as u8,
                ))
                .stroke_width(1)
                .build();
            Rectangle::new(Point::new(2, 2), Size::new(xres - 4, yres - 4))
                .into_styled(border_style)
                .draw(&mut target)
                .ok();

            let title_style = MonoTextStyle::new(
                &FONT_10X20,
                Rgb565::new(
                    ((fg >> 11) & 0x1f) as u8,
                    ((fg >> 5) & 0x3f) as u8,
                    (fg & 0x1f) as u8,
                ),
            );
            Text::with_baseline(
                "Hello CardputerZero",
                Point::new(20, 40),
                title_style,
                Baseline::Top,
            )
            .draw(&mut target)
            .ok();

            let sub_style = MonoTextStyle::new(
                &FONT_6X10,
                Rgb565::new(
                    ((accent >> 11) & 0x1f) as u8,
                    ((accent >> 5) & 0x3f) as u8,
                    (accent & 0x1f) as u8,
                ),
            );
            Text::with_baseline(
                "Rust + framebuffer + embedded-graphics",
                Point::new(20, 70),
                sub_style,
                Baseline::Top,
            )
            .draw(&mut target)
            .ok();
            Text::with_baseline(
                "Press ESC or Q to exit",
                Point::new(20, 85),
                sub_style,
                Baseline::Top,
            )
            .draw(&mut target)
            .ok();

            let bar_x: i32 = 20;
            let bar_y: i32 = 120;
            let bar_w: u32 = xres.saturating_sub(40);
            let bar_h: u32 = 18;

            let track_style: PrimitiveStyle<Rgb565> = PrimitiveStyleBuilder::new()
                .fill_color(Rgb565::new(
                    ((track >> 11) & 0x1f) as u8,
                    ((track >> 5) & 0x3f) as u8,
                    (track & 0x1f) as u8,
                ))
                .build();
            Rectangle::new(Point::new(bar_x, bar_y), Size::new(bar_w, bar_h))
                .into_styled(track_style)
                .draw(&mut target)
                .ok();

            let filled_w = (bar_w as f32 * progress) as u32;
            if filled_w > 0 {
                let fill_style: PrimitiveStyle<Rgb565> = PrimitiveStyleBuilder::new()
                    .fill_color(Rgb565::new(
                        ((accent >> 11) & 0x1f) as u8,
                        ((accent >> 5) & 0x3f) as u8,
                        (accent & 0x1f) as u8,
                    ))
                    .build();
                Rectangle::new(Point::new(bar_x, bar_y), Size::new(filled_w, bar_h))
                    .into_styled(fill_style)
                    .draw(&mut target)
                    .ok();
            }

            let pct = (progress * 100.0) as i32;
            let pct_text = format!("{}%", pct);
            Text::with_baseline(
                &pct_text,
                Point::new(bar_x, bar_y + bar_h as i32 + 4),
                sub_style,
                Baseline::Top,
            )
            .draw(&mut target)
            .ok();
        }

        fb.write_frame(&frame);

        if let Some(dev) = kb.as_mut() {
            if let Ok(events) = dev.fetch_events() {
                for ev in events {
                    if ev.event_type() == evdev::EventType::KEY && ev.value() == 1 {
                        let code = ev.code();
                        if code == Key::KEY_ESC.code() || code == Key::KEY_Q.code() {
                            return;
                        }
                    }
                }
            }
        }

        thread::sleep(Duration::from_millis(33));
    }
}
