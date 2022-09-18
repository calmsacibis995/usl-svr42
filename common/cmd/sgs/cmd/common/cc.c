/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-cmd:common/cc.c	1.155"

/*===================================================================*/
/*                                                                   */
/*                 CC Command                                        */
/*                                                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*     The cc command parses the command line to set up command      */
/*     lines for and exec whatever processes (preprocessor,          */
/*     compiler, optimizer, assembler, link editor) are required.    */
/*                                                                   */
/*===================================================================*/

#include	"cc.h"

#ifdef PERF
struct tbuffer	ptimes;
struct perfstat	stats[30];
int	stat;
#endif	/* PERF */

char	*c_out, *as_in;
char	*tmp2, *tmp3, *tmp4, *tmp5, *tmp6, *tmp7;

char	*passc0, *passc2, *passprof, *passas, *passld,
	*crtdir, *fplibdir, *libpath;

int	cflag, Oflag, Sflag, Vflag, eflag, dsflag, dlflag, ilflag,
	pflag, qpflag, qarg, gflag, debug;

int	Eflag	= 0;	/* preprocess only, output to stdout */
int	Pflag	= 0;	/* preprocess only, output to file.i */

char	*ilfile, *Parg;
char	**list[NLIST];
int	nlist[NLIST], limit[NLIST];

char	*prefix;
int	sfile;
int	inlineflag;
int	independ_optim;
int	Ocount;
int	Add_Xc2_Qy;
char	Xc0tmp[4];

/* static common variables */

#ifdef PERF
static	FILE	*perfile;
long	ttime;
int	ii = 0;
#endif

/* number of args always passed to ld; used to determine whether
 * or not to run ld when no files are given */

#define	MINLDARGS	3
#define DFT_X_OPT	"-Xt" /* -Xt is the default */

static char	
	*ld_out = NULL,	/* link editor output (object file) */
	*libdir = NULL,
	*llibdir = NULL;

static	char	*profdir= LIBDIR;


static int
	Qflag   = 1,    /* turn on version stamping - turn off if -Qn 
				is specified*/
	Xwarn	= 0,	/* warn user if more than one -X options were 
				given with differing arguments */	
	Gflag	= 0,	/* == 0 include start up routines else don't */
	Kabi	= 0;	/* 1 if Kminabi is given */

static char	*Xarg	= NULL; /* will hold the -X flag and argument 
				  Xt is default for this release */
static	char	*earg	= NULL; /* will hold the -e flag and argument */
static	char	*harg	= NULL; /* will hold the -h flag and argument */
static	char	*nopt;		/* current -W flag option */

/* lists */

static int
	nxo	= 0,	/* number of .o files in list[Xld] */
	nxn	= 0,	/* number of non-.o files in list[Xld] */
	questmark = 0,	/* specify question mark on command line */
	argcount;	/* initial length of each list */


static	char	*crt	= CRT1;
static	char	*values = VALUES;	/* name of the values-X[cat].o file */
static	char	*optstr;	/* holds option string */

static	char
	*getsuf(),
	*copy(),
	*getpref(),
	*compat_YL_YU();

static void
	idexit(),
	initialize(),
	linkedit(),
	mk_defpaths(),
	chg_pathnames(),
	mktemps(), 
	process_lib_path(),
	dexit(),
	option_indep();

static int
	nodup(),
	getXpass(),
	compile(),
	profile(),
	assemble();


