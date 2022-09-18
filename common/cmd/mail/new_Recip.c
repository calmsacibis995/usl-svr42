/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/new_Recip.c	1.4.2.2"
#ident "@(#)new_Recip.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	new_Recip - allocate and initialize a recipient

    SYNOPSIS
	Recip *new_Recip(char *name, Recip *parent, int fullyresolved)

    DESCRIPTION
	Allocate and initialize a recipient.
*/

Recip *new_Recip (name, parent, fullyresolved)
char *name;
Recip *parent;
int fullyresolved;
{
    static const char pn[] = "new_Recip";
    Recip	*rp;
    Dout(pn, 0, "entered\n");

    if ((rp = (Recip*) malloc (sizeof(Recip))) == (Recip *)NULL) {
	errmsg(E_MEM, ":407:malloc failed in %s(): %s\n", pn, strerror(errno));
	done(1);
    }

    /* store a copy of the name */
    rp->next = (Recip *)NULL;
    rp->name = s_copy(name);
    rp->cmdl = 0;
    rp->cmdr = 0;
    rp->parent = parent;
    rp->accepted = 0;
    rp->local = 0;
    rp->useruid = 0;
    rp->translated = 0;
    rp->fullyresolved = fullyresolved;
    rp->SURRcmd = 0;
    rp->SURRoutput = 0;
    rp->SURRrc = SURG_RC_DEF;
    rp->lastsurrogate = -1;
    return rp;
}
