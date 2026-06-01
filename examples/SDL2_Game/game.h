#ifndef CPZ_GAME_H
#define CPZ_GAME_H

#include <SDL2/SDL.h>
#include <stdbool.h>

#define SCREEN_W     320
#define SCREEN_H     170
#define PADDLE_W     40
#define PADDLE_H     4
#define BALL_SIZE    4
#define BRICK_ROWS   5
#define BRICK_COLS   8
#define BRICK_W      38
#define BRICK_H      8
#define BRICK_GAP_X  2
#define BRICK_GAP_Y  2
#define BRICK_OFF_X  4
#define BRICK_OFF_Y  20
#define START_LIVES  3

typedef struct {
    SDL_FRect rect;
    bool alive;
    int color;
} Brick;

typedef enum {
    STATE_READY,
    STATE_PLAYING,
    STATE_LOST,
    STATE_WON
} GameStateKind;

typedef struct {
    SDL_FRect paddle;
    SDL_FRect ball;
    float ball_vx;
    float ball_vy;
    Brick bricks[BRICK_ROWS * BRICK_COLS];
    int lives;
    int score;
    GameStateKind state;
    bool key_left;
    bool key_right;
} GameState;

typedef struct {
    int bounce;
    int brick;
} SoundEvents;

void game_init(GameState *g);
void game_reset_ball(GameState *g);
void game_restart(GameState *g);
void game_handle_key(GameState *g, SDL_Keycode key, bool down);
void game_update(GameState *g, float dt, SoundEvents *sfx);

#endif
