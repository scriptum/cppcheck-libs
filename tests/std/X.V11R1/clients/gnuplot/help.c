/* $Header: help.c,v 1.1 87/09/11 08:20:02 toddb Exp $ */
/* (c) 1987, MIT Project Athena.  Written by Paul Rubin.
    Copying permissions per GNU General Public License. */

/* Primitive help system for GNUplot.  At the moment, it can only
   read specific nodes from the GNUplot info file, which must
   have a tag table (use Info-tagify) so that it can know what
   nodes are available.  Later (e.g. when Brian Fox finishes his
   standalone Info program), something more intelligent should be
   merged in here. */

#include <stdio.h>
#include <ctype.h>
#include "plot.h"
#include "obstack.h"

#define obstack_chunk_alloc malloc
void *malloc();

/* the delimiter character used by the Info system for separating nodes. */
#define INFO_DELIM_CHAR 037

/* Contents of the info file.  The first time you type "help", the
   help_info structure is initialized. */
static struct help_info {
  BOOLEAN is_initialized;	/* nonzero means help_info is initialized */
  FILE *info_stream;		/* stream to the gnuplot.info file */
  struct tag_table {		/* pointer to a list of the tag names
				   and offsets found in the info file
				   tag table. */
    char *nodename;
    int offset;
  } *tag_table;
} help_info;

static char *info_buffer = NULL;

/* the name of the info file where the help messages live. */
char *info_file_name;

#define LINELEN 60		/* shouldn't be a magic cookie @@ */
#define SCREENHEIGHT 24		/* ditto @@ */

#ifdef STANDALONE_HELP
/* Prompt user for topic and print the help. */
static
help_interactive ()
{
  char topic[300], c;

  printf ("Topic (? for list): ");
  gets (topic);
  help_topic (topic);
}
#endif

/* Print the help node for TOPIC, or a list of available topics
   if topic requested is "?". */
help_topic (topic)
     char *topic;
{
  struct tag_table *p;

  /* fill in help_info structure, if necessary */
  if (setup_help_info () < 0) {
    printf ("No help available; sorry.  Try `set helpfile'.\n");
    return;
  }

  if (strcmp (topic, "?") == 0) {
    int len = 0;		/* no of chars on this line so far */

    printf ("Available help topics: \n");

    for (p = help_info.tag_table; p->nodename != NULL; p++) {
      if ((len += strlen(p->nodename) + 2) > LINELEN) {
	len = 0;
	printf ("\n");
      }
      printf ("%s", p->nodename);
      if ((p+1)->nodename != NULL)
	printf (", ");
    }
    printf ("\n");
    return;
  }

  for (p = help_info.tag_table; p->nodename != NULL; p++) {
    if (case_fold_prefix (topic, p->nodename)) {
      print_help_node (p);
      break;
    }
  }
  if (p->nodename == NULL)
    printf ("No help available for %s.  Type \"help ?\" for list of topics.\n",
	    topic);
}

/* print the help node pointed to by NODE */
static
print_help_node (node)
     struct tag_table *node;
{
  char c;
  int lines = 0;		/* lines printed since last prompt */

  if (fseek (help_info.info_stream, node->offset, 0) < 0) {
    printf ("Either an internal error has occured, or the GNUPLOT\n");
    printf ("help file %s has disappeared.\n", info_file_name);
    printf ("Please notify your system maintainer.\n");
    fclose (help_info.info_stream);
    help_info.is_initialized = 0;
  }

  while (1) {
    switch (c = getc(help_info.info_stream)) {
    case INFO_DELIM_CHAR:
    case EOF:
      goto done;

    case '\n':
      lines++;
      /* fall through */

    default:
      putchar (c);
      if (lines > SCREENHEIGHT) {
	int quit = 0;
	printf ("--hit return for more--");
	while ((c = getchar()) != '\n')
	  if (c == 'q')
	    quit++;
	if (quit)
	  goto done;
	lines = 0;
      }
      break;
    }
  }
 done:
  return;
}
  
/* Initialize the help_info structure.  On failure to set up
   help_info properly, print error message and return -1,
   refraining from setting is_intitialized so that the person can
   fix things and try again if appropriate.

   If help_info has already been set up, then do nothing. */

