/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkdaemon.d/file.c	1.2.5.2"
#ident  "$Header: file.c 1.2 91/06/21 $"

#include <sys/types.h>
#include <sys/stat.h>

int
f_exists( fname )
char *fname;
{
	struct stat buf;
	return( !stat( fname, &buf ) );
}
