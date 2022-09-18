/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fsba:i386/cmd/fsba/bsize.c	1.1"
#ident	"$Header: bsize.c 1.1 91/07/08 $"

#include <sys/types.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#include <sys/stat.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>
#include "fsba.h"

s5bsize(fd, super)
int fd;
struct filsys *super;
{
	return super->s_type;
}
