/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/vxfs_inode.c	1.1"
#ident	"$Header: vxfs_inode.c 1.1 91/07/23 $"

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
#include <sys/fs/vx_fs.h>
#include <sys/fs/vx_inode.h>
#include <sys/privilege.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <sys/fsinode.h>
#include <stdlib.h>

#include "crash.h"

#define AREAD	0x4				/* read permission */
#define AWRITE	0x2				/* write permission */
#define AEXEC	0x1				/* exec permission */

extern struct syment	*Vfs, *File;		/* namelist symbol pointers */
extern struct syment *S_vxfs_fshead;
extern struct syment *S_vx_vnodeops;

struct inode		vxfs_ibuf;		/* buffer for vxfs inode */
struct inode		**vxfs_iptrs;
struct vnode		vnode;
struct fshead		vxfs_fshead;
long			vxfs_ninode;		/* size of inode table */

void			vxflagprt();

struct iflag	{
	char	*name;
	int	value;
};

/*
 * flags in i_flag
 */

struct iflag	iflags[] = {
	"ILOCKED",		ILOCKED,
	"IUPD",			IUPD,
	"IACC",			IACC,
	"IMOD",			IMOD,
	"IWANT",		IWANT,
	"ICHG",			ICHG,
	"IATIMEMOD",		IATIMEMOD,
	"IMTIMEMOD",		IMTIMEMOD,
	"IADDRVALID",		IADDRVALID,
	"IWRITEI",		IWRITEI,
	"IREADI",		IREADI,
	"ILOG",			ILOG,
	"IRWLOCKED",		IRWLOCKED,
	"IINACTIVE",		IINACTIVE,
	"IBAD",			IBAD,
	"IUEREAD",		IUEREAD,
	"ITRANLAZYMOD",		ITRANLAZYMOD,
	"IGLOCKED",		IGLOCKED,
	"INOBMAPCACHE",		INOBMAPCACHE,
	"INOPUTPAGE",		INOPUTPAGE,
	"IFLUSHPAGES",		IFLUSHPAGES,
	"IPAGESCREATED",	IPAGESCREATED,
	"IDIRTYPAGES",		IDIRTYPAGES,
	"ICLOSED",		ICLOSED,
	"IFLUSHED",		IFLUSHED,
	"ISHORTENED",		ISHORTENED,
	"ISYNCWRITES",		ISYNCWRITES,
	"IBADUPD",		IBADUPD,
	"IDELSETATTR",		IDELSETATTR,
	0,			0
};

/*
 * flags in i_intrflag
 */

struct iflag intrflags[] = {
	"IDELXWRI",		IDELXWRI,
	"IDELXWRIERR",		IDELXWRIERR,
	"ILOGWRITE",		ILOGWRITE,
	"ILOGWRIERR",		ILOGWRIERR,
	"IDELBUF",		IDELBUF,
	"IDELBUFERR",		IDELBUFERR,
	"IPUTERROR",		IPUTERROR,
	"ILOGWRIFLUSH",		ILOGWRIFLUSH,
	0,			0
};

/*
 * Get arguments for VxFS inode.
 */
