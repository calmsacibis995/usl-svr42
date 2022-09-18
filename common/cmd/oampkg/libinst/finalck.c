/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/finalck.c	1.6.8.2"
#ident  "$Header: finalck.c 1.2 91/06/27 $"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <pkgstrct.h>

extern int	warnflag;
extern char	errbuf[];

extern void	logerr();
extern int	averify(),
		cverify();

int
finalck(ept, attrchg, contchg)
struct cfent *ept;
int	attrchg, contchg;
{
	int	errflg;
	int	n;

	errflg = 0;
	if(attrchg || contchg) {
		/* verify change, or fix if possible */
		if(n = averify(1, &ept->ftype, ept->path, &ept->ainfo, 0)) {
			logerr("ERROR: attribute verification of <%s> failed",
				ept->path);
			logerr(errbuf);
			errflg++;
			warnflag++;
			if(n == VE_EXIST)
				return(1); /* no need to check contents */
		}
	}
	if(contchg && strchr("fev", ept->ftype)) {
		/* verify change was executed properly */
		if((contchg < 0) || (ept->ftype == 'e')) {
			ept->cinfo.modtime = BADCONT;
			ept->cinfo.size = BADCONT;
			ept->cinfo.cksum = BADCONT;
		}
		if(cverify(1, &ept->ftype, ept->path, &ept->cinfo)) {
			logerr("ERROR: content verification of <%s> failed",
				ept->path);
			logerr(errbuf);
			errflg++;
			warnflag++;
		}
	}
	return(errflg);
}
