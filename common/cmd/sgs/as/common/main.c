/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/main.c	1.17"
/*
* common/main.c - common assembler main and misc functions
*/
#ifdef __STDC__
#   include <stdlib.h>
#   include <stdarg.h>
#   include <unistd.h>
#   include <sys/wait.h>
#else
#   include <varargs.h>
    char *malloc(), *realloc();
    void exit();
    int getopt(), unlink(), access(), wait();
#   define remove(n) unlink(n)
#   define F_OK	0
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include "common/as.h"
#include "common/dirs.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/gram.h"
#include "common/main.h"
#include "common/objf.h"
#include "common/sect.h"
#include "common/syms.h"
#include <sgs.h>
#include <paths.h>

#ifdef DEBUG
   int dbflgs['z' - 'a' + 1 + 'Z' - 'A' + 1];	/* debugging levels */
#endif

Ulong curlineno;	/* current line number of current input file */
Ushort curfileno;	/* current input file number */

Ulong flags;		/* global assembler flags */

	/*
	* Once infiles[] has been allocated, curname is always
	* the same as infiles[curfileno].  curfileno==0 (and
	* thus infiles[0]) is reserved for the pseudo-file
	* <startup> which is used as the context before any
	* input is read.
	*/
static const char *curname;	/* current input file name */
static const char **infiles;	/* array of input file names */

static const char *progname;	/* program invocation name */

static int eo_input;		/* set after all input read */
static int nerrs;		/* number of user errors */

static const char sgsname[] = SGSNAME;	/* these from <inc/$(MACH)/sgs.h> */
static const char sgu_pkg[] = SGU_PKG;
static const char sgu_rel[] = SGU_REL;
static const char outname[] = A_OUT;	/* these from <inc/$(MACH)/paths.h> */
static const char m4_path[] = M4;
static const char cm4defs[] = CM4DEFS;

static char *
#ifdef __STDC__
mkoutname(const char *in)	/* 'basename $in .s'.o, if possible */
#else
mkoutname(in)char *in;
#endif
{
	size_t len = strlen(in);
	char *res;

	if (len <= 2 || in[len - 2] != '.' || in[len - 1] != 's')
		return 0;
	if ((res = strrchr(in, '/')) != 0)
	{
		len -= res - in + 1;
		in = res + 1;
	}
	res = (char *)savestr((const Uchar *)in, len);
	res[len - 1] = 'o';
	return res;
}

static FILE *
#ifdef __STDC__
m4(const char *path, const char *defs, int n, char **list) /* run m4 */
#else
m4(path, defs, n, list)char *path, *defs; int n; char **list;
#endif
{
	char **args, **p;
	int pid;
	int fds[2];
	FILE *fp;

	if (pipe(fds) != 0)
	{
		error("cannot open pipe from m4");
		fatal((char *)0);
	}
	pid = getpid();
	switch (fork())
	{
	case -1:
		error("cannot fork for m4");
		fatal((char *)0);
		/*NOTREACHED*/
	case 0:
		/*
		* Redirect stdout for m4 to the pipe.
		*/
		(void)close(1);
		if (fcntl(fds[1], F_DUPFD, 1) != 1)
		{
			error("cannot redirect standard output for m4");
			(void)kill(pid, SIGTERM);
			fatal((char *)0);
		}
		/*
		* Set up the argument list.  After the last option for m4
		* and before the first file name argument, add the special
		* m4 macro file as the first to process, if it exists.
		* Change path and defs from optional directory names into
		* pathnames for the command and macro file, respectively.
		*/
		args = (char **)alloc((void *)0, (2 + n + 1) * sizeof(char *));
		if (path == 0)
			args[0] = (char *)m4_path;
		else
		{
			args[0] = (char *)alloc((void *)0, strlen(path) + 4);
			(void)sprintf(args[0], "%s/m4", path);
		}
		path = args[0];
		if (defs == 0)
			defs = cm4defs;
		else
		{
			args[1] = (char *)alloc((void *)0, strlen(defs) + 9);
			(void)sprintf(args[1], "%s/cm4defs", defs);
			defs = args[1];
		}
		p = &args[1];
		if (access(defs, F_OK) == 0)	/* macro file exists */
		{
			if (n != 0)	/* copy up to first filename */
			{
				while (list[0][0] == '-' && list[0][1] != '\0')
				{
					*p++ = *list++;
					if (--n == 0 || list[-1][1] == '-')
						break;
				}
			}
			*p++ = (char *)defs;
		}
		while (--n >= 0)	/* copy (rest) of arguments */
			*p++ = *list++;
		*p = 0;
		(void)execv(path, args);
		error("cannot exec %s", path);
		(void)kill(pid, SIGTERM);
		fatal((char *)0);
	}
	/*
	* Create a stream for reading from the pipe.
	*/
	if ((fp = fdopen(fds[0], "r")) == 0)
	{
		error("cannot create stream for m4 output");
		fatal((char *)0);
	}
	(void)close(fds[1]);	/* leave only one writer for pipe */
	return fp;
}

