/*
  BGI library implementation for Microsoft(R) Windows(TM)
  Copyright (C) 2006  Daniil G. Guitelson

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

#ifndef __BGI_H__
#define __BGI_H__

#include <windows.h>

#define WM_KEYPROCESSED (WM_USER+1)
#define WM_MYPALETTECHANGED (WM_USER + 2)
#define WM_VISUALPAGE_CHANGED (WM_USER + 3)
#define WM_STOP (WM_USER + 4)
#define WM_CONTINUE (WM_USER + 5)

#define WINDOW_CLASS_NAME "BGI_SERVER"
#define INVISIBLE_WINDOW_CLASS_NAME "BGI_SERVER_INVISIBLE"
#define PAGE1_NAME "BGI_PAGE1"
#define PAGE2_NAME "BGI_PAGE2"

#define KEYBOARD_MUTEX_NAME "BGI_KeyboardMutex"
#define CLIENT_PRESENT_MUTEX_NAME "BGI_clientPresenMutex"
#define SERVER_PRESENT_MUTEX_NAME "BGI_serverPresenMutex"
#define SHARED_STRUCT_NAME  "BGI_SharedStructName"
#define SERVER_STARTED_EVENT_NAME "BGI_ServerStarted"
#define PALETTE_SECTION_NAME "BGI_Palette"

#define MODE_RGB 0
#define MODE_16  1
#define MODE_FULLSCREEN 2
#define MODE_RELEASE 4
#define MODE_DEBUG 0
#define MODE_SHOW_INVISIBLE_PAGE 8

#define UPDATES_PER_SECOND 2
#define DEBUG_UPDATES_PER_SECOND 5

typedef struct 
{
  HDC dc;
  HBITMAP bmp;
  int * bits;
} PAGE;

typedef struct
{
  HANDLE keyboardEvent;
  HANDLE serverCreatedEvent;
  HANDLE clientPresentMutex;
  HANDLE serverPresentMutex;
  HANDLE pagesSection[2];
  HANDLE sharedStructSection;
  HANDLE paletteSection;
} SHARED_OBJECTS;

typedef struct
{
  int mouseX, mouseY;
  int mouseButton;
  int keyCode;
  int keyLetter;
  int visualPage;
} SHARED_STRUCT;


extern RGBQUAD * BGI_palette;
extern const char * PAGES_SECTION_NAME[2];

HINSTANCE BGI_getInstance();
int BGI_server(int width, int height, int options);
void BGI_startServer(int width, int height, int mode);
void BGI_createPage(PAGE * page, HDC dc,HANDLE section, int width, int height);
void BGI_initPalette();
PAGE * BGI_getPages(void);
int BGI_anyKeyPressed(void);
int BGI_getKeyPressed(void);
int BGI_waitForKeyPressed();
HDC BGI_getWindowDC();
HWND BGI_getWindow();
int BGI_getch();
SHARED_STRUCT * BGI_getSharedStruct();

void BGI_updateWindow(void);
void BGI_setVisualPage(int page);
void BGI_closeWindow(void);

void BGI_closeSharedObjects(SHARED_OBJECTS * objs, SHARED_STRUCT * strct);

#endif
