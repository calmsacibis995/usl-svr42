/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/fpriv.c	1.2.3.2"
#ident	"$Header: fpriv.c 1.1 91/07/23 $"


/*
 * This file contains code for the crash function: filepriv, fprv.
*/

#include	<syms.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<priv.h>
#include	<sys/secsys.h>
#include	"crash.h"

struct	syment	*Privf;

/* get arguments for filepriv function */
int
getfpriv()
{
	long	addr;
	int symbolic = 0;
	int c;
	void	prprvtbl();

	if(!Privf)
		if (!(Privf = symsrch("pm_lpktab")))
			error("kernel privilege table not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"nw:")) !=EOF) {
		switch(c) {
			case 'n' :	symbolic = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	/*
	 * Get the address of the first mounted device entry.
	*/
	readmem(Privf->n_value, 1, -1, (char *)&addr, sizeof(addr),
		"address of kernel privilege table");

	prprvtbl(addr, symbolic);
}


/* print kernel privilege table */
void
prprvtbl(daddr, symb)
long	daddr;
int	symb;
{
	lpktab_t	devtbl;
	lpdtab_t	fstbl;
	lpftab_t	flist;
	long		fsaddr,
			laddr;
	void		pr_privs();

	while (daddr) {			/* list of mounted devices loop */
		readmem(daddr, 1, -1, (char *)&devtbl, sizeof(devtbl),
			"kernel privilege table (mounted devices)");
		/*
		 * set ``fsaddr'' to the address of the file system list.
		*/
		fsaddr = (long)devtbl.lpk_list;
		/*
		 * if there's another device in the list then
		 * set ``daddr'' to it.  Otherwise, it's at the
		 * end of the devices list.
		*/
		if (devtbl.lpk_next)
			daddr = (long)devtbl.lpk_next;
		else
			daddr = 0;
		while (fsaddr) {	/* list of file systems loop */
			readmem(fsaddr, 1, -1, (char *)&fstbl, sizeof(fstbl),
				"kernel privilege table (file systems)");
			/*
			 * set ``laddr'' to the address of the privilege file
			 * list.
			*/
			laddr = (long)fstbl.lpd_list;
			/*
			 * if there's another file system in the list then
			 * set ``fsaddr'' to it.  Otherwise, it's at the
			 * end of the file systems list.
			*/
			if (fstbl.lpd_next)
				fsaddr = (long)fstbl.lpd_next;
			else
				fsaddr = 0;
			while (laddr) {		/* list of files loop */
				readmem(laddr, 1, -1, (char *)&flist,sizeof(flist),
					"kernel privilege table (files)");
				fprintf(fp, "maj,min device: %4u,%-5u",
					getemajor(devtbl.lpk_dev),
					geteminor(devtbl.lpk_dev));
				fprintf(fp, "\tfile id: %.8x", flist.lpf_nodeid);
				fprintf(fp, "\tvalidity: %.8x\n", flist.lpf_validity);
				pr_privs(flist, symb);
				/*
				 * If there's another file in the list then set
				 * set ``laddr'' to that address.  Otherwise,
				 * indicate it's at the end.
				*/
				if (flist.lpf_next)
					laddr = (long)flist.lpf_next;
				else
					laddr = 0;
			}
		}
	}
}


static	void
pr_privs(lst, symb)
lpftab_t	lst;
register int	symb;
{
	extern	void	prt_symbprvs();

	if (symb) {
		prt_symbprvs("fixed: ", lst.lpf_fixpriv);
		prt_symbprvs("inher: ", lst.lpf_inhpriv);
	}
	else {
		fprintf(fp, "fixed: %.8x", lst.lpf_fixpriv);
		fprintf(fp, "\tinher: %.8x", lst.lpf_inhpriv);
		fprintf(fp, "\n");
	}
}
