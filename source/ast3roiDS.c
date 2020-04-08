#include "ast3roiDS.h"

/* Globals */
player_ship_t     player_ship;
u32               asteroidmask;
asteroid_t        asteroids[MAX_ASTEROIDS];
C3D_RenderTarget *top;
C3D_RenderTarget *bottom;
int               score;
unsigned int      framecount; // NOTE: PRINTFRAME needs this name to be unchanged
unsigned int      last_hit_frame;
C2D_Text          score_text;
C2D_TextBuf       score_text_buffer;    

/* Quick input summary:
   xinput          : -1 is full turn left, +1 is full turn right
   yinput          : -1 is full break, 1 is full acceleration
 */
float             xinput;
float             yinput;
float             xinput_sensitivity = 2.0f;
float             yinput_sensitivity = 0.01f;
int               game_state = NORMAL_GAMESTATE;

C2D_SpriteSheet   player_spritesheet;
C2D_SpriteSheet   bullet_spritesheet;
C2D_SpriteSheet   background_spritesheet;
C2D_SpriteSheet   asteroid_spritesheet;

C2D_Sprite        bullet_normal_sprite;
C2D_Sprite        background_static_sprite;
C2D_Sprite        asteroid_sprites[SPRITE_ASTEROID_TOTAL];

u32               bulletmask; // NOTE: this has to have MAX_BULLETS bits
bullet_t          bullets[MAX_BULLETS];

/* Testing, temporal structures */
enemy_ship_t      enemy_testing_ship;

/* Main program */
int main(int argc, char *argv[])
{
  /* Seed random number generator */
  srand(time(NULL));

  /* Initialize filesystem to load sprites */
  romfsInit();

  /* Initialize screens */
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  // initializes console if necessary
  CHECKDEBUGMODE;

  top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);


  /* Initialize data structures */
  init_sprites();
  init_player();
  init_asteroids(ASTEROID_NUMBER);
  
  /* Main loop */
  while (aptMainLoop())
    {
      /* Input handling */
      hidScanInput();
      // reset input values each frame
      xinput = 0.0f; 
      yinput = 0.0f;

      u32 kDown = hidKeysDown();
      u32 kHeld = hidKeysHeld();

      /* TODO: deal with analog input */
      int input_type = process_input(kDown, kHeld);
      switch (input_type) {
      case NORMAL_INPUT: // nothing to do
        break;
      case PAUSE_GAME_INPUT: // pause on-off switch
        if (game_state == NORMAL_GAMESTATE)
          game_state = PAUSED_GAMESTATE;
        else
          game_state = NORMAL_GAMESTATE;
        break;
      case EXIT_GAME_INPUT:
        goto exit_main_loop;
        break;
#ifdef DEBUG_MODE
      case DEBUG_ENEMIES_INPUT:
        enemy_testing_ship = spawn_enemy_ship(200.0f, 120.0f, 1.0f, 1.0f, 10.0f, RED);
        break;
#endif
      default:
        PRINTDINPUT("Error on input function\n");
        break;
      }

      if (game_state == NORMAL_GAMESTATE) {
        /* Logic */
        player_logic();
        bullet_logic();
        asteroid_logic();
#ifdef DEBUG_MODE
      if (enemy_testing_ship.state == ENEMY_STATE_ACTIVE)
        enemy_ship_logic(&enemy_testing_ship);
#endif
        ++framecount;
      } else {
        PRINTDLOGIC("Game is paused\n");
      }

      /** Rendering **/
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      /* Draw to top screen */
      C2D_TargetClear(top, BLACK);
      C2D_SceneBegin(top);

      draw_background_static(&background_static_sprite);
      draw_player();
      draw_bullets();
      draw_asteroids();
#ifdef DEBUG_MODE
      if (enemy_testing_ship.state == ENEMY_STATE_ACTIVE)
        draw_enemy_ship(&enemy_testing_ship);
#endif
      C2D_Flush();

      /* Draw to bottom screen */
      C2D_TargetClear(bottom, BLACK);
      C2D_SceneBegin(bottom);
      draw_score();
      C3D_FrameEnd(0);
      
    }

 exit_main_loop:
  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}

