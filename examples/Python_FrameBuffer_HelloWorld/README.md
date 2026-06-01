# Python FrameBuffer HelloWorld (M5 CardputerZero)

A minimal Python demo that draws directly to `/dev/fb0` (320x170, RGB565) on the
M5 CardputerZero (aarch64 Linux, ST7789V). Renders a centered title plus a live
clock, refreshing once per second. Exits on Ctrl-C or on ESC/Q from the
keyboard via evdev.

## Why Python here / 为什么用 Python

- Fast prototyping for small 320x170 widgets / dashboards.
- Pillow handles text + layout, numpy handles the RGB to RGB565 conversion.
- Not intended for high FPS; use C / LVGL / SDL2 for smooth animation.
- 适合快速原型和小尺寸控件，不追求高帧率。

## Install / 安装

Option A (recommended, system packages):

```bash
sudo apt install python3-pip python3-pil python3-numpy python3-evdev fonts-dejavu-core
```

Option B (pip):

```bash
pip3 install -r requirements.txt
```

## Run / 运行

```bash
sudo ./run.sh
```

Framebuffer access usually needs root. To avoid `sudo` you can grant the user
access to `/dev/fb0`:

```bash
sudo chmod 660 /dev/fb0
sudo usermod -aG video pi
# log out and back in
```

Press `ESC` or `Q` on the Cardputer keyboard to quit, or `Ctrl-C` in the shell.

## Deploy / 部署

From your workstation:

```bash
rsync -av ./ pi@192.168.50.150:~/Python_FrameBuffer_HelloWorld/
```

Then SSH in and run `sudo ./run.sh` inside the synced directory.

## Files

- `fb_hello.py` - main script (ioctl + mmap + PIL + RGB565 + evdev).
- `requirements.txt` - pip deps.
- `run.sh` - Linux-only launcher.
