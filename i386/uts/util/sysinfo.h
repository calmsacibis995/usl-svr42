/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_SYSINFO_H	/* wrapper symbol for kernel use */
#define _UTIL_SYSINFO_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/sysinfo.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

/*
 * Values to index into sysinfo.cpu[]
 */
#define	CPU_IDLE	0	/* ticks that cpu was idle not waiting on I/O */
#define	CPU_USER	1	/* ticks that cpu was in user mode */
#define	CPU_KERNEL	2 	/* ticks that cpu was in kernel mode */
#define	CPU_WAIT	3	/* ticks that cpu was idle, waiting on I/O */
#define CPU_SXBRK	4
#define CPU_STATES	5	/* number of CPU states */

/*
 * Values to index into sysinfo.wait[]
 */
#define	W_IO		0	/* ticks that cpu has waited for block I/O */
#define	W_SWAP		1	/* ticks that cpu was idle, waiting for swapping */
#define	W_PIO		2	/* ticks that cpu has waited for physical I/O */
#define WAITON		3	/* number of idle states */

/* 
 * System information.
 */
struct sysinfo {
	time_t	cpu[CPU_STATES];
	time_t	wait[WAITON];
	long	bread;		/* blocks read from disk to buffer cache */
	long	bwrite;		/* blocks written to disk from buffer cache */
	long	lread;		/* logical reads requested */
	long	lwrite;		/* logical writes done */
	long	phread;		/* physical reads done */
	long	phwrite;	/* physical writes done */
	long	swapin;		/* obsolete */
	long	swapout;	/* obsolete */
	long	bswapin;	/* obsolete */
	long	bswapout;	/* obsolete */
	long	pswitch;	/* processes switched */
	long	syscall;	/* # of system calls made */
	long	sysread;	/* # of read system calls made */
	long	syswrite;	/* # of write system calls made */
	long	sysfork;	/* # of fork system calls made */
	long	sysexec;	/* # of exec system calls made */
	long	runque;		/* cumulative counter of the size of the run queue every second */
	long	runocc;		/* # of seconds that the run queue was occupied */
	long	swpque;		/* cumulative counter of the size of the swap queue every second */
	long	swpocc;		/* # of seconds that the swap queue was occupied */
	long	iget;		/* igets done */
	long	namei;		/* # of pathname lookups done */
	long	dirblk;		/* # of directory blocks read */
	ulong	readch;		/* characters read */
	ulong	writech;	/* characters written */
	long	rcvint;		/* # of receive type interrupts from console, ports, and hiports boards */
	long	xmtint;		/* # of transmit type interrupts from console, ports, and hiports boards */
	long	mdmint;		/* # of modem type interrupts from console, ports, and hiports boards */
	long	rawch;		/* raw characters read from console & terminals */
	long	canch;		/* # of canonical characters read from terminals*/
	long	outch;		/* # of output characters sent to terminals */
	long	msg;		/* ipc messages sent and received */
	long	sema;		/* semaphore operation sys. calls */
	long	pnpfault;	/* obsolete */
	long	wrtfault;	/* obsolete */
	long	s5ipage;	/* s5 inodes taken off freelist that did not have reusable pages attached to them */
	long	s5inopage;	/* s5 inodes taken off freelist that have reusable pages attached to them */
	long	ufsipage;	/* ufs inodes taken off freelist that have reusable pages attached to them */
	long	ufsinopage;	/* ufs inodes taken off freelist that did not have reusable pages attached to them */
	long	sfsipage;	/* sfs inodes taken off freelist that have reusable pages attached to them */
	long	sfsinopage;	/* sfs inodes taken off freelist that did not have reusable pages attached to them */
	long	xxipage;
	long	xxinopage;	
	long	vxfsipage;	/* VxFS inodes taken off freelist that have
				 * reusable pages attached to them */
	long	vxfsinopage;	/* VsFS inodes taken off freelist that did
				 * not have reusable pages attached to them */
};

/* 
 * Processes waiting on I/O.
 */
struct syswait {
	short	iowait;		/* # of processes that are asleep, waiting for buffered I/O */
	short	swap;		/* obsolete */
	short	physio;		/* # of processes waiting for raw I/O */
};

/* 
 * Physical memory information.
 */
struct minfo {
#ifdef _KERNEL
	unsigned long 	mi_freemem[2]; 	/* free memory in pages at every tick */
					/* "double" long format	*/
					/* mi_freemem[0] least significant */
#else
	unsigned long 	freemem[2]; 	/* compatibility name */
#endif /* _KERNEL */
	long	freeswap;	/* free swap space */
	long	vfault;		/* translation fault */
	long	demand;		/* demand zero and demand fill pages */
	long	swap;		/* pages on swap */
	long	cache;		/* pages in cache */
	long	file;		/* pages on file */
	long	pfault;		/* protection fault */
	long	cw;		/* copy on write */
	long	steal;		/* steal the page */
	long	freedpgs;	/* pages are freed */
	long	vfpg; 		/* pages are freed by vhand */
	long	sfpg;		/* pages are freed by sched */
	long	vspg;		/* pages are freed/swapped by vhand */
	long	sspg;		/* pages are freed/swapped by sched */
	long	unmodsw;	/* getpages finds unmodified pages on swap */
	long	unmodfl;	/* getpages finds unmodified pages in file */ 
	long	psoutok;	/* swapping out a process */
	long	psinfail;	/* swapping in a process failed */
	long	psinok;		/* swapping in a process succeeded */
	long	rsout;		/* swapping out a region */
	long	rsin;		/* swapping in a region */
};

