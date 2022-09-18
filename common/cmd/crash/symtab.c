/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/symtab.c	1.12.1.6"
#ident	"$Header: symtab.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions: nm, ds, and ts, as well
 * as the initialization routine rdsymtab.
 */

#include "a.out.h"
#include "stdio.h"
#include "string.h"
#include "crash.h"
#include "malloc.h"
#include "sys/ksym.h"
#include "sys/module.h"
#include "sys/mod_k.h"

#include "libelf.h"

extern	int	nmlst_tstamp ;		/* namelist timestamp */
extern char *namelist;
extern short N_TEXT,N_DATA,N_BSS;	/* used in symbol search */
struct syment *stbl = NULL;			/* symbol table */
int symcnt = 0;				/* symbol count */
char *strtbl;				/* pointer to string table */

int iscoff = 1;				/* presume namelist is COFF */

#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))

/* symbol table initialization function */

int
rdsymtab()
{
	FILE *np;
	struct filehdr filehdr;
	struct syment	*sp,
			*ts_symb ;
	struct scnhdr	scnptr ,
			boot_scnhdr ;
	int	i ,
		N_BOOTD ;
	char *str;
	long *str2;
	long strtblsize;

	/* see if we need to read the symbol table or will we get it on the fly
		from getksym */
	if(active)
		return(0);
	/*
	 * Open the namelist and associate a stream with it. Read the file into a buffer.
	 * Determine if the file is in the correct format via a magic number check.
	 * An invalid format results in a return to main(). Otherwise, dynamically 
	 * allocate enough space for the namelist. 
	 */

		
	if(!(np = fopen(namelist, "r")))
		fatal("cannot open namelist file\n");
	if(fread((char *)&filehdr, FILHSZ, 1, np) != 1)
		fatal("read error in namelist file\n");
	if(filehdr.f_magic != FBOMAGIC) {
		rewind(np);
		if ((rdelfsym(np,0,0,namelist) != 0))
			fatal("%s not in a.out format\n",namelist);
		findmemholes();		/* must be done here since rdlmods
						uses readmem */
		rdlmods();
		fclose(np);
		return;
	}
	if (iscoff) {
	/*
	 * Read the section headers to find the section numbers
	 * for .text, .data, and .bss.  First seek past the file header 
	 * and optional header, then loop through the section headers
	 * searching for the names .text, .data, and .bss.
	 */
	N_TEXT=0;
	N_DATA=0;
	N_BSS=0;
	N_BOOTD=0 ;
	if(fseek(np, (long)(FILHSZ + filehdr.f_opthdr), 0) != 0
	  && fread((char *)&filehdr, FILHSZ, 1, np) != 1)
		fatal("read error in section headers\n");

	for(i=1; i <= (int)filehdr.f_nscns; i++)
	{
		if(fread(&scnptr, SCNHSZ, 1, np) != 1)
			fatal("read error in section headers\n");

		if(strcmp(scnptr.s_name,_TEXT) == 0)
			N_TEXT = i ;
		else if(strcmp(scnptr.s_name,_DATA) == 0)
			N_DATA = i ;
		else if(strcmp(scnptr.s_name,_BSS) == 0)
			N_BSS = i ;
		else if(strcmp(scnptr.s_name,"boot") == 0)
		{
			/* save data section for later processing */
			N_BOOTD = 1 ;
			boot_scnhdr = scnptr ;
		}

	}
	if(N_TEXT == 0 || N_DATA == 0 || N_BSS == 0) 
		fatal(".text, .data, or .bss was not found in section headers\n");

	/*
	 * Now find the string table (if one exists) and
	 * read it in.
	 */
	if(fseek(np,filehdr.f_symptr + filehdr.f_nsyms * SYMESZ,0) != 0)
		fatal("error in seeking to string table\n");
	
	if(fread((char *)&strtblsize,sizeof(int),1,np) != 1)
		fatal("read error for string table size\n");
	
	if(strtblsize)
	{
		if(!(strtbl = (char *)malloc((unsigned)strtblsize)))
			fatal("cannot allocate space for string table\n");

		str2 = (long *)strtbl;
		*str2 = strtblsize;

		for(i = 0,str = (char *)((int)strtbl + (int)sizeof(long)); i < strtblsize - sizeof(long); i++, str++)
			if(fread(str, sizeof(char), 1, np) != 1)
				fatal("read error in string table\n");
	}
	else
		str = 0;

	if(!(stbl=(struct syment *)malloc((unsigned)(filehdr.f_nsyms*SYMESZ))))
		fatal("cannot allocate space for namelist\n");

	/*
	 * Find the beginning of the namelist and read in the contents of the list.
	 *
	 * Additionally, locate all auxiliary entries in the namelist and ignore.
	 */

	fseek(np, filehdr.f_symptr, 0);
	symcnt = 0;
	for(i=0, sp=stbl; i < filehdr.f_nsyms; i++, sp++) {
		symcnt++;
		if(fread(sp, SYMESZ, 1, np) != 1)
			fatal("read error in namelist file\n");
		if(sp->n_zeroes == 0) 
			sp->n_offset = (long) (sp->n_offset+strtbl);
		if(sp->n_numaux) {
			fseek(np,(long)AUXESZ*sp->n_numaux,1);
			i += sp->n_numaux;
		}
	}
	/* save timestamp from data space of namelist file */
		
	if(!(ts_symb = symsrch("crash_sync")) || !N_BOOTD)
		nmlst_tstamp = 0 ;
	else
	{
		if(fseek(np,(long)(boot_scnhdr.s_scnptr + (ts_symb -> n_value - boot_scnhdr.s_paddr)),0) != 0)
			fatal("could not seek to namelist timestamp\n") ;
		if(fread((char *)&nmlst_tstamp,sizeof(int),1,np) != 1)
			fatal("could not read namelist timestamp\n") ;
	}
	} /* iscoff */
	findmemholes();
	fclose(np);
}