void init_sprites()
{
  player_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/player_sprites.t3x");
  // TODO: This should be svcBreak(USERBREAK_PANIC) but doesn't seem to work
  if (!player_spritesheet) 
    PRINTDINIT("Could not load player spritesheet\n");

  bullet_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/bullet_sprites.t3x");
  if (!player_spritesheet) 
      PRINTDINIT("Could not load bullet spritesheet\n");

  background_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/background_sprites.t3x");
  if (!background_spritesheet)
    PRINTDINIT("Could not load background spritesheet\n");

  asteroid_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/asteroid_sprites.t3x");
  if (!asteroid_spritesheet)
    PRINTDINIT("Could not load asteroid spritesheet\n");

  

  C2D_SpriteFromSheet(&bullet_normal_sprite, bullet_spritesheet, SPRITE_BULLET_NORMAL);
  C2D_SpriteSetCenter(&bullet_normal_sprite, 0.5f, 0.5f);

  C2D_SpriteFromSheet(&background_static_sprite, background_spritesheet, SPRITE_BACKGROUND_STATIC);
  C2D_SpriteSetCenter(&background_static_sprite, 0.5f, 0.5f);

  for (int i = 0; i < SPRITE_ASTEROID_TOTAL; i++) {
      C2D_SpriteFromSheet(&asteroid_sprites[i], asteroid_spritesheet, i);
      C2D_SpriteSetCenter(&asteroid_sprites[i], 0.5f, 0.5f);
  }
    

}

/* Initialize player's ship attributes */
void init_player()
{
  player_ship.x      = TOP_SCREEN_WIDTH/2;
  player_ship.y      = TOP_SCREEN_HEIGHT/2;
  player_ship.yspeed = 0.0f;
  player_ship.xspeed = 0.0f;
  player_ship.angle  = 90.0f;
  player_ship.radius = 10.0f;
  player_ship.health = PLAYER_STARTING_HP;
  player_ship.color  = RED;

  /* Sprite initialization for graphics */
  C2D_SpriteFromSheet(&player_ship.sprites[SPRITE_PLAYER_NORMAL], player_spritesheet, SPRITE_PLAYER_NORMAL);
  C2D_SpriteSetCenter(&player_ship.sprites[SPRITE_PLAYER_NORMAL], 0.5f, 0.5f);

  C2D_SpriteFromSheet(&player_ship.sprites[SPRITE_PLAYER_BOOSTING], player_spritesheet, SPRITE_PLAYER_BOOSTING);
  C2D_SpriteSetCenter(&player_ship.sprites[SPRITE_PLAYER_BOOSTING], 0.5f, 0.5f);
  
  
  /* Vertices for nosprite drawing */
  player_ship.vertices[X0] = -player_ship.radius;        // vertex 0
  player_ship.vertices[Y0] =  player_ship.radius;
  player_ship.vertices[X1] =  0.0f;                      // vertex 1
  player_ship.vertices[Y1] = -player_ship.radius*2.0f;
  player_ship.vertices[X2] =  player_ship.radius;        // vertex 2
  player_ship.vertices[Y2] =  player_ship.radius;
}

