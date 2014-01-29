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

/* $Header: zoid.c,v 1.5 87/09/10 18:40:48 sun Exp $ */
#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "pixmapstr.h"
#include "gcstruct.h"   /* should be able to get GCInterestRec without GC */
#include "extnsionst.h"
#include "dixstruct.h"
#include "resource.h"
#include "opaque.h"
#include "zoid.h"

typedef struct _ZoidState {
    char alignment;
    void (*PolyFillZoid)();
} ZoidStateRec, *ZoidStatePtr;

static int ZoidReqCode = 0;
static short ZoidClass = 0;

/* ZoidProcVector is used in the ZOID validate routine */

void (* ZoidProcVector[6]) ();
extern void ValidateGC();

/****************
 * ZoidExtensionInit
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 * ZOID has no events or errors (other than the core errors)
 ****************/

void
ZoidExtensionInit()
{
    ExtensionEntry *extEntry, *AddExtension();
    int ProcZoidDispatch(), SProcZoidDispatch();
    void  ZoidResetProc();

    extEntry = AddExtension(ZOIDNAME, 0, 0, ProcZoidDispatch,
		   SProcZoidDispatch, ZoidResetProc);
    if (extEntry)
    {
	ZoidReqCode = extEntry->base;
	(void)MakeAtom(ZOIDNAME, 13, TRUE);
	ZoidClass = CreateNewResourceClass();

	/*
	 * at PolyFillTrapezoid time, if the procedure is NULL, then
	 * points are converted to a polygon and the gc's fill polygon call
	 * is used
	 */
	bzero(ZoidProcVector, sizeof(ZoidProcVector));
    } else {
	FatalError("ZoidExtensionInit: AddExtensions failed\n");
    }
}

/****************
 * ZoidValidateState
 *
 * Called from ValidateGC.
 *
 ****************/

void
ZoidValidateState(pGC, pGCI, mask, pDrawable)
    GCPtr pGC;
    GCInterestPtr pGCI;
    int mask;
    Drawable pDrawable;
{
    ZoidStatePtr pZoidState = (ZoidStatePtr)pGCI->extPriv;

    switch (GetGCValue(pGC,fillStyle))
    {
	case FillSolid:
	    if (pZoidState->alignment == XZoid_YAligned)
		pZoidState->PolyFillZoid = ZoidProcVector[SOLID_Y];
	    else
		pZoidState->PolyFillZoid = ZoidProcVector[SOLID_X];
	    break;
	case FillTiled:
	    if (pZoidState->alignment == XZoid_YAligned)
		pZoidState->PolyFillZoid = ZoidProcVector[TILE_Y];
	    else
		pZoidState->PolyFillZoid = ZoidProcVector[TILE_X];
	    break;
	case FillStippled:
	case FillOpaqueStippled:
	    if (pZoidState->alignment == XZoid_YAligned)
		pZoidState->PolyFillZoid = ZoidProcVector[STIP_Y];
	    else
		pZoidState->PolyFillZoid = ZoidProcVector[STIP_X];
	    break;
     }
#ifdef GCExtensionInterest
     pGCI->ValInterestMask &= ~GCExtensionInterest;
#endif GCExtensionInterest
}

/****************
 * AddAndInitZoidInterest
 *
 * Sets up at GCInterest ptr and places it on the GC's queue.
 *
 ****************/

