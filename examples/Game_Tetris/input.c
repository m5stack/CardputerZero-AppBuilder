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

input_key_t input_poll(int fd) {
    if (fd < 0) return INPUT_NONE;
    struct input_event ev;
    ssize_t n;
    input_key_t last = INPUT_NONE;
    while ((n = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
        if (ev.type != EV_KEY || ev.value == 0) continue;
        switch (ev.code) {
            case KEY_UP:      last = INPUT_ROTATE; break;
            case KEY_W:       last = INPUT_ROTATE; break;
            case KEY_DOWN:
            case KEY_S:       last = INPUT_DOWN; break;
            case KEY_LEFT:
            case KEY_A:       last = INPUT_LEFT; break;
            case KEY_RIGHT:
            case KEY_D:       last = INPUT_RIGHT; break;
            case KEY_SPACE:   last = INPUT_DROP; break;
            case KEY_ESC:
            case KEY_Q:       last = INPUT_QUIT; break;
            case KEY_R:       last = INPUT_RESTART; break;
            case KEY_ENTER:   last = INPUT_ROTATE; break;
            default: break;
        }
    }
    (void)n;
    (void)errno;
    return last;
}
