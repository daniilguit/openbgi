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

#ifndef _MSC_VER
  #define _WIN_VER 0x500
#endif
#include <windows.h>

#define MODE_16  0
#define MODE_RGB 1
#define MODE_FULLSCREEN 2
#define MODE_RELEASE 4
#define MODE_DEBUG 0
#define MODE_SHOW_INVISIBLE_PAGE 8

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

#define UPDATES_PER_SECOND 2
#define DEBUG_UPDATES_PER_SECOND 5


/**
 * Represents off-screen DIB image
 */
typedef struct 
{
  HDC dc;
  HBITMAP bmp;
  int * bits;
} PAGE;

/**
 * Stores handles for all IPC objects
 */
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

/**
 * Structure that is shared between two processes
 */
typedef struct
{
  int mouseX, mouseY;
  int mouseButton;
  int keyCode;
  int keyLetter;
  int visualPage;
} SHARED_STRUCT;

/* Palette that is shared between processes */
extern RGBQUAD * BGI_palette; 
/* Default palette values */
extern RGBQUAD BGI_default_palette[];
/* Names for shared-objects that share DIB-sections */
extern const char * PAGES_SECTION_NAME[2];

/* malloc replacements. needed for `server` process */
void * BGI_malloc(int size);
/* free replacements. needed for `server` process */
void BGI_free(void * ptr);

/* Returns HINSTANCE of current process */
HINSTANCE BGI_getInstance();
/* Main server procedure */
void BGI_server(DWORD param);
/* Runs `server` process */
void BGI_startServer(int width, int height, int mode);
/* Creates page (DIB-section) */
void BGI_createPage(PAGE * page, HDC dc,HANDLE section, int width, int height, int rgb);
/* initialize palette with default values */
void BGI_initPalette();
/* returns array of 2 shared pages */
PAGE * BGI_getPages(void);
/* If any key is pressed */
int BGI_anyKeyPressed(void);
/* Block current thread until user pressed some key and returns it */
int BGI_waitForKeyPressed();
/* Returns HDC of server-window */
HDC BGI_getWindowDC();
/* Returns HWND of server-window */
HWND BGI_getWindow();
/* Blocks current thread until user pressed some key and returns it */
int BGI_getch();
/* Returns structure that contains shared date */
SHARED_STRUCT * BGI_getSharedStruct();
/* Asks server window to redraw it`s content */
void BGI_updateWindow(void);
/* Changes server active window */
void BGI_setVisualPage(int page);
/* Stop server */
void BGI_closeWindow(void);
/* Destroy all shared objects */
void BGI_closeSharedObjects(SHARED_OBJECTS * objs, SHARED_STRUCT * strct);

#endif
