#ifndef AST3ROIDS_H
#define AST3ROIDS_H

/* Includes */

#include <3ds.h>
#include <citro2d.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

/* Internal includes */
#include "ast3roiDS_math.h"
#include "ast3roiDS_debug.h"

/* STB library initialization */
#define STB_SPRINTF_IMPLEMENTATION
#include "stb/stb_sprintf.h"
// NOTE(David): Commented until we use this to clean unused var warnings
// #include "stb/stretchy_buffer.h"

/* Defines */

#define TOP_SCREEN_WIDTH       400
#define TOP_SCREEN_HEIGHT      240
#define BOTTOM_SCREEN_WIDTH    320
#define BOTTOM_SCREEN_HEIGHT   240

#define MAX_BULLETS              32
#define MAX_ASTEROIDS            32
#define MAX_ENEMY_SHIPS          32
#define MAX_PICKUPS              32
#define SCORE_TEXT_LENGTH        64
#define ASTEROID_LOOT_TABLE_SIZE 8
#define WHITE                    C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define RED                      C2D_Color32(0xFF, 0x00, 0x00, 0xFF)
#define GREEN                    C2D_Color32(0x00, 0xFF, 0x00, 0xFF)
#define BLACK                    C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define DPAD                     (KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN)
#define TODO_CHANGEME            0

// Gameplay config macros
#define PLAYER_SAFE_ZONE_RADIUS 60.0f
#define PLAYER_STARTING_HP      3
#define ASTEROID_NUMBER         10
#define ASTEROID_MAXSPEED       0.5f
#define BULLET_INITIAL_SPEED    4.0f
#define MAX_ASTEROID_SIZE       40.0f
#define ASTEROID_BIG_RATIO      0.8f
#define ASTEROID_NORMAL_RATIO   0.5f
#define ASTEROID_SMALL_RATIO    0.2f
#define GRACE_PERIOD_AFTER_HIT  30
#define ASTEROID_SMALL_SCORE    50
#define ASTEROID_NORMAL_SCORE   100
#define ASTEROID_BIG_SCORE      200

// Animation config macros
#define PICKUP_HP_NSPRITES      6
#define PICKUP_HP_ANIM_SPEED    10


