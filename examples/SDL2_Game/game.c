#include "game.h"
#include <string.h>

static void build_bricks(GameState *g) {
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &g->bricks[r * BRICK_COLS + c];
            b->rect.x = BRICK_OFF_X + c * (BRICK_W + BRICK_GAP_X);
            b->rect.y = BRICK_OFF_Y + r * (BRICK_H + BRICK_GAP_Y);
            b->rect.w = BRICK_W;
            b->rect.h = BRICK_H;
            b->alive = true;
            b->color = r;
        }
    }
}

void game_reset_ball(GameState *g) {
    g->ball.w = BALL_SIZE;
    g->ball.h = BALL_SIZE;
    g->ball.x = g->paddle.x + (PADDLE_W - BALL_SIZE) / 2.0f;
    g->ball.y = g->paddle.y - BALL_SIZE - 1;
    g->ball_vx = 0;
    g->ball_vy = 0;
    g->state = STATE_READY;
}

void game_init(GameState *g) {
    memset(g, 0, sizeof(*g));
    g->paddle.w = PADDLE_W;
    g->paddle.h = PADDLE_H;
    g->paddle.x = (SCREEN_W - PADDLE_W) / 2.0f;
    g->paddle.y = SCREEN_H - PADDLE_H - 4;
    g->lives = START_LIVES;
    g->score = 0;
    build_bricks(g);
    game_reset_ball(g);
}

void game_restart(GameState *g) {
    game_init(g);
}

void game_handle_key(GameState *g, SDL_Keycode key, bool down) {
    if (key == SDLK_LEFT)  g->key_left  = down;
    if (key == SDLK_RIGHT) g->key_right = down;
    if (down && key == SDLK_SPACE && g->state == STATE_READY) {
        g->ball_vx = 80.0f;
        g->ball_vy = -110.0f;
        g->state = STATE_PLAYING;
    }
}

static bool rect_overlap(const SDL_FRect *a, const SDL_FRect *b) {
    return !(a->x + a->w <= b->x ||
             b->x + b->w <= a->x ||
             a->y + a->h <= b->y ||
             b->y + b->h <= a->y);
}

static int count_alive(const GameState *g) {
    int n = 0;
    for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++)
        if (g->bricks[i].alive) n++;
    return n;
}

void game_update(GameState *g, float dt, SoundEvents *sfx) {
    const float paddle_speed = 180.0f;
    if (g->key_left)  g->paddle.x -= paddle_speed * dt;
    if (g->key_right) g->paddle.x += paddle_speed * dt;
    if (g->paddle.x < 0) g->paddle.x = 0;
    if (g->paddle.x + PADDLE_W > SCREEN_W) g->paddle.x = SCREEN_W - PADDLE_W;

    if (g->state == STATE_READY) {
        g->ball.x = g->paddle.x + (PADDLE_W - BALL_SIZE) / 2.0f;
        g->ball.y = g->paddle.y - BALL_SIZE - 1;
        return;
    }
    if (g->state != STATE_PLAYING) return;

    float nx = g->ball.x + g->ball_vx * dt;
    float ny = g->ball.y + g->ball_vy * dt;

    if (nx < 0) { nx = 0; g->ball_vx = -g->ball_vx; sfx->bounce++; }
    if (nx + BALL_SIZE > SCREEN_W) { nx = SCREEN_W - BALL_SIZE; g->ball_vx = -g->ball_vx; sfx->bounce++; }
    if (ny < 0) { ny = 0; g->ball_vy = -g->ball_vy; sfx->bounce++; }

    g->ball.x = nx;
    g->ball.y = ny;

    if (g->ball.y + BALL_SIZE > SCREEN_H) {
        g->lives--;
        if (g->lives <= 0) {
            g->state = STATE_LOST;
        } else {
            game_reset_ball(g);
        }
        return;
    }

    if (rect_overlap(&g->ball, &g->paddle) && g->ball_vy > 0) {
        g->ball.y = g->paddle.y - BALL_SIZE;
        g->ball_vy = -g->ball_vy;
        float hit = (g->ball.x + BALL_SIZE * 0.5f) - (g->paddle.x + PADDLE_W * 0.5f);
        g->ball_vx += hit * 2.0f;
        if (g->ball_vx >  160.0f) g->ball_vx =  160.0f;
        if (g->ball_vx < -160.0f) g->ball_vx = -160.0f;
        sfx->bounce++;
    }

    for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {
        Brick *b = &g->bricks[i];
        if (!b->alive) continue;
        if (!rect_overlap(&g->ball, &b->rect)) continue;

        float bcx = b->rect.x + b->rect.w * 0.5f;
        float bcy = b->rect.y + b->rect.h * 0.5f;
        float ball_cx = g->ball.x + BALL_SIZE * 0.5f;
        float ball_cy = g->ball.y + BALL_SIZE * 0.5f;
        float dx = ball_cx - bcx;
        float dy = ball_cy - bcy;
        if ((dx < 0 ? -dx : dx) * b->rect.h > (dy < 0 ? -dy : dy) * b->rect.w)
            g->ball_vx = -g->ball_vx;
        else
            g->ball_vy = -g->ball_vy;

        b->alive = false;
        g->score += 10;
        sfx->brick++;
        break;
    }

    if (count_alive(g) == 0) g->state = STATE_WON;
}
