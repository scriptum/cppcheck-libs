/* $Header: bezier.h,v 1.1 87/09/02 02:18:45 toddb Exp $ */
/* 
 * THIS IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
 * INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE
 * PRACTICE.
 *  
 * University of Michigan CITI
 * Greg Cockroft  greg@citi.umich.edu
*/

/*
 *   Header file for the bezier X11 extension.    Wed Aug 12
*/

#define  X_PolyBezier 1   /* The minor_opcode for PolyBezier */
#include "Xmd.h"

/* These are defined in Xlib.h, but we can't include it, becuase
   it interferes with includes in bezier.c
*/

#ifndef _XLIB_H_
#define _XLIB_H_
typedef struct {		/* public to extension, cannot be changed */
	int extension;		/* extension number */
	int major_opcode;	/* major op-code assigned by server */
	int first_event;	/* first event number for the extension */
	int first_error;	/* first error number for the extension */
} XExtCodes;
#endif

extern XExtCodes  *Codes[];
extern XExtCodes *InitBezierExt();

/* This is the format of our request */

typedef struct {
    CARD8 reqType;
    BYTE minor_opcode;
    CARD16 length B16;
    Drawable drawable B32;
    GContext gc B32;
} xPolyBezierReq;    

#define GetExtReq(name, req, fd) \
	if ((dpy->bufptr + sizeof(x/**/name/**/Req)) > dpy->bufmax)\
		_XFlush(dpy);\
   if(!Codes[fd]) {  \
        Codes[fd] = InitBezierExt(dpy);\
        }\
	req = (x/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
	req->reqType =  Codes[fd]->major_opcode;\
	req->minor_opcode = X_/**/name;\
	req->length = (sizeof(x/**/name/**/Req))>>2;\
	dpy->bufptr += sizeof(x/**/name/**/Req);\
	dpy->request++

typedef struct {
     CARD16 x0,y0,x1,y1,x2,y2,x3,y3;
} xBezier;
