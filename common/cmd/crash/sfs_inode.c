/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/sfs_inode.c	1.5.1.6"
#ident	"$Header: sfs_inode.c 1.1 91/07/23 $"

#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/privilege.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <sys/fsinode.h>
#include <stdlib.h>

#include "crash.h"

#define AREAD	0x4					/* read permission */
#define AWRITE	0x2					/* write permission */
#define AEXEC	0x1					/* exec permission */

#define SEC	sec.is_secdata

extern struct syment	*Vfs, *File;			/* namelist symbol pointers */
extern struct syment *S_sfs_fshead;
extern struct syment *S_sfs_vnodeops;

struct inode		sfs_ibuf;			/* buffer for SFS inode */
struct vnode		vnode;
struct fshead		sfs_fshead;
long			sfs_ninode;			/* size of inode table */

struct listbuf {
	long	addr;
	char	state;
};

/*
 * Get arguments for SFS inode.
 */
int
get_sfs_inode ()
{
	int	slot = -1;
	int	full = 0;
	int	list = 0;
	int	all = 0;
	int	phys = 0;
	long	addr = -1;
	long	arg1 = -1;
	long	arg2 = -1;
	int	free = 0;
	long	next;
	int	c;
	int	i;
	struct ipool	freelist;
	struct inode	sfs_freebuf;
	struct idata	*fsidata;
	struct idata	sfs_idata;
	struct listbuf	*listptr;
	struct listbuf	*listbuf;
	char	*heading1 = 
	    "SLOT  MAJ/MIN    INUMB  RCNT  LINK    UID    GID     SIZE   TYPE\n";
	char	*heading2 = 
	    "ACLS DACL  ABLK ACLENTRIES LID SFLG FLGS\n"; 

	if(!Vfs)
		if(!(Vfs = symsrch("rootvfs")))
			error("vfs list not found in symbol table\n");

	if(!File)
		if(!(File = symsrch("file")))
			error("file table not found in symbol table\n");

	GETDSYM(sfs_fshead,B_TRUE);
	GETDSYM(sfs_vnodeops,B_TRUE);

	optind = 1;
	while((c = getopt(argcnt, args, "efprlw:")) !=EOF) {
		switch(c) {

		case 'e':	all = 1;
				break;

		case 'f':	full =1;
				break;

		case 'l':	list = 1;
				break;

		case 'p':	phys = 1;
				break;

		case 'r':	free = 1;
				break;

		case 'w':	redirect();
				break;

		default:	longjmp(syn, 0);
		}
	}

	readmem(S_sfs_fshead->n_value,1,-1,(char *)&sfs_fshead, 
		sizeof sfs_fshead, "SFS/UFS inode table head");
	sfs_ninode = sfs_fshead.f_curr;

	listbuf = listptr = (struct listbuf *)malloc(sizeof(struct listbuf)*sfs_ninode);
	if (listbuf == NULL) {
		fprintf(fp, "Could not allocate space for SFS/UFS inode buffer\n");
		return;
	}

	fsidata = (struct idata *)
	    (S_sfs_fshead->n_value + ((int)&sfs_fshead.f_idata - (int)&sfs_fshead));

	/* Structure Copy */
	sfs_idata = sfs_fshead.f_idata;

	/* Get the inode addresses so that they could be referenced directly */
	while(sfs_idata.id_next != fsidata) {
		addr = (long)sfs_idata.id_next;
		readmem((long)addr,1,-1,(char *)&sfs_idata, sizeof sfs_idata,
		    "SFS/UFS inode idata");
		addr += sizeof sfs_idata;
		for(i = 0; i < sfs_idata.id_total; i++, listptr++) {
			listptr->addr = addr;
			listptr->state = 'n';	/* unknown state */
			readmem((long)addr,1,-1, (char *)&sfs_ibuf,sizeof sfs_ibuf,
			    "SFS/UFS inode table");
			addr += sfs_fshead.f_isize; 
			if(sfs_ibuf.i_vp) {		/* determine type */
				readmem((long)(sfs_ibuf.i_vp),1,-1,(char *)&vnode,
					sizeof vnode,"vnode entry");
				if( (long)vnode.v_op != S_sfs_vnodeops->n_value ) {
					listptr->state = 'x';	/* not sfs/ufs */
					continue;
				}
			}
			else {
				continue;
			}
			if(sfs_ibuf.i_vnode.v_count != 0)
				listptr->state = 'u';		/* in use */
		}
	}


	if(list)
		list_sfs_inode (listbuf);
	else {
		fprintf(fp, "INODE TABLE SIZE = %d\n", sfs_ninode);
		fprintf(fp, "INODE SIZE = %d\n", sfs_fshead.f_isize);
		if(!full)
			(void) fprintf(fp, "%s", heading1);
		if(free) {
			readmem((long)sfs_fshead.f_freelist,1,-1,(char *)&freelist,
			    sizeof freelist,"SFS/UFS freelist buffer");

			next = (long)freelist.i_ff;

			while(next) {
				print_sfs_inode(1,full,slot,phys,next,heading1, heading2);
				next = (long)sfs_ibuf.i_freef;
				if(next == (long)sfs_fshead.f_freelist)
					next = 0;
			}

		} else if(args[optind]) {
			all = 1;
			do {
				getargs(sfs_ninode,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1)
					for(slot = arg1; slot <= arg2; slot++) {
						addr = listbuf[slot].addr;
						print_sfs_inode(all,full,slot,phys,addr,
						    heading1, heading2);
				} else {
					if(arg1 >=0 && arg1 < sfs_ninode) {
						slot = arg1;
						addr = listbuf[slot].addr;
					} else {
						addr = arg1;
						slot = getsfs_ipos(addr, listbuf, sfs_ninode);
					}
					print_sfs_inode(all,full,slot,phys,addr,
					    heading1, heading2);
				}
				slot = addr = arg1 = arg2 = -1;
			}while(args[++optind]);
		} else {
			listptr = listbuf;
			for(slot = 0; slot < sfs_ninode; slot++, listptr++)
				print_sfs_inode (all, full, slot, phys, listptr->addr, 
				    heading1, heading2);
		}
	}

	if (listbuf != NULL) {
		sfsfree((void *)listbuf);
		listbuf = listptr = NULL;
	}


}

