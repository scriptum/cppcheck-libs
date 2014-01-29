/* $Header: XBezier.c,v 1.1 87/09/02 02:18:53 toddb Exp $ */
/* 
 * THIS IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
 * INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE
 * PRACTICE.
 *  
 * University of Michigan CITI
 * Greg Cockroft  greg@citi.umich.edu
*/

#include "X.h"
#include "Xlibint.h" 
#include "Xproto.h"
#include "Xprotostr.h"
#include "bezier.h"
#include <stdio.h>

/* If there are more than 64 file descriptors allowed on a machine, modify accordingly */
XExtCodes  *Codes[] = { 
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/* 
 *   XPolyBezier: Xlib interface for Bezier Spline Extension.
 *   Tue Aug 11                     
*/

XPolyBezier(dpy, d, gc, bezs, nbez)
    register Display *dpy; 
    Drawable d;
    GC       gc;
    xBezier  *bezs;
    short    nbez;
{
    register xPolyBezierReq *req;
    register long length;

    LockDisplay(dpy);
    FlushGC(dpy, gc);    
    GetExtReq(PolyBezier, req, ConnectionNumber(dpy));
    req->drawable = d;
    req->gc = gc->gid;

    req->length += nbez << 2;
       /* each bezier is 8 16-bit integers, or 16 bytes */
    length = nbez << 4; 
       /* do this here, not in arguments to PackData, since PackData
          may be a macro which uses its arguments more than once */
    PackData (dpy, (char *) bezs, length);
    UnlockDisplay(dpy);
    SyncHandle();
    }

extern XExtCodes *XInitExtension();

/* 
 *  InitBezierExt : Get the goodies on an extension.
 *   WARNING: This does not setup a cleanup handler for Display Close
 *    like a good extension should.
*/

XExtCodes
*InitBezierExt(dpy)
Display *dpy;
{
XExtCodes   *ret;
        if(ret = XInitExtension(dpy, "bezier")) 
            fprintf(stderr,"in InitBezierExt major opcode is %d\n",ret->major_opcode);
        else {
            fprintf(stderr,"InitBezierExt: Could not Initialize extension\n");
            exit(-1);
            }
        return(ret);
}
