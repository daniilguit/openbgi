/*
  BGI library implementation for Microsoft(R) Windows(TM)
  Copyright (C) 2006  Daniil Guitelson

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

void main()
{
    int gd = DETECT, gm;
    int sx,sy,ex,ey,p = 0,page=0;
    g_mousestate state;
    initgraph(&gd, &gm, "");
    setwritemode(XOR_PUT);
    while(!anykeypressed())      
    {
      getmousestate(&state);
      if(state.buttons & MOUSE_LEFTBUTTON)
      {
        if(p == 0)
        {
          p = 1;
          setcolor((rand() % MAXCOLORS) + 1);
          setlinestyle(rand() % USERBIT_LINE, 0, rand() % 4);
          ex = sx = state.x;
          ey = sy = state.y;
        }
        else if(ex != state.x || ey != state.y)
        {
          line(sx, sy, ex, ey);
          ex = state.x;
          ey = state.y;
          line(sx, sy, ex, ey);
          delay(10);
        }
      }
      else
      {
        if(p)
        {
          setwritemode(COPY_PUT);
          line(sx, sy, ex, ey);
          setwritemode(XOR_PUT);
          p = 0;
        }
      }
    }
    closegraph();
}