main (argc, argv)
	int argc;
	char *argv[];
{
	int	c;		/* current option char from getopt() */
	char	*t, *s, *p;	/* char ptr temporary */
	int	done;		/* used to indicate end of arguments passed by -W */
	int	i;		/* loop counter, array index */
	char 	*chpiece = NULL,	/* list of pieces whose default location has
					   been changed by the -Y option */
		*npath = NULL;	/* new location for pieces changed by -Y option */

	opterr = 0;
#ifdef PERF
	if ((perfile = fopen("cc.perf.info", "r")) != NULL) {
		fclose(perfile);
		stat = 1;
	}
#endif

	prefix = getpref( argv[0] );

	/* initialize the lists */
	argcount= argc + 6;
	c = sizeof(char *) * argcount;
	for (i = 0; i < NLIST; i++) {
		nlist[i] = 0;
		list[i] = (char **)stralloc(c);
		limit[i]= argcount;
	}
	initialize();

	setbuf(stdout, (char *) NULL);	/* unbuffered output */

	while (optind < argc) {
		c = getopt(argc, argv, optstr);
		switch (c) {

		case 'c':       /* produce .o files only */
                        cflag++;
                        break;

		case 'C':       /* tell compiler to leave comments in (lint) */
                        addopt(Xcp,"-C");
                        break;

		case 'D': 	/* Define name with value def, otherwise 1 */
			t = stralloc(strlen(optarg)+2);
                        (void) sprintf(t,"-%c%s",c,optarg);
                        addopt(Xcp,t);
                        break;

		case 'e':       /* Make name the new entry point in ld */
                                /* Take last occurence */
                        earg = stralloc(strlen(optarg) + 3);
                        (void) sprintf(earg,"-%c%s",optopt,optarg);
                        break;
		case 'E':       /* run only preprocessor part of
                                   compiler, output to stdout */
                        Eflag++;
                        addopt(Xcp,"-E");
                        cflag++;
                        break;

		case 'f':       /* floating point interpreter */
                        error('c', "-f option ignored on this processor\n");
                        break;

		case 'g':       /* turn on symbols and line numbers */
                        dsflag = dlflag = 0;
                        gflag++;
                        break;

		case 'H':       /* preprocessor - print pathnames of 
				   included files on stderr */
                        addopt(Xcp,"-H");
                        break;

		case 'I':
                        t = stralloc(strlen(optarg)+2);
                        (void) sprintf(t,"-%c%s",c,optarg);
                        addopt(Xcp,t);
                        break;

		case 'l':       /* ld: include library */
                case 'L':       /* ld: alternate lib path prefix */
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xld,t);
                        break;
		
		case 'o':       /* object file name */
                        if (!optarg[0])
				break;
			ld_out = optarg;
			s = getsuf(ld_out);
			if ( !strcmp(s,"c") || !strcmp(s,"i") || !strcmp(s,"s")
				|| (inlineflag && !strcmp(s, "il") )) {
				error('e', "Illegal suffix for object file %s\n",
					ld_out);
				exit(1);
			}
                        break;

		case 'O':       /* invoke optimizer */
			Oflag += Ocount;
                        break;

		case 'p':	/* standard profiler */

			pflag++;
			qpflag=1;
			crt = MCRT1;
			if ( qarg != 0) {
				error('w',
					"using -p, ignoring the -q%c option\n",qarg);
				qarg= 0; /* can only have one type 
						of profiling on */
			}
			break;

		case 'P':       /* run only preprocessor part of
                                   compiler, output to file.i */
                        Pflag++;
                        addopt(Xcp,"-P");
                        cflag++;
                        break;

		case 'q':	/* xprof, lprof or standard profiler */

			if (strcmp(optarg,"p") == 0) {
				pflag++;
				qpflag=2;
				crt = MCRT1;
				if ( qarg != 0) {
                                	error('w',
					"using -qp, ignoring the -q%c option\n",
						qarg);
                                	qarg= 0; /* can only have one 
						    type of profiling on */
                        		}

			} else if (strcmp(optarg,"l")==0 || strcmp(optarg,"x")==0) {
				qarg = optarg[0];
				if (pflag != 0) {
					if (qpflag == 1)
					   error('w',
					   "using -q%c, ignoring the -p option\n",
						qarg);
					else
					   error('w',
					   "using -q%c, ignoring the -qp option\n"
						,qarg);

					pflag= 0;
				}
			} else {
				error('e', "No such profiler %sprof\n",optarg);
				exit(1);
			}
			break;

		case 'S':	/* produce assembly code only */
			Sflag++;
			cflag++;
			break;
		case 'u':       /* ld: enter sym arg as undef in sym tab */
                        addopt(Xld,"-u");
                        addopt(Xld,optarg);
                        break;

		case 'U':
                        t = stralloc(strlen(optarg)+2);
                        (void) sprintf(t,"-%c%s",c,optarg);
                        addopt(Xcp,t);
                        break;

                case 'V':       /* version flag or ld VS flag */
			if ( !Vflag ) {
                        	addopt(Xcp,"-V");
                        	addopt(Xas,"-V");
                        	addopt(Xld,"-V");
                        	error('c', "%scc: %s %s\n",
                                	prefix, CPL_PKG, CPL_REL);
			}
			Vflag++;
                        break;

		case 'W':
			if (optarg[1] != ',' || optarg[2] == '\0') {
				error('e', "Invalid subargument: -W%s\n", optarg);
				exit(1);
			}
			if ((i = getXpass(optarg, "-W")) == -1) {
				error('e', "Invalid subargument: -W%s\n", optarg);
				exit(1);
			}
			/*
			 * Added capacility to -W option to pass arguments
			 * which themselves contain a comma.
			 * The comma in the argument must be preceded by a
			 * '\' to distinguish it from commas used as
			 * delimiters in the -W option.
			 */
			t = optarg;
			t+=2;
			done=0;
			while (!done) {
				p=t;
				while (((s = strchr(p,',')) != NULL) &&
								(*(s-1) == '\\')) {
					p=s;
					s--;
					while (*s != '\0') {
						*s = *(s+1);
						s++;
					}
				}
				if (s != NULL)
					*s = '\0';
				else
					done=1;
				nopt =stralloc(strlen(t)+1);
				(void) strcpy(nopt, t);
				addopt(i,nopt);
				t+= strlen(t) + 1;
			}
			break;

		case 'Y':
			if (((chpiece=strtok(optarg,",")) != optarg) ||
				((npath=strtok(NULL,",")) == NULL)) {
				error('e', "Invalid argument: -Y %s\n",optarg);
				exit(1);
			} else if ((t=strtok(NULL," ")) != NULL) {
				error('e', "Too many arguments: -Y %s,%s,%s\n",
					chpiece, npath,t);
				exit(1);
			}
			chg_pathnames(chpiece, npath);
			break;

		case 'A':       /* preprocessor - asserts the predicate 
				   and may associate the pp-tokens with 
				   it as if a #assert */
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xcp,t);
                        break;

                case 'B':       /* Govern library searching algorithm in ld */
                        if((strcmp(optarg,"dynamic") == 0) ||
			   (strcmp(optarg,"static") == 0)  ||
			   (strcmp(optarg,"symbolic") == 0))
			{
                        	t = stralloc(strlen(optarg) + 3);
                        	(void) sprintf(t,"-%c%s",optopt,optarg);
                        	addopt(Xld,t);
                        } 
			else
                        	error('w', "illegal option -B%s\n",optarg);
                        break;


                case 'd':       /* Govern linking: -dy dynamic binding;
                                   -dn static binding
                                */

                        switch (optarg[0]) {
                                case 'y':
                                        addopt(Xld,"-dy");
                                        break;
                                case 'n':
                                        addopt(Xld,"-dn");
                                        break;
                                default:
                                        error('e', "illegal option -d%c\n"
						,optarg[0]);
					exit(1);
                        }
			if(optarg[1]) {
				error('e', "illegal option -d%s\n", optarg);
				exit(1);
			}

                        break;

		case 'G':       /* used with the -dy option to direct linker
                                   to produce a shared object. The start up
                                   routine (crt1.o) shall not be called */

                        Gflag++;
                        addopt(Xld, "-G");
                        break;

		case 'h':       /* ld: Use name as the output filename in the
                                   link_dynamic structure. Take last occurence.
                                */
                        harg = stralloc(strlen(optarg) + 3);
                        (void) sprintf(harg,"-%c%s",optopt,optarg);
                        break;

		case 'Q':       /* add version stamping information */
                        switch (optarg[0]) {
                                case 'y':
                                        Qflag = 1;
                                        break;
                                case 'n':
                                        Qflag = 0;
                                        break;
                                default:
                                        error('e', "illegal option -Q %c\n",
						optarg[0]);
                                        exit(1);
                        }
                        break;

                case 'v':       /* tell comp to run in verbose mode */
                        addopt(Xc0,"-v");
                        break;

                case 'X':       /* ANSI C options */
                        if (Xarg != NULL && Xarg[2] != optarg[0])
                                Xwarn = 1;

                        switch (optarg[0]) {
                                case 't':
                                        Xarg = "-Xt";
                                        break;
                                case 'a':
                                        Xarg = "-Xa";
                                        break;
                                case 'c':
                                        Xarg = "-Xc";
                                        break;
                                default:
                                        error('e', "illegal option -X%c\n",
						optarg[0]);
                                        exit(1);
                        }
                        break;

		case 'z':       /* turn on asserts in the linker */
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xld,t);
                        break;

		case 'K':       /* optimize for size or speed */
                        while ((s=strtok(optarg, ",")) != NULL)
                        {
                                if (strcmp(s, "PIC")==0)
                                        Parg = "-KPIC";
                                else if (strcmp(s, "pic")==0)
                                        Parg = "-Kpic";
				else if (strcmp(s, "minabi") == 0) {
					addopt(Xld,"-I");
					addopt(Xld,LDSO_NAME);
					Kabi = 1;
				}
                                else if ( !Kelse(optarg) ) {
					/*
					 * Kelse(), machine dependent routine,
					 * to cover more machine-specific '-Kxx'
					 * options.
					 */
                                        error('e',
                                        "Illegal argument to -K flag, '-K %s'\n"
                                        	,optarg);
                                    	exit(1);
                                }
                                optarg = NULL;
                        }
                        break;

		case '#':	/* cc command debug flag */
			debug++;
			break;

		default:
			/*
			 * optelse(), machine dependent routine, to cover
			 * more machine-specific options.
			 * If no more opt char in optstr, then pass to ld.
			 */
			if (optopt == '?' )
				questmark = 1;
			else if ( optelse(optopt, optarg) )
				break;
			if ( strchr(optstr,optopt) != NULL ) {
				error('e', "Option -%c requires an argument\n",
					optopt);
				exit(1);
			}
			t = stralloc( 3 );
			(void) sprintf(t,"-%c",optopt);
			addopt(Xld,t);
			break;

		case EOF:	/* file argument */
			if ((t = argv[optind++]) == NULL) /* no file name */
				break;
			s = getsuf(t);
			if (!strcmp(s,"c") || !strcmp(s,"i") || !strcmp(s,"s")
			    || Eflag) {
				/* Eflag has to be checked here.
				 * sdb calls "cc -E" to preprocess its C++
				 * codes and then passed its outputs to
				 * cfront. If Eflag were not checked here,
				 * then no outputs would be passed to cfront.
				 */
				addopt(CF,t);
				t = setsuf(t, 'o');
			}
			else if ( inlineflag && !strcmp(s, "il") ) {
				ilflag++;
				ilfile = t;
				break;
			}
			if (nodup(list[Xld], t)) {
				addopt(Xld,t);
				if ( !strcmp(getsuf(t), "o") )
					nxo++;
				else
					nxn++;
			}
			break;
		} /* end switch */
	} /* end while */

	if ( (nxo == 0) && (nxn == 0) ) {
		error('c', "Usage: cc [ -%s ] files ...\n", optstr);
		if (questmark)	/* detail option information */
			option_indep();
		exit(1);
		}

	/*
	 * Two purposes for init_mach_opt():
	 * 1. add machine dependent options which should be always put here.
	 * 2. adjust some variables to force producing different bahavior.
	 */
	init_mach_opt();

	if (Vflag)	/* Vflag may be cleaned up in init_mach_opt() */
		addopt(Xc2,"-V");
        if (earg != NULL)
                addopt(Xld, earg);
	if (harg != NULL)
		addopt(Xld, harg);
	if (Parg != NULL)
		addopt(Xc0, Xc0tmp);

	/* if -g and -O are given, disable -g */
	if (gflag & Oflag & !qarg)
	{
		gflag = 0;
		dsflag = dlflag = 1;
		
		error('w', 
		"debugging and optimization mutually exclusive; -g disabled\n");
	}

	/* if -q option is given, make sure basicblk exists */
	if (qarg) {
		if(Oflag) {
			Oflag = 0;
			error('w',
			"%cprof and optimization mutually exclusive; -O disabled\n",
			qarg);
		}
		passprof= makename(profdir,prefix,N_PROF);
		if ((debug <= 2) && (access(passprof, 0) == -1)) {
			error('e', "%cprof is not available\n",qarg);
			exit(1);
		}
		crt = PCRT1;
		dsflag = dlflag = 0;
		gflag++;
		addopt(Xld,"-lprof");
		addopt(Xld,"-lelf");
		addopt(Xld,"-lm");
	}

	/* if -o flag was given, the output file shouldn't overwrite an input file */
	if (ld_out != NULL) {
		if (!nodup(list[Xld], ld_out)) {
			error('e', "-o would overwrite %s\n",ld_out);
			exit(1);
		}
	}

	if (signal(SIGHUP, SIG_IGN) == SIG_DFL)
		(void) signal(SIGHUP, idexit);
	if (signal(SIGINT, SIG_IGN) == SIG_DFL)
		(void) signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		(void) signal(SIGTERM, idexit);

	if (nlist[CF] && !(Pflag || Eflag)) /* more than just the preprocessor is
			        	     * running, so temp files are required */
		mktemps();

	if (eflag)
		dexit();

	mk_defpaths();

	/*
	 * To add more machine dependent options which are difficult to
	 * or should not be handled by init_mach_opt().
	 */
	add_mach_opt();

	if (Parg != NULL)
		addopt(Xc2, Parg);
	if (Qflag) {
		addopt(Xcp,"-Qy");
		addopt(Xas,"-Qy"); 
		addopt(Xld,"-Qy");
		if (Oflag && Add_Xc2_Qy) 
			addopt(Xc2,"-Qy");
	}

	if ( Xarg != NULL) { /* if more then one -X option and each has a differnt */
		if ( Xwarn)  /* argument, then warning */
		error('w', "using %s, ignoring all other -X options\n",Xarg);
		addopt(Xcp,Xarg);
		if (Oflag)
			addopt(Xoptim,Xarg);
	}	
	else
		Xarg = DFT_X_OPT;

	/* process each file (name) in list[CF] */

	for (i = 0; i < nlist[CF]; i++) {
		if (nlist[CF] > 1)
			error('c', "%s:\n", list[CF][i]);
		s = getsuf(list[CF][i]);
		sfile = (strcmp(s, "s") == 0);
		if (sfile && !Eflag && !Pflag && !Sflag) {
			as_in = list[CF][i];
			(void) assemble(i);
			continue;
		}
		else if (sfile) {
			if (Sflag)
				error('w', "Conflicting option -S with %s\n",
					list[CF][i]);
			continue;
		}
		if ( strcmp(getsuf(list[CF][i]), "i") == 0 ) {
			if ( Eflag || Pflag ) {
				error('w', "Conflicting option -%c with %s\n",
					Eflag ? 'E' : 'P', list[CF][i]);
				continue;
			}
		}

		if (!compile(i))
			continue;

		if (Oflag || ilflag)
			(void) optimize(i);

		if (passprof)
			(void) profile(i);

		if (!Sflag)
			(void) assemble(i);

	} /* end loop */

	if (!eflag && !cflag)
		linkedit();

	dexit();
	/*NOTREACHED*/
}


