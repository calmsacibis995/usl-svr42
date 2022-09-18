/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2fleft.c	1.3.2.2"
#ident "@(#)send2fleft.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2findleft - an auxiliary function used by send2d_p() and send2tran()

    SYNOPSIS
	Recip *send2findleft(Msg *pmsg, int wherefrom,
	    int whereexec, int surr_num, char **lbraslist, char **lbraelist,
	    int nbra);

    DESCRIPTION
	send2findleft() looks through the list of recipients for a name which
	matches the regular expression, filling in the appropriate lists
	(whereexec for a match, surr_num+1 for a non-match). It then expands
	the commands for that recipient and returns it.
*/

Recip *send2findleft(pmsg, wherefrom, whereexec, surr_num, lbraslist, lbraelist, nbra)
Msg	*pmsg;
int	wherefrom;
int	whereexec;
int	surr_num;
char	**lbraslist;
char	**lbraelist;
{
    static const char pn[] = "send2findleft";
    Recip *l;	/* list head */
    Recip *r;	/* recipient at list head */
    int rnbra = surrfile[surr_num].recip_nbra;
    char	**rbraslist = lbraslist + nbra;
    char	**rbraelist = lbraelist + nbra;

    for (l = recips_head(pmsg, wherefrom); ((r = l->next) != (Recip*) NULL); )
	{
	/* r->cmdl will be non-NULL if it was expanded in a previous call to send2findright() */
	/* but couldn't be executed due to batchsize limitations. */
	if (r->cmdl ||
	    matchsurr(r->name, surrfile[surr_num].recip_regex, rbraslist, rbraelist, rnbra))
	    {
	    Tout(pn, "Matched recipient '%s':'%s'\n", s_to_c(surrfile[surr_num].recip_pattern), s_to_c(r->name));
	    if (!r->cmdl)
		{
		r->cmdl =
		    cmdexpand(pmsg, r, surrfile[surr_num].cmd_left, lbraslist, lbraelist, r->cmdl);
		r->cmdr =
		    cmdexpand(pmsg, r, surrfile[surr_num].cmd_right, lbraslist, lbraelist, r->cmdr);
		}

	    send2mvrecip(pmsg, wherefrom, whereexec);
	    return r;
	    }

	else
	    {
	    /* move recip to list->surr_recips[i+1] */
	    send2mvrecip(pmsg, wherefrom, surr_num + 1);
	    }
	}

    return 0;
}