void init_asteroids(int n)
{
  asteroid_t *asteroid;
  for (int i = 0; i < n; i++) {
    /* Check for free space in the asteroid mask */
    int j = 0;
    for (; j < MAX_ASTEROIDS; j++)
      if (~(asteroidmask >> j) & 1) break;

    /* If we already have the max number ignore petition */
    if (i >= MAX_ASTEROIDS) return;
    /* Reserve place in mask */
    asteroidmask = asteroidmask | (1 << j);
    
    asteroid = &asteroids[j];
    
    float initial_x;
    float initial_y;
  /* Find starting position at least X px away from player in both directions */
    do {
      initial_x = randf(400);
      initial_y = randf(240);
    } while (inside_circle(initial_x, initial_y, player_ship.x, player_ship.y, PLAYER_SAFE_ZONE_RADIUS));
    asteroid->x = initial_x;
    asteroid->y = initial_y;
    
    /* Make speed in range (-maxs,+maxs) */
    float xs = randf(ASTEROID_MAXSPEED * 2.0f);
    float ys = randf(ASTEROID_MAXSPEED * 2.0f);
  
    asteroid->xspeed = xs - ASTEROID_MAXSPEED;
    asteroid->yspeed = ys - ASTEROID_MAXSPEED;

    /* Only create asteroids of acceptable, discrete sizes */
    /* TODO(David): Figure if we can scale sprites to avoid using discrete sizes */
    float rad = MAX_ASTEROID_SIZE;
    int ast_size = rand() % ASTEROID_SIZE_TOTAL;
    switch (ast_size) {
    case ASTEROID_SIZE_SMALL:
      rad *= ASTEROID_SMALL_RATIO;
      break;
    case ASTEROID_SIZE_NORMAL:
      rad *= ASTEROID_NORMAL_RATIO;
      break;
    case ASTEROID_SIZE_BIG:
      rad *= ASTEROID_BIG_RATIO;
      break;
    default:
      // NOTE(David): Possible error-log here
      break;
    }
    asteroid->radius   = rad;
    asteroid->rotspeed = randf2(-1.0f, 1.0f);
    asteroid->angle    = 0.0f;
    asteroid->color    = WHITE;
    
  }
  PRINTDINIT("Initial AST mask %#lx\n", asteroidmask);
}

/* Updates asteroid position */
void asteroid_logic(void)
{
  asteroid_t *asteroid;
  /* Ignore inactive asteroids */
  int i = 0;
  for (; i < MAX_ASTEROIDS; i++) {
    if (asteroidmask >> i & 1) {
      asteroid = &asteroids[i];
      float old_x = asteroid->x;
      float new_x = old_x + asteroid->xspeed;

      float old_y = asteroid->y;
      float new_y = old_y + asteroid->yspeed;

      float radius = asteroid->radius;

      if (inside_circle(player_ship.x, player_ship.y, new_x, new_y, radius)) {
        PRINTDCOLLISION("Collided asteroid on %3.2f, %3.2f\n", player_ship.x, player_ship.y);

        /* Only hit player once per grace period */
        if (framecount - last_hit_frame > GRACE_PERIOD_AFTER_HIT) {
          player_ship.health--;
          asteroid->color = RED;
        }
        last_hit_frame = framecount;
      }

      if (new_y > TOP_SCREEN_HEIGHT + radius) {
        new_y = - radius;
        PRINTDLOGIC("Asteroid went out of bounds downwards\n");
      } else if (new_y < 0 - radius) {
        new_y = (float) TOP_SCREEN_HEIGHT + radius;
        PRINTDLOGIC("Asteroid went out of bounds upwards\n");
      }

      if (new_x > TOP_SCREEN_WIDTH + radius) {
        new_x = - radius;
        PRINTDLOGIC("Asteroid went out of bounds rightwards\n");
      } else if (new_x < 0 - radius) {
        new_x = (float) TOP_SCREEN_WIDTH + radius;
        PRINTDLOGIC("Asteroid went out of bounds leftwards\n");
      }

      asteroid->x     = new_x;
      asteroid->y     = new_y;
      asteroid->angle += asteroid->rotspeed;
    }
  }
}


/* Draw asteroids on screen */
void draw_asteroids_nosprite(void)
{
  u32 clr;
  float depth = 0.9f;
  asteroid_t *asteroid;
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroidmask >> i & 1) {
      asteroid = &asteroids[i];
      clr      = asteroid->color;
      C2D_DrawCircle(asteroid->x,
                     asteroid->y,
                     depth,
                     asteroid->radius,
                     clr,
                     clr,
                     clr,
                     clr);
      PRINTDRENDER("Draw asteroid X %3.2f Y %3.2f\n", asteroid->x, asteroid->y);
    }
  }
}

