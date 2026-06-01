# Game_Invaders

Simplified Space Invaders for M5 CardputerZero (CM0, 512 MB).
320x170 borderless window, SDL2 software renderer, TTF HUD.

## Controls / 控制

| Key | Action |
| --- | --- |
| Left / Right | Move ship / 左右移动 |
| Space | Fire (1 bullet) / 开火 |
| R | Restart / 重开 |
| ESC | Quit / 退出 |

## Layout

- 5 rows x 8 cols invaders (16x10 px) marching as a formation
- 1 player bullet and 1 enemy bullet on screen at a time
- HUD: score top-left, lives top-right
- Fixed 60 Hz timestep

## Build

```
sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
make
```

## Run

```
SDL_VIDEODRIVER=KMSDRM ./invaders
# Headless/CI
SDL_VIDEODRIVER=dummy ./invaders
```

## Deploy on device

```
scp dist/game-invaders_0.1-m5stack1_arm64.deb pi@<ip>:/tmp/
ssh pi@<ip> 'sudo dpkg -i /tmp/game-invaders_*.deb && \
    sudo systemctl restart APPLaunch.service'
```

## 中文说明

精简版太空侵略者。按左右移动飞船，空格开火。消灭全部外星人获胜；外星人落到底线或你失去三条命则失败。按 R 重开，ESC 退出。不需要 GPU，纯 CPU 软件渲染。
