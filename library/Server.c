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

#include "bgi.h"
#include "IPC.h"
#include "graphics.h"
#include <assert.h>
#include <stdio.h>

#define D_PRINT(STR) MessageBox(0, STR,"MESSAGE", MB_OK)

volatile static struct
{
  int width;
  int height;
  HWND wnd;
  HINSTANCE instance;
  HDC dc;
  HANDLE thread;
  int keyCode;
} window;

static HWND invisibleWindow;
static HDC invisibleWindowDC;

static PAGE pages[2];
static SHARED_STRUCT * sharedStruct;
static SHARED_OBJECTS sharedObjects;
static int exitProcess = FALSE;

static int systemKey(int key)
{
  switch(key)
  {
  case VK_LEFT:
  case VK_RIGHT:
  case VK_UP:
  case VK_DOWN:
  case VK_HOME:
  case VK_END:
  case VK_INSERT:
  case VK_DELETE:
  case VK_NEXT:
  case VK_PRIOR:
    return 1;
  }
  if(key >= VK_F1 && key <= VK_F12)
    return 1;
  return 0;
}

static void updateWindow()
{
  BitBlt(window.dc, 0, 0, window.width, window.height, pages[sharedStruct->visualPage].dc, 0, 0, SRCCOPY);
}

static LRESULT WINAPI InvisibleWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
  case WM_PAINT:
  case WM_TIMER:
    BitBlt(invisibleWindowDC, 0, 0, window.width, window.height, pages[1 - sharedStruct->visualPage].dc, 0, 0, SRCCOPY);
    break;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}


static LRESULT WINAPI MainWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static int keyProcessed = 0;
  switch(msg)
  {
  case WM_VISUALPAGE_CHANGED:
    if(invisibleWindow != NULL)
    {
      if(sharedStruct->visualPage == 0)
        SetFocus(window.wnd);
      else
        SetFocus(invisibleWindow);
    }
    break;
  case WM_MYPALETTECHANGED:
    SetDIBColorTable(pages[0].dc, (UINT)wParam, (UINT)lParam, BGI_palette + wParam);
    SetDIBColorTable(pages[1].dc, (UINT)wParam, (UINT)lParam, BGI_palette + wParam);
    updateWindow();
    break;
  case WM_LBUTTONDOWN:
    sharedStruct->mouseButton |= MOUSE_LEFTBUTTON;
    break;
  case WM_RBUTTONDOWN:
    sharedStruct->mouseButton |= MOUSE_RIGHTBUTTON;
    break;
  case WM_LBUTTONUP:
    sharedStruct->mouseButton &= ~MOUSE_LEFTBUTTON;
    break;
  case WM_RBUTTONUP:
    sharedStruct->mouseButton &= ~MOUSE_RIGHTBUTTON;
    break;
  case WM_MOUSEMOVE:
    sharedStruct->mouseX = (int)(lParam & 0xFFFF);
    sharedStruct->mouseY = (int)(lParam >> 16);
    break;
  case WM_KEYPROCESSED:
    sharedStruct->keyCode = -1;
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_KEYDOWN:
    Sleep(1);
    if(sharedStruct->keyCode == -1)
    {
      if(systemKey((int)wParam))
      {
        if(
          wParam != VK_LSHIFT && wParam != VK_RSHIFT &&
          wParam != VK_RCONTROL && wParam != VK_LCONTROL
          )
        {
          sharedStruct->keyCode = (int)wParam;
          keyProcessed = 1;
          IPC_raiseEvent(sharedObjects.keyboardEvent);
        }
      } 
      else 
      {
          keyProcessed = 0;
      }
    }
    break;
  case WM_CHAR:
    {
      if(keyProcessed)
      {
        keyProcessed = 0;
      }
      else
      {
        sharedStruct->keyLetter = (int)wParam;
        sharedStruct->keyCode = 0;
        IPC_raiseEvent(sharedObjects.keyboardEvent);
      }
    }
    break;
  case WM_CLOSE:
    PostQuitMessage(0);
    exitProcess = TRUE;
    break;
  case WM_KEYUP:
    sharedStruct->keyCode = -1;
    break;
  case WM_TIMER:
  case WM_PAINT:
    updateWindow();
    break;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}