/* Draw asteroids as a sprite */
void draw_asteroids_sprite(void)
{
  asteroid_t *asteroid;
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroidmask >> i & 1) {
      asteroid = &asteroids[i];
      int size = asteroid_size(asteroid->radius);
      C2D_SpriteSetPos(&asteroid_sprites[size], asteroid->x, asteroid->y);
      C2D_SpriteSetRotation(&asteroid_sprites[size], deg_to_rad(-asteroid->angle+90.0f));
      C2D_DrawSprite(&asteroid_sprites[size]);
      PRINTDRENDER("Draw asteroid X %3.2f Y %3.2f\n", asteroid->x, asteroid->y);
    }
  }
}

/* Draw player ship as a sprite */
void draw_player_sprite()
{
  C2D_SpriteSetPos(&player_ship.sprites[player_ship.curr_sprite], player_ship.x, player_ship.y);
  C2D_SpriteSetRotation(&player_ship.sprites[player_ship.curr_sprite], C3D_AngleFromDegrees(-player_ship.angle+90.0f));
  C2D_DrawSprite(&player_ship.sprites[player_ship.curr_sprite]);
}

/* Draw player ship as a triangle using 3 vertices */
void draw_player_nosprite()
{ 
  u32   clr   = player_ship.color;
  float depth = 1.0f;

  
  C2D_DrawTriangle(player_ship.x + player_ship.vertices[X0], // vertex 0
                   player_ship.y + player_ship.vertices[Y0],
                   clr,
                   player_ship.x + player_ship.vertices[X1], // vertex 1
                   player_ship.y + player_ship.vertices[Y1],
                   clr,
                   player_ship.x + player_ship.vertices[X2], // vertex 2
                   player_ship.y + player_ship.vertices[Y2],
                   clr,
                   depth);

  PRINTDRENDER("Draw player \tX %3.3f\tY %3.3f\n", player_ship.x, player_ship.y);
}

/* Simple processing of player input */
int process_input(u32 keys_down, u32 keys_held)
{
  // for inputs that do not care about the first press
  u32 input_keys = keys_down | keys_held; 
  
  if (input_keys & KEY_CPAD_RIGHT) {
    PRINTDINPUT("Pressed KEY_CPAD_RIGHT\n");
    xinput = 1.0f;
  }
  if (input_keys & KEY_CPAD_DOWN) {
    PRINTDINPUT("Pressed KEY_CPAD_DOWN\n");
    yinput = -1.0f;
  }
  if (input_keys & KEY_CPAD_LEFT) {
    PRINTDINPUT("Pressed KEY_CPAD_LEFT\n");
    xinput = -1.0f;
  }
  if (input_keys & KEY_CPAD_UP) {
    PRINTDINPUT("Pressed KEY_CPAD_UP\n");
    yinput = 1.0f;
    player_ship.curr_sprite = SPRITE_PLAYER_BOOSTING; // boosters ON sprite selected
  } else {
    player_ship.curr_sprite = SPRITE_PLAYER_NORMAL;   // boosters OFF sprite selected
  }

  if (keys_down & KEY_A) {
    shoot_bullet();
    PRINTDINPUT("Pressed KEY_A\n");
  }
  
  if (keys_down & KEY_SELECT) {
    PRINTDINPUT("Pressed KEY_SELECT\n");
    return EXIT_GAME_INPUT;
  }
  if (keys_down & KEY_START) {
    PRINTDINPUT("Pressed KEY_START\n");
    return PAUSE_GAME_INPUT;
  }

#ifdef DEBUG_MODE
  if (input_keys & KEY_B)
    return DEBUG_ENEMIES_INPUT;
#endif
  return NORMAL_INPUT;
}

