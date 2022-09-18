/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/SetFileLevel.c	1.5.2.3"
#ident  "$Header: SetFileLevel.c 1.2 91/06/27 $"

#include	<sys/types.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<mac.h>


/*
**  Note:
**
**  o  Assume privs are already on.
**  o  If MAC is not installed then that is the same as success.
*/

#ifdef	__STDC__
int
SetFileLevel (char *pathp, char *lvlbufp)
#else
int
SetFileLevel (pathp, lvlbufp)

char *pathp;
char *lvlbufp;
#endif
{
	int	n;
		
	level_t	lid;

	while ((n=lvlin (lvlbufp, &lid)) < 0 && errno == EINTR)
		continue;
	if (n < 0)
	{
		switch (errno) {
		case	EACCES:		/*  Assume ENOPKG  */
			return	1;
		case	EINVAL:
		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	0;
		}
	}
	while ((n=lvlfile (pathp, MAC_SET, &lid)) < 0 && errno == EINTR)
		continue;
	if (n < 0)
	{
		switch (errno) {
		case	ENOSYS:
			return	1;
		default:
			/*
			**  Leave 'errno' set as is.
			*/
			return	0;
		}
	}

	return	1;
}
