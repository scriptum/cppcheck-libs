/* 
 * $Locker:  $ 
 */ 
static char	*rcsid = "$Header: util.c,v 1.1 87/06/22 16:08:17 toddb Exp $";
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

int ChrToCode[256], CodeToChr[256];

Punt(str)
  char *str;
{
    fprintf(stderr, "Punt: %s\n", str);
    exit(1);
}


MyXSelectInput(dpy, w, mask)
  Display *dpy;
  Window w;
  int mask;
{
    XSetWindowAttributes attributes;
    attributes.event_mask = mask;
    XChangeWindowAttributes(dpy, w, CWEventMask, &attributes);
}


InitUtil()
{
    int i;
    for (i=0 ; i<256 ; i++) ChrToCode[i] = 0;
    ChrToCode['a'] = 194;    ChrToCode['b'] = 217;    ChrToCode['c'] = 206;
    ChrToCode['d'] = 205;    ChrToCode['e'] = 204;    ChrToCode['f'] = 210;
    ChrToCode['g'] = 216;    ChrToCode['h'] = 221;    ChrToCode['i'] = 230;
    ChrToCode['j'] = 226;    ChrToCode['k'] = 231;    ChrToCode['l'] = 236;
    ChrToCode['m'] = 227;    ChrToCode['n'] = 222;    ChrToCode['o'] = 235;
    ChrToCode['p'] = 240;    ChrToCode['q'] = 193;    ChrToCode['r'] = 209;
    ChrToCode['s'] = 199;    ChrToCode['t'] = 215;    ChrToCode['u'] = 225;
    ChrToCode['v'] = 211;    ChrToCode['w'] = 198;    ChrToCode['x'] = 200;
    ChrToCode['y'] = 220;    ChrToCode['z'] = 195;    ChrToCode['0'] = 239;
    ChrToCode['1'] = 192;    ChrToCode['2'] = 197;    ChrToCode['3'] = 203;
    ChrToCode['4'] = 208;    ChrToCode['5'] = 214;    ChrToCode['6'] = 219;
    ChrToCode['7'] = 224;    ChrToCode['8'] = 229;    ChrToCode['9'] = 234;
    ChrToCode['`'] = 191;    ChrToCode['~'] = 191;    ChrToCode['!'] = 192;
    ChrToCode['#'] = 203;    ChrToCode['$'] = 208;
    ChrToCode['%'] = 214;    ChrToCode['&'] = 224;
    ChrToCode['*'] = 229;    ChrToCode['('] = 234;    ChrToCode[')'] = 239;
    ChrToCode['-'] = 249;    ChrToCode['='] = 245;
    ChrToCode['+'] = 245;    ChrToCode['['] = 250;    ChrToCode['{'] = 250;
    ChrToCode[']'] = 246;    ChrToCode['}'] = 246;    ChrToCode[';'] = 242;
    ChrToCode[':'] = 242;    ChrToCode['\''] = 251;   ChrToCode['"'] = 251;
    ChrToCode['\\'] = 247;   ChrToCode['|'] = 247;    ChrToCode[','] = 232;
    ChrToCode['.'] = 237;    ChrToCode['/'] = 243;    ChrToCode['?'] = 243;
    ChrToCode['<'] = 201;    ChrToCode['>'] = 201;    ChrToCode['\t'] = 190;
    ChrToCode[' '] = 212;
    for (i=0 ; i<256 ; i++) CodeToChr[ChrToCode[i]] = i;
}


GetCharFromCode(n)
  int n;
{
    return CodeToChr[n];
}


char *StringForMode(mode)
  ModeType mode;
{
    switch (mode) {
      case line:
	return "line";
      case rect:
	return "rect";
      case filledrect:
	return "filledrect";
      case arc:
	return "arc";
      case filledarc:
	return "filledarc";
      case polygon:
	return "polygon";
      case filledpolygon:
	return "filledpolygon";
    }
    return "???";
}

char *StringForFunction(func)
  int func;
{
    switch (func) {
      case GXclear:
	return "GXclear";
      case GXand:
	return "GXand";
      case GXandReverse:
	return "GXandReverse";
      case GXcopy:
	return "GXcopy";
      case GXandInverted:
	return "GXandInverted";
      case GXnoop:
	return "GXnoop";
      case GXxor:
	return "GXxor";
      case GXor:
	return "GXor";
      case GXnor:
	return "GXnor";
      case GXequiv:
	return "GXequiv";
      case GXinvert:
	return "GXinvert";
      case GXorReverse:
	return "GXorReverse";
      case GXcopyInverted:
	return "GXcopyInverted";
      case GXorInverted:
	return "GXorInverted";
      case GXnand:
	return "GXnand";
      case GXset:
	return "GXset";
    }
}
