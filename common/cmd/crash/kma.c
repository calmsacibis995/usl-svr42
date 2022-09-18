/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/kma.c	1.1.7.3"
#ident	"$Header: kma.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function kmastat.
 */

#include <stdio.h>
#include <sys/types.h>
#include <syms.h>
#include <sys/sysinfo.h>
#include "crash.h"

extern struct syment *Kmeminfo, *Km_pools;

static void	kmainit(), prkmastat();

struct	kmeminfo kmeminfo;
int	km_pools[KMEM_NCLASS];

/* get arguments for kmastat function */
int
getkmastat()
{
	int c;

	kmainit();
	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	(void) redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	prkmastat();
	return(0);
}

/* print kernel memory allocator statistics */
static void
prkmastat()
{

	readmem(Kmeminfo->n_value,1,Procslot,(char *)&kmeminfo,
		sizeof(struct kmeminfo),"kmeminfo");
	readmem(Km_pools->n_value,1,Procslot,(char *)&km_pools,
		sizeof(km_pools),"km_pools");

	(void) fprintf(fp,
	"                       total bytes     total bytes\n");
	(void) fprintf(fp,
	"size        # pools       in pools       allocated     # failures\n");
	(void) fprintf(fp,
	"-----------------------------------------------------------------\n");

	(void) fprintf(fp,
	"small       %3u         %10u      %10u     %3u\n",
		km_pools[KMEM_SMALL], kmeminfo.km_mem[KMEM_SMALL],
		kmeminfo.km_alloc[KMEM_SMALL], kmeminfo.km_fail[KMEM_SMALL]);
	(void) fprintf(fp,
	"big         %3u         %10u      %10u     %3u\n",
		km_pools[KMEM_LARGE], kmeminfo.km_mem[KMEM_LARGE],
		kmeminfo.km_alloc[KMEM_LARGE], kmeminfo.km_fail[KMEM_LARGE]);
	(void) fprintf(fp,
	"outsize       -                  -      %10u     %3u\n",
		kmeminfo.km_alloc[KMEM_OSIZE], kmeminfo.km_fail[KMEM_OSIZE]);
	
	return;
}

/* initialization for namelist symbols */
static void
kmainit()
{
	static int kmainit_done = 0;

	if(kmainit_done)
		return;
	if(!Kmeminfo)
		if(!(Kmeminfo = symsrch("kmeminfo")))
			(void) error("kmeminfo not found in symbol table\n");
	if(!Km_pools)
		if(!(Km_pools = symsrch("Km_pools")))
			(void) error("Km_pools not found in symbol table\n");
	kmainit_done = 1;
	return;
}
