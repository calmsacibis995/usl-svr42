/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_ACCT_H	/* wrapper symbol for kernel use */
#define _PROC_ACCT_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/acct.h	1.3"
#ident	"$Header: $"
/*
 * Accounting structures
 */
#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	ushort comp_t;		/* "floating point" */
		/* 13-bit fraction, 3-bit exponent  */

/* SVR4 acct structure */
struct	acct
{
	char	ac_flag;		/* Accounting flag */
	char	ac_stat;		/* Exit status */
	uid_t	ac_uid;			/* Accounting user ID */
	gid_t	ac_gid;			/* Accounting group ID */
	dev_t	ac_tty;			/* control typewriter */
	time_t	ac_btime;		/* Beginning time */
	comp_t	ac_utime;		/* acctng user time in clock ticks */
	comp_t	ac_stime;		/* acctng system time in clock ticks */
	comp_t	ac_etime;		/* acctng elapsed time in clock ticks */
	comp_t	ac_mem;			/* memory usage */
	comp_t	ac_io;			/* chars transferred */
	comp_t	ac_rw;			/* blocks read or written */
	char	ac_comm[8];		/* command name */
};	

/* Account commands will use this header to read SVR3
** accounting data files.
*/

struct	o_acct
{
	char	ac_flag;		/* Accounting flag */
	char	ac_stat;		/* Exit status */
	o_uid_t	ac_uid;			/* Accounting user ID */
	o_gid_t	ac_gid;			/* Accounting group ID */
	o_dev_t	ac_tty;			/* control typewriter */
	time_t	ac_btime;		/* Beginning time */
	comp_t	ac_utime;		/* acctng user time in clock ticks */
	comp_t	ac_stime;		/* acctng system time in clock ticks */
	comp_t	ac_etime;		/* acctng elapsed time in clock ticks */
	comp_t	ac_mem;			/* memory usage */
	comp_t	ac_io;			/* chars transferred */
	comp_t	ac_rw;			/* blocks read or written */
	char	ac_comm[8];		/* command name */
};	

#if !defined(_KERNEL)
#if defined(__STDC__)
extern int acct(const char *);
#else
extern int acct();
#endif
#endif /* !defined(_KERNEL) */

#define	acctevt(x)		u.u_acflag |= (x)

#define	AFORK	01		/* has executed fork, but no exec */
#define	ASU	02		/* used privilege */
#define	ACCTF	0300		/* record type: 00 = acct */
#define AEXPND	040		/* expanded acct structure */

#endif	/* _PROC_ACCT_H */
