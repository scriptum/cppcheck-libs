/* $Header: x11term.c,v 1.1 87/09/11 08:21:10 toddb Exp $ */
/* GNUPLOT X window driver for X11  ---phr@athena.mit.edu, 2 August 1987
   Has window loc and size hard coded.  To do: make
   it placed and sized with rubber banding, like an emacs window.
   For now, the user can place the window anywhere using uwm, but can't
   change its size because of the size hint it sets up. */

/* turn off debugging printfs */
#define dbprintf ;

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <fcntl.h>
#include <signal.h>

#include "obstack.h"		/* will be <obstack.h> someday */
#define obstack_chunk_alloc	malloc
#define obstack_chunk_free	free
extern char *malloc();		/* should be void *, but unix cc loses */
extern void free();

#include "plot.h"

/* this stuff will go away soon... */
#define XW_XMAX 600
#define XW_YMAX 400

#define XW_XLAST (XW_XMAX - 1)
#define XW_YLAST (XW_YMAX - 1)

#define XW_VCHAR (XW_YMAX/30)
#define XW_HCHAR (XW_XMAX/50)	/* just a guess--no way to know this! */
#define XW_VTIC (XW_YMAX/80)
#define XW_HTIC (XW_YMAX/80)

#define XMARGIN (XW_HCHAR*6)	/* no. of pixels reserved for X tick labels */
#define YMARGIN (XW_VCHAR*2)	/* similar, for Y axis */

/*
 * The X driver maintains a display list for refreshing that works as
 * follows.  ALL display events that have happened in the window are
 * recorded in an "obstack" (see the GNU document `mem.texinfo')
 * that lives inside xw_info.  Everything in the obstack from its
 * beginning to xw_info.last_display_item is expected to actually
 * be present on the screen.  Everything past last_display_item
 * is being buffered until the next xw_update.
 *
 * The reason for this additional layer of buffering is that the X
 * library calls are not known to be reentrant.  Therefore, because
 * expose events must cause a SIGIO interrupt handler to redraw the
 * screen and these interrupts can arrive in the middle of X library
 * calls, the interrupts must be disabled while the calls are taking
 * place.  In order to not take a bad performance hit from calling
 * sigmask on every library request, the requests are batched.
 * xw_update then flushes the batched requests and moves
 * xw_info.last_display_item past them.  It passes the requests off
 * to xw_display, which disables SIGIO and then processes each
 * item in a specified part of the display list.  xw_display is
 * also called by the interrupt handler to redraw the entire window.
 * No attempt is made to process any exposure events other than EXPOSE.
 *
 * xw_info also contains some other random stuff pertaining to the
 * display and window, and the "pen location" of the plotter that
 * the driver calls simulate. 
 */
static struct {
  int	 initialized;		/* nonzero if xw_init has been called */

  struct obstack display_list;	/* the display list */

  /* Offset in display list of the slot after the last displayed item.
     (There can be more stuff after it, but it won't have been
     displayed yet).  This is an int because the base of the display
     list might be moved around by the obstack macros at any time. */
  int current_buffer_offset;	

  Display	*display;	/* display root window is on */
  Window	window;		/* newly created window */
  XWindowAttributes window_info; /* location and size of window, etc. */
  GC		gc;		/* graphics context */
  XFontStruct 	*font;		/* font metric information */
  int 		locx, locy;	/* current pen location */
  int		socket_flags;	/* nonzero=> server input will cause
				   a SIGIO interrupt */
} xw_info;

/* convenient abbreviations for constant values */
#define BLACKPIXEL BlackPixel(xw_info.display, DefaultScreen (xw_info.display))
#define WHITEPIXEL WhitePixel(xw_info.display, DefaultScreen (xw_info.display))

/* These are the types of objects that can be recorded in the display
   list.  More might be added sometime. */
enum display_item_type {
  xw_display_vector, xw_display_string, xw_display_linetype,
};
  
/* This is to tell xw_display whether to clear or not clear the screen
   before processing the display list */
enum clear_p {
  NOCLEAR, CLEAR,
};

/* ANSI C, and many compilers, won't allow you to have a zero-length
   array even when you know the actual storage for it is going to be
   malloc()'d.  So sometimes we have to declare a nonzero size for
   such things and correct for it when deciding how much memory to
   malloc().  With a good compiler, define DUMMY_ARRAY_LOSSAGE to be 0.
   Otherwise, it has to be set so that `sizeof title' is correct
   taking into account any padding that the compiler inserts for
   alignment purposes. */

