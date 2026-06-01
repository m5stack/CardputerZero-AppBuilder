# Game_Asteroids

Vector-style Asteroids for M5 CardputerZero (CM0, 512 MB).
320x170 borderless window, SDL2 software renderer, line rendering only.

## Controls / 控制

| Key | Action |
| --- | --- |
| Left / Right | Rotate / 旋转 |
| Up | Thrust / 推进 |
| Space | Shoot / 射击 |
| R | Restart / 重开 |
| ESC | Quit / 退出 |

## Layout

- Black background, white vector lines via SDL_RenderDrawLineF
- 3 large asteroids per wave, split into 2 medium then 2 small on hit
- Screen-wrap on all edges for ship/bullets/rocks
- HUD: score top-left, lives + wave top-right

## Build

```
sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
make
```

## Run

```
SDL_VIDEODRIVER=KMSDRM ./asteroids
SDL_VIDEODRIVER=dummy ./asteroids
```

## Deploy on device

```
scp dist/game-asteroids_0.1-m5stack1_arm64.deb pi@<ip>:/tmp/
ssh pi@<ip> 'sudo dpkg -i /tmp/game-asteroids_*.deb && \
    sudo systemctl restart APPLaunch.service'
```

## 中文说明

矢量风格小行星游戏。左右旋转，上键推进，空格射击。陨石被击中会分裂成更小的块。屏幕边缘包裹。碰到陨石失去一条命，三命耗尽结束。纯软件渲染，无需 GPU。