int
#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)int argc; char **argv;
#endif
{
	static const char MSGsyn[] = "cannot recover from syntax error";
	extern int optind;
	extern char *optarg;
	const char *outfile = 0;	/* output file name */
	int preprocess = 0;
	char *m4path = 0;		/* -Ym,<dir> generated */
	char *m4defs = 0;		/* -Yd,<dir> generated */
	int ident_requested = 0;
	int version_requested = 0;
	FILE *fp;
	char *p;
	int i;

	progname = argv[0];
	curname = "<startup>";
	while ((i = getopt(argc, argv, impdep_optstr)) != EOF)
	{
		switch (i)
		{
		case 'Q':
			if (optarg[0] == 'y')
				ident_requested = 1;
			else if (optarg[0] == 'n')
				ident_requested = 0;
			else
				goto usage;
			break;
		case 'V':
			(void)fprintf(stderr, "%s%s: %s %s\n",
				sgsname, impdep_cmdname, sgu_pkg, sgu_rel);
			version_requested = 1;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'd':
#ifdef DEBUG
			/*
			* See common/as.h for the key to common
			* debugging letters.
			*/
			while ((i = *optarg) != '\0')
			{
				if (i >= 'a' && i <= 'z')	/* common */
					dbflgs[i - 'a']++;
				else if (i >= 'A' && i <= 'Z')	/* impdep */
					dbflgs[i - 'A' + 26]++;
				else
				{
					warn("debug character ignored: %s",
						prtstr((Uchar *)optarg,
						(size_t)1));
				}
				optarg++;
			}
#else
			warn("no debugging available");
#endif
			break;
		case 'm':
			preprocess = 1;
			break;
		case 'Y':
			p = optarg;
			if ((optarg = strchr(p, ',')) == 0)
			{
				(void)fprintf(stderr,
					"%s:-Y argument missing comma\n",
					progname);
				goto usage;
			}
			if (p == optarg)
			{
				(void)fprintf(stderr,
					"%s:-Y argument missing key(s)\n",
					progname);
				goto usage;
			}
			for (optarg++; *p != ','; p++)
			{
				if (*p == 'm')
					m4path = optarg;
				else if (*p == 'd')
					m4defs = optarg;
				else
				{
					(void)fprintf(stderr,
						"%s:invalid -Y key: %s\n",
						progname, prtstr((Uchar *)p,
						(size_t)1));
					goto usage;
				}
			}
			break;
		case '?':
		usage:;
			(void)fprintf(stderr, impdep_usage, progname);
			return 2;
		default:
			impdep_option(i);
			break;
		}
	}
	/*
	* Process files.  If there are none, either issue
	* a usage message or if -V specified, quietly quit.
	*/
	if ((i = optind) >= argc)	/* no file name arguments */
	{
		if (version_requested)
			return 0;
		goto usage;
	}
	/*
	* Set up the file name list for diagnostics.  If -m,
	* the list is only "<startup>" and "<m4output>".
	*/
	if (preprocess || argc - i == 2)
	{
		static const char *list[2];

		infiles = list;
	}
	else
	{
		infiles = (const char **)alloc((void *)0,
			sizeof(const char *) * (argc - i + 1));
	}
	infiles[0] = curname;
	/*
	* Initialize everything else.
	*/
	initeval();
	initsyms();	/* must be after initeval() */
	initsect();
	initdirs();	/* must be after initsect() */
	initchtab();
	impdep_init();	/* intentionally last */
	/*
	* Parse each input file separately, collecting the
	* results as if they were one long input file.  If
	* -m, then hand the filenames to m4 and read its
	* output.  The name of the output file is determined
	* by the first filename that ends in ".s", if any.
	*/
	if (preprocess)
	{
		int status;

		fp = m4(m4path, m4defs, argc - i, &argv[i]);
		while (outfile == 0)
		{
			outfile = mkoutname(argv[i]);
			if (++i >= argc)
				break;
		}
		infiles[++curfileno] = curname = "<m4output>";
		curlineno = 0;
		setinput(fp);
		if (yyparse())
			error(MSGsyn);
		(void)fclose(fp);
		wait(&status);
		if (status != 0)
		{
			error("assembly inhibited");
			fatal((char *)0);
		}
	}
	else
	{
		do
		{
			infiles[++curfileno] = curname = argv[i];
			curlineno = 0;
			if (curname[0] == '-' && curname[1] == '\0')
				fp = stdin;
			else if ((fp = fopen(curname, "r")) == 0)
			{
				error("cannot open input file");
				continue;
			}
			else if (outfile == 0)		/* X.s -> X.o */
				outfile = mkoutname(curname);
			setinput(fp);
			if (yyparse())
				error(MSGsyn);
			if (fp != stdin)
				(void)fclose(fp);
		} while (++i < argc);
	}
	/*
	* All input processed.  Append to .comment section if -Qy
	* specified.  Make a first pass through all the sections and
	* the symbol table.  If more errors are detected, give up.
	*/
	eo_input = 1;
	if (ident_requested)	/* generate own .ident string */
	{
		static const Uchar comment[] = ".comment";
		size_t identlen = strlen(sgsname)
			+ strlen(impdep_cmdname) + 2
			+ strlen(sgu_rel) + 1;
		Uchar *ident = (Uchar *)alloc((void *)0, identlen);

		(void)sprintf((char *)ident, "%s%s: %s",
			sgsname, impdep_cmdname, sgu_rel);
		sectstr(lookup(comment, sizeof(comment) - 1)->sym_sect,
			strexpr(ident, identlen));
	}
	walksyms();
	if (nerrs > 0)
		return 2;
	walksect();
	if (nerrs > 0)
		return 2;
	/*
	* Generate output.  First fill in the symbol table,
	* then generate the section data.  From openobjf()
	* on, also remove the output file for error exits.
	*/
	if (outfile == 0)
		outfile = outname;
	if (openobjf(outfile) == 0)
		return 2;	/* don't remove outfile, if any */
	if (nerrs > 0)
		goto err;
	gensyms();
	if (nerrs > 0)
		goto err;
	gensect();
	if (nerrs > 0)
		goto err;
	closeobjf();
	if (nerrs > 0)
		goto err;
	reevaluate();
	if (nerrs > 0)
		goto err;
	return 0;
err:;
	(void)remove(outfile);
	return 2;
}