#define DUMMY_ARRAY_LOSSAGE 4

union display_item {
  struct {
    enum display_item_type type;
    int x, y, length;
    char contents[DUMMY_ARRAY_LOSSAGE];
  } title;
  struct {
    enum display_item_type type;
    int xfrom, yfrom, xto, yto;
  } vector;
  struct {
    enum display_item_type type;
    int linetype;
  } linetype;
};

/* application visible driver routines start here */

/* Open an X window, load a font, set up the obstack for the display
   list, and fill in the term_tbl slots from the sizes in the X window
   entry for the window that has just been made. */
int
xw_init ()
{
  /* undocumented flag _Xdebug causes every Xlib request to synchronously
     cause a transaction with the server instead of buffering them
     if nonzero. This makes debugging easier. */
  extern int _Xdebug;

  if (xw_info.initialized > 0)
    return 0;

  /* open display specified by DISPLAY environment variable by passing
     a null arg to XOpenDisplay.  This is different from X10, where
     passing the null arg simply means open the root window of the
     current display. */
  xw_info.display = XOpenDisplay (NULL);
  dbprintf ("display ptr returned = %x\n", xw_info.display);

  if (xw_info.display == NULL) {
    fprintf (stderr, "OpenDisplay fails\n");
    return -1;
  }

  /* set up the window, and map it in. */
  xw_setup_window ();

  /* set up the graphics context, which is used in X11 to remember
     things like the current font, line drawing style, etc. */
  xw_setup_graphics_context ();
  dbprintf ("graphics context = %x\n", xw_info.gc);

  /* Read the font metric information from the font specified in the
     graphics context.  This currently is the implementation dependent
     default, which I expect to be the standard fixed width font.
     The documentation lies when it says you can pass a GC to
     XQueryFont!!!  It really means you can pass a GC ID, which
     you pull out of the GC struct. */

  if ((xw_info.font = XQueryFont (xw_info.display, xw_info.gc->gid))
      == NULL) {
    fprintf (stderr, "XQueryFont failed??\n");
    abort ();
  }

  /* initialize the display list obstack */
  obstack_init (&xw_info.display_list);

  /* Request for window exposure to cause an interrupt which
     in turn will cause the window to be redrawn. */
  xw_setup_intr ();

  /* fill in the term driver table with data about the window */
  xw_find_termentry ();

  /* record successful initializtion. */
  ++xw_info.initialized;

  return 0;
}

/* This routine is called when gnuplot is about to exit.  Just flush
   out the X request buffer and return. */   

xw_reset ()
{
  /* If the X system is not active, don't do anything. */
  if (xw_info.initialized == 0)
    return;

  reentrant_xflush ();
  /* program is about to exit--- don't bother doing anything else. */
}

/* Put output device into "text mode" (meaningless in the case of a
   window).  Since this also means to get ready for more user input
   we update the display though. */

xw_text ()
{
  /* If the X system is not running, don't do anything.  This test
     is necessary because the GNUPLOT terminal interrupt handler
     tries to put the "plot device" back in text mode. */
  if (xw_info.initialized == 0)
    return;

  xw_update ();
}

/* put the output device into "graphics mode".  In this case, it means
   clear the window and throw away the display list. */

xw_graphics ()
{
  struct obstack *obp = &xw_info.display_list;

  /* clobber the contents of the display list. */
  xw_info.current_buffer_offset = 0;
  obstack_free (obp, obstack_finish (obp));

  /* now redisplay, clearing screen */
  xw_display (0, 0, CLEAR);
}

/* put the "pen" at location x, y in plotter coordinates.  Note below
   how these are not the same as window coordinates. */

xw_move (x, y)
     int x, y;
{
  /* this XMARGIN stuff is a real screw and should be done differently @@ */
  x += XMARGIN;
  y += YMARGIN;

  /* it turns out that gnuplot expects 0,0 to be in the lower
     left corner, but in X it is in the upper left.  Correct for this. */
  y = xw_info.window_info.height - y;

  xw_info.locx = x;
  xw_info.locy = y;
}

/* draw a line segment from the current "pen location" to location x, y
   in plotter coordinates.  Map these to window coordinates as above. */

