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

#ifndef __BGI_GRAPHICS_H__
#define __BGI_GRAPHICS_H__

#ifdef _MSC_VER
  #pragma comment(lib, "kernel32")
  #pragma comment(lib, "user32")
  #pragma comment(lib, "gdi32")
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_BACK   8
#define KEY_TAB    9
#define KEY_UP     72
#define KEY_DOWN   80
#define KEY_LEFT   75
#define KEY_RIGHT  77
#define KEY_ENTER  '\r'
#define KEY_END    79
#define KEY_HOME   71
#define KEY_INSERT 82
#define KEY_DELETE 83
#define KEY_ESCAPE 27
#define KEY_SHIFT  0
#define KEY_CONTRL 0
#define KEY_PGUP   73
#define KEY_PGDOWN 81
#define KEY_F1     59
#define KEY_F2     60
#define KEY_F3     61
#define KEY_F4     62
#define KEY_F5     63
#define KEY_F6     64
#define KEY_F7     65
#define KEY_F8     66
#define KEY_F9     67
#define KEY_F10    68


#define MOUSE_LEFTBUTTON   1
#define MOUSE_RIGHTBUTTON  2
#define MOUSE_MIDDLEBUTTON 4

#define CUSTOM_MODE(WIDTH, HEIGHT) ((WIDTH & 0xFFFF) | ((HEIGHT & 0xFFFF) << 16))

#define MAXCOLORS 16

enum graphics_drivers {
  DETECT,
  VGA,
  CUSTOM
};

enum graphics_errors {      /* graphresult error return codes */
  grOk                =   0,
  grNoInitGraph       =  -1,
  grNotDetected       =  -2,
  grFileNotFound      =  -3,
  grInvalidDriver     =  -4,
  grNoLoadMem         =  -5,
  grNoScanMem         =  -6,
  grNoFloodMem        =  -7,
  grFontNotFound      =  -8,
  grNoFontMem         =  -9,
  grInvalidMode       = -10,
  grError             = -11,   /* generic error */
  grIOerror           = -12,
  grInvalidFont       = -13,
  grInvalidFontNum    = -14,
  grInvalidVersion    = -18
};

enum graphics_mode {
  VGALO,
  VGAMED,
  VGAHI,
  GM_640x480,
  GM_800x600,
  GM_1024x768,
  GM_NOPALETTE = 0x100
};

enum line_styles { /* Line styles for get/setlinestyle */
  SOLID_LINE   = 0,
  DOTTED_LINE  = 1,
  CENTER_LINE  = 2,
  DASHED_LINE  = 3,
  USERBIT_LINE = 4,   /* User defined line style */
};

enum line_widths { /* Line widths for get/setlinestyle */
  NORM_WIDTH  = 1,
  THICK_WIDTH = 3,
};

enum font_names {
  DEFAULT_FONT   = 0,    /* 8x8 bit mapped font */
  TRIPLEX_FONT   = 1,    /* "Stroked" fonts */
  SMALL_FONT= 2,
  SANS_SERIF_FONT= 3,
  GOTHIC_FONT    = 4,
  SCRIPT_FONT    = 5,   
  SIMPLEX_FONT   = 6,  
  TRIPLEX_SCR_FONT    = 7,
  COMPLEX_FONT   = 8,  
  EUROPEAN_FONT  = 9,  
  BOLD_FONT = 10 
};

#define HORIZ_DIR   0   /* left to right */
#define VERT_DIR    1   /* bottom to top */

#define USER_CHAR_SIZE  0   /* user-defined char size */

enum fill_patterns {    /* Fill patterns for get/setfillstyle */
  EMPTY_FILL,    /* fills area in background color */
  SOLID_FILL,    /* fills area in solid fill color */
  LINE_FILL,/* --- fill */
  LTSLASH_FILL,  /* /// fill */
  SLASH_FILL,    /* /// fill with thick lines */
  BKSLASH_FILL,  /* \\\ fill with thick lines */
  LTBKSLASH_FILL,/* \\\ fill */
  HATCH_FILL,    /* light hatch fill */
  XHATCH_FILL,   /* heavy cross hatch fill */
  INTERLEAVE_FILL,    /* interleaving line fill */
  WIDE_DOT_FILL, /* Widely spaced dot fill */
  CLOSE_DOT_FILL,/* Closely spaced dot fill */
  USER_FILL /* user defined fill */
};

enum putimage_ops {/* BitBlt operators for putimage */
  COPY_PUT, /* MOV */
  XOR_PUT,  /* XOR */
  OR_PUT,   /* OR  */
  AND_PUT,  /* AND */
  NOT_PUT   /* NOT */
};

enum text_just {   /* Horizontal and vertical justification
                   for settextjustify */
  LEFT_TEXT   = 0,
  CENTER_TEXT = 6,
  RIGHT_TEXT  = 2,

  BOTTOM_TEXT = 8,
  /* CENTER_TEXT = 1,  already defined above */
  TOP_TEXT    = 0
};

enum _COLORS {
  _BLACK,          /* dark colors */
  _BLUE,
  _GREEN,
  _CYAN,
  _RED,
  _MAGENTA,
  _BROWN,
  _LIGHTGRAY,
  _DARKGRAY,       /* light colors */
  _LIGHTBLUE,
  _LIGHTGREEN,
  _LIGHTCYAN,
  _LIGHTRED,
  _LIGHTMAGENTA,
  _YELLOW,
  _WHITE
};

