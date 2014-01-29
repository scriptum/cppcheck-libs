/* 
 * $Header: paint.h,v 1.2 87/07/15 15:22:38 toddb Exp $ 
 * $Locker:  $
 */ 
#include <stdio.h>
#include "X11/Xlib.h"
#include "X11/Xatom.h"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define abs(x) ((x) < 0 ? (-(x)) : (x))

typedef enum {line, rect, filledrect, arc, filledarc, polygon, filledpolygon} ModeType;

typedef struct {
    ModeType mode;
    int thickness;
    int func;
    int angle1, angle2;
} StatusRec, *StatusPtr;

typedef struct _ImageRec {
    StatusRec stat;
    int npoints;
    XPoint *points;
    struct _ImageRec *next;
    int drawn;
} ImageRec, *ImagePtr;

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

#ifdef MAINMODULE
#define ext
#else
#define ext extern
#endif    

ext ImagePtr image, firstimage, lastimage;
ext StatusRec stat;
ext int curx, cury;
ext GC gc;
ext Window window;
ext int windowheight, windowwidth;
ext long foreground, background;

ext XPoint points[1000];
ext int npoints;

extern GC XCreateGC();
extern XFontStruct *XFont();

extern char *StringForMode();
extern char *StringForFunction();
