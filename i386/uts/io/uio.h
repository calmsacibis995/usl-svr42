/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_UIO_H	/* wrapper symbol for kernel use */
#define _IO_UIO_H	/* subject to change without notice */

#ident	"@(#)uts-x86:io/uio.h	1.7"
#ident	"$Header: $"

/*
 * I/O parameter information.  A uio structure describes the I/O which
 * is to be performed by an operation.  Typically the data movement will
 * be performed by a routine such as uiomove(), which updates the uio
 * structure to reflect what was done.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif  /* _UTIL_TYPES_H */

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct iovec {
	caddr_t	iov_base;	/* base address of data */
	int	iov_len;	/* length of data (in bytes) */
} iovec_t;

typedef struct uio {
	iovec_t	*uio_iov;	/* pointer to array of iovecs */
	int	uio_iovcnt;	/* number of iovecs */
	off_t	uio_offset;	/* file offset */
	short	uio_segflg;	/* address space (kernel or user) */
	short	uio_fmode;	/* file mode flags */
	daddr_t	uio_limit;	/* u-limit (maximum "block" offset) */
	int	uio_resid;	/* residual count; initially set to the */
				/* length of data; contains the number of */
				/* bytes not read or written after the I/O */
				/* operation has completed/terminated. */
} uio_t;

#ifndef  _KERNEL 

#ifdef __STDC__

int readv(int, struct iovec *, int);
int writev(int, const struct iovec *, int);

#else   /*__STDC__ */

int readv();
int writev();

#endif   /*__STDC__ */

#endif /* _KERNEL */


/*
 * Indicates the I/O direction. UIO_READ indicates a read into the uio
 * structure's data buffer from somewhere (specified by the I/O function) and
 * UIO_WRITE indicates contents of the uio structure's data buffer is written
 * somewhere.
 */
typedef enum uio_rw { UIO_READ, UIO_WRITE } uio_rw_t;

/*
 * Segment flag, uio_segflg, values, which indicate the origin of the base
 * address, iov_base, of the data in the uio structure.
 */
typedef enum uio_seg { UIO_USERSPACE, UIO_SYSSPACE, UIO_USERISPACE } uio_seg_t;

int	uiomove();
int	ureadc();
int	uwritec();
int	uiomvuio();
void	uioskip();
void	uioupdate();

#endif	/* _IO_UIO_H */
