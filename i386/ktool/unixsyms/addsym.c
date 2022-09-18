/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/unixsyms/addsym.c	1.6"
#ident	"$Header: $"

#include <libelf.h>
#include "usym.h"
#include <sys/module.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

extern struct section *esections;
extern int addflg;

#ifdef __STDC__
static Elf_Data *adddata(Elf_Scn *, Elf_Data *);
#else
static Elf_Data *adddata();
#endif

#ifdef __STDC__

Elf32_Addr
addsym(Elf *elf, Elf32_Ehdr *ehdr, char *scnname, Elf_Data *mod, Elf_Data *sym, Elf_Data *str, 
	Elf_Data *hash, Elf_Data *comm, Elf32_Addr *comm_addr, unsigned int *tsize)

#else

Elf32_Addr 
addsym(elf, ehdr, scnname, mod, sym, str, hash, comm, comm_addr, tsize)
Elf *elf;
Elf32_Ehdr	*ehdr;
char * scnname;
Elf_Data *mod, *sym, *str, *hash;
Elf_Data *comm;
unsigned long *comm_addr;
unsigned int *tsize;

#endif
{
	Elf32_Phdr	*phdr, *phdrout;
	Elf_Scn		*scn, *nscn, *shstrscn;
	Elf32_Shdr	*shdr, *nshdr;
	Elf_Data	*data, *nmod, *nsym, *nstr, *nhash, *ncomm;
	Elf_Data	*savedata;
	Elf32_Off	name_off = 0;
	char		*shname;
	Elf32_Half	pnum, opnum, highpnum;
	unsigned int	size1;
	unsigned int	totalsize;
	int 		lowindex;
	unsigned int 	i, loc_insert, savend, shstrndx;
	int		saveoff;


	phdrout = phdr = elf32_getphdr(elf);

	/*
	 * See if the desired section already exists.
	 */
	shname = NULL;
	for(i = 1; i < ehdr->e_shnum; i++) {
 
		shname = elf_strptr(elf, ehdr->e_shstrndx, esections[i].sec_shdr->sh_name);

		if (shname == NULL)
			continue;

		if ((strcmp(shname, scnname)) == 0) {
			scn = esections[i].sec_scn;
			shdr = esections[i].sec_shdr;
			break;
		}

		shname = NULL;

	}

	opnum = ehdr->e_phnum;
	highpnum = (Elf32_Half)-1;
	for (pnum = 0; pnum < opnum; ++pnum) {
		if (phdr[pnum].p_type != PT_LOAD)
			continue;
		if (highpnum == (Elf32_Half)-1 ||
	    	phdr[pnum].p_paddr > phdr[highpnum].p_paddr ||
	    	(phdr[pnum].p_paddr == phdr[highpnum].p_paddr &&
	     	phdr[pnum].p_vaddr > phdr[highpnum].p_vaddr)) {
			highpnum = pnum;
		}
	}

	if (highpnum == (Elf32_Half)-1) {
		fatal(23,"cannot find a loadable segment\n");
	}


	if (shname) {	/* we already have a section */
		/* must see if section is in last loadable segment */
		if(shdr->sh_addr < phdr[highpnum].p_vaddr ||
		    shdr->sh_addr > phdr[highpnum].p_vaddr + phdr[highpnum].p_memsz)
			fatal(27,"section, %s, not in last loadable segment\n",shname);

		pnum = highpnum;
		name_off = shdr->sh_name;

		/* if want to replace section contents, must zero out data already 
			there */
		if(!addflg) {
			data = elf_getdata(scn,0);
			phdrout[pnum].p_filesz -= data->d_size;
			phdrout[pnum].p_memsz -= data->d_size;
			data->d_size = 0;
			data->d_buf = NULL;
		}

	} else {	/* need to create a new section */

		/*
		 * Copy existing program headers to new array,
		* and find the one with the highest address.
		*/
		phdrout = elf32_newphdr(elf, opnum + 1);
		for (pnum = 0; pnum < opnum; ++pnum) {
			phdrout[pnum] = phdr[pnum];
		}
		opnum++;

		/*
	 	* Place the new program header after the "highest" one,
	 	* both virtually and physically.
	 	*/
		phdrout[pnum].p_vaddr = ctob(btoc(phdr[highpnum].p_vaddr +
					  	phdr[highpnum].p_memsz));
		phdrout[pnum].p_paddr = ctob(btoc(phdr[highpnum].p_paddr +
					  	phdr[highpnum].p_memsz));

		/* initialize values that will be used later */
		phdrout[pnum].p_offset = 0;
		phdrout[pnum].p_memsz= 0;
		phdrout[pnum].p_filesz = 0;

		/* make sure section alignment is 
		   correct wrt segment alignment */
		if(mod->d_size != 0)
			mod->d_align = ctob(1);


		/* Allocate the new section so that it immediately follows section 
		  in last segment in phdrout 
		  This entails zeroing out all sections from there to the end of 
		  the file, adding the new section, add readding all the old sections 
		*/
		for(i=1; i < ehdr->e_shnum; i++) {
			if(esections[i].sec_shdr->sh_offset +
				esections[i].sec_shdr->sh_size ==
				  phdrout[pnum-1].p_offset + phdrout[pnum-1].p_filesz) 
				  	break;
		}

		shstrscn = esections[ehdr->e_shstrndx].sec_scn;
		shstrndx = ehdr->e_shstrndx;
		loc_insert = i+1;
		if((savedata = (Elf_Data *) calloc((ehdr->e_shnum - loc_insert) * sizeof(Elf_Data),1)) == NULL)
			fatal(23,"Cannot allocate space for data buffers\n");

		for(i = loc_insert; i < ehdr->e_shnum; i++) {
			data = elf_getdata(esections[i].sec_scn, NULL);
			savedata[i-loc_insert] = *data;
			if(data->d_buf != NULL) {
				savedata[i-loc_insert].d_buf = (void *) malloc(data->d_size);
				memcpy(savedata[i-loc_insert].d_buf,data->d_buf,data->d_size);
			}
			data->d_buf = NULL;
			data->d_size = 0;
		}

		savend = ehdr->e_shnum;
		scn = elf_newscn(elf);
		shdr = elf32_getshdr(scn);

		for(i = loc_insert; i < savend; i++) {
			nscn = elf_newscn(elf);
			nshdr = elf32_getshdr(nscn);
			*nshdr = *esections[i].sec_shdr;
			esections[i].sec_shdr->sh_type = SHT_NULL;
			if(i == shstrndx) {
				ehdr->e_shstrndx += (savend - loc_insert +1);
				shstrscn = nscn;
			}
			if(nshdr->sh_type == SHT_SYMTAB && nshdr->sh_link > loc_insert)
				nshdr->sh_link += (savend - loc_insert +1);
			data = elf_newdata(nscn);
			*data = savedata[i-loc_insert];
		}
		/*
		 * Append new section name to end of string table.
		 */

		name_off = elf32_getshdr(shstrscn)->sh_size;

		data = elf_newdata(shstrscn);

		data->d_buf = scnname;
		data->d_type = ELF_T_BYTE;
		data->d_size = strlen(scnname) + 1;
		data->d_align = 1;
	}

	if((nmod = adddata(scn,mod)) == NULL)
		fatal(45,"cannot add data to %s section\n",scnname);
	if((nsym = adddata(scn,sym)) == NULL)
		fatal(45,"cannot add data to %s section\n",scnname);
	if((nstr = adddata(scn,str)) == NULL)
		fatal(45,"cannot add data to %s section\n",scnname);
	if((nhash = adddata(scn, hash)) == NULL)
		fatal(45,"cannot add data to %s section\n",scnname);
	if((ncomm = adddata(scn, comm)) == NULL)
		fatal(45,"cannot add data to %s section\n",scnname);
	
	shdr->sh_name = name_off;
	shdr->sh_type = SHT_PROGBITS;
	shdr->sh_flags = SHF_ALLOC;
	shdr->sh_link = SHN_UNDEF;
	shdr->sh_info = 0;
	shdr->sh_entsize = 0;

	/* save for updating offsets in phdr later */
	saveoff = esections[1].sec_shdr->sh_offset;
	elf_update(elf, ELF_C_NULL);

		totalsize = ncomm->d_off - nmod->d_off + ncomm->d_size;
		if(!phdrout[pnum].p_offset) {
			/* If new segment and nothing in the segment forget the 
				the whole thing */
			if(totalsize == 0)
				return(-1);

			phdrout[pnum].p_offset = shdr->sh_offset;
			shdr->sh_addr = phdrout[pnum].p_vaddr;
		} else {
			/* if existing segment, make sure that the section address
		   	reflects the fact that it may not be the first thing in the
		   	segment */
			shdr->sh_addr = phdrout[pnum].p_vaddr + 
			   	shdr->sh_offset - phdrout[pnum].p_offset;
		}
		phdrout[pnum].p_type = PT_LOAD;
		phdrout[pnum].p_filesz += totalsize;
		phdrout[pnum].p_memsz += totalsize;
		phdrout[pnum].p_flags = PF_R|PF_W|PF_X;
		if(nmod->d_size != 0) {
			phdrout[pnum].p_align = PAGESIZE;
			*tsize = ctob(btoc(totalsize));
		}
		else {
			phdrout[pnum].p_align = 1;
			*tsize = ncomm->d_size;
		}



	/* fill in struct module now that locations and sizes known */
	if(nmod->d_size != 0) {
		((struct module *) nmod->d_buf)->md_symspace = 
				(char *) (shdr->sh_addr + nsym->d_off);
		((struct module *) nmod->d_buf)->md_strings = 
				(char *) (shdr->sh_addr + nstr->d_off);
		((struct module *) nmod->d_buf)->md_buckets = 
				(unsigned long *) (shdr->sh_addr + nhash->d_off);
		((struct module *) nmod->d_buf)->md_chains = 
				(unsigned long *) (shdr->sh_addr + nhash->d_off +
				MOD_OBJHASH * sizeof(unsigned long));
		((struct module *) nmod->d_buf)->md_symsize = 
				(unsigned int) nhash->d_off + nhash->d_size - 
			   	nsym->d_off;
	}


	/* fill in size and location information in struct module */
	size1 =0;
	lowindex = -1;
	for(i=0; i< opnum; i++) {
		if(phdrout[i].p_type == PT_LOAD && lowindex == -1)
			lowindex = i;
		size1 += phdrout[i].p_memsz;
	}
	if(nmod->d_size != 0) {
		((struct module *) nmod->d_buf)->md_space = (char *) phdrout[lowindex].p_vaddr;
		((struct module *) nmod->d_buf)->md_space_size = size1;
	}

	/* update offsets in phdr to reflect extra entry if necessary */
	if(shname == NULL && saveoff != esections[1].sec_shdr->sh_offset) {
		/* assumes all segments move by same amount */
		for( i = 0; i < ehdr->e_phnum -1 ; i++ )
			phdrout[i].p_offset += 
			       (esections[1].sec_shdr->sh_offset - saveoff);
	}



	elf_flagphdr(elf, ELF_C_SET, ELF_F_DIRTY);

	elf_update(elf, ELF_C_WRITE);

	/* pass back info for patching file */
	*comm_addr = shdr->sh_addr + ncomm->d_off;
	return shdr->sh_addr + nmod->d_off;
}

#ifdef __STDC__
static
Elf_Data *
adddata(Elf_Scn *scn, Elf_Data *data)
#else
static
Elf_Data *
adddata(scn, data)
Elf_Scn *scn;
Elf_Data *data;
#endif
{
	Elf_Data *tmpdata;

	tmpdata = elf_newdata(scn);
	*tmpdata = *data;
	return(tmpdata);
}

