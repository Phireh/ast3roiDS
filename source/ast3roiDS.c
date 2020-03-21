#include "ast3roiDS.h"

/* Globals */
player_ship_t     player_ship;
u32               asteroidmask;
asteroid_t        asteroids[MAX_ASTEROIDS];
C3D_RenderTarget *top;
unsigned int      framecount; // NOTE: PRINTFRAME needs this name to be unchanged
unsigned int      last_hit_frame;
C2D_Text          score;
C2D_TextBuf       textBuffer;
u32               text_flags;

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
C2D_Sprite        bullet_normal_sprite;

u32               bulletmask; // NOTE: this has to have MAX_BULLETS bits
bullet_t          bullets[MAX_BULLETS];

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
      default:
        PRINTDINPUT("Error on input function\n");
        break;
      }

      if (game_state == NORMAL_GAMESTATE) {
        /* Logic */
        player_logic();
        bullet_logic();
        asteroid_logic();
        ++framecount;
      } else {
        PRINTDLOGIC("Game is paused\n");
      }

      /* Rendering */
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      C2D_TargetClear(top, BLACK);
      C2D_SceneBegin(top);

      draw_player();
      draw_bullets();
      draw_asteroids();
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

  C2D_SpriteFromSheet(&bullet_normal_sprite, bullet_spritesheet, SPRITE_BULLET_NORMAL);
  C2D_SpriteSetCenter(&bullet_normal_sprite, 0.5f, 0.5f);

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

  // TODO: make this load a second, different sprite
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
      initial_x = RANDF(400);
      initial_y = RANDF(240);
    } while (inside_circle(initial_x, initial_y, player_ship.x, player_ship.y, PLAYER_SAFE_ZONE_RADIUS));
    asteroid->x = initial_x;
    asteroid->y = initial_y;
    
    /* Make speed in range (-maxs,+maxs) */
    float xs = RANDF(ASTEROID_MAXSPEED * 2);
    float ys = RANDF(ASTEROID_MAXSPEED * 2);
  
    asteroid->xspeed = xs - ASTEROID_MAXSPEED;
    asteroid->yspeed = ys - ASTEROID_MAXSPEED;
    float rad = RANDF(20.0f);
    asteroid->radius = rad + (MAX_ASTEROID_SIZE/2);
    asteroid->color = WHITE;
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

      asteroid->x = new_x;
      asteroid->y = new_y;
    }
  }
}


/* Draw asteroids on screen */
void draw_asteroids(void)
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
  float old_angle = player_ship.angle;
  float new_angle = old_angle - xinput;
  
  // Keep angles in [0,360] range
  if (new_angle > 360.0f)
    new_angle -= 360.0f;
  if (new_angle < 0.0f)
    new_angle += 360.0f;

  player_ship.angle = new_angle;

  PRINTDLOGIC("PLAYER ANGLE %3.2f", player_ship.angle);

  /* Cache cos and sin for the duration of the frame */
  float fsin = sin( ((new_angle)*M_PI) / 180.0f );
  float fcos = cos( ((new_angle)*M_PI) / 180.0f );

  /* Save angle difference for doing rotations */
  float dfsin = sin( ((old_angle - new_angle)*M_PI) / 180.0f );
  float dfcos = cos( ((old_angle - new_angle)*M_PI) / 180.0f );

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
  // NOTE: Perhaps this is a good argument for switching to C++
  float x0 = player_ship.vertices[X0] * dfcos - player_ship.vertices[Y0] * dfsin; // vertex 0
  float y0 = player_ship.vertices[Y0] * dfcos + player_ship.vertices[X0] * dfsin;  
  float x1 = player_ship.vertices[X1] * dfcos - player_ship.vertices[Y1] * dfsin; // vertex 1
  float y1 = player_ship.vertices[Y1] * dfcos + player_ship.vertices[X1] * dfsin;  
  float x2 = player_ship.vertices[X2] * dfcos - player_ship.vertices[Y2] * dfsin; // vertex 2
  float y2 = player_ship.vertices[Y2] * dfcos + player_ship.vertices[X2] * dfsin;  





  player_ship.vertices[X0] = x0; // vertex 0
  player_ship.vertices[Y0] = y0; 
  player_ship.vertices[X1] = x1; // vertex 1
  player_ship.vertices[Y1] = y1;  
  player_ship.vertices[X2] = x2; // vertex 2
  player_ship.vertices[Y2] = y2;
  
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
  float fsin = sin( ((angle)*M_PI) / 180.0f );
  float fcos = cos( ((angle)*M_PI) / 180.0f );


  bullets[i].xspeed = BULLET_INITIAL_SPEED * fcos;
  bullets[i].yspeed = BULLET_INITIAL_SPEED * -fsin;
  bullets[i].sprite = &bullet_normal_sprite;
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
      C2D_SpriteSetRotation(bullets[i].sprite, C3D_AngleFromDegrees(-bullets[i].angle+90.0f));
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
    float xs = RANDF(ASTEROID_MAXSPEED * 2);
    float ys = RANDF(ASTEROID_MAXSPEED * 2);

    asteroid->xspeed = xs - ASTEROID_MAXSPEED;
    asteroid->yspeed = ys - ASTEROID_MAXSPEED;
    
    float rad = 0.0f;
    if (size == ASTEROID_SIZE_BIG) {
      rad = MAX_ASTEROID_SIZE*ASTEROID_BIG_RATIO;
    } else if (size == ASTEROID_SIZE_NORMAL) {
      rad = MAX_ASTEROID_SIZE*ASTEROID_NORMAL_RATIO;
    } else if (size == ASTEROID_SIZE_SMALL) {
      rad = MAX_ASTEROID_SIZE*ASTEROID_SMALL_RATIO;
    }
    asteroid->radius = rad;
    asteroid->color = WHITE;
  }
}

void draw_score()
{
  size_t s = strlen("Score: ");
  textBuffer = C2D_TextBufNew(s);
  C2D_TextParse(&score, textBuffer, "Score: ");
  C2D_DrawText(&score, text_flags, 5.0f, 5.0f, 0.0f, 1.0f, 1.0f);
}

void reset_game(void)
{
  asteroidmask = 0;
  bulletmask = 0;
  init_player();
  init_asteroids(ASTEROID_NUMBER);
  framecount = 0;
  last_hit_frame = 0;
}
