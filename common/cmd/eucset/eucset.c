/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eucset:eucset.c	1.1.2.3"
#ident  "$Header: eucset.c 1.2 91/06/26 $"

/*
 ****************************************************************
 *								*
 *	eucset.c						*
 *								*
 *	Assuming the line discipline is an EUC line disc,	*
 *	set the cswidth info which is obtained from character 	*
 *	class table (if one) into the line discipline, if no	*
 *	arguments.  If an arg is given it overrides the cswidth	*
 *	of the environment.  If there is one arg and it is	*
 *	"-p", then the current values of the EUC widths are	*
 *	obtained from the linediscipline and printed.		*
 *	Return values are 0 for successful set/get; 1 for	*
 *	failure of any kind.					*
 *								*
 ****************************************************************
 */


#include <stdio.h>
#include <termio.h>
#include <sys/euc.h>	/* for EUC width structure */
#include <getwidth.h>	/* for EUC width function */
#include <stropts.h>	/* Streams specific ioctls */
#include <locale.h>	/* for setlocale() */
#include <pfmt.h>

/*
 * Driver Include File (relative to where we are):
 */
#include <sys/eucioctl.h>


#define DEF_CSWIDTH	"1:1,0:0,0:0"

struct eucioc eucw;	/* for EUC_WSET/EUC_WGET to line discipline */
eucwidth_t width;	/* return struct from getwidth() */

char *retval;		/* hold value of environment CSWIDTH */
char *eucc;		/* for command line arg */

char *getenv();
void __getwidth();

/*
 * MAIN	------------------------------------------------------------
 */

main(argc,argv)

	int argc;
	char **argv;
{

	/* Initialize locale, message label, and message catalog. */
	(void)setlocale(LC_ALL,"");
	setlabel("UX:eucset");
	setcat("uxsysadm");

	/*
	 * No arguments?  Look in environment for cswidth info
	 *
	 *	If found in env and not NULL, use it silently.
	 *	If found in env and NULL, use DEF_CSWIDTH and warn user.
	 *	If NOT found in env, use DEF_CSWIDTH (_getwidth does it).
	 */
	if (argc == 1) {
		getwidth(&width);
	}
	/*
	 * One arg?  See if it's "-?" or "-p".  If not, assume it is
	 * supposed to be a cswidth specification.  Call the "real"
	 * getwidth function to split up the string and make eucwidth_t.
	 */
	else if (argc == 2) {
		eucc = *++argv;
		if (strcmp(eucc, "-p") == 0) {
			euclook(&eucw);
			exit(0);
		}
		else if (*eucc == '-') {
			usage();
		}
		__getwidth(&width, eucc);
	}
	else {
		usage();
	}
	mk_euc(&eucw, &width);	/* make eucwidth struct for line disc. */
	set_euc(&eucw);		/* send ioctl downstream */
	exit(0);
}

/*
 * usage	Print usage mesage & go away.
 */

usage()
{
	pfmt(stderr, MM_ACTION, 
		":9:Usage: eucset [ cswidth ]\n       eucset -p\n");
	exit(1);
}

/*
 * mk_euc	Take eucioc pointer and eucwidth_t pointer.
 *		Make eucioc from eucwidth_t.
 */

mk_euc(w, cw)

	struct eucioc *w;
	eucwidth_t *cw;
{
	w->eucw[0] = '\001';
	w->scrw[0] = '\001';

	w->eucw[1] = cw->_eucw1;
	w->scrw[1] = cw->_scrw1;

	w->eucw[2] = cw->_eucw2;
	w->scrw[2] = cw->_scrw2;

	w->eucw[3] = cw->_eucw3;
	w->scrw[3] = cw->_scrw3;
}

/*
 * set_euc	Send EUC code widths to line discipline.
 */

set_euc(e)

	struct eucioc *e;
{
	struct strioctl sb;

	sb.ic_cmd = EUC_WSET;
	sb.ic_timout = 0;
	sb.ic_len = sizeof(struct eucioc);
	sb.ic_dp = (char *) e;

	if (ioctl(0, I_STR, &sb) < 0)
		fail();
}

/*
 * euclook	Get current EUC code widths from line discipline.
 */

euclook(e)

	struct eucioc *e;
{
	struct strioctl sb;

	sb.ic_cmd = EUC_WGET;
	sb.ic_timout = 0;
	sb.ic_len = sizeof(struct eucioc);
	sb.ic_dp = (char *) e;
	if (ioctl(0, I_STR, &sb) < 0)
		fail();
	printf("cswidth %d:%d,%d:%d,%d:%d\n",
					e->eucw[1], e->scrw[1],
					e->eucw[2], e->scrw[2],
					e->eucw[3], e->scrw[3]);
}

/*
 * fail		Print a message and go away.
 */

fail()

{
	pfmt(stderr, MM_ERROR, ":10:%s failed\n", "EUC_WSET");
	exit(1);
}

/*
 * format	print "bad format" message & exit.
 */

format()

{
	pfmt(stderr, MM_ERROR, ":11:bad cswidth format\n");
	exit(1);
}

/*
 * __getwidth	get cswidth info. from given parameter.
 */

void __getwidth(wp, s)
eucwidth_t *wp;
register char *s;
{
	int w[3], sw[3];
	register int i;

	wp->_eucw1 = wp->_scrw1 = 1;
	wp->_eucw2 = wp->_eucw3 = wp->_scrw2 = wp->_scrw3 = 0;
	wp->_pcw = sizeof(unsigned short);
	wp->_multibyte = 0;

	w[0] = w[1] = w[2] =  sw[0] = sw[1] = sw[2] = 0;
	if(s) {
		for(i = 0; *s && i<3; i++) {
			switch (*s++) {
			case '1': w[i] = sw[i] = 1;
				goto subfield;
			case '2': w[i] = sw[i] = 2;
				goto subfield;
			case '3': w[i] = sw[i] = 3;
				goto subfield;
			case '4': w[i] = sw[i] = 4;
			subfield:
				for( ; *s && *s != ':' && *s != ','; s++);
				if(!*s || *s++==',') break;
				switch (*s++) {
				case '1': sw[i] = 1;
					break;
				case '2': sw[i] = 2;
					break;
				case '3': sw[i] = 3;
					break;
				case '4': sw[i] = 4;
					break;
				case ',':
					continue;
				}
			default: while(*s && *s++ != ',');
			case ',': break;
			}
		}
		wp->_eucw1 = w[0];
		wp->_eucw2 = w[1];
		wp->_eucw3 = w[2];
		if (w[0] > 1  || w[1] || w[2])
			wp->_multibyte = 1;
		if (w[0] > 2  || w[1] > 2 || w[2] > 2 )
			wp->_pcw = sizeof(unsigned long);
		wp->_scrw1 = sw[0];
		wp->_scrw2 = sw[1];
		wp->_scrw3 = sw[2];
	}
	return;
}
