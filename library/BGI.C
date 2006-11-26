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
#include <Windows.h>

RGBQUAD * BGI_palette = NULL;
RGBQUAD BGI_default_palette[16] = 
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
  memcpy(BGI_palette, BGI_default_palette, sizeof(BGI_palette[0]) * 16);
}

void BGI_createPage(PAGE * page, HDC dc, HANDLE secton, int width, int height, int rgb)
{
  BITMAPINFO * bInfo;
  int size = sizeof(BITMAPINFO);
  if(!rgb)
    size += sizeof(RGBQUAD) * 16;
  bInfo = BGI_malloc(size);
  bInfo->bmiHeader.biSize = sizeof(bInfo->bmiHeader);
  bInfo->bmiHeader.biHeight = height;
  bInfo->bmiHeader.biWidth = width;
  bInfo->bmiHeader.biPlanes = 1;
  bInfo->bmiHeader.biCompression = BI_RGB;
  bInfo->bmiHeader.biSizeImage = 0;
  bInfo->bmiHeader.biXPelsPerMeter = 0;
  bInfo->bmiHeader.biYPelsPerMeter = 0;
  bInfo->bmiHeader.biClrUsed = 0;
  bInfo->bmiHeader.biClrImportant = 0;
  if(rgb)
  {
    bInfo->bmiHeader.biBitCount = 32;
  }
  else
  {
    bInfo->bmiHeader.biBitCount = 4;
    memcpy(bInfo->bmiColors, BGI_palette, sizeof(RGBQUAD) * 16);
  }
  page->bmp = CreateDIBSection(dc,bInfo,DIB_RGB_COLORS,(void **)&page->bits, secton, 0);
  page->dc = CreateCompatibleDC(dc);
  SelectObject(page->dc, page->bmp);
  BGI_free(bInfo);
}

HINSTANCE BGI_getInstance()
{
  return GetModuleHandle(NULL);
}

void * BGI_malloc(int size)
{
  static int first = 1;
  if(first) 
  {
    first = 0;
    HeapCreate(0, 1000, 10000000);
  }
  return HeapAlloc(GetProcessHeap(), 0, size);
}

void BGI_free(void * ptr)
{
  HeapFree(GetProcessHeap(), 0, ptr);
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

