/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved. 					*/

#ident	"@(#)split:split.c	1.6.1.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/split/split.c,v 1.1 91/02/28 20:10:37 ccs Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>

unsigned count = 1000;
int	fnumber = 0;
char	*fname;
char	head[1024];
char	*ifil;
char	*ofil;
char	*tail;
char	*last;
FILE	*is;
FILE	*os;

main(argc, argv)
char *argv[];
{
	register i, c, f;
	int iflg = 0;
	struct statvfs stbuf;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:split");

	for(i=1; i<argc; i++)
		if(argv[i][0] == '-')
			switch(argv[i][1]) {
		
			case '\0':
				iflg = 1;
				continue;
		
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				count = atoi(argv[i]+1);
				continue;
			}
		else if(iflg)
			ofil = argv[i];
		else {
			ifil = argv[i];
			iflg = 2;
		}
	if (count <= 0) {
		pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":55:Usage: split [ -# ] [ file [ name ] ]\n");
		exit(1);
	}
	if(iflg != 2)
		is = stdin;
	else
		if((is=fopen(ifil,"r")) == NULL) {
			pfmt(stderr, MM_ERROR, ":3:Cannot open %s: %s\n",
				ifil, strerror(errno));
			exit(1);
		}
	if(ofil == 0)
		ofil = "x";
	else {
		if ((tail = strrchr(ofil, '/')) == NULL) {
			tail = ofil;
			getcwd(head, sizeof(head));
		}
		else {
			tail++;
			strcpy(head, ofil);
			last = strrchr(head, '/');
			*++last = '\0';
		}
		
		if (statvfs(head, &stbuf) < 0) {
			pfmt(stderr, MM_ERROR, ":56:%s: %s\n", head, strerror(errno));
			exit(1);
		}

		if (strlen(tail) > (stbuf.f_namemax-2) ) {
			pfmt(stderr, MM_ERROR, ":57:More than %d characters in output file name\n",
				stbuf.f_namemax-2);
			exit(1);
		}
	}
	fname=(char *)malloc(strlen(ofil));

loop:
	f = 1;
	for(i=0; i<count; i++)
	do {
		c = getc(is);
		if(c == EOF) {
			if(f == 0)
				fclose(os);
			exit(0);
		}
		if(f) {
			for(f=0; ofil[f]; f++)
				fname[f] = ofil[f];
			fname[f++] = fnumber/26 + 'a';
			fname[f++] = fnumber%26 + 'a';
			fname[f] = '\0';
			if(++fnumber > 676) {
				pfmt(stderr, MM_ERROR,
					":58:More than aa-zz output files needed, aborting split\n");
				exit(1);
			}
			if((os=fopen(fname,"w")) == NULL) {
				pfmt(stderr, MM_ERROR,
					":12:Cannot create %s: %s\n",
						fname, strerror(errno));
				exit(1);
			}
			f = 0;
		}
		putc(c, os);
	} while(c != '\n');
	fclose(os);
	goto loop;
}
