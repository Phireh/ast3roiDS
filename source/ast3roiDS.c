#include "ast3roiDS.h"

/* Globals */
player_ship_t     player_ship;
asteroid_t        asteroids[ASTEROID_NUMBER];
C3D_RenderTarget *top;


/* Quick input summary:
   xinput          : -1 is full turn left, +1 is full turn right
   yinput          : -1 is full break, 1 is full acceleration
 */
float             xinput;
float             yinput;
float             xinput_sensitivity = 2.0f;
float             yinput_sensitivity = 0.01f;
unsigned int      framecount; // NOTE: needed for the time debug macros
int               game_state = NORMAL_GAMESTATE;

/* Main program */
int main(int argc, char *argv[])
{
  srand(time(NULL));
  gfxInitDefault();
  
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  // initializes console if necessary
  CHECKDEBUGMODE;

  top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

  init_player();
  for (int i = 0; i < ASTEROID_NUMBER; i++)
    init_asteroid(&asteroids[i]);
  /* Main loop */
  while (aptMainLoop())
    {
      FRAME_START_CHECKPOINT;
      PRINTFRAME;
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
        for (int i = 0; i < ASTEROID_NUMBER; i++)
          asteroid_logic(&asteroids[i]);
      } else {
        PRINTDLOGIC("Game is paused\n");
      }

      /* Rendering */
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      C2D_TargetClear(top, BLACK);
      C2D_SceneBegin(top);
      
      draw_player();

      for (int i = 0; i < ASTEROID_NUMBER; i++)
        draw_asteroid(&asteroids[i]);
      C3D_FrameEnd(0);

      ++framecount;
      FRAME_END_CHECKPOINT;
      PRINTDTIME;
      FLUSH_DEBUG_OUTPUT;
    }

 exit_main_loop:
  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}

/* Initialize player's ship attributes */
void init_player()
{
  player_ship.x      = 200.0f;
  player_ship.y      = 120.0f;
  player_ship.yspeed = 0.0f;
  player_ship.xspeed = 0.0f;
  player_ship.angle  = 90.0f;
  player_ship.radius = 10.0f;
  player_ship.color  = RED;
  
  player_ship.vertices[X0] = -player_ship.radius;        // vertex 0
  player_ship.vertices[Y0] =  player_ship.radius;
  player_ship.vertices[X1] =  0.0f;                      // vertex 1
  player_ship.vertices[Y1] = -player_ship.radius*2.0f;
  player_ship.vertices[X2] =  player_ship.radius;        // vertex 2
  player_ship.vertices[Y2] =  player_ship.radius;
}

void init_asteroid(asteroid_t *asteroid)
{
  float initial_x;
  float initial_y;


  /* Find starting position at least X px away from player in both directions */
  do {
    initial_x = RANDF(400);
  } while (ABS(initial_x - player_ship.x) < PLAYER_SAFE_ZONE_RADIUS);

  do {
    initial_y = RANDF(400);
  } while (ABS(initial_y - player_ship.y) < PLAYER_SAFE_ZONE_RADIUS);

  asteroid->x = initial_x;
  asteroid->y = initial_y;
  /* Make speed in range (-maxs,+maxs) */
  float xs = RANDF(ASTEROID_MAXSPEED * 2);
  float ys = RANDF(ASTEROID_MAXSPEED * 2);
  
  asteroid->xspeed = xs - ASTEROID_MAXSPEED;
  asteroid->yspeed = ys - ASTEROID_MAXSPEED;
  float rad = RANDF(20.0f);
  asteroid->radius = rad + 20.0f;
  asteroid->color = WHITE;

  PRINTDINIT("Asteroid initialized with x %3.2f y %3.2f xs %3.2f ys %3.2f\n",
             asteroid->x,
             asteroid->y,
             asteroid->xspeed,
             asteroid->yspeed);
}

/* Updates asteroid position */
void asteroid_logic(asteroid_t *asteroid)
{
  float old_x = asteroid->x;
  float new_x = old_x + asteroid->xspeed;

  float old_y = asteroid->y;
  float new_y = old_y + asteroid->yspeed;

  float radius = asteroid->radius;

  if (new_y > TOP_SCREEN_HEIGHT + radius) {
    new_y = - radius;
    PRINTDLOGIC("Asteroid went out of bounds downwards\n");
  } else if (new_y < 0 - radius) {
    new_y = (float) TOP_SCREEN_HEIGHT + radius;
    PRINTDLOGIC("Asteroid went out of bounds upwards\n");
  }

  if (new_x > TOP_SCREEN_WIDTH + radius) {
    new_x = - radius;
    PRINTDLOGIC("Ship went out of bounds rightwards\n");
  } else if (new_x < 0 - radius) {
    new_x = (float) TOP_SCREEN_WIDTH + radius;
    PRINTDLOGIC("Ship went out of bounds leftwards\n");
  }
  

  PRINTDLOGIC("Asteroid x %3.2f y %3.2f xs %3.2f xy %3.2f\n",asteroid->x, asteroid->y, asteroid->xspeed,asteroid->yspeed);

  asteroid->x = new_x;
  asteroid->y = new_y;
}

/* Draw asteroids on screen */
void draw_asteroid(asteroid_t *asteroid)
{
  u32 clr     = WHITE;
  float depth = 0.9f;

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

/* Draw player ship as a triangle using 3 vertices */
void draw_player()
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
