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
C2D_SpriteSheet   enemy_spritesheet;
C2D_SpriteSheet   pickup_hp_spritesheet;
C2D_SpriteSheet   player_health_spritesheet;

C2D_Sprite        bullet_normal_sprite;
C2D_Sprite        background_static_sprite;
C2D_Sprite        asteroid_sprites[SPRITE_ASTEROID_TOTAL];
C2D_Sprite        enemy_sprites[SPRITE_ENEMY_TOTAL];
C2D_Sprite        pickup_hp_sprites[SPRITE_PICKUP_HP_TOTAL];
C2D_Sprite        player_health_sprite[PLAYER_STARTING_HP];


u32               bulletmask; // NOTE: this has to have MAX_BULLETS bits
bullet_t          bullets[MAX_BULLETS];
enemy_ship_t      enemy_ships[MAX_ENEMY_SHIPS];
pickup_t          pickups[MAX_PICKUPS];
health_t          health;
int               asteroid_spawn_freq = 120;
int               enemy_spawn_freq = 300;

int              gameover_frame;
int              gameover_remaining_seconds;
int              saving_score;

#ifdef DEBUG_MODE
FILE            *_debug_log;
bool             _writing_log = false;
#endif


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

  // initializes debug file if necessary
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
        for (int i = 0; i < MAX_ENEMY_SHIPS; ++i) {
          if (!enemy_ships[i].state) {// inactive
            enemy_ships[i] = spawn_enemy_ship(200.0f, 120.0f, 1.0f, 1.0f, 10.0f, RED);
            break;
          }
        }
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
        
        for (int i = 0; i < MAX_ENEMY_SHIPS; ++i)
          if (enemy_ships[i].state) // not inactive
            enemy_ship_logic(&enemy_ships[i]);

        for (int i = 0; i < MAX_PICKUPS; ++i)
          if (pickups[i].state) // not inactive
            pickup_logic(&pickups[i]);

        natural_asteroid_spawn(asteroid_spawn_freq);
        natural_enemy_spawn(enemy_spawn_freq);
        ++framecount; PRINTFRAME;
      } else if (game_state == PAUSED_GAMESTATE) {
        PRINTDLOGIC("Game is paused\n");
      } else if (game_state == GAMEOVER_GAMESTATE) {
        if (saving_score) {
          saving_score_logic();
        } else {
          gameover_logic();
          ++framecount; PRINTFRAME;
        }
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
      for (int i = 0; i < MAX_PICKUPS; ++i) {
        if (pickups[i].state) { // not inactive 
          draw_pickup(&pickups[i]);
        }
      }
      for (int i = 0; i < MAX_ENEMY_SHIPS; ++i)
        if (enemy_ships[i].state) // not inactive
          draw_enemy_ship(&enemy_ships[i]);

      if (game_state == GAMEOVER_GAMESTATE) {
        draw_gameover_fade();
      }
    
      C2D_Flush();

      /* Draw to bottom screen */
      C2D_TargetClear(bottom, BLACK);
      C2D_SceneBegin(bottom);
      if (game_state == GAMEOVER_GAMESTATE) {
        draw_gameover_screen();
      } else {
        draw_score();
        draw_health();
      }
      C3D_FrameEnd(0);
    }

 exit_main_loop:
  C2D_Fini();
  C3D_Fini();
  gfxExit();
  sdmcExit();
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
  
  enemy_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/enemy_sprites.t3x");
  if (!enemy_spritesheet)
    PRINTDINIT("Could not load asteroid spritesheet\n");

  pickup_hp_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/hp_pickup_sprites.t3x");
  if (!pickup_hp_spritesheet) exit(1);

  player_health_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/player_health_sprite.t3x");

  for (int i = 0; i < SPRITE_PICKUP_HP_TOTAL; ++i) {
    C2D_SpriteFromSheet(&pickup_hp_sprites[i], pickup_hp_spritesheet, i);
    C2D_SpriteSetCenter(&pickup_hp_sprites[i], 0.5f, 0.5f);
  }


  C2D_SpriteFromSheet(&bullet_normal_sprite, bullet_spritesheet, SPRITE_BULLET_NORMAL);
  C2D_SpriteSetCenter(&bullet_normal_sprite, 0.5f, 0.5f);

  C2D_SpriteFromSheet(&background_static_sprite, background_spritesheet, SPRITE_BACKGROUND_STATIC);
  C2D_SpriteSetCenter(&background_static_sprite, 0.5f, 0.5f);

  for (int i = 0; i < SPRITE_ASTEROID_TOTAL; i++) {
    C2D_SpriteFromSheet(&asteroid_sprites[i], asteroid_spritesheet, i);
    C2D_SpriteSetCenter(&asteroid_sprites[i], 0.5f, 0.5f);
  }
  
  C2D_SpriteFromSheet(&enemy_sprites[SPRITE_ENEMY_NORMAL], enemy_spritesheet, SPRITE_ENEMY_NORMAL);
  C2D_SpriteSetCenter(&enemy_sprites[SPRITE_ENEMY_NORMAL], 0.5f, 0.5f);

  for (int i = 0; i < PLAYER_STARTING_HP; i++) {
    C2D_SpriteFromSheet(&player_health_sprite[i], player_health_spritesheet, 0);
    C2D_SpriteSetCenter(&player_health_sprite[i], 0.5f, 0.5f);
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
  player_ship.effects &= 0;

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
    
    loot_table_t new_loot_table = {
                                   .probabilities = {0.5f, 0.7f, 1.0f},
                                   .items = {NOTHING, EXTRA_SCORE, PICKUP_HP},
    };
    asteroid->loot_table = new_loot_table;
    
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
          last_hit_frame = framecount;
          player_ship.effects |= PLAYER_EFFECT_BLINKING;
        }
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

