/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4rgb:rgb.c	1.1"
#include <sys/types.h>
#include <stdio.h>
#include <site.h>
#include <fcntl.h>
#include <ctype.h>

#define NSIZE 42

struct RGB {
  char name [NSIZE];
  unsigned short red, green, blue;
} RGB;

/*
 * Compare strings (at most n bytes) Ignores case
 *    	returns: s1>s2; >0  s1==s2; 0  s1<s2; <0
 */

int
stringcmp(s1, s2, n)
register char *s1, *s2;
register n;
{
  if(s1 == s2) {
    return(0);
  }
  while(--n >= 0 && toupper(*s1) == toupper(*s2++)) {
    if(*s1++ == '\0') {
      return(0);
    }
  }
  return((n < 0)? 0: (toupper(*s1) - toupper(*--s2)));
}

main(argc, argv)
int argc;
char **argv;
{
  struct RGB rgb;
  char *dbname;
  char line[512];
  int out;		/* Output file descriptor */
  int lineno;		/* Current line number */
  int items;		/* Number of items read */
  int red, green, blue;	/* Input rgb values */
  char name[512];	/* Input color name */
  char lastname[NSIZE];	/* Last color name used in verifying */
			/* descending order */

  if (argc == 2) {
    dbname = argv[1];
  }
  else {
    dbname = RGB_DB;
  }
  strcpy (name, dbname);
  if ((out = open (name, O_WRONLY|O_CREAT, 0644)) == -1) {
    fprintf (stderr, "can't open file %s for output\n", name);
    fflush (stderr);
    exit (1);
  }
  lineno = 0;
  lastname[0] = '\0';
  while (fgets (line, sizeof (line), stdin)) {
    lineno += 1;
    items = sscanf (line, "%d %d %d %[^\n]\n", &red, &green, &blue, name);
    if (items != 4) {
      fprintf (stderr, "syntax error on line %d\n", lineno);
      fflush (stderr);
      exit (1);
    }
    if (red < 0 || red > 0xff ||
	green < 0 || green > 0xff ||
	blue < 0 || blue > 0xff) {
      fprintf (stderr, "value for %s out of range\n", name);
      fflush (stderr);
      exit (1);
    }
    if (stringcmp (name, lastname, NSIZE) < 0) {
      fprintf (stderr, "name on line %d out of order\n", lineno);
      fflush (stderr);
      exit (1);
    }
#ifdef u3b2
    rgb.red = red;
    rgb.green = green;
    rgb.blue = blue;
#else
	/* shift the bits over one byte if compiled on a 6386*/
    rgb.red = red << 8;
    rgb.green = green << 8;
    rgb.blue = blue << 8;
#endif
    strncpy (rgb.name, name, NSIZE-1);
    rgb.name[NSIZE-1] = '\0';
    if ((items = write (out, &rgb, sizeof(struct RGB)))
		 != sizeof(struct RGB)) {
      fprintf (stderr, "store of %s failed\n", name);
      fflush (stderr);
    }
    strncpy (lastname, name, NSIZE-1);
    lastname[NSIZE-1] = '\0';
  }
}
