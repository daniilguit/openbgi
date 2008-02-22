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

#ifndef _MSC_VER
  #define _WIN_VER 0x500
#endif

#include <Windows.h>

#include "BGI.h"
#include "graphics.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

enum WhatChanged
{
  CHANGED_STYLE = 1,
  CHANGED_WIDTH = 2,
  CHANGED_COLOR = 4,
  CHANGED_ALL = 0xF
};

#define DEG_TO_RAD(X)  ((X) * M_PI / 180.0)
typedef void (*putpixelProc_t)(int x, int y, int color);

static g_pointtype modeResolution[] = {{640,200}, {640,350}, {640,480}, {640,480}, {800,600},{1024,768}};

static int lastMeasuredTime = 0;
static int frameCounter = 0;
static int FPS = 0;
static int XORMode = 0;

static HDC windowDC;
static int windowWidth;
static int windowHeight;
static int length;
static PAGE * pages;
static int activePageIndex = 0;
static unsigned char * activeBits;
static HDC activeDC;
static SHARED_STRUCT * sharedStruct;

static HBRUSH stdBrushes[USER_FILL + 1];
static HPEN stdPens[USERBIT_LINE];

static struct 
{
  double x;
  double y;
} aspectRatio = {1, 1};

static int graphMode = -1;
static int rgbMode = 0;

static COLORREF builtinPalette[MAXCOLORS];
static HBRUSH backBrush;
static HBRUSH currentBrush;
static g_pointtype currentPosition;
static int backColor = _BLACK;
static int penColor = _WHITE;

static g_arccoordstype arcCoords;

static g_palettetype palette;
static g_linesettingstype lineSettings;
static g_textsettingstype textSetting = {LEFT_TEXT, TOP_TEXT, 1};
static g_fillsettingstype fillSettings;
static g_viewporttype viewPort;
static struct
{
  double mx, my;
} userSize = {1, 1};

static unsigned short patternsBits[USER_FILL + 1][8] = 
{
  {255, 255, 255, 255, 255, 255, 255, 255},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 255, 255, 0, 0, 255, 255},
  {254, 253, 251, 247, 239, 223, 191, 127},
  {124, 248, 241, 227, 199, 143, 31, 62},
  {62, 31, 143, 199, 227, 241, 248, 124},
  {127, 191, 223, 239, 247, 251, 253, 254},
  {0, 119, 119, 119, 0, 119, 119, 119},
  {126, 189, 219, 231, 231, 219, 189, 126},
  {51, 51, 204, 204, 51, 51, 204, 204},
  {223, 255, 253, 255, 223, 255, 253, 255},
  {221, 119, 221, 119, 221, 119, 221, 119},
};

static COLORREF translateColor(int color)
{
  if(rgbMode)
    return (COLORREF)RGB(color >> 16, (color >> 8) & 0xFF, color & 0xFF);
  return builtinPalette[palette.colors[color % MAXCOLORS]];
}

static void selectObject(HANDLE object, int del)
{
  HANDLE oldObject = SelectObject(pages[0].dc, object);
  SelectObject(pages[1].dc, object);
  SelectObject(windowDC, object);
  if(del)
    DeleteObject(oldObject);
}

static void initBrushes()
{
  int i,j;
  for(i = 0; i != USER_FILL; i++)
  {
    for(j = 0; j != 8; j++)
      patternsBits[i][j] = ((unsigned char)patternsBits[i][j]);
    stdBrushes[i] = CreatePatternBrush(CreateBitmap(8,8,1,1,(LPBYTE)patternsBits[i]));
  }
}

static void endDraw()
{
  if(activePageIndex == sharedStruct->visualPage)
  {
    BGI_updateWindow();
  }
}

#define CHECK_COLOR_RANGE(COLOR) if(!rgbMode && (COLOR < 0 || COLOR >= MAXCOLORS)) return;
#define CHECK_GRAPHCS_INITED if(graphMode == -1) return;
#define ICHECK_GRAPHCS_INITED if(graphMode == -1) return -1;

#define BEGIN_DRAW  CHECK_GRAPHCS_INITED
#define END_DRAW   endDraw();

#define BEGIN_FILL { COLORREF c = translateColor(fillSettings.color); BEGIN_DRAW SetTextColor(activeDC, c);}
#define END_FILL { COLORREF c = penColor; BEGIN_DRAW SetTextColor(activeDC, c); END_DRAW  }

static void setRect(RECT * r, int x1, int y1, int x2, int y2)
{
  assert(r != NULL);
  r->left = x1;
  r->right = x2;
  r->top = y1;
  r->bottom = y2;
}

