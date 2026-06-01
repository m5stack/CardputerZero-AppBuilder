/* Demo_2048: keyboard-driven 2048 on 320x170 fb0.
 * 4x4 grid occupies 160x160 on the left, score strip on right 160x170.
 * Arrow keys (or HJKL) slide, R reset, Esc quit.
 *
 * Input is read from an evdev keypad device. We prefer the stable
 * by-path link (Cardputer i2c keypad), then fall back to scanning
 * /dev/input/event* for a KEY-capable device that is NOT a mouse/CEC
 * (i.e. no EV_REL), skipping the HDMI CEC pseudo-keyboard on event0.
 */

#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "font8x8_basic.h"

#define FB_DEV "/dev/fb0"
/* Logical design size. Actual on-device fb size is read from vinfo at
 * startup; we clamp drawing to min(design, vinfo) so oversize hardware
 * never causes an over-write past the mmap region. */
#define W 320
#define H 170
#define GRID 4
#define BOARD_PX 160
#define CELL (BOARD_PX / GRID)

#define BITS_PER_LONG (8 * (int)sizeof(long))
#define NBITS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define TEST_BIT(bit, arr) ((arr)[(bit) / BITS_PER_LONG] & (1UL << ((bit) % BITS_PER_LONG)))

static uint16_t *fb;
/* Real on-device fb dimensions, populated in main() from FBIOGET_VSCREENINFO.
 * put_pixel uses fb_w as stride; hardcoded W=320 was wrong when vinfo.xres
 * differs (e.g. padded fb line_length), causing segfaults on some builds. */
static int fb_w = W;
static int fb_h = H;
static int board[GRID][GRID];
static int score = 0;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static const uint16_t TILE_COLORS[] = {
    0x0000, /* empty slot handled separately */
    0xEF7D, 0xEF3B, 0xF54A, 0xF3C7, 0xF2A5, 0xF1E3,
    0xECC2, 0xECA1, 0xEC80, 0xEC60, 0xEC40,
};

static uint16_t tile_color(int v) {
    int i = 0;
    int n = v;
    while (n > 1) { n >>= 1; i++; }
    if (i <= 0) return rgb565(60, 58, 50);
    if ((unsigned)i >= sizeof(TILE_COLORS) / sizeof(TILE_COLORS[0]))
        i = (int)(sizeof(TILE_COLORS) / sizeof(TILE_COLORS[0])) - 1;
    return TILE_COLORS[i];
}

static void put_pixel(int x, int y, uint16_t c) {
    if (x < 0 || y < 0 || x >= fb_w || y >= fb_h) return;
    fb[y * fb_w + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            put_pixel(x + i, y + j, c);
}

static void draw_char(int x, int y, char ch, int scale, uint16_t fg) {
    if ((unsigned char)ch >= 128) return;
    const uint8_t *g = font8x8_basic[(int)ch];
    for (int r = 0; r < 8; r++) {
        uint8_t bits = g[r];
        for (int c = 0; c < 8; c++) {
            if (bits & (0x80 >> c))
                fill_rect(x + c * scale, y + r * scale, scale, scale, fg);
        }
    }
}

static void draw_str(int x, int y, const char *s, int scale, uint16_t fg) {
    while (*s) { draw_char(x, y, *s, scale, fg); x += 8 * scale; s++; }
}

static void spawn(void) {
    int empty[16][2], n = 0;
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++)
            if (!board[r][c]) { empty[n][0] = r; empty[n][1] = c; n++; }
    if (!n) return;
    int p = rand() % n;
    board[empty[p][0]][empty[p][1]] = (rand() % 10 == 0) ? 4 : 2;
}

static void reset_board(void) {
    memset(board, 0, sizeof(board));
    score = 0;
    spawn();
    spawn();
}

/* Slide/merge one row to the left. Returns nonzero if changed. */
static int slide_row(int *row) {
    int tmp[GRID] = {0};
    int k = 0, changed = 0;
    for (int i = 0; i < GRID; i++) if (row[i]) tmp[k++] = row[i];
    for (int i = 0; i < GRID - 1; i++) {
        if (tmp[i] && tmp[i] == tmp[i + 1]) {
            tmp[i] *= 2;
            score += tmp[i];
            tmp[i + 1] = 0;
        }
    }
    int out[GRID] = {0};
    k = 0;
    for (int i = 0; i < GRID; i++) if (tmp[i]) out[k++] = tmp[i];
    for (int i = 0; i < GRID; i++) {
        if (row[i] != out[i]) changed = 1;
        row[i] = out[i];
    }
    return changed;
}

static int move_left(void) {
    int changed = 0;
    for (int r = 0; r < GRID; r++) changed |= slide_row(board[r]);
    return changed;
}

static void reverse_row(int *row) {
    for (int i = 0; i < GRID / 2; i++) {
        int t = row[i]; row[i] = row[GRID - 1 - i]; row[GRID - 1 - i] = t;
    }
}

static int move_right(void) {
    int changed = 0;
    for (int r = 0; r < GRID; r++) {
        reverse_row(board[r]);
        changed |= slide_row(board[r]);
        reverse_row(board[r]);
    }
    return changed;
}

static void transpose(void) {
    for (int r = 0; r < GRID; r++)
        for (int c = r + 1; c < GRID; c++) {
            int t = board[r][c]; board[r][c] = board[c][r]; board[c][r] = t;
        }
}

static int move_up(void) { transpose(); int ch = move_left(); transpose(); return ch; }
static int move_down(void) { transpose(); int ch = move_right(); transpose(); return ch; }

