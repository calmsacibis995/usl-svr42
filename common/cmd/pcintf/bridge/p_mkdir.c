/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_mkdir.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_mkdir.c	6.4	LCC);	/* Modified: 11:56:06 12/3/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <string.h>

#include "pci_types.h"
#include "dossvr.h"


void
pci_mkdir(dos_dir, addr, request)
    char	*dos_dir;		/* Name of directory to create */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* Number of DOS request simulated */
{
    char directory[MAX_FN_TOTAL];

    /* Translate MS-DOS to UNIX directory name */
    if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_dir,
	(unsigned char *)directory)) {
	addr->hdr.res = PATH_NOT_FOUND;
	return;
    }

    if (mkdir(directory, 0777) < 0)	/* mode modified by umask */
	err_handler(&addr->hdr.res, request, directory);
    else
	addr->hdr.res = SUCCESS;
    addr->hdr.stat = NEW;
}
