/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/installf/dofinal.c	1.10.10.5"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include "install.h"

extern struct cfent 
		**eptlist;
extern char	errbuf[],
		*errstr,
		*prog,
		*pkginst;
extern int	errno,
		warnflag;

extern struct pinfo
		*eptstat();
extern void	progerr(),
		logerr(),
		quit();
extern int	finalck(),
		srchcfile(),
		putcfile(),
		rmprivs();

#define ERR_WRITE \
"write of intermediate contents file failed"


dofinal(fp, fpo, rmflag, myclass)
FILE	*fp;
FILE	*fpo;
int	rmflag;
char	*myclass;
{
	struct cfent entry;
	struct pinfo *pinfo;
	int	n, indx, dbchg;
	char	en_path[PATH_MAX];

	entry.pinfo = NULL;
	indx = 0;
	while(eptlist && eptlist[indx] && (eptlist[indx]->ftype == 'i'))
		indx++;

	dbchg = 0;
	while(n = srchcfile(&entry, "*", fp, fpo)) {
		if(n == -1) {
			progerr("bad entry read in contents file");
			if(entry.path)
				logerr("pathname=%s", entry.path);
		}
		if(n < 0) {
			logerr("problem=%s", errstr);
			quit(99);
		}
		if(myclass && strcmp(myclass, entry.class)) {
			if(putcfile(&entry, fpo)) {
				progerr(ERR_WRITE);
				quit(99);
			}
			continue;
		}

		pinfo = entry.pinfo;
		while(pinfo) {
			if(!strcmp(pkginst, pinfo->pkg))
				break;
			pinfo = pinfo->next;
		}
		if(pinfo) {
			if(rmflag && (pinfo->status == '-')) {
				dbchg++;
				(void) eptstat(&entry, pkginst, '@');
				/* remove any privileges on the file */
				if((strcmp(entry.ainfo.priv_fix, "NONE") &&
				    strcmp(entry.ainfo.priv_fix, "NULL")) ||
				    (strcmp(entry.ainfo.priv_inh, "NONE") &&
				     strcmp(entry.ainfo.priv_inh, "NULL"))) {
					if(rmprivs(entry.path)) 
						printf("WARNING: Could not remove privileges from %s\n", entry.path);
				}
				if(entry.npkgs) {
					/*
					 * Check file because contents may have changed.
					 */
					finalck(&entry, 1, -1);

					if(putcfile(&entry, fpo)) {
						progerr(ERR_WRITE);
						quit(99);
					}
				}
				continue;
			} else if(!rmflag && (pinfo->status == '+')) {
				dbchg++;
				/* copy path so we can do another srchcfile
		 		* for a pathname which could be linked to it
		 		* so that we can reset any privileges
		 		*/
				(void) strcpy(en_path, entry.path);
				if(entry.ftype == 'e')
					pinfo->status = (finalck(&entry, 1, -1) ? '!' : '\0');
				else
					pinfo->status = (finalck(&entry, 1, 1) ? '!' : '\0');
				(void) strcpy(entry.path, en_path);
			}
		}
		if(putcfile(&entry, fpo)) {
			progerr(ERR_WRITE);
			quit(99);
		}
	}
	return(dbchg);
}
