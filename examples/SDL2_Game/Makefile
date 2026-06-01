CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -O2
PKGS     := sdl2 SDL2_ttf SDL2_mixer
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS   := $(shell pkg-config --libs $(PKGS))

TARGET := breakout
SRCS   := main.c game.c render.c
OBJS   := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(PKG_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -c $< -o $@

main.o:   main.c game.h render.h
game.o:   game.c game.h
render.o: render.c render.h game.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
