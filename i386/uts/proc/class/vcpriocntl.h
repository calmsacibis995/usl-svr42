/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_VCPRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_VCPRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/class/vcpriocntl.h	1.2"
#ident	"$Header: $"

/*
 * VP/ix class specific structures for the priocntl system call.
 */

typedef struct vcparms {
	short	vc_uprilim;	/* user priority limit */
	short	vc_upri;	/* user priority */
} vcparms_t;


typedef struct vcinfo {
	short	vc_maxupri;	/* configured limits of user priority range */
} vcinfo_t;

#define	VC_NOCHANGE	-32768

/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct vcadmin {
	struct vcdpent	*vc_dpents;
	short		vc_ndpents;
	short		vc_cmd;
} vcadmin_t;

#define	VC_GETDPSIZE	1
#define	VC_GETDPTBL	2
#define	VC_SETDPTBL	3


#endif	/* _PROC_CLASS_VCPRIOCNTL_H */
