#include "ast3roiDS.h"

/* Globals */
player_ship_t     player_ship;
C3D_RenderTarget *top;
unsigned int      framecount; // NOTE: PRINTFRAME needs this name to be unchanged

/* Quick input summary:
   xinput          : -1 is full turn left, +1 is full turn right
   yinput          : -1 is full break, 1 is full acceleration
 */
float             xinput;
float             yinput;
float             xinput_sensitivity = 1.0f;
float             yinput_sensitivity = 0.01f;

/* Main program */
int main(int argc, char *argv[])
{
  gfxInitDefault();
  
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();
  
#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC)
  consoleInit(GFX_BOTTOM, NULL);
#endif

  top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

  init_player();

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
      switch (kDown | kHeld) {
      case KEY_CPAD_RIGHT:
        PRINTDINPUT("Pressed KEY_CPAD_RIGHT\n");
        xinput = 1.0f;
        break;
      case KEY_CPAD_DOWN:
        PRINTDINPUT("Pressed KEY_CPAD_DOWN\n");
        yinput = -1.0f;
        break;
      case KEY_CPAD_LEFT:
        PRINTDINPUT("Pressed KEY_CPAD_LEFT\n");
        xinput = -1.0f;
        break;
      case KEY_CPAD_UP:
        PRINTDINPUT("Pressed KEY_CPAD_UP\n");
        yinput = 1.0f;
        break;
      case KEY_START:
        PRINTDINPUT("Pressed KEY_START\n");
        goto exit_main_loop;
        break;
      case 0: // Pressed nothing
        break;
      default:
        PRINTFRAME; PRINTDINPUT("Pressed unmapped key\n");
        
        break;
      }

      /* Logic */
      player_logic();

      /* Rendering */
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      C2D_TargetClear(top, BLACK);
      C2D_SceneBegin(top);
      draw_player();
      C3D_FrameEnd(0);

      ++framecount;
    }

 exit_main_loop:
  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}

/* Initialize position, angle, and color of player ship */
void init_player()
{
  player_ship.x      = 50.0f;
  player_ship.y      = 50.0f;
  player_ship.yspeed = 0.0f;
  player_ship.xspeed = 0.0f;
  player_ship.angle  = 90.0f;
  player_ship.radius = 10.0f;
  player_ship.color  = WHITE;
  
  player_ship.vertices[X0] = -player_ship.radius;
  player_ship.vertices[Y0] =  player_ship.radius;
  player_ship.vertices[X1] =  0.0f;
  player_ship.vertices[Y1] = -player_ship.radius;
  player_ship.vertices[X2] =  player_ship.radius;
  player_ship.vertices[Y2] =  player_ship.radius;
}

/* Draw player ship as a triangle using 3 vertices */
void draw_player()
{
  /* float x0 = player_ship.x - 10.0f; */
  /* float x1 = player_ship.x; */
  /* float x2 = player_ship.x + 10.0f; */

  /* float y0    = player_ship.y + 10.0f; */
  /* float y1    = player_ship.y - 10.0f; */
  /* float y2    = player_ship.y + 10.0f; */

  
  u32   clr   = player_ship.color;
  float depth = 1.0f;

  
  C2D_DrawTriangle(player_ship.x + player_ship.vertices[X0],
                   player_ship.y + player_ship.vertices[Y0],
                   clr,
                   player_ship.x + player_ship.vertices[X1],
                   player_ship.y + player_ship.vertices[Y1],
                   clr,
                   player_ship.x + player_ship.vertices[X2],
                   player_ship.y + player_ship.vertices[Y2],
                   clr,
                   depth);

#ifdef DEBUG_RENDER
  PRINTFRAME; printf("DRAWING PLAYER\tX %3.3f\tY %3.3f\n", player_ship.x, player_ship.y);
#endif
}

/* Applies input and moves character */
/* TODO: Fix velocity, it doesn't work right */
void player_logic()
{

  /* Apply input sensitivity */
  xinput = xinput * xinput_sensitivity;
  yinput = yinput * yinput_sensitivity;
  
  /* Update angle based on player input */
  float old_angle = player_ship.angle;
  player_ship.angle = player_ship.angle - xinput;

  // TODO: over/underflow of angle values
  
#ifdef DEBUG_LOGIC
  PRINTFRAME; printf("PLAYER ANGLE %3.2f", player_ship.angle);
#endif

  /* Cache cos and sin for the duration of the frame */
  float fsin = sin( ((player_ship.angle)*M_PI) / 180.0f );
  float fcos = cos( ((player_ship.angle)*M_PI) / 180.0f );

  /* Save angle difference for doing rotations */
  float dfsin = sin( ((old_angle - player_ship.angle)*M_PI) / 180.0f );
  float dfcos = cos( ((old_angle - player_ship.angle)*M_PI) / 180.0f );

  /* Update speed vector */
  player_ship.yspeed = player_ship.yspeed + yinput * fsin;
  player_ship.xspeed = player_ship.xspeed + yinput * fcos;
  
  printf("XVEL %2.2f YVEL %2.2f\n", player_ship.xspeed, player_ship.yspeed);

  /* Update position */
  player_ship.x = player_ship.x + player_ship.xspeed;
  player_ship.y = player_ship.y + player_ship.yspeed;

  /* Update vertex positions for figure */
  // NOTE: Perhaps this is a good argument for switching to C++
  float x0 = player_ship.vertices[X0] * dfcos - player_ship.vertices[Y0] * dfsin;
  float x1 = player_ship.vertices[X1] * dfcos - player_ship.vertices[Y1] * dfsin;
  float x2 = player_ship.vertices[X2] * dfcos - player_ship.vertices[Y2] * dfsin;
  float y0 = player_ship.vertices[Y0] * dfcos + player_ship.vertices[X0] * dfsin;
  float y1 = player_ship.vertices[Y1] * dfcos + player_ship.vertices[X1] * dfsin;
  float y2 = player_ship.vertices[Y2] * dfcos + player_ship.vertices[X2] * dfsin;

  player_ship.vertices[X0] = x0;
  player_ship.vertices[X1] = x1;
  player_ship.vertices[X2] = x2;
  player_ship.vertices[Y0] = y0; 
  player_ship.vertices[Y1] = y1;
  player_ship.vertices[Y2] = y2;
  
}
