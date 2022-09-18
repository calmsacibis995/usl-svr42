/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)grep:i386/cmd/grep/grep.c	1.22.2.1"
#ident "$Header: grep.c 1.3 91/08/28 $"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
/*
 * grep -- print lines matching (or not matching) a pattern
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <regexpr.h>
#include <sys/types.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

struct { const char *id, *msg; } errstr[] = {
	":173", "Range endpoint too large" ,
	":177", "Bad number" ,
	":186", "`\\digit' out of range" ,
	":199", "No remembered search string" ,
	":200", "'\\( \\)' imbalance" ,
	":201", "Too many `\\(' s" ,
	":202", "More than two numbers given in '\\{ \\}'" ,
	":203", "'\\}' expected" ,
	":204", "First number exceeds second in '\\{ \\}'" ,
	":207", "'[ ]' imbalance" ,
	":208", "Regular expression overflow" ,
	":224", "Illegal byte sequence",
	":209", "Regular expression error",
};

#define	BLKSIZE	512

char	*strrchr();
int	temp;
long	lnum;
char	linebuf[2*BUFSIZ];
char	prntbuf[2*BUFSIZ];
int	nflag;
int	bflag;
int	lflag;
int	cflag;
int	vflag;
int	sflag;
int	iflag;
int	hflag;
int	errflg;
int	nfile;
long	tln;
int	nsucc;
int	nlflag;
char	*ptr, *ptrend;
char 	*expbuf;
const char *pref_s, *pref_ld;

/* Enhanced Application Compatibility */
int	eflag;
int	fflag;
FILE	*exprfile;
char	*expr;
/* End Enhanced Application Compatibility */


main(argc, argv)
register argc;
char **argv;
{
	register	c;
	register char	*arg;
	extern int	optind;
	extern char	*optarg;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:grep");

	while((c=getopt(argc, argv, "hblcnsviye:f:")) != -1)
		switch(c) {
		case 'h':
			hflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'b':
			bflag++;
			break;

		/* Enhanced Application Compatibility */
		case 'e':
			eflag++;
			expr = optarg;
			break;

		case 'f':
			fflag++;
			exprfile = fopen(optarg, "r");
			if (exprfile == NULL) {
				pfmt(stderr, MM_ERROR, ":1127:Cannot open %s\n", optarg);
				exit(2);
			}
			break;
		/* Enhanced Application Compatibility */

		case 's':
			sflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 'y':
		case 'i':
			iflag++;
			break;
		case '?':
			errflg++;
		}

	if(errflg || (optind >= argc)) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":1079:Usage: grep [-hblcnsvi] [-e special_expression] [-f file] regular_expression [file . . .]\n");
		exit(2);
	}


	/* Enhanced Application Compatibility */
	if (eflag || fflag) {	/* Don't skip file, thinking it is expression */
		argc -= (optind -1);
		argv = &argv[optind -1];
	} else {
	/* End Enhanced Application Compatibility */
		argv = &argv[optind];
		argc -= optind;
	}

	nfile = argc - 1;
	/* Enhanced Application Compatibility */
	if (eflag || fflag) {
		if (strrchr(expr,'\n'))
			regerr(12);
	} else {
	/* End Enhanced Application Compatibility */
		if (strrchr(*argv,'\n'))
			regerr(12);
	}

	if (iflag) {
		for(arg = *argv; *arg != NULL; ++arg)
			*arg = (char)tolower((int)((unsigned char)*arg));
	}

	expbuf = compile((eflag || fflag) ? expr : *argv, (char *)0, (char *)0);
	if(regerrno)
		regerr(regerrno);

	if (--argc == 0)
		execute((char *)NULL);
	else
		while (argc-- > 0)
			execute(*++argv);

	exit(nsucc == 2 ? 2 : nsucc == 0);
}

