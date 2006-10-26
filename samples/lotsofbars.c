/*
  BGI library sample
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
