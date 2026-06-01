# Rust FrameBuffer HelloWorld

A minimal Rust example drawing to `/dev/fb0` on the M5 CardputerZero (320x170 ST7789V, RGB565) using `framebuffer` + `embedded-graphics`. Reads keyboard via `evdev` and exits on ESC or Q.

## EN

### Native build on device

Requires `rustup` + `cargo` installed on the CardputerZero.

```bash
cd Rust_FrameBuffer_HelloWorld
cargo build --release
sudo ./target/release/rust_fb_hello
```

### Cross build from macOS (aarch64-unknown-linux-gnu)

Option A: `cross` (recommended, uses Docker)

```bash
cargo install cross
cross build --target aarch64-unknown-linux-gnu --release
```

Option B: raw `cargo` with aarch64 linker

```bash
brew install aarch64-unknown-linux-gnu  # or: brew tap messense/macos-cross-toolchains && brew install aarch64-unknown-linux-gnu
rustup target add aarch64-unknown-linux-gnu
RUSTFLAGS='-C linker=aarch64-linux-gnu-gcc' \
  cargo build --target aarch64-unknown-linux-gnu --release
```

### Deploy

```bash
scp target/aarch64-unknown-linux-gnu/release/rust_fb_hello pi@192.168.50.150:~/
ssh pi@192.168.50.150 'sudo ./rust_fb_hello'
```

`sudo` is required because `/dev/fb0` and `/dev/input/event*` are typically root-only. Alternative: add your user to the `video` and `input` groups.

```bash
sudo usermod -a -G video pi
sudo usermod -a -G input pi
# log out / log in
```

### Controls

- ESC or Q: exit

## ZH

### 设备本机构建

设备上需先通过 rustup 安装 Rust 工具链。

```bash
cd Rust_FrameBuffer_HelloWorld
cargo build --release
sudo ./target/release/rust_fb_hello
```

### macOS 交叉编译

推荐使用 `cross`：

```bash
cargo install cross
cross build --target aarch64-unknown-linux-gnu --release
```

或直接用 `cargo` + aarch64 linker：

```bash
rustup target add aarch64-unknown-linux-gnu
RUSTFLAGS='-C linker=aarch64-linux-gnu-gcc' \
  cargo build --target aarch64-unknown-linux-gnu --release
```

### 部署

```bash
scp target/aarch64-unknown-linux-gnu/release/rust_fb_hello pi@192.168.50.150:~/
ssh pi@192.168.50.150 'sudo ./rust_fb_hello'
```

需要 `sudo` 是因为 `/dev/fb0` 与 `/dev/input/event*` 默认仅 root 可访问。也可把用户加入 `video` / `input` 组以免 sudo。

按 ESC 或 Q 退出。
