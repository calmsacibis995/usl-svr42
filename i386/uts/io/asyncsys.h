/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_ASYNCSYS_H	/* wrapper symbol for kernel use */
#define _IO_ASYNCSYS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:io/asyncsys.h	1.4"
#ident	"$Header: $"

/*
**	Structure used by library routines, aread.c, awrite.c,
**	and asyncio.c to pass async request parameters 
**	to syscall async().
*/

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _FS_FILE_H
#include <fs/file.h>		/* REQUIRED */
#endif

#ifndef _PROC_PRIOCNTL_H
#include <proc/priocntl.h>	/* REQUIRED */
#endif

#ifndef _PROC_USER_H
#include <proc/user.h>		/* REQUIRED */
#endif

#ifndef _PROC_PROC_H
#include <proc/proc.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/file.h>		/* REQUIRED */
#include <sys/priocntl.h>	/* REQUIRED */
#include <sys/user.h>		/* REQUIRED */
#include <sys/proc.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	struct asyncop {
	int		a_syscall;	/* syscall to be done asynchronously 	  */
	int		a_sysarg[MAXSYSARGS];	/* syscall arguments 	  	  */
	ushort		a_flags;	/* the flags defined below 		  */
	int		a_error;	/* error code; the field is used to save  */
					/* error encountered in processing the    */
					/* async() system call. It is not for     */
					/* any error found in the actual system   */
					/* call run asynchronously because the    */
					/* aioop struct may be reused after       */
					/* returning from async() call.		  */
	off_t		a_offset;	/* relative offset in bytes               */
	pcparms_t	a_pri;		/* syscall priority 			  */
} asyncop_t;

#ifdef _KERNEL
/*
**	Structure used to pass asynchronous system call
**	request from user process to server
**
**	Note, ar_fflink and ar_fblink must be the first two fields in this
**	structure or the list manipulation routines will break.
*/

typedef struct aioreq {
	struct	aioreq	*ar_fflink;	/* file async request forward link 	  */
	struct	aioreq	*ar_fblink;	/* file async request backward link	  */
	struct	aioreq	*ar_sflink;	/* aioserver request forward link 	  */
	struct	aioreq	*ar_sblink;	/* aioserver request backward link	  */
	proc_t	 	*ar_procp;	/* proc requests the async sys call 	  */
	struct	sysent	*ar_syscallp;	/* sys call to do asynchronously 	  */
	int		ar_args[MAXSYSARGS];/* args to async sys call 	          */
	daddr_t		ar_ulimit;	/* client's ulimit 			  */
	file_t		*ar_ofilep;	/* original file table entry 		  */
	file_t		ar_aiofile;	/* file table entry server will use 	  */
	pcparms_t	ar_pri;		/* system call priority 		  */
	proc_t		*ar_serverp;	/* procp of server acting on this 	  */
					/* request. Null if on request queue 	  */
} aioreq_t;

extern	short	 naioproc;		/* # of async requests allowed per  */
					/* process			    */
extern	int	 min_aio_servers;	/* min # of aio servers to make at  */
extern  int	 max_aio_servers;	/* max # of servers allowed 	    */
extern	int	 aio_server_timeout;	/* # of seconds server should wait  */
extern	int	 aio_size;		/* # of aio request structs in pool */
extern	aioreq_t aio[];			/* pool of aio request structs      */
extern	proc_t	 *aiodaemonp;		/* aiodaemon's process table pointer*/

extern	void	 aiodaemon();		/* aiodaemon entry point	    */
extern	void	 stop_aio();		/* stop outstanding async call 	    */
extern	void	 aioinit();		/* initialize aio linked lists	    */	
#endif

#endif	/* _IO_ASYNCSYS_H */
