/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Tmp.c	1.3.2.2"
#ident "@(#)init_Tmp.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Tmpfile - initialize a Tmpfile structure

    SYNOPSIS
	void init_Tmpfile(Tmpfile *ptmpfile)

    DESCRIPTION
	init_Tmpfile initializes a Tmpfile structure as if it had just been created

*/

void init_Tmpfile(ptmpfile)
Tmpfile *ptmpfile;
{
    static const char pn[] = "init_Tmpfile";
    Dout(pn, 0, "Entered\n");
    ptmpfile->lettmp = 0;
    ptmpfile->tmpf = 0;
    ptmpfile->next = toptmpfile;
    toptmpfile = ptmpfile;
}
