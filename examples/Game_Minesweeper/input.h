#ifndef INPUT_H
#define INPUT_H

typedef enum {
    INPUT_NONE = 0,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_QUIT,
    INPUT_RESTART,
    INPUT_REVEAL,
    INPUT_FLAG
} input_key_t;

int input_open(void);
void input_close(int fd);
input_key_t input_poll(int fd);

#endif
