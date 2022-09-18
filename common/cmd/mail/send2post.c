/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2post.c	1.2.2.2"
#ident "@(#)send2post.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2post - do mail post processing

    SYNOPSIS
	void send2post(Msg *pmsg, int wherefrom, t_surrtype surr_type);

    DESCRIPTION
	Loop through the surrogate file looking for postprocessing
	commands of the appropriate type (t_postprocess or t_error).
	The commands are passed on to send2d_p() for processing.
	wherefrom is either RECIPS_SUCCESS or RECIPS_FAILURE.
*/

void send2post(pmsg, wherefrom, surr_type)
Msg		*pmsg;
int		wherefrom;
t_surrtype	surr_type;
{
    static const char pn[] = "send2post";
    int surr_num;

    Tout(pn, "Postprocessing %s\n",
        (surr_type == t_postprocess) ? "Successes" : "Errors");

    /* Move all the pertinent messages */
    /* to the first post-processing command. */
    for (surr_num = 0; surr_num < surr_len; surr_num++)
	{
	Dout(pn, 5, "surr_num=%d\n", surr_num);
	if (surrfile[surr_num].surr_type == surr_type)
	    {
	    Dout(pn, 5, "found postprocessor at %d\n", surr_num);
	    send2move(pmsg, wherefrom, surr_num);
	    break;
	    }
	}

    /* Now postprocess the messages. */
    for ( ; surr_num < surr_len; surr_num++)
	{
	Dout(pn, 5, "surr_num=%d\n", surr_num);
	if (surrfile[surr_num].surr_type == surr_type)
	    send2d_p(pmsg, surr_num);
	else
	    send2move(pmsg, surr_num, surr_num + 1);
	}

    /* Move them all back since they're now all at RECIPS_LOCAL. */
    send2move(pmsg, surr_len + RECIPS_LOCAL, wherefrom);
}
