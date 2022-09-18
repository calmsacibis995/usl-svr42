/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_SYSCONFIG_H	/* wrapper symbol for kernel use */
#define _SVC_SYSCONFIG_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/sysconfig.h	1.3"
#ident	"$Header: $"

/* cmd values for _sysconfig system call. 
** WARNING: This is an undocumented system call,
** therefore future compatibility can not
** guaranteed. 
*/ 

#define	 UNUSED			1
#define _CONFIG_NGROUPS		2	/* number of configured supplemental groups */
#define _CONFIG_CHILD_MAX	3	/* max # of processes per uid session */
#define _CONFIG_OPEN_FILES	4	/* max # of open files per process */
#define _CONFIG_POSIX_VER	5	/* POSIX version */
#define _CONFIG_PAGESIZE	6	/* system page size */
#define _CONFIG_CLK_TCK		7	/* ticks per second */
#define _CONFIG_XOPEN_VER	8	/* XOPEN version */
#define _CONFIG_NACLS_MAX	9	/* for Enhanced Security */
#define _CONFIG_ARG_MAX		10	/* max length of exec args */

#endif /* _SVC_SYSCONFIG_H */