static GCInterestPtr
AddAndInitZoidInterest(pGC)
    GCPtr pGC;
{
#define ZOID_INTEREST (GCFillStyle | GCFillRule | GCTile | GCStipple \
	| GCTileStipXOrigin | GCTileStipYOrigin)

    GCInterestPtr pGCI;
    ZoidStatePtr pZoidState;
    void ZoidFreeState();

    pGCI = (GCInterestPtr)Xalloc(sizeof(GCInterestRec));
    InsertGCI(pGC,pGCI,GCI_LAST,0)

    pGCI->ValInterestMask = ZOID_INTEREST;
    pGCI->ValidateGC = ZoidValidateState;
    pGCI->ChangeInterestMask = 0x0;
    pGCI->ChangeGC = NULL;
    pGCI->CopyGCSource = NULL;
    pGCI->CopyGCDest = NULL;
    pGCI->DestroyGC = ZoidFreeState;

    pZoidState = (ZoidStatePtr)Xalloc(sizeof(ZoidStateRec));
    pZoidState->alignment = XZoid_YAligned;
    pGCI->extPriv = (pointer)pZoidState;

    ZoidProcVector[SOLID_Y] = LookupProc("PolySolidYAlignedTrapezoid", pGC);
    ZoidProcVector[STIP_Y]  = LookupProc("PolyStipYAlignedTrapezoid", pGC);
    ZoidProcVector[TILE_Y] = LookupProc("PolyTiledYAlignedTrapezoid", pGC);
    ZoidProcVector[SOLID_X] = LookupProc("PolySolidXAlignedTrapezoid", pGC);
    ZoidProcVector[STIP_X]  = LookupProc("PolyStipXAlignedTrapezoid", pGC);
    ZoidProcVector[TILE_X] = LookupProc("PolyTiledXAlignedTrapezoid", pGC);

#ifdef GCExtensionInterest
    (* pGCI->ValidateGC) (pGC, pGCI, ZOID_INTEREST | GCExtensionInterest,
    NULL);
#else GCExtensionInterest
    (* pGCI->ValidateGC) (pGC, pGCI, ZOID_INTEREST, NULL);
#endif GCExtensionInterest
    return (pGCI);
}


/*****************
 * ProcPolyFillZoid
 *
 *****************/

static int
ProcPolyFillZoid(client)
    register ClientPtr client;
{
    int ntraps;
    register GCPtr pGC;
    register DrawablePtr pDraw;
    GCInterestPtr pGCI;
    REQUEST(xPolyFillZoidReq);          /* xPolyFillZoidReq *stuff; */
    ZoidStatePtr pZoidState;
    void ZoidDeleteState(), UseGCFillPoly();

    REQUEST_AT_LEAST_SIZE(xPolyFillZoidReq);
    GetGCAndDrawableAndValidate(stuff->gc, pGC, stuff->drawable,
	pDraw, client);

    pGCI = (GCInterestPtr) LookupID(stuff->gc, RT_GC, ZoidClass);
    if (!pGCI)
    {
	pGCI = AddAndInitZoidInterest(pGC);
	AddResource(stuff->gc, RT_GC, pGCI, ZoidDeleteState, ZoidClass);
    }
    ntraps = ((stuff->length << 2) - sizeof(xPolyFillZoidReq)) /
	     sizeof(xXYTraps);
    pZoidState = (ZoidStatePtr)pGCI->extPriv;
    if (pZoidState->PolyFillZoid)
	(* pZoidState->PolyFillZoid)(pDraw, pGC, ntraps, &stuff[1]);
    else
	UseGCFillPoly(pDraw, pGC, pZoidState, ntraps, &stuff[1]);
    return (Success);
}


/*****************
 * ProcSetTrapazoidAlignment
 *
 *****************/

static int
ProcSetZoidAlignment(client)
    register ClientPtr client;
{
    register GCPtr pGC;
    GCInterestPtr pGCI;
    REQUEST(xSetZoidAlignmentReq);
    ZoidStatePtr pZoidState;
    void ZoidDeleteState();

    REQUEST_AT_LEAST_SIZE(xSetZoidAlignmentReq);
    if ((stuff->alignment != XZoid_YAligned)
    &&  (stuff->alignment != XZoid_XAligned))
    {
	SendErrorToClient(client, ZoidReqCode, X_SetZoidAlignment,
			  stuff->gc, BadMatch);
	return(BadMatch);
    }
    pGCI = (GCInterestPtr)LookupID(stuff->gc, RT_GC, ZoidClass);

    pGC = (GC *)LookupID(stuff->gc, RT_GC, RC_CORE);
    if (!pGC)
    {
	SendErrorToClient(client, ZoidReqCode, X_SetZoidAlignment,
			  stuff->gc, BadGC);
	return(BadGC);
    }

    if (!pGCI)
    {
	pGCI = AddAndInitZoidInterest(pGC);
	AddResource(stuff->gc, RT_GC, pGCI, ZoidDeleteState, ZoidClass);
    }
    pZoidState = (ZoidStatePtr)pGCI->extPriv;
    pZoidState->alignment = stuff->alignment;

	/* tell gc that an extension must be called at next Validate */
    SetGCMask(pGC, GCExtensionInterest, GCExtensionInterest);

	/* say that ZOID is the extension that is interested */
    pGCI->ValInterestMask |= GCExtensionInterest;
    return (Success);
}

