/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:dirent.h	1.6.1.7"

#ifndef _DIRENT_H
#define _DIRENT_H

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define MAXNAMLEN	512		/* maximum filename length */
#define DIRBUF		2048		/* buffer size for fs-indep. dirs */
#endif

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

typedef struct	/* #ifdefs protect opaque type's members */
	{
#ifdef dd_fd
	int		__dd_fd;	/* file descriptor */
#else
	int		dd_fd;		/* file descriptor */
#endif
#ifdef dd_loc
	int		__dd_loc;	/* offset in block */
#else
	int		dd_loc;		/* offset in block */
#endif
#ifdef dd_size
	int		__dd_size;	/* amount of valid data */
#else
	int		dd_size;	/* amount of valid data */
#endif
#ifdef dd_buf
	char		*__dd_buf;	/* directory block */
#else
	char		*dd_buf;	/* directory block */
#endif
	}	DIR;			/* stream data from opendir() */

#if defined(__STDC__)

extern DIR		*opendir( const char * );
extern struct dirent	*readdir( DIR * );
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern long		telldir( DIR * );
extern void		seekdir( DIR *, long );
#endif /* !defined(_POSIX_SOURCE) */ 
extern void		rewinddir( DIR * );
extern int		closedir( DIR * );

#else

extern DIR		*opendir();
extern struct dirent	*readdir();
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern long		telldir();
extern void		seekdir();
#endif /* !defined(_POSIX_SOURCE) */ 
extern void		rewinddir();
extern int		closedir();

#endif

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define rewinddir( dirp )	seekdir( dirp, 0L )
#endif

#ifndef _SYS_DIRENT_H
#include <sys/dirent.h>
#endif

#endif	/* _DIRENT_H */
