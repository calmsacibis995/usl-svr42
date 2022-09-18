/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
	fqn - Convert a resource name into a fully qualified name.
	If a name does not contain a domain name, prepend one.
	The resultant name is placed in a supplied buffer buf.
	fqn does not complain.  If it can't get a domain name, it doesn't.
*/

#ident	"@(#)rmount:fqn.c	1.1.10.2"
#ident  "$Header: fqn.c 1.2 91/06/27 $"
#include <nserve.h>
#include <sys/types.h>
#include <sys/rf_sys.h>
#include <string.h>
#include "rmount.h"

extern int	rfsys();	/* undocumented rfs system call */

char *
fqn(name, buf)
	char		*name;
	char		*buf;
{
	static char	dname[MAXDNAME] = "";
	static char	*dnamep;

	if (strchr(name, '.'))	/* check if name is already fully qualified */
		return strcpy(buf, name);
	if (!dname[0])		/* dname not initialized yet */
		if (rfsys(RF_GETDNAME, dname, MAXDNAME) < 0)
			return strcpy(buf, name);
		else {
			for (dnamep=dname; *dnamep; ++dnamep) ;
			*dnamep++ = '.';
			*dnamep++ = '\0';
		}
	Strcpy(buf, dname);
	Strcat(buf, name);		
	return buf;
}
