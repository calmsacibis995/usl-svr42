/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:Alloc.c	1.2"
/*
 * $XConsortium: Alloc.c,v 1.1 89/09/14 17:11:04 keith Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* stubs for use when Xalloc, Xrealloc and Xfree are not defined */

extern char	*malloc (), *realloc ();

long *
Xalloc (amount)
    unsigned	amount;
{
    if (amount == 0)
	amount = 1;
    return (long *) malloc (amount);
}

long *
Xrealloc (old, amount)
    long	*old;
    unsigned	amount;
{
    if (amount == 0)
	amount = 1;
    if (!old)
	return (long *) malloc (amount);
    return (long *) realloc ((char *) old, amount);
}

void
Xfree (old)
    long    *old;
{
    if (old)
	free ((char *) old);
}