xw_vector (x, y)
     int x, y;
{
  union display_item frame;

  x += XMARGIN;			/* bleccch... @@ */
  y += YMARGIN;

  y = xw_info.window_info.height - y; /* see comment in xw_move */

  frame.vector.type = xw_display_vector;
  frame.vector.xfrom = xw_info.locx;
  frame.vector.yfrom = xw_info.locy;
  frame.vector.xto = x;
  frame.vector.yto = y;

  obstack_grow (&xw_info.display_list, &frame.vector, sizeof frame.vector);

  xw_info.locx = x;
  xw_info.locy = y;
}

/* This is supposed to select continuous, dashed, dotted, etc. types
   of lines.  At the moment it doesn't do anything. */

xw_linetype (linetype)
     int linetype;
{
  union display_item frame;
  frame.linetype.type = xw_display_linetype;
  frame.linetype.linetype = linetype;
  obstack_grow (&xw_info.display_list, &frame.linetype, sizeof frame.linetype);
}

/* Display the text in STR in the lower right corner of the screen, ROW
   rows above the bottom edge. */

xw_lrput_text (row, str)
     int row;
     char *str;
{
  XWindowAttributes *wi = &xw_info.window_info;
  XFontStruct *fi = xw_info.font;
  int tx, ty;

  int offset_x = XTextWidth (fi, "xxxxxx", 6); /* nice amount? */

  tx = XW_XMAX - XTextWidth (fi, str, strlen(str)) - offset_x;
  ty = XW_YMAX - (row + 2) * (fi->ascent + fi->descent);

  tx += XMARGIN;
  ty -= YMARGIN;		/* subtract instead of add, because
				   these are window coordinates */

  record_string (str, tx, ty);
}


/* Display the text in STR in the upper left corner of the screen, ROW
   rows below the top edge. */

xw_ulput_text (row, str)
     int row;
     char *str;
{
  XFontStruct *fi = xw_info.font;
  int tx, ty;

  int offset_x =  XTextWidth (fi, "xxxxxx", 6);	/* nice amount? */

  tx = offset_x;
  ty = (2 * (row + 4) * (fi->ascent + fi->descent));

  tx += XMARGIN;
  ty -= YMARGIN;		/* see lrput comment above */

  record_string (str, tx, ty);
}

/* Generate tick mark label STR on axis AXIS.  AXIS must be either 'x'
   or 'y' and specifies which axis to draw the label on.  Location X, Y
   is the point where the tick mark to be labelled touches the axis. */

xw_tickmark_text (str, axis, x, y)
     char *str, axis;
     int x, y;
{
  int tx, ty;
  XFontStruct *fi = xw_info.font;
  int offset_y = (XTextWidth (fi, "x", 1) + 1) / 2;

  x += XMARGIN;
  y += YMARGIN;
  y = xw_info.window_info.height - y; /* see comment in xw_move */

  switch (axis) {
  case 'x':
    /* for X axis, the label is centered and placed a little bit below
       that point. */
    tx = x - XTextWidth (fi, str, strlen(str)) / 2;
    ty = y + (3 * fi->ascent) / 2; /* maybe tweak this ratio */
    break;

  case 'y':
    /* For Y axis, label is placed to the left of the Y axis point,
       centered vertically around the point. */
    tx = x - XTextWidth (fi, str, strlen(str)) - offset_y;
    ty = y + (fi->ascent + 1) / 2;
    break;

  default:
    abort ();			/* illegal */
  }

  record_string (str, tx, ty);
}


/* From here on down is utility routines used by the driver calls.
   Only the driver calls above here should be visible to the rest
   of the program. */

/* set up term_tbl slots from X entry, by clumsily searching the
   table for the X window entry.  Something smarter should be done. */
static
xw_find_termentry ()
{
  int i;
  extern struct termentry term_tbl[];
  XWindowAttributes *wi = &xw_info.window_info;
  XFontStruct *fi = xw_info.font;
  
  for (i = 0; TERM_IS_GOOD(i); i++) {
    if (strcmp (term_tbl[i].name, "xwindow") == 0) {
      struct termentry *t = &term_tbl[i];
      
      t->xmax = XW_XMAX;
      t->ymax = XW_YMAX;
      t->v_char = fi->ascent;
      /* set h_char to be the same thing as v_char, since there is
	 no "logical character width" defined in X11.  It doesn't
	 really matter what this is set to, since nothing ever
	 uses it.  I wonder why the term structure has it in the
	 first place. */
      t->h_char = fi->ascent;
      /* make tick marks also the same length as a character height */
      t->v_tic = fi->ascent;
      t->h_tic = fi->ascent;
      break;
    }
  }
  if (!TERM_IS_GOOD (i)) {
    printf ("couldn't find xwindow entry--- how did you set it?\n");
    abort ();
  }
}