#ifdef __STDC__
void *
alloc(void *ptr, size_t sz)	/* realloc() front end */
{
	if ((ptr = realloc(ptr, sz)) == 0)
	{
		if (sz == 0) return alloc(ptr, 1);
		error("cannot allocate %lu bytes", (Ulong)sz);
		fatal((char *)0);
	}
#ifdef DEBUG
	if (DEBUG('a') > 1)
		(void)fprintf(stderr, "allocated %lu bytes\n", (Ulong)sz);
#endif
	return ptr;
}
#else
char *
alloc(ptr, sz)		/* combination of malloc() and realloc() */
	char *ptr;
	size_t sz;
{
	if (ptr == 0)
		ptr = malloc(sz);
	else
		ptr = realloc(ptr, sz);
	if (ptr == 0)
	{
		if (sz == 0) return alloc(ptr, 1);
		error("cannot allocate %lu bytes", (Ulong)sz);
		fatal((char *)0);
	}
#ifdef DEBUG
	if (DEBUG('a') > 1)
		(void)fprintf(stderr, "allocated %lu bytes\n", (Ulong)sz);
#endif
	return ptr;
}
#endif

char *
#ifdef __STDC__
prtstr(const Uchar *s, size_t n) /* return printable version of string */
#else
prtstr(s, n)Uchar *s; size_t n;
#endif
{
	static char *res;
	static size_t len;
	register char *r;

	/*
	* Each call to prtstr() invalidates the previous call's return.
	* Thus the caller must be completely finished with each return
	* from prtstr() before the next call can occur.
	*/
	if (len < 4 * n + 1)
		res = alloc((void *)res, len = 4 * n + 1);
	for (r = res; n != 0; n--)
	{
		if (isprint(*s) && *s != '\\')
		{
			*r++ = *s++;
			continue;
		}
		*r++ = '\\';	/* start escape sequence */
		switch (*s++)	/* rest of escape sequence */
		{
		case '\\':
			*r++ = '\\';
			continue;
		case '\n':
			*r++ = 'n';
			continue;
		case '\t':
			*r++ = 't';
			continue;
		default:
			r += sprintf(r, "%3.3o", s[-1]);
			continue;
		}
	}
	*r = '\0';
	return res;
}

