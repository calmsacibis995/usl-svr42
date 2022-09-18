/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/io.c	1.5.5.2"
#ident  "$Header: io.c 1.2 91/06/21 $"

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

int read(), write();

int
safe_read(fd, buf, size)
int fd;
char *buf;
int size;
{
	int ret;

	errno = 0;

	while(((ret = read(fd, buf, (unsigned) size)) < 0) && (errno == EINTR))
		continue;

	return(ret);
}



int
safe_write(fd, buf, size)
int fd;
char *buf;
int size;
{
	int ret;

	errno = 0;

	while(((ret = write(fd, buf, (unsigned) size)) < 0) && (errno == EINTR))
		continue;
	return(ret);
}



int
safe_stat(path, st)
char *path;
struct stat *st;
{
	int ret;

	errno = 0;

	while(((ret = stat(path, st)) < 0) && (errno == EINTR))
		continue;
	return(ret);
}