/*===================================================================*/
/*                                                                   */
/*                  COMPILER 					     */
/*                                                                   */
/*===================================================================*/

static int
compile (i)
	int i;
{
	int j;
	int front_ret;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_C0));
	addopt(AV,"-i");
	addopt(AV,list[CF][i]);
	addopt(AV,"-o");
	if (Eflag || Pflag)
		addopt(AV, Eflag ? "-" : setsuf(list[CF][i], 'i') );
	else {
		if (Sflag && !qarg) {
			if (Oflag && independ_optim)
				as_in = tmp2;
			else
				as_in = setsuf(list[CF][i], 's');
		} else
			as_in = tmp2;
		addopt(AV,c_out = as_in);
	}

	addopt(AV,"-f");
	addopt(AV,list[CF][i]);

	if(!Eflag && !Pflag)
	{
		if (dsflag)
			addopt(AV,"-ds");
		if (dlflag)
			addopt(AV,"-dl");
	}
	for (j = 0; j < nlist[Xc0]; j++)
		addopt(AV,list[Xc0][j]);

	AVmore(); /* Appended more options to acomp command */

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	front_ret = callsys( passc0, list[AV] );
	if ((Eflag || Pflag) && front_ret == 0 )
		return(0);
	else
	if (front_ret >= 1) {
		cflag++;
		eflag++;
		if (Pflag){
			cunlink(setsuf(list[CF][i], 'i'));
			}
		else {
			cunlink(c_out);
			}
		return(0);
	}

