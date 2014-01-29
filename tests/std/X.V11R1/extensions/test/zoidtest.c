/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/* $Header: zoidtest.c,v 1.4 87/09/12 18:45:11 sun Locked $ */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "zoid.h"


#define rnd(x)          (random() % (x))

Display *dpy;

#define TEMPLATE        0
#define RECTANGLES      1
#define XTRAPEZOIDS     2
#define YTRAPEZOIDS     3
/*int      mode = TEMPLATE;*/
int      mode = YTRAPEZOIDS;

extern int _Xdebug;

#define TILE_WIDTH  32
#define TILE_HEIGHT 32
static short doublearrow32_bits[] = {
   0x0000, 0x0000, 0x0000, 0x0000,
   0x1800, 0x0000, 0x3c00, 0x0000,
   0x7e00, 0x0000, 0xff00, 0x0000,
   0xff80, 0x0001, 0xbdc0, 0x0003,
   0x3ce0, 0x0007, 0x3c00, 0x0000,
   0x3c00, 0x0000, 0x3c00, 0x0000,
   0x3c00, 0x0000, 0x3c00, 0x0000,
   0x3c00, 0x0000, 0x3ce0, 0x0007,
   0xbdc0, 0x0003, 0xff80, 0x0001,
   0xff00, 0x0000, 0x7e00, 0x0000,
   0x3c00, 0x0000, 0x1800, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000,
   0x0000, 0x0000, 0x0000, 0x0000
};

static short stipple32_bits[] = {
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc,
   0x3333, 0x3333, 0x3333, 0x3333,
   0xcccc, 0xcccc, 0xcccc, 0xcccc
};

int fill_styles[] = {FillSolid, FillTiled, FillStippled};

template(dpy, w, gc)
    Display     *dpy;
    Window       w;
    GC           gc;
{
    int i;
    srandom(getpid());

    /* some interesting data */
    for (i = 0; i < 100; i++)
	    XDrawLine(dpy,  w, gc, rnd(400), rnd(400), rnd(400), rnd(400));
}

static XRectangle rects[3] = {
    20,   20, 100, 100,
    120, 120, 100, 100,
    220, 220, 100, 100
};

rectangles(dpy, w, gc)
    Display     *dpy;
    Window       w;
    GC           gc;
{
    XFillRectangles(dpy, w, gc, rects, 3);
}

static xXTraps zoids[] = {
     30, 110,  10, 130,  20, 120,
    130, 210, 110, 230, 120, 220,
    230, 310, 210, 330, 220, 320
};

threezoids(dpy, w, gc)
    Display     *dpy;
    Window       w;
    GC           gc;
{
    /* PolyFillTrapezoid gc
     *      20 100   0 120  10 110
     *     120 200 100 220 110 210
     *     220 300 200 320 210 310
     */
     XFillTrapezoids(dpy, w, gc, zoids, 3);
}

xtrapezoids(dpy, w, gc)
    Display     *dpy;
    Window       w;
    GC           gc;
{
    XSetZoidAlignment(dpy, gc, XZoid_XAligned);
    threezoids(dpy, w, gc);
}

ytrapezoids(dpy, w, gc)
    Display     *dpy;
    Window       w;
    GC           gc;
{
    XSetZoidAlignment(dpy, gc, XZoid_YAligned);
    threezoids(dpy, w, gc);
}

StartConnectionToServer(argc, argv)
int     argc;
char    *argv[];
{
    char *display;

    display = NULL;
    for(--argc, ++argv; argc; --argc, ++argv)
    {
	if ((*argv)[0] == '-') {
	    switch((*argv)[1]) {
	    case 't':
		mode = TEMPLATE;
		break;
	    case 'r':
		mode = RECTANGLES;
		break;
	    case 'x':
		mode = XTRAPEZOIDS;
		break;
	    case 'y':
		mode = YTRAPEZOIDS;
		break;
	    }
	}
	if(index((*argv), ':') != NULL)
	    display = *argv;
    }
    if (!(dpy = XOpenDisplay(display)))
    {
       perror("Cannot open display\n");
       exit(0);
   }
}

