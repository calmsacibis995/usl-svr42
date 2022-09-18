/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libiaf:common/lib/libiaf/idmap/namemap.c	1.5.1.2"
#ident  "$Header: namemap.c 1.2 91/06/25 $"


#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "idmap.h"

#include "breakname.c"
#include "gmatch.c"

#define	FOUND_INVALID	-1
#define	NOT_FOUND	0
#define	FOUND_VALID	1

static	int	user_map();
static	int	system_map();

/*
 * namemap() takes as input a scheme and a g_name, looks up the
 * remote name in the appropriate database, and if the right
 * entry is found, copies the mapped string into logname.
 * logname should have enough space to store a login name
 * terminated by a NULL character.
 *
 */

int
namemap(scheme, g_name, logname)
char	*scheme;
char	*g_name;
char	*logname;
{
	int	user = NOT_FOUND,
		sys = NOT_FOUND;	/* XXX_map() return code */

	/* check user ID map file */
	user = user_map(scheme, g_name, logname);

	/* if invalid, quit early */
	if (user == FOUND_INVALID)
		return(-1);

	/* if not found, check the system map file */
	if (user == NOT_FOUND)
		sys = system_map(scheme, g_name, logname);

	/* verify that the mapped user exists */
	if (((user == FOUND_VALID) || (sys == FOUND_VALID)) &&
	    (getpwnam(logname) != (struct passwd *) NULL))
		return(0);
	else /* no mapping was found */
		return(-1);
}


static int
user_map(s, r, l)
char	*s;	/* scheme name */
char	*r;	/* remote name */
char	*l;	/* mapped logname */
{
	char	mapfile[MAXFILE];	/* map file name */
	char	descr[MAXFILE];		/* remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	FILE	*fp;			/* map file stream pointer */
	int	found = 0;		/* found flag */
	int	eof = 0;		/* eof flag */
	int	sr;			/* scanf return code */
	struct stat	statbuf;	/* file stat structure buffer */

	sprintf(mapfile, "%s/%s/%s", MAPDIR, s, UIDATA);

	/* is user mapping on? */
	if (stat(mapfile, &statbuf) < 0)
		return(NOT_FOUND);
	if ((statbuf.st_mode & UIDATA_MODE) != UIDATA_MODE)
		return(NOT_FOUND);

	/* open uidata file */
	if ((fp = fopen(mapfile, "r")) == (FILE *) NULL)
		return(NOT_FOUND);

	/* get descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) fclose(fp);
		return(NOT_FOUND);
	}

	/* look for the mapping */
	while (!found && !eof) {
		sr = fscanf(fp, "%s %s\n", remote, local);
		switch(sr) {
		case EOF:
			eof++;
			break;
		case 2:
			if (strcmp(r, remote) == 0)
				found++;
			break;
		default:
			break;
		}
	}
	(void) fclose(fp);

	if (found) {
		(void) strcpy(l, local);
		return(FOUND_VALID);
	} else
		return(NOT_FOUND);
}


static int
system_map(s, r, l)
char	*s;	/* scheme name */
char	*r;	/* remote name */
char	*l;	/* mapped logname */
{
	char	mapfile[MAXFILE];	/* map file name */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	char	r_copy[MAXLINE];	/* copy of remote name */
	char	f_copy[MAXLINE];	/* copy of name from file */
	FILE	*fp;			/* map file stream pointer */
	int	found = 0;		/* found flag */
	int	eof = 0;		/* eof flag */
	int	sr;			/* scanf return code */
	int	fieldnum;		/* name field number */
	FIELD	namefields[MAXFIELDS];	/* fields for r */
	FIELD	filefields[MAXFIELDS];	/* fields for each line from file */

	/* open idata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, s, IDATA);
	if ((fp = fopen(mapfile, "r")) == NULL)
		return(NOT_FOUND);

	/* get descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) fclose(fp);
		return(NOT_FOUND);
	}

	strncpy(r_copy, r, MAXLINE);
	if (breakname(r_copy, descr, namefields) < 0)
		return(NOT_FOUND);

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
			return(FOUND_INVALID);
		if (*local == '%') {
			fieldnum = (char) *(local+1) - '0';
			(void) strcpy(l, namefields[fieldnum].value);
		} else
			(void) strcpy(l, local);
		return(FOUND_VALID);
	} else
		return(NOT_FOUND);
}
