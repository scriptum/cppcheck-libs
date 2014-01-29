/* $Header: cursor.c,v 1.2 87/09/12 22:37:49 toddb Exp $ */
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

#include <stdio.h>
#include "X11/Xlib.h"
#include "X11/Xatom.h"
#include "X11/cursorfont.h"


#define rnd(x)		(random() % (x))

Display *dpy;
int cursornum = XC_X_cursor;

StartConnectionToServer(argc, argv)
int	argc;
char	*argv[];
{
    char *display;
    int i;

    display = NULL;
    for(i = 1; i < argc; i++)
    {
        if(index(argv[i], ':') != NULL)
	    display = argv[i];
    }
    if (!(dpy = XOpenDisplay(display)))
    {
       perror("Cannot open display\n");
       exit(0);
   }
}

main(argc, argv)
    int argc;
    char **argv;

{
	int	i;
	Window	w;
	GC gc;
	XGCValues xgcv;
        char *windowName = "Cursor Color Test";
        char line[30];
        XSetWindowAttributes xswa;
	XEvent pe;
        Visual visual;
	char *fg_color = "black", *bg_color = "white";

	StartConnectionToServer(argc, argv);

	while (argc-- > 0) {
	    if (**argv == '-') {
		switch (*++*argv) {
		case 'f':
			if (argc > 0) {
			    fg_color = *++argv;
			    argc--;
			}
			break;
		case 'b':
			if (argc > 0) {
			    bg_color = *++argv;
			    argc--;
			}
			break;
		case 'c':
			if (argc > 0) {
			    cursornum = atoi(*++argv);
			    argc--;
			}
		}
	    }
	    argv++;
	}

	fprintf(stderr, "X%d.%d server from %s release %d\n",
		ProtocolVersion(dpy), ProtocolRevision(dpy),
		ServerVendor(dpy), VendorRelease(dpy));
	fprintf(stderr, "Cursor should be %s on %s\n", fg_color, bg_color);
        xswa.event_mask = ExposureMask;
	xswa.background_pixel = BlackPixel(dpy, DefaultScreen(dpy));
        visual.visualid = CopyFromParent;
        w = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)),
	    100, 100, 200, 200,
	    2, DefaultDepth(dpy, DefaultScreen(dpy)), InputOutput, &visual,
	    (CWBackPixel|CWEventMask),  &xswa);

	XChangeProperty(dpy, 
	    w, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
	    windowName, strlen(windowName));

        
	XMapWindow(dpy, w);
	
	gc = XCreateGC(dpy, w, 0, 0);
        XSync(dpy, 0);
	
	{
	    Cursor cu;
	    XColor hfg, hbg;
	    XColor fg, bg;

	    if (XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
		fg_color, &hfg, &fg) == 0) {
		fprintf(stderr, "Can't find %s\n", fg_color);
		exit (1);
	    }
	    if (XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
		bg_color, &hbg, &bg) == 0) {
		fprintf(stderr, "Can't find %s\n", bg_color);
		exit (1);
	    }
	    cu = XCreateFontCursor(dpy, cursornum);
	    XRecolorCursor(dpy, cu, &hfg, &hbg);
	    XDefineCursor(dpy, w, cu);
	}
	/* wait for exposure event so window manager can get reparented, etc */
        while (1)
        {
            XNextEvent(dpy, &pe);   
            XSync(dpy, 0);
        }
        XSync(dpy, 0);

}