/* Set up the window, but don't map it.  Gnuplot is not yet smart enough
   to draw in an arbitrary sized window, so this routine sets a size
   hint for a minimum window that is pretty big.  Do something better
   later.  Also, it tries to place the window at a fixed location.
   The users' window manager can override this if desired. */

static
xw_setup_window ()
{
  extern int gnuplot_argc;
  extern char **gnuplot_argv;
  
  XSizeHints hints;
  int display_width, display_height;

  hints.width = hints.min_width = XW_XMAX + (3 * XMARGIN) / 2;
  hints.height = hints.min_height = XW_YMAX + (3 * YMARGIN) / 2;
  hints.flags = PSize | PMinSize;

  display_width = DisplayWidth (xw_info.display,
				DefaultScreen(xw_info.display));
  display_height = DisplayHeight (xw_info.display,
				  DefaultScreen(xw_info.display));

  xw_info.window = XCreateSimpleWindow (xw_info.display,
					DefaultRootWindow (xw_info.display),
					display_width / 6, display_height / 6,
					hints.min_width,
					hints.min_height,
					2, BLACKPIXEL, WHITEPIXEL);
  dbprintf ("new_window = %x\n", xw_info.window);

  XSetStandardProperties (xw_info.display, xw_info.window,
			  "GNUplot", "GNUplot", None,
			  gnuplot_argv, gnuplot_argc,
			  &hints);

  /* read in size and location of window. */
  if (XGetWindowAttributes (xw_info.display, xw_info.window,
			    &xw_info.window_info) == 0) {
    fprintf (stderr, "XGetWindowAttributes fails---this can't happen\n");
    abort ();
  }

  /* Ask for an X input event to be generated on expose events for
     this window. This means that whenever an expose event happens,
     data is created and placed on the display->fd socket. */
  XSelectInput (xw_info.display, xw_info.window, ExposureMask);

  /* map the window, meaning cause it to actually be displayed on the
     screen.  The map operation returns asynchronously, and queues an
     exposure event when the mapping has actually taken place (this
     could require user input in the case of a rubber band window).
     Stuff drawn into the window before the map operation finishes
     will not be displayed correctly.  Therefore, wait for the
     exposure event to be received before returning. */
  {
    XEvent junk;
    XMapWindow (xw_info.display, xw_info.window);
    XNextEvent (xw_info.display, &junk); /* blocks until event arrives */
  }
  dbprintf ("mapped window.\n");
}

/* Set up the graphics context, used to remember things like the
   current font, etc.  As currently written this routine requests
   the default values to be used for nearly all items in the GC. */
xw_setup_graphics_context ()
{
  XGCValues context;

  bzero (&context, sizeof context);
  xw_info.gc = XCreateGC (xw_info.display, xw_info.window, 0, &context);
}

/* Enter request into display list to write string STR at location X,Y */

static
record_string (str, x, y)
     char *str;
     int x, y;
{
  union display_item frame;

  frame.title.type = xw_display_string;
  frame.title.x = x;
  frame.title.y = y;

  /* this length includes the null at the end, which is pushed onto
     the display list, below. */
  frame.title.length = strlen (str);
  obstack_grow (&xw_info.display_list, &frame,
		sizeof frame.title - DUMMY_ARRAY_LOSSAGE);
  /* obstack_grow0 puts a null on the end; not really needed, but
     makes life easier in debugging. */
  obstack_grow0 (&xw_info.display_list, str, frame.title.length);
}

/* Push the pending requests on the display list out to the X server,
   and move the pointer to the pending area past them. */
static 
xw_update ()
{
  int new_offset =
    obstack_next_free (&xw_info.display_list) -
      obstack_base (&xw_info.display_list);
  xw_display (xw_info.current_buffer_offset, new_offset, NOCLEAR);
  xw_info.current_buffer_offset = new_offset;
}

