# Game_Pong

[中文](#中文) | [English](#english)

## 中文

CardputerZero 上的 Pong 经典双拍游戏，纯 C11 + `/dev/fb0` + evdev。左拍玩家控制，右拍为 AI。先达 10 分者胜。

### 按键

| 键 | 功能 |
| --- | --- |
| UP / W | 左拍上移 |
| DOWN / S | 左拍下移 |
| SPACE | 发球 / 游戏结束后重开 |
| R | 重置比赛 |
| ESC / Q | 退出 |

### 构建 / 运行

```bash
make
./pong
```

### 部署到设备

```bash
scp pong pi@192.168.50.150:~/ && ssh pi@192.168.50.150 ./pong
```

---

## English

Two-paddle Pong for the M5 CardputerZero 320x170 framebuffer, pure C11 using `/dev/fb0` and evdev. Left paddle is human, right paddle is AI. First to 10 wins.

### Controls

| Key | Action |
| --- | --- |
| UP / W | left paddle up |
| DOWN / S | left paddle down |
| SPACE | serve / restart after game over |
| R | restart match |
| ESC / Q | quit |

### Build / Run

```bash
make
./pong
```

### Deploy

```bash
scp pong pi@192.168.50.150:~/ && ssh pi@192.168.50.150 ./pong
```
