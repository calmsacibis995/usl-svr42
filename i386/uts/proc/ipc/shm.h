/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifndef _PROC_IPC_SHM_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_SHM_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/ipc/shm.h	1.6"
#ident	"$Header: $"

/*
**	IPC Shared Memory Facility.
*/

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#ifndef _PROC_IPC_IPC_H
#include <proc/ipc/ipc.h> /* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ipc.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
**	Implementation Constants.
*/

#define	SHMLBA	ctob(1)	/* segment low boundary address multiple */
			/* (SHMLBA must be a power of 2) */

/*
**	Padding constants used to reserve space for future use.
*/

#define	SHM_PAD		2
#define	SHM_PAD1	4

/*
**	Permission Definitions.
*/

#define	SHM_R	IPC_R	/* read permission */
#define	SHM_W	IPC_W	/* write permission */

/*
**	ipc_perm Mode Definitions.
*/

#define SHM_LOCKED      010000	/* shmid locked */
#define SHM_LOCKWAIT    020000	/* shmid wanted */

/*
**	Definitions (obsolete) kept for source-level compatibility only
*/
#define	SHM_INIT	01000	/* grow segment on next attach */
#define	SHM_DEST	02000	/* destroy segment when # attached = 0 */

#define PSHM    (PZERO + 1)     /* sleep priority */

/* define resource locking macros */
#define SHMLOCK(sp) { \
        while ((sp)->shm_perm.mode & SHM_LOCKED) { \
                (sp)->shm_perm.mode |= SHM_LOCKWAIT; \
                (void) sleep((caddr_t)(sp), PSHM); \
        } \
        (sp)->shm_perm.mode |= SHM_LOCKED; \
}

#define SHMUNLOCK(sp) { \
        (sp)->shm_perm.mode &= ~SHM_LOCKED; \
        if ((sp)->shm_perm.mode & SHM_LOCKWAIT) { \
                (sp)->shm_perm.mode &= ~SHM_LOCKWAIT; \
                wakeprocs((caddr_t)(sp), PRMPT); \
        } \
}

/*
**	Message Operation Flags.
*/

#define	SHM_RDONLY	010000	/* attach read-only (else read-write) */
#define	SHM_RND		020000	/* round attach address to SHMLBA */

/*
**	Structure Definitions.
*/

/*
**	There is a shared mem id data structure (shmid_ds) for each 
**	segment in the system.
*/

#if defined(_KERNEL) || defined(_KMEMUSER)
struct shmid_ds {
	struct ipc_perm shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort_t		shm_lkcnt;	/* number of times it is being locked */
	pid_t		shm_lpid;	/* pid of last shmop */
	pid_t		shm_cpid;	/* pid of creator */
	ulong_t		shm_nattch;	/* used only for shminfo */
	ulong_t		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	long		shm_pad1;	/* reserved for time_t expansion */
	time_t		shm_dtime;	/* last shmdt time */
	long		shm_pad2;	/* reserved for time_t expansion */
	time_t		shm_ctime;	/* last change time */
	long		shm_pad3;	/* reserved for time_t expansion */
	long		shm_pad4[SHM_PAD1];	/* reserve area  */
};

/* SVR3 structure */
struct o_shmid_ds {
	struct o_ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort_t		shm_lkcnt;	/* number of times it is being locked */
	char 		pad[SHM_PAD];		
	o_pid_t		shm_lpid;	/* pid of last shmop */
	o_pid_t		shm_cpid;	/* pid of creator */
	ushort_t		shm_nattch;	/* used only for shminfo */
	ushort_t		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
};
#else	/* user definition */
#if !defined(_STYPES)
/* this maps to the kernel struct shmid_ds */
struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort_t		shm_lkcnt;	/* number of times it is being locked */
	pid_t		shm_lpid;	/* pid of last shmop */
	pid_t		shm_cpid;	/* pid of creator */
	ulong_t		shm_nattch;	/* used only for shminfo */
	ulong_t		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	long		shm_pad1;	/* reserved for time_t expansion */
	time_t		shm_dtime;	/* last shmdt time */
	long		shm_pad2;	/* reserved for time_t expansion */
	time_t		shm_ctime;	/* last change time */
	long		shm_pad3;	/* reserved for time_t expansion */
	long		shm_pad4[SHM_PAD1];	/* reserve area  */
};
#else	/* NON EFT */
/* old struct for compatibility */
struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort_t		shm_lkcnt;	/* number of times it is being locked */
	char 		pad[SHM_PAD];		
	o_pid_t		shm_lpid;	/* pid of last shmop */
	o_pid_t		shm_cpid;	/* pid of creator */
	ushort_t		shm_nattch;	/* used only for shminfo */
	ushort_t		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
};
#endif	/* end !defined(_STYPES) */
#endif	/* end defined(_KERNEL */

struct	shminfo {
	int	shmmax,		/* max shared memory segment size */
		shmmin,		/* min shared memory segment size */
		shmmni,		/* # of shared memory identifiers */
		shmseg;		/* max attached shared memory	  */
				/* segments per process		  */
};


/*
 * Shared memory control operations
 */

#define SHM_LOCK	3	/* Lock segment in core */
#define SHM_UNLOCK	4	/* Unlock segment */

#ifdef	_KERNEL
extern int	shmconv();
extern void	shminit();
#endif	/* _KERNEL */

#if defined(__STDC__) && !defined(_KERNEL)
int shmctl(int, int, ...);
int shmget(key_t, int, int);
void *shmat(int, void *, int);
int shmdt(void *);
#endif

typedef struct segacct {
	struct segacct	*sa_next;
	caddr_t		 sa_addr;
	size_t		 sa_len;
	struct anon_map *sa_amp;
} segacct_t;

#endif	/* _PROC_IPC_SHM_H */
