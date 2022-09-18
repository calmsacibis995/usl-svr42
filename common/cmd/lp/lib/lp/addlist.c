/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/addlist.c	1.7.7.4"
#ident  "$Header: addlist.c 1.2 91/06/27 $"

#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"

/*
**  addlist() - ADD ITEM TO (char **) LIST
**
**  Do not allow duplicate items.
*/

int
#if	defined(__STDC__)
addlist (
	char ***		plist,
	char *			item
)
#else
addlist (plist, item)
	register char		***plist;
	char			*item;
#endif
{
	register char		**pl;

	register int		n;

	if (*plist) {

		n = lenlist(*plist);

		for (pl = *plist; *pl; pl++)
			if (STREQU(*pl, item))
				break;

		if (!*pl) {

			n++;
			*plist = (char **)Realloc(
				(char *)*plist,
				(n + 1) * sizeof(char *)
			);
			if (!*plist) {
				errno = ENOMEM;
				return (-1);
			}
			(*plist)[n - 1] = Strdup(item);
			(*plist)[n] = 0;

		}

	} else {

		*plist = (char **)Malloc(2 * sizeof(char *));
		if (!*plist) {
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[0] = Strdup(item);
		(*plist)[1] = 0;

	}

	return (0);
}

/*
**  addlist2() - ADD ITEM TO (char **) LIST
**
**  Do not check for duplicate items.
*/

int
#if	defined(__STDC__)
addlist2 (char ***plist, char *item)
#else
addlist2 (plist, item)

register
char ***	plist;
char *		item;
#endif
{
	register int		n;

	if (*plist)
	{
		n = lenlist (*plist);

		n++;
		*plist = (char **)Realloc((char *)*plist,
			(n + 1) * sizeof(char *));
		if (!*plist)
		{
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[n - 1] = Strdup(item);
		(*plist)[n] = 0;
	}
	else
	{
		*plist = (char **)Malloc(2 * sizeof(char *));
		if (!*plist)
		{
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[0] = Strdup (item);
		(*plist)[1] = 0;

	}
	return	0;
}
