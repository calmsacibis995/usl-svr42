/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/disp.c	1.1.8.3"
#ident	"$Header: disp.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  dispq
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/priocntl.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include "crash.h"


struct syment	*Nglobpris;	/* namelist symbol */
int		nglobpris;

struct syment	*Dispq;		/* namelist symbol */
dispq_t		*dispqbuf;

/* get arguments for dispq function */
int
getdispq()
{
	int slot = -1;
	int c, i;
	long arg1 = -1;
	long arg2 = -1;
	dispq_t	*dispqaddr;

	char *dispqhdg = "SLOT     DQ_FIRST     DQ_LAST     RUNNABLE COUNT\n\n";

	if(!Nglobpris && !(Nglobpris = symsrch("nglobpris")))
		error("nglobpris not found in symbol table\n");

	readmem((long)Nglobpris->n_value, 1, -1, (char *)&nglobpris,
		sizeof nglobpris, "nglobpris");

	if(!Dispq && !(Dispq = symsrch("dispq")))
		error("dispq not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}


	/* Allocate enough space to read in the whole table at once */

	dispqbuf = (dispq_t *)malloc(nglobpris * sizeof(dispq_t));

	/* Read the dispq table address */

	readmem((long)Dispq->n_value, 1, -1, (char *)&dispqaddr, 
		sizeof(dispq_t *), "dispq address");

	/* Read in the entire table of dispq headers */

	readmem((long)dispqaddr, 1, -1, (char *)dispqbuf,
	    nglobpris * sizeof(dispq_t), "dispq header table");

	fprintf(fp, "%s", dispqhdg);

	if(args[optind]){
		do {
			getargs(nglobpris,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prdispq(slot);
			else {
				if(arg1 >=0 && arg1 < nglobpris)
					prdispq(arg1);
				else
					fprintf(fp,"invalid dispq slot: %d\n",arg1);
			}
			slot = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for(slot = 0; slot < nglobpris; slot++)
		prdispq(slot);

	free(dispqbuf);
}

/* print dispq header table  */
int
prdispq(slot)
int slot;
{
	fprintf(fp, "%4d     %8x    %8x          %4d\n",
		slot, dispqbuf[slot].dq_first,
		dispqbuf[slot].dq_last, dispqbuf[slot].dq_sruncnt);
}
