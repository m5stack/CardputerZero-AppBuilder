CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c11
TARGET   = hello_fb
SRC      = main.c

.PHONY: all clean run deploy

all: $(TARGET)

$(TARGET): $(SRC) font8x8_basic.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

deploy: $(TARGET)
	scp $(TARGET) pi@192.168.50.150:~/
	ssh pi@192.168.50.150 ./$(TARGET)

clean:
	rm -f $(TARGET)
