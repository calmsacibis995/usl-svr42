/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/vfssw.c	1.1.6.3"
#ident	"$Header: vfssw.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function vfssw.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/var.h>
#include <sys/fstyp.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include "crash.h"

struct syment  *Nfstype;		/*namelist symbol pointers */
struct syment *Vfssw;

/* get arguments for vfssw function */
int
getvfssw()
{
	int slot = -1;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	int nfstypes;

	if(!Vfssw)
		if((Vfssw = symsrch("vfssw")) == NULL)
			error("vfssw not found in symbol table\n");
	if(!Nfstype)
		if((Nfstype = symsrch("nfstype")) == NULL)
			error("nfstype not found in symbol table\n");


	optind = 1;
	while((c = getopt(argcnt,args,"pw:")) !=EOF) {
		switch(c) {
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	readmem((long)Nfstype->n_value,1,-1,(char *)&nfstypes,
		sizeof (int), "number of file systems types");
	fprintf(fp,"FILE SYSTEM SWITCH TABLE SIZE = %d\n",nfstypes-1);
	fprintf(fp,"SLOT   NAME     FLAGS\n");
	if(args[optind]) {
		all = 1;
		do {
			getargs(nfstypes,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg1 == 0) {
				fprintf(fp,"0 is out of range\n");
				continue;
			}
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prvfssw(all,slot,phys,addr,nfstypes);
			else {
				if(arg1 >= 0 && arg1 < nfstypes)
					slot = arg1;
				else addr = arg1;
				prvfssw(all,slot,phys,addr,nfstypes);
			}
			slot = addr = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for(slot = 1; slot < nfstypes; slot++)
		prvfssw(all,slot,phys,addr,nfstypes);
}

/* print vfs switch table */
int
prvfssw(all,slot,phys,addr,max)
int all,slot,phys,max;
long addr;
{
	struct vfssw vfsswbuf;
	char name[FSTYPSZ+1];

	if(addr == -1)
		addr = Vfssw->n_value + slot *sizeof(vfsswbuf);
	else
		slot = getslot(addr,(long)Vfssw->n_value,sizeof vfsswbuf,phys,max);
	readmem(addr,!phys,-1,
		(char *)&vfsswbuf,sizeof vfsswbuf,"file system switch table");
	if(!vfsswbuf.vsw_name && !all)
		return; 
	if(slot == -1)
		fprintf(fp,"  - ");
	else
		fprintf(fp, "%4d", slot);
	readmem((long)vfsswbuf.vsw_name,1,-1,name,sizeof name,"fs_name");
	fprintf(fp,"   %-10s", name); 
	fprintf(fp," %x\n",
		vfsswbuf.vsw_flag);
}

char * 
getfsname(slot)
int slot;
{

	struct vfssw vfsswbuf;
	static char name[FSTYPSZ+1];
	int nfstypes;

	if(!Vfssw)
		if((Vfssw = symsrch("vfssw")) == NULL)
			return(NULL);
	if(!Nfstype)
		if((Nfstype = symsrch("nfstype")) == NULL)
			return(NULL);
	readmem((long)Nfstype->n_value,1,-1,(char *)&nfstypes,
		sizeof (int), "number of file systems types");

	if(slot < 0 || slot >= nfstypes)
		return(NULL);
	readmem((long)(Vfssw->n_value+slot*sizeof vfsswbuf),1,-1,
		(char *)&vfsswbuf,sizeof vfsswbuf,"file system switch table");
	if(!vfsswbuf.vsw_name)
		return(NULL); 
	readmem((long)vfsswbuf.vsw_name,1,-1,name,sizeof name,"fs_name");
	return(name);
}

char *
vnotofsname(vnop)
vnode_t *vnop;
{
	vfs_t vfsbuf;
	extern struct syment *Spec_vnodeops, *Fifo_vnodeops, *Prvnodeops;
		/* Handle special cases */
		if(vnop->v_op == (struct vnodeops *)Spec_vnodeops->n_value){
			return("spec");;
		}
		else if(vnop->v_op == (struct vnodeops *)Fifo_vnodeops->n_value){
			return("fifo");
		}
		else if(vnop->v_op == (struct vnodeops *)Prvnodeops->n_value){
			return("proc");
		}

		/* Handle s5,ufs,sfs,vxfs */
		readmem(vnop->v_vfsp,1,-1,&vfsbuf,sizeof(vfsbuf),"vfs structure");
		return(getfsname(vfsbuf.vfs_fstype));
}
