/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_DES_DES_H	/* wrapper symbol for kernel use */
#define _NET_DES_DES_H	/* subject to change without notice */

#ident	"@(#)uts-comm:net/des/des.h	1.3.2.2"
#ident	"$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
*          All rights reserved.
*/ 

/*
 * Generic DES driver interface
 * Keep this file hardware independent!
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define DES_MAXLEN 	65536	/* maximum # of bytes to encrypt  */
#define DES_QUICKLEN	16	/* maximum # of bytes to encrypt quickly */

enum desdir { ENCRYPT, DECRYPT };
enum desmode { CBC, ECB };

/*
 * parameters to ioctl call
 */
struct desparams {
	u_char des_key[8];	/* key (with low bit parity) */
	enum desdir des_dir;	/* direction */
	enum desmode des_mode;	/* mode */
	u_char des_ivec[8];	/* input vector */
	unsigned des_len;	/* number of bytes to crypt */
	union {
		u_char UDES_data[DES_QUICKLEN];
		u_char *UDES_buf;
	} UDES;
#	define des_data UDES.UDES_data	/* direct data here if quick */
#	define des_buf	UDES.UDES_buf	/* otherwise, pointer to data */
};

/*
 * Encrypt an arbitrary sized buffer
 */
#define	DESIOCBLOCK	_IOWR('d', 6, struct desparams)

/* 
 * Encrypt of small amount of data, quickly
 */
#define DESIOCQUICK	_IOWR('d', 7, struct desparams) 

#endif /* _NET_DES_DES_H*/
