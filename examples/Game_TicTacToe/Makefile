CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -O2
PKGS     := sdl2 SDL2_ttf
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS   := $(shell pkg-config --libs $(PKGS))

TARGET := tictactoe
SRCS   := main.c
OBJS   := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(PKG_LIBS) -lm

%.o: %.c
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