static void
#ifdef __STDC__
errstr(void)	/* end diagnostic line, add errno string if appropriate */
#else
errstr()
#endif
{
	const char *str;

#if FORMAT == ELF
	if ((str = elf_errmsg(0)) != 0)
	{
		(void)fprintf(stderr, ": %s", str);
		(void)elf_errno();	/* clear elf internal error */
	}
#endif
	if (errno == 0)
		(void)putc('\n', stderr);
	else
	{
#ifdef __STDC__
		str = strerror(errno);	/* assume possible NULL return */
#else
		extern char *sys_errlist[];
		extern int sys_nerr;

		if (errno > 0 && errno < sys_nerr)
			str = sys_errlist[errno];
		else
			str = 0;
#endif
		if (str != 0)
			(void)fprintf(stderr, ": %s\n", str);
		else
			(void)fprintf(stderr, " <errno=%d>\n", errno);
		errno = 0;
	}
}

static void	/* print location, label, and variable part of err/warn */
#ifdef __STDC__
where(const char *lab, Ulong fno, Ulong lno, const char *fmt, va_list ap)
#else
where(lab, fno, lno, fmt, ap)char *lab, *fmt; Ulong fno, lno; va_list ap;
#endif
{
	if (fno > curfileno)
		fatal("where():invalid file number: %lu", fno);
	if (eo_input)
	{
		if (fno == curfileno && lno == curlineno)
			(void)fprintf(stderr, "(EOF):%s", lab);
		else
		{
			(void)fprintf(stderr, "%s:%lu:%s",
				infiles[fno], lno, lab);
		}
	}
	else if (fno != curfileno)
	{
		(void)fprintf(stderr, "%s:%lu:(at %s:%lu):%s",
			curname, curlineno, infiles[fno], lno, lab);
	}
	else if (lno != curlineno)
	{
		(void)fprintf(stderr, "%s:%lu:(at line %lu):%s",
			curname, curlineno, lno, lab);
	}
	else
		(void)fprintf(stderr, "%s:%lu:%s", curname, lno, lab);
	if (fmt != 0)	/* special case due to fatal() */
		(void)vfprintf(stderr, fmt, ap);
}