/*
 * The next routine sets up the interrupt handler for asynchronous
 *  refresh on expose events.  Method:
 *    1. Remember previous status flags for socket, as these flags
 *       will be diddled to request and unrequest SIGIO interrupts later.
 *    2. Use signal() to catch SIGIO interrupts with the interrupt
 *	 handler.  We must do this before calling enable_sigio,
 *       otherwise some signals could happen with nobody to catch
 *	 them, causing a core dump.
 *    3. Use fcntl() to request that the current process be the one
 *       to which SIGIO interrupts are sent when I/O generates them.
 *       Otherwise, they are sent to process 0 for some reason.
 *    4. Call enable_sigio(), which uses fcntl() again to request a SIGIO
 *       when data appears on the X server socket.  Supposedly this
 *       will work in 4.3 and in Ultrix 2.0, even though certain BSD
 *       doc says that this call only works when the channel passed it
 *       is a tty channel.  (Other docs say it works on ttys and
 *       sockets).  So it is possible that this will lose on some old
 *       4.2bsd systems.  I am not sure what to do then.
 */
static 
xw_setup_intr ()
{
  extern int xw_handle_intr ();

  int socket = xw_info.display->fd; /* channel number of the tcp
				       socket to the server */
  
  xw_info.socket_flags = fcntl (socket, F_GETFL, 0);

  signal (SIGIO, xw_handle_intr);
  fcntl (socket, F_SETOWN, getpid());
  enable_sigio ();
}

/* Request a SIGIO interrupt to be sent whenever input is available
   on the socket connected to the X server.   Return old state. */
static int
enable_sigio ()
{
  return set_sigio_state (xw_info.socket_flags | FASYNC);
}
 
/* Cancel request for SIGIO interrupt on input from the server.
   The interrupt should be turned off whenever "normal" traffic
   (i.e., plotting and anything else that does not have to
   asynchronously notify Gnuplot that something is happening)
   is taking place.  Return old state. */
static int
disable_sigio ()
{
  return set_sigio_state (xw_info.socket_flags & ~FASYNC);
}

/* Set state of sigio to VALUE, if it was something different.
   Return old value so it can be restored. */
static int
set_sigio_state (value)
     int value;
{
  int socket = xw_info.display->fd; /* channel number of the tcp
				     socket to the server */
  int oldval = xw_info.socket_flags;

  /* Only call fcntl if we're asking for a real state change (old
     state different from new state). */
  if (xw_info.socket_flags != value) {
    fcntl (socket, F_SETFL, value);
    xw_info.socket_flags = value;
  }
  return oldval;
}

/* actually handle the interrupt by simply repeating everything on
   the display list up to the last xw_flush(). */
static 
xw_handle_intr()
{
  XEvent event;
  int queuesize, expose_received = 0;
  int old_intr;

  /* first, we need to get rid of the X events on the queue.  However,
     since reading the events in order to throw them away involves
     Xlib calls, we need to disable SIGIO while it is happening. */

  old_intr = disable_sigio ();

  /* Check the X queue for actual exposure events.  This is necessary
     for two reasons:
     1. SIGIO interrupts seem to get sent here even though the SIGIO
        request is carefully turned off during any activity that can
        cause traffic on the socket (a kernel bug?).
     2. The Xlib documentation lies when it says XSelectInput
        masks events other than the ones selected.  I.e., sometimes
        events can arrive that were not asked for.  Therefore,
	we must make sure the purported exposure event really is one. */

  while ((queuesize = XPending (xw_info.display)) > 0) {
    XNextEvent (xw_info.display, &event);
    if (event.type == Expose)
      expose_received++;
  }

  /* redraw the screen, but only if the interrupt was not spurious. */
  if (expose_received > 0)
    xw_display (0, xw_info.current_buffer_offset, NOCLEAR);
  
  set_sigio_state (old_intr);
}

/* Cause an XFlush() call, with interrupts disabled.
   Do this by asking for a display update, displaying nothing. */
static 
reentrant_xflush ()
{
  xw_display (0, 0, NOCLEAR);
}

/* Send the contents of the display list to the X server, starting
   at offset FROM, ending before TO.  SIGIO interrupts are disabled while
   this is happening, because the X library calls are not reentrant,
   so getting an expose event while this was happening could lose badly
   if interrupts were enabled.

   If CLEAR_WINDOW is CLEAR, clear the window before starting to draw. */

static 
xw_display (from, to, clear_window)
     int from, to;
     enum clear_p clear_window;
{
  int display_offset;
  int old_sigio = disable_sigio ();
  
  if (clear_window == CLEAR)
    XClearWindow (xw_info.display, xw_info.window);

