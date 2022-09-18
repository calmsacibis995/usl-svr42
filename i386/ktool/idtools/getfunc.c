/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/getfunc.c	1.4"
#ident	"$Header: "

/*
 * This module examines the symbol table of a Driver.o to find out
 * which entry points are present.  This is used by idconfig.
 */
#include "inst.h"
#include "defines.h"

extern struct entry_def *entry_defs;
extern short debug;

int
get_funcs(filename, prefix)
char *filename;	/* Name of Driver.o file */
char *prefix;	/* Routine name prefix */
{
	struct entry_def *edefp;

	if(debug)
		fprintf(stderr,"\tGET_FUNC: %s\n",filename);

	/* prepending desired prefix */
	for(edefp = entry_defs; edefp != NULL; edefp = edefp->next){
		strcpy(edefp->sname, prefix);
		strcat(edefp->sname, edefp->suffix);
	}

	return(mark_globals(filename,entry_defs));
}


/*
 * In a cross-environment, make sure these headers are for the host system
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

/*
 * The remaining headers are for the target system.
 */
#include <libelf.h>

static int
mark_globals(filename,edefbase)
char *filename; struct entry_def *edefbase;
{
	FILE		*fp;		/* stream for elf file 	*/
	Elf		*elf_file;	/* elf file		*/
	Elf32_Shdr      *section;	/* elf section header	*/
	Elf_Scn		*scn;		/* elf section header	*/
	Elf_Data	*data;		/* info on section tab	*/
	size_t		str_ndx;	/* String index		*/
	Elf32_Sym	*esym;		/* pointer to ELF symbol*/
	size_t		nsyms = 0 ;	/* number of symbols	*/
	struct entry_def *edefp;
	int scncount;

	if ((fp = fopen(filename,"r")) == NULL){
		if(debug)
			fprintf(stderr,"mark_sym: cannot open %s\n",filename);
		return(-1);
	}

	if (elf_version(EV_CURRENT) == EV_NONE){
		if(debug)
			fprintf(stderr,"mark_sym: ELF lib is out of date\n");
		fclose(fp); 
		return(-1);
	}

	if ((elf_file = elf_begin(fileno(fp), ELF_C_READ, (Elf *)0)) == 0) {
		if(debug)
			fprintf(stderr,
				"mark_sym: ELF error in elf_begin: %s, %s\n",
				filename,elf_errmsg(elf_errno()));
		fclose(fp);
		return(-1);
	}

	/*
	 * load section table
	 */
	scncount = 0;
	scn = 0;
	while(scncount < 2 && (scn = elf_nextscn(elf_file,scn)) != 0){
		section = elf32_getshdr(scn);
		if(section->sh_type == SHT_STRTAB){
			data = 0;
			if((data = elf_getdata(scn,data)) == 0 ||
			    data->d_size == 0){
				if(debug)
				   fprintf(stderr,
				   "mark_sym: bad string section data %s.\n",
				   filename);
				elf_end(elf_file);
				fclose(fp);
				return(-1);
			}
			scncount++;
		}else if(section->sh_type == SHT_SYMTAB){
			str_ndx = section->sh_link;
			data = 0;
			if((data = elf_getdata(scn,data)) == 0){
				if(debug)
				   fprintf(stderr,
				   "mark_sym: bad symbol table %s.\n",
				   filename);
				elf_end(elf_file);
				fclose(fp);
				return(-1);
			}

			esym  = (Elf32_Sym *)data->d_buf;
			nsyms = data->d_size / sizeof(Elf32_Sym);
			esym++;	/* first member holds number of symbols	*/
			scncount++;
		}
	}

	for(edefp = edefbase; edefp != NULL; edefp = edefp->next)
		symlookup(elf_file,str_ndx,esym,nsyms,edefp);

	elf_end(elf_file);
	fclose(fp);

	return(0);
}

/*
 * Lookup symbol within symbol list and mark it if avail
 */
static int
symlookup(elf_file,str_ndx,esym,nsyms,edefp)
Elf *elf_file; size_t str_ndx;
Elf32_Sym *esym; size_t nsyms;
struct entry_def *edefp;
{
	register int i;
	char *name;

	/*
	 * for each symbol in input file...
	 */
	edefp->has_sym = 0;
	for(i = 1; i < nsyms ; i++, esym++ ){
		/*
		 * we only care about it if it is external
		 */
		if(ELF32_ST_BIND( esym->st_info) != STB_GLOBAL)
			continue;

		name = elf_strptr(elf_file,str_ndx,(size_t)esym->st_name);

		/* skip symbols that start with '.' */
		if (name[0] == '.')
			continue;

		if(strcmp(name,edefp->sname) == 0){
			edefp->has_sym = 1;
			
			if(debug)
				fprintf(stderr,"\t\t%s\n",name);

			return(1);
		}
	}

	return(0);
}