/* Applies input and moves character */
void player_logic()
{

  /* If player is dead, reset game and return */
  // TODO: game-over screen and better things here
  if (player_ship.health == 0)
    reset_game();
  
  /* Apply input sensitivity */
  xinput = xinput * xinput_sensitivity;
  yinput = yinput * yinput_sensitivity;
  
  /* Update angle based on player input */
  float new_angle = player_ship.angle - xinput;
  player_ship.angle = clamp_deg(new_angle);

  /* Cache cos and sin for the duration of the frame */
  float fsin = sin(deg_to_rad(new_angle));
  float fcos = cos(deg_to_rad(new_angle));

  /* Update speed vector */
  player_ship.yspeed = player_ship.yspeed + yinput * -fsin;
  player_ship.xspeed = player_ship.xspeed + yinput * fcos;
  
  PRINTDLOGIC("XVEL %2.2f YVEL %2.2f\n", player_ship.xspeed, player_ship.yspeed);

  /* Update position */
  float old_x = player_ship.x;
  float new_x = old_x + player_ship.xspeed;

  float old_y = player_ship.y;
  float new_y = old_y + player_ship.yspeed;

  if (new_y > TOP_SCREEN_HEIGHT) {
    new_y = 0.0f;
    PRINTDLOGIC("Ship went out of bounds downwards\n");
  } else if (new_y < 0) {
    new_y = (float) TOP_SCREEN_HEIGHT;
    PRINTDLOGIC("Ship went out of bounds upwards\n");
  }

  if (new_x > TOP_SCREEN_WIDTH) {
    new_x = 0;
    PRINTDLOGIC("Ship went out of bounds rightwards\n");
  } else if (new_x < 0) {
    new_x = (float) TOP_SCREEN_WIDTH;
    PRINTDLOGIC("Ship went out of bounds leftwards\n");
  }
  
  player_ship.y = new_y;
  player_ship.x = new_x;

  /* Update vertex positions for figure */
  rotate_2f_deg(&player_ship.v1, xinput);
  rotate_2f_deg(&player_ship.v2, xinput);
  rotate_2f_deg(&player_ship.v3, xinput);  
}

void enemy_ship_logic(enemy_ship_t *enemy)
{
  float old_x = enemy->x;
  float old_y = enemy->y;
  float new_x = old_x + enemy->xspeed;
  float new_y = old_y + enemy->yspeed;
  float angle = enemy->angle;
  
  if (new_y > TOP_SCREEN_HEIGHT) {
    new_y = 0.0f;
  } else if (new_y < 0) {
    new_y = (float) TOP_SCREEN_HEIGHT;
  }
  
  if (new_x > TOP_SCREEN_WIDTH) {
    new_x = 0;
  } else if (new_x < 0) {
    new_x = (float) TOP_SCREEN_WIDTH;
  }

  enemy->x = new_x;
  enemy->y = new_y;

  /* Try to look at player */
  /* We calculate the direction towards the player ship. Then we calculate if we would
   * approximate that vector by going to the right or left using the scalar product */
  
  /* NOTE(David): Is this really the most efficient way to track the player ? */
  vec2f look_at_player_v;
  look_at_player_v.x = player_ship.x - enemy->x;
  look_at_player_v.y = player_ship.y - enemy->y;
  
  vec2f curr_direction_v;
  curr_direction_v.x = cos(deg_to_rad(angle));
  curr_direction_v.y = sin(deg_to_rad(angle));
  float direction_proj = scalar_prod_2f(look_at_player_v, curr_direction_v);

  vec2f turn_left_v = curr_direction_v;
  rotate_2f_deg(&turn_left_v, enemy->turnrate);
  float turn_left_proj = scalar_prod_2f(look_at_player_v, turn_left_v);

  vec2f turn_right_v = curr_direction_v;
  rotate_2f_deg(&turn_right_v, -enemy->turnrate);
  float turn_right_proj = scalar_prod_2f(look_at_player_v, turn_right_v);
  int direction = 0;
  
  if      (turn_left_proj > direction_proj)  direction = -1; // left 
  else if (turn_right_proj > direction_proj) direction =  1; // right

  float angle_delta = enemy->turnrate * direction;
  
  enemy->angle += angle_delta;
  
  if (direction) {
    /* Update vertex positions for figure */
    rotate_2f_deg(&enemy->v1, angle_delta);
    rotate_2f_deg(&enemy->v2, angle_delta);
    rotate_2f_deg(&enemy->v3, angle_delta);
  }
}

