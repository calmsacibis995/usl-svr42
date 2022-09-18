/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_tables.c	1.4.3.2"
#ident	"$Header: $"

#include <fs/sfs/sfs_tables.h>
#include <util/debug.h>
#include <util/types.h>

/*
 * Bit patterns for identifying fragments in the block map
 * used as ((map & around) == inside)
 */
int sfs_around[] = SFS_AROUND_INIT;

int sfs_inside[] = SFS_INSIDE_INIT;

/*
 * Given a block map bit pattern, the frag tables tell whether a
 * particular size fragment is available. 
 *
 * used as:
 * if ((1 << (size - 1)) & fragtbl[fs->fs_frag][map] {
 *	at least one fragment of the indicated size is available
 * }
 *
 * These tables were originally designed to be used by the scanc
 * instruction on the VAX to quickly find an appropriate fragment.
 */
STATIC u_char sfs_fragtbl124[] = SFS_TBL124_INIT;

STATIC u_char sfs_fragtbl8[] = SFS_TBL8_INIT;

/*
 * The actual fragtbl array.
 */
u_char *sfs_fragtbl[] = SFS_FRAGTBL_INIT;
