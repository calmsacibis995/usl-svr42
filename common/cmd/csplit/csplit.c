/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)csplit:csplit.c	1.13.1.5"
#ident	"$Header: csplit.c 1.1 91/02/28 $"
/*
*	csplit - Context or line file splitter
*	Compile: cc -O -s -o csplit csplit.c
*/

#include <stdio.h>
#include <errno.h>
#include <regexpr.h>
#include <signal.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>


#define LAST	  0L
#define ERR	 -1
#define FALSE	  0
#define TRUE	  1
#define EXPMODE	  2
#define LINMODE	  3
#define	LINSIZ	  256
#define MAXFLS	  99

	/* Globals */

char linbuf[LINSIZ];		/* Input line buffer */
char *expbuf;
char tmpbuf[BUFSIZ];		/* Temporary buffer for stdin */
char *file = NULL;		/* File name buffer */
char *targ;			/* Arg ptr for error messages */
char *sptr;
FILE *infile, *outfile;		/* I/O file streams */
int silent, keep, create;	/* Flags: -s(ilent), -k(eep), (create) */
int errflg;
extern int optind;
extern char *optarg;
long offset;			/* Regular expression offset value */
long curline;			/* Current line in input file */

/*
*	These defines are needed for regexp handling (see regexp(7))
*/
#define PERROR(x)	fatal(MM_ERROR,":5:%s: Illegal Regular Expression\n",targ,NULL);

char *getline();

static	const	char	outofrangee[] = ":6:%s - Out of range\n";


main(argc,argv)
int argc;
char **argv;
{
	void sig();
	long findline();
	FILE *getfile();
	int fopt = 0; 		/* f option flag */
	int ch, mode;
	char *tail;		/* pointer to basename of file prefix */
	char *head;		/* dirname of file prefix */
	long name_max;  	/* maximum file name length */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:csplit");
	while((ch=getopt(argc,argv,"skf:")) != EOF) {
		switch(ch) {
			case 'f':
				/* allocate space for the file name plus the 
				null character.  Make sure prefix is not bigger 
				than file system will allow.                 */

				file = (char *)malloc(strlen(optarg)+3);
				strcpy(file,optarg);
				head = dirname(optarg);
				tail = basename(file);
				if ((name_max = pathconf(head,_PC_NAME_MAX)) == -1)
				     fatal(MM_NOGET, "%s: %s\n", head,strerror(errno));
				else{
					if (((long)strlen(tail)) > (name_max-2))
	    					fatal(MM_ERROR,":7:Prefix %s too long\n",tail);
				}	   
				fopt++;
				break;
			case 's':
				silent++;
				break;
			case 'k':
				keep++;
				break;
			case '?':
				errflg++;
		}
	}


	/* if -f option not used, set file to "xx" and allocate
	   enough space for the suffix and null char.          */
	if (!fopt) {
		file = (char *)malloc(5);
		strcpy(file,"xx");
	}

	argv = &argv[optind];
	argc -= optind;
	if(argc <= 1 || errflg) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
		fatal(MM_ACTION,
			":8:Usage: csplit [-s] [-k] [-f prefix] file args ...\n");
	}
	if(strcmp(*argv, "-") == 0) {
		infile = tmpfile();

		while(fread(tmpbuf, 1, BUFSIZ, stdin) != 0) {
			if(fwrite(tmpbuf, 1, BUFSIZ, infile) == 0)
				if(errno == ENOSPC) {
					pfmt(stderr, MM_ERROR,
						":9:No space left on device\n");
					exit(1);
				}else{
					pfmt(stderr, MM_ERROR,
						":10:Cannot write temporary file: %s\n",
						strerror (errno));
					exit(1);
				}

	/*	clear the buffer to get correct size when writing buffer	*/

			memset(tmpbuf,'\0',sizeof(tmpbuf));
		}
		rewind(infile);
	}
	else if((infile = fopen(*argv,"r")) == NULL)
		fatal(MM_ERROR,":3:Cannot open %s: %s\n",
			*argv, strerror (errno));
	++argv;
	curline = 1L;
	signal(SIGINT,sig);

	/*
	*	The following for loop handles the different argument types.
	*	A switch is performed on the first character of the argument
	*	and each case calls the appropriate argument handling routine.
	*/

	for(; *argv; ++argv) {
		targ = *argv;
		switch(**argv) {
		case '/':
			mode = EXPMODE;
			create = TRUE;
			re_arg(*argv);
			break;
		case '%':
			mode = EXPMODE;
			create = FALSE;
			re_arg(*argv);
			break;
		case '{':
			num_arg(*argv,mode);
			mode = FALSE;
			break;
		default:
			mode = LINMODE;
			create = TRUE;
			line_arg(*argv);
			break;
		}
	}
	create = TRUE;
	to_line(LAST);

	exit (0);
}