typedef enum {
              NORMAL_INPUT,        // 0
              PAUSE_GAME_INPUT,    // 1
              EXIT_GAME_INPUT,     // 2
#ifndef DEBUG_MODE
              TOTAL_INPUT,         // 3

#else
              DEBUG_ENEMIES_INPUT, // 3
              TOTAL_INPUT,         // 4
#endif
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

typedef enum {
              SPRITE_BACKGROUND_STATIC,   // 0
              SPRITE_BACKGROUND_TOTAL     // 1
} background_spritesheet_idx_t;

typedef enum {
              SPRITE_ASTEROID_SMALL,      // 0
              SPRITE_ASTEROID_NORMAL,     // 1
              SPRITE_ASTEROID_BIG,        // 2
              SPRITE_ASTEROID_TOTAL
} asteroid_spritesheet_idx_t;

typedef enum {
              ASTEROID_SIZE_SMALL,        // 0
              ASTEROID_SIZE_NORMAL,       // 1
              ASTEROID_SIZE_BIG,          // 2
              ASTEROID_SIZE_TOTAL         // 3
} asteroid_size_t;

typedef enum {
              SPRITE_ENEMY_NORMAL,        // 0
              SPRITE_ENEMY_BOOSTING,      // 1
              SPRITE_ENEMY_TOTAL          // 2
} enemy_spritesheet_idx_t;

typedef enum {
              SPRITE_PICKUP_HP_F0,        // 0: frame 0 of animation
              SPRITE_PICKUP_HP_F1,        // 1: frame 1 of animation
              SPRITE_PICKUP_HP_F2,        // 2: frame 2 of animation
              SPRITE_PICKUP_HP_F3,        // 3: frame 3 of animation
              SPRITE_PICKUP_HP_F4,        // 4: frame 4 of animation
              SPRITE_PICKUP_HP_F5,        // 5: frame 5 of animation
              SPRITE_PICKUP_HP_TOTAL,     // 6
} pickup_hp_spritesheet_idx_t;

typedef enum {
              ENEMY_STATE_INACTIVE,       // 0 
              ENEMY_STATE_ACTIVE,         // 1
              ENEMY_STATE_TOTAL           // 2
} enemy_state_t;

typedef enum {
              PICKUP_STATE_INACTIVE,       // 0 
              PICKUP_STATE_ACTIVE,         // 1
              PICKUP_STATE_TOTAL           // 2
} pickup_state_t;

typedef enum {
              LOOT_TABLE_IDX_NOTHING,     // 0: Get nothing
              LOOT_TABLE_IDX_1,           // 1: First item
              LOOT_TABLE_IDX_2,           // 2: Second item
              LOOT_TABLE_IDX_3,           // 3: Third item
              LOOT_TABLE_IDX_4,           // 4: Fourth item
              LOOT_TABLE_IDX_5,           // 5: Fifth item
              LOOT_TABLE_IDX_6,           // 6: Sixth item
              LOOT_TABLE_IDX_7,           // 7: Seventh item
              LOOT_TABLE_IDX_TOTAL        // 8
              
} loot_table_idx_t;

typedef enum { // Possible values for the result of a loot table dispatch
              NOTHING,      // 0
              PICKUP_BOMB,  // 1
              PICKUP_HP,    // 2
              EXTRA_SCORE,  // 3
} loot_table_item_t;

/* Basic loot table.
 * Usage: fill probabilities[] with growing values between 0.0f and 1.0f,
 *        probabilities[0] represents the prob of nothing happening.
 *        the probability of something else happening is prob[n] - prob[n-1].
 *        This is meant to be used with dispatch_loot_table and a switch statement
 *        to capture the output value.
 * For example: prob[0] = 0.5f, prob[1] = 0.7f, prob[2] = 1.0f means:
 *              50% of nothing, 20% of getting item 1, 30% of getting item 2
 */
typedef struct loot_table_t
{
  float probabilities[LOOT_TABLE_IDX_TOTAL]; // holds probability of item n in range [0,1]
  int items[LOOT_TABLE_IDX_TOTAL];           // holds identifier of item n
} loot_table_t;

typedef struct pickup_t
{
  union {
    struct {
      float x;
      float y;
    };
    vec2f pos;
    float p[1];
  };
  union {
    struct {
      float xspeed;
      float yspeed;
    };
    vec2f speed;
    float s[1];
  };
  float radius;
  u32 color;
  int type;
  int state;
  
  int anim_speed;
  int nsprites;
  int curr_sprite;
  C2D_Sprite *sprites;
} pickup_t;

typedef struct player_ship_t {
  union {
    struct {
      float x;
      float y;
    };
    vec2f pos;
    float p[1];
  };
  
  union {
    struct {
      float xspeed;
      float yspeed;
    };
    vec2f speed;
    float s[1];
  };
  
  float angle;
  float radius;
  int health;
  u32 color;
  union {
    struct {
      vec2f v1;
      vec2f v2;
      vec2f v3;
    };
    float vertices[XY_TOTAL]; // relative to local coordinates
  };
  C2D_Sprite sprites[SPRITE_PLAYER_TOTAL];
  unsigned int curr_sprite;
} player_ship_t;

typedef struct enemy_ship_t {
    union {
    struct {
      float x;
      float y;
    };
    vec2f pos;
    float p[1];
  };
  
  union {
    struct {
      float xspeed;
      float yspeed;
    };
    vec2f speed;
    float s[1];
  };
  
  float angle;
  float radius;
  float turnrate;
  int health;
  enemy_state_t state;
  loot_table_t loot_table;
  u32 color;
  union {
    struct {
      vec2f v1;
      vec2f v2;
      vec2f v3;
    };
    float vertices[XY_TOTAL]; // relative to local coordinates
  };
  C2D_Sprite sprites[SPRITE_ENEMY_TOTAL];
  unsigned int curr_sprite;
} enemy_ship_t;

typedef struct asteroid_t {
    union {
    struct {
      float x;
      float y;
    };
    vec2f pos;
    float p[1];
  };
  
  union {
    struct {
      float xspeed;
      float yspeed;
    };
    vec2f speed;
    float s[1];
  };
  float angle;
  float rotspeed;
  float radius;
  loot_table_t loot_table;
  u32 color;
} asteroid_t;

typedef struct bullet_t {
    union {
    struct {
      float x;
      float y;
    };
    vec2f pos;
    float p[1];
  };

    union {
    struct {
      float xspeed;
      float yspeed;
    };
    vec2f speed;
    float s[1];
  };
  
  float angle;
  C2D_Sprite *sprite;
} bullet_t;


inline int asteroid_size(float radius)
{
  float relative_size = radius/MAX_ASTEROID_SIZE;
  if      (relative_size >= ASTEROID_BIG_RATIO)    return ASTEROID_SIZE_BIG;
  else if (relative_size >= ASTEROID_NORMAL_RATIO) return ASTEROID_SIZE_NORMAL;
  else                                             return ASTEROID_SIZE_SMALL;
}

inline int dispatch_loot_table(loot_table_t table)
{
  float n = randf2(0.0f, 1.0f); // Get our probability between 0% and 100%
  if (table.probabilities[LOOT_TABLE_IDX_NOTHING] > n) return NOTHING;
  for (int i = LOOT_TABLE_IDX_1; i < LOOT_TABLE_IDX_TOTAL; ++i) {
    if (table.probabilities[i] > n) return table.items[i];
  }
  return NOTHING; // if loot table isn't set correctly and we didn't find the item
}


extern inline void print_in_rect(char *s, size_t n, float x, float y, float scale)
{
  char buf[n];
  sprintf(buf, "%s", s);
  C2D_TextBuf text_buf = C2D_TextBufNew(n);
  C2D_Text text;
  C2D_TextParse(&text, text_buf, buf);
  float w, h;
  C2D_TextGetDimensions(&text, scale, scale, &w, &h);
  C2D_DrawRectSolid(x-1.0f, y-1.0f, 0.9f, w+2.0f, h+2.0f, WHITE);
  C2D_DrawRectSolid(x, y, 0.9f, w, h, BLACK);
  C2D_DrawText(&text, C2D_WithColor, x, y, 1.0f, scale, scale, WHITE);
}

// Print a rectangle of color c1, with border of size b and color c2 at profundity p
extern inline void print_rect_border(rect r, float b, float p, u32 c1, u32 c2)
{
  C2D_DrawRectSolid(r.x - b, r.y - b, p, r.w + b*2, r.h + b*2, c2);
  C2D_DrawRectSolid(r.x , r.y, p, r.w, r.h, c1);
}

// Check if point is inside top screen
inline int inside_top_screen(float x, float y)
{
  return inside_rect(x, y, 0.0f, TOP_SCREEN_WIDTH, 0.0f, TOP_SCREEN_HEIGHT);
}

/* Functions */

void init_sprites(void);

void init_asteroids(int n);
void asteroid_logic();
void spawn_asteroids(float x, float y, asteroid_size_t size, int n);
void break_asteroid(asteroid_t *asteroid, int idx);

void init_player();
void player_logic();

enemy_ship_t spawn_enemy_ship(float x, float y, float xs, float ys, float r, u32 color);
void draw_enemy_ship_sprite(enemy_ship_t *enemy);
void draw_enemy_ship_nosprite(enemy_ship_t *enemy);
void enemy_ship_logic(enemy_ship_t *enemy);

void draw_player_sprite(void);
void draw_player_nosprite(void);

void draw_asteroids_sprite(void);
void draw_asteroids_nosprite(void);

void shoot_bullet(void);
void bullet_logic(void);
void draw_bullets(void);

pickup_t spawn_pickup(int type, float x, float y, float xs, float ys, float r, u32 color);
void pickup_logic(pickup_t *pickup);
void draw_pickup(pickup_t *pickup);

void draw_background_static(C2D_Sprite *background);

int process_input(u32 keys_down, u32 keys_held);
void reset_game(void);
void draw_score(void);

/* Function pointers. */
void (*draw_player)    (void)                = draw_player_sprite;
void (*draw_asteroids) (void)                = draw_asteroids_sprite;
void (*draw_enemy_ship)(enemy_ship_t *enemy) = draw_enemy_ship_sprite;
#endif
