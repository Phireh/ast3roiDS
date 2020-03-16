#ifndef AST3ROIDS_H
#define AST3ROIDS_H

/* Includes */

#include <3ds.h>
#include <citro2d.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

/* Defines */

#define TOP_SCREEN_WIDTH    400
#define TOP_SCREEN_HEIGHT   240
#define MAX_BULLETS         32
#define WHITE               C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define RED                 C2D_Color32(0xFF, 0x00, 0x00, 0xFF)
#define GREEN               C2D_Color32(0x00, 0xFF, 0x00, 0xFF)
#define BLACK               C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define DPAD                (KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN)



// Utility macros
#define RANDF(a)            (float)rand()/(float)(RAND_MAX/(a));
#define ABS(a)              (((a)<0)?(-(a)):(a))

// Gameplay config macros
#define PLAYER_SAFE_ZONE_RADIUS 60.0f
#define ASTEROID_NUMBER         10
#define ASTEROID_MAXSPEED       0.5f
#define BULLET_INITIAL_SPEED    4.0f

// Debug macros
#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_INIT) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS)
#define CHECKDEBUGMODE      consoleInit(GFX_BOTTOM, NULL);
#else
#define CHECKDEBUGMODE      
#endif

#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS) && !defined(DEBUG_INIT)
#define PRINTFRAME          printf("FRAME %d:  ", framecount)
#else
#define PRINTFRAME
#endif

#ifdef DEBUG_INPUT
#define PRINTDINPUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDINPUT(fmt, ...) 
#endif

#ifdef DEBUG_LOGIC
#define PRINTDLOGIC(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDLOGIC(fmt, ...)
#endif

#ifdef DEBUG_RENDER
#define PRINTDRENDER(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDRENDER(fmt, ...)
#endif

#ifdef DEBUG_INIT
#define PRINTDINIT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDINIT(fmt, ...)
#endif

#ifdef DEBUG_COLLISION
#define PRINTDCOLLISION(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDCOLLISION(fmt, ...) 
#endif

#ifdef DEBUG_BULLETS
#define PRINTDBULLETS(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTDBULLETS(fmt, ...) 
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

typedef enum {
              SPRITE_PLAYER_NORMAL,       // 0
              SPRITE_PLAYER_BOOSTING,     // 1
              SPRITE_PLAYER_TOTAL         // 2
} player_spritesheet_idx_t;

typedef enum {
              SPRITE_BULLET_NORMAL,       // 0
              SPRITE_BULLET_TOTAL         // 1
} bullet_spritesheet_idx_t;

typedef struct player_ship_t {
  float x;
  float y;
  float xspeed;
  float yspeed;
  float angle;
  float radius;
  u32 color;
  float vertices[XY_TOTAL]; // relative to local coordinates
  C2D_Sprite sprites[SPRITE_PLAYER_TOTAL];
  unsigned int curr_sprite;
} player_ship_t;

typedef struct asteroid_t {
  float x;
  float y;
  float xspeed;
  float yspeed;
  float radius;
  u32 color;
} asteroid_t;

typedef struct bullet_t {
  float x;
  float y;
  float xspeed;
  float yspeed;
  float angle;
  C2D_Sprite *sprite;
} bullet_t;


/* Inline functions */

// Check if point is inside rectangle
inline int inside_rect(float x, float y, float leftx, float rightx, float downy, float upy)
{
  return x > 0 && y > 0 && rightx - x > leftx && upy - y > downy;
}

// Check if point is inside circle
inline int inside_circle(float x, float y, float cx, float cy, float crad)
{
  return sqrt((cx - x) * (cx - x) + (cy - y) * (cy -y)) < crad;
}

// Limits a number inside range [floor, ceiling]
inline float clampf(float f, float floor, float ceiling)
{
  return f < floor ? floor : (f > ceiling ? ceiling : f);
}

// Check if point is inside top screen
inline int inside_top_screen(float x, float y)
{
  return inside_rect(x, y, 0.0f, TOP_SCREEN_WIDTH, 0.0f, TOP_SCREEN_HEIGHT);
}

/* Functions */

void init_sprites(void);

void init_asteroid(asteroid_t *asteroid);
void asteroid_logic(asteroid_t *asteroid);
void draw_asteroid(asteroid_t *asteroid);

void init_player();
void player_logic();

void draw_player_sprite(void);
void draw_player_nosprite(void);

void shoot_bullet(void);
void bullet_logic(void);
void draw_bullets(void);

int process_input(u32 keys_down, u32 keys_held);

/* Function pointers */
void (*draw_player)(void) = draw_player_sprite;

#endif