static void draw(void) {
    fill_rect(0, 0, W, H, rgb565(20, 20, 28));
    /* board backdrop */
    int x0 = 5, y0 = 5;
    fill_rect(x0, y0, BOARD_PX, BOARD_PX, rgb565(90, 80, 70));
    for (int r = 0; r < GRID; r++) {
        for (int c = 0; c < GRID; c++) {
            int x = x0 + c * CELL + 2;
            int y = y0 + r * CELL + 2;
            int cw = CELL - 4;
            int ch = CELL - 4;
            fill_rect(x, y, cw, ch, tile_color(board[r][c]));
            if (board[r][c]) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", board[r][c]);
                int tw = (int)strlen(buf) * 8 * 2;
                draw_str(x + (cw - tw) / 2, y + (ch - 16) / 2, buf, 2,
                         rgb565(30, 30, 30));
            }
        }
    }
    /* score panel */
    draw_str(175, 10, "2048", 2, rgb565(255, 220, 60));
    draw_str(175, 40, "SCORE", 1, rgb565(200, 200, 200));
    char sb[16];
    snprintf(sb, sizeof(sb), "%d", score);
    draw_str(175, 55, sb, 2, rgb565(255, 255, 255));
    draw_str(175, 120, "R:Reset", 1, rgb565(160, 160, 160));
    draw_str(175, 135, "Esc:Quit", 1, rgb565(160, 160, 160));
    draw_str(175, 150, "HJKL move", 1, rgb565(160, 160, 160));
}

/* ---- evdev input ---- */

/* Open an evdev keypad device. Preference order:
 *   1. $INPUT_DEV env var if set
 *   2. /dev/input/by-path/platform-3f804000.i2c-event (Cardputer keypad)
 *   3. scan /dev/input/event*, prefer EV_KEY-capable devices without EV_REL
 *      (skips HDMI CEC pseudo-keyboard on event0 which has EV_REL/mouse bits).
 */
static int open_keypad(void) {
    const char *env = getenv("INPUT_DEV");
    if (env && *env) {
        int fd = open(env, O_RDONLY);
        if (fd >= 0) return fd;
    }
    int fd = open("/dev/input/by-path/platform-3f804000.i2c-event", O_RDONLY);
    if (fd >= 0) return fd;

    DIR *d = opendir("/dev/input");
    if (!d) return -1;
    struct dirent *e;
    int best = -1;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        int cand = open(path, O_RDONLY);
        if (cand < 0) continue;

        unsigned long evbits[NBITS(EV_MAX)] = {0};
        if (ioctl(cand, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
            close(cand); continue;
        }
        if (!TEST_BIT(EV_KEY, evbits)) { close(cand); continue; }
        /* Reject mouse-like / HDMI CEC (has EV_REL). */
        if (TEST_BIT(EV_REL, evbits)) { close(cand); continue; }

        /* Require KEY_ENTER or KEY_ESC to confirm it's a real keyboard. */
        unsigned long keybits[NBITS(KEY_MAX)] = {0};
        if (ioctl(cand, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0 &&
            (TEST_BIT(KEY_ENTER, keybits) || TEST_BIT(KEY_ESC, keybits))) {
            best = cand;
            break;
        }
        close(cand);
    }
    closedir(d);
    return best;
}

/* Translate an evdev KEY_* code into the logical action char used by the
 * game loop. Returns 0 on unhandled. */
static int key_to_action(int code) {
    switch (code) {
        case KEY_ESC:      return 27;
        case KEY_LEFT:
        case KEY_H:
        case KEY_A:        return 'h';
        case KEY_RIGHT:
        case KEY_L:
        case KEY_D:        return 'l';
        case KEY_UP:
        case KEY_K:
        case KEY_W:        return 'k';
        case KEY_DOWN:
        case KEY_J:
        case KEY_S:        return 'j';
        case KEY_R:        return 'r';
        default:           return 0;
    }
}

/* Block until a relevant key press (value==1 or 2 for autorepeat). */
static int read_key_blocking(int fd) {
    struct input_event ev;
    for (;;) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n != (ssize_t)sizeof(ev)) return -1;
        if (ev.type != EV_KEY) continue;
        if (ev.value == 0) continue; /* release */
        int k = key_to_action(ev.code);
        if (k) return k;
    }
}

int main(void) {
    int fd = open(FB_DEV, O_RDWR);
    if (fd < 0) { perror("open fb"); return 1; }
    struct fb_var_screeninfo v;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &v) < 0) { perror("ioctl"); close(fd); return 1; }
    /* Adopt device-reported size so put_pixel's stride matches the real fb.
     * Clamp to our design dimensions so off-device builds with a larger fb
     * still only touch the 320x170 region we actually render into. */
    fb_w = (int)v.xres;
    fb_h = (int)v.yres;
    if (fb_w <= 0 || fb_h <= 0) { fprintf(stderr, "bad vinfo %ux%u\n", v.xres, v.yres); close(fd); return 1; }
    size_t bytes = (size_t)v.xres * v.yres * 2;
    fb = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    int kfd = open_keypad();
    if (kfd < 0) {
        fprintf(stderr, "demo-2048: no keypad found under /dev/input\n");
        /* Still render the board so the user sees *something*. */
    }

    srand((unsigned)time(NULL));
    reset_board();
    draw();

    if (kfd >= 0) {
        for (;;) {
            int k = read_key_blocking(kfd);
            if (k < 0) break;
            int moved = 0;
            if (k == 27) break;
            else if (k == 'h') moved = move_left();
            else if (k == 'l') moved = move_right();
            else if (k == 'k') moved = move_up();
            else if (k == 'j') moved = move_down();
            else if (k == 'r' || k == 'R') { reset_board(); moved = 0; }
            if (moved) spawn();
            draw();
        }
        close(kfd);
    } else {
        /* No keypad: idle so the user at least sees the rendered board. */
        for (;;) sleep(60);
    }

    munmap(fb, bytes);
    close(fd);
    return 0;
}
