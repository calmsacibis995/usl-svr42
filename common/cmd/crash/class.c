/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/class.c	1.2.7.3"
#ident	"$Header: class.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  class, claddr
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/priocntl.h>
#include <sys/tspriocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/class.h>
#include "crash.h"


struct syment *Cls;	/* namelist symbol */
struct syment *Ncls;	/* namelist symbol for number of classes */
class_t *classaddr;

/* get arguments for class function */
int
getclass()
{
	int slot = -1;
	int c, i;
	unsigned long arg1 = -1;
	unsigned long arg2 = -1;

	char *classhdg = "SLOT\tCLASS\tINIT FUNCTION\tCLASS FUNCTION\n\n";
	int nclass;

	if(!Cls)
		if(!(Cls = symsrch("class")))
			error("class not found in symbol table\n");
	if(!Ncls)
		if(!(Ncls = symsrch("nclass")))
			error("nclass not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}


	/* Determine how many entries are in the class table */

	readmem((long)Ncls->n_value, 1, -1, (char *)&nclass, 
		sizeof(int), "number of classes");

	/* Allocate enough space to read in the whole table at once */

	classaddr=(class_t *)malloc(nclass * sizeof(class_t));

	/* Read in the entire class table */

	readmem((long)Cls->n_value, 1, -1, (char *)classaddr, 
		nclass*sizeof(class_t), "class table");

	fprintf(fp, "%s", classhdg);

	if(args[optind]){
		do {
			getargs(nclass,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prclass(slot);
			else {
				if(arg1 >= 0 && arg1 < nclass)
					prclass(arg1);
				else
					fprintf(fp,"invalid class slot %d\n",
							arg1);
			}
			arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for(slot = 0; slot < nclass; slot++)
		prclass(slot);

	free(classaddr);
}

/* print class table  */
int
prclass(slot)
int slot;
{
	char name[PC_CLNMSZ];

	readmem((long)classaddr[slot].cl_name, 1, -1, name,
		sizeof name, "class name");

	fprintf(fp, "%d\t%s\t%x\t%x\n", slot, name, classaddr[slot].cl_init,
		classaddr[slot].cl_funcs);
}