void init_health(void)
{
  health.x = 20.0f;
  health.y = 50.0f;
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
  if (player_ship.effects & PLAYER_EFFECT_BLINKING && !(framecount % 4))
    return;
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

  if (game_state == NORMAL_GAMESTATE) {
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
    if (keys_down & KEY_B)
      return DEBUG_ENEMIES_INPUT;
#endif
  } else if (game_state == GAMEOVER_GAMESTATE) {
    if (keys_down & KEY_A) {
      saving_score = score;
    }
  }
  
  return NORMAL_INPUT;
}

/* Applies input and moves character */
void player_logic()
{

  /* If player is dead, reset game and return */
  // TODO: game-over screen and better things here
  if (player_ship.health == 0) {
    player_ship.effects &= 0;
    player_ship.effects |= (PLAYER_EFFECT_DEAD);
    game_state = GAMEOVER_GAMESTATE;
    gameover_frame = framecount;
    return;
  }
  
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

  /* Get rid / apply blinking effect if needed */
  player_ship.effects &= framecount - last_hit_frame < GRACE_PERIOD_AFTER_HIT ?
    (~0) : ~(PLAYER_EFFECT_BLINKING);
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
  
  vec2f look_at_player_v;
  look_at_player_v.x = player_ship.x - new_x;
  look_at_player_v.y = player_ship.y - new_y;
  normalize_2f(&look_at_player_v);
  
  vec2f curr_direction_v;
  curr_direction_v.x = cos(deg_to_rad(angle));
  curr_direction_v.y = -sin(deg_to_rad(angle));
  float curr_direction_proj = scalar_prod_2f(look_at_player_v, curr_direction_v);

  vec2f turn_left_v = curr_direction_v;
  rotate_2f_deg(&turn_left_v, -enemy->turnrate);
  float turn_left_proj = scalar_prod_2f(look_at_player_v, turn_left_v);

  vec2f turn_right_v = curr_direction_v;
  rotate_2f_deg(&turn_right_v, +enemy->turnrate);
  float turn_right_proj = scalar_prod_2f(look_at_player_v, turn_right_v);
  float angle_delta;
  angle_delta = 0.0f;
  if (turn_left_proj > curr_direction_proj) {         // turn left
    angle_delta = +enemy->turnrate;
    enemy->angle = clamp_deg(angle + angle_delta);
  } else if (turn_right_proj > curr_direction_proj) { // turn right
    angle_delta = -enemy->turnrate;
    enemy->angle = clamp_deg(angle + angle_delta);
  } 

  /* Update vertex positions for figure */
  rotate_2f_deg(&enemy->v1, -angle_delta);
  rotate_2f_deg(&enemy->v2, -angle_delta);
  rotate_2f_deg(&enemy->v3, -angle_delta);

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
  .turnrate     = 5.5f,
  .state        = ENEMY_STATE_ACTIVE,  
  .vertices[X0] = -r,
  .vertices[Y0] =  r,
  .vertices[X1] =  0.0f,
  .vertices[Y1] =  -r*2.0f,
  .vertices[X2] =  r,
  .vertices[Y2] =  r,
  .sprites[0]   =  enemy_sprites[0],
  .curr_sprite  =  SPRITE_ENEMY_NORMAL
  };
  return new_enemy;
}