/* find symbol */
struct syment *
findsym(addr)
unsigned long addr;
{
	struct syment *sp;
	struct syment *save;
	unsigned long value;
	char name[MAXSYMNMLEN];
	unsigned long offset;
	char *tname;

	if(active) {
		if(getksym(name,&addr,&offset) != 0)
			return(NULL);
		sp = findsp(name);
		if(sp->n_offset == 0) {
			sp->n_offset = (long) calloc(strlen(name) + 1,1);
			strcpy((char *) sp->n_offset,name);
		}
		offset =0;
		(void) getksym(name,&sp->n_value,&offset);
		sp->n_type = offset;
		sp->n_sclass =  C_EXT;
		return(sp);
	}

	value = MAINSTORE;
	save = NULL;

	for(sp = stbl; sp < &stbl[symcnt]; sp++) {
		if(((sp->n_sclass == C_EXT) || (sp->n_sclass == C_STAT)) && 
			((unsigned long)sp->n_value <= addr)
		  && ((unsigned long)sp->n_value > value)) {
			value = (unsigned long)sp->n_value;
			save = sp;
		}
	}
	if(save && save->n_zeroes) {
		tname = calloc(SYMNMLEN+1,1);
		strncpy(tname,save->n_name,SYMNMLEN);
		save->n_zeroes = 0;
		save->n_offset = (long) tname;
	}
	return(save);
}

/* get arguments for ds and ts functions */
int
getsymbol()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do {prsymbol(args[optind++], 0);
		}while(args[optind]);
	}
	else longjmp(syn,0);
}

/* print result of ds and ts functions */
int
prsymbol(string, addr)
char *string;
long addr;
{
	struct syment *sp = NULL;

	if (!addr) {
		if((addr = strcon(string,'h')) == -1)
			error("\n");
	}

	if(!(sp = findsym((unsigned long)addr))) {
		if (string)
			prerrmes("%s does not match\n",string);
		else
			prerrmes("%x does not match\n",addr);
		return;
	}

	fprintf(fp,"%s",(char *) sp->n_offset);		

	fprintf(fp," + %x\n",addr - (long)sp->n_value);
}


