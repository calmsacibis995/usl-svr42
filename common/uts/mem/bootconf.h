/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifndef _MEM_BOOTCONF_H	/* wrapper symbol for kernel use */
#define _MEM_BOOTCONF_H	/* subject to change without notice */

#ident	"@(#)uts-comm:mem/bootconf.h	1.1.2.2"
#ident	"$Header: $"

/*
 * Boot time configuration information objects.
 *
 * We currently use only one instance of struct bootobj,
 * to store information on the configured swap device.
 * That structure is allocated at configuration time.
 */

#define	MAXFSNAME	16
#define	MAXOBJNAME	128

struct bootobj {
	char	bo_fstype[MAXFSNAME];	/* filesystem type name (e.g. nfs) */
	char	bo_name[MAXOBJNAME];	/* name of object */
	int	bo_flags;		/* flags, see below */
	int	bo_offset;		/* number of blocks */
	int	bo_size;		/* number of blocks */
	struct vnode *bo_vp;		/* vnode of object */
};

/*
 * Flags.
 */
#define	BO_VALID	0x01		/* all info in object is valid */
#define	BO_BUSY		0x02		/* object is busy */

#endif	/* _MEM_BOOTCONF_H */