static void setWriteMode()
{
  int op;
  op = XORMode ? R2_XORPEN : R2_COPYPEN; 
  if(XORMode)
  {
    SetROP2(pages[0].dc, op);
    SetROP2(pages[1].dc, op);
    SetROP2(windowDC, op);
  }
}

static void unsetWriteMode()
{
  SetROP2(pages[0].dc, R2_COPYPEN);
  SetROP2(pages[1].dc, R2_COPYPEN);
  SetROP2(windowDC, R2_COPYPEN);
}

static void updatePosition(int x, int y)
{
  currentPosition.x = x;
  currentPosition.y = y;
  MoveToEx(
    activeDC, 
    (currentPosition.x), 
    (currentPosition.y),
    NULL
    );
  MoveToEx(
    windowDC, 
    (currentPosition.x), 
    (currentPosition.y),
    NULL
    );
}

static void retrivePosition()
{
  MoveToEx(activeDC, 0, 0, (POINT *)(void *)&currentPosition);
  MoveToEx(activeDC, currentPosition.x, currentPosition.y, NULL);
  
  currentPosition.x = (currentPosition.x);
  currentPosition.y = (currentPosition.y);
}

static int convertToBits(DWORD bits[32], int pattern)
{
  int i = 0, j;
  pattern &= 0xFFFF;

  for(;;)
  { 
    for (j = 0; pattern & 1; j++) pattern >>= 1;
    bits[i++] = j;
    if (pattern == 0) 
    { 
      bits[i++] = 16 - j;
      return i;
    }
    for (j = 0; !(pattern & 1); j++) 
      pattern >>= 1;
    bits[i++] = j;
  }
}

static void updatePen(int whatChanged)
{
  COLORREF c = translateColor(penColor);
  LOGBRUSH br;
  DWORD bits[32] = {0};

  if(whatChanged & CHANGED_COLOR || whatChanged & CHANGED_STYLE || whatChanged & CHANGED_WIDTH)
  {
    HPEN pen;
    switch(lineSettings.linestyle)
    {
    case SOLID_LINE:
      pen = CreatePen(PS_SOLID, lineSettings.thickness, c);
      break;
    case DOTTED_LINE:
      pen = CreatePen(PS_DOT, lineSettings.thickness, c);
      break;
    case CENTER_LINE:
    case DASHED_LINE:
      pen = CreatePen(PS_DASH, lineSettings.thickness, c);
      break;
    case USER_FILL:
      br.lbColor = c;
      br.lbStyle = BS_SOLID;
      c = convertToBits(bits, lineSettings.upattern);
      pen = ExtCreatePen(
              PS_USERSTYLE | PS_GEOMETRIC,
              lineSettings.thickness,
              &br,
              c,
              bits
              );      
    }
    selectObject(pen, 1);
  }

  if(whatChanged & CHANGED_COLOR)
  {
    SetTextColor(pages[0].dc, c);
    SetTextColor(pages[1].dc, c);
    SetTextColor(windowDC, c);
  }
}

static void updateBrush(int whatChanged)
{
  COLORREF c = translateColor(fillSettings.color);
  if(whatChanged & CHANGED_STYLE)
  {
  	currentBrush = stdBrushes[fillSettings.pattern];
    selectObject(stdBrushes[fillSettings.pattern], 0);
  }
  if(whatChanged & CHANGED_COLOR)
  {
    SetBkColor(pages[0].dc, c);
    SetBkColor(pages[1].dc, c);
    SetBkColor(windowDC, c);
  }
}

static void updateFont()
{
  int opt = 0;
  LOGFONT lf;
  ZeroMemory(&lf, sizeof(lf));
#if _MSC_VER >= 1400
  strcpy_s(lf.lfFaceName, sizeof(lf.lfFaceName), "Lucida Console");
#else
  strcpy(lf.lfFaceName,"Lucida Console");
#endif
  if(textSetting.direction == VERT_DIR)
    lf.lfEscapement = 900;
  lf.lfWeight = (int)((textSetting.charsize + 8) * userSize.mx);
  lf.lfHeight = (int)((textSetting.charsize + 10) * userSize.my);
  selectObject(CreateFontIndirect(&lf), 1);
  
  if(textSetting.direction == HORIZ_DIR && textSetting.horiz == LEFT_TEXT)
    opt = TA_UPDATECP;

  switch(textSetting.horiz)
  {
  case LEFT_TEXT:
  case RIGHT_TEXT:
    opt |= textSetting.horiz;
    break;
  case CENTER_TEXT:
    opt |= TA_CENTER;
  }
  switch(textSetting.vert)
  {
  case TOP_TEXT:
  case BOTTOM_TEXT:
    opt |= textSetting.vert;
    break;
  case CENTER_TEXT:
    opt |= TA_CENTER;
  }
  SetTextAlign(pages[0].dc, opt);
  SetTextAlign(pages[1].dc, opt);
}