  /* for each item in the display list, make the appropriate Xlib
     call for that item, then skip over it by calculating its size
     and adding that much to display_offset */

  for (display_offset = from; display_offset < to; ) {
    union display_item *dp =
      (union display_item *)
	((char *) obstack_base (&xw_info.display_list) + display_offset);

    switch (dp->vector.type) {
    case xw_display_vector:
      XDrawLine (xw_info.display, xw_info.window, xw_info.gc,
		 dp->vector.xfrom, dp->vector.yfrom,
		 dp->vector.xto, dp->vector.yto);
      /* skip to the next item in the display list */
      display_offset += sizeof dp->vector;
      break;

    case xw_display_string:
      XDrawString (xw_info.display, xw_info.window, xw_info.gc,
		   dp->title.x, dp->title.y,
		   dp->title.contents, dp->title.length);
      /* skip to the next item in the display list */
      display_offset +=
	(sizeof dp->title) + dp->title.length + 1 - DUMMY_ARRAY_LOSSAGE;
      break;
      
    case xw_display_linetype:
      hack_linetype (dp->linetype.linetype);
      /* skip to the next item in the display list */
      display_offset += sizeof dp->linetype;
      break;

    default:
      fprintf (stderr, "illegal type object on display list\n");
      abort ();
    }
  }
  XFlush (xw_info.display);	/* cause stuff to actually appear
				   on screen */
  set_sigio_state (old_sigio);
}

/* set the dashed line pattern to LINETYPE. */
static
hack_linetype (linetype)
     int linetype;
{
  char *pat;
  int len;

/* this routine is commented out because it crashes the server and
   there is no time to fix it right now.  8/21 4pm */

#ifdef SERVER_FIXED
  /* the dash patterns are lifted from the PostScript driver. */
    switch ((linetype+2)%7) {
    case 0 :			/* solid. */
    case 2 :
      pat = "";
      break ;
    case 1 :			/* longdashed. */
    case 6 :
      pat = "\011\003";
      break;
    case 3 :			/* dotted. */
      pat = "\003\003";
	break ;
    case 4 :			/* shortdashed. */
      pat = "\006\003";
      break ;
    case 5 :			/* dotdashed. */
      pat = "\003\003\006\003";
      break;
    }
  len = strlen (pat);
  if (len == 0)
    XSetLineAttributes (xw_info.display, xw_info.gc, 1, LineSolid,
			CapButt, JoinBevel);
  else {
    XSetDashes (xw_info.display, xw_info.gc, 0, pat, strlen (pat));
    XSetLineAttributes (xw_info.display, xw_info.gc, 1, LineDoubleDash,
			CapButt, JoinBevel);
  }
#endif
}     

/* dump out the contents of the display list, for debugging.
   Same args as xw_display. */
static
xw_dump (from, to, clear_window)
     int from, to;
     enum clear_p clear_window;

{
  int display_offset;

  printf ("xw_dump (from=%d, to=%d, clear_window=%d)\n",
	  from, to, clear_window);

  /* for each item in the display list, print out the info from
     that item, then skip over it by calculating its size
     and adding that much to display_offset */

  for (display_offset = from; display_offset < to; ) {
    union display_item *dp =
      (union display_item *)
	((char *) obstack_base (&xw_info.display_list) + display_offset);

    switch (dp->vector.type) {
    case xw_display_vector:
      printf ("%x (offset %d): VECTOR xfrom=%d yfrom=%d xto=%d yto=%d\n",
	      dp, display_offset,
	      dp->vector.xfrom, dp->vector.yfrom,
	      dp->vector.xto, dp->vector.yto);
      /* skip to the next item in the display list */
      display_offset += sizeof dp->vector;
      break;

    case xw_display_string:
      printf ("%x (offset %d): STRING x=%d y=%d length=%d contents=%x\n",
	      dp, display_offset,
	      dp->title.x, dp->title.y,
	      dp->title.length, dp->title.contents);
      /* skip to the next item in the display list */
      display_offset +=
	(sizeof dp->title) + dp->title.length + 1 - DUMMY_ARRAY_LOSSAGE;
      break;

    case xw_display_linetype:
      printf ("%x (offset %d): LINETYPE %d\n",
	      dp, display_offset, dp->linetype);
      /* skip to the next item in the display list */
      display_offset += sizeof dp->linetype;
      break;

    default:
      fprintf (stderr, "illegal type object on display list\n");
      abort ();
      break;
    }
  }
}  
