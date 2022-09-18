/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/unixsyms/unixsyms.c	1.11"
#ident	"$Header:"

/* In a cross-environment, make sure these headers are for the host system */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

/* The remaining headers are for the target system. */
#include <libelf.h>
#include <sys/module.h>
#include "usym.h"

long lseek();

#ifdef __STDC__
extern Elf32_Addr addsym(Elf *, Elf32_Ehdr *,char *, Elf_Data *, Elf_Data *, Elf_Data *, Elf_Data *,
	Elf_Data *, Elf32_Addr *,  unsigned int *);
#else
extern Elf32_Addr addsym();
#endif

/* Special symbol names */
#define SN_KERNMOD	"mod_obj_kern"	/* kernel module pointer */
#define SN_SYMSIZE	"mod_obj_size"
#define SN_KDBCOMMANDS	"kdbcommands"	/* location for kdb commands strings*/
#define SYMSECTION	".unixsyms"	/* section name for symbol data */

/* initialize Elf_Data structure to minimal values */
/* The version field is initialized inline */
#define INITDATA(data)	{ data.d_size = 0; data.d_buf = NULL; \
				data.d_align = 1; data.d_type = ELF_T_BYTE; }

/* offsets for special symbols */
Elf32_Addr loc_kernmod;
unsigned long sec_kernmod;
Elf32_Addr loc_kdbcommands;
unsigned long sec_kdbcommands;
Elf32_Addr loc_symsize;
unsigned long sec_symsize;

int addflg = 0;			/* flag indicating that new data should be appended
					to existing section of given name instead
					of replacing it */

struct section *esections;

int
#ifdef __STC__
lstrcmp(char *s1, char **s2)
#else
lstrcmp(s1,s2)
char *s1, **s2;
#endif
{
	return(strcmp(s1,*s2));
}

