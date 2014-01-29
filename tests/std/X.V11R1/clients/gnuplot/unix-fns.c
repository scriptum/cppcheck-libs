/* $Header: unix-fns.c,v 1.1 87/09/11 08:21:01 toddb Exp $ */
#include   <math.h>
#include   <errno.h>

/* global junk that unix gnuplot must keep around */
int	 gnuplot_argc;
char	**gnuplot_argv;

/* floating point error handler for libnm. */

extern int errno ;

double     infnan(iarg)
int        iarg ;
{
  switch(iarg) {
  case ERANGE:
    errno = ERANGE;
    return(HUGE);
  case -ERANGE:
    errno = EDOM;
    return(-HUGE);
  default:
    errno = EDOM;
    return(0);
  }
}
