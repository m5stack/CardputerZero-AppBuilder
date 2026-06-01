FROM --platform=linux/arm64 ubuntu:24.04

LABEL org.opencontainers.image.source="https://github.com/CardputerZero/CardputerZero-Examples"
LABEL org.opencontainers.image.description="CardputerZero SDK - arm64 build environment for apps and .deb packaging"
LABEL org.opencontainers.image.licenses="MIT"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    # Core build tools
    build-essential gcc g++ make cmake git pkg-config \
    # Deb packaging
    dpkg-dev fakeroot \
    # SDL2 development
    libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev \
    # Qt5 development
    qtbase5-dev \
    # LVGL / framebuffer
    libfreetype-dev libpng-dev libjpeg-dev libinput-dev libxkbcommon-dev libudev-dev \
    # Audio
    libasound2-dev \
    # Python (for py_compile and Python apps)
    python3 python3-pil python3-numpy \
    # Rust
    rustc cargo \
    # Misc utilities
    wget curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
CMD ["bash", "-c", "scripts/pack-all.sh --output-dir /src/dist"]