static void updateViewport()
{
  if(viewPort.clip)
  {
	  SetViewportOrgEx(pages[0].dc, viewPort.left, viewPort.top, NULL);
	  SetViewportOrgEx(pages[1].dc, viewPort.left, viewPort.top, NULL);
    selectObject(
      CreateRectRgn(
        viewPort.left, 
        viewPort.top, 
        viewPort.right,
        viewPort.bottom
        ),
        1
      );
  }
}

static void initPallette()
{
  int i;
  for(i = 0; i != MAXCOLORS; i++)
    builtinPalette[i] = RGB(BGI_palette[i].rgbRed, BGI_palette[i].rgbGreen, BGI_palette[i].rgbBlue);
  for(i = 0; i != MAXCOLORS; i++)
    palette.colors[i] = i;
  palette.size = MAXCOLORS;
}

/** 
 * Initialize graphics mode 
 *
 * @param gd must contains DETECT or VGA (they are equivalent) 
 * @param gm can be VGAHI, VGALO, VGAMED , they has width and height
             same as it had in dos BGI. Also there are extension modes:
             GM_800x600, GM_1024x768 (it has meaningful names)
 * @param path can be empty, since no old BGI driver are used 
 *             but, it can contains some extension substrings
 *             (that are tested by strstr so you can use any delimiter
 *              not use delimiters at all)
 *             "RGB" - use rgb mode instead of 16 colors
 *             "SHOW_INVISIBLE_PAGE" - two graphics window will be 
 *                                     present on the screen : normal and
 *                                     another, that always show 'invisible' page
 *             "FULL_SCREEN" - set full screen (for example for games).
 *
 */
void initgraph(int * gd, int * gm, const char * path)
{
  int options = 0;
  if(graphMode != -1) 
    closegraph();
  graphMode = *gm;
  if(*gd == DETECT)
  {
    graphMode = VGAHI;
  }
  if(graphMode < 0 || (graphMode > GM_1024x768 && *gd != CUSTOM))
  {
    graphMode = -1;
    return;
  }
  if(strstr(path, "SHOW_INVISIBLE_PAGE") != NULL)
    options |= MODE_SHOW_INVISIBLE_PAGE;
  if(strstr(path, "DISABLE_DEBUG") != NULL)
    options |= MODE_RELEASE;
  else 
    options |= MODE_DEBUG;
  if(strstr(path, "RGB") != NULL)
  {
    options |= MODE_RGB;
    rgbMode = 1;
  }
  if(strstr(path, "FULL_SCREEN") != NULL && *gd != CUSTOM)
    options |= MODE_FULLSCREEN;
  if(*gd == CUSTOM) {
    windowWidth = *gm & 0xFFFF;
    windowHeight = *gm >> 16;
  } else {
    windowWidth = modeResolution[graphMode].x;
    windowHeight = modeResolution[graphMode].y;
  }

  length = windowWidth * windowHeight / 2;
  fillSettings.pattern = SOLID_FILL;
  if(options & MODE_RGB) {
    penColor = RGB(255,255,255);
    fillSettings.color = RGB(255, 255, 255);
  }
  else {
    fillSettings.color = penColor = getmaxcolor();
  }
  
  BGI_startServer(windowWidth, windowHeight, options);
  
  windowDC = BGI_getWindowDC();
  pages = BGI_getPages();
  initPallette();

  memset(&viewPort, 0, sizeof(viewPort));
  viewPort.right = getmaxx();
  viewPort.bottom = getmaxy();

  backBrush = CreateSolidBrush(0);
  sharedStruct = BGI_getSharedStruct();
  
  SetBkMode(pages[0].dc, TRANSPARENT);
  SetBkMode(pages[1].dc, TRANSPARENT);
  
  setactivepage(0);
  setvisualpage(0);
  initBrushes();
  setbkcolor(backColor);

  cleardevice();
  updateBrush(CHANGED_ALL);
  updatePen(CHANGED_ALL);
  updateFont();
  updatePosition(0,0);
}

static void lineto_(int x, int y)
{
  LineTo(activeDC, x, y);
  SetPixelV(activeDC, x, y, translateColor(penColor));
}

void arc(int x, int y, int stangle, int endangle, int radius)
{
  BEGIN_DRAW
    moveto( 
    (int)(x + radius * cos(DEG_TO_RAD(stangle))), 
    (int)(y - radius * sin(DEG_TO_RAD(stangle)))
    );

  AngleArc(
    activeDC,
    (x),
    (y),
    radius,
    (FLOAT)stangle, 
    (FLOAT)(endangle - stangle)
    );
  END_DRAW
  /*arcCoords.x = x;
  arcCoords.y = y;
  stangle *= M_PI / 360;
  endangle *= M_PI / 360;
  arcCoords.xstart = x + radius * cos(stangle);
  arcCoords.ystart = y + radius * sin(stangle);
  arcCoords.xend = x + radius * cos(endangle);
  arcCoords.yend = y + radius * sin(endangle);*/
}


