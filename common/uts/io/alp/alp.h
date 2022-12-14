/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_ALP_ALP_H	/* wrapper symbol for kernel use */
#define _IO_ALP_ALP_H	/* subject to change without notice */

#ident	"@(#)uts-comm:io/alp/alp.h	1.3.2.1"
#ident	"$Header: $"

#ifndef ALP_QUERY	/* possible source dependency */

#ifdef _KERNEL
struct algo {
	int al_flag;		/* 0 = in-core, 1=user-level */
	queue_t *al_rq;		/* queues for user-level algorithms */
	queue_t *al_wq;		/* queues for user-level algorithms */
	mblk_t *(*al_func)();	/* interface routine */
	caddr_t (*al_open)();	/* open/close routine */
	unsigned char *al_name;	/* name */
	unsigned char *al_expl;	/* explanation */
	struct algo *al_next;	/* next in chain */
};
#endif _KERNEL

/*
 * alp_con is a function returning a pointer to a function of two arguments
 * returning an mblk_t:
 */

mblk_t *(*alp_con())(mblk_t *x, caddr_t y);
mblk_t *alp_discon();
struct algo *alp_query();
int alp_register();

#ifdef TIMID
typedef mblk_t *(*ptr2fn_R_mblk_t_ptr)();
#endif

/*
 * "ptr2fun_R_mblk_t_ptr" is otherwise known as
 * "pointer to function returning pointer to mblk_t":
 *
 * The timid can cast an x->y that's not a function pointer by saying:
 *		z = (ptr2fn_R_mblk_t_ptr) x->y;
 * otherwise, this suffices:
 *		z = (mblk_t *(*)()) x->y;
 * alternatively, x->y can be called directly with:
 *		i = (*(mblk_t *(*)()) x->y)(a1, a2);
 * The whole issue can be bypassed by declaring a variable ("x") that has
 * the correct type to receive the return value of "alp_con":
 *		mblk_t *(*x)();
 */

#define IALP	('&'<<8|128)

#define ALP_QUERY	(IALP| 1)	/* query ioctl */

/*
 * query return value
 */

struct alp_q {
	int a_seq;		/* sequence number */
	int a_flag;		/* flag */
	unsigned char a_name[16];	/* algorithm name */
	unsigned char a_expl[64];	/* explanation field */
};

#endif	/* ALP_HEADER */
#endif	/* _IO_ALP_ALP_H */