#define BLACK (_getabsolutecolor(_BLACK))
#define BLUE (_getabsolutecolor(_BLUE))
#define GREEN (_getabsolutecolor(_GREEN))
#define CYAN (_getabsolutecolor(_CYAN))
#define RED (_getabsolutecolor(_RED))
#define MAGENTA (_getabsolutecolor(_MAGENTA))
#define BROWN (_getabsolutecolor(_BROWN))
#define LIGHTGRAY (_getabsolutecolor(_LIGHTGRAY))
#define DARKGRAY (_getabsolutecolor(_DARKGRAY))
#define LIGHTBLUE (_getabsolutecolor(_LIGHTBLUE))
#define LIGHTCYAN (_getabsolutecolor(_LIGHTCYAN))
#define LIGHTRED (_getabsolutecolor(_LIGHTRED))
#define YELLOW (_getabsolutecolor(_YELLOW))
#define WHITE (_getabsolutecolor(_WHITE))


typedef unsigned int colortype;

typedef struct mousestate {
  int x, y;
  int buttons;
} g_mousestate;

typedef struct palettetype{
  unsigned char size;
  colortype colors[MAXCOLORS+1];
} g_palettetype;

typedef struct linesettingstype{
  int linestyle;
  unsigned upattern;
  int thickness;
} g_linesettingstype;

typedef struct textsettingstype {
  int horiz;
  int vert;
  int charsize;
  int direction;
  int font;
} g_textsettingstype;

typedef struct fillsettingstype {
  int pattern;
  int color;
} g_fillsettingstype;

typedef struct pointtype {
  int x, y;
} g_pointtype;

typedef struct viewporttype {
  int left, top, right, bottom;
  int clip;
} g_viewporttype;

typedef struct arccoordstype {
  int x, y;
  int xstart, ystart, xend, yend;
} g_arccoordstype;

/**
 * Public functionality
 */
extern void arc(int x, int y, int stangle, int endangle, int radius);
extern void bar(int left, int top, int right, int bottom);
extern void bar3d(int left, int top, int right, int bottom, int depth, int topflag);
extern void circle(int x, int y, int radius);
extern void cleardevice(void);
extern void clearviewport(void);
extern void closegraph(void);
extern void detectgraph(int  *graphdriver,int * graphmode);
extern void drawpoly(int numpoints, const int * polypoints);
extern void ellipse(int x, int y, int stangle, int endangle, int xradius, int yradius);
extern void fillellipse( int x, int y, int xradius, int yradius );
extern void fillpoly(int numpoints, const int * polypoints);
extern void floodfill(int x, int y, int border);
extern void getarccoords(g_arccoordstype  *arccoords);
extern void getaspectratio(int * xasp, int * yasp);
extern int getbkcolor(void);
extern int getcolor(void);
extern g_palettetype * getdefaultpalette( void );
extern void getfillpattern(char  *pattern);
extern void getfillsettings(g_fillsettingstype  *fillinfo);
extern int getgraphmode(void);
extern void getimage(int left, int top, int right, int bottom,void *bitmap);
extern void getlinesettings(g_linesettingstype  *lineinfo);
extern int getmaxcolor(void);
extern int getmaxmode(void);
extern int getmaxx(void);
extern int getmaxy(void);
extern char *  getmodename( int mode_number );
extern void getmoderange(int graphdriver, int  *lomode, int *himode);
extern unsigned getpixel(int x, int y);
extern void getpalette(g_palettetype  *palette);
extern int getpalettesize( void );
extern void gettextsettings(g_textsettingstype  *texttypeinfo);
extern void getviewsettings(g_viewporttype  *viewport);
extern int getx(void);
extern int gety(void);
extern char *  grapherrormsg(int errorcode);
extern int graphresult(void);
unsigned imagesize(int left, int top, int right, int bottom);
extern void initgraph(int   *graphdriver, int   *graphmode, const char  *pathtodriver);
extern void line(int x1, int y1, int x2, int y2);
extern void linerel(int dx, int dy);
extern void lineto(int x, int y);
extern void moverel(int dx, int dy);
extern void moveto(int x, int y);
extern void outtext(const char  *textstring);
extern void outtextxy(int x, int y, const char  *textstring);
extern void pieslice(int x, int y, int stangle, int endangle, int radius);
extern void putimage(int left, int top, const void  *bitmap, int op);
extern void putpixel(int x, int y, int color);
extern void rectangle(int left, int top, int right, int bottom);
extern void sector( int X, int Y, int StAngle, int EndAngle, int XRadius, int YRadius );
extern void setactivepage(int page);
extern void setallpalette(const g_palettetype  * palette);
extern void setaspectratio(int xasp, int yasp);
extern void setbkcolor(int color);
extern void setcolor(int color);
extern void setfillpattern(const char  *upattern, int color);
extern void setfillstyle(int pattern, int color);
extern void setlinestyle(int linestyle, unsigned upattern, int thickness);
extern void setpalette(int colornum, int color);
extern void setrgbpalette(int colornum, int red, int green, int blue);
extern void settextjustify(int horiz, int vert);
extern void settextstyle(int font, int direction, int charsize);
extern void setusercharsize(int multx, int divx, int multy, int divy);
extern void setviewport(int left, int top, int right, int bottom, int clip);
extern void setvisualpage(int page);
extern void setwritemode( int mode );
extern int textheight(const char  *textstring);
extern int textwidth(const char  *textstring);

/*
 * Extended functionality
 */
extern void delay(int miliSeconds);
extern int readkey();
extern int keypressed(int key);
extern int anykeypressed();
extern int getfps();
extern void getmousestate(g_mousestate * state);
extern void setmousepos(int x, int y);
extern int rgb(int r, int g, int b);

/*
 * For internal use only
 */

extern int _getabsolutecolor(int colorid);

#ifdef __cplusplus
}
#endif

#endif
