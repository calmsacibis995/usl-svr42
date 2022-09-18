/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_PROCFS_PRDATA_H	/* wrapper symbol for kernel use */
#define _FS_PROCFS_PRDATA_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/procfs/prdata.h	1.8.4.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>		/* REQUIRED */
#endif

#ifndef _IO_UIO_H
#include <io/uio.h>		/* REQUIRED */
#endif

#ifndef _PROC_USER_H
#include <proc/user.h>		/* REQUIRED */
#endif

#ifndef _PROC_PROC_H
#include <proc/proc.h>		/* REQUIRED */
#endif

#ifndef _MEM_SEG_H
#include <mem/seg.h>		/* REQUIRED */
#endif

#ifndef _FS_PROCFS_PROCFS_H
#include <fs/procfs/procfs.h>	/* REQUIRED */
#endif

#ifndef _PROC_REGSET_H
#include <proc/regset.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/uio.h>		/* REQUIRED */
#include <sys/user.h>		/* REQUIRED */
#include <sys/proc.h>		/* REQUIRED */
#include <vm/seg.h>		/* REQUIRED */
#include <sys/procfs.h>		/* REQUIRED */
#include <sys/regset.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#define	min(a,b)	((a) <= (b) ? (a) : (b))
#define	max(a,b)	((a) >= (b) ? (a) : (b))
#define	round(r)	(((r)+sizeof(int)-1)&(~(sizeof(int)-1)))

#define	PNSIZ	5			/* size of /proc name entries */


extern struct vfs *procvfs;	/* Points to /proc vfs entry. */
extern dev_t procdev;
extern struct vnodeops prvnodeops;


/*
 * Macros for mapping between i-numbers and pids.
 */
#define	PRBIAS	64
#define	itop(n)	((int)((n)-PRBIAS))	/* i-number to pid */
#define	ptoi(n)	((int)((n)+PRBIAS))	/* pid to i-number */

typedef struct prnode {
	struct prnode	*pr_free;	/* freelist pointer */
	struct vnode	*pr_vnext;	/* linked list of invalid vnodes */
	struct vnode	pr_vnode;	/* associated vnode */
	struct proc	*pr_proc;	/* process being traced */
	short		pr_mode;	/* file mode bits */
	short		pr_opens;	/* count of opens */
	short		pr_writers;	/* count of opens for writing */
	short		pr_flags;	/* private flags */
} prnode_t;

/*
 * Conversion macros.
 */
#define	VTOP(vp)	((struct prnode *)(vp)->v_data)
#define	PTOV(pnp)	((struct vnode *)&(pnp)->pr_vnode)

/*
 * Flags for pr_flags.
 */
#define	PREXCL		0x01	/* Exclusive-use (disallow opens for write) */
#define	PRINVAL		0x02	/* vnode is invalid (security provision) */

/*
 * Block constants.
 */
#define	PRBSIZE		1024	/* /proc block size */
#define	PRFSIZE		1024	/* /proc fundamental block size */

/*
 * Flags to prlock().
 */
#define	ZNO	0	/* Fail on encountering a zombie process. */
#define	ZYES	1	/* Allow zombies. */

/*
 * Assign one set to another (possible different sizes).
 *
 * Assigning to a smaller set causes members to be lost.
 * Assigning to a larger set causes extra members to be cleared.
 */
#define	prassignset(ap, sp)					\
{								\
	register int _i_ = sizeof(*(ap))/sizeof(u_long);	\
	while (--_i_ >= 0)					\
		((u_long*)(ap))[_i_] =				\
		  (_i_ >= sizeof(*(sp))/sizeof(u_long)) ?	\
		  0 : ((u_long*)(sp))[_i_];			\
}

/*
 * Determine whether or not a set (of arbitrary size) is empty.
 */
#define prisempty(sp) setisempty((u_long *)(sp), sizeof(*(sp))/sizeof(u_long))

#if defined(__STDC__)
int		prusrio(proc_t *, enum uio_rw, struct uio *);
int		prisreadable(proc_t *, cred_t *);
int		prlock(struct prnode *, int, int);
void		prunlock(struct prnode *);
int		prgetstatus(proc_t *, prstatus_t *);
void		prgetaction(proc_t *, user_t *, u_int, struct sigaction *);
int		prnsegs(proc_t *);
void		prgetmap(proc_t *, prmap_t *);
vnode_t		*prvnode(proc_t *, struct seg *, caddr_t);
int		prgetpsinfo(proc_t *, struct prpsinfo *);
int		setisempty(u_long *, unsigned);

#else

int		prusrio();
int		prisreadable();
int		prlock();
void		prunlock();
int		prgetstatus();
void		prgetaction();
int		prnsegs();
void		prgetmap();
vnode_t		*prvnode();
int		prgetpsinfo();
int		setisempty();

#endif

/*
 * Machine-dependent routines (defined in prmachdep.c).
 */

#if defined(__STDC__)

user_t		*prumap(proc_t *);
void		prunmap(proc_t *);
void		prgetregs(user_t *, gregset_t);
void		prsetregs(user_t *, gregset_t);
greg_t		prgetpc(gregset_t);
int		prhasfp(void);
int		prgetfpregs(proc_t *, fpregset_t *);
int		prsetfpregs(proc_t *, fpregset_t *);
int		prgetdbregs(proc_t *, dbregset_t *);
int		prsetdbregs(proc_t *, dbregset_t *);
caddr_t		prgetpsaddr(proc_t *);
void		prstep(proc_t *, user_t *);
void		prsvaddr(proc_t *, user_t *, caddr_t);
caddr_t		prmapin(proc_t *, caddr_t, int);
void		prmapout(proc_t *, caddr_t, caddr_t, int);
caddr_t		prfastmapin(proc_t *, caddr_t, int);
void		prfastmapout(proc_t *, caddr_t, caddr_t, int);
int		prrgetu(caddr_t, caddr_t, u_int);

#else

user_t		*prumap();
void		prunmap();
void		prgetregs();
void		prsetregs();
greg_t		prgetpc();
int		prhasfp();
int		prgetfpregs();
int		prsetfpregs();
int		prgetdbregs();
int		prsetdbregs();
caddr_t		prgetpsaddr();
void		prstep();
void		prsvaddr();
caddr_t		prmapin();
void		prmapout();
caddr_t		prfastmapin();
void		prfastmapout();
int		prrgetu();

#endif

/*
 * Prototypes for other external functions; these should appear elsewhere
 * but don't.
 */
#if defined(__STDC__)

int	donice(proc_t *, cred_t *, int, int *);
int	fsig(proc_t *);

#else

int	donice();
int	fsig();

#endif

#endif	/* _FS_PROCFS_PRDATA_H */
