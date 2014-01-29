/* $Header: bezier.c,v 1.4 87/09/09 20:24:22 sun Exp $ */
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
 *   This file implements the bezier X11 extension.  Wed Aug 12
 *
 *     ProcPolyBezier() Called by bezier extension dispatch to draw beziers.
 *     ProcBezierDispatch() Bezier extension dispatch for nonswapped clients.
 *     SProcBezierDispatch() Swapping dispatch. (Not Written).
 *     BezierExtensionInit() Inits the bezier extension.
 *     bez() converts beziers to polylines.
*/

#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "Xprotostr.h"
#include "windowstr.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "osstruct.h"
#include "resource.h"
#include "selection.h"
#include "colormapst.h"
#include "dixstruct.h"
#include "extension.h"
#include "input.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "opaque.h"

#include "extnsionst.h"
#include "bezier.h"

int  BezierReqCode = 0;

#define MAX(a,b) ((a) > (b) ? (a) : (b)) 
#define MIN(a,b) ((a) < (b) ? (a) : (b)) 
#define FIXED(a) (((int) (a)) << 16)
#define INT(a) (((a) + (1 << 15)) >> 16 )

#define LEGAL_NEW_RESOURCE(id)\
if ((LookupID(id, RT_ANY, RC_CORE) != 0) || (id & SERVER_BIT) \
	|| (client->clientAsMask != CLIENT_BITS(id)))\
        return(BadIDChoice)

/* The documented call in doc/extension.doc is GetGCAndDrawableAndValidate.
 * This does not exist. I pulled the following macro out of dispatch.c, if
 * this is what extension writers are supposed to use, then it should be placed
 * in a header file which can be included. Bug??
*/

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, pGC, client)\
    if ((client->lastDrawableID != drawID) || (client->lastGCID != stuff->gc))\
    {\
        if (client->lastDrawableID != drawID)\
	{\
    	    pDraw = (DrawablePtr)LookupID(drawID, RT_DRAWABLE, RC_CORE);\
    	    if (!pDraw)\
	    {\
	        client->errorValue = drawID; \
                return (BadDrawable);\
	    }\
	    if ((pDraw->type == DRAWABLE_WINDOW) || \
		(pDraw->type == DRAWABLE_PIXMAP))\
    	    {\
	        client->lastDrawable = (DrawablePtr)pDraw;\
	        client->lastDrawableID = drawID;\
	    }\
            else\
	    {\
	        client->errorValue = drawID;\
                return (BadDrawable);\
	    }\
        }\
        else\
	    pDraw = (DrawablePtr)client->lastDrawable;\
        if (client->lastGCID != stuff->gc)\
	{\
	    pGC = (GC *)LookupID(stuff->gc, RT_GC, RC_CORE);\
            if (!pGC)\
            {\
	        client->errorValue = stuff->gc;\
	        return (BadGC);\
            }\
            client->lastGC = (GCPtr)pGC;\
            client->lastGCID = stuff->gc;\
        }\
        else\
            pGC = (GC *) client->lastGC;\
        if ((pGC->depth != pDraw->depth) || (pGC->pScreen != pDraw->pScreen))\
	{\
            client->errorValue = stuff->gc;\
	    client->lastGCID = -1;\
	    return (BadMatch);\
         }\
    }\
    else\
    {\
        pGC = (GC *) client->lastGC;\
        pDraw = (DrawablePtr)client->lastDrawable;\
    }\
    if (pGC->serialNumber != pDraw->serialNumber)\
    { \
	ValidateGC(pDraw, pGC);\
    }

static short    *pointptr;
static int      npoint;        /* Number of points in Polyline */

