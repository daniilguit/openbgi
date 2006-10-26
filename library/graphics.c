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

#include "BGI.h"
#include "graphics.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <time.h>
#include <string.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#define TO_ABSOLUTE_X(X) ((int)((viewPort.left + X) * aspectRatio.x))
#define TO_ABSOLUTE_Y(Y) ((int)((viewPort.top + Y) * aspectRatio.y))
#define TO_RELATIVE_X(X) ((int)((X - viewPort.left) / aspectRatio.x))
#define TO_RELATIVE_Y(Y) ((int)((Y - viewPort.right) / aspectRatio.y))

#define PALETTE_INDEX(i) builtinPalette[palette.colors[i % MAXCOLORS]]
#define DEG_TO_RAD(X)  ((X) * M_PI / 180.0)
typedef void (*putpixelProc_t)(int x, int y, int color);

static g_pointtype modeResolution[] = {{640,480},{640,200}, {640,350}, {640,480}, {640,480}, {800,600},{1024,768}};

static lastMeasuredTime = 0;
static int frameCounter = 0;
static int FPS = 0;
static int XORMode = 0;

static HDC windowDC;
static int windowWidth;
static int windowHeight;
static int length;
static PAGE * pages;
static int activePageIndex = 0;
static PAGE * activePage;
static unsigned char * activeBits;
static HDC activeDC;
static SHARED_STRUCT * sharedStruct;

static HBRUSH stdBrushes[USER_FILL + 1];

static struct 
{
  double x;
  double y;
} aspectRatio = {1, 1};

static int graphMode = -1;

static COLORREF builtinPalette[MAXCOLORS];
static HPALETTE hPalette;
static HBRUSH currentBrush;
static HBRUSH backBrush;
static g_pointtype currentPosition;
static int backColor = BLACK;
static int penColor = WHITE;
static int brushColor = BLACK;

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

static int userPatternColor;

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