void  bar_(int left, int top, int right, int bottom)
{
  RECT r;
  r.left = (left);
  r.top = (top);
  r.right = (right + 1);
  r.bottom = (bottom + 1);
  FillRect(activeDC, &r, currentBrush); 
}

void  bar(int left, int top, int right, int bottom)
{
  BEGIN_FILL
    bar_(left, top, right, bottom); 
  END_FILL
}

void  bar3d(int left, int top, int right, int bottom, int depth, int topflag)
{
  int hdep = depth * 3 / 5;
  BEGIN_FILL
  	bar_(left, top, right, bottom);
    if(topflag) 
    {
      moveto(left, top);
      lineto_(left + depth,top - hdep);
      lineto_(right + depth, top - hdep);
      lineto_(right + depth, bottom - hdep);
      lineto_(right, bottom);
      moveto(right + depth, top - hdep);
      lineto_(right, top);
    }
  END_FILL
}

void circle_(HDC dc, int x, int y, int radius)
{
  BEGIN_DRAW
    Arc(
      dc, 
      (x - radius),
      (y - radius),
      (x + radius),
      (y + radius),
      0, 0, 0, 0
      );
  END_DRAW
}

void  circle(int x, int y, int radius)
{
  circle_(activeDC, x, y, radius);
  if(activePageIndex == sharedStruct->visualPage)
    circle_(windowDC, x, y, radius);
}

void  cleardevice(void)
{
  RECT r;
  setRect(&r, 0, 0, windowWidth + 1, windowHeight + 1);
  BEGIN_FILL
    FillRect(activeDC, &r, backBrush);
  END_FILL
}

void  clearviewport(void)
{
  RECT r;
  setRect(&r, viewPort.left, viewPort.top, viewPort.right + 1, viewPort.bottom + 1);
  BEGIN_DRAW
    FillRect(activeDC, &r, backBrush);
  END_DRAW
}

void  closegraph(void)
{
  if(graphMode != -1)
  {
    BGI_closeWindow();
    graphMode = -1;
  }
  //SetFocus(GetConsoleWindow());
}

void  detectgraph(int  *graphdriver,int  *graphmode)
{
  *graphdriver = DETECT;
  *graphmode = VGAHI;
}

void  drawpoly(int numpoints, const int  *polypoints)
{
  int i;
  POINT * points;
  CHECK_GRAPHCS_INITED
  points = malloc(numpoints * sizeof(POINT));
  for(i = 0; i != numpoints; i++)
  {
    points[i].x = (*polypoints++);
    points[i].y = (*polypoints++);
  }
  BEGIN_DRAW
    Polyline(activeDC, points, numpoints);
  END_DRAW
  free(points);
}

void ellipse_(HDC dc, int x, int y, int stangle, int endangle, int xradius, int yradius)
{
  Arc(
    dc,
    (x - xradius), 
    (y - yradius),
    (x + xradius),
    (y + yradius),
    (x + xradius * cos(DEG_TO_RAD(stangle))),
    (y - yradius * sin(DEG_TO_RAD(stangle))),
    (x + xradius * cos(DEG_TO_RAD(endangle))),
    (y - yradius * sin(DEG_TO_RAD(endangle)))
    );
}
void  ellipse(int x, int y, int stangle, int endangle, int xradius, int yradius)
{
  BEGIN_DRAW
    ellipse_(activeDC, x, y, stangle, endangle, xradius, yradius);
  END_DRAW
  if(activePageIndex == sharedStruct->visualPage)
    ellipse_(windowDC, x, y, stangle, endangle, xradius, yradius);
  arcCoords.x = x;
  arcCoords.y = y;
}

void  fillellipse( int x, int y, int xradius, int yradius )
{
  BEGIN_FILL
      Ellipse(
        activeDC,
        (x - xradius),
        (y - yradius),
        (x + xradius),
        (y + yradius)
        );
  END_FILL
}

void  fillpoly(int numpoints, const int  *polypoints)
{
  int i;
  POINT * points;
  CHECK_GRAPHCS_INITED
  points = malloc(numpoints * sizeof(POINT));
  for(i = 0; i != numpoints; i++)
  {
    points[i].x = (*polypoints++);
    points[i].y = (*polypoints++);
  }
  BEGIN_FILL
      Polygon(activeDC, points, numpoints);
  END_FILL
  free(points);
}