/*********** 
   ProcPolyBezier: Convert Beziers to Polylines by calling bez, and then call 
                   through the GC to display the polyline.
***********/
int
ProcPolyBezier(client)
register ClientPtr client;
{
    int nbeziers;
    register DrawablePtr pDraw;
    register GC *pGC; 
    REQUEST(xPolyBezierReq);
    short   points[2000];      /* Storage for polyline created from Bezier */
    xBezier  *b;

    /*ErrorF("in  ProcPolyBezier \n");*/
    REQUEST_AT_LEAST_SIZE(xPolyBezierReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    nbeziers = (stuff->length << 2) - sizeof(xPolyBezierReq);
    if(nbeziers % sizeof(xBezier) != 0)
       return(BadLength);
    nbeziers >>= 4;
    if (nbeziers < 1)
      return(BadLength);
    
    b = (xBezier *)  &stuff[1];
    while(nbeziers--) {
       npoint = 1; 
       pointptr = points;
       *pointptr++ = b->x0;
       *pointptr++ = b->y0;
       /*printf("x0 %d y0 %d x3 %d y3 %d\n",b->x0,b->y0,b->x3,b->y3);*/
       bez(FIXED(b->x0),FIXED(b->y0),FIXED(b->x1),FIXED(b->y1),
           FIXED(b->x2),FIXED(b->y2),FIXED(b->x3),FIXED(b->y3));
      /*printf("npoint is %d\n",npoint);*/
     (*pGC->Polylines)(pDraw, pGC, CoordModeOrigin, npoint, points);
       b++;
       }
    return(client->noClientException);
}

/*****************
 * ProcBezierDispatch
 *****************/

int
ProcBezierDispatch(client)
register ClientPtr client;
{
	REQUEST(xReq);
	/*printf("in ProcBezierDispatch stuff->data is %x\n",stuff->data);*/
	if (stuff->data == X_PolyBezier )
		return(ProcPolyBezier(client));
	else {
		SendErrorToClient(client,
			BezierReqCode, stuff->data, 0, BadRequest);
		return(BadRequest);
	}
}

/* Macros needed for byte-swapping, copied from swapreq.c.  Really
   should be in a header file somewhere. */

#define LengthRestS(stuff) \
    ((stuff->length << 1)  - (sizeof(*stuff) >> 1))

#define SwapRestS(stuff) \
    SwapShorts(stuff + 1, LengthRestS(stuff))

int
SProcBezierDispatch(client)
register ClientPtr client;
{
  	register char n;

	REQUEST(xPolyBezierReq);
	if (stuff->minor_opcode == X_PolyBezier) {
	    swaps(&stuff->length, n);
	    swapl(&stuff->drawable, n);
	    swapl(&stuff->gc, n);
	    SwapRestS(stuff);
	}
	return (ProcBezierDispatch (client));
}

void
BezierReset()
{
}


/*
 * BezierExtensionInit
 *
 * Called from InitExtensions in server/dix/extension.c
 *
 * Bezier extension at this time has no events or errors 
 * (other than the core errors)
*/

void
BezierExtensionInit()
{
	ExtensionEntry *extEntry;

	extEntry = AddExtension("bezier", 0, 0, ProcBezierDispatch,
				SProcBezierDispatch, BezierReset);
	if (extEntry) {
		BezierReqCode = extEntry->base;
	    }
    else
        FatalError("BezierExtensionInit: AddExtensions failed\n");
}          

/*
 * bez: Subdivide a Bezier spline, until it is thin enough to be
 *      considered a line. Store line point in static array points.
*/

bez(x0, y0, x1, y1, x2, y2, x3, y3)
    int x0, y0, x1, y1, x2, y2, x3, y3;
{
	int maxx,minx,maxy,miny;         
          /* find bounding box of 4 control points */
	maxx = x0;
	maxx = MAX(maxx, x1);
	maxx = MAX(maxx, x2);
	maxx = MAX(maxx, x3);

	maxy = y0;
	maxy = MAX(maxy, y1);
	maxy = MAX(maxy, y2);
	maxy = MAX(maxy, y3);

	minx = x0;
	minx = MIN(minx, x1);
	minx = MIN(minx, x2);
	minx = MIN(minx, x3);

	miny = y0;
	miny = MIN(miny, y1);
	miny = MIN(miny, y2);
	miny = MIN(miny, y3);

    if (((maxx - minx) < FIXED(2)) || ((maxy - miny) < FIXED(2))) { 
          /* Consider it a line segment */
       npoint++;             
       /*printf("add point (%d,%d) \n", INT(x3),   INT(y3));*/
       *pointptr++ = INT(x3);
       *pointptr++ = INT(y3);
    	}
    else {
	register int tempx, tempy;
		/* Subdivide into 2 new beziers */
	tempx = (x0 >> 3) + 3 * (x1 >> 3) + 3 * (x2 >> 3) + (x3 >> 3);
	tempy = (y0 >> 3) + 3 * (y1 >> 3) + 3 * (y2 >> 3) + (y3 >> 3);
	bez(x0, y0,
		 (x0 >> 1) + (x1 >> 1), (y0 >> 1) + (y1 >> 1),
		 (x0 >> 2) + (x1 >> 1) + (x2 >> 2),
		 (y0 >> 2) + (y1 >> 1) + (y2 >> 2),
		 tempx, tempy);
	bez(tempx, tempy,
		 (x3 >> 2) + (x2 >> 1) + (x1 >> 2),
		 (y3 >> 2) + (y2 >> 1) + (y1 >> 2),
		 (x3 >> 1) + (x2 >> 1), (y3 >> 1) + (y2 >> 1),
		 x3, y3);
    }
}
