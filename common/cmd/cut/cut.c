/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cut:cut.c	1.11.1.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/cut/cut.c,v 1.1 91/02/28 16:48:46 ccs Exp $"
#
/* cut : cut and paste columns of a table (projection of a relation) */
/* Release 1.5; handles single backspaces as produced by nroff    */

# include <stdio.h>	/* make: cc cut.c */
# include <ctype.h>
# include <locale.h>
# include <pfmt.h>
# include <errno.h>
# include <string.h>
#include <sys/euc.h>	
#include <getwidth.h>

# define NFIELDS 1024	/* max no of fields or resulting line length */
# define BACKSPACE '\b'
#define MLTDUMB ' '

int strcmp(), atoi();
void exit();

void usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	pfmt(stderr, MM_ACTION,
		":137:Usage:\n\tcut [-s] [-d<char>] {-c<list> | -f<list>} file ...\n");
	exit(2);
}

main(argc, argv)
int	argc;
char	**argv;
{
	extern int	optind;
	extern char	*optarg;
	static char	cflist[] = ":138:Bad list for c/f option\n";
	register int	c;
	register char	*p1, *rbuf;
	register char	*p, *list;
	int errcnt = 0;		/* exit value from main */
	void diag();
	void usage();

	/* permits multibyte delimiter */
	char	*del;
	int	num, j, count, poscnt, r, s;
	int	endflag, supflag, cflag, fflag, backflag, filenr;
	static  int	sel[NFIELDS];
	char 	buf[NFIELDS];
	char	*p2, outbuf[NFIELDS];
	FILE	*inptr;
	eucwidth_t wp;
	int eucwc;
	int scrwc;
	int mltflag = 0;
	int delflag = 0;
	int dumb = 0;
	char *pdel;
	int delw = 1;
	int trailflag = 0;
	int i;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:cut");

	eucwc = 0;
	del = "\t";

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;
	supflag = cflag = fflag = r = num = s = 0;

	while((c = getopt(argc, argv, "c:d:f:s")) != EOF)
		switch(c) {
			case 'c':
				if (fflag)
					usage(1);
				cflag++;
				list = optarg;
				break;
			case 'd':
			/* permits multibyte delimiter 	*/
				if (ISASCII(*optarg))
					delw = 1;
				else
					delw = ISSET2(*optarg) ? wp._eucw2 :
					       ISSET3(*optarg) ? wp._eucw3 :
								 wp._eucw1 ;
				if ((int)strlen(optarg) > delw)
					diag(":139:No delimiter\n");
				else
					del = optarg;
				break;
			case 'f':
				if (cflag)
					usage(1);
				fflag++;
				list = optarg;
				break;
			case 's':
				supflag++;
				break;
			case '?':
				usage(0);
		}

	argv = &argv[optind];
	argc -= optind;

	if (!(cflag || fflag))
		diag(cflist);

	do {
		p = list;
		switch(*p) {
			case '-':
				if (r)
					diag(cflist);
				r = 1;
				if (num == 0)
					s = 1;
				else {
					s = num;
					num = 0;
				}
				break;
			case '\0' :
			case ','  :
				if (num >= NFIELDS)
					diag(cflist);
				if (r) {
					if (num == 0)
						num = NFIELDS - 1;
					if (num < s)
						diag(cflist);
					for (j = s; j <= num; j++)
						sel[j] = 1;
				} else
					sel[num] = (num > 0 ? 1 : 0);
				s = num = r = 0;
				if (*p == '\0')
					continue;
				break;
			default:
				if (!isdigit(*p))
					diag(cflist);
				num = atoi(p);
				while (isdigit(*list))
					list++;
				continue;
		}
		list++;
	}while (*p != '\0');
	for (j=0; j < NFIELDS && !sel[j]; j++);
	if (j >= NFIELDS)
		diag(":140:No fields\n");

	filenr = 0;
	do {	/* for all input files */
		if ( argc == 0 || strcmp(argv[filenr],"-") == 0 )
			inptr = stdin;
		else
			if ((inptr = fopen(argv[filenr], "r")) == NULL) {
				pfmt(stderr, MM_WARNING,
					":92:Cannot open %s: %s\n",
					argv[filenr], strerror(errno));
				errcnt = 1;
				continue;
			}
		endflag = 0;
		do {	/* for all lines of a file */
			count = poscnt = backflag = 0;
			p1 = &outbuf[0] - 1 ;
			p2 = p1;
			rbuf = buf;
			if ((fgets(buf, NFIELDS, inptr)) == NULL) {
				endflag = 1;
				continue;
			}
			do { 	/* for all char of the line */
				if (rbuf >= &buf[NFIELDS])
					diag(":141:Line too long\n");
				delflag = 0;
				pdel = del;
				if (*rbuf != '\n')
					if (ISASCII(*rbuf)) {
						*++p1 = *rbuf;
						if (*rbuf == *pdel)
							delflag = 1;
					} else {
						mltflag = 1;
						if (ISSET2(*rbuf)) {
							eucwc = wp._eucw2;
							scrwc = wp._scrw2;
						} else
						if (ISSET3(*rbuf)) {
							eucwc = wp._eucw3;
							scrwc = wp._scrw3;
						} else {
							eucwc = wp._eucw1;
							scrwc = wp._scrw1;
						}
						do {
							*++p1 = *rbuf;
							if (*rbuf == *pdel)
								delflag++;
							rbuf++;
							pdel++;
						} while (--eucwc);
						if (delflag != delw)
							delflag = 0;
						rbuf--;
						pdel--;
					}
				if (cflag && (*rbuf == BACKSPACE))
					backflag++;
				else if (!backflag)
					poscnt += 1;
				else
					backflag--;
				if ( backflag > 1 )
					diag(":142:Cannot handle multiple adjacent backspaces\n");
				/* permits mulutibyte delimiter */
				if (*rbuf == '\n' && count > 0  || delflag || cflag) {
					count += 1;
					if (fflag)
						poscnt = count;
				/* consider multiple adjacent EUC code */
					if (mltflag && cflag) {
						for (i = poscnt; i < (poscnt + scrwc); i++) {
							if (sel[i])
								continue;
							else
								dumb++;
						}
						if (dumb) {
							p1 = p2;
							if (dumb != scrwc) {
								for (i = 0; i < (scrwc - dumb); i++) {
									p1++;
									*p1 = MLTDUMB;
								}
								p2 = p1;
							}
							dumb = 0;
						} else {
							p2 = p1;
							trailflag = delflag ? 1 : 0;
						}
						mltflag = 0;
						poscnt += (scrwc - 1);
					} else {
						if (sel[poscnt]) {
							p2 = p1;
							trailflag = delflag ? 1 : 0;
						} else
							p1 = p2;
					}
				}
			} while (*rbuf++ != '\n');
			if ( !endflag && (count > 0 || !supflag)) {
			/* permits mulutibyte delimiter */
				if (trailflag && !sel[count]) {
					p1 -= (delw-1);
					*p1 = '\0'; /*suppress trailing delimiter*/
					trailflag = 0;
				}
				else
					*++p1 = '\0';
				(void)puts(outbuf);
			}
		} while (!endflag);
		(void)fclose(inptr);
	} while (++filenr < argc);

	exit(errcnt);
	/* NOTREACHED */
}


void diag(s)
char	*s;
{
	pfmt(stderr, MM_ERROR, s);
	exit(2);
}
