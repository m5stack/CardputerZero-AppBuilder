# Demo_MusicSpectrum

## 目的 / Purpose
- 架构 stub：ALSA 麦克风采集 -> FFT (fftw3) -> 320x170 fb0 上 16 条频谱条。
- Architectural stub: ALSA capture -> fftw3 -> 16 spectrum bars on fb0.

## 架构 / Architecture
```
  mic --(arecord / snd_pcm_readi)--> int16 PCM @ 44.1 kHz mono
     --(windowing + fftw3 r2c)--> 512 complex bins
     --(log-space bucket to 16 bands)--> bars[16]
     --(draw rects into mmap'd /dev/fb0 RGB565)--> LCD
```

## 当前状态 / Current state
- `skeleton.c` opens the default ALSA capture PCM and reads frames in a loop.
- `compute_spectrum()` and `draw_bars()` are TODO (FFT + framebuffer render).
- `font8x8_basic.h` bundled for future axis labels.

## 依赖 / Deps
- `libasound2-dev` (ALSA)
- `libfftw3-dev` (for the real FFT; add `-lfftw3f` to Makefile LDLIBS when wiring)
- Inline `font8x8_basic.h` included.

## 构建 / Build
```
sudo apt install libasound2-dev libfftw3-dev
make
```

## 运行 / Run
```
sudo ./spectrum
```
Ctrl+C to exit. Today only PCM is consumed; no display output yet.

## 部署 / Deploy
```
scp -r ../Demo_MusicSpectrum pi@192.168.50.150:~/
ssh pi@192.168.50.150 "cd Demo_MusicSpectrum && make && sudo ./spectrum"
```

## TODO
- Hook fftw3f r2c plan in `compute_spectrum`.
- Implement `draw_bars` against `/dev/fb0` (RGB565 320x170).
- Apply Hann window + frame overlap.
- Map amplitude to log-scale bar heights 0..150 px.
