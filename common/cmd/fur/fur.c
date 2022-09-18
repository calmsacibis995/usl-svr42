/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:common/cmd/fur/fur.c	1.1"
#ident	"$Header:"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#include <malloc.h>
#endif
#include <libelf.h>
#include "fur.h"

char *prog;				/* program name */
struct section *esections;		/* list of section in target file */
int debugflg =0;

#define MARK	0x80000000

#ifdef __STDC__
void usage(char *);
#else
void usage();
typedef enum { B_FALSE, B_TRUE } boolean_t;
#endif

#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif
{
	/* variables dealing with list of functions given by user */
	int 		lfnum;			/* number of functions */
	int 		lfsize = 0;			/* sizeof list */
	char 		*lffile = NULL;		/* file name for list */
	char 		*lfbuf;			/* buffer holding list */
	char 		**lfnames = NULL;	/* array of pointers to names */

	/* temporary variables */
	unsigned int 	i;
	char 		*name;
	Elf32_Sym	*esym;
	Elf_Scn		*scn;

	/* file information for target file */
	int 		fd;
	Elf		*elf_file;
	Elf32_Ehdr	*p_ehdr;	/* elf file header */

	/* info on symbol table section */
	Elf_Data	*sym_data = NULL;
	Elf32_Sym	*endsym;	/* pointer to end of symbol table */

	/* info on string table for symbol table */
	Elf_Data	*str_data;
	int		str_ndx;

	/* info on target section */
	int 		text_ndx = -1;	/* index of target section*/
	Elf_Data 	*text_data;	/* input text section */
	char 		*text_name = ".text";	
					/* name of target section*/
	int 		text_ptr = 0;	/* used as index into text_newdata */
	char 		*text_newdata;	/* rearranged section */
	Elf32_Shdr	*text_shdr;	/* section header of the target sect */
	int		textsym_ndx; /* index in symtab of sect symbol */

	/* getopt processing */
	int		c;
	extern char	*optarg;
	extern int	optind;


	prog = argv[0];

	while ((c = getopt(argc, argv, "?l:s:d")) != EOF) {
		switch (c) {
		case 's':
			text_name = optarg;
			break;
		case 'd':
			debugflg = 1;
			break;
		case 'l':
			if((fd = open(optarg, O_RDONLY)) < 0) 
				error( "cannot open function list file %s\n",
						optarg);
			else {
				lffile = optarg;
				if ((lfsize = lseek(fd, 0L, 2)) == -1L)
					error( "seek error on %s\n",
					      optarg);
				lseek(fd, 0L, 0);
				if((lfbuf = calloc(lfsize+1, 1)) == NULL)
					error("cannot calloc memory for function list\n");
				if(read(fd,lfbuf,lfsize) != lfsize)
					error("read error on %s\n", optarg);
				
				i = 0;
				lfnum = 0;
				while(i < lfsize) {
					if(*(lfbuf + i) == '\n')
						lfnum++;
					i++;
				}
				if((lfnames = 
				     (char ** )malloc(lfnum * sizeof(char *))) == NULL)
					error("cannot malloc memory for function list\n");
				i = 1;
				*(lfnames) = strtok(lfbuf,"\n");
				while((name = strtok(NULL,"\n")) != NULL) {
					*(lfnames+i) = name;
					i++;
				}
				close(fd);
			}
			break;
		default:
			usage("illegal option");
		}
	}

	if(lfsize == 0) {
		usage("no function list given");
	}



	if ((fd = open(argv[optind], O_RDWR)) < 0)
		error( "cannot open %s\n", argv[optind]);

	if (elf_version(EV_CURRENT) == EV_NONE)
		error( "ELF library is out of date\n");

	if ((elf_file = elf_begin(fd, ELF_C_RDWR, (Elf *)0)) == 0) {
		error( "ELF error in elf_begin: %s\n", 
				elf_errmsg(elf_errno()));
	}

	/*
	 *	get ELF header
	 */
	if ((p_ehdr = elf32_getehdr( elf_file )) == 0) {
		error( "problem with ELF header: %s\n", 
				elf_errmsg(elf_errno()));
	}

	/*
	 *	check that it is a relocatable file
	 */
	 if(p_ehdr->e_type != ET_REL)
		error( "only processes relocatable files\n");

	/*
	 *	load section table
	 */
	if((esections = 
		calloc(sizeof(struct section), p_ehdr->e_shnum)) == NULL)
		   error( "cannot allocate space for section headers\n");


	i=1;	/* skip the first entry so indexes match with file */
	scn = 0;

	while(( scn =  elf_nextscn( elf_file,scn )) != 0 ) {
		esections[ i ].sec_scn =  scn;
		esections[ i ].sec_shdr = elf32_getshdr( scn );
		if( esections[ i ].sec_shdr->sh_type == SHT_SYMTAB){

			if(sym_data != NULL)
				error( "multiple symbol table sections not allowed\n");
			esections[i].sec_data = sym_data = 
			   myelf_getdata(scn, 0,"symbol table");
			endsym = (Elf32_Sym *) 
			     ((char *) sym_data->d_buf + sym_data->d_size);

			/* get string data for symbol table */
			str_ndx = esections[ i ].sec_shdr->sh_link;
		}

		/* is this the target section */
		if((name = 
		    elf_strptr(elf_file,p_ehdr->e_shstrndx,
			       esections[i].sec_shdr->sh_name)) == NULL)
		     error("cannot get name for section header %d\n",i);
		if(strcmp(name,text_name) == 0) {
			/* if so save, some information */
			if(text_ndx != -1)
				error( "multiple %s sections\n",name);
			text_ndx = i;
			esections[i].sec_data = text_data = 
			   myelf_getdata(scn,0,"section to be rearranged\n");
			text_shdr = esections[i].sec_shdr;
		}
		i++;
	}
	if(esections[str_ndx].sec_shdr->sh_type != SHT_STRTAB)
		error( "symbol table does not point to string table.\n");
	esections[str_ndx].sec_data = str_data =
		myelf_getdata(esections[str_ndx].sec_scn,0, "string table");




	/* determine new addresses for given functions */
	for(i = 0; i < lfnum; i++) {
		boolean_t foundsym = B_FALSE;
		Elf32_Sym *tsym = sym_data->d_buf;

		/* multiple symbols of the same name in sym table will
		   be placed together in the order found in symtab */
		while((esym = findsym(*(lfnames+i), tsym, endsym, text_ndx, 
				       str_data->d_buf)) != NULL) {

			foundsym = B_TRUE;
			tsym = esym+1;

			/* MARK is a sign that have already passed this way */
			if(esym->st_size & MARK)
			    	error(
				  "function %s appears more than once in given function list %s or functions size > %x\n", 
				     *(lfnames +i), lffile,MARK);

			if(esym->st_size == 0)
				error( "function of unknown size %s\n",
						*(lfnames+i));

			esym->st_size |= MARK;

			/* create and initialize text_info structure */
			esym->st_value = 
			  (Elf32_Addr) gettextinfo(esym, text_data->d_buf);

			text_ptr = ALIGN((long) text_ptr,
				text_shdr->sh_addralign);

			((struct text_info *) esym->st_value)->ti_newaddr = 
				(Elf32_Addr) text_ptr;


			text_ptr += (esym->st_size & ~MARK);
				
		}
		if(!foundsym)
			fprintf(stderr,"WARNING: function %s not found in symbol table\n",
					    *(lfnames+i));
	}

	/* find the rest of the functions in the target section */
	for (esym = ((Elf32_Sym *) sym_data->d_buf) + 1; esym < endsym; esym++) 	{
		if(esym->st_shndx != text_ndx 
		   || esym->st_size & MARK)
			continue;

		if(ELF32_ST_TYPE(esym->st_info) == STT_SECTION) {
			textsym_ndx = esym - 
				 (Elf32_Sym *) sym_data->d_buf;
			continue;
		}

		if(ELF32_ST_TYPE(esym->st_info) != STT_FUNC)
		    error(
			  "illegal type in section to be rearranged: %d\n",
			  ELF32_ST_TYPE(esym->st_info));

		if(esym->st_size == 0)
				error( "function of unknown size %s\n",
						*(lfnames+i));

		/* create and initialize text_info structure */
		esym->st_value = 
			  (Elf32_Addr) gettextinfo(esym, text_data->d_buf);

		text_ptr = ALIGN((long) text_ptr, text_shdr->sh_addralign);

		((struct text_info *) esym->st_value)->ti_newaddr = 
				(Elf32_Addr) text_ptr;

		text_ptr += esym->st_size;

	}


	/* update all relocation sections for rearranged section */
	for(i = 1; i < p_ehdr->e_shnum; i++) {
		Elf_Data *td;
		int rtype = esections[i].sec_shdr->sh_type;

		if(rtype != SHT_REL && rtype != SHT_RELA) 
			continue;

		esections[i].sec_data = td = 
		   myelf_getdata(esections[i].sec_scn,0, "relocation section");

		if(esections[i].sec_shdr->sh_info != text_ndx) {
			/* check for relocations against text section
			   symbol in relocations for other sections */
			chktextrels(td, sym_data, esections[i].sec_shdr->sh_info, 
					text_ndx, textsym_ndx, rtype);
			continue;
		}

		/* update relocations for section being rearranged */
		updaterels(td, sym_data, text_ndx, rtype);
	}

	/* now actually rarrange the section */

	if((text_newdata = calloc(text_ptr,1)) == NULL) 
		error("cannot calloc data for new text section\n");
	text_ptr = 0;

	/* first the given functions */
	for(i = 0; i < lfnum; i++) {
		Elf32_Sym *tsym = sym_data->d_buf;

		while((esym = findsym(*(lfnames+i),tsym,endsym, text_ndx,
				       str_data->d_buf)) != NULL) {
			int tptr;
			tsym = esym +1;

			/* align pointer and fill in with desired value */
			tptr = text_ptr;
			text_ptr = 
			  ALIGN(text_ptr, text_shdr->sh_addralign);
			filltext(text_newdata + tptr, text_newdata + text_ptr);

			memcpy(text_newdata + text_ptr,
				((struct text_info *) esym->st_value)->ti_data,
				esym->st_size & ~MARK);
				
			text_ptr += (esym->st_size & ~MARK);
		}

	}

	/* then the rest of the functions */
	for (esym = ((Elf32_Sym *) sym_data->d_buf) + 1; esym < endsym; esym++) 	{
		int tptr;
		if(esym->st_shndx != text_ndx 
		   || ELF32_ST_TYPE(esym->st_info) == STT_SECTION
		   || esym->st_size & MARK)
			continue;

		/* align pointer and fill in with desired value */
		tptr = text_ptr;
		text_ptr = ALIGN(text_ptr, text_shdr->sh_addralign);
		filltext(text_newdata + tptr, text_newdata + text_ptr);

		memcpy(text_newdata + text_ptr,
			((struct text_info *) esym->st_value)->ti_data,
			esym->st_size);
				
		text_ptr += esym->st_size;

	}

	/* update the Elf_Data structure for the rearranged section*/
	text_data->d_buf=text_newdata;
	text_data->d_size = text_ptr;

	/* fix the symbol table back up */
	for (esym = (Elf32_Sym *)sym_data->d_buf + 1; esym < endsym; esym++) {

		if(esym->st_shndx != text_ndx 
		    || ELF32_ST_TYPE(esym->st_info) == STT_SECTION)
			continue;

		esym->st_size = esym->st_size & ~MARK;
		if(debugflg)
			printf("name %s oldaddr %x newaddr %x\n",
				esym->st_name + (char *) str_data->d_buf,
				((struct text_info *) esym->st_value)->ti_curaddr,
				((struct text_info *) esym->st_value)->ti_newaddr);
		esym->st_value = ((struct text_info *) esym->st_value)->ti_newaddr;
			
	}

	elf_flagphdr(elf_file, ELF_C_SET, ELF_F_DIRTY);

	elf_update(elf_file, ELF_C_WRITE);

	elf_end(elf_file);

	close(fd);
}



