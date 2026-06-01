# Demo_Clock

## 目的 / Purpose
- 在 CardputerZero 的 1.9" 320x170 LCD 上显示大字体时钟。
- Big-font wall clock on the M5 CardputerZero ST7789V framebuffer.

## 依赖 / Deps
- Linux with `/dev/fb0` (RGB565, 320x170).
- `gcc`, `make`, libc. No other deps.

## 构建 / Build
```
make
```

## 运行 / Run
```
sudo ./clock
```
Press Ctrl+C to exit.

## 部署 / Deploy
```
scp -r ../Demo_Clock pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_Clock && make && sudo ./clock"
```

## Notes
- Uses inline public-domain `font8x8_basic.h` scaled 4x for HH:MM, 2x for seconds and date.
- Refreshes every 500 ms.
- Style mirrors `../FrameBuffer_HelloWorld`.