execute(file)
register char *file;
{
	register char *lbuf, *p;
	int count, count1;
	
	if (file == NULL)
		temp = 0;
	else if ((temp = open(file, 0)) == -1) {
		if (!sflag)
			pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n", file,
				strerror(errno));
		nsucc = 2;
		return;
	}
	/* read in first block of bytes */
	if((count = bread(temp, prntbuf, BUFSIZ)) <= 0) {
		close(temp);

		if (cflag) {
			if (nfile>1 && !hflag && file)
				fprintf(stdout, pref_s ? pref_s :
					gettxt(":318", "%s:"), file);
			fprintf(stdout, "%ld\n", tln);
		}
		return;
	}
		
	lnum = 0;
	tln = 0;
	ptr = prntbuf;
	for(;;) {
		/* look for next newline */
		if((ptrend = memchr(ptr, '\n', prntbuf + count - ptr)) == NULL) {
			count = prntbuf + count - ptr;
			if(count <= BUFSIZ) {
				/* 
				 * shift end of block to beginning of buffer
				 * if necessary
				 * and fill up buffer until newline 
				 * is found 
				 */
				if(ptr != prntbuf)
				/* assumes memcpy copies correctly with overlap */
					memmove(prntbuf, ptr, count);
				p = prntbuf + count;
				ptr = prntbuf;
			} else {
				/*
				 * No newline in current block.
				 * Throw it away and get next
				 * block.
				 */
				count = 0;
				ptr = p = prntbuf;
			}
			if((count1 = bread(temp, p, BUFSIZ)) > 0) {
				count += count1;
				continue;
			}
			/* end of file - last line has no newline */
			ptrend = ptr + count;
			nlflag = 0;
		} else
			nlflag = 1;
		lnum++;
		*ptrend = '\0';
		if (iflag) {
			p = ptr;
			for(lbuf=linebuf; p < ptrend; )
				*lbuf++ = (char)tolower((int)(unsigned char)*p++);
			*lbuf = '\0';
			lbuf = linebuf;
		} else
			lbuf = ptr;

		if((!errflg)
		     && (step(lbuf, expbuf) ^ vflag) && succeed(file) == 1)
			break;	/* lflag only once */
		errflg = 0;
		if(!nlflag)
			break;
		ptr = ptrend + 1;
		if(ptr >= prntbuf + count) {
			/* at end of block; read in another block */
			ptr = prntbuf;
			if((count = bread(temp, prntbuf, BUFSIZ)) <= 0)
				break;
		}
	}
	close(temp);

	if (cflag) {
		if (nfile>1 && !hflag && file)
			fprintf(stdout, pref_s ? pref_s : gettxt(":318", "%s:"),
				file);
		fprintf(stdout, "%ld\n", tln);
	}
	return;
}


/* bread(fildes, buf, n) loops until n bytes have been read or  *
 * the end of file has been reached. This is necessary because  *
 * reading from a pipe or a device does not always              *
 * return complete lines at once.                               */

bread(fildes, buf, n)
int fildes;
char *buf;
unsigned n;
{
int i, rest, result;
char *tptr;

	result = 0;
	tptr = buf;
	rest = n;
	while(rest > 0 && (i = read(fildes, tptr, rest)) > 0) {
		rest -= i;
		tptr += i;
		result += i;
	}
	return(result);
}

succeed(f)
register char *f;
{
	int nchars;
	nsucc = (nsucc == 2) ? 2 : 1;
	if (f == NULL)
		f = "<stdin>";
	if (cflag) {
		tln++;
		return(0);
	}
	if (lflag) {
		fprintf(stdout, "%s\n", f);
		return(1);
	}

	if (nfile > 1 && !hflag)	/* print filename */
		fprintf(stdout, pref_s ? pref_s : gettxt(":318", "%s:"), f);

	if (bflag)	/* print block number */
		fprintf(stdout, pref_ld ? pref_ld : gettxt(":319", "%ld:"),
			(lseek(temp, 0L, 1)-1)/BLKSIZE);

	if (nflag)	/* print line number */
		fprintf(stdout, pref_ld ? pref_ld : gettxt(":319", "%ld:"), lnum);
	if(nlflag) {
		/* newline at end of line */
		*ptrend = '\n';
		nchars = ptrend - ptr + 1;
	} else
		nchars = ptrend - ptr;
	fwrite(ptr, 1, nchars, stdout);
	return(0);
}

regerr(err)
register err;
{
	switch(err) {
		case 11:
			err = 0;
			break;
		case 16:
			err = 1;
			break;
		case 25:
			err = 2;
			break;
		case 41:
			err = 3;
			break;
		case 42:
			err = 4;
			break;
		case 43:
			err = 5;
			break;
		case 44:
			err = 6;
			break;
		case 45:
			err = 7;
			break;
		case 46:
			err = 8;
			break;
		case 49:
			err = 9;
			break;
		case 50:
			err = 10;
			break;
		case 67:
			err = 11;
			break;
		default:
			err = 12;
			break;
	}
	pfmt(stderr, MM_ERROR, ":320:RE error %d: %s\n", err,
		gettxt(errstr[err].id, errstr[err].msg));

	exit(2);
}
