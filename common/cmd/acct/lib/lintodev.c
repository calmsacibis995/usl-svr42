/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/lib/lintodev.c	1.9.3.3"
#ident "$Header: $"
/*
 *	convert linename to device
 *	return -1 if nonexistent or not character device
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include "acctdef.h"
#include <sys/stat.h>
static	char	devtty[5+LSZ+1]	= "/dev/xxxxxxxx";
char	*strncpy();

dev_t
lintodev(linename)
char linename[LSZ];
{
	struct stat sb;
	strncpy(&devtty[5], linename, LSZ);
	if (stat(devtty, &sb) != -1 && (sb.st_mode&S_IFMT) == S_IFCHR)
		return((dev_t)sb.st_rdev);
	return((dev_t)NODEV);
}
