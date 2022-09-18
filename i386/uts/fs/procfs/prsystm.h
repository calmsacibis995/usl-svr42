/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_PROCFS_PRSYSTM_H	/* wrapper symbol for kernel use */
#define _FS_PROCFS_PRSYSTM_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/procfs/prsystm.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _FS_PROCFS_PROCFS_H
#include <fs/procfs/procfs.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/procfs.h>		/* SVR4.0COMPAT */

#else

#include <sys/procfs.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined(__STDC__)

struct user;	/* to eliminate warning from function prototype */

extern void	prawake(proc_t *);
extern void	prinvalidate(struct user *);
extern void	prgetpsinfo(proc_t *, struct prpsinfo *);
extern void	prgetfpregs(proc_t *, fpregset_t *);
extern int	prnsegs(proc_t *);
extern void	prexit(proc_t *);
extern void	prgetstatus(proc_t *, prstatus_t *);

#else

extern void	prawake();
extern void	prinvalidate();
extern void	prgetpsinfo();
extern void	prgetfpregs();
extern int	prnsegs();
extern void	prexit();
extern void	prgetstatus();

#endif	/* __STDC__ */

#endif	/* _FS_PROCFS_PRSYSTM_H */
