/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libiaf:common/lib/libiaf/idmap/attrmap.c	1.4.1.2"
#ident  "$Header: attrmap.c 1.2 91/06/25 $"


#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "idmap.h"

#include "breakname.c"
#include "gmatch.c"

#define	NOT_FOUND	-1
#define	FOUND_INVALID	NOT_FOUND
#define	FOUND		0

/*
 * attrmap() takes as input an attr_in and an attr_name , looks up the
 * remote attribute in the appropriate database, and if the right
 * entry is found, sets attr_out to point to the mapped attribute value
 *
 */

int
attrmap(attr_name, attr_in, attr_out)
char	*attr_name;
char	*attr_in;
char	*attr_out;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	descr[MAXLINE];		/* remote attribute descriptor */
	char	remote[MAXLINE];	/* remote attribute */
	char	local[MAXLINE];		/* local attribute */
	char	r_copy[MAXLINE];	/* copy of remote attribute */
	char	f_copy[MAXLINE];	/* copy of attribute from file */
	FILE	*fp;			/* map file stream pointer */
	int	found = 0;		/* found flag */
	int	eof = 0;		/* eof flag */
	int	sr;			/* scanf return code */
	int	fieldnum;		/* name field number */
	FIELD	namefields[MAXFIELDS];	/* fields for attr_in */
	FIELD	filefields[MAXFIELDS];	/* fields for each line from file */

	/* open attribute map file */
	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr_name, DOTMAP);
	if ((fp = fopen(mapfile, "r")) == NULL)
		return(-1);

	/* get descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) fclose(fp);
		return(-1);
	}

	strncpy(r_copy, attr_in, MAXLINE);
	if (breakname(r_copy, descr, namefields) < 0)
		return(-1);

	/* read file */
	while (!found && !eof) {
		sr = fscanf(fp, "%s %s\n", remote, local);
		switch(sr) {
		case EOF:
			eof++;
			break;
		case 2:
			strncpy(f_copy, remote, MAXLINE);
			(void) breakname(f_copy, descr, filefields);
			for (fieldnum = 0, found = 1;
			     (fieldnum < MAXFIELDS) && (found); fieldnum++)
				if (namefields[fieldnum].type == 'M')
					if (!gmatch(namefields[fieldnum].value,
						    filefields[fieldnum].value))
						found = 0;
			break;
		default:
			break;
		}
	}
	(void) fclose(fp);

	/* substitute for %x */
	if (found) {
		if (strcmp(local, "%i") == 0)
			return(-1);
		if (*local == '%') {
			fieldnum = (char) *(local+1) - '0';
			(void) strcpy(attr_out, namefields[fieldnum].value);
		} else
			(void) strcpy(attr_out, local);
		return(0);
	} else
		return(-1);
}
