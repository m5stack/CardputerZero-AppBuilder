#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

enum {
    KBIT_UP    = 1u << 0,
    KBIT_DOWN  = 1u << 1,
    KBIT_LEFT  = 1u << 2,
    KBIT_RIGHT = 1u << 3,
    KBIT_SPACE = 1u << 4,
    KBIT_R     = 1u << 5,
    KBIT_ESC   = 1u << 6,
};

typedef struct {
    uint32_t held;
    uint32_t pressed;
    uint32_t released;
} input_state_t;

int  input_open(void);
void input_close(int fd);
void input_update(int fd, input_state_t *st);

#endif
