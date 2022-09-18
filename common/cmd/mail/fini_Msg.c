/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Msg.c	1.3.2.2"
#ident "@(#)fini_Msg.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Msg - delete the resources used by a message

    SYNOPSIS
	void fini_Msg (Msg *pmsg)

    DESCRIPTION
	Free the space used by a message.
*/

void fini_Msg(pmsg)
Msg *pmsg;
{
    register int i;
    topmsg = pmsg->parent;
    for (i = surr_len + RECIPS_MAX; i-- > 0; )
	fini_Reciplist(&pmsg->preciplist[i]);
    free((char*)pmsg->preciplist);
    clr_hdrinfo(&pmsg->hdrinfo);
    s_free(pmsg->Rpath);
    s_free(pmsg->orig);
    fini_Tmpfile(&pmsg->tmpfile);
    fini_Tmpfile(&pmsg->SURRinput);
    if (pmsg->SURRerrfile)
	fclose(pmsg->SURRerrfile);
    if (pmsg->SURRoutfile)
	fclose(pmsg->SURRoutfile);
    s_free(pmsg->SURRcmd);
    if (pmsg->errmsg)
	del_Msg(pmsg->errmsg);
}
