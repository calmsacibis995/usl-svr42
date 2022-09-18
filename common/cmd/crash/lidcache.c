/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/lidcache.c	1.1.2.3"
#ident	"$Header: lidcache.c 1.1 91/07/23 $"


#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/var.h>
#include "crash.h"
#include <sys/time.h>
#include <mac.h>

struct syment		*Lidvp = NULL; /*lid.internel vnode pointer*/
struct syment		*Lidcache = NULL; /*the lid cache itself*/
struct syment		*Mac_cachel = NULL; /*cache length*/
struct syment		*Mac_cachew = NULL; /*cache width*/

/*
 * Get and print the lidvp and lid cache
 */

int
getlidcache()
{
	struct vnode *lidvp;	/*lid.internal vnode pointer*/
	struct mac_cachent *lidcache; /*the lid cache itself*/
	struct mac_cachent cachent; /*the current cache entry */
	int clength, cwidth; /*the lid cache length and width, in lid cache entries*/
	int row, col; /*for looping through lid cache */
	char	*heading1 = "                 LAST REFERENCE TIME FL\n";
	char	*heading2 = "ROW  COL      LID     (SEC)   (NSEC) AG CLASS CATEGORIES\n\n";

	if(!Lidvp)
		if(!(Lidvp = symsrch("mac_lidvp")))
			error("lid.internal vnode pointer not found in symbol table\n");

	if(!Mac_cachel)
		if(!(Mac_cachel = symsrch("mac_cachel")))
			error("lid cache length not found in symbol table\n");

	if(!Mac_cachew)
		if(!(Mac_cachew = symsrch("mac_cachew")))
			error("lid cache width not found in symbol table\n");

	if(!Lidcache)
		if(!(Lidcache = symsrch("mac_lidcache")))
			error("lid cache not found in symbol table\n");

	readmem(Lidvp->n_value, 1, -1, (char *)&lidvp, sizeof(struct vnode *),
		"lid.internal vnode pointer");

	readmem(Mac_cachel->n_value, 1, -1, (char *)&clength, sizeof clength,
		"lid cache length");

	readmem(Mac_cachew->n_value, 1, -1, (char *)&cwidth, sizeof cwidth,
		"lid cache width");

	readmem((long)(Lidcache->n_value), 1, -1, (char *)&lidcache, sizeof lidcache, "lid cache");

	fprintf(fp, "LID.INTERNAL VNODE POINTER = 0x%x\n", lidvp);
	fprintf(fp, "LID CACHE LENGTH = %d\n", clength);
	fprintf(fp, "LID CACHE WIDTH = %d\n\n", cwidth);
	fprintf(fp, heading1);
	fprintf(fp, heading2);

	for (row = 0; row < clength; row++) {
		for (col = 0; col < cwidth; col++) {
			readmem(lidcache,1,-1, &cachent,
				sizeof(struct mac_cachent), "lid cache entry");
				prt_cachent(row,col,&cachent);
				lidcache++;
		}
	}
}

/*
 * Print out a single lid cache entry.
 */

prt_cachent(row, col, entryp)
int row, col;
struct mac_cachent *entryp;
{
	ushort *catsigp; /* ptr to category significance array entry */
	ulong *catp;	 /* ptr to category array entry */
	int i, firstcat = 1;

	if (entryp->ca_lid == 0)
		return 0;
	fprintf(fp,"[%2d][%2d] %8d %9d %9d %c  %3d  ", row, col, entryp->ca_lid,
		entryp->ca_lastref.tv_sec, entryp->ca_lastref.tv_nsec,
		entryp->ca_level.lvl_valid, entryp->ca_level.lvl_class);

	/*
	 * Print out the category numbers in effect.
	 */
	for (catsigp = &entryp->ca_level.lvl_catsig[0],
		catp = &entryp->ca_level.lvl_cat[0];
	     *catsigp != 0; catsigp++, catp++) {
		for (i = 0; i < NB_LONG; i++) {
			if (*catp & ((ulong)(1 << (NB_LONG - 1))>>i)) {
				if (!firstcat)
					fprintf(fp,",");
				/* print the category # */
				fprintf(fp,"%d",
				   ((ulong)(*catsigp-1)<<CAT_SHIFT)+i+1);
				firstcat = 0;
			} /* end-if catp */
		} /* end-for till CATSIZE */
	} /* end-for catsig */
	fprintf(fp,"\n");
}
