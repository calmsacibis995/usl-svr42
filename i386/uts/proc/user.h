/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_USER_H	/* wrapper symbol for kernel use */
#define _PROC_USER_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/user.h	1.12"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h>		/* SVR4.0COMPAT */
#endif

#ifndef _PROC_PROC_H
#include <proc/proc.h>		/* REQUIRED */
#endif

#ifndef _PROC_SIGNAL_H
#include <proc/signal.h>	/* REQUIRED */
#endif

#ifndef _PROC_SIGINFO_H
#include <proc/siginfo.h>	/* REQUIRED */
#endif

#ifndef _UTIL_WEITEK_H
#include <util/weitek.h>	/* REQUIRED */
#endif

#ifndef _SVC_RESOURCE_H
#include <svc/resource.h>	/* REQUIRED */
#endif

#ifndef _PROC_TSS_H
#include <proc/tss.h>		/* REQUIRED */
#endif

#ifndef _PROC_SEG_H
#include <proc/seg.h>		/* REQUIRED */
#endif

#ifndef _MEM_FAULTCATCH_H
#include <mem/faultcatch.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* SVR4.0COMPAT */
#include <sys/proc.h>		/* REQUIRED */
#include <sys/signal.h>		/* REQUIRED */
#include <sys/siginfo.h>	/* REQUIRED */
#include <sys/weitek.h>		/* REQUIRED */
#include <sys/resource.h>	/* REQUIRED */
#include <sys/tss.h>		/* REQUIRED */
#include <sys/seg.h>		/* REQUIRED */
#include <vm/faultcatch.h>	/* REQUIRED */

#else

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/siginfo.h>
#include <sys/weitek.h>
#include <sys/resource.h>
#include <sys/tss.h>
#include <sys/seg.h>
#include <vm/faultcatch.h>

#endif /* _KERNEL_HEADERS */

/*
 * The user structure; one allocated per process.  Contains all the
 * per-process data that doesn't need to be referenced while the
 * process is swapped.
 *
 * The user block is USIZE*click bytes long; resides at virtual kernel
 * address 0xE0000000 on the 386; contains the system stack per user;
 * is cross referenced with the proc structure for the same process.
 *
 *			NOTE  --  NOTE
 *
 * 	WHEN THIS HEADER CHANGES, BOTH ml/misc.s AND ml/ttrap.s MUST BE
 *	   EXAMINED FOR ANY DEPENDENCIES UPON OFFSETS WITHIN THE UBLOCK.
 *	   IN PARTICULAR - .SET's ARE DONE ON:
 *				pr_base,
 *				pr_size,
 *				pr_off, and
 *				pr_scale
 *
 **********************************************************************
 * 
 */

/*
 * User file descriptors are allocate dynamically, in multiples
 * of NFPCHUNK.  WARNING: ml/misc.s:vstart_s knows about the
 * size of struct ufchunk.  If this changes, or in NFPCHUNK is
 * changed, remember to update misc.s.
 */

#define NFPCHUNK 24

struct ufchunk {
	struct file *uf_ofile[NFPCHUNK];
	char uf_pofile[NFPCHUNK];
	struct ufchunk *uf_next;
};

#define UF_FDLOCK	0x2	/* file descriptor locked (SysV-style) */

#define MAXSYSARGS	8	/* Maximum # of arguments passed to a syscall */ 
#define PSARGSZ		80	/* Space for exec arguments (used by ps(1)) */ 

#define	PSCOMSIZ	14	/* space for command name */

#define KSTKSZ		3712

typedef struct {		/* kernel syscall set type */
	long	word[16];
} k_sysset_t;

/* flags for u_sigflag field */
#define SOMASK 		0x001	/* use u_sigoldmask on return from signal */

