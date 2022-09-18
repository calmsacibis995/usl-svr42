/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Msg.c	1.4.2.2"
#ident "@(#)init_Msg.c	1.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Msg - initialize a Msg structure

    SYNOPSIS
	void init_Msg(Msg *pmsg)

    DESCRIPTION
	init_Msg initializes a Msg structure as if it had just been created
*/

void init_Msg(pmsg)
Msg *pmsg;
{
    static const char pn[] = "init_Msg";
    register int i;

    Dout(pn, 0, "Entered\n");
    pmsg->type = Msg_msg;
    pmsg->preciplist = (Reciplist*) malloc(sizeof(Reciplist) * (surr_len + RECIPS_MAX));
    if (!pmsg->preciplist)
        {
	pfmt(stderr, MM_ERROR, ":10:Out of memory: %s\n", strerror(errno));
	error = E_MEM;
	Dout(pn, 0, "Can't allocate memory\n");
	done(0);
	}

    for (i = surr_len + RECIPS_MAX; i-- > 0; )
	init_Reciplist(&pmsg->preciplist[i]);

    pmsg->Rpath = s_new();
    pmsg->binflag = C_Text;
    init_Hdrinfo(&pmsg->hdrinfo);
    pmsg->orig = s_new();
    pmsg->msgsize = 0;
    pmsg->ret_on_error = (flgT || flglb) ? 0 : 1;
    pmsg->SURRerrfile = 0;
    pmsg->SURRoutfile = 0;
    pmsg->SURRcmd = 0;
    pmsg->parent = 0;
    pmsg->surg_rc = SURG_RC_DEF;
    pmsg->delivopts = 0;
    pmsg->errmsg = 0;
    topmsg = pmsg;
    init_Tmpfile(&pmsg->tmpfile);
    init_Tmpfile(&pmsg->SURRinput);
}
