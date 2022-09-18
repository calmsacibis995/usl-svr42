/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_FS_FS_SUBR_H	/* wrapper symbol for kernel use */
#define _FS_FS_SUBR_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/fs_subr.h	1.3.2.1"
#ident	"$Header: $"

/*
 * Utilities shared among file system implementations.
 */
extern int	fs_nosys();
extern int	fs_sync();
extern void	fs_rwlock();
extern void	fs_rwunlock();
extern int	fs_cmp();
extern int	fs_frlock();
extern int	fs_setfl();
extern int	fs_poll();
extern int	fs_vcode();
extern int	fs_pathconf();
extern void	fs_itoh();

#endif /* _FS_FS_SUBR_H */