Pixmap
MakePixmap(w, data, depth)
    Window      w;
    short *data;
    int depth;
{
    XImage ximage;
    Pixmap pid;
    GC pgc;
    XColor screen_def_green, exact_def_green;
    XColor screen_def_violet, exact_def_violet;

    /* stipples are always depth 1 */
    pid = XCreatePixmap(dpy, w, TILE_WIDTH, TILE_HEIGHT, depth);
    /* create gc of same depth as pixmap dest */
    pgc = XCreateGC(dpy, pid, 0, 0);  

    XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "green",
	       &screen_def_green,  &exact_def_green);
    XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "violet",
	       &screen_def_violet,  &exact_def_violet);
    XSetForeground(dpy, pgc, screen_def_violet.pixel);
    XSetBackground(dpy, pgc, screen_def_green.pixel);

    bzero(&ximage, sizeof(ximage));
    ximage.height = TILE_HEIGHT;
    ximage.width = TILE_WIDTH;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = (char *)data;
    ximage.byte_order = LSBFirst;
    ximage.bitmap_unit = 16;
    ximage.bitmap_bit_order = LSBFirst;
    ximage.bitmap_pad = 32;
    ximage.bytes_per_line = 4;
    ximage.depth = 1;

    /* depths of pid and pgc must be equal */
    XPutImage(dpy, pid, pgc, &ximage, 0, 0, 0, 0, TILE_WIDTH, TILE_HEIGHT);
    XFreeGC(dpy, pgc);
    return(pid);
}

main(argc, argv)
    int argc;
    char **argv;

{
	Window  w;
	GC gc;
	XGCValues xgcv;
	char *windowName = "Test of Zoid Extension";
	char line[30];
	XSetWindowAttributes xswa;
	XEvent pe;
	Pixmap tile_pat, stip_pat;
	int     iteration = 0;
	XColor screen_def_blue, exact_def_blue;
	XColor screen_def_red, exact_def_red;

	/*_Xdebug = 1;*/   /* turn on synchronization */

	StartConnectionToServer(argc, argv);

	xswa.event_mask = ExposureMask;
	w = XCreateSimpleWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)), 100, 100, 340, 340,
	    2, 1, 0);
	XChangeWindowAttributes(dpy, w, CWEventMask,  &xswa);

	XChangeProperty(dpy,
	    w, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
	    windowName, strlen(windowName));


	XMapWindow(dpy, w);

	gc = XCreateGC(dpy, w, 0, 0);
	XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "blue",
	       &screen_def_blue,  &exact_def_blue);
	XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "red",
	       &screen_def_red,  &exact_def_red);
	XSetForeground(dpy, gc, screen_def_red.pixel);
	XSetBackground(dpy, gc, screen_def_blue.pixel);
	tile_pat = MakePixmap(w, doublearrow32_bits, DefaultDepth(dpy, DefaultScreen(dpy)));
	stip_pat = MakePixmap(w, stipple32_bits, 1); /* depth must be 1 */
	XSetTile(dpy, gc, tile_pat);
	XSetStipple(dpy, gc, stip_pat);
	XSync(dpy, 0);

	while (1) {
	    if (XPending(dpy))
		XNextEvent(dpy, &pe);
	    XSync(dpy, 0);

	    xgcv.fill_style = fill_styles[iteration];
	    XChangeGC(dpy, gc, GCFillStyle, &xgcv);
	    switch (mode) {
	    case TEMPLATE:
		template(dpy, w, gc);
		break;
	    case RECTANGLES:
		rectangles(dpy, w, gc);
		break;
	    case XTRAPEZOIDS:
		xtrapezoids(dpy, w, gc);
		break;
	    case YTRAPEZOIDS:
		ytrapezoids(dpy, w, gc);
		break;
	    }
	    XSync(dpy, 0);
	    sleep(1);
	    if (++iteration >= 3)
		iteration = 0;
	}
}
