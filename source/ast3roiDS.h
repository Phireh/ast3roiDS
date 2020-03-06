#ifndef AST3ROIDS_H
#define AST3ROIDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <citro2d.h>
#include <math.h>

#define WHITE C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define BLACK C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define DPAD (KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN)
#define PRINTFRAME printf("FRAME %d:  ", framecount)

// NOTE: This is a two-function-macro, do not mix with one-line ifs
#ifdef DEBUG_INPUT
#define PRINTDINPUT(x) PRINTFRAME; printf(x)
#else
#define PRINTDINPUT(x)
#endif

typedef enum {
              X0,        // 0
              Y0,        // 1
              X1,        // 2
              Y1,        // 3
              X2,        // 4
              Y2,        // 5
              XY_TOTAL   // 6
} vert_idx_t;

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



#endif
