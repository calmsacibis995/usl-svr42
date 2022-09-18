/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _USYM_H
#define _USYM_H
#ident	"@(#)idtools:i386/ktool/unixsyms/usym.h	1.1"
#ident	"$Header: $"

/* structure to keep track of sections in input file */
struct section {
	Elf_Scn *sec_scn;
	Elf32_Shdr *sec_shdr;
};
#endif /* _USYM_H */
