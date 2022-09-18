/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_PROCFS_PROCFS_H	/* wrapper symbol for kernel use */
#define _FS_PROCFS_PROCFS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/procfs/procfs.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>		/* REQUIRED */
#endif

#ifndef _PROC_REGSET_H
#include <proc/regset.h>	/* REQUIRED */
#endif

#ifndef _PROC_SIGNAL_H
#include <proc/signal.h>	/* REQUIRED */
#endif

#ifndef _PROC_SIGINFO_H
#include <proc/siginfo.h>	/* REQUIRED */
#endif

#ifndef _UTIL_FAULT_H
#include <util/fault.h>		/* REQUIRED */
#endif

#ifndef _PROC_UCONTEXT_H
#include <proc/ucontext.h>	/* SVR4.0COMPAT */
#endif

#ifndef _SVC_SYSCALL_H
#include <svc/syscall.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/regset.h>		/* REQUIRED */
#include <sys/signal.h>		/* REQUIRED */
#include <sys/siginfo.h>	/* REQUIRED */
#include <sys/fault.h>		/* REQUIRED */
#include <sys/ucontext.h>	/* SVR4.0COMPAT */
#include <sys/syscall.h>	/* SVR4.0COMPAT */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/time.h>		/* SVR4.0COMPAT */
#include <sys/regset.h>		/* SVR4.0COMPAT */
#include <sys/signal.h>		/* SVR4.0COMPAT */
#include <sys/siginfo.h>	/* SVR4.0COMPAT */
#include <sys/ucontext.h>	/* SVR4.0COMPAT */
#include <sys/fault.h>		/* SVR4.0COMPAT */
#include <sys/syscall.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * The ioctl codes and system call interfaces for /proc.
 */

#define	PIOC		('q'<<8)
#define	PIOCSTATUS	(PIOC|1)	/* get process status */
#define	PIOCSTOP	(PIOC|2)	/* post STOP request and... */
#define	PIOCWSTOP	(PIOC|3)	/* wait for process to STOP */
#define	PIOCRUN		(PIOC|4)	/* make process runnable */
#define	PIOCGTRACE	(PIOC|5)	/* get traced signal set */
#define	PIOCSTRACE	(PIOC|6)	/* set traced signal set */
#define	PIOCSSIG	(PIOC|7)	/* set current signal */
#define	PIOCKILL	(PIOC|8)	/* send signal */
#define	PIOCUNKILL	(PIOC|9)	/* delete a signal */
#define	PIOCGHOLD	(PIOC|10)	/* get held signal set */
#define	PIOCSHOLD	(PIOC|11)	/* set held signal set */
#define	PIOCMAXSIG	(PIOC|12)	/* get max signal number */
#define	PIOCACTION	(PIOC|13)	/* get signal action structs */
#define	PIOCGFAULT	(PIOC|14)	/* get traced fault set */
#define	PIOCSFAULT	(PIOC|15)	/* set traced fault set */
#define	PIOCCFAULT	(PIOC|16)	/* clear current fault */
#define	PIOCGENTRY	(PIOC|17)	/* get syscall entry set */
#define	PIOCSENTRY	(PIOC|18)	/* set syscall entry set */
#define	PIOCGEXIT	(PIOC|19)	/* get syscall exit set */
#define	PIOCSEXIT	(PIOC|20)	/* set syscall exit set */
#define	PIOCSFORK	(PIOC|21)	/* set inherit-on-fork flag */
#define	PIOCRFORK	(PIOC|22)	/* reset inherit-on-fork flag */
#define	PIOCSRLC	(PIOC|23)	/* set run-on-last-close flag */
#define	PIOCRRLC	(PIOC|24)	/* reset run-on-last-close flag */
#define	PIOCGREG	(PIOC|25)	/* get general registers */
#define	PIOCSREG	(PIOC|26)	/* set general registers */
#define	PIOCGFPREG	(PIOC|27)	/* get floating-point registers */
#define	PIOCSFPREG	(PIOC|28)	/* set floating-point registers */
#define	PIOCNICE	(PIOC|29)	/* set nice priority */
#define	PIOCPSINFO	(PIOC|30)	/* get ps(1) information */
#define	PIOCNMAP	(PIOC|31)	/* get number of memory mappings */
#define	PIOCMAP		(PIOC|32)	/* get memory map information */
#define	PIOCOPENM	(PIOC|33)	/* open mapped object for reading */
#define	PIOCCRED	(PIOC|34)	/* get process credentials */
#define	PIOCGROUPS	(PIOC|35)	/* get supplementary groups */
#define	PIOCGETPR	(PIOC|36)	/* read struct proc */
#define	PIOCGETU	(PIOC|37)	/* read user area */
#define PIOCGDBREG	(PIOC|40)	/* get debug registers */
#define PIOCSDBREG	(PIOC|41)	/* set debug registers */

