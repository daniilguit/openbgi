#include <graphics.h>
#include <assert.h>

int main() 
{
    int gd = DETECT, gm = 0;
    initgraph(&gd, &gm, "");
    outtext("8-bit mode test");
    putpixel(0,0,GREEN);
    putpixel(getmaxx(), 0, BLUE);
    putpixel(0, getmaxy(), WHITE);
    putpixel(getmaxx(),getmaxy(),YELLOW);
    putpixel(getmaxx() / 2,getmaxy() / 2,BROWN);
    assert(getpixel(0,0) == GREEN);
    assert(getpixel(getmaxx(), 0) == BLUE);
    assert(getpixel(0, getmaxy()) == WHITE);
    assert(getpixel(getmaxx(), getmaxy()) == YELLOW);
    assert(getpixel(getmaxx() / 2, getmaxy() / 2) == BROWN);
    readkey();
    closegraph();
    initgraph(&gd, &gm, "RGB");
    outtext("RGB mode test");
    putpixel(0,0,GREEN);
    putpixel(getmaxx(), 0, BLUE);
    putpixel(0, getmaxy(), WHITE);
    putpixel(getmaxx(),getmaxy(),YELLOW);
    putpixel(getmaxx() / 2,getmaxy() / 2,BROWN);
    assert(getpixel(0,0) == GREEN);
    assert(getpixel(getmaxx(), 0) == BLUE);
    assert(getpixel(0, getmaxy()) == WHITE);
    assert(getpixel(getmaxx(), getmaxy()) == YELLOW);
    assert(getpixel(getmaxx() / 2, getmaxy() / 2) == BROWN);
    readkey();
    closegraph();
}