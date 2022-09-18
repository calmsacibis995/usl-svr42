/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/WrapLevel.c	1.3.1.3"
#ident	"$Header: $"

#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>
/*
*/

extern	int	errno;

#ifdef	__STDC__
char *
WrapLevel (char *lvlbufp, int wraplen)
#else
char *
WrapLevel (lvlbufp, wraplen)

char	*lvlbufp;
int	wraplen;
#endif
{
	register
	char	*cp, *lp, *wp;
	char	*wrapbufp,
		*prevCommap = (char *)0;
	int	lvllen,
		count = 0;

	/*
	**  Common errors
	*/
	if (!lvlbufp)
	{
		errno = EINVAL;
		return	(char *)0;
	}
	/*
	**  Do we need to truncate at all?
	*/
	if ((lvllen = strlen (lvlbufp)) <= wraplen)
	{
		return	lvlbufp;
	}
	/*
	**  Do we have just an aliased level?
	*/
	cp = lvlbufp;
	while (*cp && *cp != ':')
		cp++;

	if (!*cp)
	{
		errno = EINVAL;
		return	(char *)0;
	}
	/*
	**  How much space do we need?
	*/
	wrapbufp = (char *) calloc (1, lvllen+(lvllen / wraplen)+1);
	if (!wrapbufp)
	{
		return	(char *)0;
	}
	count = 0;
	lp = lvlbufp;
	wp = wrapbufp;
	while (*lp)
	{
		*wp = *lp;
		count++;
		if (count == wraplen)
		{
			if (*lp == ',')
			{
				*++wp = '\n';
				prevCommap = (char *)0;
				count = 0;
			}
			else
			{
				if (prevCommap)
				{
					register char	c1, c2;

					cp = prevCommap+1;
					c1 = *cp;
					*cp = '\n';
					for (count=0, cp++, wp++; cp <= wp;
					     cp++, count++)
					{
						c2 = *cp;
						*cp = c1;
						c1 = c2;
					}
					prevCommap = (char *)0;
				}
				else
				{
					free (wrapbufp);
					errno = EINVAL;
					return	(char *)0;
				}
			}
		}
		else
		if (*lp == ',')
			prevCommap = wp;
		lp++;
		wp++;
	}

	return	wrapbufp;
}