/* holds one 3B2 instruction op code */

typedef	char	instr_t;

/* process status structure */

typedef struct prstatus {
	long	pr_flags;	/* process flags */
	short	pr_why;		/* reason for process stop (if stopped) */
	short	pr_what;	/* more detailed reason */
	siginfo_t pr_info;	/* info associated with signal or fault */
	short	pr_cursig;	/* current signal */
	short	pr_pad;		/* pad to long boundary */
	sigset_t pr_sigpend;	/* set of other pending signals */
	sigset_t pr_sighold;	/* set of held signals */
	struct	sigaltstack pr_altstack; /* alternate signal stack info */
	struct	sigaction pr_action; /* signal action for current signal */
	pid_t	pr_pid;		/* process id */
	pid_t	pr_ppid;	/* parent process id */
	pid_t	pr_pgrp;	/* process group id */
	pid_t	pr_sid;		/* session id */
	timestruc_t pr_utime;	/* process user cpu time */
	timestruc_t pr_stime;	/* process system cpu time */
	timestruc_t pr_cutime;	/* sum of children's user times */
	timestruc_t pr_cstime;	/* sum of children's system times */
	char	pr_clname[8];	/* scheduling class name */
	long	pr_filler[20];	/* filler area for future expansion */
	long	pr_instr;	/* current instruction */
	gregset_t pr_reg;	/* general registers */
} prstatus_t;

/* process status flags */

#define	PR_STOPPED	0x0001	/* process is stopped */
#define	PR_ISTOP	0x0002	/* process stopped on an event of interest */
#define	PR_DSTOP	0x0004	/* a stop directive is in effect */
#define	PR_ASLEEP	0x0008	/* process is sleep()ing in a system call */
#define	PR_FORK		0x0010	/* inherit-on-fork is in effect */
#define	PR_RLC		0x0020	/* run-on-last-close is in effect */
#define	PR_PTRACE	0x0040	/* process is being controlled by ptrace(2) */
#define	PR_PCINVAL	0x0080	/* %pc refers to an invalid virtual address */
#define	PR_ISSYS	0x0100	/* system process */

/* reasons for stopping */

#define	PR_REQUESTED	1
#define	PR_SIGNALLED	2
#define	PR_SYSENTRY	3
#define	PR_SYSEXIT	4
#define	PR_JOBCONTROL	5
#define	PR_FAULTED	6

/* information for the ps(1) command */

#define	PRARGSZ		80		/* number of chars of arguments */

typedef struct prpsinfo {
	char	pr_state;	/* numeric process state (see pr_sname) */
	char	pr_sname;	/* printable character representing pr_state */
	char	pr_zomb;	/* !=0: process terminated but not waited for */
	char	pr_nice;	/* nice for cpu usage */
	u_long	pr_flag;	/* process flags */
	uid_t	pr_uid;		/* real user id */
	gid_t	pr_gid;		/* real group id */
	pid_t	pr_pid;		/* unique process id */
	pid_t	pr_ppid;	/* process id of parent */
	pid_t	pr_pgrp;	/* pid of process group leader */
	pid_t	pr_sid;		/* session id */
	caddr_t	pr_addr;	/* physical address of process */
	long	pr_size;	/* size of process image in pages */
	long	pr_rssize;	/* resident set size in pages */
	caddr_t	pr_wchan;	/* wait addr for sleeping process */
	timestruc_t pr_start;	/* process start time, sec+nsec since epoch */
	timestruc_t pr_time;	/* usr+sys cpu time for this process */
	long	pr_pri;		/* priority, high value is high priority */
	char	pr_oldpri;	/* pre-SVR4, low value is high priority */
	char	pr_cpu;		/* pre-SVR4, cpu usage for scheduling */
	o_dev_t	pr_ottydev;	/* short tty device number */
	dev_t	pr_lttydev;	/* controlling tty device (PRNODEV if none) */
	char	pr_clname[8];	/* scheduling class name */
	char	pr_fname[16];	/* last component of exec()ed pathname */
	char	pr_psargs[PRARGSZ];	/* initial characters of arg list */
	long	pr_filler[20];	/* for future expansion */
} prpsinfo_t;

