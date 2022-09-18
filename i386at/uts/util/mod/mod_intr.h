/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _UTIL_MOD_MOD_INTR_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_INTR_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:util/mod/mod_intr.h	1.3"
#ident "$Header: $"

/*
 * The number of interrupt handlers each packet can hold.
 * This number should be large enough to make the need for
 * multiple packets a rarity.
 */
#define	MOD_NSI	8

/*
 * Packet of interrupt handlers for a given vector.
 * When needed, multiple packets can be allocated and linked together.
 */
struct	mod_shr_v	{
	void	(*msv_sih[MOD_NSI+1])();	/* pointers to interrupt handlers */
	int	msv_cnt;			/* number of handlers in this packet */
	struct	mod_shr_v	*msv_next;	/* pointer to next packet */
};

#endif	/* _UTIL_MOD_MOD_INTR_H */
