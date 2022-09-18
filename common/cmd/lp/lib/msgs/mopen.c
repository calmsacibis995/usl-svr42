/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/msgs/mopen.c	1.6.1.3"
#ident	"$Header: $"
/* LINTLIBRARY */

# include	<errno.h>

# include	"lp.h"
# include	"msgs.h"


MESG	*lp_Md = 0;

/*
** mopen() - OPEN A MESSAGE PATH
*/

int
#ifdef	__STDC__
mopen (void)
#else
mopen ()
#endif
{
    if (lp_Md)
    {
	errno = EEXIST;
	return (-1);
    }

    if ((lp_Md = mconnect(Lp_FIFO, 0, 0)) == NULL)
	return(-1);

    return(0);
}
