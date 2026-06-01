# Demo_SysDashboard

## 目的 / Purpose
- 在 320x170 LCD 上显示 CPU、内存、温度、IP 的实时仪表盘（每秒刷新）。
- Realtime CPU / Mem / Temp / IP dashboard on the CardputerZero framebuffer.

## 依赖 / Deps
- Python 3.8+
- Pillow (`pip install -r requirements.txt`)
- DejaVuSans font at `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`
- procfs / sysfs: `/proc/stat`, `/proc/meminfo`, `/sys/class/thermal/thermal_zone0/temp`
- `ip` command for wlan0 address

Reference: framebuffer mmap pattern from `../Python_FrameBuffer_HelloWorld` README.

## 构建 / Build
```
pip install -r requirements.txt
```

## 运行 / Run
```
sudo python3 dashboard.py
```

## 部署 / Deploy
```
scp -r ../Demo_SysDashboard pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_SysDashboard && pip install -r requirements.txt && sudo python3 dashboard.py"
```
