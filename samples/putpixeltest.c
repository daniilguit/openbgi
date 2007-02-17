#include <graphics.h>

int main() 
{
    int gd = DETECT, gm = 0;
    initgraph(&gd, &gm, "");
    putpixel(0,0,GREEN);
    putpixel(getmaxx(), 0, BLUE);
    putpixel(0, getmaxy(), WHITE);
    putpixel(getmaxx(),getmaxy(),YELLOW);
    readkey();
    closegraph();
}