Elf32_Sym *
#ifdef __STDC__
findsymbyoff(Elf32_Addr off, int text_ndx, Elf32_Sym *firstsym, Elf32_Sym *end)
#else
findsymbyoff(off,text_ndx,firstsym,end)
Elf32_Addr off;
int text_ndx;
Elf32_Sym *firstsym, *end;
#endif
{
	Elf32_Sym *esym = firstsym;
	while(esym < end) {

		if(esym->st_shndx == text_ndx &&
			ELF32_ST_TYPE(esym->st_info) != STT_SECTION &&
			off >= ((struct text_info *) esym->st_value)->ti_curaddr &&
			off < ((struct text_info *) esym->st_value)->ti_curaddr +
				(esym->st_size & ~MARK))
				return(esym);
		esym++;
	}
	return(NULL);
}

Elf32_Sym *
#ifdef __STDC__
findsym(char *name, Elf32_Sym *firstsym, Elf32_Sym *end, int text_ndx, char *str_dbuf)
#else
findsym(name,firstsym, end, text_ndx, str_dbuf)
char *name;
Elf32_Sym *firstsym;
Elf32_Sym *end;
int text_ndx;
char *str_dbuf;
#endif
{
	Elf32_Sym *esym = firstsym;
	while(esym < end) {

		if(ELF32_ST_TYPE(esym->st_info) == STT_FUNC &&
			esym->st_shndx == text_ndx &&
			strcmp(name, str_dbuf + esym->st_name) == 0)
				return(esym);
		esym++;
	}
	return(NULL);
}

