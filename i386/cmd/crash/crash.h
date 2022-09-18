/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/crash.h	1.4"
#ident	"$Header: crash.h 1.1 91/07/23 $"

#include "setjmp.h"
#include "string.h"

/* This file should include only command independent declarations */

#define ARGLEN 40	/* max length of argument */
#define NARGS 25	/* number of arguments to one function */
#define LINESIZE 256	/* size of function input line */

extern FILE *fp, *rp;	/* output file, redirect file pointer */
extern int Procslot;	/* current process slot number */
extern int Virtmode;	/* current address translation mode */
extern int mem;		/* file descriptor for dumpfile */
extern jmp_buf syn;	/* syntax error label */
extern struct var vbuf;	/* tunable variables buffer */
extern char *args[];	/* argument array */
extern int argcnt;	/* number of arguments */
extern int optind;	/* argument index */
extern char *optarg;	/* getopt argument */
extern long getargs();	/* function to get arguments */
extern long strcon();	/* function to convert strings to long */
extern long eval();	/* function to evaluate expressions */
extern struct syment *symsrch();	/* function for symbol search */
extern int tabsize;	/* Size of function table */
extern struct func functab[]; /* Function table */
extern char outfile[];	/* Holds name of output file for redirection */
extern int active;	/* Flag set if crash is examining an active system */
extern struct syment* findsp(); /* when using getksym to get symbol info, keeps
					track of manufactured syments */

/* function definition */
struct func {
	char *name;
	char *syntax;
	int (*call)();
	char *description;
};

extern int debugmode;

#define MAINSTORE 0

#ifdef __STDC__
#define GETDSYM(sym,fatal)	getdsym(&S_##sym,#sym,fatal);
#else
#define	GETDSYM(sym,fatal)	getdsym(&S_sym,"sym",fatal);
#endif

extern void getdsym();
extern char *vnotofsname();