/*
*	Atol takes an ascii argument (str) and converts it to a long (plc)
*	It returns ERR if an illegal character.  The reason that atol
*	does not return an answer (long) is that any value for the long
*	is legal, and this version of atol detects error strings.
*/

atol(str,plc)
register char *str;
long *plc;
{
	register int f;
	*plc = 0;
	f = 0;
	for(;;str++) {
		switch(*str) {
		case ' ':
		case '\t':
			continue;
		case '-':
			f++;
		case '+':
			str++;
		}
		break;
	}
	for(; *str != NULL; str++)
		if(*str >= '0' && *str <= '9')
			*plc = *plc * 10 + *str - '0';
		else
			return(ERR);
	if(f)
		*plc = -(*plc);
	return(TRUE);	/* not error */
}

/*
*	Closefile prints the byte count of the file created, (via fseek
*	and ftell), if the create flag is on and the silent flag is not on.
*	If the create flag is on closefile then closes the file (fclose).
*/

closefile()
{
	long ftell();

	if(!silent && create) {
		fseek(outfile,0L,2);
		fprintf(stdout,"%ld\n",ftell(outfile));
	}
	if(create)
		fclose(outfile);
}

/*
*	Fatal handles error messages and cleanup.
*	Because "arg" can be the global file, and the cleanup processing
*	uses the global file, the error message is printed first.  If the
*	"keep" flag is not set, fatal unlinks all created files.  If the
*	"keep" flag is set, fatal closes the current file (if there is one).
*	Fatal exits with a value of 1.
*/

fatal(howbad, string,arg1, arg2)
int	howbad;
char *string, *arg1, *arg2;
{
	register char *fls;
	register int num;

	pfmt(stderr, howbad, string, arg1, arg2);
	if(!keep) {
		if(outfile) {
			fclose(outfile);
			for(fls=file; *fls != NULL; fls++)
			fls -= 2;
			for(num=atoi(fls); num >= 0; num--) {
				sprintf(fls,"%.02d",num);
				unlink(file);
			}
		}
	} else
		if(outfile)
			closefile();
	exit(1);
}

/*
*	Findline returns the line number referenced by the current argument.
*	Its arguments are a pointer to the compiled regular expression (expr),
*	and an offset (oset).  The variable lncnt is used to count the number
*	of lines searched.  First the current stream location is saved via
*	ftell(), and getline is called so that R.E. searching starts at the
*	line after the previously referenced line.  The while loop checks
*	that there are more lines (error if none), bumps the line count, and
*	checks for the R.E. on each line.  If the R.E. matches on one of the
*	lines the old stream location is restored, and the line number
*	referenced by the R.E. and the offset is returned.
*/

long findline(expr,oset)
register char *expr;
long oset;
{
	static int benhere;
	long lncnt = 0, saveloc, ftell();

	saveloc = ftell(infile);
	if(curline != 1L || benhere)		/* If first line, first time, */
		getline(FALSE);			/* then don't skip */
	else
		lncnt--;
	benhere = 1;
	while(getline(FALSE) != NULL) {
		lncnt++;
		if((sptr=strrchr(linbuf,'\n')) != NULL)
			*sptr = '\0';
		if(step(linbuf,expr)) {
			fseek(infile,saveloc,0);
			return(curline+lncnt+oset);
		}
	}
	fseek(infile,saveloc,0);
	return(curline+lncnt+oset+2);
}

/*
*	Flush uses fputs to put lines on the output file stream (outfile)
*	Since fputs does its own buffering, flush doesn't need to.
*	Flush does nothing if the create flag is not set.
*/

flush()
{
	if(create)
		fputs(linbuf,outfile);
}

/*
*	Getfile does nothing if the create flag is not set.  If the
*	create flag is set, getfile positions the file pointer (fptr) at
*	the end of the file name prefix on the first call (fptr=0).
*	Next the file counter (ctr) is tested for MAXFLS, fatal if too
*	many file creations are attempted.  Then the file counter is
*	stored in the file name and incremented.  If the subsequent
*	fopen fails, the file name is copied to tfile for the error
*	message, the previous file name is restored for cleanup, and
*	fatal is called.  If the fopen succecedes, the stream (opfil)
*	is returned.
*/

FILE *getfile()
{
	static char *fptr;
	static int ctr;
	FILE *opfil;
	char *tfile;

	if(create) {
		if(fptr == 0)
			for(fptr = file; *fptr != NULL; fptr++);
		if(ctr > MAXFLS)
			fatal(MM_ERROR,":11:100 file limit reached at arg %s\n",targ);
		sprintf(fptr,"%.02d",ctr++);
		if((opfil = fopen(file,"w")) == NULL) {
			if ((tfile = strdup(file)) == NULL)
				pfmt(stderr,MM_NOGET,"%s: %s\n",file,strerror(errno));
			sprintf(fptr,"%.02d",(ctr-2));
			fatal(MM_ERROR,":12:Cannot create %s: %s\n",tfile, strerror (errno));
		}
		return(opfil);
	}
	return(NULL);
}