void registerClass(const char * className, WNDPROC wndProc)
{
  WNDCLASSEX wcx;
  wcx.cbSize = sizeof(wcx);
  wcx.style = CS_HREDRAW | CS_VREDRAW; 
  wcx.lpfnWndProc = (WNDPROC)wndProc;
  wcx.cbClsExtra = 0;
  wcx.cbWndExtra = 0;
  wcx.hInstance = BGI_getInstance();
  wcx.hIcon = NULL;
  wcx.hCursor = LoadCursor(NULL, IDC_ARROW); 
  wcx.hbrBackground = GetStockObject(WHITE_BRUSH); 
  wcx.lpszMenuName = NULL;
  wcx.lpszClassName = className;
  wcx.hIconSm = NULL;
  RegisterClassEx(&wcx);
}

/* Creates all server-side shared objects (mutexes, pages, events etc) */
void createSharedObjects(int rgb)
{
  int i;
  sharedObjects.serverCreatedEvent = IPC_openEvent(SERVER_STARTED_EVENT_NAME);
  sharedObjects.keyboardEvent = IPC_createEvent(KEYBOARD_MUTEX_NAME);
  sharedObjects.clientPresentMutex = IPC_openMutex(CLIENT_PRESENT_MUTEX_NAME);
  sharedObjects.serverPresentMutex = IPC_createMutex(SERVER_PRESENT_MUTEX_NAME, TRUE);
  sharedStruct = IPC_createSharedMemory(SHARED_STRUCT_NAME, sizeof(SHARED_STRUCT_NAME));
  BGI_palette = IPC_createSharedMemory(PALETTE_SECTION_NAME, sizeof(RGBQUAD)*16);
  BGI_initPalette();
  sharedStruct->keyCode = -1;
  for(i = 0; i != 2; i++)
  {
    sharedObjects.pagesSection[i] = IPC_createSection(PAGES_SECTION_NAME[i], window.width * window.height * 4);
    BGI_createPage(pages+ i, window.dc, sharedObjects.pagesSection[i], window.width, window.height, rgb);
  }
}

/* Procedure of thread that checks for client presence */
void clientChecker()
{
  IPC_lockMutex(sharedObjects.clientPresentMutex);
  SendMessage(window.wnd, WM_CLOSE, 0, 0);
}

void windowLoop(void)
{
  MSG msg;
  while(GetMessage(&msg,NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void initFullScreen()
{
  DEVMODE dm;
  int res;
  memset(&dm, 0, sizeof(dm));
  dm.dmSize = sizeof(dm);
  dm.dmPelsWidth = window.width;
  dm.dmPelsHeight = window.height;
  dm.dmFields = DM_PELSHEIGHT | DM_PELSWIDTH;
  res = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
}

HWND createWindow(LPCSTR className, LPCSTR title)
{
  RECT r;
  HWND result;
  SetRect(&r, 0, 0, window.width, window.height);
  AdjustWindowRect(&r, WS_CAPTION | WS_SYSMENU, FALSE);
  result = CreateWindow(className, title, WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, BGI_getInstance(), NULL);
  return result;
}

void BGI_server(DWORD param)
{
  int options = (param >> 24) & 0xFFF;
  window.width = param & 0xFFF;
  window.height = (param >> 12) & 0xFFF;
  
  registerClass(WINDOW_CLASS_NAME, &MainWindowProc);
  registerClass(INVISIBLE_WINDOW_CLASS_NAME, &InvisibleWindowProc);

  if(options & MODE_SHOW_INVISIBLE_PAGE)
  {
    invisibleWindow = createWindow(INVISIBLE_WINDOW_CLASS_NAME, "Invisible page");
    SetTimer(invisibleWindow, 0, 1000 / DEBUG_UPDATES_PER_SECOND, NULL);
    invisibleWindowDC = GetDC(invisibleWindow);
  }

  if(options & MODE_FULLSCREEN)
  {
    initFullScreen();
    window.wnd = CreateWindow(WINDOW_CLASS_NAME, "Graphics", WS_VISIBLE | WS_POPUP, 0, 0, window.width, window.height, NULL, NULL, BGI_getInstance(), NULL);
  }
  else 
  {
    window.wnd = createWindow(WINDOW_CLASS_NAME, "Graphics");
  }
  
  window.dc = GetDC(window.wnd);

  createSharedObjects(options & MODE_RGB);
  SetTimer(window.wnd, 0, 1000 / UPDATES_PER_SECOND, NULL);
  if((options & MODE_RELEASE) == 0)
  {
    exitProcess = TRUE;
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientChecker, NULL, 0, NULL);
  }
  IPC_raiseEvent(sharedObjects.serverCreatedEvent);
  windowLoop();
  BGI_closeSharedObjects(&sharedObjects, sharedStruct);
  if(exitProcess) 
    ExitProcess(0);
}


