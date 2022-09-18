/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/refer/shell.c	1.1"
#ident	"$Header: $"
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */


/*
 * SORTS UP.
 * IF THERE ARE NO EXCHANGES (IEX=0) ON A SWEEP
 * THE COMPARISON GAP (IGAP) IS HALVED FOR THE NEXT SWEEP
 */
shell (n, comp, exch)
int (*comp)(), (*exch)();
{
	int igap, iplusg, iex, i, imax;
	igap=n;
	while (igap > 1)
	{
		igap /= 2;
		imax = n-igap;
		do
		    {
			iex=0;
			for(i=0; i<imax; i++)
			{
				iplusg = i + igap;
				if ((*comp) (i, iplusg) ) continue;
				(*exch) (i, iplusg);
				iex=1;
			}
		} 
		while (iex>0);
	}
}