/* search symbol table */
struct syment *
_symsrch(s,glob_only)
char *s;
boolean_t glob_only;
{
	struct syment *sp;
	struct syment *found;
	char *name;
	unsigned long info;
	unsigned long value = 0;
	char *tname;

	if(active) {
		if(getksym(s,&value,&info) != 0)
			return(NULL);
		sp = findsp(s);
		sp->n_value = value;
		if(sp->n_zeroes) {
			tname = calloc(SYMNMLEN+1,1);
			strncpy(tname,sp->n_name,SYMNMLEN);
			sp->n_zeroes = 0;
			sp->n_offset = (long) tname;
		}
		sp->n_type = info;
		sp->n_sclass =  C_EXT;
		return(sp);
	}
	found = 0;


	for(sp = stbl; sp < &stbl[symcnt]; sp++) {
		if(((sp->n_sclass == C_EXT) || (!glob_only && sp->n_sclass == C_STAT)) &&
		   ((unsigned long)sp->n_value >= MAINSTORE)) {
			if(sp->n_zeroes) {
				name = calloc(SYMNMLEN+1,1);
				strncpy(name,sp->n_name,SYMNMLEN);
				sp->n_zeroes = 0;
				sp->n_offset = (long) name;
			} else
				name = (char *) sp->n_offset;
			if(!strcmp(name,s)) {
				found = sp;
				break;
			}
		}
	}
	return(found);
}

struct syment *
symsrch(name)
char *name;
{
	return(_symsrch(name,B_FALSE));
}

/* get arguments for nm function */
int
getnm()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		do { prnm(args[optind++]);
		}while(args[optind]);
	else longjmp(syn,0);
}


/* print result of nm function */
int
prnm(string)
char *string;
{
	char *cp;
	struct syment *sp;

	if(!(sp = symsrch(string))) {
		prerrmes("%s does not match in symbol table\n",string);
		return;
	}
	fprintf(fp,"%s   %08.8lx  ",string,sp->n_value);

	if(active || !iscoff) {
		if(sp->n_type == STT_FUNC)
			cp = "text";
		else if(sp->n_type == STT_OBJECT)
			cp = "data";
		else if	(sp->n_scnum == N_ABS)
			cp = " absolute";
		else
			cp = "type unknown";
	} else if (iscoff) {
		if      (sp -> n_scnum == N_TEXT)
			cp = " text";
		else if (sp -> n_scnum == N_DATA)
			cp = " data";
		else if (sp -> n_scnum == N_BSS)
			cp = " bss";
		else if (sp -> n_scnum == N_UNDEF)
			cp = " undefined";
		else if (sp -> n_scnum == N_ABS)
			cp = " absolute";
		else
			cp = " type unknown";

	} 

	fprintf(fp,"%s (%s symbol)\n", cp,
		(sp->n_sclass == C_EXT ? "global" : "static/local"));
}

/*
**	Read symbol table of ELF namelist.
**	tdstrt and comstrt are the starting addresses at which the 
**	text/data/bss and defined common symbols respectively are loaded
**	for a given loadable module.  They are 0 for the static kernel.
**	Necessarily, the algorith for address assignments in loadable modules
**	herein must be kept up to date  with the one in the kernel.
*/

