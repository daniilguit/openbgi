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

#include "BGI.H"
#include "IPC.h"
#include <stdio.h>

static HINSTANCE hInstance;

RGBQUAD * BGI_palette = NULL;
RGBQUAD BGI_palette_ [16] = 
{
  {0,0,0},
  {128,0,0},
  {0,128,0},
  {255,128,0},
  {0,0,128},
  {128,0,128},
  {0,64,128},
  {192,192,192},
  {128,128,128},
  {255,0,0},
  {0,255,0},
  {255,255,0},
  {0,0,192},
  {128,0,255},
  {0,255,255},
  {255,255,255}
};

const char * PAGES_SECTION_NAME[2] = {"BGI_PAGE1_SECTION", "BGI_PAGE2_SECTION"};

void BGI_initPalette()
{
   int i;
   for(i = 0; i != 16; i++)
      BGI_palette[i] = BGI_palette_[i];
}

void BGI_createPage(PAGE * page, HDC dc, HANDLE secton, int width, int height)
{
  BITMAPINFO * bInfo;
  int size = sizeof(BITMAPINFO);
  //if(mode == MODE_16)
  size += sizeof(RGBQUAD) * 16;
  bInfo = malloc(size);
  bInfo->bmiHeader.biSize = sizeof(bInfo->bmiHeader);
  bInfo->bmiHeader.biHeight = height;
  bInfo->bmiHeader.biWidth = width;
  bInfo->bmiHeader.biPlanes = 1;
  bInfo->bmiHeader.biBitCount = 4;
  bInfo->bmiHeader.biCompression = BI_RGB;
  bInfo->bmiHeader.biSizeImage = 0;
  bInfo->bmiHeader.biXPelsPerMeter = 0;
  bInfo->bmiHeader.biYPelsPerMeter = 0;
  bInfo->bmiHeader.biClrUsed = 0;
  bInfo->bmiHeader.biClrImportant = 0;
  memcpy(bInfo->bmiColors, BGI_palette, sizeof(RGBQUAD) * 16);
  page->bmp = CreateDIBSection(dc,bInfo,DIB_RGB_COLORS,(void **)&page->bits, secton, 0);
  page->dc = CreateCompatibleDC(dc);
  SelectObject(page->dc, page->bmp);
  free(bInfo);
}

HINSTANCE BGI_getInstance()
{
  return GetModuleHandle(NULL);
}

void BGI_closeSharedObjects(SHARED_OBJECTS * sharedObjects, SHARED_STRUCT * sharedStruct)
{
  IPC_closeSharedMemory(sharedStruct);
  IPC_closeSharedMemory(BGI_palette);
  CloseHandle(sharedObjects->keyboardEvent);
  CloseHandle(sharedObjects->serverPresentMutex);
  CloseHandle(sharedObjects->clientPresentMutex);
  CloseHandle(sharedObjects->serverCreatedEvent);
}