sfsfree(ptr)
void *ptr;
{
	free(ptr);
}

list_sfs_inode(listbuf)
struct listbuf listbuf[];
{
	struct listbuf	*listptr;
	int		i, j;
	long		next;
	struct ipool	freelist;
	struct inode	freebuf;

	if (listbuf == NULL)
		return;


	readmem((long)sfs_fshead.f_freelist,1,-1,(char *)&freelist,
		sizeof freelist,"SFS/UFS ifreelist buffer");
	next = (long)freelist.i_ff;
	while(next) {
		i = getsfs_ipos((long)next, listbuf, sfs_ninode);
		readmem((long)next, 1, -1, (char *)&sfs_ibuf, sizeof sfs_ibuf, "SFS inode");
		if( listbuf[i].state == 'u' )
			listbuf[i].state = 'b' ;
		else
			if( listbuf[i].state != 'x' )
				listbuf[i].state = 'f';
		next = (long)sfs_ibuf.i_freef;
		if(next == (long)sfs_fshead.f_freelist)
			next = 0;
	}

	(void) fprintf(fp, "The following SFS inodes are in use:\n");
	for(i = 0, j = 0; i < sfs_ninode; i++) {
		if(listbuf[i].state == 'u') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}

	fprintf(fp, "\n\nThe following SFS inodes are on the freelist:\n");
	for(i = 0, j=0; i < sfs_ninode; i++) {
		if(listbuf[i].state == 'f') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}

	fprintf(fp, "\n\nThe following SFS inodes are on the freelist but have non-zero reference counts:\n");
	for(i = 0, j=0; i < sfs_ninode; i++) {
		if(listbuf[i].state == 'b') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}

	fprintf(fp, "\n\nThe following SFS inodes are in unknown states:\n");
	for(i = 0, j = 0; i < sfs_ninode; i++) {
		if(listbuf[i].state == 'n') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}
	fprintf(fp, "\n");
}

/*
 * Print SFS inode table.
 */

