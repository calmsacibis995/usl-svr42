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

#ident	"@(#)uts-x86at:boot/bootlib/getfhdr.c	1.1"
#ident  "$Header: $"

#include "util/types.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "svc/bootinfo.h"
#include "boot/libfm.h"

#include "boot/boot.h"
#include "boot/error.h"

BFHDR	bf_hdr;
#define	ELFHDR	bf_hdr.elf
#define COFFHDR	bf_hdr.coff

extern	struct	bootenv bootenv;

/*
 *	high level interface routine for identifying COFF/ELF
 *	format files
 *	transfer header information into the boot file table
 */
getfhdr(bftblp)
struct	bftbl	*bftblp;
{
	int	status;
	ulong	actual;

/*	start at the beginning of the file 				*/
	BL_file_lseek(0L, &status);
        BL_file_read(
		physaddr(&bf_hdr),NULL,(long)sizeof(BFHDR),&actual,&status);
        if (status != E_OK) {
                printf("gethead: cannot read header.\n");
		return(FAILURE);
	}

	switch(COFFHDR.mainhdr.f_magic) {
	case I386MAGIC:
		bftblp->t_type = COFF;
		bftblp->t_nsect = (int)COFFHDR.mainhdr.f_nscns;
		bftblp->t_entry = (ulong)COFFHDR.secondhdr.entry;
		bftblp->t_offset = (ulong)sizeof(struct coffhdrs);
#ifdef BOOT_DEBUG
		if (bootenv.db_flag & LOADDBG)
			printf("getfhdr: COFF nsect= 0x%x entry= 0x%x offset= 0x%x\n",
			bftblp->t_nsect, bftblp->t_entry, bftblp->t_offset); 	
#endif
		status = coffsect(bftblp);
		break;
	case ELFMAGIC:
		bftblp->t_type = ELF;
		bftblp->t_nsect = (int)ELFHDR.e_phnum;
		bftblp->t_entry = (ulong)ELFHDR.e_entry;
		bftblp->t_offset = (ulong)ELFHDR.e_phoff;
#ifdef BOOT_DEBUG
		if (bootenv.db_flag & LOADDBG)
			printf("getfhdr: ELF nsect= 0x%x entry= 0x%x offset= 0x%x\n",
			bftblp->t_nsect, bftblp->t_entry, bftblp->t_offset); 	
#endif
		status = elfseg(bftblp);
		break;
	default:
                printf("gethead: invalid header.\n");
		return(FAILURE);
	}
	return(status);
}

/*
 *	COFF file header handler
 */

coffsect(bftblp)
struct	bftbl	*bftblp;
{
	struct	scnhdr coffsect;
	int	i,j;
	ulong	scnsz;
	int	status;
	ulong	actual;

	scnsz = sizeof(struct scnhdr);

	BL_file_lseek(bftblp->t_offset, &status);
	for (i=0, j=0; i<bftblp->t_nsect; i++) {
        	BL_file_read(physaddr(&coffsect),NULL,scnsz,&actual,&status);
        	if (status != E_OK) {
                	printf("coffsect: cannot read COFF section header.\n");
			return(FAILURE);
		}
		if (j >= NBPH) {
			printf("coffsect: reserved boot program header space overflow.\n");
			return(FAILURE);
		}
/*		fill in section header information			*/
		if (coffsect.s_flags & STYP_TEXT)
			bftblp->t_bph[j].p_type = TLOAD;
		else if (coffsect.s_flags & STYP_DATA)
			bftblp->t_bph[j].p_type = DLOAD;
		else if (coffsect.s_flags & STYP_BSS)
			bftblp->t_bph[j].p_type = BLOAD;
		else if ((*(long *)coffsect.s_name == BKINAME) && 
			 (coffsect.s_size >=2))
			bftblp->t_bph[j].p_type = BKI;
		else
			continue;
		bftblp->t_bph[j].p_vaddr = coffsect.s_vaddr;
		bftblp->t_bph[j].p_memsz = coffsect.s_size;
		bftblp->t_bph[j].p_filsz = coffsect.s_size;
		bftblp->t_bph[j].p_offset = coffsect.s_scnptr;
		if (coffsect.s_flags & STYP_BSS)
			bftblp->t_bph[j].p_filsz = 0;
#ifdef BOOT_DEBUG
		if (bootenv.db_flag & LOADDBG) {
			printf("coffsect: idx= %d type= %d vaddr= 0x%x memsz= 0x%x offset= 0x%x\n",
				j, bftblp->t_bph[j].p_type,
				bftblp->t_bph[j].p_vaddr,
				bftblp->t_bph[j].p_memsz,
				bftblp->t_bph[j].p_offset);
		}
#endif
		j++;
	}
	bftblp->t_nbph = j;
	return(SUCCESS);
}


/*
 *	ELF file header handler
 */

elfseg(bftblp)
struct	bftbl	*bftblp;
{
	Elf32_Phdr elfphdr;
	int	i,j;
	ulong	scnsz;
	int	status;
	ulong	actual;

	scnsz = sizeof(Elf32_Phdr);

	BL_file_lseek(bftblp->t_offset,&status);
	for (i=0, j=0; i<bftblp->t_nsect; i++) {
        	BL_file_read(physaddr(&elfphdr),NULL,scnsz,&actual,&status);
        	if (status != E_OK) {
                	printf("elfseg: cannot read ELF program header.\n");
			return(FAILURE);
		}
		if (j >= NBPH) {
			printf("elfseg: reserved boot program header space overflow.\n");
			return(FAILURE);
		}
/*		fill in program header information			*/
		switch(elfphdr.p_type) {
			case PT_LOAD:
				switch(elfphdr.p_flags & (PF_R|PF_W|PF_X)) {
					case (PF_R|PF_W|PF_X):
						bftblp->t_bph[j].p_type= DLOAD;
						break;
					case (PF_R|PF_X):
						bftblp->t_bph[j].p_type= TLOAD;
						break;
					default:
						bftblp->t_bph[j].p_type= NOLOAD;
				}
				break;
			case PT_NOTE:
				bftblp->t_bph[j].p_type = BKI;
				break;
			default:
				bftblp->t_bph[j].p_type = NOLOAD;
		}
		if (bftblp->t_bph[j].p_type == NOLOAD)
			continue;
		bftblp->t_bph[j].p_vaddr = elfphdr.p_vaddr;
		bftblp->t_bph[j].p_filsz = elfphdr.p_filesz;
		bftblp->t_bph[j].p_memsz = elfphdr.p_memsz;
		bftblp->t_bph[j].p_offset = elfphdr.p_offset;
/*		check for BSS segment					*/
		if (elfphdr.p_filesz == 0)
			bftblp->t_bph[j].p_type= BLOAD;
		j++;
	}
	bftblp->t_nbph = j;
	return(SUCCESS);
}
