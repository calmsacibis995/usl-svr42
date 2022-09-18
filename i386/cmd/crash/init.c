/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/init.c	1.1.1.7"
#ident "$Header: init.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash initialization.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <memory.h>
#include <sys/fs/s5dir.h>
#include <sys/user.h>
#include <sys/immu.h>
#include <sys/var.h>
#include <sys/sysi86.h>
#include "crash.h"

int	nmlst_tstamp ,		/* timestamps for namelist and cdumpfiles */
	dmp_tstamp ;

#ifdef __STDC__
struct syment dummyent = { (long)0, (long)0, (long)0, (short)0,
				(unsigned short)0, (char)0, (char)0 };
#else
struct syment dummyent;
#endif

extern char *strtbl ;		/* pointer to string table in symtab.c */
extern char *dumpfile;		
extern char *namelist;

extern int active;		/* flag for active system */
extern long vtop();
extern long lseek();
extern char *malloc();
struct syment *sp;		/* pointer to symbol table */
struct user *ubp;		/* pointer to ublock buffer */
extern struct syment *File, *Vnode, *Vfs, *V,
	*Panic, *Curproc;	/* namelist symbol pointers */
struct syment *S_s5fshead, *S_sfs_fshead, *S_vxfs_fshead;
struct syment *Ninode, *Ifreelist, *Snode, *Sndd, *Rcvd, *Nrcvd, *Ngrps;
struct syment *S_s5vnodeops,  *Spec_vnodeops, *Fifo_vnodeops, *Prvnodeops, *S_sfs_vnodeops, *S_vx_vnodeops;

/* initialize buffers, symbols, and global variables for crash session */
int
init(rwflag)
int rwflag;
{
	int offset ;
	struct syment	*ts_symb = NULL;
	extern void sigint();
	
	/* open dump file, if error print */
	if((mem = open(dumpfile, rwflag)) < 0)
		fatal("cannot open dump file %s\n",dumpfile);
	/*
	 * Set a flag if the dumpfile is of an active system.
	 */
	if(strcmp(dumpfile,"/dev/mem") == 0)
		active = 1;

#ifndef __STDC__
	memset(&dummyent,0,sizeof(dummyent));
#endif
	rdsymtab();			/* open and read the symbol table */

	if(!(V = symsrch("v")))
		fatal("var structure not found in symbol table\n");
	if(!(File = symsrch("file")))
		fatal("file not found in symbol table\n");
	if(!(Vfs = symsrch("rootvfs")))
		fatal("vfs not found in symbol table\n");
	if(!(Panic = symsrch("panicstr")))
		fatal("panicstr not found in symbol table\n");
	if(!(Curproc = symsrch("curproc")))
		fatal("curproc not found in symbol table\n");

	if(!(Sndd = symsrch("sndd")))
		Sndd = &dummyent;
	if(!(Snode = symsrch("spectable")))
		error("snode table not found\n");

	if(!(Spec_vnodeops = symsrch("spec_vnodeops")))
		error("spec_vnodeops not found\n");
	if(!(Fifo_vnodeops = symsrch("fifo_vnodeops")))
		error("fifo_vnodeops not found\n");
	if(!(Prvnodeops = symsrch("prvnodeops")))
		Prvnodeops = &dummyent;
	if(!(Ngrps = symsrch("ngroups_max")))
		error("ngroups_max not found in symbol table\n");

	readmem((long)V->n_value,1,-1,(char *)&vbuf,
		sizeof vbuf,"var structure");

	/* Allocate ublock buffer */
	ubp = (user_t*)malloc((unsigned)(USIZE*NBPC));

	Procslot = getcurproc();
	/* setup break signal handling */
	if(signal(SIGINT,sigint) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
}

