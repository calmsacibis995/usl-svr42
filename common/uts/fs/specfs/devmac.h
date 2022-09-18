/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SPECFS_DEVMAC_H	/* wrapper symbol for kernel use */
#define	_FS_SPECFS_DEVMAC_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/specfs/devmac.h	1.5.4.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*****************************************************/
/*                                                   */
/*   Device Security Structure used by SPECFS        */	
/*                                                   */
/*****************************************************/

/* 
 * The devmac structure contains the security attributes of a device.
 */

struct devmac {
	ushort	d_relflag;	/* dev release flags */
	ushort	d_pad;		/* pad reserved for future extensions */
	lid_t	d_hilid;	/* maximum level of device */
	lid_t	d_lolid;	/* minimum level of device */
};

/*
 * Macro to get the state of a block or character device special file.
 */
#define	 STATE(sp)	 ((sp)->s_dstate) 

/*
 * Macro to get the mode of a block or character device special file.
 */

#define	 MODE(sp)	 ((sp)->s_dmode)

/*
 * Macro to get the release flag  of a block or character device special file.
 */

#define	 REL_FLAG(sp)	 ((sp)->s_dsecp ? (sp)->s_dsecp->d_relflag : DEV_SYSTEM)

/* 
 * Macro to get the maximum level of a device.
 */

#define	 HI_LEVEL(sp)	 ((sp)->s_dsecp ? (sp)->s_dsecp->d_hilid : 0)

/* 
 * Macro to get the minimum level of a device.
 */

#define	 LO_LEVEL(sp)	 ((sp)->s_dsecp ? (sp)->s_dsecp->d_lolid : 0)

#endif	/* _FS_SPECFS_DEVMAC_H */
