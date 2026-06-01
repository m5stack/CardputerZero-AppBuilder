# Audio assets

The game expects two short WAV files here:

- `assets/bounce.wav` - paddle / wall hit
- `assets/break.wav`  - brick destroyed

We do not check in binary audio. Generate them on the device with `sox`
(`sudo apt install sox`).

## Generate

```
# Short 440 Hz beep for paddle/wall bounce (~50 ms)
sox -n -r 22050 -c 1 bounce.wav synth 0.05 sine 440

# Higher-pitched 880 Hz beep for brick break (~80 ms)
sox -n -r 22050 -c 1 break.wav synth 0.08 sine 880

# Optional: give each beep a fast fade-out to avoid clicks
sox bounce.wav bounce_f.wav fade 0 0.05 0.02 && mv bounce_f.wav bounce.wav
sox break.wav  break_f.wav  fade 0 0.08 0.03 && mv break_f.wav  break.wav
```

Place the resulting files in this directory (`assets/`). The game
continues to run silently if either file is missing.

## Alternate sources

- Any royalty-free short WAV from freesound.org will work as long as it
  is 22050 Hz, mono, 16-bit PCM and well under a second.
- Re-encode with: `sox input.wav -r 22050 -c 1 -b 16 output.wav`.