rdelfsym(fp, tdstrt, comstrt,path)
FILE *fp;
Elf32_Addr tdstrt, comstrt;
char *path;
{
	register int i;
	register Elf32_Sym *sy;
	volatile struct syment *sp;
	struct syment *tsp;
	struct syment *ts_symb;
	Elf *elfd;
	Elf_Scn	*scn;
	Elf_Scn	*symscn;
	Elf32_Shdr *eshdr;
	Elf32_Shdr *symhdr;
	Elf32_Sym *symtab;
	Elf_Data *data;
	Elf_Data *strdata;
	int fd;
	int nsyms;
	Elf32_Shdr **shdrs;
	Elf32_Ehdr *ehdr;

        if (elf_version (EV_CURRENT) == EV_NONE) {
		fatal("ELF Access Library out of date\n");
	}
	
	fd = fileno(fp);

	if ((lseek(fd, 0L, 0)) == -1L) {
		fatal("Unable to rewind %s\n",path);
	}

        if ((elfd = elf_begin (fd, ELF_C_READ, NULL)) == NULL) {
		fatal("Unable to elf begin on %s\n",path);
	}

	if ((elf_kind(elfd)) != ELF_K_ELF) {
		elf_end(elfd);
		return (-1);
	}

	if((ehdr = elf32_getehdr(elfd)) == NULL) 
		fatal("cannot get elf header of %s\n",path);

	/* loadable modules are relocatable ELF files */
	if(tdstrt != 0 && ehdr->e_type != ET_REL) {
		elf_end(elfd);
		return(-1);
	}

	if((shdrs = malloc(ehdr->e_shnum*sizeof(Elf32_Shdr *))) == NULL)
		fatal("cannot malloc space for section header table for %s\n",path);

	scn = NULL;
	i = 1;
	while ((scn = elf_nextscn(elfd, scn)) != NULL) {

		if ((eshdr = elf32_getshdr(scn)) == NULL) {
			elf_end(elfd);
			fatal("cannot read section header of %s\n",path);
		}

		if (eshdr->sh_type == SHT_SYMTAB) {
			symhdr = eshdr;		/* Can only do 1 symbol table */
			symscn = scn;
		}
		*(shdrs+i) = eshdr;
		i++;
	}

	/* set up section header addresses so the proper addresses
	   can be determined for symbols in this loadable module */
	if(tdstrt != 0) 
		for(i = 1; i < (int) ehdr->e_shnum; i++) {
			Elf32_Shdr *shp;
			shp = *(shdrs+i);
			if((shp->sh_type != SHT_MOD 
				&& shp->sh_type != SHT_PROGBITS
				&& shp->sh_type != SHT_NOBITS)
			    || !(shp->sh_flags & SHF_ALLOC))
				continue;
			tdstrt = ALIGN(tdstrt, shp->sh_addralign);
			shp->sh_addr = tdstrt;
			tdstrt += shp->sh_size;
		}


		/* Should have scn and eshdr for symtab */

	data = NULL;
	if (((data = elf_getdata(symscn, data)) == NULL) ||
		(data->d_size == 0) || (!data->d_buf)) {
			elf_end(elfd);
			fatal("can not read symbol table of %s\n",path);
	}

	symtab = (Elf32_Sym *)data->d_buf;

	nsyms = data->d_size / sizeof(Elf32_Sym);

	/*
	**	get string table
	*/

	if ((scn = elf_getscn(elfd, symhdr->sh_link)) == NULL) {
		elf_end(elfd);
		fatal("ELF strtab read error on %s\n",path);
	}

	strdata = NULL;
	if (((strdata = elf_getdata(scn, strdata)) == NULL) ||
		(strdata->d_size == 0) || (!strdata->d_buf)) {
			elf_end(elfd);
			fatal("string table read failure on %s\n",path);
	}

	if ((strtbl = malloc(strdata->d_size)) == NULL)
		fatal("cannot allocate space for string table of %s\n",path);

	(void)memcpy(strtbl, strdata->d_buf, strdata->d_size);

	if (stbl != NULL) {
		if((stbl=(struct syment *)realloc(stbl,(unsigned)((symcnt+nsyms)*sizeof(SYMENT)))) == NULL)
			fatal("cannot allocate space for symbol info from %s\n",path);
	} else
		if((stbl=(struct syment *)malloc((unsigned)(nsyms*sizeof(SYMENT)))) == NULL)
			fatal("cannot allocate space for symbol info from %s\n",path);

	/*
	**	convert ELF symbol table info to COFF
	**	since rest of pgm uses COFF
	*/

	sp = (stbl + symcnt);
	sy = symtab;
	for (i = 0; i < nsyms; i++, sy++) {

		if ((ELF32_ST_TYPE(sy->st_info)) == STT_FILE)
			continue;

		if ((ELF32_ST_TYPE(sy->st_info)) == STT_SECTION)
			continue;

		sp->n_zeroes = 0L;
		sp->n_offset = (long) (sy->st_name + strtbl);
		if(tdstrt != 0 && sy->st_shndx < SHN_LORESERVE) {
			Elf32_Shdr *shp = *(shdrs+sy->st_shndx);
			if( (shp->sh_type != SHT_MOD 
				&& shp->sh_type != SHT_PROGBITS
				&& shp->sh_type != SHT_NOBITS)
			    || !(shp->sh_flags & SHF_ALLOC))
				sp->n_value = sy->st_value;
			else
				sp->n_value = shp->sh_addr + sy->st_value;
		}
		else if(tdstrt != 0 && sy->st_shndx == SHN_COMMON) {
			/* Is the common symbol defined by this module? */
			if((tsp = _symsrch((char *) sp->n_offset,B_TRUE))
					== NULL) {
				if(comstrt == 0)
					fatal("unresolved common symbol %s in %s\n",
						sp->n_offset,path);
				sp->n_value = comstrt;
				comstrt += sy->st_size;
			}
			else
				sp->n_value = tsp->n_value;
				
		}
		else
			sp->n_value = sy->st_value;
		sp->n_scnum = sy->st_shndx;
		sp->n_type = ELF32_ST_TYPE(sy->st_info);
		sp->n_sclass =  ELF32_ST_BIND(sy->st_info);
		sp->n_numaux = 0;

		if (sp->n_scnum == SHN_ABS)
			sp->n_scnum = N_ABS;

		if (sp->n_sclass == STB_GLOBAL)
			sp->n_sclass = C_EXT;
		else
			sp->n_sclass = C_STAT;

		sp++;
		symcnt++;
	}

	free(shdrs);
	/* Get time stamp */

	if(!(ts_symb = symsrch("crash_sync")))
                nmlst_tstamp = 0 ;
        else {

		if ((scn = elf_getscn(elfd, ts_symb->n_scnum)) == NULL) {
			elf_end(elfd);
			fatal("ELF timestamp scn read error on %s\n",path);
		}

		if ((eshdr = elf32_getshdr(scn)) == NULL) {
			elf_end(elfd);
			fatal("cannot read timestamp section header on %s\n",path);
		}

		if ((lseek(fd,
			(long)(ts_symb->n_value - eshdr->sh_addr + eshdr->sh_offset),
				0)) == -1L)
                        fatal("could not seek to timestamp on %s\n",path) ;

                if ((read(fd, (char *)&nmlst_tstamp, sizeof(nmlst_tstamp))) !=
				sizeof(nmlst_tstamp))
                        fatal("could not read timestamp on %s\n",path);
        }

	iscoff = 0;

	elf_end(elfd);

	return(0);
}

