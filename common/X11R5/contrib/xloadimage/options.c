/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:options.c	1.1"
/* options.c:
 *
 * finds which option in an array an argument matches
 *
 * jim frost 10.03.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "options.h"

int optionNumber(arg, options)
     char *arg;
     char *options[];
{ int a, b;

  if ((*arg) != '-')
    return(OPT_NOTOPT);
  for (a= 0; options[a]; a++) {
    if (!strncmp(arg + 1, options[a], strlen(arg) - 1)) {
      for (b= a + 1; options[b]; b++)
	if (!strncmp(arg + 1, options[b], strlen(arg) - 1))
	  return(OPT_SHORTOPT);
      return(a);
    }
  }
  return(OPT_BADOPT);
}