#ifdef PERF
	STATS("compiler ");
#endif
	return(1);
}


/*===================================================================*/
/*                                                                   */
/*                      PROFILER                                     */
/*                                                                   */
/*===================================================================*/

static int
profile(i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_PROF));
	if (qarg == 'l')
		addopt(AV,"-l");
	else
		addopt(AV,"-x");

	if (Qflag)  /* By default, version stamping option is passed */
		addopt(AV,"-Qy"); 
	for (j=0; j < nlist[Xbb]; j++)
		addopt(AV,list[Xbb][j]);
	addopt(AV,c_out);
	addopt(AV,as_in = Sflag ? setsuf(list[CF][i], 's') : tmp7);
	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	if (callsys(passprof, list[AV])) {
		if (Sflag) {
			if (move(c_out, as_in)) { /* move failed */
				cunlink(c_out);
				return(0);
			}
		}
		else {
			cunlink(as_in);
			as_in = c_out;
		}
		error('w', "Profiler failed, '-q %c' ignored for %s\n",
			qarg, list[CF][i]);
	}
		

#ifdef PERF
	STATS("profiler");
#endif

	return(1);
}
	
/*===================================================================*/
/*                                                                   */
/*                    ASSEMBLER                                      */
/*                                                                   */
/*===================================================================*/

static int
assemble (i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_AS));

	addopt(AV,"-o");
	addopt(AV,setsuf(list[CF][i], 'o'));
	for (j = 0; j < nlist[Xas]; j++)
		addopt(AV,list[Xas][j]);
	addopt(AV,as_in);

	ADDassemble();

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	if (callsys(passas, list[AV])) {
		cflag++;
		eflag++;
		return(0);
	}

#ifdef PERF
	STATS("assembler");
#endif

	return(1);
}


/*===================================================================*/
/*                                                                   */
/*                LINKAGE EDITOR                                     */
/*                                                                   */
/*===================================================================*/