void  floodfill(int x, int y, int border)
{
  BEGIN_FILL
     ExtFloodFill(activeDC, x, y, translateColor(border), FLOODFILLBORDER);
  END_FILL
}

void  getarccoords(g_arccoordstype  *arccoords)
{
  *arccoords = arcCoords;
}

void  getaspectratio(int  *xasp, int  *yasp)
{
  *xasp = (int)(aspectRatio.x * windowWidth);
  *yasp = (int)(aspectRatio.y * windowHeight);
}

int getbkcolor(void)
{
  return backColor;
}
int getcolor(void)
{
  return penColor;
}

g_palettetype * getdefaultpalette( void )
{
  return &palette;
}

void  getfillpattern(char  *pattern)
{
  int i;
  for(i = 0; i != 8; i++)
    pattern[i] = (char)patternsBits[USER_FILL][i];
}

void  getfillsettings(g_fillsettingstype  *fillinfo)
{
  *fillinfo = fillSettings;
}

int getfps()
{
  return FPS;
}

int getgraphmode(void)
{
  return graphMode;
}

void  getimage(int left, int top, int right, int bottom,void  *bitmap)
{
  int x, y;
  int * bits = (int *) bitmap;
  int width = right - left;
  int height = bottom - top;
  int c;
  *bits++ = width;
  *bits++ = height;
  for(y = top; y <= bottom; y++)
    for(x = left; x <= right ; x++)
      *bits++ = c = getpixel(x, y);
}

void  getlinesettings(g_linesettingstype  *lineinfo)
{
  *lineinfo = lineSettings;
}

int getmaxcolor(void)
{
  return MAXCOLORS - 1;
}

int getmaxmode(void)
{
  return GM_1024x768;
}

int getmaxx(void)
{
  return windowWidth - 1;
}

int getmaxy(void)
{
  return windowHeight - 1;
}

char *  getmodename( int mode_number )
{
  switch(mode_number)
  {
  case VGALO:
    return "VGA_LO";
  case VGAHI:
    return "VGA_HI";
  case VGAMED:
    return "VGA_MED";
  case GM_640x480:
    return "640x480";
  case GM_800x600:
    return "800x600";
  case GM_1024x768:
    return "1024x768";
  }
  return "";
}

void  getmoderange(int graphdriver, int  *lomode, int *himode)
{
  (void)graphdriver; // this prevents waring of unused graphdriver
                     // it presents only for compatibility with Borland
                     // implementation
  *lomode = VGALO;
  *himode = GM_1024x768;
}

unsigned getpixel(int x, int y)
{
  x = (x);
  y = (y);
  if(rgbMode) {
    return ((unsigned *)activeBits)[x + (windowHeight - y - 1) * windowWidth];
  } else {
    int index = (x) + (windowHeight - (y) - 1) * windowWidth;
    int delta = index % 2 ? 0 : 4;
    if(x >= 0 && x < windowWidth && y >= 0 && y < windowHeight) {
      return (activeBits[index / 2] & (0xF << delta)) >> delta;
    }
  }
  return 0;
}

void  getpalette(g_palettetype  * _palette)
{
  memcpy(_palette, &palette, sizeof(palette));
}

int getpalettesize( void )
{
  return sizeof(palette);
}

void  gettextsettings(g_textsettingstype  *texttypeinfo)
{
  *texttypeinfo = textSetting;
}

void  getviewsettings(g_viewporttype  *viewport)
{
  *viewport = viewPort;
}

int getx(void)
{
  return currentPosition.x;
}

int gety(void)
{
  return currentPosition.y;
}

char *  grapherrormsg(int errorcode)
{
  if(errorcode == grNoInitGraph)
    return "Graphics in not initialized";
  return "OK";
}

int graphresult(void)
{
  return graphMode != -1 ? grOk : grNoInitGraph;
}

unsigned imagesize(int left, int top, int right, int bottom)
{
  return (abs((right - left + 1) * (bottom - top + 1))  + 2) * sizeof(int);
}

void lineto__(HDC dc, int x, int y)
{
  LineTo(dc, (x), (y));
}
void line_(HDC dc, int x1, int y1, int x2, int y2)
{
  MoveToEx(dc, (x1), (y1), NULL);
  lineto__(dc, x2, y2);
  SetPixelV(dc, (x2), (y2), translateColor(penColor));
}

#define BEGIN_LINEDRAW setWriteMode();
#define END_LINEDRAW unsetWriteMode(); 