#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif
{
	int fd, ifd, efd, numlim, lim;
	char *namebuf;
	char **namelist = NULL;
	char *tmpnam;
	int nksyms;			/* number of symbols in output symbol table */
	unsigned int tsize;		/* size of symbol table and commands stuff
						added to file */
	off_t icmd_len = 0, ecmd_len = 0;
	int i;
	char *name;
	Elf		*elf_file;
	Elf32_Ehdr	*p_ehdr;	/* elf file header */
	unsigned	data_encoding;	/* byte encoding format in file */
	Elf_Scn		*scn;		/* elf section header	*/
	Elf_Scn		*sym_scn;
	Elf_Data	*sym_data;	/* info on symtab	*/
	Elf_Data	nsymdata, nstrdata;	
	Elf_Data	hashdata, moddata, commdata;
	Elf32_Sym	*symspace;
	unsigned int	symsize, strsize;
	char		*strspace;
	unsigned int	str_ndx, sym_ndx;
	unsigned long	*hashspace;
	unsigned long	hval;
	Elf32_Sym	*esym;		/* pointer to ELF symbol	*/
	Elf32_Addr	kernmod_addr, comm_addr;
	char		*prog = argv[0];
	int		c;
	char 		*symsection = SYMSECTION;
	int		debugonlyflg = 0;
	extern char	*optarg;
	extern int	optind;

	INITDATA(commdata);

	while ((c = getopt(argc, argv, "?dpavl:s:i:e:")) != EOF) {
		switch (c) {
		case 'v':
			break;
		case 'i':
			if ((ifd = open(optarg, O_RDONLY)) >=0) {
				if ((icmd_len = lseek(ifd, 0L, 2)) == -1L)
					fatal(7, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(ifd, 0L, 0);
				commdata.d_size += (icmd_len + 1);
			}
			break;
		case 'e':
			if ((efd = open(optarg, O_RDONLY)) < 0)
				errno = 0;
			else {
				if ((ecmd_len = lseek(efd, 0L, 2)) == -1L)
					fatal(7, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(efd, 0L, 0);
				commdata.d_size += (ecmd_len + 1);
			}
			break;
		case 'd':
			debugonlyflg++;
			break;
		case 's':
			symsection = optarg;
			break;
		case 'a':
			addflg++;
			break;
		case 'l':
			if((fd = open(optarg, O_RDONLY)) < 0) 
				fatal(5,"%s: cannot open limit file %s\n",prog,optarg);
			else {
				if ((lim = lseek(fd, 0L, 2)) == -1L)
					fatal(6, "%s: seek error on %s\n",
					      prog, optarg);
				lseek(fd, 0L, 0);
				if((namebuf = calloc(lim+1, 1)) == NULL)
					fatal(8, "%s: cannot calloc memory for limit list\n",prog);
				if(read(fd,namebuf,lim) != lim)
					fatal(8, "%s: read error on %s\n",prog, optarg);
				
				i = 0;
				numlim = 0;
				while(i < lim) {
					if(*(namebuf + i) == '\n')
						numlim++;
					i++;
				}
				if((namelist = 
				     (char ** )malloc(numlim * sizeof(char *))) == NULL)
					fatal(8, "%s: cannot malloc memory for limit list\n",prog);
				i = 1;
				*(namelist) = strtok(namebuf,"\n");
				while((tmpnam = strtok(NULL,"\n")) != NULL) {
					*(namelist+i) = tmpnam;
					i++;
				}
				close(fd);
			}
			break;

				
		default:
			goto usage;
		}
	}

	if (argc - optind != 1) {
usage:
		fatal(99,
	"%s: usage: unixsyms [-v] [-s section-name] [-a] [-d] [[-i|-e] init-cmd-file] kernel-file\n",
			prog);
	}


	if ((fd = open(argv[optind], O_RDWR)) < 0)
		fatal(10, "%s: cannot open %s\n", prog, argv[optind]);

	if (elf_version(EV_CURRENT) == EV_NONE)
		fatal(11, "%s: ELF library is out of date\n", prog);

	if ((elf_file = elf_begin(fd, ELF_C_RDWR, (Elf *)0)) == 0) {
		fatal(12, "%s: ELF error in elf_begin: %s\n", prog,
				elf_errmsg(elf_errno()));
	}

	/*
	 *	get ELF header
	 */
	if ((p_ehdr = elf32_getehdr( elf_file )) == 0) {
		fatal(13, "%s: problem with ELF header: %s\n", prog,
				elf_errmsg(elf_errno()));
	}
	data_encoding = p_ehdr->e_ident[EI_DATA];

	/*
	 *	load section table
	 */
	if((esections = calloc(sizeof(struct section), p_ehdr->e_shnum)) == NULL)
		fatal(53,"Cannot allocate space for section headers\n");
	i=1;	/* skip the first entry so indexes match with file */
	scn = 0;
	while(( scn =  elf_nextscn( elf_file,scn )) != 0 ) {
		esections[ i ].sec_scn =  scn;
		esections[ i ].sec_shdr = elf32_getshdr( scn );
		if( esections[ i ].sec_shdr->sh_type == SHT_SYMTAB){
			sym_scn = scn;
			str_ndx = esections[ i ].sec_shdr->sh_link;

			esym = NULL;
			sym_ndx = i;
			sym_data = 0;
			if ((sym_data = elf_getdata(sym_scn, sym_data)) == 0)
				fatal(10, "%s: no symbol table data.\n", prog);
			esym = (Elf32_Sym *)sym_data->d_buf;
			if(namelist != NULL)
				nksyms = numlim + 1;
			else
				nksyms = sym_data->d_size / sizeof(Elf32_Sym) - 
					esections[i].sec_shdr->sh_info + 1;
		}
		i++;
	}
	if(esections[str_ndx].sec_shdr->sh_type != SHT_STRTAB)
		fatal(10, "%s: symbol table does not point to string table.\n",prog);

	if(!debugonlyflg) {
		symsize = nksyms * sizeof(Elf32_Sym);
		symspace = (Elf32_Sym *) malloc(symsize);
		if (symspace == NULL) {
			fatal(20, "%s: not enough memory to build symbol table.\n",
		      	prog);
		}
	
		nsymdata.d_buf = symspace; 	/*  initialize now since symspace will be
								destroyed */
	
		symspace++;			/* leave room for empty symbol slot 0 */
	
		if(namelist != NULL)
			strspace = calloc(lim+1,1);
		else
			strspace = calloc(esections[str_ndx].sec_shdr->sh_size,1);
		strsize = 1;
	
		if (strspace == NULL) {
			fatal(20, "%s: not enough memory to build string table.\n",
		      	prog);
		}
		hashspace = (unsigned long *)
			calloc((MOD_OBJHASH + nksyms) *sizeof(unsigned long), 1);
		if (hashspace == NULL)
			fatal(20, "%s: not enough memory to build hash table.\n",
		      	prog);
	}
	
	
	/* for each desired global symbol in input file... 
	  		sh_info field has index of first non-LOCAL,
	  	also make first symtable entry blank (std ELF) */

	for (esym = esym + esections[sym_ndx].sec_shdr->sh_info, i = 1; 
		esym < (Elf32_Sym *) ((char *)sym_data->d_buf + sym_data->d_size); esym++) {


		name = elf_strptr( elf_file, str_ndx, (size_t)esym->st_name);

		/* if it's a symbol we'll be patching, remember its location */

		if (strcmp(name, SN_KDBCOMMANDS) == 0) {
			sec_kdbcommands = esym->st_shndx;
			loc_kdbcommands = esym->st_value -
				 esections[sec_kdbcommands].sec_shdr->sh_addr;
		} 
		if(!debugonlyflg) {
			if (strcmp(name, SN_KERNMOD) == 0) {
				sec_kernmod = esym->st_shndx;
				loc_kernmod = esym->st_value -
				 	esections[sec_kernmod].sec_shdr->sh_addr;
			} else if (strcmp(name, SN_SYMSIZE) == 0) {
				sec_symsize = esym->st_shndx;
				loc_symsize = esym->st_value -
				 	esections[sec_symsize].sec_shdr->sh_addr;
			}
	
	
			if(namelist == NULL || bsearch(name,namelist,numlim,
			     sizeof(char *), lstrcmp) != NULL) {
				hval = elf_hash(name);
				hashspace[i+MOD_OBJHASH] = hashspace[hval % MOD_OBJHASH];
				hashspace[hval % MOD_OBJHASH] = i;

				*symspace = *esym;
				strncpy(strspace+strsize, name, strlen(name)+1);
				symspace->st_name = strsize;
				symspace++;
				strsize += strlen(name)+1;
				i++;
			}
		}
	
	}
	/* make sure we found the symbol we need to patch */

	if (!debugonlyflg && !loc_kernmod)
		fatal(1, "no symbol named '%s' found in %s\n",
		      SN_KERNMOD, argv[optind]);
	if (!debugonlyflg && !loc_symsize)
		fatal(1, "no symbol named '%s' found in %s\n",
		      SN_SYMSIZE, argv[optind]);

	/* fill in Data structures to pass to addsym */

	moddata.d_version = nsymdata.d_version = nstrdata.d_version = 
		commdata.d_version = hashdata.d_version = EV_CURRENT;

	if(!debugonlyflg) {
		moddata.d_size = sizeof(struct module);
		moddata.d_buf = calloc(sizeof(struct module), 1);
		if(moddata.d_buf == NULL)
			fatal(20, "%s: not enough memory to build module structure.\n",
		      	prog);
		moddata.d_align = sizeof(char *);
		moddata.d_type = ELF_T_WORD;	/* assumes ELF_T_WORD translated same as
					   	ELF_T_ADDR  since struct module is 
					   	a mix of pointers and int/longs */
	
		/* fill in part of struct module with info in hand now 
	    	- rest will be filled in in addsym */
		((struct module *)moddata.d_buf)->md_symentsize = esections[sym_ndx].sec_shdr->sh_entsize;
	
		/* nsymdata.d_buf all ready points to symspace above */
		nsymdata.d_size = symsize;
		nsymdata.d_align = sym_data->d_align;
		nsymdata.d_type = ELF_T_SYM;
	
		nstrdata.d_size = strsize;
		nstrdata.d_buf = strspace;
		nstrdata.d_align = esections[str_ndx].sec_shdr->sh_addralign;
		nstrdata.d_type = ELF_T_BYTE;
	
		hashdata.d_size = (MOD_OBJHASH + nksyms) * sizeof(unsigned long);
		hashdata.d_buf = hashspace;
		hashdata.d_align = sizeof(unsigned long);
		hashdata.d_type = ELF_T_WORD;
	} 
	else {
		INITDATA(moddata);
		INITDATA(nsymdata);
		INITDATA(nstrdata);
		INITDATA(hashdata);
	}
	
	/* next comes initial command string */

	if(loc_kdbcommands && commdata.d_size != 0) {
		if((commdata.d_buf = calloc(commdata.d_size,1)) == NULL) {
			fatal(20, "%s: not enough memory to build command info.\n",
		      	prog);
		}
		if (icmd_len > 0) {
			if (read(ifd, commdata.d_buf, icmd_len) != icmd_len) {
				fatal(21,
			      	"%s: error reading initial command file.\n",
			      	prog);
			}
	
			fprintf(stderr,
				"initial command string loaded (%d bytes)\n",
				icmd_len);
			close(ifd);
		}
		/* null-terminate the icmd string */
		*((char *)commdata.d_buf +icmd_len) = '\0';
	
	
		/* next comes early-access command string */
	
		if (ecmd_len > 0) {
			if (read(efd, (char *)commdata.d_buf + icmd_len + 1,
				 ecmd_len) != ecmd_len) {
				fatal(21,
			      	"%s: error reading early-access command file.\n",
			      	prog);
			}
	
			fprintf(stderr,
				"early-access command string loaded (%d bytes)\n",
				ecmd_len);
			close(efd);
		}
		/* null-terminate the ecmd string */
		*((char *)commdata.d_buf + icmd_len + 1 + ecmd_len) = '\0';
	}
	
	
	/* Patch the symbol information into the ELF file */

	kernmod_addr = addsym(elf_file, p_ehdr, symsection,
			    &moddata, &nsymdata, &nstrdata, &hashdata, &commdata,
			    &comm_addr, &tsize);

	if(loc_kernmod) {
		loc_kernmod += esections[sec_kernmod].sec_shdr->sh_offset;
		loc_symsize += esections[sec_symsize].sec_shdr->sh_offset;
	}
	if(loc_kdbcommands)
		loc_kdbcommands += esections[sec_kdbcommands].sec_shdr->sh_offset;

	elf_end(elf_file);


	if(!debugonlyflg) {
		if(patchfile(fd,loc_kernmod,kernmod_addr,data_encoding) != 0)
			fatal(7,"%s: Failed to patch %s in file %s.\n",prog,SN_KERNMOD,argv[optind]);
		if(patchfile(fd,loc_symsize,tsize,data_encoding) != 0)
			fatal(7,"%s: Failed to patch %s in file %s.\n",prog,SN_SYMSIZE,argv[optind]);
	}

	if(commdata.d_size != 0 && loc_kdbcommands != 0) {
		if(patchfile(fd,loc_kdbcommands,comm_addr,data_encoding) != 0)
			fatal(7,"%s: Failed to patch %s in file %s.\n",prog,SN_KDBCOMMANDS,argv[optind]);

	
	}
	close(fd);
}


#ifdef __STDC__
fatal(int code, char *format, char *arg1, char *arg2, char *arg3)
#else
fatal(code, format, arg1, arg2, arg3)
int code;
char *format;
char *arg1, *arg2, *arg3;
#endif
{
	fprintf(stderr, format, arg1, arg2, arg3);
	exit(code);
}

#ifdef __STDC__
int
patchfile(int fd, Elf32_Addr location, Elf32_Addr value, unsigned int data_encoding)
#else
int patchfile(fd, location, value, data_encoding)
int fd;
Elf32_Addr location, value;
unsigned int data_encoding;
#endif
{
	Elf_Data src_data, dst_data;
	unsigned long data_word;

	/* set up to patch symbols */
	src_data.d_version = dst_data.d_version = EV_CURRENT;
	src_data.d_buf = dst_data.d_buf = (char *)&data_word;
	src_data.d_size = dst_data.d_size = sizeof(data_word);
	src_data.d_type = ELF_T_WORD;

	/* Patch the address for the symbol table */
	if (lseek(fd, location, 0) == -1L)
		return(-1);
	data_word = (unsigned long)value;
	if (!elf32_xlatetof(&dst_data, &src_data, data_encoding)) {
		return(-1);
	}
	if (write(fd, (char *)&data_word, sizeof(data_word)) == -1)
		return(-1);
	return(0);
}