#if !defined(_STYPES)
#define	pr_ttydev	pr_lttydev
#else
#define	pr_ttydev	pr_ottydev
#endif	

#define	PRNODEV	(dev_t)(-1)	/* non-existent device */

/* optional actions to take when process continues */

typedef struct prrun {
	long	pr_flags;	/* flags */
	sigset_t pr_trace;	/* set of signals to be traced */
	sigset_t pr_sighold;	/* set of signals to be held */
	fltset_t pr_fault;	/* set of faults to be traced */
	caddr_t	pr_vaddr;	/* virtual address at which to resume */
	long	pr_filler[8];	/* filler area for future expansion */
} prrun_t;

#define	PRCSIG		0x001	/* clear current signal */
#define	PRCFAULT	0x002	/* clear current fault */
#define	PRSTRACE	0x004	/* use traced-signal set in pr_trace */
#define	PRSHOLD		0x008	/* use held-signal set in pr_sighold */
#define	PRSFAULT	0x010	/* use traced-fault set in pr_fault */
#define	PRSVADDR	0x020	/* resume at virtual address in pr_vaddr */
#define	PRSTEP		0x040	/* single-step the process */
#define	PRSABORT	0x080	/* abort syscall */
#define	PRSTOP		0x100	/* set directed stop request */

/* memory-management interface */

typedef struct prmap {
	caddr_t		pr_vaddr;	/* virtual address base */
	u_long		pr_size;	/* size of mapping in bytes */
	off_t		pr_off;		/* offset into mapped object, if any */
	long		pr_mflags;	/* protection and attribute flags */
	long		pr_filler[4];	/* filler for future expansion */
} prmap_t;

/* protection and attribute flags */

#define	MA_READ		0x04	/* readable by the traced process */
#define	MA_WRITE	0x02	/* writable by the traced process */
#define	MA_EXEC		0x01	/* executable by the traced process */
#define	MA_SHARED	0x08	/* changes are shared by mapped object */
#define	MA_BREAK	0x10	/* grown by brk(2) */
#define	MA_STACK	0x20	/* grown automatically on stack faults */

/* process credentials */

typedef struct prcred {
	uid_t	pr_euid;	/* effective user id */
	uid_t	pr_ruid;	/* real user id */
	uid_t	pr_suid;	/* saved user id (from exec) */
	gid_t	pr_egid;	/* effective group id */
	gid_t	pr_rgid;	/* real group id */
	gid_t	pr_sgid;	/* saved group id (from exec) */
	u_int	pr_ngroups;	/* number of supplementary groups */
} prcred_t;

/*
 * Macros for manipulating sets of flags.
 * sp must be a pointer to one of sigset_t, fltset_t, or sysset_t.
 * flag must be a member of the enumeration corresponding to *sp.
 */

/* turn on all flags in set */
#define	prfillset(sp) {						\
	register int _i_ = sizeof(*(sp))/sizeof(u_long);	\
	while(_i_)						\
		((u_long *)(sp))[--_i_] = (u_long)~0;		\
}

/* turn off all flags in set */
#define	premptyset(sp) {					\
	register int _i_ = sizeof(*(sp))/sizeof(u_long);	\
	while(_i_)						\
		((u_long *)(sp))[--_i_] = 0;			\
}

/* turn on specified flag in set */
#define	praddset(sp, flag) \
	(((unsigned)((flag)-1) < NSIG*sizeof(*(sp))/sizeof(u_long)) ? \
	(((u_long*)(sp))[((flag)-1)/NSIG] |= ((u_long)1<<(((flag)-1)%NSIG))) : 0)

/* turn off specified flag in set */
#define	prdelset(sp, flag) \
	(((unsigned)((flag)-1) < NSIG*sizeof(*(sp))/sizeof(u_long)) ? \
	(((u_long*)(sp))[((flag)-1)/NSIG] &= ~((u_long)1<<(((flag)-1)%NSIG))) : 0)

/* query: != 0 iff flag is turned on in set */
#define	prismember(sp, flag) \
	(((unsigned)((flag)-1) < NSIG*sizeof(*(sp))/sizeof(u_long)) \
	&& (((u_long*)(sp))[((flag)-1)/NSIG] & ((u_long)1<<(((flag)-1)%NSIG))))


#endif	/* _FS_PROCFS_PROCFS_H */
