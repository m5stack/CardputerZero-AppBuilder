# Demo_ImageViewer

## 目的 / Purpose
- 浏览目录中的 JPG/PNG 图片，自动缩放到 320x170 居中显示。
- Browse JPG/PNG images fitted to the 320x170 framebuffer.

## 依赖 / Deps
- Python 3.8+, Pillow

## 构建 / Build
```
pip install -r requirements.txt
```

## 运行 / Run
```
sudo python3 viewer.py /path/to/images
```
Keys: `h`/`l` (or `a`/`d`, `p`/`n`) prev/next, `Esc` quit.

## 部署 / Deploy
```
scp -r ../Demo_ImageViewer pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_ImageViewer && pip install -r requirements.txt && sudo python3 viewer.py ~/Pictures"
```

## TODO
- Use evdev for real arrow-key handling on Cardputer.
