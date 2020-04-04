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
#define MAX_ASTEROIDS       32
#define SCORE_TEXT_LENGTH   64
#define WHITE               C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define RED                 C2D_Color32(0xFF, 0x00, 0x00, 0xFF)
#define GREEN               C2D_Color32(0x00, 0xFF, 0x00, 0xFF)
#define BLACK               C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define DPAD                (KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN)
#define TODO_CHANGEME       0



// Utility macros
#define RANDF(a)            (float)rand()/(float)(RAND_MAX/(a));
#define ABS(a)              (((a)<0)?(-(a)):(a))
#define NORM(a,b)       ((a)/(b))

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


// Debug macros
// TODO: make TODO macros print to submenu
#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_INIT) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS)
#define CHECKDEBUGMODE
#define DEBUG_MODE 1
#else
#define CHECKDEBUGMODE
#endif

#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS) && !defined(DEBUG_INIT)
#define PRINTFRAME          
#else
#define PRINTFRAME
#endif

#ifdef DEBUG_INPUT
#define PRINTDINPUT(fmt, ...) 
#else
#define PRINTDINPUT(fmt, ...) 
#endif

#ifdef DEBUG_LOGIC
#define PRINTDLOGIC(fmt, ...) 
#else
#define PRINTDLOGIC(fmt, ...)
#endif

#ifdef DEBUG_RENDER
#define PRINTDRENDER(fmt, ...)
#else
#define PRINTDRENDER(fmt, ...)
#endif

#ifdef DEBUG_INIT
#define PRINTDINIT(fmt, ...) 
#else
#define PRINTDINIT(fmt, ...)
#endif

#ifdef DEBUG_COLLISION
#define PRINTDCOLLISION(fmt, ...)
#else
#define PRINTDCOLLISION(fmt, ...) 
#endif

#ifdef DEBUG_BULLETS
#define PRINTDBULLETS(fmt, ...)
#else
#define PRINTDBULLETS(fmt, ...) 
#endif


/* Types */

typedef struct {
  float x;
  float y;
} vec2f;

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
              ENEMY_STATE_INACTIVE,       // 0 
              ENEMY_STATE_ACTIVE,         // 1
              ENEMY_STATE_TOTAL           // 2
} enemy_state_t;

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
  u32 color;
  float vertices[XY_TOTAL]; // relative to local coordinates
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

  float radius;
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


/* Inline functions */

// Calculate the normal of 2D vector
inline float norm_2f(vec2f v)
{
  return sqrt((v.x*v.x) + (v.y*v.y));
}

// Normalize vector 2D in place
inline void normalize_2f(vec2f *v)
{
  float norm = norm_2f(*v);
  v->x /= norm;
  v->y /= norm;
}

// Takes an angle in degrees and computes the radian equivalent in place
inline float deg_to_rad(float degrees)
{
  return (degrees * M_PI) / 180.0f;
}

// Rotate a vector by angle in rads in place
inline void rotate_2f_rad(vec2f *v, float angle_in_rads)
{
  float x = v->x;
  float y = v->y;
  float rot_cos = cos(angle_in_rads);
  float rot_sin = sin(angle_in_rads);
  v->x = (rot_cos * x) - (rot_sin * y);
  v->y = (rot_sin * x) + (rot_cos * y);
}

// Rotate vector by angle in degrees
inline void rotate_2f_deg(vec2f *v, float angle_in_degs)
{
  float angle_in_rads = deg_to_rad(angle_in_degs);
  rotate_2f_rad(v, angle_in_rads);
}

// Scalar product of two 2D angles
inline float scalar_prod_2f(vec2f v1, vec2f v2)
{
  return (v1.x*v2.x) + (v1.y*v2.y);
}


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

inline int asteroid_size(float radius)
{
  float relative_size = NORM(radius, MAX_ASTEROID_SIZE);
  if      (relative_size >= ASTEROID_BIG_RATIO)    return ASTEROID_SIZE_BIG;
  else if (relative_size >= ASTEROID_NORMAL_RATIO) return ASTEROID_SIZE_NORMAL;
  else                                             return ASTEROID_SIZE_SMALL;
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
void draw_enemy_ship(enemy_ship_t *enemy_ship);
void enemy_ship_logic(enemy_ship_t *enemy_ship);

void draw_player_sprite(void);
void draw_player_nosprite(void);

void draw_asteroids_sprite(void);
void draw_asteroids_nosprite(void);

void shoot_bullet(void);
void bullet_logic(void);
void draw_bullets(void);

void draw_background_static(C2D_Sprite *background);

int process_input(u32 keys_down, u32 keys_held);
void reset_game(void);
void draw_score(void);

/* Function pointers. */
void (*draw_player)(void)    = draw_player_sprite;
void (*draw_asteroids)(void) = draw_asteroids_sprite;
#endif
