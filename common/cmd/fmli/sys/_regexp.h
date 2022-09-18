/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fmli:sys/_regexp.h	1.2"
#define	CBRA	2
#define	CCHR	4
#define	CDOT	8
#define	CCL	12
#define	CDOL	20
#define	CCEOF	22
#define	CKET	24
#define	CBACK	36
#define MCCHR	40
#define MCCL	44
#define NMCCL	48
#define CBRC	52
#define CLET	56
#define MCCE	60	/*  Multi-character collating element  */
#define NMCCE	64	/*  Negated MCCE  */

#define	STAR	01
#define RNGE	03

#define	NBRA	10

#define PLACE(c)	ep[c >> 3] |= _bittab[c & 07]
#define ISTHERE(c)	(ep[c >> 3] & _bittab[c & 07])
#define M_PLACE(c)	mcce_bmap[c >> 3] |= _bittab[c & 07]
#define M_UNPLACE(c)	mcce_bmap[c >> 3] &= ~_bittab[c & 07]
#define M_ISTHERE(c)	(mcce_bmap[c >> 3] & _bittab[c & 07])
