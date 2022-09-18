/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:boot/at386/prot_sym.c	1.1"
#ident	"$Header: $"

#include <util/types.h>
#include <boot/bootdef.h>

size_t __SYMBOL__SECBOOT_STACKSIZ = SECBOOT_STACKSIZ;
size_t __SYMBOL__STK_SBML = STK_SBML;
size_t __SYMBOL__STK_SPC = STK_SPC;
size_t __SYMBOL__STK_SPT = STK_SPT;
size_t __SYMBOL__STK_BPS = STK_BPS;
size_t __SYMBOL__STK_EDS = STK_EDS;
size_t __SYMBOL__STK_AP = STK_AP;
size_t __SYMBOL__PROTMASK = PROTMASK;
size_t __SYMBOL__NOPROTMASK = NOPROTMASK;
size_t __SYMBOL__B_GDT = B_GDT;
size_t __SYMBOL__C_GDT = C_GDT;
size_t __SYMBOL__C16GDT = C16GDT;
size_t __SYMBOL__D_GDT = D_GDT;
