/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/srcpath.c	1.1.6.2"
#ident  "$Header: srcpath.c 1.2 91/06/27 $"

#include <limits.h>
#include <string.h>

extern int	sprintf();

char *
srcpath(d, p, part, nparts)
char	*d, *p;
int	part, nparts;
{
	static char tmppath[PATH_MAX];
	char	*copy;

	copy = tmppath;
	if(d) {
		(void) strcpy(copy, d);
		copy += strlen(copy);
	} else
		copy[0] = '\0';

	if(nparts > 1) {
		(void) sprintf(copy,
			((p[0] == '/') ? "/root.%d%s" : "/reloc.%d/%s"), 
			part, p);
	} else {
		(void) sprintf(copy, 
			((p[0] == '/') ? "/root%s" : "/reloc/%s"), p);
	}
	return(tmppath);
}
