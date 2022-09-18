/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)tee:tee.c	1.6.2.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/tee/tee.c,v 1.1 91/02/28 20:12:14 ccs Exp $"
/*
 * tee-- pipe fitting
 */

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>

/*
** Exit values 
**
** EXITVAL_INVALARG:	Invalid argument(s) on the command line
** EXITVAL_ALLINVALARG:	All arguments on the command line are invalid
** EXITVAL_CMDFAIL:	Something with the command failed
*/
#define	EXITVAL_INVALARG	(2)
#define	EXITVAL_ALLINVALARG	(4)
#define	EXITVAL_CMDFAIL		(5)

int *openfp;	/* open file descriptor table */
int n = 1;	/* indicates number of files being written to */
int t = 0;	/*
		** flag to indicate if any output is going to a Device Special
		** File or a pipe.
		*/

char in[BUFSIZ];

void exit();
int write();
int fstat();
int stat();
long lseek();
int read();

extern int errno = 0;
long	lseek();

main(argc,argv)
char **argv;
{
	int register w;
	extern int optind;
	int c;
	int stdoutfd;
	struct stat buf;
	int errflg = 0;
	int aflag = 0;
	int numarg;		/* number of arguments on command line */
	int argfailcnt = 0;	/* number of arg(s) on command that had something wrong */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:tee");

	while ((c = getopt(argc, argv, "ai")) != EOF)
		switch(c) {
			case 'a':
				aflag++;
				break;
			case 'i':
				signal(SIGINT, SIG_IGN);
				break;
			case '?':
				errflg++;
		}
	if (errflg) {
		pfmt(stderr, MM_ACTION,
			":582:Usage: tee [ -i ] [ -a ] [file ] ...\n");
		exit(2);
	}

	argc -= optind;
	numarg = argc;
	argv = &argv[optind];
	stdoutfd = fileno(stdout);

	/*
	** Allocate open file descriptor table for (numarg + 1) entries.  The
	** extra entry is for stdout.
	*/
	if ((openfp = (int *)malloc(sizeof(int) * (numarg + 1)))
	 == (int *)NULL){
		pfmt(stderr, MM_ERROR,
			":1019:Cannot allocate memory\n");
		exit(EXITVAL_CMDFAIL);
	} else {
		openfp[0] = stdoutfd;
	}

	fstat(stdoutfd,&buf);
	t = (buf.st_mode&S_IFMT)==S_IFCHR;
	if(lseek(stdoutfd,0L,1)==-1&&errno==ESPIPE)
		t++;
	while(argc-->0) {
		if((openfp[n++] = open(argv[0],O_WRONLY|O_CREAT|
			(aflag?O_APPEND:O_TRUNC), 0666)) == -1) {
			if (errno == EACCES || errno == EISDIR ||
			 errno == ENOENT || errno == ENOTDIR) {
				pfmt(stderr, MM_ERROR,
				 ":5:Cannot access %s: %s\n",
				 argv[0], strerror(errno));
			} else {
				pfmt(stderr, MM_ERROR,
				 ":4:Cannot open %s: %s\n",
				 argv[0], strerror(errno));
			}
			argfailcnt++;
			n--;
		} else {
			if(stat(argv[0],&buf)>=0) {
				if((buf.st_mode&S_IFMT)==S_IFCHR)
					t++;
			} else {
				/*
				** The only other probable failure condition
				** for stat not covered by open would be
				** EOVERFLOW.  So set t anyway.  Chances are
				** t is set anyway since stdout is typically
				** to a Device Special File (DSF).  Even if
				** none of the files are DSFs, setting t will
				** just cause writes to files to be written in
				** smaller increments.
				*/
				t++;
			}
		}
		argv++;
	}

	w = 0;
	for(;;) {
		w = read(0, in, BUFSIZ);
		if (w > 0)
			stash(w);
		else
			break;
	}

	if (argfailcnt) {
		if (argfailcnt == numarg) {
			exit(EXITVAL_ALLINVALARG);
		} else {
			exit(EXITVAL_INVALARG);
		}
	} else {
		exit(0);
	}
}

stash(p)
int	p;	/* number of bytes to write */
{
	int k;
	int i;	/* index into input buffer from where to write output */
	int d;	/* number of bytes to write */

	/*
	** Write in 16 byte increments if any file we are writing to is a
	** DSF or pipe.
	*/
	d = (t ? 16 : p);
	for(i=0; i<p; i+=d)
		for(k=0;k<n;k++)
			write(openfp[k], in+i, (d<(p-i))?d:p-i);
}
