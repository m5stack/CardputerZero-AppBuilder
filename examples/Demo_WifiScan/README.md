# Demo_WifiScan

## 目的 / Purpose
- 扫描并在 LCD 上显示附近 WiFi 列表（信号强度条），支持滚动与刷新。
- Scan nearby WiFi networks and render a scrollable list on fb0.

## 依赖 / Deps
- Python 3.8+, Pillow
- `iw` package (`sudo apt install iw`)
- wlan0 interface
- DejaVu font

## 构建 / Build
```
pip install -r requirements.txt
```

## 运行 / Run
```
sudo python3 scan.py
```
Keys: `R` rescan, `j`/`k` scroll, `Esc`/`q` quit. (For Cardputer: bind evdev arrow keys to these.)

## 部署 / Deploy
```
scp -r ../Demo_WifiScan pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_WifiScan && pip install -r requirements.txt && sudo python3 scan.py"
```

## TODO
- Replace stdin key reader with evdev for proper Cardputer arrow-key support.
