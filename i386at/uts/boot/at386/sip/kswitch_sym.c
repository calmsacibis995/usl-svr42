/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:boot/at386/sip/kswitch_sym.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "boot/bootdef.h"

size_t __SYMBOL__CR0_PG = CR0_PG;
size_t __SYMBOL__CR0_EM = CR0_EM;
size_t __SYMBOL__PROTMASK = PROTMASK;
size_t __SYMBOL__NOPROTMASK = NOPROTMASK;
size_t __SYMBOL__C16GDT = C16GDT;
size_t __SYMBOL__D_GDT = D_GDT;
size_t __SYMBOL__KPD_LOC = KPD_LOC;
size_t __SYMBOL__B_BKI_MAGIC = B_BKI_MAGIC;
size_t __SYMBOL__KTSSSEL = KTSSSEL;
size_t __SYMBOL__JTSSSEL = JTSSSEL;
