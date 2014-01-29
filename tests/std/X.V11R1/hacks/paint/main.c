/* 
 * $Locker:  $ 
 */ 
static char	*rcsid = "$Header: main.c,v 1.4 87/09/08 08:07:02 swick Exp $";
#define MAINMODULE 1

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
#include "paint.h"

/* DEBUGGING HACKS */

#define XClear(w) XClearArea(dpy, w, 0, 0, 9999, 9999, FALSE)
/* #define XSetClipRectangles(a, b, c, d, e, f)*/

Display *dpy;

int shiftbit = FALSE;

HandleKey(event)
  XEvent *event;
{
    XGCValues gcvalues;
    int code;
    char c;
    static int arg = 0;
    static int argval = 1;
    static int sgn = 0;
    code = event->xkey.keycode;
    if (code == 174) {
	shiftbit = TRUE;
	return;
    }
    c = GetCharFromCode(code);
    if (c >= 'a' && c <= 'z' && shiftbit)
	c += 'A' - 'a';
    switch (c) {
      case 'C':
	XClearWindow(dpy, window);
	for (; firstimage ; firstimage = firstimage->next) {
	    free(firstimage->points);
	    free(firstimage);
	}
	image = firstimage = lastimage = NULL;
	break;
      case 'l':
	stat.mode = line;
	break;
      case 'r':
	stat.mode = rect;
	break;
      case 'R':
	stat.mode = filledrect;
	break;
      case 'p':
	stat.mode = polygon;
	npoints = 0;
	break;
      case 'P':
	stat.mode = filledpolygon;
	npoints = 0;
	break;
      case 'a':
	stat.mode = arc;
	break;
      case 'A':
	stat.mode = filledarc;
	break;
      case 'f':
	stat.func = arg;
	if (arg != argval)
	    stat.func = GXinvert;
	gcvalues.function = stat.func;
	XChangeGC(dpy, gc, GCFunction, &gcvalues);
	break;
      case ',':
	stat.angle1 = arg * 64;
	break;
      case '.':
	stat.angle2 = arg * 64;
	break;
      case '-':
	sgn = -1;
	if (arg != argval) argval = -1;
	else argval = arg = abs(arg) * sgn;
	return;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '0':
	if (sgn == 0) sgn = 1;
	argval = arg = sgn * (abs(arg) * 10 + c - '0');
	return;
      case 't':
	stat.thickness = argval;
	gcvalues.line_width = stat.thickness;
	XChangeGC(dpy, gc, GCLineWidth, &gcvalues);
	break;
      default:
	printf("Unknown code %d (%c)\n", event->xkey.keycode, c);
	return;
    }
    DisplayMode();
    arg = 0;
    argval = 1;
    sgn = 1;
}

HandleKeyUp(event)
  XEvent *event;
{
    if (event->xkey.keycode == 174)
	shiftbit = FALSE;
}

DisplayMode()
{
    char str[500];
    sprintf(str,
	    "Mode %s; Thickness %d; Function %s; angle1 %.2f; angle2 %.2f",
	    StringForMode(stat.mode), stat.thickness, StringForFunction(stat.func),
	    stat.angle1 / 64.0, stat.angle2/64.0);
    XDrawImageString(dpy, window, gc, 0, 0, str, strlen(str));
    printf("%s\n", str);
}

HandleButtonDown(event)
  XEvent *event;
{
    switch (stat.mode) {
      case line:
      case rect:
      case filledrect:
      case arc:
      case filledarc:
	curx = event->xbutton.x;
	cury = event->xbutton.y;
	break;
    }
}

ImagePtr NewImage()
{
    ImagePtr image;
    image = (ImagePtr) malloc(sizeof(ImageRec));
    image->stat = stat;
    image->npoints = 0;
    image->points = (XPoint *) malloc(1);
    return image;
}

StuffPoint(image, x, y)
  ImagePtr image;
  int x, y;
{
    npoints = image->npoints++;
    image->points = (XPoint *)
	realloc(image->points, sizeof(XPoint) * image->npoints);
    image->points[npoints].x = x;
    image->points[npoints].y = y;
    image->next = NULL;
    image->drawn = FALSE;
}

