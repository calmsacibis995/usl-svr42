/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/bootlib/blfile.c	1.1"
#ident  "$Header: $"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990"

#include "util/types.h"
#include "io/vtoc.h"
#include "svc/bootinfo.h"

#include "boot/boot.h"
#include "boot/libfm.h"
#include "boot/error.h"
#include "boot/dib.h"

extern 	off_t	boot_delta;
extern	struct	bootenv bootenv;
extern	bfstyp_t boot_fs_type;

off_t	disk_file_offset;

/*
 *  BL_init for the disk must initialize the disk params and alttrack 
 *  mapping so that subsequent open and reads will work.
 *
 *  get_fs() then checks for boot file system (BFS) and initializes parameters
 *  for BFS and root files sytem.  It returns the root_delta: physical block
 *  number  for the beginning of the root filesystem.
 */

BL_file_init()
{
   extern bfstyp_t root_fs_type;
   off_t   rootdelta;

    /* returns offset for root partition */
    rootdelta=get_fs();

    /* check if root is s5 and initialize */
    if ( s5_init(rootdelta) == 0 )
	root_fs_type = s5;
	
    /* code can be added to check for other file system types later */
}

/*
 *  BL_file_open for AT ignores dib.  *status gets error code from open
 *  and translates to either E_OK or ~E_OK.  
 *  In addition, with file pointer disk_file_off is set to 0; subsequent
 *  read updates this.
 *
 */

BL_file_open(path, dib, status)
register char	*path;
struct	dib	*dib;
register ulong	*status;
{
	int	mystatus;

	if (boot_fs_type == s5)
		mystatus = biget(bnami(V_ROOT, path));
	else
		mystatus = bfsopen(path);

	if (mystatus <= 0)
		*status = E_FNEXIST;
	else {
		disk_file_offset = 0;
		*status = E_OK;
	}
}


/*
 *	Calling the file system specific read routine
 */
BL_file_read(buffer, buffer_sel, buffer_size, actual, status)
register char 	*buffer;
ushort	buffer_sel;
register ulong	buffer_size;
ulong	*actual;
register ulong	*status;
{
#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG )
		printf("BL_file_read: dfoffset= 0x%x rdcnt= 0x%x\n",
				disk_file_offset,buffer_size);
#endif
	if (boot_fs_type == s5)
		*actual = breadi(disk_file_offset,buffer,buffer_size);
	else
		*actual = bfsread(disk_file_offset,buffer,buffer_size);
	if (*actual == buffer_size) {
		*status = E_OK;
		disk_file_offset += *actual;
	} else {
#ifdef BOOT_DEBUG
		if (bootenv.db_flag & LOADDBG )
			printf("BL_file_read failed: dfoffset= 0x%x rdcnt= 0x%x\n",
				disk_file_offset,*actual);
#endif
		*status = E_IO;
	}
}

/*
 *	Moving the disk file offset pointer to the given offset position
 */
BL_file_lseek(offset, status)
off_t	offset;
register ulong	*status;
{
	disk_file_offset = offset;
	*status = E_OK;
}

/*
 *	Close routine
 */
BL_file_close(status)
register ulong	*status;
{
	*status = E_OK;
}