static void
linkedit ()
{
	int j;
	char *t;
	static	char *cclibs, *cclibs1, *cclibs2;
	static char whites[] = "\n\t\b \f\v";
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_LD));

		if ( Gflag == 0)
			addopt(AV,makename(crtdir,prefix,crt));

        if (Gflag && ( qarg == 'l') ) /* for C++ */
		addopt(AV,makename(crtdir,prefix,PCRTI));
	else
		addopt(AV,makename(crtdir,prefix,CRTI));

	t = stralloc( strlen(values) + strlen(Xarg) + 2);
	(void) sprintf(t,"%s%s.o",values,Xarg);
	addopt(AV,makename(crtdir,prefix,t));

	if (ld_out != NULL)
        {
                addopt(AV,"-o");
                addopt(AV,ld_out);
        }

	for (j = 0; j < nlist[Xld]; j++) /* app files, opts, and libs */
		addopt(AV,list[Xld][j]);

	cclibs = getenv("CCLIBS");
	if (cclibs != NULL)
	{
		if ( (cclibs1 = strtok(cclibs, whites)) != NULL)
			addopt(AV,cclibs1);

		while ( (cclibs2 = strtok(NULL, whites)) != NULL )
			addopt(AV,cclibs2);
	}

	if (Gflag == 0)
	{
		addopt(AV,"-lc");
	}


	if (Gflag && (qarg == 'l')) /* for C++ */
		addopt(AV,makename(crtdir,prefix,PCRTN));
	else
		addopt(AV,makename(crtdir,prefix,CRTN));
	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	if (nlist[AV] > MINLDARGS) /* if file given or flag set by user */
	{
		PARGS;
		eflag |= callsys(passld, list[AV]);
	}

	if ((nlist[CF] == 1) && (nxo == 1) && (eflag == 0) && (debug <= 2) )
		/* delete .o file if single file compiled and loaded */
		cunlink(setsuf(list[CF][0], 'o'));

#ifdef PERF
	STATS("link edit");
#endif

}


/* 
   chg_pathnames() overrides the default pathnames as specified by the -Y option
*/

static void
chg_pathnames(chpiece, npath)
char *chpiece;
char *npath;
{
	char	*t;
	char	*topt;

	for (t = chpiece; *t; t++)
		switch (*t) {
		case 'p':
		case '0':
			passc0 = makename( npath, prefix, N_C0 );
			break;
		case '2':
			if (independ_optim)
				passc2 = makename( npath, prefix, N_OPTIM );
			break;
		case 'b':
			profdir= stralloc(strlen(npath));
			(void) strcpy(profdir,npath);
			break;
		case 'a':
			passas = makename( npath, prefix, N_AS );
			break;
		case 'l':
			passld = makename( npath, prefix, N_LD );
			break;
		case 'S':
			crtdir= stralloc(strlen(npath));
			(void) strcpy(crtdir,npath);
			break;
		case 'I':
			topt = stralloc(strlen(npath)+4);
			(void) sprintf(topt,"-Y%s",npath);
			addopt(Xcp,topt);
			break;

		case 'L':
			if(libpath) {
				error('e', "-YL can not be used with -YP\n");
				exit(1);
			}
			libdir = stralloc(strlen(npath));
			(void) strcpy(libdir,npath);
			break;
		case 'U':
			if(libpath) {
				error('e', "-YU can not be used with -YP\n");
				exit(1);
			}
			llibdir = stralloc(strlen(npath));
			(void) strcpy(llibdir,npath);
			break;
		case 'P':
			if(libdir) {
				error('e', "-YP can not be used with -YL\n");
				exit(1);
			}
			if(llibdir) {
				error('e', "-YP may not be used with -YU\n");
				exit(1);
			}
			if(fplibdir) {
				error('e', "-YP may not be used with -YF\n");
				exit(1);
			}
			libpath = stralloc(strlen(npath));
			(void) strcpy(libpath,npath);
			break;

			
		default: /* machine-specific '-Yx' options or error */
			if ( Yelse( (int)*t, npath ) )
				break;
			error('e', "Bad option -Y %s,%s\n",chpiece, npath);
			exit(1);
		} /* end switch */
}

static void
mk_defpaths()
{
	register char 	*nlibpath;
	int x,y;

	/* make defaults */
	if (!crtdir) {
		crtdir = LIBDIR;
	}
	if(!libpath)
		libpath = LIBPATH;
	if(libdir || llibdir) {
		/* fix NULL pointer problem */
		if(libdir)
			x = strlen(libdir);
		else
			x = 0;
		if(llibdir)
			y = strlen(llibdir);
		else
			y = 0;
		nlibpath = stralloc(strlen(libpath) + x + y);
		process_lib_path(libpath,nlibpath);
		libpath = nlibpath;
	}

	if (!passc0)
		passc0 = makename( LIBDIR, prefix, N_C0 );
	if (!passc2)
		passc2 = makename( LIBDIR, prefix, N_OPTIM );
	if (!passas)
		passas = makename( BINDIR, prefix, N_AS );
	if (!passld)
		passld = makename( BINDIR, prefix, N_LD );

	/*
	 * mach_defpath is empty routine by default.
	 * If there is any machine dependent default path should
	 * be made, then could be handled by mach_defpath().
	 */
	mach_defpath();

	if(Kabi) {
		nlibpath = stralloc(strlen(libpath) + strlen(ABILIBDIR)+1);
		(void) sprintf(nlibpath,"%s:%s",ABILIBDIR,libpath);
		libpath = nlibpath;
	}
		
	if (pflag) {
		int i;
		char * cp;
		char * cp2;

		nlibpath = libpath;
		/* count number of paths */
		for(i=0; ; i++) {
			nlibpath = strchr(nlibpath,':');
			if(nlibpath == 0) {
				i++;
				break;
			}
			nlibpath++;
		}

		/* get enough space for path/libp for every path in libpath +
			enough for the :s */
		nlibpath = stralloc(2 * strlen(libpath) - 1 + i*sizeof("./libp:") );

		cp2 = libpath;
		while(cp =  strchr(cp2,':')) {
			if(cp == cp2)
				(void) strcat(nlibpath,"./libp:");
			else {
				(void) strncat(nlibpath,cp2,cp - cp2);
				(void) strcat(nlibpath,"/libp:");
			}
			cp2 = cp + 1;
		}

		if(*cp2 == '\0')
			(void) strcat(nlibpath,"./libp:");
		else {
			(void) strcat(nlibpath,cp2);
			(void) strcat(nlibpath,"/libp:");
		}

		(void) strcat(nlibpath,libpath);
		libpath = nlibpath;

	}
	addopt(Xld,"-Y");
	nlibpath = stralloc(strlen(libpath) + 2);
	(void) sprintf(nlibpath,"P,%s",libpath);
	addopt(Xld,nlibpath);
}


