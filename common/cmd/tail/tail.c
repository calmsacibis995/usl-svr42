/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tail:tail.c	1.16.2.4"
#ident  "$Header: tail.c 1.2 91/06/27 $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/* tail command 
**
**	tail where [file]
**	where is [+|-]n[type]
**	- means n lines before end
**	+ means nth line from beginning
**	type 'b' means tail n blocks, not lines
**	type 'c' means tail n characters
**	Type 'r' means in lines in reverse order from end
**	 (for -r, default is entire buffer )
**	option 'f' means loop endlessly trying to read more
**		characters after the end of file, on the  assumption
**		that the file is growing
*/

#include	<stdio.h>
#include	<ctype.h>
#include	<sys/signal.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<string.h>

#define	LBIN	4097
#define BSHIFT	9	/* log2(512)=9, 512 is standard buffer size */

struct	stat	statb;
char	bin[LBIN];
extern	int	errno;
int	follow;		/* f option flag; 0 not following, 1 following*/
int	piped;		/*
			** Indicates whether or not stdin is from a pipe;
			** 0 no, non-zero yes.
			*/

main(argc,argv)
char **argv;
{
	register i,j,k;
	long	n;	/*
			** Relative position in file in units (lines,
			** characters, blocks) to start output.
			*/
	long	di;
	int	fromend;/*
			** Indicates relative position in file where tail
			** starts its output; 0 indicates from the beginning,
			** 1 indicates from the end.
			*/
	int	partial;
	int	bylines;/*
			** Indicates whether or not output is by lines;
			** 0 indicates not by lines, 1 indicates by lines,
			** -1 is a precondition state.
			*/
	int	bkwds;	/*
			** r option flag indicates the order of the output;
			** 0 indicates from relative beginning of file to end,
			** 1 indicates reverse order, relative end of file to
			** relative beginning.
			*/
	int	lastnl;
	char	*p;
	char	*arg;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:tail");

	lseek(0,(long)0,1);
	piped = errno==ESPIPE;
	arg = argv[1];
	/* default settings if no options are specified */
	if(argc<=1 || *arg!='-'&&*arg!='+') {
		arg = "-10l";
		argc++;
		argv--;
	}
	fromend = *arg=='-';
	arg++;
	if (isdigit(*arg)) {
		n = 0;
		while(isdigit(*arg))
			n = n*10 + *arg++ - '0';
	}
	else
		n = -1;	/* indicates no unit count specified */
	/* ignore input from pipe (if any) if a file is specified */
	if(argc>2) {
		close(0);
		piped = 0;
		if(open(argv[2],0)!=0) {
			pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",
				argv[2], strerror(errno));
			exit(2);
		}
	}

	bylines = -1;
	bkwds = 0;
	/*
	** 'l' 'b' and 'c' options are mutually exclusive.
	** 'b' and 'c' options can not be used with the 'r' option
	** 'r' and 'f' options are mutually exclusive.
	*/
	while(*arg)
	switch(*arg++) {
	case 'b':
		if(n == -1) n = 10;
		n <<= BSHIFT;
		if(bylines!=-1 || bkwds==1) goto errcom;
		bylines=0;
		break;
	case 'c':
		if(bylines!=-1 || bkwds==1) goto errcom;
		bylines=0;
		break;
	case 'f':
		if(bkwds) goto errcom;
		follow = 1;
		break;
	case 'r':
		if(follow) goto errcom;
		if (n==-1) n = LBIN;
		if (bylines==0) goto errcom;
		bkwds = 1;
		/*
		** r option is always from relative end of file regardless of
		** sign.
		*/
		fromend = 1;
		break;
	case 'l':
		if(bylines!=-1 && bylines==1) goto errcom;
		bylines = 1;
		break;
	default:
		goto errcom;
	}
	if (n == -1) n = 10;
	if(!fromend&&n>0)
		n--;
	if(bylines==-1) bylines = 1;
	if(fromend)
		goto keep;

	/*seek from beginning */

	if(bylines) {
		j = 0;
		while(n-->0) {
			do {
				if(j--<=0) {
					p = bin;
					j = read(0,p,512);
					if(j--<=0)
						fexit();
				}
			} while(*p++ != '\n');
		}
		write(1,p,j);
	} else  if(n>0) {
		if(!piped)
			fstat(0,&statb);
		if(piped||(statb.st_mode&S_IFMT)==S_IFCHR)
			while(n>0) {
				i = n>512?512:n;
				i = read(0,bin,i);
				if(i<=0)
					fexit();
				n -= i;
			}
		else
			lseek(0,n,0);
	}
copy:
	while((i=read(0,bin,512))>0)
		write(1,bin,i);
	fexit();

	/*seek from end*/

keep:
	if(n < 0)
		fexit();
	if(!piped) {
		fstat(0,&statb);
		di = !bylines&&n<LBIN?n:LBIN-1;
		if(statb.st_size > di)
			lseek(0,-di,2);
		if (!bylines) 
			goto copy;
	}
	partial = 1;
	for(;;) {
		i = 0;
		do {
			j = read(0,&bin[i],LBIN-i);
			if(j<=0)
				goto brka;
			i += j;
		} while(i<LBIN);
		partial = 0;
	}
brka:
	if(!bylines) {
		k =
		    n<=i ? i-n:
		    partial ? 0:
		    n>=LBIN ? i+1:
		    i-n+LBIN;
		k--;
	} else {
		if(bkwds && bin[i==0?LBIN-1:i-1]!='\n'){	/* force trailing newline */
			bin[i]='\n';
			if(++i>=LBIN) {i = 0; partial = 0;}
		}
		k = i;
		j = 0;
		do {
			lastnl = k;
			do {
				if(--k<0) {
					if(partial) {
						if(bkwds) 
						    (void)write(1,bin,lastnl+1);
						goto brkb;
					}
					k = LBIN -1;
				}
			} while(bin[k]!='\n'&&k!=i);
			if(bkwds && j>0){
				if(k<lastnl) (void)write(1,&bin[k+1],lastnl-k);
				else {
					(void)write(1,&bin[k+1],LBIN-k-1);
					(void)write(1,bin,lastnl+1);
				}
			}
		} while(j++<n&&k!=i);
brkb:
		if (bkwds) exit(0);
		if(k==i) do {
			if(++k>=LBIN)
				k = 0;
		} while(bin[k]!='\n'&&k!=i);
	}
	if(k<i)
		write(1,&bin[k+1],i-k-1);
	else {
		write(1,&bin[k+1],LBIN-k-1);
		write(1,bin,i);
	}
	fexit();
errcom:
	pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	pfmt(stderr, MM_ACTION,
		":401:Usage:\ttail [+/-[n][lbc][f]] [file]\n\t\t\ttail [+/-[n][l][r|f]] [file]\n");
	exit(2);
}
fexit()
{	register int n;
	if (!follow || piped) exit(0);
	for (;;)
	{	sleep(1);
		while ((n = read (0, bin, 512)) > 0)
			write (1, bin, n);
	}
}
