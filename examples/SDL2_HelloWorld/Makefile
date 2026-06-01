CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -O2
PKGS     := sdl2 SDL2_ttf SDL2_mixer
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS   := $(shell pkg-config --libs $(PKGS))

TARGET := hello_sdl2
SRC    := main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(SRC) -o $(TARGET) $(PKG_LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
