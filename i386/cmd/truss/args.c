/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/args.c	1.1"
#ident	"$Header: args.c 1.1 91/07/09 $"

#include <stdio.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

void
showargs(Pr, raw)	/* display arguments to successful exec() */
	register process_t *Pr;
	int raw;
{
	int nargs;
	char * ap;
	char * sp;

	length = 0;

	Pgetareg(Pr, R_SP); ap = (char *)Pr->REG[R_SP]; /* UESP */

	if ( Pread(Pr, (long)ap, (char *)&nargs, sizeof(nargs))
	     != sizeof(nargs)) {
		printf("\n%s\t*** Bad argument list? ***\n", pname);
		return;
	}
	if (debugflag) {
		int i, n, stack[256];
		n = 0x7fffffff - (long)ap;
		if ( n > 1024 ) n = 1024;
		fprintf(stderr, "ap = 0x%x, nargs = %d, stacksize = %d\n",
						ap, nargs, n);
		Pread(Pr, (long)ap, (char *)stack, n);
		for ( i = 0 ; i < 256 ; i++ ) {
			if ( (n -= 4) < 0 )
				break;
			fprintf(stderr, "%08x:	%8x\n", ap + 4 * i, stack[i]);
		}
	}

	(void) printf("  argc = %d\n", nargs);
	if (raw)
		showpaths(&systable[SYS_exec]);

	show_cred(Pr, FALSE);

	if (aflag || eflag) {		/* dump args or environment */

		/* enter region of (potentially) lengthy output */
		Eserialize();

		ap += sizeof(int);

		if (aflag)		/* dump the argument list */
			dumpargs(Pr, ap, "argv:");

		ap += (nargs+1) * sizeof(char *);

		if (eflag)		/* dump the environment */
			dumpargs(Pr, ap, "envp:");

		/* exit region of lengthy output */
		Xserialize();
	}
}

unsigned
getargp(Pr, nbp)		/* get address of arguments to syscall */
	register process_t *Pr;
	int *nbp;		/* also return # of bytes of arguments */
{
	unsigned ap, sp;
	int nabyte;

	(void) Pgetareg(Pr, R_SP); sp = Pr->REG[R_SP];
	ap = sp + sizeof(int);
	nabyte = 512;
	if (nbp != NULL)
		*nbp = nabyte;
	return ap;
}
