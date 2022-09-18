/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)idmap:breakname.c	1.2.3.2"
#ident  "$Header: breakname.c 2.0 91/07/12 $"

#include <string.h>
#include "idmap.h"

#define	EMPTYSTR	""

/*
 * breakname() - breaks a name into fields according to a field
 * descriptor.
 *
 * input:	name - unbroken name (see note below)
 *		fd - field descriptor line (like "!M1@M2\n")
 *		     NOTE: fd has to end with a new line
 * output:      fields[] - array of pointers into field info structures
 *
 * NOTE: This function modifies the argument 'name' by replacing each field
 * separator with the end-of-string character ('\0').
 *
 */

int
breakname(name, fd, fields)
char *name;		/* name string */
char *fd;		/* field descriptor line, with leading '!' */
FIELD fields[];		/* field information pointers */
{
	int	fieldno;	/* field number */
	char	delim;		/* delimiter */

	/* initialize field pointers */
	for (fieldno = 0; fieldno < MAXFIELDS; fieldno++) {
		fields[fieldno].value = EMPTYSTR;
		fields[fieldno].type = ' ';
	}

	while (*(fd + 3) != '\n') {

		fieldno = ((char) *(fd + 2)) - '0';
		delim = *(fd + 3);

		fields[fieldno].value = name;
		fields[fieldno].type = *(fd + 1);

		while ((*name != delim) && (*name != '\0'))
			name++;

		if (*name == '\0')
			/* early end of name (not all fields were present) */
			return(-1);

		*name = '\0';
		name++;
		fd += 3;
	}

	/* take care of the last field */

	fieldno = ((char) *(fd + 2)) - '0';
	fields[fieldno].value = name;
	fields[fieldno].type = *(fd + 1);

	return(0);
}