void  line(int x1, int y1, int x2, int y2)
{
  BEGIN_LINEDRAW
    line_(activeDC, x1, y1, x2, y2);
    if(sharedStruct->visualPage == activePageIndex)
      line_(windowDC, x1, y1, x2, y2);
  END_LINEDRAW
  currentPosition.x = x2;
  currentPosition.y = y2;
}

void  linerel(int dx, int dy)
{
  BEGIN_LINEDRAW
    lineto__(activeDC, currentPosition.x + dx, currentPosition.y + dy);
    if(sharedStruct->visualPage == activePageIndex)
      lineto__(windowDC, currentPosition.x += dx, currentPosition.y += dy);
  END_LINEDRAW
}

void  lineto(int x, int y)
{
  BEGIN_LINEDRAW
    lineto__(activeDC, x, y);
    if(sharedStruct->visualPage == activePageIndex)
      lineto__(windowDC, x, y);
  END_LINEDRAW
  currentPosition.x = x;
  currentPosition.y = y;
}

void  moverel(int dx, int dy)
{
  CHECK_GRAPHCS_INITED
  moveto(currentPosition.x += dx, currentPosition.y += dy);
}
void  moveto(int x, int y)
{
  CHECK_GRAPHCS_INITED
  updatePosition(x, y);
}

void  outtext(const char  *textstring)
{
  BEGIN_DRAW
    outtextxy(currentPosition.x, currentPosition.y, textstring);
  END_DRAW
}

void  outtextxy(int x, int y, const char  *textstring)
{
  BOOL r;
  BEGIN_DRAW
    moveto(x, y);
    r = TextOut(
      activeDC,
      (x),
      (y),
      textstring,
      (int)strlen(textstring)
      );
    retrivePosition();
  END_DRAW
}

void  pieslice(int x, int y, int stangle, int endangle, int radius)
{
  BEGIN_DRAW
    Pie(
      activeDC, 
      (x - radius), 
      (y - radius), 
      (x + radius), 
      (y + radius), 
      ((int)(x + radius * cos(DEG_TO_RAD(stangle)))), 
      ((int)(y - radius * sin(DEG_TO_RAD(stangle)))),
      ((int)(x + radius * cos(DEG_TO_RAD(endangle)))), 
      ((int)(y - radius * sin(DEG_TO_RAD(endangle))))
      );
  END_DRAW
}

#define PUTPIXEL_16(X,Y,COLOR, OP) {\
  int index = X + (windowHeight - Y - 1) * windowWidth;\
  int delta = X % 2 ? 0 : 4;\
  activeBits[index / 2] OP (BYTE)((COLOR & 0xF) << delta);\
}

#define PUTPIXEL_RGB(X,Y,COLOR, OP) {\
  ((int *)activeBits)[X + (windowHeight - Y - 1) * windowWidth] OP COLOR;\
}


static void putpixelCOPY(int x, int y, int color)
{
  int index = x + (windowHeight - y-1) * windowWidth ;
  int delta = x % 2 ? 0 : 4;
  activeBits[index / 2] &= 0xF0 >> delta;
  activeBits[index / 2] |= (color & 0xF) << delta;
}

static void putpixelXOR(int x, int y, int color)
{
  PUTPIXEL_16(x, y, color, ^=);
}

void  putimage(int left, int top, const void  *bitmap, int op)
{
  typedef void ( *PUTPIXEL_PROC)(int x, int y, int color);
  
  int x, y, width = ((int *)bitmap)[0], height = ((int *)bitmap)[1];
  int * color= (int *)bitmap + 2;
  int maxx, maxy;
  maxy = top + height < windowHeight ? top + height : windowHeight;
  maxx = left + width < windowWidth ? left + width : windowWidth;

  BEGIN_DRAW
   if(rgbMode) {
        if(op == COPY_PUT) {
          for(y = top; y <= maxy; y++) 
            for(x = left; x <= maxx; x++)
              if( y >= 0 && y <= windowHeight && x >= 0 && y <= windowWidth)
               putpixelCOPY(x, y, *color++);
        } else {
          for(y = top; y <= maxy; y++) 
            for(x = left; x <= maxx; x++)
              if( y >= 0 && y <= windowHeight && x >= 0 && y <= windowWidth)
                putpixelXOR(x, y, *color++);
        }
   } else {
        if(op == COPY_PUT) {
          for(y = top; y <= maxy; y++) 
            for(x = left; x <= maxx; x++)
              if( y >= 0 && y <= windowHeight && x >= 0 && y <= windowWidth)
                PUTPIXEL_RGB(x, y, *color++, =);
        } else {
          for(y = top; y <= maxy; y++) 
            for(x = left; x <= maxx; x++)
              if( y >= 0 && y <= windowHeight && x >= 0 && y <= windowWidth)
                PUTPIXEL_RGB(x, y, *color++, ^=);
        }
   }
  END_DRAW
}

