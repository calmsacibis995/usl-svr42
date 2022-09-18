/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/siutils.c	1.2"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */


/*
 * si_pfill(depth, val)	-- replicate the value passed through the
 *			word as many time as is required by the depth.
 *
 * Input:
 *	int		depth	-- depth being filled.
 *	unsigned int	val	-- value to be replicated.
 */
unsigned int
si_pfill(depth, val)
int	depth;
register unsigned int val;
{
	extern unsigned int sipfillmask1[];
	extern unsigned int sipfillmask2[];
	extern unsigned int sipfillmask4[];

	val &= (1 << depth) - 1;

	if (depth == 1)
		return(sipfillmask1[val]);
	if (depth == 2)
		return(sipfillmask2[val]);
	if (depth <= 4)
		return(sipfillmask4[val]);
	if (depth <= 8)
		return(val | val << 8 | val << 16 | val << 24);
	if (depth <= 16)
		return(val | val << 16);
	if (depth <= 32)
		return(val);
}
