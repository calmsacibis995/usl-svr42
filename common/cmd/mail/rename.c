/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/rename.c	1.1.2.2"
#ident "@(#)rename.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	rename - implementation of rename() for older systems

    SYNOPSIS
	int rename(char *old, char *new)

    DESCRIPTION
	Rename the given old filename to the given new filename.
*/
#ifdef SVR3
int rename(old, new)
char *old;
char *new;
{
    /* eliminate the new name */
    (void) unlink(new);

    /* link it in */
    if (link(old,new) != 0)
	return -1;

    /* unlink the old name */
    if (unlink(old) != 0)
	return -1;
    return 0;
}
#endif