static struct symlist {
	struct syment *sl_ent;
	struct symlist *sl_next;
} *slhead = NULL;

struct syment *
findsp(name)
char *name;
{
	struct symlist *tsl;
	char buf[SYMNMLEN+1];

	tsl = slhead;
	while(tsl) {
		if(strcmp(name,(char *) tsl->sl_ent->n_offset) == 0)
			return(tsl->sl_ent);
		tsl = tsl->sl_next;
	}
	tsl = (struct symlist *) malloc(sizeof(struct symlist));
	tsl->sl_ent = (struct syment *) calloc(sizeof(struct syment), 1);
	tsl->sl_next = slhead;
	slhead = tsl;
	return(tsl->sl_ent);
}


/* build the path on which to open the loadable module.
** Use the path from which the module is loaded unless the -m
** command option was given
*/
char *
getmodpath(addr)
Elf32_Addr addr;
{
	int i=0;
	unsigned char c;
	Elf32_Addr taddr = addr;
	Elf32_Addr lslash = addr;
	char *tret;
	extern char *modpath;

	/* strlen */
	do {
		readmem(taddr,1,-1,&c,1,"module path");
		if(c == '/')
			lslash = taddr;
		i++;
		taddr++;
	} while(c != '\0');
	if(modpath != NULL) {
		if((tret = (char *) malloc(strlen(modpath)+taddr-lslash)) == NULL)
			fatal("cannot malloc space for module name\n");
		strcpy(tret,modpath);
		readmem(lslash,1,-1,tret+strlen(modpath),taddr-lslash,"module path");
		return(tret);
	}
	if((tret = (char *) malloc(i)) == NULL)
		fatal("cannot malloc space for module name");
	readmem(addr,1,-1,tret,i,"module path");
	return(tret);
}

