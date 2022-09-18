/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/sendlist.c	1.10.2.3"
#ident "@(#)sendlist.c	1.15 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	sendlist - send copy to specified users

    SYNOPSIS
	int sendlist(Msg *pmsg, int level)

    DESCRIPTION
	sendlist() will traverse the current recipient list and
	send a copy of the given letter to each user specified.
	It returns 1 if the sending fails, 0 otherwise.
*/

int sendlist(pmsg, level)
Msg	*pmsg;
int	level;
{
    static const char pn[] = "sendlist";
    register int surr_num;

    Dout(pn, 0, "entered, level=%d\n", level);

    if (level > FWRDLEVELS)
	{
	error = E_UNBND;
	retmail(pmsg, 0, E_UNBND, ":343:Unbounded local forwarding loop\n");
	return 0;
	}

    for (surr_num = 0; surr_num < surr_len && !interrupted; surr_num++)
	{
	Dout(pn, 5, "surr_num=%d\n", surr_num);
	if (recips_exist(pmsg, surr_num))
	    {
	    Dout(pn, 5, "recips exist!\n");
	    switch (surrfile[surr_num].surr_type)
		{
		case t_accept_name:
		    send2accept(pmsg, surr_num);
		    break;

		case t_deny_name:
		    send2deny(pmsg, surr_num);
		    break;

		case t_postprocess:
		case t_error:
		    send2move(pmsg, surr_num, surr_num + 1);
		    break;

		case t_transport:
		    send2d_p(pmsg, surr_num);
		    break;

		case t_translate:
		    if (send2tran(pmsg, surr_num))
			/* If we now have an entry on the 1st surrogate queue, */
			/* start the loop over from the beginning. */
			surr_num = -1;
		    break;

		case t_quit:
		    send2quit(pmsg, surr_num);
		}
	    }
	}

    /* Deliver mail locally. */
    if (recips_exist(pmsg, surr_len + RECIPS_LOCAL) && !interrupted)
	{
	Dout(pn, 5, "local mail\n");
	send2local(pmsg, surr_len + RECIPS_LOCAL, level);
	}

    /* Deliver all non-delivery notifications. */
    if (!interrupted && pmsg->errmsg)
	send_retmail(pmsg->errmsg);

    /* Do post-processing */
    if (!flglb && !interrupted)
	{
	send2post(pmsg, surr_len + RECIPS_SUCCESS, t_postprocess);
	send2post(pmsg, surr_len + RECIPS_FAILURE, t_error);
	}

    /* If we've been interrupted, finish up quickly! */
    if (interrupted)
	done(0);

    /* If there were no failures, then return 1 */
    return !recips_exist(pmsg, surr_len + RECIPS_FAILURE);
}
