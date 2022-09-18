/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_RF_AUTH_H
#define _FS_RFS_RF_AUTH_H

#ident	"@(#)uts-comm:fs/rfs/rf_auth.h	1.5.2.2"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

/*
 * Exported interfaces of RFS id mapping.
 */

extern uid_t	glid();
extern void	vattr_rmap();
extern int	rf_setidmap();
extern void	rf_freeidmap();
extern int	rf_addalist();
extern int	rf_checkalist();
extern void	rf_heapfree();
extern void	auth_init();
extern void	rf_acl_idmap();
extern void	rf_setgdplp();

enum acl_dir { CL2SR, SR2CL };


#define rf_remalist(clist) ((void)(!(clist) || (rf_heapfree(clist), 0)))

#endif /* _FS_RFS_RF_AUTH_H */