/*
*	Getline gets a line via fgets from the input stream "infile".
*	The line is put into linbuf and may not be larger than LINSIZ.
*	If getline is called with a non-zero value, the current line
*	is bumped, otherwise it is not (for R.E. searching).
*/

char *getline(bumpcur)
int bumpcur;
{
	char *ret;
	if(bumpcur)
		curline++;
	ret=fgets(linbuf,LINSIZ,infile);
	return(ret);
}

/*
*	Line_arg handles line number arguments.
*	line_arg takes as its argument a pointer to a character string
*	(assumed to be a line number).  If that character string can be
*	converted to a number (long), to_line is called with that number,
*	otherwise error.
*/

line_arg(line)
char *line;
{
	long to;

	if(atol(line,&to) == ERR)
		fatal(MM_ERROR,":13:%s: Bad line number\n",line);
	to_line(to);
}

/*
*	Num_arg handles repeat arguments.
*	Num_arg copies the numeric argument to "rep" (error if number is
*	larger than 11 characters or } is left off).  Num_arg then converts
*	the number and checks for validity.  Next num_arg checks the mode
*	of the previous argument, and applys the argument the correct number
*	of times. If the mode is not set properly its an error.
*/

num_arg(arg,md)
register char *arg;
int md;
{
	long repeat, toline;
	char rep[12];
	register char *ptr;

	ptr = rep;
	for(++arg; *arg != '}'; arg++) {
		if(ptr == &rep[11])
			fatal(MM_ERROR,":14:%s: Repeat count too large\n",targ);
		if(*arg == NULL)
			fatal(MM_ERROR,":15:%s: Missing '}'\n",targ);
		*ptr++ = *arg;
	}
	*ptr = NULL;
	if((atol(rep,&repeat) == ERR) || repeat < 0L)
		fatal(MM_ERROR,":16:Illegal repeat count: %s\n",targ);
	if(md == LINMODE) {
		toline = offset = curline;
		for(;repeat > 0L; repeat--) {
			toline += offset;
			to_line(toline);
		}
	} else	if(md == EXPMODE)
			for(;repeat > 0L; repeat--)
				to_line(findline(expbuf,offset));
		else
			fatal(MM_ERROR,":17:No operation for %s\n",targ);
}

/*
*	Re_arg handles regular expression arguments.
*	Re_arg takes a csplit regular expression argument.  It checks for
*	delimiter balance, computes any offset, and compiles the regular
*	expression.  Findline is called with the compiled expression and
*	offset, and returns the corresponding line number, which is used
*	as input to the to_line function.
*/

re_arg(string)
char *string;
{
	register char *ptr;
	register char ch;

	ch = *string;
	ptr = string;
	while(*(++ptr) != ch) {
		if(*ptr == '\\')
			++ptr;
		if(*ptr == NULL)
			fatal(MM_ERROR,":18:%s: Missing delimiter\n",targ);
	}

	/*	The line below was added because compile no longer supports	*
	 *	the fourth argument being passed.   The fourth argument used	*
	 *	to be '/' or '%'.						*/

	*ptr = NULL;
	if(atol(++ptr,&offset) == ERR)
		fatal(MM_ERROR,":19:%s: Illegal offset\n",string);

	/*	The line below was added because INIT which did this for us*
	 *	was removed from compile in regexp.h			   */

	string++;
	expbuf = compile(string, (char *)0, (char *)0);
	if (regerrno)
		PERROR(regerrno);
	to_line(findline(expbuf,offset));
}

/*
*	Sig handles breaks.  When a break occurs the signal is reset,
*	and fatal is called to clean up and print the argument which
*	was being processed at the time the interrupt occured.
*/

void
sig(s)
int	s;
{
	signal(SIGINT,sig);
	fatal(MM_ERROR,":20:Interrupt - Program aborted at arg '%s'\n",targ);
}

/*
*	To_line creates split files.
*	To_line gets as its argument the line which the current argument
*	referenced.  To_line calls getfile for a new output stream, which
*	does nothing if create is False.  If to_line's argument is not LAST
*	it checks that the current line is not greater than its argument.
*	While the current line is less than the desired line to_line gets
*	lines and flushes (error if EOF is reached).
*	If to_line's argument is LAST, it checks for more lines, and gets
*	and flushes lines till the end of file.
*	Finally, to_line calls closefile to close the output stream.
*/

to_line(ln)
long ln;
{
	outfile = getfile();
	if(ln != LAST) {
		if(curline > ln)
			fatal(MM_ERROR,outofrangee,targ);
		while(curline < ln) {
			if(getline(TRUE) == NULL)
				fatal(MM_ERROR,outofrangee,targ);
			flush();
		}
	} else		/* last file */
		if(getline(TRUE) != NULL) {
			flush();
			while(TRUE) {
				if(getline(TRUE) == NULL)
					break;
				flush();
			}
		} else
			fatal(MM_ERROR,outofrangee,targ);
	closefile();
}
