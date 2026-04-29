FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    python3 \
    python3-pip \
    dpkg-dev \
    git \
    make \
    libfreetype-dev \
    libpng-dev \
    libinput-dev \
    libxkbcommon-dev \
    libudev-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir scons parse requests tqdm

WORKDIR /workspace
