CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=gnu11
# Add -lfftw3f once FFT is wired in.
LDLIBS = -lasound -lm
TARGET = spectrum

$(TARGET): skeleton.c font8x8_basic.h
	$(CC) $(CFLAGS) skeleton.c -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