/* return the prefix of "cc" */

static char *
getpref( cp )
	char *cp;	/* how cc was called */
{
	static char	*tprefix;
	int		cmdlen,
			preflen;
	char		*prefptr,
			*ccname;

	ccname= "cc";
	if ((prefptr= strrchr(cp,'/')) == NULL)
		prefptr=cp;
	else
		prefptr++;
	cmdlen= strlen(prefptr);
	preflen= cmdlen - strlen(ccname);
	if ( (preflen < 0 )		/* if invoked with a name shorter
					   than ccname */
	    || (strcmp(prefptr + preflen, ccname) != 0)) {
		error('e', "command name must end in \"%s\"\n", ccname);
		exit(1);
		/*NOTREACHED*/
	} else {
		tprefix = stralloc(preflen+1);
		(void) strncpy(tprefix,prefptr,preflen);
		return(tprefix);
	}
}

/* initialize all common and machine dependent variables */

static void
initialize()
{
#ifdef PERF
	stat = 0;
#endif
	tmp2 = tmp3 = tmp4 = tmp5 = tmp6 = tmp7 = NULL;
	passc0 = passc2 = passprof = passas = passld = NULL;
	c_out = as_in = NULL;
	crtdir	= NULL;
	fplibdir = NULL;
	libpath = NULL;
	ilfile = NULL;
	Parg = NULL;
	Vflag = Oflag = Sflag = 0;
	cflag = eflag = pflag = gflag = qpflag = ilflag = 0;
	dsflag = dlflag = 1;
	qarg = 0;
	debug = 0;
	sfile = 0;

	/* setup optstr:
	 * optstr = combine OPTSTR and MACHOPTSTR together
	 */
	optstr = stralloc( strlen(OPTSTR) + strlen(MACHOPTSTR) );
	(void) strcpy(optstr, OPTSTR);
	(void) strcat(optstr, MACHOPTSTR);

	/*
	 * The following variables are initialized to default value which
	 * are used on most machines, however, they might be overwritten
	 * in machine-specific initvars() routines for some special usage.
	 */

	/* defined inline file are not available */
	inlineflag = 0;

	/* indicates optimizor is available */
	independ_optim = 1;

	/* defines the number Oflag would be accumulated */
	Ocount = 1;

	/* Add "-Qy"/"-Qn" to Xc2 (optimizor) ? */
	Add_Xc2_Qy = 1;

	/*
	 * -KPIC and -Kpic are always passed to acomp as "-2k".
	 *
	 * Here "-2k" is copied to Xc0tmp (temporary Xc0, only 4 bytes)
	 * and ready to be appended to Xc0 if Parg is not NULL.
	 *
	 * However, some machines may use different string other "-2k"
	 * (e.g. sparc uses "-2K"), then Xc0tmp could be overwritten
	 * to appended correct string to acomp.
	 */
	(void) strcpy(Xc0tmp, "-2k");

	/* machine-specific variables initialization routine */
	initvars();
}

/* Add the string pointed to by opt to the list given by list[lidx]. */

void
addopt(lidx, opt)
int	lidx;	/* index of list */
char	*opt;  /* new argument to be added to the list */
{
	/* check to see if the list is full */
	if ( nlist[lidx] == limit[lidx] - 1 ) {
		limit[lidx] += argcount;
		if ((list[lidx]= (char **)realloc((char *)list[lidx],
					limit[lidx]*sizeof(char *))) == NULL) {
			error('e', "Out of space\n");
			dexit();
		}
	}

	list[lidx][nlist[lidx]++]= opt;
}

/* make absolute path names of called programs */

char *
makename( path, prefix_cmd, name )
	char *path;
	char *prefix_cmd;
	char *name;
{
	char	*p;

	p = stralloc(strlen(path)+strlen(prefix_cmd)+strlen(name)+1);
	(void) strcpy( p, path );
	(void) strcat( p, "/" );
	(void) strcat( p, prefix_cmd );
	(void) strcat( p, name );

	return( p );
}

/* make the name of the pass */

char *
passname(prefix_cmd, name)
	char *prefix_cmd;
	char *name;
{
	char	*p;

	p = stralloc(strlen( prefix_cmd ) + strlen( name ));
	(void) strcpy( p, prefix_cmd );
	(void) strcat( p, name );
	return( p );
}

/*ARGSUSED0*/
static void
idexit(i)
	int i;
{
        (void) signal(SIGINT, idexit);
        (void) signal(SIGTERM, idexit);
        eflag = i;
        dexit();
}


static void
dexit()
{
	if (!Pflag) {
		if (qarg)
			cunlink(tmp7);
		if (ilflag)
			cunlink(tmp6);
		if (Oflag && independ_optim)
			cunlink(tmp5);
		cunlink(tmp4);
		cunlink(tmp3);
		cunlink(tmp2);
	}
#ifdef PERF
	if (eflag == 0)
		pwrap();
#endif
	exit(eflag);
}


/* VARARGS */
void
error(c, fmt, va_alist)
	char c;
	char *fmt;
	va_dcl
{
	va_list ap;

	va_start(ap);

	switch (c) {
	case 'e':	/* Error Messages */
		cflag++;
		eflag++;
		(void) fprintf(stderr, "%scc: Error: ", prefix);
		break;
	case 'w':	/* Warning Messages */
		(void) fprintf(stderr, "%scc: Warning: ", prefix);
		break;
	case 'c':	/* Common Messages */
	default:
		/* do nothing */
		break;
	}

	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	return;
}




static char *
getsuf(as)
	char *as;
{
	register char *s;
	static char *empty = "";

	if ((s = strrchr(as, '/')) == NULL)
		s = as;
	else
		s++;

	if ((s = strrchr(s, '.')) == NULL)
		return(empty);
	else if ( *(++s) == '\0' )
		return(empty);
	else
		return(s);
}

