/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/del_Msg.c	1.1"
#ident "@(#)del_Msg.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	del_Msg - delete an allocated message

    SYNOPSIS
	void del_Msg(Msg*)

    DESCRIPTION
	Deallocate an allocated message.
*/

void del_Msg(old)
Msg *old;
{
    static const char pn[] = "del_Msg";
    Dout(pn, 0, "entered\n");
    fini_Msg(old);
    free((char*)old);
}
