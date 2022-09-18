/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_PT_PTMS_H	/* wrapper symbol for kernel use */
#define _IO_PT_PTMS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:io/pt/ptms.h	1.3"
#ident	"$Header: $"
/*
 ================================================================
 =   WARNING!!!! This source is not supported in future source  =
 =   releases.                                                  =
 ================================================================
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)


#include <sys/types.h>	/* REQUIRED */
#include <sys/stream.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Structures and definitions supporting the pseudo terminal
 * drivers.
 */
struct pt_ttys {
	unsigned pt_state;	/* state of master/slave pair */
	queue_t *ptm_wrq; 	/* master's write queue pointer */
	queue_t *pts_wrq; 	/* slave's write queue pointer */
	mblk_t *pt_bufp;        /* ptr. to zero byte msg. blk. */
	pid_t tty;	        /* controlling tty pid */
};

/*
 * pt_state values
 */
#define PTLOCK	01	/* master/slave pair is locked */
#define PTMOPEN 02  	/* master side is open */
#define PTSOPEN 04	/* slave side is open */

/*
 * ioctl commands
 */
#define ISPTM	(('P'<<8)|1)	/* query for master */
#define UNLKPT	(('P'<<8)|2)	/* unlock master/slave pair */

#ifdef _KERNEL

/*
 * tunable parameters defined in ptm.cf
 */
extern struct pt_ttys ptms_tty[];	
extern int pt_cnt;

#endif	/* _KERNEL */

#endif	/* _IO_PT_PTMS_H */
