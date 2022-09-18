/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_FCPRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_FCPRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/class/fcpriocntl.h	1.1"
#ident	"$Header: $"

/*
 * Fixed Class specific structures for the priocntl system call.
 */

typedef struct fcparms {
	short	fc_uprilim;	/* user priority limit */
	short	fc_upri;	/* user priority */
	long	fc_timeleft;	/* time-left for this process */
	short	fc_cpupri;	/* the assigned cpu priority */
	short	fc_umdpri;	/* the computed user mode priority */
} fcparms_t;


typedef struct fcinfo {
	short	fc_maxupri;	/* configured limits of user priority range */
} fcinfo_t;

#define	FC_NOCHANGE	-32768

/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct fcadmin {
	struct fcdpent	*fc_dpents;
	short		fc_ndpents;
	short		fc_cmd;
} fcadmin_t;

#define	FC_GETDPSIZE	1
#define	FC_GETDPTBL	2
#define	FC_SETDPTBL	3


#endif	/* _PROC_CLASS_FCPRIOCNTL_H */