int
get_vxfs_inode ()
{
	int	slot = -1;
	int	full = 0;
	int	list = 0;
	int	all = 0;
	int	phys = 0;
	long	addr = -1;
	long	arg1 = -1;
	long	arg2 = -1;
	int	lfree = 0;
	long	next;
	int	c;
	int	i;
	char	*heading1 = 
	    "SLOT  MAJ/MIN    INUMB  RCNT  LINK    UID    GID     SIZE   TYPE  MODE\n";
	if(!Vfs)
		if(!(Vfs = symsrch("rootvfs")))
			error("vfs list not found in symbol table\n");

	if(!File)
		if(!(File = symsrch("file")))
			error("file table not found in symbol table\n");

	GETDSYM(vxfs_fshead,B_TRUE);
	GETDSYM(vx_vnodeops,B_TRUE);

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

		case 'r':	lfree = 1;
				break;

		case 'w':	redirect();
				break;

		default:	longjmp(syn, 0);
		}
	}

	readmem(S_vxfs_fshead->n_value,1,-1,(char *)&vxfs_fshead, 
		sizeof vxfs_fshead, "vxfs inode table head");
	vxfs_ninode = vxfs_fshead.f_max;
	vxfs_iptrs = (struct inode **)malloc(sizeof (struct inode *) * vxfs_ninode);
	if (vxfs_iptrs == NULL) {
		fprintf(fp, "Could not allocate space for vxfs inode pointers\n");
		return;
	}
	readmem((long)(vxfs_fshead.f_freelist),1,-1,(char *)vxfs_iptrs,
		sizeof (struct inode *) * vxfs_ninode, "vxfs inode pointers");

	if(list) {
		list_vxfs_inode(vxfs_iptrs, vxfs_fshead.f_curr);
	} else {
		fprintf(fp, "INODE TABLE SIZE = %d\n", vxfs_fshead.f_curr);
		fprintf(fp, "INODE SIZE = %d\n", vxfs_fshead.f_isize);
		if(!full)
			(void) fprintf(fp, "%s", heading1);
		if (lfree) {
			for(slot = 0; slot < vxfs_fshead.f_curr; slot++) {
				print_vxfs_inode (1, full, slot, phys, lfree,
					*(vxfs_iptrs + slot), heading1);
			}
		} else if(args[optind]) {
			all = 1;
			do {
				getargs(vxfs_fshead.f_curr,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1) {
					for(slot = arg1; slot <= arg2; slot++) {
						print_vxfs_inode(all,full,slot,
							phys,lfree,
							*(vxfs_iptrs + slot),
							heading1);
					}
				} else {
					if(arg1 >=0 && arg1 < vxfs_fshead.f_curr) {
						slot = arg1;
					} else {
						addr = arg1;
						slot = getvxfs_ipos(addr, vxfs_iptrs, vxfs_fshead.f_curr);
					}
					print_vxfs_inode(all,full,slot,
						phys,lfree,
						*(vxfs_iptrs + slot),
						heading1);
				}
				slot = addr = arg1 = arg2 = -1;
			} while(args[++optind]);
		} else {
			for(slot = 0; slot < vxfs_fshead.f_curr; slot++)
				print_vxfs_inode (all, full, slot, phys, lfree,
					*(vxfs_iptrs + slot), heading1);
		}
	}
	free(vxfs_iptrs);
}