static int
setup_help_info ()
{
  if (help_info.is_initialized)
    return 0;

  printf ("reading help file... ");
  fflush (stdout);

  if ((help_info.info_stream = fopen(info_file_name, "r")) == NULL) {
    perror (info_file_name);
    goto lose;
  }
  if (find_tag_table_beginning () < 0) {
    printf ("%s: can't find tag table\n", info_file_name);
    goto lose1;
  }
  if (read_tag_table () < 0) {
    printf ("%s: error in tag table\n", info_file_name);
    goto lose1;
  }
  printf ("done.\n");
  ++help_info.is_initialized;
  return 0;

  /* jump here to close stream and return error. */

lose1:

  fclose (help_info.info_stream);

lose:  

  return -1;
}

/* Find the beginning of the tag table in the info stream, by reading
   it backwards.  The tag table is supposed to begin immediately
   after the second INFO_DELIM_CHAR character from the end. */
static
find_tag_table_beginning ()
{
  int delim = 0;		/* number of delim chars seen so far */
  int c, previous_char();

  fseek (help_info.info_stream, 0, 2); /* seek to end of file */

  do {
    c = previous_char ();

    if (c == INFO_DELIM_CHAR)
      delim++;

    else if (c == EOF)
      return -1;
  } while (delim < 2);

  getc (help_info.info_stream);	/* throw away delim char */
  if (match_ahead ("\014\nTag table:\n") < 0)
    return -1;

  return 0;
}

/* read all the node names and offsets from the tag table, and remember
   them. */
static int
read_tag_table ()
{
  char name_buf[200];		/* max possible node name length */
  int offset;
  char c;

  struct obstack names, help_items;

  /* buffer used for reading a single tag table entry. */
  struct tag_table item;

  obstack_init (&names);
  obstack_init (&help_items);

#ifdef LOSE			/* doesn't work for some reason */
  /* don't waste too much space on obstacks */
  obstack_chunk_size (&names) = 500;
  obstack_chunk_size (&help_items) = 500;
#endif

  for (;;) {
    if (match_ahead("Node: ") < 0)
      break;
    while ((c = getc (help_info.info_stream)) != 0177)
      obstack_1grow (&names, c);
    /* put null char onto end of name */
    obstack_1grow (&names, '\0');
    item.nodename = (char *) obstack_finish (&names);
    item.offset = 0;
    while ((c = getc (help_info.info_stream)) != '\n') {
      if (isdigit(c))
	item.offset = item.offset * 10 + c - '0';
      else
	abort ();		/* should report error reasonably @@ */
    }
    /* The offsets in the tag table are Emacs buffer character numbers,
       which start at 1.  Decrement item.offset to turn it into an
       absolute file offset starting at 0. */
    --item.offset;
    obstack_grow (&help_items, &item, sizeof item);
  }

  /* put a null item at the end of the table */
  bzero (&item, sizeof item);
  obstack_grow (&help_items, &item, sizeof item);
  
  help_info.tag_table =
    (struct tag_table *) obstack_finish (&help_items);

  return 0;
}

/* return char from info input stream BEFORE the current char,
   by reading backwards.  Leaves the pointer pointing at the
   char that has just been returned. */
static int
previous_char()
{
  int c;

  /* back up pointer one character */
  fseek (help_info.info_stream, -1, 1);

  c = getc (help_info.info_stream);

  /* back up over char just read */
  fseek (help_info.info_stream, -1, 1);
  return c;
}

/* read past STRING in the info input stream.  Return -1 if
   STRING doesn't match. */
static
match_ahead (string)
     char *string;
{
  while (*string)
    if (getc (help_info.info_stream) != *string++)
      return -1;
  return 0;
}

/* return nonzero if PREFIX is a prefix of STRING, ignoring case. */
#define force_lower(c) (isupper(c) ? tolower(c) : c)

static int
case_fold_prefix (prefix, string)
     char *prefix, *string;
{
  while (*prefix) {
    if (force_lower(*prefix) != force_lower(*string))
      return 0;
    prefix++, string++;
  }
  return 1;
}
  
#ifdef STANDALONE_HELP
/* main program for testing GNUPLOT help system. */
main ()
{
  do_help ();
  printf ("done\n");
}
#endif
