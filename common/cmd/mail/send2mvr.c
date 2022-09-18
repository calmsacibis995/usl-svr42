/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2mvr.c	1.2.2.2"
#ident "@(#)send2mvr.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2mvrecip - move a recipient from one surrogate list to another

    SYNOPSIS
	void send2mvrecip(Recip **fromlist, Recip **tolist, Recip **fromptr;

    DESCRIPTION
	send2mvrecip moves the top recipient from one surrogate list
	to the end of another surrogate list.

	NOTE: It assumes that there is a recipient on the list to be moved.
*/

void send2mvrecip(pmsg, osurr_num, nsurr_num)
Msg *pmsg;
int osurr_num;
int nsurr_num;
{
    static const char pn[] = "send2mvrecip";
    Reciplist *o = &pmsg->preciplist[osurr_num];
    Reciplist *n = &pmsg->preciplist[nsurr_num];
    Recip *or = o->recip_list.next;
    Recip *sv = or->next;

    /* move recipient to end of new list */
    Dout(pn, 50, "name='%s', from=%d, to=%d\n", s_to_c(or->name), osurr_num, nsurr_num);
    or->next = (Recip *)NULL;
    n->last_recip->next = or;
    n->last_recip = or;

    /* patch up the old list */
    o->recip_list.next = sv;
    if (o->last_recip == or)
        o->last_recip = &o->recip_list;
}
