/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2quit.c	1.2.2.2"
#ident "@(#)send2quit.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2quit - move users directly to local processing

    SYNOPSIS
	void send2quit(Msg *pmsg, int surr_num)

    DESCRIPTION
	send2quit() will traverse the current recipient list and
	move each matched user directly to local processing.
*/

void send2quit(pmsg, surr_num)
Msg	*pmsg;
int	surr_num;
{
    static const char pn[] = "send2quit";
    Recip *l, *r;
    int origmatch = matchsurr(pmsg->orig, surrfile[surr_num].orig_regex, (char**)0, (char**)0, 0);

    Tout(pn, "Quit to Local\n");
    if (origmatch)
	Tout(pn, "Matched originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));
    for (l = recips_head(pmsg, surr_num); ((r = l->next) != (Recip*) NULL); )
	{
	if (origmatch &&
	    matchsurr(r->name, surrfile[surr_num].recip_regex, (char**)0, (char**)0, 0))
	    {
	    Tout(pn, "Matched recipient '%s'!\n", s_to_c(surrfile[surr_num].recip_pattern));
	    Tout((char*)0, "%s moved to Local queue\n", s_to_c(r->name));
	    send2mvrecip(pmsg, surr_num, surr_len + RECIPS_LOCAL);
	    }

	else
	    send2mvrecip(pmsg, surr_num, surr_num + 1);
	}
}