void shoot_bullet(void)
{
  /* Check first free bullet size in array */
  int i = 0;
  for (; i < MAX_BULLETS; i++)
    if (~(bulletmask >> i) & 1) break;
  PRINTDBULLETS("i %d, mask %#lx, size %d\n", i, bulletmask, sizeof(bulletmask));

  /* If there is none ignore bullet fire */
  if (i >= MAX_BULLETS) return;
  PRINTDBULLETS("i %d, mask %#lx\n", i, bulletmask);
  /* Otherwise initialize bullet */
  bulletmask = bulletmask | (1 << i);
  bullets[i].x = player_ship.x;
  bullets[i].y = player_ship.y;
  float angle = player_ship.angle;
  bullets[i].angle = angle;
  /* Check sin and cos this frame */
  float fsin = sin(deg_to_rad(angle));
  float fcos = cos(deg_to_rad(angle));


  bullets[i].xspeed = BULLET_INITIAL_SPEED * fcos;
  bullets[i].yspeed = BULLET_INITIAL_SPEED * -fsin;
  bullets[i].sprite = &bullet_normal_sprite;
}

enemy_ship_t spawn_enemy_ship(float x, float y, float xs, float ys, float r, u32 color)
{
  enemy_ship_t new_enemy = {
  .x            = x,
  .y            = y,
  .xspeed       = xs,
  .yspeed       = ys,
  .radius       = r,
  .color        = color,
  .angle        = 90.0f,
  .health       = TODO_CHANGEME,
  .turnrate     = 5.0f,
  .state        = ENEMY_STATE_ACTIVE,  
  .vertices[X0] = -r,
  .vertices[Y0] =  r,
  .vertices[X1] =  0.0f,
  .vertices[Y1] =  -r*2.0f,
  .vertices[X2] =  r,
  .vertices[Y2] =  r,
  };
  return new_enemy;
}

void draw_enemy_ship(enemy_ship_t *enemy_ship)
{
  u32   clr   = enemy_ship->color;
  float depth = 1.0f;

  
  C2D_DrawTriangle(enemy_ship->x + enemy_ship->vertices[X0], // vertex 0
                   enemy_ship->y + enemy_ship->vertices[Y0],
                   clr,
                   enemy_ship->x + enemy_ship->vertices[X1], // vertex 1
                   enemy_ship->y + enemy_ship->vertices[Y1],
                   clr,
                   enemy_ship->x + enemy_ship->vertices[X2], // vertex 2
                   enemy_ship->y + enemy_ship->vertices[Y2],
                   clr,
                   depth);

}

void bullet_logic(void)
{
  int i = 0;
  for (; i < MAX_BULLETS; i++) {
    if (bulletmask >> i & 1) {
      float bx     = bullets[i].x + bullets[i].xspeed;
      float by     = bullets[i].y + bullets[i].yspeed;
      bullets[i].x = bx;
      bullets[i].y = by;

      /* Check for asteroid hit */

      // Ignore inactive asteroids
      int j = 0;
      for (; j < MAX_ASTEROIDS; j++) {
        if (asteroidmask >> j & 1) {
          float ax   = asteroids[j].x;
          float ay   = asteroids[j].y;
          float arad = asteroids[j].radius;
          if (inside_circle(bx, by, ax, ay, arad)) {
            PRINTDBULLETS("Bullet hit at %3.2f, %3.2f\n", bx, by);
            /* TODO: FX on bullet hit */
            bulletmask = bulletmask & ~(1 << i);
            break_asteroid(&asteroids[j], j);
          }
        }

        /* If bullet went out of bounds we disable it */
        if (!(inside_top_screen(bx, by))) {
          PRINTDBULLETS("Bullet went out of screen");
          bulletmask &= ~(1 << i);
        }
      }
    }
  }
}