void  putpixel(int x, int y, int color)
{
  //static counter = 0;
  CHECK_GRAPHCS_INITED
  x = (x);
  y = (y);
  if(rgbMode) 
  {
    ((unsigned *)activeBits)[x + (windowHeight - y - 1) * windowWidth] = color;
    //SetPixelV(activeDC, (x), (y), translateColor(color));
  }
  else
  {
    if(x >= 0 && x < windowWidth && y >= 0 && y < windowHeight)
      putpixelCOPY(x, y, color);
  }
  if(activePageIndex == sharedStruct->visualPage)
    SetPixelV(windowDC, x, y, translateColor(color));
}

void  rectangle(int left, int top, int right, int bottom)
{
  BEGIN_DRAW
    moveto(left, top);
    lineto_(right, top);
    lineto_(right, bottom);
    lineto_(left, bottom);
    lineto_(left, top);
  END_DRAW
  updatePosition(right, bottom);
}

void  sector( int X, int Y, int StAngle, int EndAngle, int XRadius, int YRadius )
{
  BEGIN_DRAW
    moveto(
      (cos(DEG_TO_RAD(StAngle)) * XRadius), 
      (sin(DEG_TO_RAD(StAngle)) * YRadius)
      );
    Pie(
      activeDC, 
      (X - XRadius), 
      (Y - YRadius), 
      (X + XRadius), 
      (Y + YRadius), 
      (X + cos(DEG_TO_RAD(StAngle)) * XRadius),
      (Y - sin(DEG_TO_RAD(StAngle)) * YRadius),
      (X + cos(DEG_TO_RAD(EndAngle)) * XRadius),
      (Y - sin(DEG_TO_RAD(EndAngle)) * YRadius)
      );
  END_DRAW
}

void  setactivepage(int page)
{
  CHECK_GRAPHCS_INITED

  if(page == 0 || page == 1)
  {
    activeDC = pages[page].dc;
    activePageIndex = page;
    activeBits = (unsigned char *)pages[page].bits;
  }
}

void  setallpalette(const g_palettetype  * _palette)
{
  int i;
  CHECK_GRAPHCS_INITED
  for(i = 0; i != _palette->size; i++)
    BGI_palette[i] = BGI_default_palette[_palette->colors[i]];
  SetDIBColorTable(pages[0].dc, 0, MAXCOLORS, BGI_palette);
  SetDIBColorTable(pages[1].dc, 0, MAXCOLORS, BGI_palette);
  SendMessage(BGI_getWindow(), WM_MYPALETTECHANGED, 0, _palette->size);
}

void  setaspectratio(int xasp, int yasp)
{
  aspectRatio.x = ((double)xasp) / windowWidth;
  aspectRatio.y = ((double)yasp) / windowHeight;
}

void  setbkcolor(int color)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  backColor = color;
  DeleteObject(backBrush);
  backBrush = CreateSolidBrush(translateColor(color));
}

void  setcolor(int color)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  penColor = color;
  updatePen(CHANGED_COLOR);
}

void  setfillpattern(const char  *upattern, int color)
{
  int i;
  HANDLE old;
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  for(i = 0; i != 8; i++)
    patternsBits[USER_FILL][i] = upattern[i];
  old = stdBrushes[USER_FILL];
  stdBrushes[USER_FILL] = CreatePatternBrush(CreateBitmap(8,8,1,1,(LPBYTE)patternsBits[USER_FILL]));
  fillSettings.color = color;
  fillSettings.pattern = USER_FILL;
  updateBrush(CHANGED_ALL);
  DeleteObject(old);
}

void  setfillstyle(int pattern, int color)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  if(fillSettings.pattern == pattern) 
  {
    if(fillSettings.color != color)    
    {
      fillSettings.color = color;
      updateBrush(CHANGED_COLOR);
    }
  }
  else
  {
    fillSettings.pattern = pattern;
    updateBrush(CHANGED_ALL);
  }
}

void  setlinestyle(int linestyle, unsigned upattern, int thickness)
{
  CHECK_GRAPHCS_INITED
  if(linestyle >= 0 && linestyle <= USERBIT_LINE)
    lineSettings.linestyle = linestyle;
  if(linestyle == USERBIT_LINE)
  {
    HPEN pen;
    LOGBRUSH br;
    int c;
    static DWORD bits[32];
    br.lbColor = translateColor(penColor);
    br.lbStyle = BS_SOLID;
    c = convertToBits(bits, lineSettings.upattern);
    pen = ExtCreatePen(
      PS_USERSTYLE | PS_GEOMETRIC,
      lineSettings.thickness,
      &br, 
      0,
      bits
      );
    DeleteObject(stdPens[USERBIT_LINE]);
  }
  lineSettings.upattern = upattern;
  lineSettings.thickness = thickness;
  updatePen(CHANGED_ALL);
}

