/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:eaccess.c	2.1.2.2"
/*	Determine if the effective user id has the appropriate permission
	on a file.  
*/

#ifdef __STDC__
	#pragma weak eaccess = _eaccess
#endif
#include "synonyms.h"
#include <unistd.h>

extern int	access();


int
eaccess( path, amode )
const char		*path;
register int	amode;
{
	/* Use effective id bits */
	return access(path, EFF_ONLY_OK|amode); 
}