void draw_bullets(void)
{
  int i = 0;
  for (; i < MAX_BULLETS; i++) {
    if (bulletmask >> i & 1) {
      C2D_SpriteSetPos(bullets[i].sprite, bullets[i].x, bullets[i].y);
      C2D_SpriteSetRotation(bullets[i].sprite, deg_to_rad(-bullets[i].angle+90.0f));
      C2D_DrawSprite(bullets[i].sprite);
    }
  }
}

void break_asteroid(asteroid_t *asteroid, int idx)
{
  /* Check asteroid type */
  int size = asteroid_size(asteroid->radius);
  /* Free asteroid struct position */
  asteroidmask &= ~(1 << idx);
  if (size > ASTEROID_SIZE_SMALL) {
    spawn_asteroids(asteroid->x, asteroid->y, size-1, 2);
  }
  /* Increment score based on asteroid size */
  switch (size) {
  case ASTEROID_SIZE_SMALL:
    score += ASTEROID_SMALL_SCORE;
    break;
  case ASTEROID_SIZE_NORMAL:
    score += ASTEROID_NORMAL_SCORE;
    break;
  case ASTEROID_SIZE_BIG:
    score += ASTEROID_BIG_SCORE;
    break;
  default:
    break;
  }
}

void spawn_asteroids(float x, float y, asteroid_size_t size, int n)
{
  asteroid_t *asteroid;
  for (int i = 0; i < n; i++) {
    /* Look for free positions in array */
    int j = 0;
    for (; j < MAX_ASTEROIDS; j++)
      if (~(asteroidmask >> j) & 1) break;

    /* If there are none do nothing */
    if (j >= MAX_ASTEROIDS) return;

    /* Reserve space in mask */
    asteroidmask |= (1 << j);
    
    asteroid = &asteroids[j];

    asteroid->x = x;
    asteroid->y = y;
    
    /* Make speed in range (-maxs,+maxs) */
    asteroid->xspeed = randf2(-ASTEROID_MAXSPEED, ASTEROID_MAXSPEED);
    
    float rad = 0.0f;
    if (size == ASTEROID_SIZE_BIG) {
      rad = MAX_ASTEROID_SIZE*ASTEROID_BIG_RATIO;
    } else if (size == ASTEROID_SIZE_NORMAL) {
      rad = MAX_ASTEROID_SIZE*ASTEROID_NORMAL_RATIO;
    } else if (size == ASTEROID_SIZE_SMALL) {
      rad = MAX_ASTEROID_SIZE*ASTEROID_SMALL_RATIO;
    }
    asteroid->radius   = rad;
    asteroid->rotspeed = randf(1.0f);
    asteroid->color    = WHITE;
  }
}

void draw_score(void)
{
  char buf[SCORE_TEXT_LENGTH];
  sprintf(buf, "Score: %d", score);
  
  score_text_buffer = C2D_TextBufNew(SCORE_TEXT_LENGTH);
  C2D_TextParse(&score_text, score_text_buffer, buf);
  C2D_DrawText(&score_text, C2D_WithColor, 5.0f, 5.0f, 0.0f, 1.0f, 1.0f, WHITE);
}

/* Draw static background sprite, we assume that its center is at 0.5, 0.5 */
void draw_background_static(C2D_Sprite *background)
{
  C2D_SpriteSetPos(background, TOP_SCREEN_WIDTH/2, TOP_SCREEN_HEIGHT/2);
  C2D_DrawSprite(background);
}

void reset_game(void)
{
  asteroidmask = 0;
  bulletmask = 0;
  framecount = 0;
  last_hit_frame = 0;
  score = 0;

  /* NOTE(David): at the moment init_sprites() should not be called here.
     This may change at some point if we free graphics memory in-between levels */
  
  init_player();
  init_asteroids(ASTEROID_NUMBER);
}
