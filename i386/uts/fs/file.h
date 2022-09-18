/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FILE_H	/* wrapper symbol for kernel use */
#define _FS_FILE_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/file.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * One file structure is allocated for each open/creat/pipe call.
 * Main use is to hold the read/write pointer associated with
 * each open file.
 *
 * Fields f_aiof and f_aiob were changed from struct aioreq (which was defined
 * in asyncsys.h which was removed since asynchronous io is not supported)
 * to caddr_t.  The reason they were changed to caddr_t types is that the
 * crash command references these fields.  f_aiof and f_aiob should be removed
 * in the next major release when crash command can be modified.
 */
typedef struct file {
	struct file  *f_next;		/* pointer to next entry */
	struct file  *f_prev;		/* pointer to previous entry */
	ushort	f_flag;
	cnt_t	f_count;		/* reference count */
	struct vnode *f_vnode;		/* pointer to vnode structure */
	off_t	f_offset;		/* read/write character pointer */
	struct	cred *f_cred;		/* credentials of user who opened it */
	caddr_t *f_aiof;		/* aio file list forward link	*/
	caddr_t *f_aiob;		/* aio file list backward link	*/
/* XENIX Support */
	struct	file *f_slnk;		/* XENIX semaphore queue */
/* End XENIX Support */
} file_t;

/* flags */

#define	FOPEN		0xFFFFFFFF
#define	FREAD		0x01
#define	FWRITE		0x02
#define	FNDELAY		0x04
#define	FAPPEND		0x08
#define	FSYNC		0x10
#define	FNONBLOCK	0x80

#define	FMASK		0xFF	/* should be disjoint from FASYNC */

/* open-only modes */

#define	FCREAT		0x0100
#define	FTRUNC		0x0200
#define	FEXCL		0x0400
#define	FNOCTTY		0x0800
#define FASYNC		0x1000

/*
 * This mode is a kludge to allow pre-SVR4 RFS servers to survive opens
 * of namefs-mounted files.  The SVR4 implmentation of the old protocol
 * expects a lookup to precede every open, but namefs departs from that
 * model.  We provide the mode to allow the client to detect and fail
 * the open, thereby protecting server reference counts.  This mode
 * will disappear when support for the old RFS protocol is dropped.
 */
#define FNMFS		0x2000

/* file descriptor flags */
#define FCLOSEXEC	001	/* close on exec */

/* miscellaneous defines */

#define NULLFP ((struct file *)0)

#ifndef L_SET
#define	L_SET	0	/* for lseek */
#endif /* L_SET */

/*
 * Routines dealing with user per-open file flags and
 * user open files.  
 */

#if defined(__STDC__)
extern int getf(int, file_t **);
extern void closeall(int);
extern int closef(file_t *);
extern int ufalloc(int, int *);
extern int falloc(struct vnode *, int, file_t **, int *);
extern void finit(void);
extern void unfalloc(file_t *);
extern void setf(int, file_t *);
extern char getpof(int);
extern void setpof(int, char);
extern int fassign(struct vnode **, int, int*);

#else

extern int getf();
extern void closeall();
extern int closef();
extern int ufalloc();
extern int falloc();
extern void finit();
extern void unfalloc();
extern void setf();
extern char getpof();
extern void setpof();
extern int fassign();

#endif	/* __STDC__ */

#endif	/* _FS_FILE_H */
