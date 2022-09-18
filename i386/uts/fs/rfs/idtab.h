/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_IDTAB_H	/* wrapper symbol for kernel use */
#define _FS_RFS_IDTAB_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/rfs/idtab.h	1.4"
#ident	"$Header: $"
/*
 *
 *    defines for uid/gid translation.
 *
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define MAXSNAME	20
#define CFREE		0
#define CINUSE		1
#define CINTER		2
#define GLOBAL_CH	'.'	/* name of the "global" table	*/
#define UID_DEV		0	/* minor device number for uid device	*/
#define	GID_DEV		1	/* minor device number for gid device	*/
#define UID_MAP		UID_DEV
#define GID_MAP		GID_DEV

struct idtab	{
	uid_t		i_rem;
	uid_t		i_loc;
};
#define i_defval i_rem
#define i_tblsiz i_loc

struct idhead {
	uid_t		i_default;
	uid_t		i_size;
	unsigned long	i_cend;
	unsigned long	i_next;
	unsigned long	i_tries;
	unsigned long	i_hits;
};
#define HEADSIZE \
    ((sizeof(struct idhead) + sizeof(struct idtab) -1) / sizeof(struct idtab))

#ifdef _KERNEL
#define	gluid(a,b)	glid(UID_DEV,a,b)
#define glgid(a,b)	glid(GID_DEV,a,b)
#endif

#endif	/* _FS_RFS_IDTAB_H */
