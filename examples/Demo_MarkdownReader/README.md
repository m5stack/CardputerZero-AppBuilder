# Demo_MarkdownReader

## 目的 / Purpose
- 在 320x170 LCD 上分页阅读 .md / .txt 文件。
- Paged text reader for .md files on the CardputerZero framebuffer.

## 依赖 / Deps
- Python 3.8+, Pillow
- DejaVuSansMono font

## 构建 / Build
```
pip install -r requirements.txt
```

## 运行 / Run
```
sudo python3 reader.py sample.md
```
Keys: `Space` next page, `b` previous, `Esc` quit.

## 部署 / Deploy
```
scp -r ../Demo_MarkdownReader pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_MarkdownReader && pip install -r requirements.txt && sudo python3 reader.py sample.md"
```

## Notes
- Markdown syntax is not parsed in v1 (plain text rendering).
- `sample.md` contains ~200 lines so paging is exercised.
