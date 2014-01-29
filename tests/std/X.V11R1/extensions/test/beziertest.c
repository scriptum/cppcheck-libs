/* $Header: beziertest.c,v 1.1 87/09/02 02:19:11 toddb Exp $ */
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
/* 
 * THIS IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
 * INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE
 * PRACTICE.
 *  
 * University of Michigan CITI
 * Greg Cockroft  greg@citi.umich.edu
*/

/******************************************************************************
 * Description 
 *	Display a part picture using bezier extension.
 *
 * Arguments:
 *	-i            Invert black & white
 *	=wxh+x+y	X geometry for new window (default 600x600 centered)
 *	host:display	X display on which to run
 *****************************************************************************/



#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "bezier.h"


extern GC XCreateGC();
extern long time();
extern long random();
   
#define MAX_BEZS 500
#define MAX_LINES 500

Display *dpy;
Window  win;
GC      gc;


short    line_points[MAX_LINES*4];
short    bez_points[MAX_BEZS*8];
short    sline_points[MAX_LINES*4];
short    sbez_points[MAX_BEZS*8];
short    nlines = 0;
short    nbezs = 0;

main(argc, argv)
int argc;
char **argv;
	{
	char *display = NULL;
	char *geom = NULL;
	int fg, bg;
	int invert = 0;
       xBezier   b;
       FILE      *fp;
       char      buf[256];
       XEvent pe;
       XExposeEvent *ee;
       short     *btemp,*ltemp;

	int winX, winY, winW, winH;
	XSetWindowAttributes xswa;


	XEvent xev;
	XGCValues xgcv;

	/* Process arguments: */

	while (*++argv)
		{
		if (**argv == '=') 
			geom = *argv;
		else if (index(*argv, ':'))
			display = *argv;
		else if (!strcmp(*argv, "-i"))
			invert = 1;
		}


              
           /* READ in the DATA */
       fp = fopen("datafile","r");
       if(fp == NULL) {
           fprintf(stderr,"Could not open file datafile\n");
           exit(-1);
           }
       
       btemp = bez_points;
       ltemp = line_points;
       while(fscanf(fp,"%s",buf) == 1) {
           switch(*buf) {
               case 'b' :
                    fscanf(fp,"%hd %hd %hd %hd %hd %hd %hd %hd",&btemp[0],&btemp[1],&btemp[2],&btemp[3],
                             &btemp[4],&btemp[5],&btemp[6],&btemp[7]);
                    btemp += 8;
                    nbezs++; 
                    if(nbezs > MAX_BEZS) {
                        fprintf(stderr,"Too many splines in file\n");
                        exit(-1);
                        }
                    break;
               case 'l' :
                    fscanf(fp,"%hd %hd %hd %hd",&ltemp[0],&ltemp[1],&ltemp[2],&ltemp[3]);
                    ltemp += 4;
                    nlines++;
                    if(nlines > MAX_LINES) {
                        fprintf(stderr,"Too many lines in file\n");
                        exit(-1);
                        }
                     break; 
               default :
                    puts("Bad Datafile");
                    break;
               }
           }
    
      printf("CAD: read in %d lines %d beziers\n",nlines,nbezs);

	if (!(dpy= XOpenDisplay(display)))
	        {
		perror("Cannot open display\n");
		exit(-1);
	        }

	if (invert)
		{
		fg = BlackPixel(dpy, DefaultScreen(dpy));
		bg = WhitePixel(dpy, DefaultScreen(dpy));
		}
	else
		{
		fg = WhitePixel(dpy, DefaultScreen(dpy));
		bg = BlackPixel(dpy, DefaultScreen(dpy));
		}

	/* Set up window parameters, create and map window  */

	winW = 600;
	winH = 600;
	winX = (DisplayWidth(dpy, DefaultScreen(dpy)) - winW) >> 1;
	winY = (DisplayHeight(dpy, DefaultScreen(dpy)) - winH) >> 1;
	if (geom) 
		XParseGeometry(geom, &winX, &winY, &winW, &winH);

	xswa.event_mask = ExposureMask | StructureNotifyMask;
	xswa.background_pixel = bg;
	xswa.border_pixel = fg;
	win = XCreateWindow(dpy, DefaultRootWindow(dpy), 
            winX, winY, winW, winH, 0, 
            DefaultDepth(dpy, DefaultScreen(dpy)), 
            InputOutput, DefaultVisual(dpy, DefaultScreen(dpy)),
            CWEventMask | CWBackPixel | CWBorderPixel, &xswa);
	     XChangeProperty(dpy, win, XA_WM_NAME, XA_STRING, 8, 
	     PropModeReplace, "Cad", 3);
	XMapWindow(dpy, win);


	/* Set up a graphics context: */

	gc = XCreateGC(dpy, win, 0, NULL);
	XSetForeground(dpy, gc, fg);
	XSetBackground(dpy, gc, bg);

    while(1) {
       XNextEvent(dpy, &pe);	
	if (pe.type == Expose) {
	    ee = (XExposeEvent *) &pe;
	    while (ee->count) {
               XNextEvent(dpy, &pe);	    
	        ee = (XExposeEvent *) &pe;
	        }
	    }
	else if (pe.type == ConfigureNotify) {
	    XConfigureEvent *ce = (XConfigureEvent *)&pe;
	    winX = ce->x;
	    winY = ce->y;
	    winW = ce->width;
	    winH = ce->height;
	    }
           else 
	       printf("Unknown event type: %d\n", pe.type);

	XClearArea(dpy, win, 0, 0, winW, winH, 0);
printf("width %d height %d\n",winW,winH);
       draw(winW,winH);
       }
}

 
/****************
   draw:  Scale and draw the lines and beziers
***************/

/* The data points were created to fit in a 2048x2048 square,
   this means scale_factor will always be small enough so that
   we don't need to worry about overflows.
*/
 
#define SCALE(a) ((((int) (a)) * scale_factor) >> 16)
draw(w,h)
int   w,h;
{                 
register short  *temp1, *temp2;
register short  i;
register int  scale_factor;

      if(h < w)
           w = h;

      scale_factor = w << 5;
 
        /* scale lines to window */     
      temp2 = line_points;
      temp1 = sline_points; 

      i = nlines * 4;
      while(i--) {
         *temp1 = SCALE(*temp2);
         temp1++;
         temp2++;
         } 
                          
        /* scale beziers to window */
      temp2 = bez_points;
      temp1 = sbez_points; 

      i = nbezs * 8;
      while(i--) {
         *temp1 = SCALE(*temp2);
         temp1++;
         temp2++;
         } 


       XPolyBezier(dpy, win, gc, sbez_points, nbezs);
       XDrawSegments (dpy, win, gc, sline_points, nlines);
       XFlush(dpy);
}
 
