/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/snode.c	1.2.12.3"
#ident	"$Header: snode.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  snode.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/fs/snode.h>
#include <sys/fs/devmac.h>
#include "crash.h"

extern struct syment *Vfs, *Vfssw, *File;	/* namelist symbol pointers */ 
struct syment *Snode;
struct syment *Mac_installed;
int mac_on = 0; 	/* to verify if mac is installed or not */

/* get arguments for snode function */
int
getsnode()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	char *baseheading = "SLOT MAJ/MIN  REALVP     COMMONVP  NEXTR  SIZE    COUNT FLAGS\n";
	char *secheading = "SLOT MAJ/MIN  REALVP     COMMONVP  NEXTR  SIZE    COUNT FLAGS  S_DSECP  S_DSTATE S_DMODE S_SECFLAG D_LOLID D_HILID D_RELFLAG \n";
	char *heading;


	if(!Snode)
		if(!(Snode = symsrch("spectable")))
			error("snode table not found in symbol table\n");
	if (Mac_installed = symsrch("mac_installed"))
		readmem((long)Mac_installed->n_value, 1, -1, (char *)&mac_on,
		sizeof mac_on, "value of mac_installed ");
	if (mac_on)
		heading= secheading;
	else 	heading= baseheading;
	optind = 1;
	while((c = getopt(argcnt,args,"efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	fprintf(fp,"SNODE TABLE SIZE = %d\n", SPECTBSIZE);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		all = 1;
		do {
			getargs(SPECTBSIZE,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prsnode(all,full,slot,phys,addr,heading);
			else {
				if(arg1 >= 0 && arg1 < SPECTBSIZE)
					slot = arg1;
				else addr = arg1;
				prsnode(all,full,slot,phys,addr,heading);
			}
			slot = addr = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for(slot = 0; slot < SPECTBSIZE; slot++)
		prsnode(all,full,slot,phys,addr,heading);
}



/* print snode table */
int
prsnode(all,full,slot,phys,addr,heading)
int all,full,slot,phys;
long addr;
char *heading;
{
	struct snode *snp, snbuf;
	extern long lseek();
	struct devmac secmac;
	struct devmac *Ksecp;

	if(addr == -1) {
		readmem((long)(Snode->n_value+slot*sizeof snp),1,-1,
			(char *)&snp,sizeof snp,"snode address");
		if(snp == 0)
			return;
		readmem((long)snp,1,-1,(char *)&snbuf,sizeof snbuf,"snode table");
	}
	else
		readmem(addr,1,!phys,(char *) &snbuf,sizeof(snbuf),"snode table");

	while( 1 )
	{
	/* taken out, prints now referenced snodes, not just opened snodes */
	/*	if(!snbuf.s_count && !all)
			return ;
	*/
		if(full)
			fprintf(fp,"%s",heading);

		if(slot == -1)
			fprintf(fp,"  - ");
		else fprintf(fp,"%4d",slot);
		fprintf(fp," %4u,%-5u %8x    %8x %4d %5d    %5d ",
			getemajor(snbuf.s_dev),
			geteminor(snbuf.s_dev),
			snbuf.s_realvp,
			snbuf.s_commonvp,
			snbuf.s_nextr,
			snbuf.s_size,
			snbuf.s_count);

		fprintf(fp,"%s%s%s%s%s",
			snbuf.s_flag & SLOCKED ? " lk" : "",
			snbuf.s_flag & SUPD ? " up" : "",
			snbuf.s_flag & SACC ? " ac" : "",
			snbuf.s_flag & SWANT ? " wt" : "",
			snbuf.s_flag & SCHG ? " ch" : "");
		if (mac_on) {
			fprintf(fp, "%8x %5d %5d",
			snbuf.s_dsecp,
			snbuf.s_dstate,
			snbuf.s_dmode);
			fprintf(fp, "%s%s%s\n",
			snbuf.s_secflag & D_INITPUB ? " ipub" : "",
			snbuf.s_secflag & D_RDWEQ ? " rweq" : "",
			snbuf.s_secflag & D_NOSPECMACDATA ? " nomac" : "");
			Ksecp=snbuf.s_dsecp;
			if (Ksecp){
				readmem(Ksecp, 1, -1, (char *)&secmac, sizeof(secmac), "kernel security structure");
				fprintf(fp," %ld %ld %d",
				secmac.d_lolid,
				secmac.d_hilid,
				secmac.d_relflag);
			}
		}
		fprintf(fp, "\n");
		if(full)
		{
			/* print vnode info */
			fprintf(fp,"VNODE :\n");
			fprintf(fp,"VCNT VFSMNTED   VFSP   STREAMP VTYPE   RDEV    VDATA   VFILOCKS VFLAG     \n");
			cprvnode(&snbuf.s_vnode);
			fprintf(fp,"\n");
		}

		if(snbuf.s_next == NULL)
			return;
		snp = snbuf.s_next;
		readmem((long)snp,1,-1,(char *)&snbuf,sizeof snbuf,
			"snode table");
	}
}


