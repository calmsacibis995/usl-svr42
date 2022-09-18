/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/map.c	1.4.6.3"
#ident	"$Header: map.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function map.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/map.h"
#include "crash.h"

/* get arguments for map function */
int
getmap()
{
	struct syment *sp;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind])
		do {
			if(sp = symsrch(args[optind])) {
				fprintf(fp,"\n%s:\n",args[optind]);
				prmap(sp);
			}
			else fprintf(fp,"%s not found in symbol table\n",
				args[optind]);
		}while(args[++optind]);
	else longjmp(syn,0);
}

/* print map */
int
prmap(name)
struct syment *name;
{
	struct map mbuf;
	unsigned units = 0, seg = 0;
	unsigned long addr;

	readmem((long)name->n_value,1,-1,(char *)&mbuf,
		sizeof mbuf,"map table");
	addr = name->n_value + sizeof(mbuf);

	fprintf(fp,"MAPSIZE: %u\tSLEEP VALUE: %u\n",
		mbuf.m_size,
		mbuf.m_addr);
	fprintf(fp,"\nSIZE    ADDRESS\n");
	for(;;) {
		readmem(addr,1,-1,(char *)&mbuf,sizeof(mbuf),"map table");
		addr += sizeof(mbuf);
		if (!mbuf.m_size) {
			fprintf(fp,"%u SEGMENTS, %u UNITS\n",
				seg,
				units);
			return;
		}
		fprintf(fp,"%4u   %8x\n",
			mbuf.m_size,
			mbuf.m_addr);
		units += mbuf.m_size;
		seg++;
	}
}
