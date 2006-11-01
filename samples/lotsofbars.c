#include <graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

int CALLBACK WinMain(HINSTANCE hI, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow) {
}

int main(void)
{
  /* select a driver and mode that supports the use */
  /* of the setrgbpalette function.                 */
  int gdriver = VGA, gmode = VGAHI, errorcode;
  struct palettetype pal;
  int i, ht, y, xmax;

  /* initialize graphics and local variables */
  initgraph(&gdriver, &gmode, "DISABLE_DEBUG");

  /* read result of initialization */
  errorcode = graphresult();
  if (errorcode != grOk)  /* an error occurred */
  {
    printf("Graphics error: %s\n", grapherrormsg(errorcode));
    printf("Press any key to halt:");
    getch();
    exit(1); /* terminate with an error code */
  }

  /* grab a copy of the palette */
  getpalette(&pal);

  /* create gray scale */
  for (i=0; i<pal.size; i++)
    setrgbpalette(pal.colors[i], i*4, i*4, i*4);

  /* display the gray scale */
  ht = getmaxy() / 16;
  xmax = getmaxx();
  y = 0;
  for (i=0; i<pal.size; i++)
  {
    setfillstyle(SOLID_FILL, i);
    bar(0, y, xmax, y+ht);
    y += ht;
  }

  /* clean up */
  getch();
  closegraph();
  return 0;
}

