# Demo_2048

## 目的 / Purpose
- 键盘控制的 2048 小游戏，4x4 方格 160x160 居左，右侧 160x170 显示分数。
- Keyboard 2048 on the 320x170 LCD.

## 依赖 / Deps
- Linux `/dev/fb0` (RGB565 320x170), `gcc`, `make`.

## 构建 / Build
```
make
```

## 运行 / Run
```
sudo ./demo2048
```
Keys (current skeleton, stdin): `h`/`j`/`k`/`l` = left/down/up/right, `R` reset, `Esc` quit.
TODO: swap stdin for evdev to capture the Cardputer arrow keys.

## 部署 / Deploy
```
scp -r ../Demo_2048 pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_2048 && make && sudo ./demo2048"
```

## Notes
- Inline `font8x8_basic.h` copied from `../Demo_Clock` (public-domain font).
- fb/input pattern follows `../FrameBuffer_Game`.
