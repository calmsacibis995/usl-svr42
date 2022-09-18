/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2move.c	1.1.2.2"
#ident "@(#)send2move.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2move - move list of recipients from one surrogate entry to another

    SYNOPSIS
	void send2move(Msg *pmsg, int osurr_num, int nsurr_num)

    DESCRIPTION
	send2move moves an entire list of recipients from one surrogate list
	to another surrogate list.
*/

void send2move(pmsg, osurr_num, nsurr_num)
Msg		*pmsg;
int		osurr_num;
int		nsurr_num;
{
    Recip *l;
    
    for (l = recips_head(pmsg, osurr_num); l->next != (Recip*) NULL; )
	{
	/* move recip to pmsg->preciplist[nsurr_num] */
	send2mvrecip(pmsg, osurr_num, nsurr_num);
	}
}
