/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/mod_objmd.c	1.1"
#ident	"$Header: $"
#include <util/types.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <svc/errno.h>
#include <proc/obj/elf.h>
#include <proc/obj/elf_386.h>
#include <util/mod/mod_obj.h>
#include <util/mod/mod_objmd.h>

int
mod_obj_relone(const struct module *mp, const Elf32_Rel *reltbl, unsigned int nreloc, size_t relocsize, const Elf32_Shdr *shp, const Elf32_Shdr *symhdr)
{

	Elf32_Rel *rend;
	unsigned int offset;
	unsigned int stndx;
	unsigned long value;
	Elf32_Sym *sp;
	unsigned int nsyms;


	rend = (Elf32_Rel *) ((char *) reltbl + nreloc * relocsize);

	nsyms = symhdr->sh_size / symhdr->sh_entsize;
	while(reltbl < rend) {
		offset = reltbl->r_offset + shp->sh_addr;
		value = 0;
		switch(ELF32_R_TYPE(reltbl->r_info)) {

			case R_386_NONE:
				break;
			case R_386_PC32:
				value = - (int) offset;
			case R_386_32:
				stndx = ELF32_R_SYM(reltbl->r_info);
				if(stndx >= nsyms)
				{
					cmn_err(CE_WARN, "!MOD: Bad symbol table index %d in relocation in module %s.\n", stndx,mp->md_path);
					return(-1);
				}
				value += (stndx == STN_UNDEF ? 0 :
		       			((Elf32_Sym *) (symhdr->sh_addr + 
					 mp->md_symentsize * stndx))->st_value);

				*(unsigned long *) offset = value + 
					*(unsigned long *) offset;
				break;
			default:
				cmn_err(CE_WARN,"!MOD: Illegal relocation type %d in module %s.\n",
					ELF32_R_TYPE(reltbl->r_info), mp->md_path);
				return(-1);
		}
		reltbl = (Elf32_Rel *) ((char *) reltbl + relocsize);
	}

	return(0);
}

