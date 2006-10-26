#include <graphics.h>
#include <stdlib.h>

void _main(void)
{
  int gd = DETECT, gm = 0;
  initgraph(&gd, &gm, "");
  printf("Hello, world\n");
  while(!anykeypressed()) {
    setpalette(3, rand() % 16);
    setbkcolor(rand() % (MAXCOLORS + 1));
    setcolor(3);
    bar(rand() % 640,rand() % 480,rand() % 640,rand() % 480);
    delay(10);
  }
  closegraph();
}
