/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgmk/getinst.c	1.1.2.2"
#ident  "$Header: getinst.c 1.2 91/06/27 $"
#include <stdio.h>
#include <string.h>
#include <pkginfo.h>
#include "install.h"

extern char	*pkgarch, 
		*pkgvers,
		*pkgdir;

extern int	pkginfo();

static char	newinst[15];
static char	*nextinst();

char *
getinst(pkgabrv, info, npkgs)
char	*pkgabrv;
struct pkginfo *info;
int	npkgs;
{
	int	i;
	char	*inst, *sameinst;

	sameinst  = NULL;
	for(i=0; i < npkgs; i++) {
		if(!strcmp(info[i].arch, pkgarch)) {
		   	if(!strcmp(info[i].version, pkgvers))
				sameinst = info[i].pkginst;
		}
	}

	if(sameinst) 
		inst = sameinst;
	else {
		inst = nextinst(pkgabrv);
		printf("\nCreating %s as <%s>\n", pkgabrv, inst);
	}

	return(inst);
}

static char *
nextinst(pkgabrv)
char	*pkgabrv;
{
	struct pkginfo info;
	int	n;

	n = 2; /* requirements say start at 2 */

	info.pkginst = NULL;
	(void) strcpy(newinst, pkgabrv);
	while(pkginfo(&info, newinst, NULL, NULL) == 0)
		(void) sprintf(newinst, "%s.%d", pkgabrv, n++);
	return(newinst);
}
