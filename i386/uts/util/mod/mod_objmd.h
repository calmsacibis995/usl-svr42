/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_OBJMD_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_OBJMD_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/mod/mod_objmd.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _PROC_OBJ_ELFTYPES_H
#include <proc/obj/elftypes.h>
#endif

#elif defined(_KERNEL)

#include <sys/elftypes.h>

#endif /* _KERNEL_HEADERS */

#define MOD_OBJ_MACHTYPE	EM_386
#define MOD_OBJ_VALRELOC(x)	(x == SHT_REL)
#define MOD_OBJ_ERRRELOC(x)	(x == SHT_RELA)
extern int mod_obj_relone(const struct module *, const Elf32_Rel *, 
				unsigned int, size_t, const Elf32_Shdr *, 
				const Elf32_Shdr *);

#endif	/* _UTIL_MOD_MOD_OBJMD_H */