char *
setsuf(as, ch)
	char *as;
	char ch;
{
	register char *s, *s1;
	register char *t1;

	s1 = copy(as);
	if ((s = strrchr(s1, '/')) == NULL)
		t1 = s1;
	else
		t1 = ++s;

	if ((s = strrchr(t1, '.')) == NULL || *(s+1) == '\0')
		s1 += (strlen(s1) - 1);
	else
		s1 = ++s;
	*s1++ = ch;
	*s1 = '\0';

	return(t1);
}

int
callsys(f, v)
	char f[], *v[];
{
	register pid_t pid, w;
	char *tf;
	int status;

	(void) fflush(stdout);
	(void) fflush(stderr);

	if (debug >= 2) {	/* user specified at least two #'s */
		error('c', "%scc: process: %s\n", prefix, f);
		if (debug >= 3)	/* 3 or more #'s:  don't exec anything */
			return(0);
	}

#ifdef PERF
	ttime = times(&ptimes);
#endif

	if ((pid = fork()) == 0) {
		(void) execv(f, v);
		error('e', "Can't exec %s\n", f);
		exit(1);
	}
	else
		if (pid == -1) {
			error('e', "Process table full - try again later\n");
			eflag = 1;
			dexit();
		}
	while ((w = wait(&status)) != pid && w != -1) ;

#ifdef PERF
	ttime = times(&ptimes) - ttime;
#endif

	if (w == -1) {
		error('e', "Lost %s - No child process!\n", f);
		eflag = w;
		dexit();
	}
	else {
		if (((w = status & 0xff) != 0) && (w != SIGALRM)) {
			if (w != SIGINT) {
				if (w & WCOREFLG)
					error('e',"Process %s core dumped with signal %d\n",f,(w & 0x7f));
				else
					error('e', "Process %s exited with status %d \n",
					f, status );
			}
			if (  (tf = strrchr(f,'/'))  == NULL )
				tf=f;
			else
				tf++;
			if ( strcmp(tf,passname(prefix,N_OPTIM)) == 0 ) {
				return(status);
				}				
			else {
				eflag = status;
                        	dexit();	
				}
		}
	}
	return((status >> 8) & 0xff);
}

static int
nodup(l, os)
	char **l, *os;
{
	register char *t;

	if ( strcmp(getsuf(os), "o") )
		return(1);
	while(t = *l++) {
		if (strcmp(t,os) == 0)
			return(0);
	}
	return(1);
}

int
move(from, to)
	char *from, *to;
{
	list[AV][0] = "mv";
	list[AV][1] = from;
	list[AV][2] = to;
	list[AV][3] = 0;
	if (callsys("/bin/mv", list[AV])) {
		error('w', "Can't move %s to %s\n", from, to);
		return(1);
	}
	return(0);
}

static char *
copy(s)
	register char *s;
{
	register char *ns;

	ns = stralloc(strlen(s));
	return(strcpy(ns, s));
}


char *
stralloc(n)
	int n;
{
	register char *s;

	if ((s = (char *)calloc((unsigned)(n+1),1)) == NULL) {
		error('e', "out of space\n");
		dexit();
	}
	return(s);
}

static void
mktemps()
{
	tmp2 = tempnam(TMPDIR, "ctm2");
	tmp3 = tempnam(TMPDIR, "ctm3");
	tmp4 = tempnam(TMPDIR, "ctm4");
	tmp5 = tempnam(TMPDIR, "ctm5");
	tmp6 = tempnam(TMPDIR, "ctm6");
	tmp7 = tempnam(TMPDIR, "ctm7");
	if (!(tmp2 && tmp3 && tmp4 && tmp5 && tmp6 && tmp7)
		|| creat(tmp2,(mode_t)0666) < 0)
		error('e', "%scc: cannot create temporaries: %s\n", prefix, tmp2);
}

static int
getXpass(s, opt)
	char	*s, *opt;
{
	int d;

	switch (*s) {
	case '0':
		return(Xc0);
	case '2':
		return(Xc2);
	case 'b':
		return(Xbb);
	case 'p':
		return(Xcp);
	case 'a':
		return(Xas);
	case 'l':
		return(Xld);
	default:
		if ( (d = Welse(*s)) == -1 ) {
			error('w', "Unrecognized pass name: '%s%c\n'", opt, *s);
			return(-1);
		} else
			return(d);
	}
}

#ifdef PERF
pexit()
{
	error('e', "Too many files for performance stats\n");
	dexit();
}
#endif

#ifdef PERF
pwrap()
{
	int	i;

	if ((perfile = fopen("cc.perf.info", "r")) == NULL)
		dexit();
	fclose(perfile);
	if ((perfile = fopen("cc.perf.info", "a")) == NULL)
		dexit();
	for (i = ii-1; i > 0; i--) {
		stats[i].perform.proc_user_time -= stats[i-1].perform.proc_user_time;
		stats[i].perform.proc_system_time -= stats[i-1].perform.proc_system_time;
		stats[i].perform.child_user_time -= stats[i-1].perform.child_user_time;
		stats[i].perform.child_system_time -= stats[i-1].perform.child_system_time;
	}
	for (i = 0; i < ii; i++)
		(void) fprintf(perfile, "%s\t%07ld\t%07ld\t%07ld\t%07ld\t%07ld\n",
			stats[i].module,stats[i].ttim,stats[i].perform);
	fclose(perfile);
}
#endif


/* function to handle -YL and -YU substitutions in LIBPATH */

static char *
compat_YL_YU(index)
int index;
{
	/* user supplied -YL,libdir  and this is the pathname that corresponds 
		for compatibility to -YL (as defined in paths.h) */
	if(libdir && index == YLDIR)
		return(libdir);

	/* user supplied -YU,llibdir  and this is the pathname that corresponds 
		for compatibility to -YU (as defined in paths.h) */
	if(llibdir && index == YUDIR)
		return(llibdir);

	return(NULL);
}

