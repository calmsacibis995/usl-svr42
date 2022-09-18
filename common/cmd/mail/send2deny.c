/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2deny.c	1.4.2.2"
#ident "@(#)send2deny.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2deny - check list of users for denial

    SYNOPSIS
	void send2deny(Msg *pmsg, int surr_num)

    DESCRIPTION
	send2deny() will traverse the current recipient list and
	do denial processing for each user which matches.
*/

void send2deny(pmsg, surr_num)
Msg	*pmsg;
int	surr_num;
{
    static const char pn[] = "send2deny";
    Recip *l, *r;
    char *lbraslist[2 * RE_NBRAK], *lbraelist[2 * RE_NBRAK];
    int onbra = surrfile[surr_num].orig_nbra;
    int rnbra = surrfile[surr_num].recip_nbra;
    int origmatch = matchsurr(pmsg->orig, surrfile[surr_num].orig_regex, lbraslist, lbraelist, onbra);

    Tout(pn, "Deny '%s'\n", surrfile[surr_num].deny_msg ? s_to_c(surrfile[surr_num].deny_msg) : "");
    if (origmatch)
	Tout(pn, "Matched originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));
    for (l = recips_head(pmsg, surr_num); ((r = l->next) != (Recip*) NULL); )
	{
	if (origmatch &&
	    !r->accepted &&
	    matchsurr(r->name, surrfile[surr_num].recip_regex, lbraslist+onbra, lbraelist+onbra, rnbra))
	    {
	    /* reject mail */
	    Tout(pn, "Matched recipient '%s'!\n", s_to_c(surrfile[surr_num].recip_pattern));
	    Tout((char*)0, "%s Denied\n", s_to_c(r->name));
	    pfmt(stderr, MM_ERROR, ":437:Denied access for '%s'->'%s'\n", s_to_c(pmsg->orig), s_to_c(r->name));
	    if (pmsg->ret_on_error)
		{
		r->cmdl = cmdexpand(pmsg, r, surrfile[surr_num].deny_msg, lbraslist, lbraelist, r->cmdl);
		Tout(pn, "Returning denied mail\n");
		send2mvrecip(pmsg, surr_num, surr_len + RECIPS_TEMP);
		if (r->cmdl)
		    pfmt(stderr, MM_NOSTD, ":445:Reason for denied access: %s\n", s_to_c(r->cmdl));
		}

	    else
		{
		Tout(pn, "ret_on_error == 0, not returning denied mail\n");
		pfmt(stderr, MM_ERROR, ":62:Cannot return mail.\n");
		send2mvrecip(pmsg, surr_num, surr_len + RECIPS_FAILURE);
		}
	    }

	else
	    {
	    Dout(pn, 5, "%s Moved to %d\n", s_to_c(r->name), surr_num+1);
	    /* move recip to pmsg->preciplist[surr_num+1] */
	    send2mvrecip(pmsg, surr_num, surr_num + 1);
	    }
	}

    if (recips_exist(pmsg, surr_len + RECIPS_TEMP))
	{
	retmail(pmsg, surr_len + RECIPS_TEMP, E_DENY, (char*)0);
	send2clean(pmsg, surr_len + RECIPS_TEMP, surr_len + RECIPS_FAILURE);
	}
}