/* Draws the enemy ship as a sprite*/

void draw_enemy_ship_sprite(enemy_ship_t *enemy)
{
  C2D_SpriteSetPos(&enemy->sprites[enemy->curr_sprite], enemy->x, enemy->y);
  C2D_SpriteSetRotation(&enemy->sprites[enemy->curr_sprite], C3D_AngleFromDegrees(-enemy->angle+90.0f));
  C2D_DrawSprite(&enemy->sprites[enemy->curr_sprite]);
}

void draw_enemy_ship_nosprite(enemy_ship_t *enemy_ship)
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

int natural_enemy_spawn(int freq)
{
  enemy_ship_t *enemy = NULL;
  int i = 0;
  for (; i < MAX_ENEMY_SHIPS; ++i)
    if (enemy_ships[i].state == ENEMY_STATE_INACTIVE) {
      enemy = &enemy_ships[i];
      break;
    }
  if (!enemy) return 0;
  int side;
  float r = 10.0f;
  if (!(framecount % freq)) { // spawn new asteroids naturally
    side = rand() % 3;
    // case 1: spawn in left side
    if (side == 1) *enemy = spawn_enemy_ship(-r, randf(240.0f), randf2(-1.0f, 1.0f), randf2(-1.0f, 1.0f), r, WHITE);
    // case 2: spawn in top side
    if (side == 2) *enemy = spawn_enemy_ship(randf(400.0f), -r, randf2(-1.0f, 1.0f), randf2(-1.0f, 1.0f), r, WHITE);
    // case 3: spawn in bottom side
    if (side == 3) *enemy = spawn_enemy_ship(240.0f, randf(240.0f), randf2(-1.0f, 1.0f), randf2(-1.0f, 1.0f), r, WHITE);
  }

  return 1;
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
  /* Check for loot */
  int loot = dispatch_loot_table(asteroid->loot_table);
  switch (loot) {
  case PICKUP_BOMB:
    break;
  case PICKUP_HP:
    for (int i = 0; i < MAX_PICKUPS; ++i) {
      if (!pickups[i].state) { // inactive
        pickups[i] = spawn_pickup(PICKUP_HP, asteroid->x, asteroid->y, 0.0f, 0.0f, 10.0f, WHITE);
        break;
      }
    }
    break;
  case EXTRA_SCORE:
    score += 10000000; // TODO(David): change this nonsense. This is for testing purposes
    break;
  }
  
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
    asteroid->yspeed = randf2(-ASTEROID_MAXSPEED, ASTEROID_MAXSPEED);
    
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

/* Makes new asteroids spawn from the sides of the screen */
int natural_asteroid_spawn(int freq)
{
  int n = 0, side;
  if (!(framecount % freq)) { // spawn new asteroids naturally
    side = rand() % 3;
    n = rand() % 3;

    // case 1: spawn in left side
    if (side == 1) spawn_asteroids(-MAX_ASTEROID_SIZE, randf(240.0f), ASTEROID_SIZE_BIG, n);
    // case 2: spawn in top side
    if (side == 2) spawn_asteroids(randf(400.0f), -MAX_ASTEROID_SIZE, ASTEROID_SIZE_BIG, n);
    // case 3: spawn in bottom side
    if (side == 3) spawn_asteroids(240.0f, randf(240.0f)+MAX_ASTEROID_SIZE, ASTEROID_SIZE_BIG, n);
  }
  return n;
}

pickup_t spawn_pickup(int type, float x, float y, float xs, float ys, float r, u32 color)
{
  pickup_t new_pickup = {
                         .x           = x,
                         .y           = y,
                         .xspeed      = xs,
                         .yspeed      = ys,
                         .type        = type,
                         .radius      = r,
                         .curr_sprite = 0,
                         .color       = color,
                         .state       = PICKUP_STATE_ACTIVE,
  };
  switch (type) {
  case PICKUP_HP:
    new_pickup.sprites      = pickup_hp_sprites;
    new_pickup.nsprites     = PICKUP_HP_NSPRITES;
    new_pickup.anim_speed   = PICKUP_HP_ANIM_SPEED;
    break;
  default: // TODO(David): insert some kind of WIP or debug sprite to see the error
    break;
  }

  return new_pickup;
}

void pickup_logic(pickup_t *pickup)
{
  /* Movement */
  float new_x = pickup->x + pickup->xspeed;
  float new_y = pickup->y + pickup->yspeed;
  float radius = pickup->radius;
  
  if (new_y > TOP_SCREEN_HEIGHT + radius) {
    new_y = - radius;
  } else if (new_y < 0 - radius) {
    new_y = (float) TOP_SCREEN_HEIGHT + radius;
  }

  if (new_x > TOP_SCREEN_WIDTH + radius) {
    new_x = - radius;
  } else if (new_x < 0 - radius) {
    new_x = (float) TOP_SCREEN_WIDTH + radius;
  }

  pickup->x = new_x;
  pickup->y = new_y;

  /* Collision detection with player */
  if (inside_circle(player_ship.x, player_ship.y, new_x, new_y, radius)) {
    ++player_ship.health;
    pickup->state = PICKUP_STATE_INACTIVE;
  }
}

void draw_pickup(pickup_t *pickup)
{
  /* Get and update current state of animation */
  C2D_Sprite *the_sprite;
  int nsprites = pickup->nsprites;
  int curr_sprite = pickup->curr_sprite;
  int anim_speed = pickup->anim_speed;

  if (game_state == NORMAL_GAMESTATE) {
  
    if (anim_speed) {
      if (!(framecount % anim_speed)) ++curr_sprite;
      curr_sprite = curr_sprite >= nsprites ? 0 : curr_sprite;
      the_sprite = &pickup->sprites[curr_sprite];
      pickup->curr_sprite = curr_sprite;
    } else {
      the_sprite = &pickup->sprites[0]; // no animation
    }
  } else {
    the_sprite = &pickup->sprites[curr_sprite];
  }

  /* Draw animation */
  C2D_SpriteSetPos(the_sprite, pickup->x, pickup->y);
  C2D_DrawSprite(the_sprite);

}

void draw_score(void)
{
  char buf[SCORE_TEXT_LENGTH];
  stbsp_sprintf(buf, "Score: %d", score);
 
  score_text_buffer = C2D_TextBufNew(SCORE_TEXT_LENGTH);
  C2D_TextParse(&score_text, score_text_buffer, buf);
  C2D_DrawText(&score_text, C2D_WithColor, 5.0f, 5.0f, 0.0f, 1.0f, 1.0f, WHITE);
}

void draw_health(void)
{
  init_health();

  for (int i = 0; i < player_ship.health; ++i) {
    C2D_SpriteSetPos(&player_health_sprite[i], health.x , health.y);
    C2D_DrawSprite(&player_health_sprite[i]);
    health.x += 30.0f;
  }
}

/* Draw static background sprite, we assume that its center is at 0.5, 0.5 */
void draw_background_static(C2D_Sprite *background)
{
  C2D_SpriteSetPos(background, TOP_SCREEN_WIDTH/2, TOP_SCREEN_HEIGHT/2);
  C2D_DrawSprite(background);
}

void reset_game(void)
{
  game_state = NORMAL_GAMESTATE;
  asteroidmask = 0;
  bulletmask = 0;
  framecount = 0;
  last_hit_frame = 0;
  score = 0;
  player_ship.effects = 0;

  for (int i = 0; i < MAX_ENEMY_SHIPS; ++i)
    enemy_ships[i].state = ENEMY_STATE_INACTIVE;

  /* NOTE(David): at the moment init_sprites() should not be called here.
     This may change at some point if we free graphics memory in-between levels */
  
  init_player();
  init_asteroids(ASTEROID_NUMBER);
}

void draw_gameover_screen(void)
{
  /* TODO: Do something better here */
  print_in_rect_centered("GAME OVER", sizeof("GAME OVER"), BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_HEIGHT/2, 1.5f);
  char buf[32];
  stbsp_sprintf(buf, "Time remaining: %ds", gameover_remaining_seconds);
  print_in_rect_centered(buf, 32, BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_HEIGHT/2 + 40.0f, 1.0f);

  if (score) {
    stbsp_sprintf(buf, "Press A to save your score", gameover_remaining_seconds);
    print_in_rect_centered(buf, 32, BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_HEIGHT -10.0f, 0.8f);
  }
}

void gameover_logic(void)
{
  gameover_remaining_seconds = GAMEOVER_SCREEN_TIME/60 - ((framecount - gameover_frame) / 60);
  if (gameover_remaining_seconds <= 0)
    reset_game();
}

void draw_gameover_fade(void)
{
  u32 color = C2D_Color32f(0.0f, 0.0f, 0.0f, 0.0f + (framecount - gameover_frame)/300.0f);
  C2D_DrawRectSolid(0.0f, 0.0f, 1.0f, TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT, color);
}

void saving_score_logic()
{
  static char hint_buf[64], input_buf[64];
  if (saving_score) {
    SwkbdState       swkbd;
    SwkbdButton      button_pressed = SWKBD_BUTTON_NONE;
    stbsp_sprintf(hint_buf, "Enter your name to save your score -> %d", saving_score);
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK | SWKBD_FILTER_AT, 0, 0);
    swkbdSetHintText(&swkbd, hint_buf);
    /* The system OS stops us here until the user is done inputting text */
    button_pressed = swkbdInputText(&swkbd, input_buf, sizeof(input_buf));
    /* We resume execution here */
    if (button_pressed == SWKBD_BUTTON_CONFIRM) {
      write_score_to_disk(input_buf, saving_score);
      saving_score = 0;
    } else {
      saving_score = 0;
    }
  }
}

void write_score_to_disk(char *name, int score_to_save)
{

  FILE *fr = fopen("test.txt", "r");
  score_record_t *records = NULL;

  if (fr) {
    /* Read all records */
    do {
      score_record_t new_record;
      if (fscanf(fr, "%s\n", new_record.name) == EOF) break;
      if (fscanf(fr, "@%d\n", &new_record.score) == EOF) break;
      sb_push(records, new_record);
    } while(1);
    fclose(fr);
  }

  /* Add our score to the record list */
  score_record_t last_score;
  strcpy(last_score.name, name);
  last_score.score = score_to_save;
  sb_push(records, last_score);
  
  /* Reorder scores */
  for (int i = 0; i < sb_count(records); ++i) {
    for (int j = 0; j < i; ++j) {
      if (records[j].score < records[i].score) {
        score_record_t tmp = records[j];
        records[j] = records[i];
        records[i] = tmp;
      }
    }
  }

  /* Rewrite ordered scores */
  FILE *fw = fopen("test.txt", "w");
  for (int i = 0; i < sb_count(records); ++i) {
    fprintf(fw, "%s\n", records[i].name);
    fprintf(fw, "@%d\n", records[i].score);
  }
  fclose(fw);
  sb_free(records);
}