static void selectObject(HANDLE object)
{
  HANDLE oldObject = SelectObject(pages[0].dc, object);
  HANDLE r = SelectObject(pages[1].dc, object);
  SelectObject(windowDC, object);
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

#define CHECK_COLOR_RANGE(COLOR) if(COLOR < 0 || COLOR >= MAXCOLORS) return;
#define CHECK_GRAPHCS_INITED if(graphMode == -1) return;
#define ICHECK_GRAPHCS_INITED if(graphMode == -1) return -1;

#define BEGIN_DRAW  CHECK_GRAPHCS_INITED
#define END_DRAW   endDraw();

#define BEGIN_FILL {BEGIN_DRAW SetTextColor(activeDC, PALETTE_INDEX(fillSettings.color)); }
#define END_FILL {END_DRAW SetTextColor(activeDC, PALETTE_INDEX(penColor)); }

static void setWriteMode()
{
  int op;
  op = XORMode ? R2_XORPEN : R2_COPYPEN; 
  if(XORMode)
  {
    SetROP2(pages[0].dc, op);
    SetROP2(pages[1].dc, op);
    SetROP2(activeDC, op);
  }
}

static void unsetWriteMode()
{
  SetROP2(pages[0].dc, R2_COPYPEN);
  SetROP2(pages[1].dc, R2_COPYPEN);
  SetROP2(activeDC, R2_COPYPEN);
}

static void updatePosition(int x, int y)
{
  currentPosition.x = x;
  currentPosition.y = y;
  MoveToEx(
    activeDC, 
    TO_ABSOLUTE_X(currentPosition.x), 
    TO_ABSOLUTE_Y(currentPosition.y),
    NULL
    );
}

static void retrivePosition()
{
  MoveToEx(activeDC, 0, 0, (LPPOINT)&currentPosition);
  MoveToEx(activeDC, currentPosition.x, currentPosition.y, NULL);
  
  currentPosition.x = TO_RELATIVE_X(currentPosition.x);
  currentPosition.y = TO_RELATIVE_X(currentPosition.y);
}

static int convertToBits(DWORD bits[32], int pattern)
{
  int i = 0, j;
  pattern &= 0xFFFF;

  while (1) 
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

static void updatePen()
{
  HPEN pen;
  DWORD bits[32] = {0};
  LOGBRUSH br;
  int c;
  SetTextColor(pages[0].dc, PALETTE_INDEX(penColor));
  SetTextColor(pages[1].dc, PALETTE_INDEX(penColor));
  switch(lineSettings.linestyle)
  {
  case SOLID_LINE:
    pen = CreatePen(PS_SOLID, lineSettings.thickness, PALETTE_INDEX(penColor));
    break;
  case DOTTED_LINE:
    pen = CreatePen(PS_DOT,  lineSettings.thickness, PALETTE_INDEX(penColor));
    break;
  case CENTER_LINE:
    pen = CreatePen(PS_DASH,  lineSettings.thickness, PALETTE_INDEX(penColor));
    break;
  case DASHED_LINE:
    pen = CreatePen(PS_DASH,  lineSettings.thickness, PALETTE_INDEX(penColor));
    break;
  case USERBIT_LINE:
    br.lbColor = PALETTE_INDEX(penColor);
    br.lbStyle = BS_SOLID;
    c = convertToBits(bits, lineSettings.upattern);
    pen = ExtCreatePen(
            PS_USERSTYLE | PS_GEOMETRIC,
            lineSettings.thickness,
            &br, 
            c,
            bits
            );
    break;
  default:
    pen = CreatePen(PS_SOLID, lineSettings.thickness, PALETTE_INDEX(penColor));
    break;
  }
  selectObject(pen);
}

static void updateBrush()
{
  SelectObject(pages[0].dc, stdBrushes[fillSettings.pattern]);
  SelectObject(pages[1].dc, stdBrushes[fillSettings.pattern]);
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
  selectObject(CreateFontIndirect(&lf));
  
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
    selectObject(
      CreateRectRgn(
        viewPort.left, 
        viewPort.top, 
        viewPort.right,
        viewPort.bottom
        )
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

void initgraph(int * gd, int * gm, const char * path)
{
  int options = 0;
  if(graphMode != -1) 
    closegraph();
  graphMode = *gm;
  if(*gd == DETECT || *gd == VGA)
  {
    graphMode = VGAHI;
  }
  if(graphMode < 0 || graphMode > GM_1024x768)
  {
    graphMode = -1;
    return;
  }
  if(strstr(path, "SHOW_INVISIBLE_PAGE") != NULL)
    options |= MODE_SHOW_INVISIBLE_PAGE;
  if(strstr(path, "DISABLE_DEBUG") != NULL)
    options = MODE_RELEASE;
  else 
    options = MODE_DEBUG;
  if(strstr(path, "RGB") != NULL)
    options |= MODE_RGB;
  if(strstr(path, "FULL_SCREEN") != NULL)
    options |= MODE_FULLSCREEN;
  windowWidth = modeResolution[graphMode].x;
  windowHeight = modeResolution[graphMode].y;

  length = windowWidth * windowHeight / 2;
  penColor = getmaxcolor();
  
  BGI_startServer(windowWidth, windowHeight, options);
  
  windowDC = BGI_getWindowDC();
  pages = BGI_getPages();
  initPallette();

  backBrush = CreateSolidBrush(0);
  sharedStruct = BGI_getSharedStruct();
  
  SetBkMode(pages[0].dc, TRANSPARENT);
  SetBkMode(pages[1].dc, TRANSPARENT);
  
  setactivepage(0);
  setvisualpage(0);
  initBrushes();

  setbkcolor(backColor);
  updateBrush();
  updatePen();
  updateFont();
  updatePosition(0,0);

  //SetFocus(GetConsoleWindow());
}

static void lineto_(int x, int y)
{
  LineTo(activeDC, x, y);
  SetPixelV(activeDC, x, y, PALETTE_INDEX(penColor));
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
    TO_ABSOLUTE_X(x),
    TO_ABSOLUTE_Y(y),
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

void  bar(int left, int top, int right, int bottom)
{
  BEGIN_FILL
    Rectangle(
      activeDC,
      TO_ABSOLUTE_X(left),
      TO_ABSOLUTE_Y(top),
      TO_ABSOLUTE_X(right),
      TO_ABSOLUTE_Y(bottom)
      );
  END_FILL
}

void  bar3d(int left, int top, int right, int bottom, int depth, int topflag)
{
  int hdep = depth * 3 / 5;
  BEGIN_FILL
    Rectangle(
      activeDC,
      TO_ABSOLUTE_X(left),
      TO_ABSOLUTE_Y(top),
      TO_ABSOLUTE_X(right),
      TO_ABSOLUTE_Y(bottom)
      );
    moveto(left, top);
    lineto_(left + depth,top - hdep);
    lineto_(right + depth, top - hdep);
    lineto_(right + depth, bottom - hdep);
    lineto_(right, bottom);
    moveto(right + depth, top - hdep);
    lineto_(right, top);
  END_FILL
}

void circle_(HDC dc, int x, int y, int radius)
{
  BEGIN_DRAW
    Arc(
      dc, 
      TO_ABSOLUTE_X(x - radius),
      TO_ABSOLUTE_Y(y - radius),
      TO_ABSOLUTE_X(x + radius),
      TO_ABSOLUTE_Y(y + radius),
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
  RECT r = {0, 0, windowWidth, windowHeight};
  BEGIN_DRAW
    FillRect(activeDC, &r, backBrush);
  END_DRAW
}

void  clearviewport(void)
{
  RECT r = {viewPort.left, viewPort.top, viewPort.right, viewPort.bottom};
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
    points[i].x = TO_ABSOLUTE_X(*polypoints++);
    points[i].y = TO_ABSOLUTE_Y(*polypoints++);
  }
  BEGIN_DRAW
    Polyline(activeDC, points, numpoints);
  END_DRAW
  free(points);
}

void ellipse_(HDC dc, int x, int y, int stangle, int endangle, int xradius, int yradius)
{
  Arc(
    activeDC,
    TO_ABSOLUTE_X(x - xradius), 
    TO_ABSOLUTE_Y(y - yradius),
    TO_ABSOLUTE_X(x + xradius),
    TO_ABSOLUTE_Y(y + yradius),
    TO_ABSOLUTE_X(x + xradius * cos(DEG_TO_RAD(stangle))),
    TO_ABSOLUTE_Y(y - yradius * sin(DEG_TO_RAD(stangle))),
    TO_ABSOLUTE_X(x + xradius * cos(DEG_TO_RAD(endangle))),
    TO_ABSOLUTE_Y(y - yradius * sin(DEG_TO_RAD(endangle)))
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
        TO_ABSOLUTE_X(x - xradius),
        TO_ABSOLUTE_Y(y - yradius),
        TO_ABSOLUTE_X(x + xradius),
        TO_ABSOLUTE_Y(y + yradius)
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
    points[i].x = TO_ABSOLUTE_X(*polypoints++);
    points[i].y = TO_ABSOLUTE_Y(*polypoints++);
  }
  BEGIN_FILL
      Polygon(activeDC, points, numpoints);
  END_FILL
  free(points);
}

void  floodfill(int x, int y, int border)
{
  BEGIN_FILL
     ExtFloodFill(activeDC, x, y, PALETTE_INDEX(border), FLOODFILLBORDER);
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
  return windowWidth;
}

int getmaxy(void)
{
  return windowHeight;
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
  *lomode = VGALO;
  *himode = GM_1024x768;
}

unsigned getpixel(int x, int y)
{
  int index = TO_ABSOLUTE_X(x) + (windowHeight - TO_ABSOLUTE_Y(y) - 1) * windowWidth;
  int delta = index % 2 ? 0 : 4;
  x = TO_ABSOLUTE_X(x);
  y = TO_ABSOLUTE_Y(y);
  if(x >= 0 && x < windowWidth && y >= 0 && y < windowHeight)
    return (activeBits[index / 2] & (0xF << delta)) >> delta;
  //return GetPixel(activeDC, TO_ABSOLUTE_X(x), TO_ABSOLUTE_Y(y));
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
  if(graphMode == -1)
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
  LineTo(dc, TO_ABSOLUTE_X(x), TO_ABSOLUTE_Y(y));
}
void line_(HDC dc, int x1, int y1, int x2, int y2)
{
  MoveToEx(dc, TO_ABSOLUTE_X(x1), TO_ABSOLUTE_Y(y1), NULL);
  lineto__(dc, x2, y2);
  SetPixelV(dc, TO_ABSOLUTE_X(x2), TO_ABSOLUTE_Y(y2), PALETTE_INDEX(penColor));
}

#define BEGIN_LINEDRAW setWriteMode();
#define END_LINEDRAW unsetWriteMode(); END_DRAW

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
      TO_ABSOLUTE_X(x),
      TO_ABSOLUTE_Y(y),
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
      TO_ABSOLUTE_X(x - radius), 
      TO_ABSOLUTE_Y(y - radius), 
      TO_ABSOLUTE_X(x + radius), 
      TO_ABSOLUTE_Y(y + radius), 
      TO_ABSOLUTE_X((int)(x + radius * cos(DEG_TO_RAD(stangle)))), 
      TO_ABSOLUTE_Y((int)(y - radius * sin(DEG_TO_RAD(stangle)))),
      TO_ABSOLUTE_X((int)(x + radius * cos(DEG_TO_RAD(endangle)))), 
      TO_ABSOLUTE_Y((int)(y - radius * sin(DEG_TO_RAD(endangle))))
      );
  END_DRAW
}

#define PUTPIXEL(X,Y,COLOR, OP)\
  int index = X + (windowHeight - Y - 1) * windowWidth;\
  int delta = X % 2 ? 0 : 4;\
  activeBits[index / 2] OP (COLOR & 0xF) << delta


static void putpixelCOPY(int x, int y, int color)
{
  int index = x + (windowHeight - y-1) * windowWidth ;
  int delta = x % 2 ? 0 : 4;
  activeBits[index / 2] &= 0xF0 >> delta;
  activeBits[index / 2] |= (color & 0xF) << delta;
}

static void putpixelXOR(int x, int y, int color)
{
  PUTPIXEL(x, y, color, ^=);
}

static void putpixelOR(int x, int y, int color)
{
  PUTPIXEL(x, y, color, |=);
}

static void putpixelAND(int x, int y, int color)
{
  PUTPIXEL(x, y, color, &=);
}

static void putpixelNOT(int x, int y, int color)
{
  PUTPIXEL(x, y, color, =!);
}

void  putimage(int left, int top, const void  *bitmap, int op)
{
  typedef void ( *PUTPIXEL_PROC)(int x, int y, int color);
  
  static PUTPIXEL_PROC puts[] = {putpixelCOPY, putpixelXOR, putpixelOR, putpixelAND, putpixelNOT};
  PUTPIXEL_PROC put = puts[op];
  int x, y, width = ((int *)bitmap)[0], height = ((int *)bitmap)[1];
  int * color= (int *)bitmap + 2;
  int maxx, maxy;
  maxy = top + height < windowHeight ? top + height : windowHeight;
  maxx = left + width < windowWidth ? left + width : windowWidth;

  BEGIN_DRAW
    for(y = top; y <= maxy; y++) 
      for(x = left; x <= maxx; x++)
        if( y >= 0 && y <= windowHeight && x >= 0 && y <= windowWidth)
        put(x, y, *color++);
  END_DRAW
}

void  putpixel(int x, int y, int color)
{
  //static counter = 0;
  CHECK_GRAPHCS_INITED
  if(x >= 0 && x < windowWidth && y >= 0 && y < windowHeight)
    putpixelCOPY(TO_ABSOLUTE_X(x), TO_ABSOLUTE_Y(y), color);
  if(activePageIndex == sharedStruct->visualPage)
    SetPixelV(windowDC, TO_ABSOLUTE_X(x), TO_ABSOLUTE_Y(y), PALETTE_INDEX(color));
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
      TO_ABSOLUTE_X(cos(DEG_TO_RAD(StAngle)) * XRadius), 
      TO_ABSOLUTE_Y(sin(DEG_TO_RAD(StAngle)) * YRadius)
      );
    Pie(
      activeDC, 
      TO_ABSOLUTE_X(X - XRadius), 
      TO_ABSOLUTE_Y(Y - YRadius), 
      TO_ABSOLUTE_X(X + XRadius), 
      TO_ABSOLUTE_Y(Y + YRadius), 
      TO_ABSOLUTE_X(X + cos(DEG_TO_RAD(StAngle)) * XRadius),
      TO_ABSOLUTE_Y(Y - sin(DEG_TO_RAD(StAngle)) * YRadius),
      TO_ABSOLUTE_X(X + cos(DEG_TO_RAD(EndAngle)) * XRadius),
      TO_ABSOLUTE_Y(Y - sin(DEG_TO_RAD(EndAngle)) * YRadius)
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
  CHECK_GRAPHCS_INITED
  memcpy(&palette, _palette, sizeof(palette));
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
  backBrush = CreateSolidBrush(color);
  SetBkColor(pages[0].dc, PALETTE_INDEX(color));
  SetBkColor(pages[1].dc, PALETTE_INDEX(color));
}

void  setcolor(int color)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  penColor = color;
  updatePen();
}

void  setfillpattern(const char  *upattern, int color)
{
  int i;
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  for(i = 0; i != 8; i++)
    patternsBits[USER_FILL][i] = upattern[i];
  DeleteObject(stdBrushes[USER_FILL]);
  fillSettings.pattern = USER_FILL;
  fillSettings.color = color;
  stdBrushes[USER_FILL] = CreatePatternBrush(CreateBitmap(8,8,1,1,(LPBYTE)patternsBits[USER_FILL]));
  updateBrush();
}

void  setfillstyle(int pattern, int color)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(color)
  fillSettings.pattern = pattern;
  fillSettings.color = color;
  updateBrush();
}

void  setlinestyle(int linestyle, unsigned upattern, int thickness)
{
  CHECK_GRAPHCS_INITED
  if(linestyle >= 0 && linestyle < USERBIT_LINE)
    lineSettings.linestyle = linestyle;
  lineSettings.upattern = upattern;
  lineSettings.thickness = thickness;
  updatePen();
}

void  setpalette(int colornum, int color)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(colornum)
  palette.colors[colornum] = color;
}

void  setrgbpalette(int colornum, int red, int green, int blue)
{
  CHECK_GRAPHCS_INITED
  CHECK_COLOR_RANGE(colornum)
  BGI_palette[colornum].rgbRed = red;
  BGI_palette[colornum].rgbGreen = green;
  BGI_palette[colornum].rgbBlue = blue;
  SetDIBColorTable(pages[0].dc, 0, MAXCOLORS, (const RGBQUAD *)BGI_palette);
  SetDIBColorTable(pages[1].dc, 0, MAXCOLORS, (const RGBQUAD *)BGI_palette);
  SendMessage(BGI_getWindow(), WM_MYPALETTECHANGED, 0, 0);
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
    userSize.my = multx / (double) divy;
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

int kbhit()
{
  ICHECK_GRAPHCS_INITED
  return sharedStruct->keyCode != -1;
}

int anykeypressed()
{
  return sharedStruct->keyCode != -1;
}

int keypressed(int key)
{
  return GetAsyncKeyState(key);
}

int getkeypressed()
{
  return sharedStruct->keyCode != -1;
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

void setmousecursor(int * cur)
{
}

int readkey()
{
  ICHECK_GRAPHCS_INITED
  return BGI_getch();
}


