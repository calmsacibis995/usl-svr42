/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_TYPES_H	/* wrapper symbol for kernel use */
#define _UTIL_TYPES_H	/* subject to change without notice */
#define _SYS_TYPES_H

#ident	"@(#)uts-x86:util/types.h	1.11"
#ident	"$Header: $"

typedef char *		addr_t; 	/* ?<core address> type (BSD compat) */
typedef char *		caddr_t;	/* ?<core address> type */
typedef long		daddr_t;	/* <disk address> type */
typedef char *		faddr_t;	/* XENIX Compat */
typedef long		off_t;		/* ?<offset> type */
typedef short		cnt_t;		/* ?<count> type */
typedef unsigned long	paddr_t;	/* <physical address> type */
typedef unsigned char	use_t;		/* use count for swap */
typedef short		sysid_t;	/* system id */
typedef short		index_t;	/* index into bitmaps */
typedef short		lock_t;		/* lock work for busy wait */
typedef	unsigned short	sel_t;		/* selector type */
typedef unsigned long	k_sigset_t;	/* kernel signal set type */
typedef unsigned long	k_fltset_t;	/* kernel fault set type */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
typedef struct _label { int val[6]; } label_t;	/* setjmp/longjmp save area */

typedef enum boolean { B_FALSE, B_TRUE } boolean_t;
#endif

/* POSIX Extensions */

typedef unsigned char	uchar_t;
typedef unsigned short	ushort_t;
typedef unsigned int	uint_t;
typedef unsigned long	ulong_t;

/*
 * The following type is for various kinds of identifiers
 * (process id, process group id, session id, scheduling
 * class id, user id, group id).  The actual type must be
 * the same for all since some system calls (such as sigsend)
 * take arguments that may be any of these types.
 * The enumeration type idtype_t defined in procset.h
 * is used to indicate what type of id is being specified.
 */
typedef long		id_t;		/* process id, group id, etc. */

/* typedef for kernel privilege mechanism */

typedef unsigned long	pvec_t;		/* kernel privilege vector */

/* typedefs for Mandatory Access Controls (MAC) */

typedef unsigned long	lid_t;		/* internal rep of security level */
typedef lid_t		level_t;	/* user's view of security level */

/* typedef for AUDIT event mask */

#if (!defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE))

#define ADT_EMASKSIZE	8		/* size of auditable event mask */
typedef unsigned long	adtemask_t[ADT_EMASKSIZE]; /* audit event mask type */

#endif

/* typedefs for dev_t components */

typedef unsigned long	major_t;	/* major part of device number */
typedef unsigned long	minor_t;	/* minor part of device number */

/*
 * For compatibility reasons the following typedefs (prefixed o_) 
 * can't grow regardless of the EFT definition.
 * For example, the definitions in s5inode.h remain small
 * to preserve compatibility in the S5 file system type.
 * Although applications should not explicitly use these typedefs
 * they may be included via a system header definition.
 * WARNING:  These typedefs may be removed in a future release.
 */

typedef unsigned short	o_mode_t;	/* old file attribute type */
typedef short		o_dev_t;	/* old device type */
typedef unsigned short	o_uid_t;	/* old UID type */
typedef o_uid_t		o_gid_t;	/* old GID type */
typedef short		o_nlink_t;	/* old file link type */
typedef short		o_pid_t;	/* old process id type */
typedef unsigned short	o_ino_t;	/* old inode type */

/* POSIX and XOPEN typedefs */

typedef int		key_t;		/* IPC key type */
typedef unsigned long	mode_t;		/* file attribute type */
typedef long		uid_t;		/* UID type */
typedef uid_t		gid_t;		/* GID type */
typedef unsigned long	nlink_t;	/* file link type */
typedef unsigned long	dev_t;		/* expanded device type */
typedef unsigned long	ino_t;		/* expanded inode type */
typedef long		pid_t;		/* process id type */

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int	size_t;		/* len param for string functions */
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef int		ssize_t;	/* to return byte count or indicate error */
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long		time_t;		/* time of day in seconds */
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long		clock_t;	/* relative time in a specified resolution */
#endif

#if defined(_KERNEL) || ( !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) )

typedef struct { int r[1]; } * physadr;
typedef unsigned char	unchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;
typedef int		spl_t;

#if defined(_KERNEL)

#define SHRT_MIN	-32768		/* min value of a "short int" */
#define SHRT_MAX	32767		/* max value of a "short int" */
#define USHRT_MAX	65535		/* max value of an "unsigned short int" */
#define INT_MIN		(-2147483647-1)	/* min value of an "int" */
#define INT_MAX		2147483647	/* max value of an "int" */
#define UINT_MAX	4294967295	/* max value of an "unsigned int" */
#define LONG_MIN	(-2147483647-1)	/* min value of a "long int" */
#define LONG_MAX	2147483647	/* max value of a "long int" */
#define ULONG_MAX	4294967295	/* max value of an "unsigned long int" */

#endif	/* _KERNEL */

/*
 * The following is the value of type id_t to use to indicate the
 * caller's current id.  See procset.h for the type idtype_t
 * which defines which kind of id is being specified.
 */
#define P_MYID		(-1)

#define NOPID		((pid_t)(-1))

#ifndef NODEV
#define NODEV		((dev_t)(-1))
#endif

#define P_MYPID		((pid_t)0)

/*
 * A host identifier is used to uniquely define a particular node
 * on an rfs network.  Its type is as follows.
 */
typedef long		hostid_t;

/*
 * The following value of type hostid_t is used to indicate the
 * current host.
 */
#define P_MYHOSTID	((hostid_t)(-1))

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;
typedef struct _quad { long val[2]; } quad;	/* used by UFS */

/*
 * Nested include for BSD/sockets source compatibility.
 * (The select macros used to be defined here).
 */

#ifdef _KERNEL_HEADERS

#ifndef _FS_SELECT_H
#include <fs/select.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/select.h>	/* SVR4.0COMPAT */

#else

#include <sys/select.h>	/* SVR4.0COMPAT */

#endif	/* _KERNEL_HEADERS */

#endif	/* _KERNEL || (!_POSIX_SOURCE && !_XOPEN_SOURCE) */

/*
 * These were added to allow non-ANSI compilers to compile the system.
 */

#ifdef __STDC__

#ifndef _VOID
#define _VOID		void
#endif

#else	/* not ANSI */

#ifndef _VOID
#define _VOID		char
#endif

#ifndef const
#define const
#endif

#ifndef volatile
#define volatile
#endif

#endif	/* __STDC__ */

#endif	/* _UTIL_TYPES_H */
