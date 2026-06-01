# Demo_Matrix

## 目的 / Purpose
- Matrix 绿色字符雨屏保。
- Green falling-char screensaver on fb0 (320x170).

## 依赖 / Deps
- Linux `/dev/fb0`, `gcc`, `make`.

## 构建 / Build
```
make
```

## 运行 / Run
```
sudo ./matrix
```
Press any key to quit.

## 部署 / Deploy
```
scp -r ../Demo_Matrix pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_Matrix && make && sudo ./matrix"
```

## Notes
- 20 columns x 11 rows at 16x16 cells (8x8 font scale 2).
- Inline `font8x8_basic.h` duplicated.