void  setpalette(int colornum, int color)
{
  CHECK_GRAPHCS_INITED
  if(!rgbMode)
  {
    CHECK_COLOR_RANGE(colornum)
    BGI_palette[colornum] = BGI_default_palette[color];
    SetDIBColorTable(pages[0].dc, colornum, 1, BGI_palette + colornum);
    SetDIBColorTable(pages[1].dc, colornum, 1, BGI_palette + colornum);
    SendMessage(BGI_getWindow(), WM_MYPALETTECHANGED, colornum, 1);
  }
}

void  setrgbpalette(int colornum, int red, int green, int blue)
{
  CHECK_GRAPHCS_INITED
  if(!rgbMode)
  {
    CHECK_COLOR_RANGE(colornum)
    BGI_palette[colornum].rgbRed = (BYTE)red;
    BGI_palette[colornum].rgbGreen = (BYTE)green;
    BGI_palette[colornum].rgbBlue = (BYTE)blue;
    builtinPalette[colornum] = RGB(red, green, blue);
    SetDIBColorTable(pages[0].dc, colornum, 1, BGI_palette + colornum);
    SetDIBColorTable(pages[1].dc, colornum, 1, BGI_palette + colornum);
    SendMessage(BGI_getWindow(), WM_MYPALETTECHANGED, colornum, 1);
  }
}

void  settextjustify(int horiz, int vert)
{
  CHECK_GRAPHCS_INITED
  textSetting.horiz = horiz;
  textSetting.vert = vert;
  updateFont();
}

void  settextstyle(int font, int direction, int charsize)
{
  CHECK_GRAPHCS_INITED
  textSetting.font = font;
  textSetting.direction = direction;
  textSetting.charsize = charsize;
  updateFont();
}

void  setusercharsize(int multx, int divx, int multy, int divy)
{
  CHECK_GRAPHCS_INITED
  if(divx != 0 && divy != 0) 
  {
    userSize.mx = multx / (double) divx;
    userSize.my = multy / (double) divy;
  }
}

void  setviewport(int left, int top, int right, int bottom, int clip)
{
  CHECK_GRAPHCS_INITED
  viewPort.left = left;
  viewPort.top = top;
  viewPort.right = right;
  viewPort.bottom = bottom;
  viewPort.clip = clip;
  updateViewport();
}

void  setvisualpage(int page)
{
  CHECK_GRAPHCS_INITED
  if(page == 0 || page == 1)
  {
    BGI_setVisualPage(page);
    frameCounter++;
    if(clock() >= lastMeasuredTime + CLOCKS_PER_SEC)
    {
      FPS = frameCounter;
      frameCounter = 0;
      lastMeasuredTime = clock();
    }
    BGI_updateWindow();
  }
}

void  setwritemode( int mode )
{
  XORMode = mode == XOR_PUT;
}

int textheight(const char  *textstring)
{
  SIZE size;
  ICHECK_GRAPHCS_INITED
  GetTextExtentPoint(activeDC, textstring, (int)strlen(textstring), &size);
  return size.cy;
}

int textwidth(const char  *textstring)
{
  SIZE size;
  ICHECK_GRAPHCS_INITED
  GetTextExtentPoint(activeDC, textstring, (int)strlen(textstring), &size);
  return size.cx;
}

void delay(int miliSeconds)
{
  Sleep(miliSeconds);
}

int anykeypressed()
{
  return sharedStruct->keyCode != -1;
}

int keypressed(int key)
{
  return GetAsyncKeyState(key);
}

void getmousestate(g_mousestate * state)
{
  CHECK_GRAPHCS_INITED
  state->x = sharedStruct->mouseX;
  state->y = sharedStruct->mouseY;
  state->buttons = sharedStruct->mouseButton;
}

void setmousepos(int x, int y)
{
  RECT r;
  CHECK_GRAPHCS_INITED
  
  GetWindowRect(BGI_getWindow(), &r);
  SetCursorPos(r.left + x, r.top + y);
}

int readkey()
{
  ICHECK_GRAPHCS_INITED
  return BGI_getch();
}

int rgb(int r, int g, int b)
{
    return (b & 0xFF) | ((g & 0xFF) << 8) | ((r & 0xFF) << 16);
}

int _getabsolutecolor(int c) 
{
    if(rgbMode) 
    {
        return    rgb(
                    BGI_default_palette[c].rgbRed,
                    BGI_default_palette[c].rgbGreen,
                    BGI_default_palette[c].rgbBlue
                    );
    }
    return c;
}