static void
process_lib_path(pathlib,npathlib)
char * pathlib;
char * npathlib;
{
	int i;
	char * cp;
	char * cp2;


	for(i=1;; i++) {
		cp = strpbrk(pathlib,":");
		if(cp == NULL) {
			cp2 = compat_YL_YU(i);
			if(cp2 == NULL) {
				(void) strcpy(npathlib,pathlib);
			}
			else {
				(void) strcpy(npathlib,cp2);
			}
			return;
		}

		if(*cp == ':') {
			cp2 = compat_YL_YU(i);
			if(cp2 == NULL) {
				(void) strncpy(npathlib,pathlib,cp - pathlib +1);
				npathlib = npathlib + (cp - pathlib + 1);
			}
			else {
				(void) strcpy(npathlib,cp2);
				npathlib += strlen(cp2);
				*npathlib = ':';
				npathlib++;
			}
			pathlib = cp + 1;
			continue;
		}
		
	}
}

/*
 * Print out detail Usage messages.
 * This routine calls machine dependent option_mach()
 * because some options are machine dependent. Those
 * options should be handled by machine dependent routine.
 */
static void
option_indep()
{
	OPTPTR("\t[-A name[(tokens)]]: associates name as a predicate with the\n");
	OPTPTR("\t\tspecified tokens as if by a #assert preprocessing directive.\n");
	OPTPTR("\t[-B c]: c can be either dynamic or static.\n");
	OPTPTR("\t[-C]: cause the preprocessing phase to pass all comments.\n");
	OPTPTR("\t[-c]: suppress the link editing phase of the compilation and\n");
	OPTPTR("\t\tdo not remove any produced object files.\n");
	OPTPTR("\t[-D name[=tokens]]: associates name with the specified tokens\n");
	OPTPTR("\t\tas if by a #define preprocessing directive.\n");
	OPTPTR("\t[-d c]: c can be either y or n. -dy specifies dynamic linking\n");
	OPTPTR("\t\tin the link editor. -dn specifies static linking.\n");
	OPTPTR("\t[-E]: only preprocess the named C files and send the results\n");
	OPTPTR("\t\tto the standard output.\n");
	OPTPTR("\t[-e optarg]: pass to link editor.\n");
	OPTPTR("\t[-f]: this option is obsolete and will be ignored.\n");
	OPTPTR("\t[-G]: directs the link editor to produce shared object.\n");
	OPTPTR("\t[-g]: cause the compiler to produce additional information\n");
	OPTPTR("\t\tneeded for the use of sdb.\n");
	OPTPTR("\t[-H]: print the path name of each file included during the\n");
	OPTPTR("\t\tcurrent compilation on the standard error output.\n");
	OPTPTR("\t[-h optarg]: pass to like editor.\n");
	OPTPTR("\t[-I dir]: alter the search for included files whose names don't\n");
	OPTPTR("\t\tbegin with / to look in dir prior to the usual directories.\n");
	OPTPTR("\t[-K [mode,goal,PIC,minabi]]: -K mode will compile the named C\n");
	OPTPTR("\t\tfiles with the indictaed floating-point mode. -K goal pass\n");
	OPTPTR("\t\tnecessary information to optimizor. -K PIC causes position-\n");
	OPTPTR("\t\tindependent code (PIC) to be generated. -K minabi directs\n");
	OPTPTR("\t\tthe compilation system to use the C library which minimize\n");
	OPTPTR("\t\tdynamic linking.\n");
	OPTPTR("\t[-L dir]: add dir to the list of directories searched for\n");
	OPTPTR("\t\tlibrary by link editor.\n");
	OPTPTR("\t[-l name]: search the library libname.a or libname.so.\n");
	OPTPTR("\t[-O]: arrange for compilation phase optimization.\n");
	OPTPTR("\t[-o pathname]: produce an output object file pathname, instead\n");
	OPTPTR("\t\tof the default a.out.\n");
	OPTPTR("\t[-P]: only preprocess the named C files and leave the result\n");
	OPTPTR("\t\tin corresponding files suffixed .i.\n");
	OPTPTR("\t[-p]: arrange for the compiler to produce code that counts\n");
	OPTPTR("\t\tthe number of times each routine is called.\n");
	OPTPTR("\t[-Q c]: c can be either y or n. -Qy indicates identification\n");
	OPTPTR("\t\tinformation about each invoked compilation tool will be\n");
	OPTPTR("\t\tadded to the output files. -Qn suppress the information.\n");
	OPTPTR("\t[-q c]: c can be either l or p. -ql arranges for the production\n");
	OPTPTR("\t\tof code that counts the number of times each source line\n");
	OPTPTR("\t\tis executed. -qp is synonym for -p.\n");
	OPTPTR("\t[-S]: compile but do not assemble or link edit the named C files.\n");
	OPTPTR("\t[-U name]: causes any definition of name to be forgotten.\n");
	OPTPTR("\t[-u optarg]: pass to link editor.\n");
	OPTPTR("\t[-V]: cause each invoked tool to print its version information\n");
	OPTPTR("\t\ton the standard error output.\n");
	OPTPTR("\t[-v]: cause the compiler to perform more and stricter semantic\n");
	OPTPTR("\t\tcheck, and to enable lint-like checks on the named C files.\n");
	OPTPTR("\t[-W tool,arg1[,arg2 ...]]: hand off the arguments \"arg(x)\"\n");
	OPTPTR("\t\teach as a separate argument to tool.\n");
	OPTPTR("\t[-X c]: specify degree of conformance to the ANSI C standard.\n");
	OPTPTR("\t[-Y item,dir]: specify a new directory dir for item.\n");
	OPTPTR("\t[-z optarg]: pass to link editor.\n");
	OPTPTR("\t[-#]: turn on debug information.\n");
	OPTPTR("\t[-?]: display cc options usage.\n");

	/* append some machine dependent options messages */
	option_mach();

	return;
}
