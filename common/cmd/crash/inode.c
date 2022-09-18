/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: inode.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  vnode, inode, file.
 */
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/rf_messg.h>
#include <sys/rf_comm.h>
#include <sys/fs/s5inode.h>
#include <sys/fs/xnamnode.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <sys/fsinode.h>
#include <stdlib.h>

#include "crash.h"

extern struct syment *Vnode, *Streams, *Vfs, *File;	/* namelist symbol pointers */

extern struct syment *Snode, *Sndd, *Rcvd, *Nrcvd, *Ngrps;
extern struct syment *S_s5fshead, *S_sfs_fshead, *S_vxfs_fshead;
extern struct syment *S_s5vnodeops, *Spec_vnodeops, *Fifo_vnodeops, *Prvnodeops, *S_sfs_vnodeops, *S_vx_vnodeops;

struct vnode vnbuf;			/* buffer for vnode */

struct inode ibuf;		/* buffer for s5inode */
struct vnode vnode;
struct fshead s5fshead;
int ninode;

struct listbuf {
	long	addr;
	char	state;
};



/* get arguments for vnode function */
int
getvnode()
{
	long addr = -1;
	int phys = 0;
	int c;


	optind = 1;
	while((c = getopt(argcnt,args,"pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
					break;
		}
	}
	if(args[optind]){
		fprintf(fp,"VCNT  VFSMNTED   VFSP   STREAMP VTYPE   RDEV    VDATA   VFILOCKS VFLAG VLID\n");
		do{
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			readmem(addr,!phys,-1,(char *)&vnbuf,sizeof vnbuf,"vnode");
			cprvnode(&vnbuf);
		}while(args[optind]);

		fprintf(fp, "\n");
	}
	else longjmp(syn,0);

}

/*
 * Crash version of prvnode(), 
 * differs from _KERNEL __STDC__ function prototype
 * in <fs/procfs/prdata.h>
 */
cprvnode(vnptr)
struct vnode *vnptr;
{
	

	fprintf(fp,"%3d %8x %8x %8x",
		vnptr->v_count,
		vnptr->v_vfsmountedhere,
		vnptr->v_vfsp,
		vnptr->v_stream);
	switch(vnptr->v_type){
		case VREG :	fprintf(fp, "  f       -   "); break;
		case VDIR :	fprintf(fp, "  d       -   "); break;
		case VLNK :	fprintf(fp, "  l       -   "); break;
		case VCHR :
				fprintf(fp,"   c     %4u,%-5u",
					getemajor(vnptr->v_rdev),
					geteminor(vnptr->v_rdev));
				break;
		case VBLK :
				fprintf(fp,"  b  %4u,%-5u",
					getemajor(vnptr->v_rdev),
					geteminor(vnptr->v_rdev));
				break;
		case VFIFO :	fprintf(fp, "  p       -   "); break;
		case VNON :	fprintf(fp, "  n       -   "); break;
		default :	fprintf(fp, "  -       -   "); break;
	}
	fprintf(fp,"  %8x %8x",
		vnptr->v_data,
		vnptr->v_filocks);
	fprintf(fp,"%s",
		vnptr->v_flag & VROOT ? " root" : "     ");
	fprintf(fp," %4x\n",
		vnptr->v_lid);
		
}


/* get arguments for S5 inode function */
int
getinode()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int free = 0;
	long next;
	int list = 0;
	int i;
	struct ipool freelist;
	struct inode freebuf;
	struct idata *fsidata;
	struct idata s5idata;
	struct listbuf *listptr;
	struct listbuf *listbuf;
	int c;
	char *heading = 
	    "SLOT   MAJ/MIN  INUMB RCNT  LINK     UID     GID     SIZE     MODE    FLAGS\n";

	GETDSYM(s5fshead,B_TRUE);
	GETDSYM(s5vnodeops,B_TRUE);

	optind = 1;
	while((c = getopt(argcnt,args,"efprlw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'r' :	free = 1;
					break;
			case 'l' :	list = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	readmem(S_s5fshead->n_value,1,-1,(char *)&s5fshead, 
		sizeof s5fshead, "S5 inode table head");
	ninode = s5fshead.f_curr;

	listbuf = listptr = (struct listbuf *)malloc(sizeof(struct listbuf)*ninode);
	if (listbuf == NULL) {
		fprintf(fp,"Could not allocate space for S5 inode buffer\n" );
		return;
	}

	fsidata = (struct idata *)
	    (S_s5fshead->n_value + ((int)&s5fshead.f_idata - (int)&s5fshead));

	/* Structure Copy */
	s5idata = s5fshead.f_idata;

	/* Get the inode addresses so that they could be referenced directly */
	while(s5idata.id_next != fsidata) {
		addr = (long)s5idata.id_next;
		readmem((long)addr,1,-1,(char *)&s5idata, sizeof s5idata,
		    "S5 inode idata");
		addr += sizeof s5idata;
		for(i = 0; i < s5idata.id_total; i++, listptr++) {
			listptr->addr = addr;
			listptr->state = 'n';	/* unknown state */
			readmem((long)addr,1,-1, (char *)&ibuf,sizeof ibuf,
			    "S5 inode table");
			addr += s5fshead.f_isize; 
			if(ibuf.i_vp) {		/* determine type */
				readmem((long)(ibuf.i_vp),1,-1,(char *)&vnode,
					sizeof vnode,"vnode entry");
				if( (long)vnode.v_op != S_s5vnodeops->n_value ) {
					listptr->state = 'x';	/* not s5 */
					continue;
				}
			}
			else {
				continue;
			}
			if(ibuf.i_vnode.v_count != 0)
				listptr->state = 'u';		/* in use */
		}
	}

	if(list)
		listinode(listbuf);
	else {
		fprintf(fp,"INODE TABLE SIZE = %d\n", ninode);
		fprintf(fp,"INODE SIZE = %d\n", s5fshead.f_isize);
		if(!full)
			fprintf(fp,"%s",heading);
		if(free) {
			readmem((long)s5fshead.f_freelist,1,-1,(char *)&freelist,
			    sizeof freelist,"S5 freelist buffer");

			next = (long)freelist.i_ff;
			while(next) {
				prinode(1,full,slot,phys,next,heading);
				next = (long)ibuf.av_forw;
				if(next == (long)s5fshead.f_freelist)
					next = 0;
			}
		} else if(args[optind]) {
			all = 1;
			do {
				getargs(ninode,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1)
					for(slot = arg1; slot <= arg2; slot++) {
						addr = listbuf[slot].addr;
						prinode(all,full,slot,phys,addr,heading);
				} else {
					if(arg1 >= 0 && arg1 < ninode) {
						slot = arg1;
						addr = listbuf[slot].addr;
					} else {
						addr = arg1;
						slot = gets5_ipos(addr, listbuf, ninode);
					}
					prinode(all,full,slot,phys,addr,heading);
				}
				slot = addr = arg1 = arg2 = -1;
			}while(args[++optind]);
		} else {
			listptr = listbuf;
			for(slot = 0; slot < ninode; slot++, listptr++)
				prinode(all,full,slot,phys,listptr->addr,heading);
		}
	}

	if (listbuf != NULL) {
		s5free((void *)listbuf);
		listbuf = listptr = NULL;
	}
}

s5free(ptr)
void *ptr;
{
	free(ptr);
}

int
listinode(listbuf)
struct listbuf listbuf[];
{
	struct listbuf *listptr;
	int i,j;
	long next;
	struct inode freebuf;
	struct ipool freelist;

	if (listbuf == NULL)
		return;


	readmem((long)s5fshead.f_freelist,1,-1,(char *)&freelist,
		sizeof freelist,"ifreelist buffer");
	next = (long)freelist.i_ff;
	while(next) {
		i = gets5_ipos((long)next,listbuf,ninode);
		readmem((long)next,1,-1,(char *)&ibuf,sizeof ibuf,"S5 inode");
		if( listbuf[i].state == 'u' ) 
			listbuf[i].state = 'b';
		else
			if( listbuf[i].state  != 'x' )
				listbuf[i].state = 'f';
		next = (long)ibuf.av_forw;
		if(next == (long)s5fshead.f_freelist)
			next = 0;
	}
	fprintf(fp,"The following S5 inodes are in use:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'u') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following S5 inodes are on the freelist:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'f') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following S5 inodes are on the freelist but have non-zero reference counts:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'b') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}

	fprintf(fp,"\n\nThe following S5 inodes are in unknown states:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'n') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n");
}


/* print inode table */
int
prinode(all,full,slot,phys,addr,heading)
int all,full,slot,phys;
long addr;
char *heading;
{
	char ch;
	int i;
	extern long lseek();


	if (addr == -1)
		return;

	readmem(addr, !phys,-1,(char *)&ibuf,sizeof ibuf,"inode table");

	if(!ibuf.i_vp) 
		return;

	if(!ibuf.i_vnode.v_count && !all) 
			return ;

	readmem((long)(ibuf.i_vp),1,-1,(char *)&vnode,
		sizeof vnode,"vnode entry");

	if( (long)vnode.v_op != S_s5vnodeops->n_value )
		return;	/* not s5 */

	if(full)
		fprintf(fp,"%s",heading);

	if(slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);

	fprintf(fp," %4u,%-5u %5u  %3d %5d %7d %7d %8ld",
		getemajor(ibuf.i_dev),
		geteminor(ibuf.i_dev),
		ibuf.i_number,
		ibuf.i_vnode.v_count,
		ibuf.i_nlink,
		ibuf.i_uid,
		ibuf.i_gid,
		ibuf.i_size);
	switch(ibuf.i_vnode.v_type) {
		case VDIR: ch = 'd'; break;
		case VCHR: ch = 'c'; break;
		case VBLK: ch = 'b'; break;
		case VREG: ch = 'f'; break;
		case VLNK: ch = 'l'; break;
		case VFIFO: ch = 'p'; break;
		case VXNAM:
                        switch(ibuf.i_rdev) {
                                case XNAM_SEM: ch = 's'; break;
                                case XNAM_SD: ch = 'm'; break;
                                default: ch = '-'; break;
                        };
                        break;
		default:    ch = '-'; break;
	}
	fprintf(fp,"  %c",ch);
	fprintf(fp,"%s%s%s%03o",
		ibuf.i_mode & ISUID ? "u" : "-",
		ibuf.i_mode & ISGID ? "g" : "-",
		ibuf.i_mode & ISVTX ? "v" : "-",
		ibuf.i_mode & 0777);

	fprintf(fp,"%s%s%s%s%s%s%s\n",
		ibuf.i_flag & ILOCKED ? " lk" : "",
		ibuf.i_flag & IUPD ? " up" : "",
		ibuf.i_flag & IACC ? " ac" : "",
		ibuf.i_flag & IWANT ? " wt" : "",
		ibuf.i_flag & ICHG ? " ch" : "",
		ibuf.i_flag & ISYN ? " sy" : "",
		ibuf.i_flag & IMOD ? " md" : "");
	if(!full)
		return;
	fprintf(fp,"\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp,"\t%8x",ibuf.i_forw);
	fprintf(fp,"\t%8x",ibuf.i_back);
	fprintf(fp,"\t%8x",ibuf.av_forw);
	fprintf(fp,"\t%8x\n",ibuf.av_back);

	fprintf(fp,"\t   NEXTR\n");
	fprintf(fp,"\t%8x",
		ibuf.i_nextr);

	if((ibuf.i_vnode.v_type == VDIR) || (ibuf.i_vnode.v_type == VREG)
		|| (ibuf.i_vnode.v_type == VLNK)) {
		for(i = 0; i < NADDR; i++) {
			if(!(i & 3))
				fprintf(fp,"\n\t");
			fprintf(fp,"[%2d]: %-10x",i,ibuf.i_addr[i]);
		}
		fprintf(fp,"\n");
	}
	else
		fprintf(fp,"\n");

	/* print vnode info */
	fprintf(fp,"\nVNODE :\n");
	fprintf(fp,"VCNT VFSMNTED   VFSP   STREAMP VTYPE   RDEV    VDATA   VFILOCKS VFLAG     \n");
	cprvnode(&ibuf.i_vnode);
	fprintf(fp,"\n");
}

/* get arguments for file function */
int
getfile()
{
	int all = 0;
	int full = 0;
	int phys = 0;
	int c;
	long filep;
	char *heading = "ADDRESS  RCNT    TYPE/ADDR       OFFSET   FLAGS\n";
	long prfile();

	optind = 1;
	while((c = getopt(argcnt,args,"epfw:")) !=EOF) {
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
	GETDSYM(s5vnodeops,B_FALSE);
	GETDSYM(sfs_vnodeops,B_FALSE);
	GETDSYM(vx_vnodeops,B_FALSE);

	if(!full)
		fprintf(fp,"%s", heading);
	if(args[optind]) {
		all = 1;
		do {
			filep = strcon(args[optind],'h');
			if(filep == -1) 
				continue;
			else
				(void) prfile(all,full,phys,filep,heading);
			filep = -1;
		} while(args[++optind]);
	} else {
		readmem(File->n_value, 1, -1, (char *)&filep, sizeof filep,
		    "file table");
		while (filep)
			filep = prfile(all,full,phys,filep,heading);
	}
}


/* print file table */
long
prfile(all,full,phys,addr,heading)
int all,full,phys;
long addr;
char *heading;
{
	struct file fbuf;
	struct cred *credbufp;
	int fileslot;
	int ngrpbuf;
	short i;
	char fstyp[5];
	struct vnode vno;
	extern int ufsvno();

	readmem(addr,!phys,-1,(char *)&fbuf,sizeof fbuf,"file table");
	if(!fbuf.f_count && !all)
		return(0);
	if(full)
		fprintf(fp,"\n%s", heading);
	fprintf(fp,"%.8x", addr);
	fprintf(fp," %3d", fbuf.f_count);


	if(fbuf.f_count && fbuf.f_vnode != 0){
		char *tname;
		/* read in vnode */
		readmem(((long)fbuf.f_vnode),1,-1,(char *)&vno,sizeof vno,"vnode");

		tname = vnotofsname(&vno);
		if(tname == NULL)
			strcpy(fstyp, " ?  ");
		else {
			int i;
			strncpy(fstyp,tname,4);
			for(i = strlen(tname); i < 4; i++)
				fstyp[i] = ' ';
			fstyp[5] = '\0';
		}

	} else
		strcpy(fstyp, " ?  ");
	fprintf(fp,"    %s/%8x",fstyp,fbuf.f_vnode);
	fprintf(fp," %8x",fbuf.f_offset);
	fprintf(fp,"  %s%s%s%s%s%s%s%s\n",
		fbuf.f_flag & FREAD ? " read" : "",
		fbuf.f_flag & FWRITE ? " write" : "",  /* print the file flag */
		fbuf.f_flag & FAPPEND ? " appen" : "",
		fbuf.f_flag & FSYNC ? " sync" : "",
		fbuf.f_flag & FCREAT ? " creat" : "",
		fbuf.f_flag & FTRUNC ? " trunc" : "",
		fbuf.f_flag & FEXCL ? " excl" : "",
		fbuf.f_flag & FNDELAY ? " ndelay" : "");

	if(!full)
		return((long)fbuf.f_next);

	/* user credentials */
	if(!Ngrps)
		if(!(Ngrps = symsrch("ngroups_max")))
			error("ngroups_max not found in symbol table\n");
	readmem((long)Ngrps->n_value, 1, -1, (char *)&ngrpbuf,
		sizeof ngrpbuf, "max groups");

	credbufp=(struct cred *)malloc(sizeof(struct cred) + sizeof(uid_t) * (ngrpbuf-1));
	readmem((long)fbuf.f_cred,1,-1,(char *)credbufp,sizeof (struct cred) + sizeof(uid_t) * (ngrpbuf-1),"user cred");
	fprintf(fp,"User Credential : \n");
	fprintf(fp,"rcnt:%3d, uid:%-10d, gid:%-10d, ruid:%-10d, rgid:%-10d, ngroup:%4d",
		credbufp->cr_ref,
		credbufp->cr_uid,
		credbufp->cr_gid,
		credbufp->cr_ruid,
		credbufp->cr_rgid,
		credbufp->cr_ngroups);
	for(i=0; i < (short)credbufp->cr_ngroups; i++){
		if(!(i % 4))
			fprintf(fp, "\n");
		fprintf(fp,"group[%d]:%4d ", i, credbufp->cr_groups[i]);
	}
	fprintf(fp, "\n");

	/* Asyncio links */

	fprintf(fp, "Asyncio Links: \n");
	fprintf(fp, "Forward: %x  Backward: %x\n", fbuf.f_aiof, fbuf.f_aiob);
	return((long)fbuf.f_next);
}


gets5_ipos(addr, list, max)
long	addr;
struct listbuf *list;
int	max;
{

	int	i;
	int	pos;
	struct listbuf *listptr;

	listptr = list;
	pos = -1;
	for(i = 0; i < max; i++, listptr++) {
		if (listptr->addr == addr) {
			pos = i;
			break;
		}
	}
	return(pos);
}

struct filock *
s5getifilock(addr)
unsigned long addr;
{
	struct inode ibuf;
			readmem((long)addr,1,-1, (char *)&ibuf,sizeof ibuf,
			    "S5 inode ");

			if(ibuf.i_mode == 0)
				return 0;
	
			if(!ibuf.i_vp) 
				return 0;
			return(ibuf.i_vnode.v_filocks);
}

