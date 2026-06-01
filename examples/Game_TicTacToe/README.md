# Game_TicTacToe

Human vs CPU Tic-Tac-Toe for M5 CardputerZero (CM0, 512 MB).
320x170 borderless window, SDL2 software renderer. Minimax CPU.

## Controls / 控制

| Key | Action |
| --- | --- |
| Arrows | Move cursor / 移动光标 |
| Space / Enter | Place X / 落子 |
| R | Restart / 重开 |
| ESC | Quit / 退出 |

## Layout

- 20 px status bar at top, 3x3 grid (120x120 px) centered below
- Status: "Your turn (X)" / "CPU thinking..." / "You won! (R)" / "CPU wins (R)" / "Draw (R)"
- CPU uses minimax over 9 cells (first move randomised)
- Winning line highlighted in yellow

## Build

```
sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
make
```

## Run

```
SDL_VIDEODRIVER=KMSDRM ./tictactoe
SDL_VIDEODRIVER=dummy ./tictactoe
```

## Deploy on device

```
scp dist/game-tictactoe_0.1-m5stack1_arm64.deb pi@<ip>:/tmp/
ssh pi@<ip> 'sudo dpkg -i /tmp/game-tictactoe_*.deb && \
    sudo systemctl restart APPLaunch.service'
```

## 中文说明

人机井字棋。方向键移动光标，空格或回车落子。CPU 使用 minimax，首步随机。R 重开，ESC 退出。纯软件渲染，无需 GPU。
