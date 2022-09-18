/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FCNTL_H	/* wrapper symbol for kernel use */
#define _FS_FCNTL_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/fcntl.h	1.5"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Flag values accessible to open(2) and fcntl(2)
 * (the first three can only be set by open).
 */
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#if !defined(_POSIX_SOURCE) 
#define	O_NDELAY	0x04	/* non-blocking I/O */
#endif /* !defined(_POSIX_SOURCE) */ 
#define	O_APPEND	0x08	/* append (writes guaranteed at the end) */
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) 
#define	O_SYNC		0x10	/* synchronous write option */
#endif /* !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE */ 
#define	O_NONBLOCK	0x80	/* non-blocking I/O (POSIX) */

/*
 * Flag values accessible only to open(2).
 */
#define	O_CREAT		0x100	/* open with file create (uses third open arg) */
#define	O_TRUNC		0x200	/* open with truncation */
#define	O_EXCL		0x400	/* exclusive open */
#define	O_NOCTTY	0x800	/* don't allocate controlling tty (POSIX) */

/* fcntl(2) requests */
#define	F_DUPFD		0	/* duplicate fildes */
#define	F_GETFD		1	/* get fildes flags */
#define	F_SETFD		2	/* set fildes flags */
#define	F_GETFL		3	/* get file flags */
#define	F_SETFL		4	/* set file flags */

/*
 * Applications that read /dev/mem must be built like the kernel.  A
 * new symbol "_KMEMUSER" is defined for this purpose.
 */
#if defined(_KERNEL) || defined(_KMEMUSER)
#define	F_GETLK		14	/* get file lock */
#define	F_O_GETLK	5	/* SVR3 get file lock */

#else	/* user definition */

#if defined(_STYPES)	/* SVR3 definition */
#define	F_GETLK		5	/* get file lock */
#else
#define	F_GETLK		14	/* get file lock */
#endif	/* defined(_STYPES) */

#endif	/* defined(_KERNEL) */

#define	F_SETLK		6	/* set file lock */
#define	F_SETLKW	7	/* set file lock and wait */

#if !defined(_POSIX_SOURCE) 
#define	F_CHKFL		8	/* unused */

#define	F_ALLOCSP	10	/* reserved */
#define	F_FREESP	11	/* free file space */

#define F_RSETLK	20	/* remote SETLK for NFS */
#define F_RGETLK	21	/* remote GETLK for NFS */
#define F_RSETLKW	22	/* remote SETLKW for NFS */

#define	F_GETOWN	23	/* get owner (socket emulation) */
#define	F_SETOWN	24	/* set owner (socket emulation) */
#endif /* !defined(_POSIX_SOURCE) */ 

#define	F_CHSIZE	0x6000	/* XENIX chsize() system call */
#define	F_RDCHK		0x6001	/* XENIX rdchk() system call */
/*
 * File segment locking set data type - information passed to system by user.
 */
#if defined(_KERNEL) || defined(_KMEMUSER)
	/* EFT definition */
typedef struct flock {
	short	l_type;		/* type of lock */
	short	l_whence;	/* flag for starting offset */
	off_t	l_start;	/* relative offset in bytes */
	off_t	l_len;		/* size; if 0 then until EOF */
        long	l_sysid;	/* returned with F_GETLCK */
        pid_t	l_pid;		/* returned with F_GETLCK */
	long	l_pad[4];	/* reserve area */
} flock_t;

typedef struct o_flock {
	short	l_type;		/* type of lock */
	short	l_whence;	/* flag for starting offset */
	long	l_start;	/* relative offset in bytes */
	long	l_len;		/* size; if 0 then until EOF */
        short   l_sysid;	/* returned with F_GETLCK */
        o_pid_t l_pid;		/* returned with F_GETLCK */
} o_flock_t;

#else		/* user level definition */

#if defined(_STYPES)
	/* SVR3 definition */
typedef struct flock {
	short	l_type;		/* type of lock */
	short	l_whence;	/* flag for starting offset */
	off_t	l_start;	/* relative offset in bytes */
	off_t	l_len;		/* size; if 0 then until EOF */
	short	l_sysid;	/* returned with F_GETLCK */
        o_pid_t	l_pid;		/* returned with F_GETLCK */
} flock_t;


#else

typedef struct flock {
	short	l_type;		/* type of lock */
	short	l_whence;	/* flag for starting offset */
	off_t	l_start;	/* relative offset in bytes */
	off_t	l_len;		/* len == 0 means until end of file */
	long	l_sysid;	/* returned with F_GETLCK */
        pid_t	l_pid;		/* returned with F_GETLCK */
	long	l_pad[4];	/* reserve area */
} flock_t;

#endif	/* defined(_STYPES) */

#endif	/* defined(_KERNEL) */

/*
 * File segment locking types.
 */
#define	F_RDLCK	01	/* read lock */
#define	F_WRLCK	02	/* write lock */
#define	F_UNLCK	03	/* remove lock(s) */

/*
 * POSIX constants 
 */

#define	O_ACCMODE	3	/* mask for file access modes */
#define	FD_CLOEXEC	1	/* close on exec flag */

#endif	/* _FS_FCNTL_H */