long
#ifdef __STDC__
symchg(Elf32_Addr off, int text_ndx, Elf32_Sym *firstsym, Elf32_Sym *end)
#else
symchg(off,text_ndx,firstsym,end)
Elf32_Addr off;
int text_ndx;
Elf32_Sym *firstsym, *end;
#endif
{
	Elf32_Sym *esym;

	if((esym = findsymbyoff(off,text_ndx,firstsym,end)) == NULL)
		error("cannot find symbol at offset %x in target section\n",off);

	return(((struct text_info *) esym->st_value)->ti_newaddr -
		((struct text_info *) esym->st_value)->ti_curaddr);
}

		

Elf_Data *
#ifdef __STDC__
myelf_getdata(Elf_Scn *scn, Elf_Data *data, const char *errmsg)
#else
myelf_getdata(scn, data, errmsg)
Elf_Scn *scn;
Elf_Data *data;
const char *errmsg;
#endif
{
	Elf_Data *td;
	if((td = elf_getdata(scn,data)) == NULL)
		error( "cannot get data for %s\n", errmsg);
	return(td);
}

struct text_info *
#ifdef __STDC__
gettextinfo(Elf32_Sym *esym, char *text_dptr)
#else
gettextinfo(esym, text_dptr)
Elf32_Sym *esym;
char *text_dptr;
#endif
{
	struct text_info *titemp;
	if((titemp = (struct text_info *) calloc(1,sizeof(struct text_info))) 
	     == 0)
		  error( "cannot allocate memory for text_info\n");

	titemp->ti_curaddr = esym->st_value;

	/* set pointer to body of function in data */
	titemp->ti_data = text_dptr + esym->st_value;
	return(titemp);
}

void
#ifdef __STDC__
usage(char *msg)
#else
usage(msg)
char *msg;
#endif
{
	fprintf(stderr,"%s: %s\n", prog,msg);
	fprintf(stderr,"Usage: %s -l function_list [ -s section_name ] reloc_file\n");
	exit(-1);
}

void
#ifdef __STDC__
error(const char *fmt, ...)
#else
error(va_alist)
va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;
	va_start(ap);
	fmt = va_arg(ap, char *);
#endif
	fprintf(stderr,"%s: ", prog);
	vfprintf(stderr, fmt, ap);
	exit(-1);
}

