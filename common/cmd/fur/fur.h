/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:common/cmd/fur/fur.h	1.1"
#ifndef _FUR_H
#define _FUR_H

#ifdef __STDC__
void error(const char *, ...);
Elf_Data * myelf_getdata(Elf_Scn *, Elf_Data *, const char *);
Elf32_Sym *findsymbyoff(Elf32_Addr, int, Elf32_Sym *, Elf32_Sym *);
long symchg(Elf32_Addr off, int text_ndx, Elf32_Sym *firstsym, Elf32_Sym *end);
Elf32_Sym *findsym(char *, Elf32_Sym *, Elf32_Sym *, int, char *);
struct text_info *gettextinfo(Elf32_Sym *, char *);
void updaterels(Elf_Data *, Elf_Data *, int, int);
void chktextrels(Elf_Data *, Elf_Data *, int, int, int, int);
#else
void error();
Elf32_Sym *findsym();
Elf32_Sym *findsymbyoff();
long symchg();
struct text_info *gettextinfo();
void usage();
Elf_Data *myelf_getdata();
void updaterels();
void chktextrels();
#endif

/* keeps track of functions as the are moved */
struct text_info {
	Elf32_Addr ti_curaddr;		/* symtab input address */
	Elf32_Addr ti_newaddr;		/* symtab output address */
	char *ti_data;			/* input buffer ptr to function body */
};

/* structure to keep track of sections in file */
struct section {
	Elf_Scn *sec_scn;		/* scn pointer */
	Elf32_Shdr *sec_shdr;		/* section header */
	Elf_Data *sec_data;		/* data associated with section */
};

extern struct section *esections;
#endif