Elf32_Addr *modlist = NULL;
int modnum = 0;
#define MODMOD	15

/* make sure that no module symbol information is loaded twice */
int
trackmods(maddr)
Elf32_Addr maddr;
{
	int i;
	for(i = 0; i < modnum; i++) {
		if(maddr == *(modlist+i))
			return(0);
	}
	if(modnum == 0) {
		if((modlist = (Elf32_Addr *) malloc(MODMOD*sizeof(Elf32_Addr)))
			==NULL)
		   fatal("cannot malloc space for dependency list\n");
	}
	else if(modnum % MODMOD == 0) {
		if((modlist = (Elf32_Addr *) realloc(modlist, 
				(modnum +MODMOD)*sizeof(Elf32_Addr))) == NULL)
		   fatal("cannot realloc space for dependency list\n");
	}
	*(modlist+modnum) = maddr;
	return(++modnum);
}

/* prepare to load a module's symbol information */
Elf32_Addr
ldmod(maddr)
Elf32_Addr maddr;
{
	struct module module;
	FILE *fp;
	char *path;
	struct modctl modctl;
	struct modctl_list mcl;

		
	readmem(maddr,1,-1,&modctl,sizeof(struct modctl),
			"modctl structure");

	if(trackmods(maddr) == 0 || !(modctl.mod_flags & MOD_SYMTABOK))
		return((Elf32_Addr) modctl.mod_next);
	readmem(modctl.mod_mp,1,-1,&module,sizeof(struct module),
		"module structure");
	maddr = (Elf32_Addr) module.md_mcl;
	while(maddr != NULL) {
		readmem(maddr,1,-1,&mcl,sizeof(struct modctl_list), "dependency list");
		(void) ldmod(mcl.mcl_mcp);
		maddr = (Elf32_Addr) mcl.mcl_next;
	}
		
	path = getmodpath(module.md_path);
	if((fp = fopen(path,"r")) == NULL 
	   || rdelfsym(fp,module.md_space,module.md_bss,path) != 0) 
		fprintf(stderr,"crash: cannot process %s to get loadable module info\n",path);
	
	fclose(fp);
	free(path);
	return((Elf32_Addr) modctl.mod_next);
}


/* trundle through the list of loadable modules, 
** loading the symbol info for each.
*/
rdlmods()
{
	struct modctl modctl;
	struct syment *Modhead;
	Elf32_Addr maddr;

	if(!(Modhead = symsrch("modhead")))
		return;
	
	readmem(Modhead->n_value,1,-1,&modctl,sizeof(struct modctl),"modhead");

	maddr = (Elf32_Addr) modctl.mod_next;
	while(maddr != Modhead->n_value) {
		maddr = ldmod(maddr);
	}
}

