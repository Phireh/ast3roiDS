#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

int main(int argc, char *argv[])
{
  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);
  printf("Hello, world!\n");

  while (aptMainLoop())
    {
      gspWaitForVBlank();
      gfxSwapBuffers();
      hidScanInput();

      u32 kDown = hidKeysDown();
      if (kDown & KEY_START)
        break;
    }
  gfxExit();
  return 0;
}
