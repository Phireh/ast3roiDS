#ifndef AST3ROIDS_H
#define AST3ROIDS_H

/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <citro2d.h>
#include <math.h>

/* Defines */

#define WHITE C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define BLACK C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define DPAD (KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN)
#define PRINTFRAME printf("FRAME %d:  ", framecount)

// NOTE: This is a two-function-macro, do not mix with one-line ifs
#ifdef DEBUG_INPUT
#define PRINTDINPUT(fmt, ...) PRINTFRAME; printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDINPUT(fmt, ...) 
#endif

#ifdef DEBUG_LOGIC
#define PRINTDLOGIC(fmt, ...) PRINTFRAME; printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDLOGIC(fmt, ...)
#endif


/* Types */

typedef enum {
              X0,        // 0
              Y0,        // 1
              X1,        // 2
              Y1,        // 3
              X2,        // 4
              Y2,        // 5
              XY_TOTAL   // 6
} vert_idx_t;

typedef enum {
              NORMAL_INPUT,     // 0
              PAUSE_GAME_INPUT, // 1
              EXIT_GAME_INPUT,  // 2
              TOTAL_INPUT       // 3
} input_return_t;

typedef enum {
              NORMAL_GAMESTATE, // 0
              PAUSED_GAMESTATE, // 1
              TOTAL_GAMESTATE   // 2
} game_state_t;

typedef struct player_ship_t {
  float x;
  float y;
  float xspeed;
  float yspeed;
  float angle;
  float radius;
  u32 color;
  float vertices[XY_TOTAL]; // relative to local coordinates
} player_ship_t;


/* Functions */
void draw_player();
void init_player();
void player_logic();
int process_input(u32 keys_down, u32 keys_held);

#endif
