/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-cmd:i386/machdep.c	1.7"

#include	"cc.h"

static char	*Mach = NULL;	/* will hold the -Ki386, -Ki486 or -KP5 (i586)
				   argument */
static char	*Frame = NULL;  /* will hold the -Kno_frame and -Kframe arg.*/
static char	*Inline= NULL;  /* will hold the -Kinline and -Kno_inline arg.*/

void initvars()
{
	return;
}

int Kelse(s)
char *s;
{
	if (strcmp(s, "i386")==0)
		Mach = "-386";
	else if (strcmp(s, "i486")==0)
		Mach = "-486";
	else if (strcmp(s, "P5")==0)
		Mach = "-586";
	else if (strcmp(s, "frame")==0)
		Frame = "-_e";
	else if (strcmp(s, "inline")==0)
		Inline = "-1I1";
	else if (strcmp(s, "ieee")==0) {
		addopt(Xc0, "-2i1");
		addopt(Xc2, "-Kieee");
	} else if (strcmp(s, "noieee")==0) {
		addopt(Xc0, "-2i0");
		addopt(Xc2, "-Knoieee");
	} else if ((strcmp(s, "no_inline")==0) || (strcmp(s, "no_frame")==0))
		return 1;	
	else
		return 0;
	return 1;
}

int Yelse(c, np)
int c;
char *np;
{
	return (0);
}

Welse(c)
char c;
{
	return (-1);
}

int optelse(c, s)
int c;
char *s;
{
	switch(c) {
	case 'Z':	/* acomp: pack structures for 386 */
		if (!s) {
			error('e', "Option -Z requires an argument\n");
			exit(1);
		} else if (*s++ != 'p') {
			error('e', "Illegal argument to -Z flag\n");
			exit(1);
		}
		switch ( *s ) {
		case '1':
			addopt(Xc0, "-Zp1");
			break;
		case '2':
			addopt(Xc0, "-Zp2");
			break;
		case '4':
			addopt(Xc0, "-Zp4");
			break;
		case '\0':
			addopt(Xc0, "-Zp1");
			break;
		default:
			error('e', "Illegal argument to -Zp flag\n");
			exit(1);
		}
		return 1;
	}
	return 0;
}

void init_mach_opt()
{
	return;
}

void add_mach_opt()
{
	if (!Oflag)
		return;

	if (Mach != NULL)
		addopt(Xc2, Mach);
	if (Frame != NULL)
		addopt(Xc2, Frame);
	if (Inline != NULL)
		addopt(Xc0, Inline);
	
	return;

}

void mach_defpath()
{
	return;
}

void AVmore()
{
	if (Oflag)	/* pass -O to acomp */
		addopt(AV, "-O");

	if (Eflag || Pflag)
		return;

	if (pflag)
		addopt(AV,"-p");

	return;
}

/*===================================================================*/
/*                                                                   */
/*                      OPTIMIZER                                    */
/*                                                                   */
/*===================================================================*/
int optimize (i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
		addopt(AV,passname(prefix, N_OPTIM));

	addopt(AV,"-I");
	addopt(AV,c_out);
	addopt(AV,"-O");
	addopt(AV,as_in
		 = (Sflag && !qarg) ? setsuf(list[CF][i], 's') : tmp5);
	for (j = 0; j < nlist[Xc2]; j++)
		addopt(AV,list[Xc2][j]);

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	if (callsys(passc2, list[AV])) {
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
		error('w', "Optimizer failed, -O ignored for %s\n", list[CF][i]);
	} else {
		c_out= as_in;
		cunlink(tmp2);
		}

#ifdef PERF
	STATS("optimizer");
#endif

	return(1);
}

void option_mach()
{
	OPTPTR("\t[-Zp[124]]: pack structure for i386 acomp.\n");

	return;
}
