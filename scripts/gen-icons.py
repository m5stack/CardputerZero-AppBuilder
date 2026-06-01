#!/usr/bin/env python3
"""
Generate 48x48 PNG icons for every example that lacks packaging/icon.png.

Design: rounded-square background in a stack-themed color, with a
category-themed badge + short uppercase label.

Categories & palette are keyed off PKG_NAME from packaging/meta.env.
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
SIZE = 48
RADIUS = 9
FONT_BOLD = "/System/Library/Fonts/SFNS.ttf"
FONT_FALLBACK = "/System/Library/Fonts/Helvetica.ttc"


# ── per-example config: (bg_color, fg_color, glyph, label) ──────────────
# bg = background hex, glyph = decoration ("circle","bar","grid","triangle",
#                                          "plus","dot3","arrow","pixel"),
# label = 1-5 char uppercase caption under the glyph.
ICONS = {
    # ── framebuffer (stack: pure C / fb) — slate gray ──
    "framebuffer-hello":  ("#2D3748", "#F6AD55", "bar",     "FB"),
    "framebuffer-snake":  ("#2D3748", "#68D391", "grid",    "SNK"),

    # ── SDL2 — teal ──
    "sdl2-hello":         ("#0F766E", "#F0FDFA", "circle",  "SDL"),
    "sdl2-breakout":      ("#0F766E", "#FDE68A", "grid",    "BRK"),

    # ── LVGL — indigo ──
    "lvgl-hello":         ("#4338CA", "#FDE68A", "circle",  "LV"),

    # ── Rust — orange ──
    "rust-fb-hello":      ("#B7410E", "#FFE4C4", "triangle","RS"),

    # ── Python — blue/yellow ──
    "py-fb-hello":        ("#1D4ED8", "#FDE047", "pixel",   "PY"),
    "py-tk-hello":        ("#1D4ED8", "#FDE047", "plus",    "TK"),

    # ── Qt / PyQt — green ──
    "qt-hello":           ("#14532D", "#A7F3D0", "circle",  "QT"),
    "py-qt-hello":        ("#14532D", "#FDE047", "circle",  "PQT"),

    # ── Slint — blue-green ──
    "slint-hello":        ("#065F46", "#99F6E4", "arrow",   "SL"),

    # ── Demos (domain-themed) ──────────────────────────
    "demo-2048":          ("#EDC22E", "#3C3A32", "grid",    "2048"),
    "demo-clock":         ("#1E293B", "#F1F5F9", "circle",  "CLK"),
    "demo-imgview":       ("#7C3AED", "#FDE68A", "plus",    "IMG"),
    "demo-matrix":        ("#000000", "#22C55E", "pixel",   "MTX"),
    "demo-mdread":        ("#1F2937", "#F3F4F6", "bar",     "MD"),
    "demo-spectrum":      ("#701A75", "#F0ABFC", "bar",     "SPEC"),
    "demo-sysdash":       ("#0E7490", "#E0F2FE", "grid",    "SYS"),
    "demo-wifiscan":      ("#164E63", "#22D3EE", "dot3",    "WIFI"),

    # ── Games (vivid accent colors) ──
    "game-asteroids":     ("#0B0F19", "#D1D5DB", "dot3",    "AST"),
    "game-flappy":        ("#38BDF8", "#FACC15", "triangle","FLP"),
    "game-invaders":      ("#111827", "#22D3EE", "pixel",   "INV"),
    "game-lander":        ("#0F172A", "#E5E7EB", "triangle","LND"),
    "game-minesweeper":   ("#475569", "#F97316", "grid",    "MIN"),
    "game-pong":          ("#0F172A", "#FFFFFF", "bar",     "PNG"),
    "game-sokoban":       ("#713F12", "#FCD34D", "grid",    "SKB"),
    "game-tetris":        ("#1E40AF", "#A78BFA", "grid",    "TET"),
    "game-tictactoe":     ("#991B1B", "#FDE68A", "grid",    "TTT"),

    # ── NES ──
    "nes-emulator":       ("#7F1D1D", "#FECACA", "pixel",   "NES"),
}


def load_font(size: int) -> ImageFont.FreeTypeFont:
    for path in (FONT_BOLD, FONT_FALLBACK):
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, size)
            except OSError:
                continue
    return ImageFont.load_default()


def hex_to_rgb(h: str) -> tuple[int, int, int]:
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def rounded_bg(bg: tuple[int, int, int]) -> Image.Image:
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, SIZE - 1, SIZE - 1), radius=RADIUS, fill=bg + (255,))
    # subtle top-highlight
    d.rounded_rectangle((2, 2, SIZE - 3, SIZE // 2), radius=RADIUS - 2,
                        fill=None, outline=(255, 255, 255, 28), width=1)
    return img


def draw_glyph(img: Image.Image, kind: str, fg: tuple[int, int, int]) -> None:
    d = ImageDraw.Draw(img, "RGBA")
    cx, cy = SIZE // 2, 20  # glyph centered in top half
    col = fg + (255,)

    if kind == "circle":
        r = 9
        d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=col, width=2)
    elif kind == "bar":
        heights = [6, 14, 10, 18, 8]
        total_w = len(heights) * 4 - 1
        x0 = cx - total_w // 2
        for i, h in enumerate(heights):
            x = x0 + i * 4
            d.rectangle((x, cy + 10 - h, x + 3, cy + 10), fill=col)
    elif kind == "grid":
        sz = 4
        gap = 1
        n = 4
        total = n * sz + (n - 1) * gap
        x0 = cx - total // 2
        y0 = cy - total // 2
        for i in range(n):
            for j in range(n):
                if (i + j) % 2 == 0:
                    x = x0 + j * (sz + gap)
                    y = y0 + i * (sz + gap)
                    d.rectangle((x, y, x + sz - 1, y + sz - 1), fill=col)
    elif kind == "triangle":
        d.polygon([(cx, cy - 10), (cx - 9, cy + 8), (cx + 9, cy + 8)],
                  outline=col, width=2)
    elif kind == "plus":
        d.rectangle((cx - 2, cy - 10, cx + 2, cy + 10), fill=col)
        d.rectangle((cx - 10, cy - 2, cx + 10, cy + 2), fill=col)
    elif kind == "dot3":
        for dx in (-8, 0, 8):
            d.ellipse((cx + dx - 3, cy - 3, cx + dx + 3, cy + 3), fill=col)
    elif kind == "arrow":
        d.line((cx - 10, cy + 4, cx, cy - 6), fill=col, width=3)
        d.line((cx, cy - 6, cx + 10, cy + 4), fill=col, width=3)
    elif kind == "pixel":
        pts = [(-6, -6), (0, -6), (6, -6),
               (-6,  0), (6,  0),
               (-6,  6), (0,  6), (6,  6)]
        for dx, dy in pts:
            d.rectangle((cx + dx - 2, cy + dy - 2, cx + dx + 2, cy + dy + 2), fill=col)


def draw_label(img: Image.Image, text: str, fg: tuple[int, int, int]) -> None:
    d = ImageDraw.Draw(img)
    # pick size so the word fits
    for sz in (16, 14, 12, 11, 10, 9):
        font = load_font(sz)
        bbox = d.textbbox((0, 0), text, font=font)
        w = bbox[2] - bbox[0]
        if w <= SIZE - 8:
            break
    h = bbox[3] - bbox[1]
    x = (SIZE - w) // 2 - bbox[0]
    y = SIZE - h - 5 - bbox[1]
    d.text((x, y), text, font=font, fill=fg + (255,))


def render_icon(pkg: str, cfg: tuple[str, str, str, str]) -> Image.Image:
    bg_hex, fg_hex, glyph, label = cfg
    bg = hex_to_rgb(bg_hex)
    fg = hex_to_rgb(fg_hex)
    img = rounded_bg(bg)
    draw_glyph(img, glyph, fg)
    draw_label(img, label, fg)
    return img


def example_dirs_with_meta() -> list[Path]:
    result = []
    for d in sorted(REPO.iterdir()):
        if not d.is_dir():
            continue
        meta = d / "packaging" / "meta.env"
        if meta.exists():
            result.append(d)
    return result


def parse_pkg_name(meta: Path) -> str | None:
    txt = meta.read_text()
    m = re.search(r'^\s*PKG_NAME\s*=\s*"?([A-Za-z0-9._+\-]+)"?\s*$', txt, re.M)
    return m.group(1) if m else None


def main() -> int:
    out_dir = REPO / "scripts" / "generated-icons"
    out_dir.mkdir(parents=True, exist_ok=True)

    rendered = 0
    skipped = 0
    missing_cfg: list[str] = []

    for ex in example_dirs_with_meta():
        meta = ex / "packaging" / "meta.env"
        pkg = parse_pkg_name(meta)
        if not pkg:
            print(f"skip {ex.name}: no PKG_NAME")
            skipped += 1
            continue

        dst = ex / "packaging" / "icon.png"
        if dst.exists():
            print(f"skip {pkg}: packaging/icon.png already exists")
            skipped += 1
            continue

        if pkg not in ICONS:
            missing_cfg.append(pkg)
            continue

        img = render_icon(pkg, ICONS[pkg])
        img.save(dst, "PNG")
        # Also drop a copy into generated-icons/ for quick preview / device
        # push without requiring re-running build steps.
        img.save(out_dir / f"{pkg}.png", "PNG")
        print(f"  {pkg}.png")
        rendered += 1

    print(f"\nrendered: {rendered}, skipped: {skipped}")
    if missing_cfg:
        print(f"no icon config for: {missing_cfg}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
