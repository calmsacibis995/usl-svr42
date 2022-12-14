/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FILIO_H	/* wrapper symbol for kernel use */
#define _FS_FILIO_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/filio.h	1.5"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * General file ioctl definitions.
 */

#ifdef _KERNEL_HEADERS

#ifndef _FS_IOCCOM_H
#include <fs/ioccom.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/ioccom.h>	/* REQUIRED */

#else

#include <sys/ioccom.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define	FIOCLEX		_IO('f', 1)		/* set exclusive use on fd */
#define	FIONCLEX	_IO('f', 2)		/* remove exclusive use */
/* another local */
#define	FIONREAD	_IOR('f', 127, int)	/* get # bytes to read */
#define	FIONBIO		_IOW('f', 126, int)	/* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW('f', 125, int)	/* set/clear async i/o */
#define	FIOSETOWN	_IOW('f', 124, int)	/* set owner */
#define	FIOGETOWN	_IOR('f', 123, int)	/* get owner */

#endif	/* _FS_FILIO_H */
