/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/GetProcLevel.c	1.1.2.2"
#ident  "$Header: GetProcLevel.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <mac.h>


#ifdef	__STDC__
char *
GetProcLevel (int format)
#else
int
GetProcLevel (format)

int	format;
#endif
{
	int	lvlsize;
	char	*lvlbufp;
	level_t	level;

	if (lvlproc (MAC_GET, &level) < 0)
		return	NULL;

	lvlsize = lvlout (&level, NULL, 0, format);

	if (lvlsize < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			errno = ENOPKG;
			return	NULL;

		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	NULL;
		}
	}
	lvlbufp = (char *) malloc (lvlsize);

	if (!lvlbufp)
	{
		/*
		**  'errno' is likely ENOSPC.
		*/
		return	NULL;
	}
	if (lvlout (&level, lvlbufp, lvlsize, format) < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			errno = ENOPKG;
			return	NULL;

		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	NULL;
		}
	}
	return	lvlbufp;
}
