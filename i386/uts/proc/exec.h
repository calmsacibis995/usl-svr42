/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_EXEC_H	/* wrapper symbol for kernel use */
#define _PROC_EXEC_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/exec.h	1.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h> /* REQUIRED */
#endif

#ifndef _IO_UIO_H
#include <io/uio.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h> /* REQUIRED */
#include <sys/vnode.h> /* REQUIRED */
#include <sys/uio.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define getexmag(x)   (x[1] << 8) + x[0]

/*
 * User argument structure for stack image management
 */

struct uarg {
	caddr_t estkstart;	/* start address of new process's userstack */
	int estksize;		/* size of new process' userstack */
	u_int estkhflag;	/* flags for userstack allocation */
	int stringsize;		/* size of string */
	int argsize;		/* size of command argument array */
	int envsize;		/* size of environ variable array */
	int argc;		/* number of command arguments */
	int envc;		/* number of environment variables */
	int prefixc;		/* intp arg prefix invisible to psargs */
	int prefixsize;		/* number of chars in intp prefix */
	caddr_t *prefixp;	/* pointer to the intp prefix string */
	int auxsize;		/* info on stack for dynamic linker */
	addr_t stacklow;	/* address of userstack */
	caddr_t stackend;	/* size of userstack */
	char **argp;		/* pointer to argument array */
	char **envp;		/* pointer to environ variables array */
	char *fname;		/* pointer to name of executable file */
	uio_seg_t fnameseg;	/* UIO_USERSPACE or UIO_SYSSPACE */
	int traceinval;		/* flag to invalidate process tracing */
	caddr_t auxaddr;
	caddr_t argaddr;
	int flags;
};

typedef struct execenv {
	caddr_t ex_brkbase;	/* base of the highest writable segment */
	short   ex_magic;	/* magic number */
	vnode_t *ex_vp;		/* pointer to vnode for executable file */
} execenv_t; 

/* flags definition */

#define	  RINTP	  0x1 /* A run-time interpreter is active */
#define	  EMULA	  0x2 /* Invoking emulator */

struct execsw {
	short *exec_magic;	/* magic number */
	int   (*exec_func)();	/* object file specific exec function */
	int   (*exec_core)();	/* object file specific core function */
};

extern int nexectype;		/* number of elements in execsw */
extern struct execsw execsw[];

typedef struct exhdmap {
	struct exhdmap	*nextmap;	/* pointer to next map on maplist */
	off_t		curbase;	/* current base address */
	off_t		curoff;		/* current offset address */
	int		cureoff;	/* current end of offset */
	caddr_t		bndrycasep;	/* pointer to the boundary memory */
	long		bndrycasesz;	/* size of boundary memory */
	struct fbuf	*fbufp;		/* pointer to file buffer structure */
	int		keepcnt;	/* flag to keep file in map list */
} exhdmap_t;

typedef struct exhda {
	vnode_t	*vp;			/* pointer to file vnode */
	u_long	vnsize;			/* size of vnode pointer */
	exhdmap_t	*maplist;	/* pointer to map list */
	int state;			/* used to indicate an error */
	int nomap;			/* mappability of vnode */
} exhda_t;

#define EXHDA_HADERROR	1

#ifdef _KERNEL

#if defined(__STDC__)

#ifdef _KERNEL_HEADERS

#ifndef _SVC_RESOURCE_H
#include <svc/resource.h>	/* REQUIRED */
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>	/* REQUIRED */
#endif

#ifndef _PROC_PROC_H
#include <proc/proc.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/resource.h>	/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/proc.h>		/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

extern int exhd_getmap(exhda_t *, off_t, int, int, char *);
extern void exhd_release(exhda_t *);
extern int remove_proc(struct uarg *);
extern int execmap(vnode_t *, caddr_t, size_t, size_t, off_t, int);
extern void setexecenv(struct execenv *);
extern int setregs(struct uarg *);
extern int core_seg(proc_t *, vnode_t *, off_t, caddr_t, size_t, rlim_t, cred_t *);
extern int gexec(vnode_t **, struct uarg *, int, long *);
extern caddr_t execstk_addr(int, u_int *);
extern int execpermissions(struct vnode *, struct vattr *, exhda_t *, struct uarg *);

#else

extern int exhd_getmap();
extern void exechd_release();
extern int execmap();
extern int remove_proc();
extern void setexecenv();
extern int setregs();
extern int core_seg();
extern int gexec();
extern caddr_t execstk_addr();
extern int execpermissions();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

/* flags for exhd_getmap(): */
#define	EXHD_NOALIGN	0
#define EXHD_4BALIGN	1	/* align on 4 byte boundary */
#define EXHD_KEEPMAP	2	/* keep for parallel use with other maps */
				/* if not set, map will be freed
				 * automatically on next getmap
				 */
#define EXHD_COPY	4	/* Copy to the provided address */

/* the following macro is a machine dependent encapsulation of
 * postfix processing to hide the stack direction from elf.c
 * thereby making the elf.c code machine independent.
 */

#define execpoststack(ARGS, ARRAYADDR, BYTESIZE)  \
	(copyout((caddr_t)(ARRAYADDR), (ARGS)->auxaddr, BYTESIZE) ? EFAULT \
		: (((ARGS)->auxaddr += (BYTESIZE)), 0))

#endif /* _PROC_EXEC_H */
