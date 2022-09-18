/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Rcpl.c	1.2.2.2"
#ident "@(#)fini_Rcpl.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Reciplist - free the resources used by a recipient list

    SYNOPSIS
	fini_Reciplist (reciplist *list)

    DESCRIPTION
	Free the space used by a recipient list.
*/

void fini_Reciplist (plist)
Reciplist	*plist;
{
	static const char pn[] = "fini_Reciplist";
	Recip		*r = &plist->recip_list;
	Dout(pn, 0, "entered\n");
	if (r->next != (Recip *)NULL) {
		for (r = r->next; r != (Recip *)NULL; ) {
			Recip *old = r;
			r = old->next;
			del_Recip(old);
		}
	}
}
