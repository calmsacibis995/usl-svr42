/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:boot/at386/priboot_sym.c	1.1"
#ident	"$Header: $"

#include <util/types.h>
#include <boot/bootdef.h>

#ifdef WINI

#include "wsbt_pconf.h"		 /* hard disk boot */

size_t __SYMBOL__BOOTDRV_HD = BOOTDRV_HD;
size_t __SYMBOL__B_BOOTSZ = B_BOOTSZ;
size_t __SYMBOL__B_FD_NUMPART = B_FD_NUMPART;
size_t __SYMBOL__B_ACTIVE = B_ACTIVE;

#elif HDTST

#include "hdsbt_pconf.h"	/* hard disk boot test using floppy */

#else

#include "fsbt_pconf.h"		/* floppy disk boot */

size_t __SYMBOL__BOOTDRV_FP = BOOTDRV_FP;
size_t __SYMBOL__FDB_ADDR = FDB_ADDR;

#endif

size_t __SYMBOL__SECBOOT_ADDR = SECBOOT_ADDR;
size_t __SYMBOL__PRIBOOT_ADDR = PRIBOOT_ADDR;
size_t __SYMBOL__PROG_SECT = PROG_SECT;
