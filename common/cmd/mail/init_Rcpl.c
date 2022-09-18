/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Rcpl.c	1.2.2.2"
#ident "@(#)init_Rcpl.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Reciplist - initialize a recipient list

    SYNOPSIS
	init_Reciplist (reciplist *list)

    DESCRIPTION
	Initialize a recipient list to have no recipients.
*/

void init_Reciplist (plist)
Reciplist	*plist;
{
	static const char pn[] = "init_Reciplist";
	Dout(pn, 0, "entered\n");
	plist->recip_list.next = 0;
	plist->recip_list.name = 0;
	plist->last_recip = &plist->recip_list;
}
