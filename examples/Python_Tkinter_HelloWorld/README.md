# Python Tkinter HelloWorld on CardputerZero

A minimal Tkinter demo targeting the M5 CardputerZero (Raspberry Pi CM0, 320x170 LCD on `/dev/fb0`). Shows a centered "Hello CardputerZero" label plus a live 1 Hz clock.

## Why Tkinter at 320x170

Tkinter ships with Python, needs no extra packaging, and is handy for quick keyboard-driven prototypes on a tiny screen. It is not meant for high-FPS UIs; for real apps prefer LVGL, Slint, or SDL2.

## The Honest Constraint

Tk **requires an X server** (a `DISPLAY`). CM0 does not run Xorg by default, so we need either:

- a tiny X server (Xvfb or Xfbdev/kdrive), or
- a desktop session.

Pure Tkinter against raw `/dev/fb0` without any X is **not possible**. Both paths below use Xvfb, then mirror its buffer onto `/dev/fb0`.

## Install Dependencies (on the device)

```bash
sudo apt update
sudo apt install -y python3-tk python3-pil python3-numpy xvfb ffmpeg
# optional: pip install -r requirements.txt
```

Add user to `video` group so `/dev/fb0` is writable without sudo:

```bash
sudo usermod -aG video pi
# re-login required
```

## Path A: Xvfb + ffmpeg mirror (recommended)

Simplest, highest FPS. `ffmpeg` grabs X11 frames and writes RGB565 directly to `/dev/fb0`.

```bash
chmod +x run_xvfb.sh
sudo ./run_xvfb.sh
```

## Path B: Xvfb + pure-Python PIL mirror

Same idea, but the Python app itself grabs and blits. Useful when `ffmpeg` is not installed.

```bash
chmod +x run_self_mirror.sh
./run_self_mirror.sh
```

The script uses `sudo -E python3 hello_tk.py --mirror-to-fb` so `/dev/fb0` is writable while `DISPLAY` is preserved.

## Deploy via SSH

```bash
rsync -av ./ pi@192.168.50.150:~/Python_Tkinter_HelloWorld/
ssh pi@192.168.50.150
cd ~/Python_Tkinter_HelloWorld && sudo ./run_xvfb.sh
```

(password: `pi`)

## Controls

- `ESC` or `Q`: quit the Tk app.
- `Ctrl-C` in the terminal: the wrapper scripts trap and clean up Xvfb / ffmpeg.

## Performance

- Expect roughly 10-15 FPS end-to-end. Tkinter redraws are not cheap on aarch64 at 1 GHz.
- Lower `--fps` on the Python mirror path if you see CPU saturation: `python3 hello_tk.py --mirror-to-fb --fps 8`.

## Troubleshooting

- `cannot open display`: Xvfb did not start. Check with `ps aux | grep Xvfb`. Try launching it manually: `Xvfb :1 -screen 0 320x170x16 &`.
- Garbled colors on the LCD: format mismatch. In `run_xvfb.sh` try `-pix_fmt bgr565le` instead of `rgb565le`.
- `Permission denied` on `/dev/fb0`: run with `sudo` or add your user to the `video` group (see above).
- Nothing appears on the LCD but Xvfb is running: verify `/dev/fb0` exists and `fbset -i` reports 320x170x16.

---

## 中文说明

在 M5 CardputerZero (CM0, 320x170 LCD, `/dev/fb0`, RGB565) 上用 Tkinter 显示 "Hello CardputerZero" 和每秒刷新的时钟。

### 为什么要跑 Xvfb

Tkinter 必须要有 X 服务器 (`DISPLAY`)。CM0 通常没有图形桌面，所以我们用 **Xvfb** (虚拟 X 服务器) 跑 Tk，然后把 Xvfb 的画面镜像到 `/dev/fb0`。完全没有 X 的 Tkinter 是跑不起来的，这是 Tk 本身的限制。

### 安装

```bash
sudo apt install -y python3-tk python3-pil python3-numpy xvfb ffmpeg
sudo usermod -aG video pi   # 让 pi 用户可写 /dev/fb0
```

### 路径 A: Xvfb + ffmpeg (推荐)

```bash
chmod +x run_xvfb.sh
sudo ./run_xvfb.sh
```

### 路径 B: Xvfb + Python PIL 自己镜像

```bash
chmod +x run_self_mirror.sh
./run_self_mirror.sh
```

### 部署

```bash
rsync -av ./ pi@192.168.50.150:~/Python_Tkinter_HelloWorld/
```

### 退出与清理

按 `ESC` 或 `Q` 退出 Tk；终端里 `Ctrl-C` 会被脚本捕获并清理 Xvfb 与 ffmpeg。

### 常见问题

- `cannot open display`：Xvfb 没起来，`ps aux | grep Xvfb` 看一下。
- 颜色错乱：换 `-pix_fmt bgr565le`。
- `/dev/fb0` 无权限：`sudo` 或加入 `video` 组。

性能约 10-15 FPS，Tkinter 不适合做高刷新率 UI；要更丝滑请用 LVGL / Slint。