typedef	struct	user {
	char	u_stack[KSTKSZ];/* kernel stack */
	char	u_stack_filler_1[2];
	/* floating point support variables */
	char    u_fpvalid;              /* flag if saved state is valid     */
	char    u_weitek;               /* flag if process uses weitek chip */
	union {
		struct  fpstate         /* floating point extension state   */
		{
			int     state[27];/* 287/387 saved state           */
			int     status;   /* status word saved at exception */
		} u_fpstate;
		struct	fp_emul
		{
			char	fp_emul[246];  /* (extras for emulator) */
			char	fp_epad[2];
		} fp_emul;
	} u_fps;
	long	u_weitek_reg[33];	/* bits needed to save weitek state */
					/* NOTE: If the WEITEK is actually  */
					/* present, only 32 longs will be   */
					/* used, but if it is not, the      */
					/* emulator will need 33.           */

	/* NOTE: The second page of the uarea must begin here.
	   That is, the offset at this point must be NBPP. */

	struct tss386 *u_tss;	/* pointer to user TSS */
	ushort	u_sztss;	/* size of tss (including bit map) */

	char	u_sigfault;	/* catch general protection violations
				   caused by user modifying his stack
				   where the old state info is kept */
	char	u_usigfailed;	/* allows the user to know that he caused
				   a general protection violation by
				   modifying his register save area used
				   when the user was allowed to do his own
				   signal processing */

	ulong	u_sub;		/* stack upper bound.
				   The address of the first byte of
				   the first page of user stack
				   allocated so far */
#ifdef DEBUG
	long	u_aoutstamp;
	char	u_filler1[4];
#else
     	char	u_filler1[8];
#endif
	
	long	u_386b1;	/* For 386 B1 chip we must set a flag when the 
				   kernel will be writing to user pages so that
				   copy on write faults will be broken */

	/* the addr/size of locked memory for the raw disk async i/o */
	caddr_t	u_raioaddr;
	uint	u_raiosize;

	caddr_t	u_escbug;	/* address of 2nd page of hung ESC instruction;
				   used in workaround for 80386 B1 stepping
				   errata 17 */

	/* Segment descriptors for quick-loading */
	struct seg_desc	u_tss_desc;
	struct seg_desc	u_ldt_desc;

	char	u_filler2[4];

	/*
	 * Number of shared-memory segments currently attached.
	 */
	char	u_nshmseg;

	/*
	 * The following for RFS.  u_srchan is needed because ioctls on
	 * the server can hit RF_SERVER() hooks in copyin and copyout,
	 * and rcopyin/rcopyout need the implicit parameter.
	 */
	ushort		rfs_pad0;
	ushort 		rfs_pad1;
	int 		u_syscall;	/* system call number */
	struct sndd	*u_srchan;	/* server channel back to client */
	long		rfs_pad2;
	ulong		u_userstack;

	long	u_bsize;		/* block size of device */

	struct execsw	*u_execsw;	/* pointer into execsw[] for the
					   current executable */

	int	u_ageinterval;		/* pageing ageing countdown counter */
	char 	u_psargs[PSARGSZ];	/* arguments from exec */	
	char	*u_tracepc;		/* Return PC if tracing enabled */

	int	u_arg[MAXSYSARGS];	/* arguments to current system call */
	label_t	u_qsav;			/* longjmp label for quits and intrs */

	/*
	 * The following (u_segflg and u_error) are obsolete.
	 */
	char	u_segflg;		/* 0=user D, 1=system, 2=user I */
	char	u_error;		/* return error code */

	/*
	 * The following four fields are for backward compatibility
	 * with old device drivers; the actual user credentials are
	 * found through p_cred in struct proc. These fields may
	 * not support valid uid's or gid's when the system is
	 * configured with large user id's.
	 */

	o_uid_t	u_uid;		/* effective user id */
	o_gid_t	u_gid;		/* effective group id */
	o_uid_t	u_ruid;		/* real user id */
	o_gid_t	u_rgid;		/* real group id */

	proc_t *u_procp;		/* pointer to proc structure */

	int	*u_ap;			/* pointer to arglist */

	/*
	 * The following (u_r through u_rablock) are all obsolete.
	 */
	union {
		struct	{
			int	r_v1;
			int	r_v2;
		} r_reg;
		off_t	r_off;
		time_t	r_time;
	} u_r;
	caddr_t	u_base;			/* base address for IO */
	unsigned u_count;		/* bytes remaining for IO */
	off_t	u_offset;		/* offset in file for IO */
	short	u_fmode;		/* file mode for IO */
	ushort	u_pbsize;		/* Bytes in block for IO */
	ushort	u_pboff;		/* offset in block for IO */
	short	u_errcnt;		/* syscall error count */
	daddr_t	u_rablock;		/* read ahead block address */

	int	u_sysabort;		/* Debugging: if set, abort syscall */
	k_sysset_t u_entrymask;		/* syscall stop-on-entry mask */
	k_sysset_t u_exitmask;		/* syscall stop-on-exit mask */

	struct vnode *u_cdir;		/* current directory */

	struct vnode *u_rdir;		/* root directory */

	struct rlimit u_rlimit[RLIM_NLIMITS];     /* resource usage limits */

	uint	u_tsize;		/* text size (clicks) */
	uint	u_dsize;		/* data size (clicks) */
	uint	u_ssize;		/* stack size (clicks) */

	k_siginfo_t u_siginfo;		/* debugger interface: stop-on-fault */

	int	u_systrap;		/* Are any syscall mask bits set? */ 

	int	u_execid;		/* magic number from exec */
	long	u_execsz;		/* total memory requirements (in clicks) */


		/* WARNING: the definitions for u_ttyp and
		** u_ttyd will be deleted at the next major
		** release following SVR4.
		*/

	o_pid_t  *u_ttyp;		/* for binary compatibility only ! */
	o_dev_t  u_ttyd;		/* for binary compatibility only -
					** NODEV will be assigned for large
					** controlling terminal devices.
					*/

	/*
	 * Flag to indicate there is a signal or event pending to
	 * the current process.  Used to make a quick check just
	 * prior to return from kernel to user mode.
	 */
	char	u_sigevpend;


	/*
	 * u_utime, u_stime, u_cutime, u_cstime have been moved to proc table
	 */

	clock_t	u_uservirt;		/* User virtual time */
	clock_t	u_procvirt;		/* Process virtual time */

	int	*u_ar0;			/* address of user's saved R0 */

	int	u_sigflag;		/* per-process signal flags */
	struct ucontext *u_oldcontext;	/* previous user context */
	stack_t u_sigaltstack;		/* sp & on-stack state variable */
	k_sigset_t u_signodefer;	/* signals defered when caught */
	k_sigset_t u_sigonstack;	/* signals taken on alternate stack */
	k_sigset_t u_sigresethand;	/* signals reset when caught */
	k_sigset_t u_sigrestart;	/* signals that restart system calls */
	k_sigset_t u_sigoldmask;	/* for sigsuspend */
	k_sigset_t u_sigmask[MAXSIG];	/* signals held while in catcher */
	void	(*u_signal[MAXSIG])();	/* Disposition of signals */
	void	(*u_sigreturn)();	/* for cleanup (signal/sigset handlers)*/

	struct prof {			/* profile arguments */
		u_short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;

	/* XENIX SUPPORT */

	ulong	u_renv;			/* runtime environment. 	  */
					/*   for meaning of bits:	  */
					/*     0-15  see x_renv (x.out.h) */
					/*    16-23  see x_cpu  (x.out.h) */
					/*    24-31  see below		  */

	/* End XENIX Support */

	/*
	 * Executable file info.
	 */
	struct exdata {
		struct    vnode  *vp;	/* a.out vnode */
		size_t    ux_tsize;	/* text size */
		size_t    ux_dsize;	/* data size */
		size_t    ux_bsize;	/* bss size */
		size_t    ux_lsize;  	/* lib size */
		long      ux_nshlibs; 	/* number of shared libs needed */
		short     ux_mag;   	/* magic number MUST be here */
		off_t     ux_toffset;	/* file offset to raw text */
		off_t     ux_doffset;	/* file offset to raw data */
		off_t     ux_loffset;	/* file offset to lib sctn */
		caddr_t   ux_txtorg;	/* start addr of text in mem */
		caddr_t   ux_datorg;	/* start addr of data in mem */
		caddr_t	  ux_entloc;	/* entry location */
		ulong	  ux_renv;	/* runtime environment */
	} u_exdata;

	char	u_comm[PSCOMSIZ];	/* command name */

	time_t	u_start;		/* start time (in seconds) */
	clock_t	u_ticks;		/* start time (HZ since last boot) */
	long	u_mem;			/* size of process' address space */
	long	u_ior;			/* blocks read */
	long	u_iow;			/* blocks written */
	long	u_iosw;			/* number of swap IO transfers (obsolete) */
	long	u_ioch;			/* number of characters transferred */
	char	u_acflag;		/* flags for accounting */
	mode_t	u_cmask;		/* mask for file creation */

	k_sigset_t	u_oldsig;	/* signals using old-style handlers
					   (i.e. set by signal() or sigset()) */

	/*
	 * Info on how to handle failed memory faults encountered by the kernel.
	 * This replaces u_caddrflt.  See <vm/faultcatch.h>.
	 */
	fault_catch_t	u_fault_catch;

	/*
	 * Some processes (specifically 4.1 and later ELF files) may have
	 * an extra region in the address space from which mmap'd memory
	 * and shared memory may be allocated.  This region would be in
	 * addition to memory above UVSHM.  When allocating mmap'd
	 * or shared memory, the system first looks for an address space
	 * gap in this extra region and then, if that fails, looks for
	 * the gap in the usual place above UVSHM.
	 *
	 * u_shmbase is the base of this extra region and u_shmend
	 * is the end.  If either of these is 0, then there is no such
	 * extra region.
	 */
	caddr_t	u_shmbase, u_shmend;
	/*
	 * For use by the covert channel generic limiter.
	 */
	ushort	u_covchan;
	char	u_filler3[78];		/* unused */

/* Enhanced Application Compatibility Support */
	ulong	u_renv2;		/* holds flags for binary
					   compatibility support */
/* End Enhanced Application Compatibility Support */

	int     u_fpintgate[2];         /* fp intr gate descriptor image   */

	/* i286 emulation variables */
	int    *u_callgatep;            /* pointer to call gate in gdt  */
	int     u_callgate[2];          /* call gate descriptor image   */
	int     u_ldtmodified;          /* if set, LDT was modified     */
	ushort	u_ldtlimit;		/* current size (index) of ldt */

	/* Flag single-step of lcall for a system call. */
	/* The signal is delivered after the system call*/
	char    u_debugpend;            /* SIGTRAP pending for this proc */

	/* debug registers, accessible by ptrace(2) but monitored by kernel */
	char    u_debugon;              /* Debug registers in use, set by kernel */
	int     u_debugreg[8];
	short	u_lock;			/* process/text locking flags */

	int u_nofiles;			/* number of open file slots */
	struct ufchunk u_flist;		/* open file list */
} user_t;

extern struct user u;

/* floating u area support */
#define KERNSTACK	KSTKSZ
struct seguser {
	union {
		char segu_stack[KERNSTACK];
		struct user segu_u;
	} segusr;
};

/* Do u_386b1 flags */

#define	IN_USERWRITE	0x0001		/* kernel is writing to user pages */

/* u_systrap values */

#define P_STOP	0x01	/* /proc tracing is set */
#define	P_PROF	0x02	/* profiling is set     */
#define	P_AUD	0x04	/* auditing is enabled  */
#define	P_CC	0x08	/* covert channel processing required */

#define KUSER(seg)	(&((seg)->segusr.segu_u))

#define	u_rval1	u_r.r_reg.r_v1
#define	u_rval2	u_r.r_reg.r_v2
#define	u_roff	u_r.r_off
#define	u_rtime	u_r.r_time

#define	u_cred	u_procp->p_cred


/* ioflag values: Read/Write, User/Kernel, Ins/Data */

#define	U_WUD	0
#define	U_RUD	1
#define	U_WKD	2
#define	U_RKD	3
#define	U_WUI	4
#define	U_RUI	5

/* defines for Weitek */

#define	WEITEK_CONTEXT	0
#define	WEITEK_CAE	0xFFFFFF00	/* clear accum. exception byte	*/

/* XENIX SUPPORT */

/*
 * defines for bits 24-31 of u_renv and various macros for accessing
 * fields of u_renv.  All bits not currently defined are reserved
 * for future expansion.
 */
#define U_ISCOFF	0x1000000
#define U_ISELF		0x2000000
#define	U_RENVMASK	0x3000000	/* runtime environment bits */

#define	U_CPUTYPE	(XC_CPU << 16)
#define	U_IS386		(XC_386 << 16)
#define	U_ISWSWAP	(XC_WSWAP << 16)

/* binary types */
#define	isCOFF		((u.u_renv & U_RENVMASK) == U_ISCOFF)	/* 386 COFF */
#define	isXOUT		((u.u_renv & U_RENVMASK) == 0)		/* 386 x.out */

#define	IS386()		(((u.u_renv >> 16) & XC_CPU) == XC_386)

/*
 * The following define is used to indicate that the program being
 * exec'd is one of the 286 emulators.  This bit is set in u_renv.
 */
#define	UE_EMUL		0x4000000	/* 286 emulator */
#define	is286EMUL	((u.u_renv & UE_EMUL) == UE_EMUL)

/*
 * Defines for badvise bits of u_renv and various macros for accessing
 * these bits.  
 */

#define UB_PRE_SV	0x8000000	/* badvise pre-System V */
#define UB_XOUT		0x10000000	/* badvise x.out */
#define UB_LOCKING	0x20000000	/* badvise locking() system call  */
					/*      (for kernel use only)     */
#define UB_FCNTL	0x40000000	/* badvise fcntl() system call    */
					/*      (for kernel use only)     */
#define UB_XSDSWTCH	0x80000000	/* badvise XENIX shared data context */
					/*	switching		     */

				/* badvise indicates x.out behavior */
#define BADVISE_XOUT	((u.u_renv & UB_XOUT) == UB_XOUT)

				/* badvise indicates pre-System V behavior */
#define BADVISE_PRE_SV	((u.u_renv & UB_PRE_SV) == UB_PRE_SV)
				/* badvise indicates XENIX locking() call */
#define ISLOCKING	((u.u_renv & UB_LOCKING) == UB_LOCKING)
				/* badvise indicates fcntl() call */
#define ISFCNTL		((u.u_renv & UB_FCNTL) == UB_FCNTL)
				/* badvise indicates XENIX shared data
				 * 	context switching is enabled
				 */
#define BADVISE_XSDSWTCH ((u.u_renv & UB_XSDSWTCH) == UB_XSDSWTCH)
				/* x.out binary or badvise indicates x.out */
#define VIRTUAL_XOUT	(isXOUT || BADVISE_XOUT)

/* End XENIX Support */

/* These macros MUST be defined on architectures that don't provide XENIX Support */

#ifndef BADVISE_PRE_SV
#define BADVISE_PRE_SV 0
#endif

#ifndef ISLOCKING 
#define ISLOCKING 0
#endif

#ifndef ISFCNTL
#define ISFCNTL 0
#endif

#ifndef VIRTUAL_XOUT
#define VIRTUAL_XOUT 0
#endif

/* End of macro definitions disabling XENIX Support	*/

/* Enhanced Application Compatibility Support */

/* Defines for u_renv2 flags */
#define SCO_SHNSL	0x1		/* Uses static shared libnsl */
#define SCO_USES_SHNSL	(u.u_renv2 & SCO_SHNSL)

#define	ISC_POSIX	0x2
#define ISC_USES_POSIX	(u.u_renv2 & ISC_POSIX)

/* End Enhanced Application Compatibility Support */

/* flag values for NFA */
#define NFA_SERVER	0x4000	/* the NFA network server */
#define NFA_CASELESS	0x8000	/* caseless support for DOS */

#define U_COMM(s)	(bcopy((s), u.u_comm,   sizeof(s)), \
			 bcopy((s), u.u_psargs, sizeof(s)))

#ifdef _KERNEL

#if defined(__STDC__)
extern void addupc(void(*)(), int);
#else
extern void addupc();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _PROC_USER_H */
