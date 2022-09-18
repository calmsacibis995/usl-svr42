/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)rmount:getrmntent.c	1.1.1.2"
#ident  "$Header: getrmntent.c 1.1 91/06/28 $"

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"rmnttab.h"
#include	<string.h>

static char		line[RMNT_LINE_MAX];
static const char	sepstr[]= " \t\n";
static const char	dash[]	= "-";
static int		getline();

#define	GETTOK(xx, ll)\
	if ((rmp->xx = strtok(ll, sepstr)) == NULL)\
		return	RMNT_TOOFEW;\
	if (strcmp(rmp->xx, dash) == 0)\
		rmp->xx = NULL

int
getrmntent(fp, rmp)
	register FILE		*fp;
	register struct rmnttab	*rmp;
{
	register int		ret;

	/* skip leading spaces and comments */
	if ((ret = getline(line, fp)) != 0)
		return	ret;

	/* split up each field */
	GETTOK(rmnt_special, line);
	GETTOK(rmnt_mountp,  NULL);
	GETTOK(rmnt_fstype,  NULL);
	GETTOK(rmnt_mntopts, NULL);
	GETTOK(rmnt_time,    NULL);
	GETTOK(rmnt_lvl,     NULL);

	/* check for too many fields */
	if (strtok(NULL, sepstr) != NULL)
		return	RMNT_TOOMANY;

	return	0;
}

static int
getline(lp, fp)
	register char	*lp;
	register FILE	*fp;
{
	register char	*cp;

	while ((lp = fgets(lp, RMNT_LINE_MAX, fp)) != NULL) {
		if ( strlen(lp) == RMNT_LINE_MAX-1
		&&   lp[RMNT_LINE_MAX-2] != '\n')
			return	RMNT_TOOLONG;

		/* skip leading blanks*/
		for (cp = lp; *cp == ' ' || *cp == '\t'; cp++)
			;

		/* check for comment line and empty line */
		if (*cp == '#' || *cp == '\n')
			continue;
		else
			return 0;
	}
	return	-1;
}
