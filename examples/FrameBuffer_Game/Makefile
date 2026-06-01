CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c11
LDLIBS   = -lm
TARGET   = snake_fb
OBJS     = main.o fb.o input.o

.PHONY: all clean run deploy

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDLIBS)

main.o: main.c fb.h input.h
fb.o:   fb.c   fb.h font8x8.h
input.o: input.c input.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

deploy: $(TARGET)
	scp $(TARGET) pi@192.168.50.150:~/
	ssh pi@192.168.50.150 ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