void
#ifdef __STDC__
fatal(const char *fmt, ...)	/* print internal error message and exit */
#else
fatal(va_alist)va_dcl
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
	where(fmt != 0 ? (const char *)"internal error:"
		: (const char *)"cannot recover from previous error",
		(Ulong)curfileno, curlineno, fmt, ap);
	errstr();
	va_end(ap);
	exit(2);
}

static void
#ifdef __STDC__
do_error(Ulong fno, Ulong lno, const char *fmt, va_list ap)
#else
do_error(fno, lno, fmt, ap)Ulong fno, lno; char *fmt; va_list ap;
#endif
{
	where("", fno, lno, fmt, ap);
	errstr();
	if (++nerrs > 20)
	{
		(void)fputs("\t...too many errors\n", stderr);
		exit(2);
	}
}

void
#ifdef __STDC__
error(const char *fmt, ...)	/* print user error message */
#else
error(va_alist)va_dcl
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
	do_error((Ulong)curfileno, curlineno, fmt, ap);
	va_end(ap);
}

void
#ifdef __STDC__
backerror(Ulong fno, Ulong lno, const char *fmt, ...) /* back ref'd error */
#else
backerror(va_alist)va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;
	Ulong fno, lno;

	va_start(ap);
	fno = va_arg(ap, Ulong);
	lno = va_arg(ap, Ulong);
	fmt = va_arg(ap, char *);
#endif
	do_error(fno, lno, fmt, ap);
	va_end(ap);
}

void
#ifdef __STDC__
exprerror(const Expr *ep, const char *fmt, ...)	/* error w/expr's location */
#else
exprerror(va_alist)va_dcl
#endif
{
	Ulong fno, lno;
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	Expr *ep;
	char *fmt;

	va_start(ap);
	ep = va_arg(ap, Expr *);
	fmt = va_arg(ap, char *);
#endif
	exprfrom(&fno, &lno, ep);
	do_error(fno, lno, fmt, ap);
	va_end(ap);
}

static void
#ifdef __STDC__
do_warn(Ulong fno, Ulong lno, const char *fmt, va_list ap)
#else
do_warn(fno, lno, fmt, ap)Ulong fno, lno; char *fmt; va_list ap;
#endif
{
	where("warning:", fno, lno, fmt, ap);
	(void)putc('\n', stderr);
}

void
#ifdef __STDC__
warn(const char *fmt, ...)	/* print warning message */
#else
warn(va_alist)va_dcl
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
	do_warn((Ulong)curfileno, curlineno, fmt, ap);
	va_end(ap);
}

void
#ifdef __STDC__
backwarn(Ulong fno, Ulong lno, const char *fmt, ...) /* back ref'd warning */
#else
backwarn(va_alist)va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;
	Ulong fno, lno;

	va_start(ap);
	fno = va_arg(ap, Ulong);
	lno = va_arg(ap, Ulong);
	fmt = va_arg(ap, char *);
#endif
	do_warn(fno, lno, fmt, ap);
	va_end(ap);
}

void
#ifdef __STDC__
exprwarn(const Expr *ep, const char *fmt, ...)	/* warning w/expr's location */
#else
exprwarn(va_alist)va_dcl
#endif
{
	Ulong fno, lno;
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	Expr *ep;
	char *fmt;

	va_start(ap);
	ep = va_arg(ap, Expr *);
	fmt = va_arg(ap, char *);
#endif
	exprfrom(&fno, &lno, ep);
	do_warn(fno, lno, fmt, ap);
	va_end(ap);
}