int
print_sfs_inode (all, full, slot, phys, addr, heading1, heading2)
	int	all, full, slot, phys;
	long	addr;
	char	*heading1;
	char	*heading2;
{
	char		flags[22];
	char		prflag[3] = "  ";
	char		ch;
	char		typechar;
	int		i;
	int		defflag = 0;	/* default ACLs not found yet */
	union		i_secure sec;
	extern long	lseek();


	if (addr == -1)
		return;

	readmem(addr, !phys, -1, (char *)&sfs_ibuf,sizeof sfs_ibuf,"SFS/UFS inode");

	if(!sfs_ibuf.i_vp)
		return;

	readmem((long)(sfs_ibuf.i_vp),1,-1,(char *)&vnode,
		sizeof vnode,"vnode entry");

	if( (long)vnode.v_op != S_sfs_vnodeops->n_value )
		return;	/* not sfs */

	if( !vnode.v_count && !all)
			return;

	if(sfs_ibuf.i_secp) {
		readmem((long)(sfs_ibuf.i_secp),1,-1,(char *)&sec,
			sizeof sec,"security data");
	}
	if(full)
		fprintf(fp, "%s", heading1);

	if(slot == -1)
		fprintf(fp, "  - ");
	else
		fprintf(fp, "%4d", slot);

	fprintf(fp, " %4u,%-5u%7u   %3d %5d% 7d%7d %8ld",
		getemajor(sfs_ibuf.i_dev),
		geteminor(sfs_ibuf.i_dev),
		sfs_ibuf.i_number,
		vnode.v_count,
		sfs_ibuf.i_nlink,
		sfs_ibuf.i_uid,
		sfs_ibuf.i_gid,
		sfs_ibuf.i_size);
	switch(vnode.v_type) {
		case VDIR: ch = 'd'; break;
		case VCHR: ch = 'c'; break;
		case VBLK: ch = 'b'; break;
		case VREG: ch = 'f'; break;
		case VLNK: ch = 'l'; break;
		case VFIFO: ch = 'p'; break;
		default:    ch = '-'; break;
	}
	fprintf(fp, "   %c", ch);
	fprintf(fp, "%s%s%s\n",
		sfs_ibuf.i_mode & ISUID ? "u" : "-",
		sfs_ibuf.i_mode & ISGID ? "g" : "-",
		sfs_ibuf.i_mode & ISVTX ? "v" : "-");

	if (!full)
		return;

	if (sfs_ibuf.i_secp) {

		fprintf(fp, "%s", heading2);
		fprintf(fp, " %4d %4d %5x",
			SEC.isd_aclcnt, SEC.isd_daclcnt, SEC.isd_aclblk);

		/*
		 * print USER_OBJ ACL entry from permission bits.
		 */
		fprintf(fp, " u::%c%c%c   ", 
			(sfs_ibuf.i_mode >> 6) & AREAD ? 'r' : '-',
			(sfs_ibuf.i_mode >> 6) & AWRITE ? 'w' : '-',
			(sfs_ibuf.i_mode >> 6) & AEXEC ? 'x' : '-');
	
		fprintf(fp, "%5d %4x", SEC.isd_lid, SEC.isd_sflags);
	
		fprintf(fp, "%s%s%s%s%s%s%s%s%s%s\n",
			sfs_ibuf.i_flag & ILOCKED ? " lk" : "",
			sfs_ibuf.i_flag & IUPD ? " up" : "",
			sfs_ibuf.i_flag & IACC ? " ac" : "",
			sfs_ibuf.i_flag & IWANT ? " wt" : "",
			sfs_ibuf.i_flag & ICHG ? " ch" : "",
			sfs_ibuf.i_flag & ISYNC ? " sy" : "",
			sfs_ibuf.i_flag & ILWAIT ? " wt" : "",
			sfs_ibuf.i_flag & IREF ? " rf" : "",
			sfs_ibuf.i_flag & INOACC ? " na" : "",
			sfs_ibuf.i_flag & IMODTIME ? " mt" : "",
			sfs_ibuf.i_flag & IMOD ? " md" : "");
	
		if ((SEC.isd_aclcnt == 0) || 
			(SEC.isd_aclcnt == SEC.isd_daclcnt)) {
			/*
			 * No non-default ACL entries.  Print GROUP_OBJ entry
			 * from permission bits.
			 */
			fprintf(fp, "%69s g::%c%c%c\n", "", 
				(sfs_ibuf.i_mode >> 3) & AREAD ? 'r' : '-',
				(sfs_ibuf.i_mode >> 3) & AWRITE ? 'w' : '-',
				(sfs_ibuf.i_mode >> 3) & AEXEC ? 'x' : '-');
		} 
		if (SEC.isd_aclcnt > 0) {
			for (i = 0; (i < SEC.isd_aclcnt) && (i < NACLI); i++) {
				if (SEC.isd_acl[i].a_type & ACL_DEFAULT) {
					if (defflag == 0) {
						/*
						 * 1st default ACL entry.  Print
						 * CLASS_OBJ & OTHER_OBJ entries 
						 * from permission bits before 
						 * default entry.
						 */
						fprintf(fp, "%69s c:%c%c%c\n", "", 
						(sfs_ibuf.i_mode >> 3) & AREAD 
							? 'r' : '-',
						(sfs_ibuf.i_mode >> 3) & AWRITE 
							? 'w' : '-',
						(sfs_ibuf.i_mode >> 3) & AEXEC 
							? 'x' : '-');
						fprintf(fp, "%69s o:%c%c%c\n", "", 
						sfs_ibuf.i_mode & AREAD ? 'r' : '-',
						sfs_ibuf.i_mode & AWRITE ? 'w' : '-',
						sfs_ibuf.i_mode & AEXEC ? 'x' : '-');
						defflag++;
					}
				}
				/* print each ACL entry stored in inode */
				fprintf(fp, "%69s %s", "",
				SEC.isd_acl[i].a_type & ACL_DEFAULT ? "d:" : "");
				switch (SEC.isd_acl[i].a_type & ~ACL_DEFAULT) {
				case GROUP:
				case GROUP_OBJ:
					typechar = 'g';
					break;
				case USER:
				case USER_OBJ:
					typechar = 'u';
					break;
				case CLASS_OBJ:
					typechar = 'c';
					break;
				case OTHER_OBJ:
					typechar = 'o';
					break;
				default:
					typechar = '?';
					break;
				}	/* end switch */
				if ((SEC.isd_acl[i].a_type & GROUP) ||
				    (SEC.isd_acl[i].a_type & USER)) 
					fprintf(fp, "%c:%d:%c%c%c\n",
					typechar,
					SEC.isd_acl[i].a_id,
					SEC.isd_acl[i].a_perm & AREAD ? 'r' : '-',
					SEC.isd_acl[i].a_perm & AWRITE ? 'w' : '-',
					SEC.isd_acl[i].a_perm & AEXEC ? 'x' : '-');
				else if ((SEC.isd_acl[i].a_type & USER_OBJ) ||
					(SEC.isd_acl[i].a_type & GROUP_OBJ))
					fprintf(fp, "%c::%c%c%c\n", 
					typechar,
					SEC.isd_acl[i].a_perm & AREAD ? 'r' : '-',
					SEC.isd_acl[i].a_perm & AWRITE ? 'w' : '-',
					SEC.isd_acl[i].a_perm & AEXEC ? 'x' : '-');
				else
					fprintf(fp, "%c:%c%c%c\n", 
					typechar,
					SEC.isd_acl[i].a_perm & AREAD ? 'r' : '-',
					SEC.isd_acl[i].a_perm & AWRITE ? 'w' : '-',
					SEC.isd_acl[i].a_perm & AEXEC ? 'x' : '-');
			}	/* end for */
		}	/* end if */
		if (defflag == 0) {
			/*
			 * No default ACL entries.  Print CLASS_OBJ & 
			 * OTHER_OBJ entries from permission bits now.
			 */
			fprintf(fp, "%69s c:%c%c%c\n", "", 
				(sfs_ibuf.i_mode >> 3) & AREAD ? 'r' : '-',
				(sfs_ibuf.i_mode >> 3) & AWRITE ? 'w' : '-',
				(sfs_ibuf.i_mode >> 3) & AEXEC ? 'x' : '-');
			fprintf(fp, "%69s o:%c%c%c\n", "", 
				sfs_ibuf.i_mode & AREAD ? 'r' : '-',
				sfs_ibuf.i_mode & AWRITE ? 'w' : '-',
				sfs_ibuf.i_mode & AEXEC ? 'x' : '-');
		}
	}


	fprintf(fp,"\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp,"\t%8x",sfs_ibuf.i_forw);
	fprintf(fp,"\t%8x",sfs_ibuf.i_back);
	fprintf(fp,"\t%8x",sfs_ibuf.i_freef);
	fprintf(fp,"\t%8x\n",sfs_ibuf.i_freeb);

	fprintf(fp, "\t  OWNER COUNT NEXTR \n");
	fprintf(fp, "\t%4d", sfs_ibuf.i_owner);
	fprintf(fp, " %4d", sfs_ibuf.i_count);
	fprintf(fp, " %8x\n", sfs_ibuf.i_nextr);

	if((vnode.v_type == VDIR) || (vnode.v_type == VREG)
		|| (vnode.v_type == VLNK)) {
		for(i = 0; i < NADDR; i++) {
			if(!(i & 3))
				fprintf(fp, "\n\t");
			fprintf(fp, "[%2d]: %-10x", i, sfs_ibuf.i_db[i]);
		}
		fprintf(fp, "\n");
	} else
		fprintf(fp, "\n");

	/* print vnode info */
	fprintf(fp, "\nVNODE :\n");
	fprintf(fp, "VCNT VFSMNTED   VFSP   STREAMP VTYPE   RDEV    VDATA   VFILOCKS VFLAG \n");
	cprvnode(&vnode);
	fprintf(fp, "\n");
}


getsfs_ipos(addr, list, max)
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
sfsgetifilock(addr)
unsigned long addr;
{
	struct inode ibuf;
			readmem((long)addr,1,-1, (char *)&ibuf,sizeof ibuf,
			    "sfs/ufs inode ");

			if(ibuf.i_mode == 0)
				return 0;
	
			if(!ibuf.i_vp) 
				return 0;
			return(ibuf.i_vnode.v_filocks);
}
