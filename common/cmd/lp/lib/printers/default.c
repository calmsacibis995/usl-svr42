/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/printers/default.c	1.9.2.4"
#ident	"$Header: $"

#include "stdio.h"
#include "stdlib.h"

#include "lp.h"
#include "printers.h"

/** ------------------PRINTER DEFAULT -------------------------------
 ** getdefault() - READ THE NAME OF THE DEFAULT DESTINATION FROM DISK
 **/

char *
#if	defined(__STDC__)
getdefault (
	void
)
#else
getdefault ()
#endif
{
	return (loadline(Lp_Default));
}

/**
 ** putdefault() - WRITE THE NAME OF THE DEFAULT DESTINATION TO DISK
 **/

int
#if	defined(__STDC__)
putdefault (
	char *			dflt
)
#else
putdefault (dflt)
	char			*dflt;
#endif
{
	register FILE	*fp;
	level_t		lid;
	int		n;

	if (!dflt || !*dflt)
		return	deldefault ();

	if (!(fp = open_lpfile(Lp_Default, "w", MODE_READ)))
		return	-1;

	(void)fprintf (fp, "%s\n", dflt);

	close_lpfile (fp);
	lid = PR_SYS_PUBLIC;
	while ((n=lvlfile (Lp_Default, MAC_SET, &lid)) < 0 && errno == EINTR)
		continue;

	if (n < 0 && errno != ENOSYS)
		return	-1;

	return	0;
}

/**
 ** deldefault() - REMOVE THE NAME OF THE DEFAULT DESTINATION
 **/

int
#if	defined(__STDC__)
deldefault (
	void
)
#else
deldefault ()
#endif
{
	return (rmfile(Lp_Default));
}

/** ------------------COPY DEFAULT -------------------------------
 ** getcpdefault() - READ THE NAME OF THE DEFAULT COPY MODE FROM DISK
 **/

char *
#if	defined(__STDC__)
getcpdefault (
	void
)
#else
getcpdefault ()
#endif
{
	return (loadline(Lp_cpDefault));
}

/**
 ** putcpdefault() - WRITE THE NAME OF THE DEFAULT COPY MODE TO DISK
 **/

int
#if	defined(__STDC__)
putcpdefault (
	char *			dflt
)
#else
putcpdefault (dflt)
	char			*dflt;
#endif
{
	register FILE	*fp;
	level_t		lid;
	int		n;

	if (!(fp = open_lpfile(Lp_cpDefault, "w", MODE_READ)))
		return (-1);

	(void)fprintf (fp, "copy-files: %s\n", dflt);

	close_lpfile (fp);
	lid = PR_SYS_PUBLIC;
	while ((n=lvlfile (Lp_cpDefault, MAC_SET, &lid)) < 0 && errno == EINTR)
		continue;

	if (n < 0 && errno != ENOSYS)
		return	-1;

	return (0);
}
