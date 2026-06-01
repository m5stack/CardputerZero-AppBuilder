#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH            16
#define LV_COLOR_16_SWAP          0

#define LV_HOR_RES_MAX            320
#define LV_VER_RES_MAX            170

#define LV_MEM_SIZE               (64U * 1024U)

#define LV_USE_STDLIB_MALLOC      LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING      LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF     LV_STDLIB_BUILTIN

#define LV_TICK_CUSTOM            1
#define LV_TICK_CUSTOM_INCLUDE    "time.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR \
    ((uint32_t)({ struct timespec _ts; clock_gettime(CLOCK_MONOTONIC, &_ts); \
                  (_ts.tv_sec * 1000U) + (_ts.tv_nsec / 1000000U); }))

#define LV_DEF_REFR_PERIOD        16

/* Disable Helium/ARM SIMD assembly backend; upstream .S file fails to
 * assemble on ubuntu-24.04-arm's stock binutils. Plain C path is fine. */
#define LV_USE_DRAW_SW_ASM        LV_DRAW_SW_ASM_NONE

#define LV_USE_LOG                0

#ifndef LV_USE_LINUX_FBDEV
#define LV_USE_LINUX_FBDEV        1
#endif
#define LV_LINUX_FBDEV_BSD        0

/* v9.2.2 renamed the evdev driver guard to LV_USE_EVDEV (non-linux prefix). */
#ifndef LV_USE_EVDEV
#define LV_USE_EVDEV              1
#endif

#define LV_FONT_MONTSERRAT_12     1
#define LV_FONT_MONTSERRAT_14     1
#define LV_FONT_DEFAULT           &lv_font_montserrat_14

#define LV_USE_DEMO_WIDGETS       0
#define LV_USE_DEMO_BENCHMARK     0
#define LV_USE_DEMO_STRESS        0
#define LV_USE_DEMO_MUSIC         0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

#endif /* LV_CONF_H */
