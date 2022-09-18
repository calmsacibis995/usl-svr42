/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_KSYM_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_KSYM_H	/* subject to change without notice */

#ident	"@(#)uts-comm:util/mod/ksym.h	1.3"
#ident	"$Header: $"

/* info for ioctl on /dev/kmem (driver mm) */
#define MIOC_READKSYM	(('M'<<8)|4)	/* ioctl command to read kernel space
					based on symbol name */
#define MIOC_IREADKSYM 	(('M'<<8)|5)	/* treat symbol as pointer to object to be read */

#define MIOC_WRITEKSYM  (('M'<<8)|6)	/* ioctl to write to kernel space based on symbol name */

struct mioc_rksym {
	char *mirk_symname;	/* symbol at whose address read will start */
	void *mirk_buf;		/* buffer into which data will be written */
	size_t mirk_buflen;	/* length of read buffer */
};


/* info for getksym */
#define MAXSYMNMLEN	1024	/* max number of characters (incl. '\0') in
				   symbol name returned through getksym */
#ifndef _KERNEL
extern int getksym(char *, unsigned long *, unsigned long *);
#endif /* ! _KERNEL */

#endif /* _UTIL_MOD_KSYM_H */
