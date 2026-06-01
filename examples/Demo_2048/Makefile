CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
TARGET = demo2048

$(TARGET): main.c font8x8_basic.h
	$(CC) $(CFLAGS) main.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