int
list_vxfs_inode(vxfs_iptrs, vxfs_ninode)
	struct inode	**vxfs_iptrs;
	int		vxfs_ninode;
{
	int		i, j;
	struct inode	*addr;
	int		slot;

	fprintf(fp, "The following vxfs inodes are in use:\n");
	for (j = 0, slot = 0; slot < vxfs_ninode; slot++) {
		addr = *(vxfs_iptrs + slot);
		readmem((caddr_t)addr,1,-1,(char *)&vxfs_ibuf,sizeof vxfs_ibuf,
			"vxfs inode");
		if (vxfs_ibuf.av_forw == NULL) {
			if (j && (j % 5) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%4d: %8x  ", slot, addr);
			j++;
		}
	}
	fprintf(fp, "\n\nThe following vxfs inodes are on the freelist:\n");
	for (j = 0, slot = 0; slot < vxfs_ninode; slot++) {
		addr = *(vxfs_iptrs + slot);
		readmem((caddr_t)addr,1,-1,(char *)&vxfs_ibuf,sizeof vxfs_ibuf,
			"vxfs inode");
		if (vxfs_ibuf.av_forw != NULL) {
			if (j && (j % 5) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%4d: %8x  ", slot, addr);
			j++;
		}
	}
	fprintf(fp, "\n");
}

/*
 * Print vxfs inode table.
 */

int
print_vxfs_inode (all, full, slot, phys, free, addr, heading1)
	int	all, full, slot, phys, free;
	long	addr;
	char	*heading1;
{
	struct vnode 	*vp = &vxfs_ibuf.i_vnode;
	char		extbuf[50];
	char		ch;
	char		typechar;
	int		i;
	extern long	lseek();

	if (addr == -1)
		return;
	readmem(addr, !phys, -1, (char *)&vxfs_ibuf,sizeof (struct inode),"vxfs inode");
	if((long)vxfs_ibuf.i_vnode.v_op != S_vx_vnodeops->n_value)
		return;	/* not vxfs */
	if(!vxfs_ibuf.i_vnode.v_count && !all)
		return;
	if (free && !vxfs_ibuf.av_forw)
		return;
	if(full)
		fprintf(fp, "%s", heading1);
	if(slot == -1)
		fprintf(fp, "  - ");
	else
		fprintf(fp, "%4d", slot);

	fprintf(fp, " %4u,%-5u%7u   %3d %5d% 7d%7d %8ld",
		getemajor(vxfs_ibuf.i_dev),
		geteminor(vxfs_ibuf.i_dev),
		vxfs_ibuf.i_number,
		vxfs_ibuf.i_vnode.v_count,
		vxfs_ibuf.i_nlink,
		vxfs_ibuf.i_uid,
		vxfs_ibuf.i_gid,
		vxfs_ibuf.i_size);
	switch(vxfs_ibuf.i_vnode.v_type) {
		case VDIR: ch = 'd'; break;
		case VCHR: ch = 'c'; break;
		case VBLK: ch = 'b'; break;
		case VREG: ch = 'f'; break;
		case VLNK: ch = 'l'; break;
		case VFIFO: ch = 'p'; break;
		case VXNAM: ch = 'x'; break;
		default:    ch = '-'; break;
	}
	fprintf(fp, "   %c", ch);
	fprintf(fp, "%s%s%s",
		vxfs_ibuf.i_mode & ISUID ? "u" : "-",
		vxfs_ibuf.i_mode & ISGID ? "g" : "-",
		vxfs_ibuf.i_mode & ISVTX ? "v" : "-");
	fprintf(fp, "  %s%s%s%s%s%s%s%s%s",
		vxfs_ibuf.i_mode & IREAD         ? "r" : "-",
		vxfs_ibuf.i_mode & IWRITE        ? "w" : "-",
		vxfs_ibuf.i_mode & IEXEC         ? "x" : "-",
		vxfs_ibuf.i_mode & (IREAD >>  3) ? "r" : "-",
		vxfs_ibuf.i_mode & (IWRITE >> 3) ? "w" : "-",
		vxfs_ibuf.i_mode & (IEXEC >>  3) ? "x" : "-",
		vxfs_ibuf.i_mode & (IREAD >>  6) ? "r" : "-",
		vxfs_ibuf.i_mode & (IWRITE >> 6) ? "w" : "-",
		vxfs_ibuf.i_mode & (IEXEC >>  6) ? "x" : "-");
	fprintf(fp, "\n");

	if (!full)
		return;

	vxflagprt(&vxfs_ibuf);
	fprintf(fp,"\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp,"\t%8x",vxfs_ibuf.i_forw);
	fprintf(fp,"\t%8x",vxfs_ibuf.i_back);
	fprintf(fp,"\t%8x",vxfs_ibuf.av_forw);
	fprintf(fp,"\t%8x\n",vxfs_ibuf.av_back);

	fprintf(fp, "\t IOWNER ICOUNT GOWNER GCOUNT RWOWNER RWCOUNT \n");
	fprintf(fp, "\t  %5d  %5d", vxfs_ibuf.i_owner, vxfs_ibuf.i_count);
	fprintf(fp, "  %5d  %5d", vxfs_ibuf.i_gowner, vxfs_ibuf.i_gcount);
	fprintf(fp, "   %5d   %5d", vxfs_ibuf.i_rwowner, vxfs_ibuf.i_rwcount);

	if (vxfs_ibuf.i_orgtype == IORG_IMMED) {
		fprintf(fp, "\n    immediate\n");
	} else if (vxfs_ibuf.i_orgtype == IORG_EXT4) {
		for(i = 0; i < NDADDR_N; i++) {
			if (!(i % 3)) {
				fprintf(fp, "\n    ");
			} else {
				fprintf(fp, "  ");
			}
			sprintf(extbuf, "[%d, %d]", vxfs_ibuf.i_dext[i].ic_de,
				vxfs_ibuf.i_dext[i].ic_des);
			fprintf(fp, "e%d: %-19s", i, extbuf);
		}
		fprintf(fp, "\n    ie0: %-8d      ie1: %-8d      ies: %-8d\n",
			vxfs_ibuf.i_ie[0], vxfs_ibuf.i_ie[1],
			vxfs_ibuf.i_ies);
	}

	/* print vnode info */
	fprintf(fp, "VNODE :\n");
	fprintf(fp, "VCNT VFSMNTED   VFSP   STREAMP VTYPE   RDEV    VDATA   VFILOCKS VFLAG \n");
	cprvnode(&vxfs_ibuf.i_vnode);
	fprintf(fp, "\n");
}

struct filock *
vxfsgetifilock(addr)
unsigned long addr;
{
	struct inode ibuf;

	readmem((long)addr,1,-1,(char *)&ibuf,sizeof (struct inode), "vxfs inode ");
	if(ibuf.i_mode == 0)
		return 0;
	return(ibuf.i_vnode.v_filocks);
}

int
getvxfs_ipos(addr, vxfs_iptrs, vxfs_ninode)
	struct inode	*addr;
	struct inode	**vxfs_iptrs;
	int		vxfs_ninode;
{
	int 		slot;

	for (slot = 0; slot < vxfs_ninode; slot++) {
		if (*(vxfs_iptrs + slot) == addr) {
			return slot;
		}
	}
	return -1;
}

void
vxflagprt(ip)
	struct inode	*ip;
{
	int		i;

	fprintf(fp, "FLAGS:");
	for (i = 0; iflags[i].value; i++) {
		if (ip->i_flag & iflags[i].value)
			fprintf(fp, " %s", iflags[i].name);
	}
	for (i = 0; intrflags[i].value; i++) {
		if (ip->i_intrflag & intrflags[i].value)
			fprintf(fp, " %s", intrflags[i].name);
	}
	fprintf(fp, "\n");
	return;
}

