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

/* $Header: XFillZoids.c,v 1.2 87/09/10 18:45:40 sun Exp $ */

#include "Xlibint.h"
#include "zoid.h"

static int ZoidReqCode = 0;

XFillTrapezoids(dpy, d, gc, zoids, n_zoids)
register Display *dpy;
Drawable d;
GC gc;
xXYTraps *zoids;
int n_zoids;
{
    register xPolyFillZoidReq *req;
    register long nbytes;
    int first_event, first_error;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    if (!ZoidReqCode) {
	XQueryExtension(
	    dpy, ZOIDNAME, &ZoidReqCode, &first_event, &first_error);
    }
    GetReq(PolyFillZoid, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->reqType = ZoidReqCode;
    req->zoidReqType = X_PolyFillZoid;

    /* sizeof(xXYTraps) will be a multiple of 4 */
    req->length += n_zoids * (sizeof(xXYTraps) / 4);

    nbytes = n_zoids * sizeof(xXYTraps);

    PackData (dpy, (char *) zoids, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetZoidAlignment(dpy, gc, alignment)
register Display *dpy;
GC gc;
unsigned int alignment;
{
    register xSetZoidAlignmentReq *req;
    int first_event, first_error;

    LockDisplay(dpy);
    if (!ZoidReqCode) {
	XQueryExtension(
	    dpy, ZOIDNAME, &ZoidReqCode, &first_event, &first_error);
    }
    GetReq (SetZoidAlignment,req);
    req->gc = gc->gid;
    req->reqType = ZoidReqCode;
    req->zoidReqType = X_SetZoidAlignment;
    req->alignment = (CARD8) alignment;
    UnlockDisplay(dpy);
    SyncHandle();
}

