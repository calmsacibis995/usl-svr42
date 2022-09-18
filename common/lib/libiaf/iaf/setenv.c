/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libiaf:common/lib/libiaf/iaf/setenv.c	1.1.1.2"
#ident  "$Header: setenv.c 1.2 91/06/25 $"
#include	<stdio.h>
#include	<iaf.h>

extern	char	**_environ;

int
set_env()
{

	char	**envp, **avap;
	char	*ptr;

	if (( avap = retava(0)) == NULL)
		return(1);

	if (( ptr = getava("ENV", avap)) == NULL)
		return(1);

	if (( envp = strtoargv(ptr)) == NULL)
		return(1);

	_environ = envp;
	return(0);
}