/* 
 * File system information.
 */
typedef struct fsinfo {
	ulong fsireadch;	/* characters read by vnode operations */
	ulong fsiwritech;	/* characters written by vnode operations */
	ulong fsivop_open;	/* incremented on open vnode operation */
	ulong fsivop_close;	/* incremented on close vnode operation */
	ulong fsivop_read;	/* incremented on read vnode operation */
	ulong fsivop_write;	/* incremented on write vnode operation */
	ulong fsivop_lookup;	/* incremented on lookup vnode operation */
	ulong fsivop_create;	/* incremented on create vnode operation */
	ulong fsivop_readdir;	/* incremented on readdir vnode operation */
	ulong fsivop_getpage;	/* incremented on getpage vnode operation */
	ulong fsivop_putpage;	/* incremented on putpage vnode operation */
	ulong fsivop_other;	/* incremented on any vnode operation other than the above */
} fsinfo_t;

/* 
 * Record virtual memory information.
 */
struct vminfo {
	ulong	v_pgrec;	/* # of pages reclaimed from freelist */
	ulong	v_xsfrec;	/* # of pages reclaimed from freelist rather than retrieved from swap device */
	ulong	v_xifrec;	/* # of pages reclaimed from freelist rather than retrieved from the originating file */
	ulong	v_pgin;		/* # of page-ins done */
	ulong	v_pgpgin;	/* # of pages paged-in */
	ulong	v_pgout;	/* # of page-outs done */
	ulong	v_pgpgout;	/* # of pages paged-out */
	ulong	v_swpout;	/* # of processes swapped out */
	ulong	v_pswpout;	/* # of pages swapped out */
	ulong	v_swpin;	/* # of processes swapped in */
	ulong	v_pswpin;	/* # of pages swapped in */
	ulong	v_dfree;	/* pages freed by the page stealing daemon */
	ulong	v_scan;		/* # of page examinations done by the pageout daemon */
	ulong	v_pfault;	/* # of protection faults */
	ulong	v_vfault;	/* # of validity faults */
	ulong	v_sftlock;	/* # of softlock faults */
};

/*
 * Record system error statistics.
 */
struct syserr {
	long	inodeovf;	/* # of times an inode could not be found on the inode freelist */
	long	fileovf;	/* # of times that file table structures could not be allocated from KMA */
	long	textovf;	
	long	procovf;	/* # of times a proc table entry could not be given */
};

/*
 * Record information on shared libraries.
 */
struct shlbinfo {
	long	shlbs;		/* Max # of libs a process can link in	*/
				/*   at one time.			*/
	long	shlblnks;	/* # of times processes that have used	*/
				/*   static shared libraries.		*/
	long	shlbovf;	/* # of processes needed more shlibs	*/
				/*   than the system imposed limit.	*/
	long	shlbatts;	/* # of times processes have attached	*/
				/*   run time libraries.		*/
};

/* 
 * Record information on co-processor.
 */
struct bpbinfo {
	long	usr;		/* usr time for the co-processor */
	long	sys;		/* system time for the co-processor */
	long	idle;		/* idle time for the co-processor */
	long	syscall;	/* # of system calls on the */
				/*	co-processor */
};

/*
 * Values to index into kmeminfo.km_mem[], etc.
 */
#define KMEM_SMALL	0	/* small KMEM request index */
#define KMEM_LARGE	1	/* large KMEM request index */
#define KMEM_OSIZE	2	/* outsize KMEM request index */
#define KMEM_NCLASS	3	/* number of KMEM request classes */

/* 
 * Record kernel memory usage.
 */
struct kmeminfo {
	ulong	km_mem[KMEM_NCLASS];	/* amount of memory owned by KMEM */
	ulong	km_alloc[KMEM_NCLASS];	/* amount of memory allocated */
	ulong	km_fail[KMEM_NCLASS];	/* # of failed requests */
};

#ifdef _KERNEL
extern struct sysinfo	sysinfo;
extern struct syswait	syswait;
extern struct minfo	minfo;
extern struct vminfo	vminfo;
extern struct syserr	syserr;
extern struct shlbinfo	shlbinfo;
extern struct bpbinfo	bpbinfo[];
#endif

#endif	/* _UTIL_SYSINFO_H */
