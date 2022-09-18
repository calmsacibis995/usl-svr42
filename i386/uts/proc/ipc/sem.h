/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_SEM_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_SEM_H	/* subject to change without notice */


#ident	"@(#)uts-x86:proc/ipc/sem.h	1.6"
#ident	"$Header: $"
/*
 * IPC Semaphore Facility.
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
 * Implementation Constants.
 */

#define	PSEMN	(PZERO + 3)	/* sleep priority waiting for greater value */
#define	PSEMZ	(PZERO + 2)	/* sleep priority waiting for zero */

/*
 * Padding constants used to reserve space for future use.
 */

#define	SEM_PAD		4

/*
 * Permission Definitions.
 */

#define	SEM_A	IPC_W	/* alter permission */
#define	SEM_R	IPC_R	/* read permission */

/*
 * Semaphore Operation Flags.
 */

#define	SEM_UNDO	010000	/* set up adjust on exit entry */

/*
 * Semctl Command Definitions.
 */

#define	GETNCNT	3	/* get semncnt */
#define	GETPID	4	/* get sempid */
#define	GETVAL	5	/* get semval */
#define	GETALL	6	/* get all semval's */
#define	GETZCNT	7	/* get semzcnt */
#define	SETVAL	8	/* set semval */
#define	SETALL	9	/* set all semval's */

/*
 * Structure Definitions.
 */

/*
 * There is one semaphore id data structure (semid_ds) for each set of semaphores
 * in the system.
 */


#if defined(_KERNEL) || defined(_KMEMUSER)
/* expanded semid_ds structure */
struct semid_ds {
	struct ipc_perm sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	ushort_t		sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	long		sem_pad1;	/* reserved for time_t expansion */
	time_t		sem_ctime;	/* last change time */
	long		sem_pad2;	/* time_t expansion */
	long		sem_pad3[SEM_PAD];	/* reserve area */
};
/* SVR3 structure */
struct o_semid_ds {
	struct o_ipc_perm sem_perm;	/* operation permission struct */
	struct o_sem	*sem_base;	/* ptr to first semaphore in set */
	ushort_t		sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	time_t		sem_ctime;	/* last change time */
};
/*
 * There is one semaphore structure (sem) for each semaphore in the system.
 */

struct sem {
	ushort_t	semval;		/* semaphore value */
	pid_t	sempid;		/* pid of last operation */
	ushort_t	semncnt;	/* # awaiting semval > cval */
	ushort_t	semzcnt;	/* # awaiting semval = 0 */
};

/* SVR3 sem structure */
struct o_sem {
	ushort_t	semval;		/* semaphore value */
	o_pid_t	sempid;		/* pid of last operation */
	ushort_t	semncnt;	/* # awaiting semval > cval */
	ushort_t	semzcnt;	/* # awaiting semval = 0 */
};


#else		/* user level definition */

#if !defined(_STYPES)

struct semid_ds {
	struct ipc_perm sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	ushort_t		sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	long		sem_pad1;	/* reserved for time_t expansion */
	time_t		sem_ctime;	/* last change time */
	long		sem_pad2;		/* time_t expansion */
	long		sem_pad3[SEM_PAD];	/* reserve area */
};

struct sem {
	ushort_t	semval;		/* semaphore value */
	pid_t	sempid;		/* pid of last operation */
	ushort_t	semncnt;	/* # awaiting semval > cval */
	ushort_t	semzcnt;	/* # awaiting semval = 0 */
};

#else	/* SVR3 definition */

struct semid_ds {
	struct ipc_perm sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	ushort_t		sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	time_t		sem_ctime;	/* last change time */
};

struct sem {
	ushort_t	semval;		/* semaphore value */
	o_pid_t	sempid;		/* pid of last operation */
	ushort_t	semncnt;	/* # awaiting semval > cval */
	ushort_t	semzcnt;	/* # awaiting semval = 0 */
};

#endif	/* !defined(_STYPES) */
#endif	/* defined(_KERNEL) */




/*
 * There is one undo structure per process in the system.
 */

struct sem_undo {
	struct sem_undo	*un_np;	/* ptr to next active undo structure */
	short		un_cnt;	/* # of active entries */
	struct undo {
		short	un_aoe;	/* adjust on exit values */
		ushort_t	un_num;	/* semaphore # */
		int	un_id;	/* semid */
	}	un_ent[1];	/* undo entries (one minimum) */
};

/*
 * Semaphore information structure
 */
struct	seminfo	{
	int	semmap;		/* # of entries in semaphore map */
	int	semmni;		/* # of semaphore identifiers */
	int	semmns;		/* # of semaphores in system */
	int	semmnu;		/* # of undo structures in system */
	int	semmsl;		/* max # of semaphores per id */
	int	semopm;		/* max # of operations per semop call */
	int	semume;		/* max # of undo entries per process */
	int	semusz;		/* size in bytes of undo structure */
	int	semvmx;		/* semaphore maximum value */
	int	semaem;		/* adjust on exit max value */
};

/*
 * User semaphore template for semop system calls.
 */

struct sembuf {
	ushort_t	sem_num;	/* semaphore # */
	short	sem_op;		/* semaphore operation */
	short	sem_flg;	/* operation flags */
};

#ifdef	_KERNEL
extern int	semconv();
extern void	seminit();
#endif	/* _KERNEL */

#if defined(__STDC__) && !defined(_KERNEL)
int semctl(int, int, int, ...);
int semget(key_t, int, int);
int semop(int, struct sembuf *, unsigned);
#endif

#endif	/* _PROC_IPC_SEM_H */
