#define _POSIX_C_SOURCE 200809L
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>

int input_open(void) {
    const char *env = getenv("INPUT_DEV");
    if (env && *env) {
        int fd = open(env, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) return fd;
    }
    DIR *d = opendir("/dev/input");
    if (!d) return -1;
    struct dirent *e;
    int best = -1;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        unsigned long evbit = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) >= 0 && (evbit & (1UL << EV_KEY))) {
            best = fd;
            break;
        }
        close(fd);
    }
    closedir(d);
    return best;
}

void input_close(int fd) {
    if (fd >= 0) close(fd);
}

static uint32_t bit_for_code(uint16_t code) {
    switch (code) {
        case KEY_UP:    return KBIT_UP;
        case KEY_W:     return KBIT_UP;
        case KEY_DOWN:  return KBIT_DOWN;
        case KEY_S:     return KBIT_DOWN;
        case KEY_LEFT:  return KBIT_LEFT;
        case KEY_A:     return KBIT_LEFT;
        case KEY_RIGHT: return KBIT_RIGHT;
        case KEY_D:     return KBIT_RIGHT;
        case KEY_SPACE: return KBIT_SPACE;
        case KEY_ENTER: return KBIT_SPACE;
        case KEY_R:     return KBIT_R;
        case KEY_ESC:   return KBIT_ESC;
        case KEY_Q:     return KBIT_ESC;
        default:        return 0;
    }
}

void input_update(int fd, input_state_t *st) {
    uint32_t prev = st->held;
    st->pressed = 0;
    st->released = 0;
    if (fd < 0) return;
    struct input_event ev;
    ssize_t n;
    while ((n = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
        if (ev.type != EV_KEY) continue;
        uint32_t b = bit_for_code(ev.code);
        if (!b) continue;
        if (ev.value == 1) {
            st->held |= b;
        } else if (ev.value == 0) {
            st->held &= ~b;
        }
    }
    (void)n;
    (void)errno;
    st->pressed  = st->held & ~prev;
    st->released = prev & ~st->held;
}
