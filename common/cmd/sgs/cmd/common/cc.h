/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-cmd:common/cc.h	1.3"

#include	<stdio.h>
#include	<string.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<ccstypes.h>
#include	<unistd.h>
#include	<malloc.h>
#include	<varargs.h>
#ifdef __STDC__
#include	<stdlib.h>
#include	<wait.h>
#else
#define	WCOREFLG	0200
#endif
#include	"paths.h"
#include	"sgs.h"
#include	"machdep.h"

#define	PARGS	if(debug) {						\
			(void)printf("%scc: ", prefix);			\
			for(j=0;j<nlist[AV];j++)			\
				(void)printf(" '%s'",list[AV][j]);	\
			(void)printf("\n");				\
		}

/* performance statistics */

#ifdef PERF

#define	STATS( s )	if (stat == 1) {			\
				stats[ii].module = s;		\
				stats[ii].ttim = ttime;		\
				stats[ii++].perform = ptimes;	\
				if (ii > 25)			\
					pexit();		\
			}

extern	long	times();

struct	tbuffer {
	long	proc_user_time;
	long	proc_system_time;
	long	child_user_time;
	long	child_system_time;
};

struct	perfstat {
	char	*module;
	long	ttim;
	struct	tbuffer	perform;
};

extern struct tbuffer	ptimes;
extern struct perfstat	stats[30];
extern int	stat;
extern int	ii;
extern long	ttime;

#endif	/* PERF */

#define cunlink(x)	if (x)	(void)unlink(x);

#ifndef	ADDassemble
#define	ADDassemble()			/* empty */
#endif

/* tool names */

#define CRT1	"crt1.o"
#define CRTI	"crti.o"
#define VALUES	"values"
#define MCRT1	"mcrt1.o"
#define CRTN	"crtn.o"

#define PCRT1   "pcrt1.o"
#define PCRTI   "pcrti.o"
#define PCRTN	"pcrtn.o"

#define N_C0    "acomp"

#ifndef	N_OPTIM
#define	N_OPTIM	"optim"
#endif

#define N_PROF	"basicblk"
#define	N_AS	"as"
#define	N_LD	"ld"


/* list indexes */

#define	Xcp	1	/* index is the same if the compiler and cpp are merged */
#define	Xc0	1	/* index of list containing options to the compiler */
#define	Xc2	2	/* index of list containing options to the optimizer */
#define Xbb	3	/* index of list containing options to basicblk */
#define	Xas	4	/* index of list containing options to the assembler */
#define	Xld	5	/* index of list containing options to ld */
#define	AV	6	/* index of the argv list to be supplied to execvp */
#define	CF	7	/* index of list containing the names of files to be 
				compiled and assembled */
#ifndef	NLIST
#define	NLIST	8	/* total number of lists, can be overwritten in machdep.h */
#endif
#ifndef	Xoptim
#define	Xoptim	Xc2	/* adjust some options not to be appended to optimizor */
#endif

/* option string for getopt():
 *	OPTSTR     == machine independent options
 *	MACHOPTSTR == machine dependent options
 */

#define OPTSTR "A:B:Ccd:D:e:EfgGh:HI:K:l:L:o:OpPq:Q:Su:U:vVW:X:Y:z:#"
#ifndef MACHOPTSTR
#define MACHOPTSTR	""
#endif

#define OPTPTR(s)	(void)fprintf(stderr, s);


/* file names */

extern
char	*c_out,		/* compiler output */
	*as_in,		/* assembler input */
	*tmp2,
	*tmp3,
	*tmp4,
	*tmp5,
	*tmp6,
	*tmp7;


/* path names of the various tools and directories */

extern char	
	*passc0,
	*passc2,
	*passprof,
	*passas,
	*passld,
	*crtdir,
	*fplibdir,
	*libpath;


/* flags: ?flag corresponds to ? option */
extern
int	cflag,		/* compile and assemble only to .o; no load */
	Oflag,		/* optimizer request */
	Sflag,		/* leave assembly output in .s file */
	Vflag,		/* "-V" option flag */
	eflag,		/* error count */
	dsflag,		/* turn off symbol attribute output in compiler */
	dlflag,		/* turn of line number output in compiler */
	ilflag,		/* if *.il is present */
	pflag,		/* profile request for standard profiler */
	qpflag,		/* if = 1, then -p. if = 2, then -qp. else none */
	qarg,		/* profile request for xprof or lprof */
	gflag,		/* include libg.a on load line */
	debug,		/* cc command debug flag (prints command lines) */
	Eflag,		/* preprocess only, output to stdout */
	Pflag;		/* preprocess only, output to file.i */

extern char	*ilfile;/* will hold the .il file name */
extern char    *Parg;	/* will hold the -KPIC or -Kpic argument */

/* lists */

extern char	**list[NLIST];	/* list of lists */
extern int
	nlist[NLIST],	/* current index for each list */
	limit[NLIST];	/* length of each list */


extern char	*prefix;
extern int	sfile;		/* indicates current file in list[CF] is .s */

extern int	inlineflag;	/* inline file indicator. 1==ON */
extern int	independ_optim;	/* 1 indicates independent optimizor */
extern int	Ocount;		/* Oflag counting flag */
extern int	Add_Xc2_Qy;	/* Add -Qy to Xc2 ? 1== YES (default), 0== NO */
extern char	Xc0tmp[4];	/* "-KPIC" option for Xc0 (acomp) */

/* functions */

#ifndef __STDC__
extern  int     optind;         /* arg list index */
extern	int	opterr;         /* turn off error message */
extern	int	optopt;         /* current option char */
extern	int	access();
extern	int	unlink();
extern	int	execvp();
extern	int	creat();
extern	int	getopt();
extern  void    exit();
extern  char    *getenv();
extern  int     putenv();
extern  char    *optarg;        /* current option argument */
#endif	/* __SDTC__ */

/* machine independent routines */

extern char	*stralloc(); 
extern char	*setsuf();
extern char	*makename();
extern char	*passname();
extern void	addopt();
extern void	error();
extern int	callsys();
extern int	move();

/* machine dependent routines */

extern int	optimize();	/* pass arguments to optimizor */
extern int	optelse();	/* more legal options? 1==TRUE,0==FALSE */
extern int	Kelse();	/* more legal '-K xxx' options? 1==TRUE,0==FALSE */
extern int	Yelse();	/* more legal '-Y xxx' options? 1==TRUE,0==FALSE */
extern int	Welse();	/* more legal '-W xxx' options? */
				/* (-1)==FALSE, otherwise, returns optarg value */

extern void	init_mach_opt();/* add more machine dependent options, stage 1 */
extern void	add_mach_opt();	/* add more machine dependent options, stage 2 */
extern void	initvars();	/* machine dependent initialization routine */
extern void	mach_defpath();	/* make machine dependent default path */
extern void	AVmore();	/* append machine dependent options to acomp */
extern void	option_mach();/* machine dependent option usage messages */
