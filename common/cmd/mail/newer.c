/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/newer.c	1.2.2.2"
#ident "@(#)newer.c	1.2 'attmail mail(1) command'"
#include "libmail.h"

/*
    NAME
	newer - check which file is newer

    SYNOPSIS
	int newer(const char *file1, const char *file2)

    DESCRIPTION
	Check to see if file1 is newer than file2
*/

int newer(file1, file2)
const char *file1;
const char *file2;
{
    struct stat s1, s2;
    if (stat(file1, &s1) == -1)
	return 0;
    if (stat(file2, &s2) == -1)
	return 0;
    return (s1.st_mtime > s2.st_mtime);
}
