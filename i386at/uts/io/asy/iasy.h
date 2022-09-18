/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)uts-x86at:io/asy/iasy.h	1.10"
#ident	"$Header: iasy.h 1.2 91/11/04 16:26:43 anil unit-tested $"

/*
 *	Definitions for generic async support
*/
#ifndef T_CONNECT
#define T_CONNECT 42	/* Augment tty.h */
#endif

#define	T_TRLVL1 43	/* set rcv trigger level to 1 character	*/
#define	T_TRLVL2 44 	/* set rcv trigger level to 2 character	*/
#define	T_TRLVL3 45	/* set rcv trigger level to 3 character	*/
#define	T_TRLVL4 46 	/* set rcv trigger level to 4 character	*/

#define	IASY_HIWAT	512
#define	IASY_LOWAT	256
#define	IASY_BUFSZ	64	/* Chosen to be about	CLSIZE	*/

/*
 *	This is used to remember where the interrupt-time code is for
 *	each async line.
*/
struct iasy_hw {
	int  (*proc)();		/* proc routine does most operations */
	void (*hwdep)();	/* Called as last resort for unknown ioctls */
	int  (*vmproc)();	/* VPIX and MERGE hooks */
};

#define	L_BUF		0
#define	L_BREAK		3

#define	SPL	splstr


/* Define for mouse ioctl to set receive trigger levels */
#define	SETRTRLVL		(AIOC|68)	/* set rcv trigger level */

/* Serial in/out requests */
#define SO_DIVLLSB		1
#define SO_DIVLMSB		2
#define SO_LCR			3
#define SO_MCR			4
#define SI_MSR			1
#define SIO_MASK(elem)		(1<<((elem)-1))

#define 	iasychan(dev)    (dev&0x0f)
#define 	asymajor(dev)   ((dev>>8)&0x7f)

#define	IASY_UNIT_TO_TP(id, unit)	(struct strtty *)(&(asy_tty[(id)+(unit)]))
#define	IASY_TP_TO_UNIT(id, tp) 		(int)((tp) - &asy_tty[id])

/*
 * Function prototypes.
 */
int	iasy_input(struct strtty *, unsigned int);
int		iasy_output(struct strtty *);
void	iasy_hup(struct strtty *);
int	iasy_ctime(struct strtty *, int);
void	iasyhwdep(queue_t *, mblk_t *);
#ifdef VPIX
int iasy_register(minor_t, int, int (*)(), void (*)(), int (*)(), void (*)(), int (*)());
#else
int iasy_register(minor_t, int, int (*)(), void (*)(), int (*)(), void (*)());
#endif /* VPIX */

