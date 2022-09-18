/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/SetProcLevel.c	1.2.2.2"
#ident  "$Header: SetProcLevel.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <mac.h>


#ifdef	__STDC__
int
SetProcLevel (char *lvlbufp)
#else
int
SetProcLevel (lvlbufp)

char *lvlbufp;
#endif
{
	level_t	level;

	if (lvlin (lvlbufp, &level) < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			errno = ENOPKG;
			return	-1;

		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	-1;
		}
	}
	if (lvlproc (MAC_SET, &level) < 0)
	{
		/*
		**  Leave 'errno' set as is.
		*/
		return	-1;
	}
	return	0;
}