/*****************
 * ProcZoidDispatch
 *****************/

int
ProcZoidDispatch(client)
    register ClientPtr client;
{
    REQUEST(xReq);
    if (stuff->data == X_PolyFillZoid)
	return(ProcPolyFillZoid(client));
    else if (stuff->data == X_SetZoidAlignment)
	return(ProcSetZoidAlignment(client));
    else
    {
	SendErrorToClient(client, ZoidReqCode, stuff->data, 0, BadRequest);
	return(BadRequest);
    }
}

int
SProcZoidDispatch(client)
    register ClientPtr client;
{
    REQUEST(xReq);
    if (stuff->data == X_PolyFillZoid)
	return(SProcPolyFillZoid(client));
    else if (stuff->data == X_SetZoidAlignment)
	return(SProcSetZoidAlignment(client));
    else
    {
	SendErrorToClient(client, ZoidReqCode, stuff->data, 0, BadRequest);
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
SProcPolyFillZoid(client)
     register ClientPtr client;
{
     register char n;

     REQUEST(xPolyFillZoidReq);
     swaps(&stuff->length, n);
     swapl(&stuff->drawable, n);
     swapl(&stuff->gc, n);
     SwapRestS(stuff);
     return (ProcPolyFillZoid(client));
}

int
SProcSetZoidAlignment(client)
     register ClientPtr client;
{
     register char n;

     REQUEST(xSetZoidAlignmentReq);
     swaps(&stuff->length, n);
     swapl(&stuff->gc, n);
     return (ProcSetZoidAlignment(client));
}

void
ZoidResetProc()
{
}

void
ZoidFreeState()
{
}

void
ZoidDeleteState()
{
}

/*
 *  Y aligned
 *
 *         [x1,y1]     [x2,y1]
 *            +-----------+
 *           /             \
 *          +---------------+
 *       [x3,y2]         [x4,y2]
 *
 *  X aligned
 *
 *              [x2,y3]
 *                 +
 *                /|
 *       [x1,y1] / |
 *              +  |
 *              |  |
 *              +  |
 *       [x1,y2] \ |
 *                \|
 *                 +
 *              [x2,y4]
 */
void
UseGCFillPoly(pDraw, pGC, pZoidState, ntraps, traps)
    DrawablePtr pDraw;
    GCPtr pGC;
    ZoidStatePtr pZoidState;
    int ntraps;
    xXYTraps *traps;
{
    DDXPointRec         zoidpoly[4];
    int                 i;

    for (i = 0; i < ntraps; i++, traps++) {
	if (pZoidState->alignment == XZoid_XAligned) {
	    zoidpoly[0].x = traps->Xt.x1;
	    zoidpoly[0].y = traps->Xt.y1;
	    zoidpoly[1].x = traps->Xt.x2;
	    zoidpoly[1].y = traps->Xt.y3;
	    zoidpoly[2].x = traps->Xt.x2;
	    zoidpoly[2].y = traps->Xt.y4;
	    zoidpoly[3].x = traps->Xt.x1;
	    zoidpoly[3].y = traps->Xt.y2;
	} else {
	    zoidpoly[0].x = traps->Yt.x1;
	    zoidpoly[0].y = traps->Yt.y1;
	    zoidpoly[1].x = traps->Yt.x2;
	    zoidpoly[1].y = traps->Yt.y1;
	    zoidpoly[2].x = traps->Yt.x4;
	    zoidpoly[2].y = traps->Yt.y2;
	    zoidpoly[3].x = traps->Yt.x3;
	    zoidpoly[3].y = traps->Yt.y2;
	}
	(*GetGCValue(pGC, FillPolygon)) (pDraw, pGC, Convex,
					 CoordModeOrigin, 4, zoidpoly);
    }
}