DrawImage(image)
  ImagePtr image;
{
    XGCValues gcvalues;
    int x1, x2, y1, y2, x, y, width, height;
    gcvalues.function = image->stat.func;
    gcvalues.line_width = image->stat.thickness;
    XChangeGC(dpy, gc, GCFunction | GCLineWidth, &gcvalues);
    if (!image->drawn) {
	if (lastimage)
	    lastimage->next = image;
	else
	    firstimage = image;
	lastimage = image;
	image->drawn = TRUE;
    }
    if (image->npoints == 2) {
	x1 = image->points[0].x;
	x2 = image->points[1].x;
	y1 = image->points[0].y;
	y2 = image->points[1].y;
	x = min(x1, x2);
	y = min(y1, y2);
	width = x1 + x2 - x - x;
	height = y1 + y2 - y - y;
    }
    switch (image->stat.mode) {
      case line:
	XDrawLine(dpy, window, gc, x1, y1, x2, y2);
	break;
      case rect:
	XDrawRectangle(dpy, window, gc, x, y, width, height);
	break;
      case filledrect:
	XFillRectangle(dpy, window, gc, x, y, width, height);
	break;
      case arc:
        XDrawArc(dpy, window, gc, x, y, width, height,
	     image->stat.angle1, image->stat.angle2);
        break;
      case filledarc:
        XFillArc(dpy, window, gc, x, y, width, height,
		 image->stat.angle1, image->stat.angle2);
        break;
      case polygon:
	XDrawLines(dpy, window, gc, image->points, image->npoints,
		CoordModeOrigin);
	break;
      case filledpolygon:
	XFillPolygon(dpy, window, gc, image->points, image->npoints, Complex,
		     CoordModeOrigin);
	break;
    }
}


HandleButtonUp(event)
  XEvent *event;
{
    int newx, newy;
    newx = event->xbutton.x;
    newy = event->xbutton.y;
    switch (stat.mode) {
      case line:
      case rect:
      case filledrect:
      case arc:
      case filledarc:
	image = NewImage();
	StuffPoint(image, curx, cury);
	StuffPoint(image, newx, newy);
	DrawImage(image);
	break;
      case polygon:
      case filledpolygon:
	if (image == NULL || image->stat.mode != stat.mode)
	    image = NewImage();
	StuffPoint(image, newx, newy);
	if (event->xbutton.button == Button3) {
	    DrawImage(image);
	}
    }
    curx = newx;
    cury = newy;
    printf("*");
    fflush(stdout);
}

XRectangle rects[500];
int numrects = 0;

RepaintDisplay(event)
  XEvent *event;
{
    ImagePtr image;
    rects[numrects].x = event->xexpose.x;
    rects[numrects].y = event->xexpose.y;
    rects[numrects].width = event->xexpose.width;
    rects[numrects].height = event->xexpose.height;
    XClearArea(dpy, window, event->xexpose.x, event->xexpose.y,
	       event->xexpose.width, event->xexpose.height, FALSE);
    numrects++;
    if (! event->xexpose.count) {
	XSetClipRectangles(dpy, gc, 0, 0, rects, numrects, Unsorted);
	numrects = 0;
	for (image = firstimage; image; image = image->next)
	    DrawImage(image);
	rects[0].x = 0;
	rects[0].y = 0;
	rects[0].width = 9999;
	rects[0].height = 9999;
	XSetClipRectangles(dpy, gc, 0, 0, rects, 1, Unsorted);
    }
}

main()
{
    Font font;
    XEvent event;
    Visual visual;
    XSetWindowAttributes attributes;
    XGCValues gcvalues;
    if ((dpy = XOpenDisplay("")) == NULL)
	Punt("Couldn't open display!");
    InitUtil();
    windowwidth = 400;
    windowheight = 400;
    stat.thickness = 1;
    stat.func = GXinvert;
    stat.angle1 = 0;
    stat.angle2 = 360 * 64;
    foreground = WhitePixel(dpy, DefaultScreen(dpy));
    background = BlackPixel(dpy, DefaultScreen(dpy));
    font = XLoadFont(dpy, "fixed");
    visual.visualid = CopyFromParent;
    attributes.background_pixel = background;
    attributes.border_pixel = foreground;
    attributes.backing_store = Always;
    window = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		    20, 20, windowwidth, windowheight, 1,
		    DefaultDepth(dpy, DefaultScreen(dpy)), CopyFromParent, &visual,
		    CWBackPixel | CWBorderPixel /*| CWBackingStore */,
		    &attributes);
    XChangeProperty(dpy, window, XA_WM_NAME, XA_STRING, 8, 
		    PropModeReplace, "Paint", 5);
    MyXSelectInput(dpy, window, KeyPressMask | KeyReleaseMask | ButtonPressMask
		   | ButtonReleaseMask | ButtonMotionMask | ExposureMask);
    XMapWindow(dpy, window);
    gcvalues.foreground = foreground;
    gcvalues.background = background;
    gcvalues.function = stat.func;
    gcvalues.line_width = stat.thickness;
    gcvalues.font = font;
/*     gcvalues.line_style = Solid; */
    gc = XCreateGC(dpy, window, GCFont | GCFunction | GCForeground
		   | GCBackground | GCLineWidth,
		   &gcvalues);
    stat.mode = line;
    image = firstimage = lastimage = NULL;
    DisplayMode();
    while (1) {
	XNextEvent(dpy, &event);
	switch(event.type) {
	  case KeyPress:
	    HandleKey(&event);
	    break;
	  case KeyRelease:
	    HandleKeyUp(&event);
	    break;
	  case ButtonPress:
	    HandleButtonDown(&event);
	    break;
	  case ButtonRelease:
	    HandleButtonUp(&event);
	    break;
	  case MotionNotify:
	    break;
	  case Expose:
	    RepaintDisplay(&event);
	    break;
	}
    }	    
}
