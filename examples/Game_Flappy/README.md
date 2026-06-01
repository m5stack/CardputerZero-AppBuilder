# Game_Flappy

[中文](#中文) | [English](#english)

## 中文

CardputerZero 上的 Flappy Bird 克隆，纯 C11 + `/dev/fb0` + evdev。点按跳跃穿过管道间隙。

### 按键

| 键 | 功能 |
| --- | --- |
| SPACE / UP | 跳跃 |
| R | 死亡后重开 |
| ESC / Q | 退出 |

### 构建 / 运行

```bash
make
./flappy
```

### 部署到设备

```bash
scp flappy pi@192.168.50.150:~/ && ssh pi@192.168.50.150 ./flappy
```

---

## English

Flappy Bird clone for the M5 CardputerZero 320x170 framebuffer, pure C11 using `/dev/fb0` and evdev. Tap to jump through pipe gaps.

### Controls

| Key | Action |
| --- | --- |
| SPACE / UP | flap / jump |
| R | restart after death |
| ESC / Q | quit |

### Build / Run

```bash
make
./flappy
```

### Deploy

```bash
scp flappy pi@192.168.50.150:~/ && ssh pi@192.168.50.150 ./flappy
```
