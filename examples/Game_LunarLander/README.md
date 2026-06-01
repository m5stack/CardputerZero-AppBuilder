# Game_LunarLander

[中文](#中文) | [English](#english)

## 中文

CardputerZero 月球着陆器，纯 C11 + `/dev/fb0` + evdev。程序化生成地形与两块平台（x2/x5 倍率）。垂直 ≤2、水平 ≤1.5、角度 ±8°、落在平台上判定成功。

### 按键

| 键 | 功能 |
| --- | --- |
| UP / W | 主推进 |
| LEFT / A | 左转 2°/帧 |
| RIGHT / D | 右转 2°/帧 |
| SPACE | 重置当局 |
| R | 重新播种地形 |
| ESC / Q | 退出 |

### 构建 / 运行 / 部署

```bash
make && ./lander
scp lander pi@192.168.50.150:~/ && ssh pi@192.168.50.150 ./lander
```

---

## English

Lunar Lander for CardputerZero, pure C11 + `/dev/fb0` + evdev. Procedural terrain with two flat pads (x2/x5 bonus). Land with vy<=2, vx<=1.5, angle within +/-8 deg, on a pad.

### Controls

| Key | Action |
| --- | --- |
| UP / W | main thrust |
| LEFT / A | rotate left 2 deg/frame |
| RIGHT / D | rotate right 2 deg/frame |
| SPACE | reset round |
| R | reseed terrain |
| ESC / Q | quit |

### Build / Run / Deploy

```bash
make && ./lander
scp lander pi@192.168.50.150:~/ && ssh pi@192.168.50.150 ./lander
```
