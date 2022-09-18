/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/new_Hdrs.c	1.4.2.3"
#ident "@(#)new_Hdrs.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	new_Hdrs - allocate and initialize a header

    SYNOPSIS
	Hdrs *new_Hdrs(int hdrtype, const char *name, const char *val)

    DESCRIPTION
	Allocate and initialize a header.
*/

static char MAnomem[] = ":407:malloc failed in %s(): %s\n";

Hdrs *new_Hdrs(hdrtype, name, val)
int hdrtype;
const char *name;
const char *val;
{
    static const char pn[] = "new_Hdrs";
    Hdrs *nhp;

    Dout(pn, 0, "entered\n");

    if ((nhp = (Hdrs*)malloc(sizeof(Hdrs))) == (Hdrs*)NULL)
	{
	errmsg(E_MEM, MAnomem, pn, strerror(errno));
	done(1);
	}

    nhp->next = nhp->prev = nhp->cont = 0;
    if ((nhp->value = strdup(val)) == (char*)NULL)
	{
	errmsg(E_MEM, MAnomem, pn, strerror(errno));
	done(1);
	}

    if ((hdrtype == H_NAMEVALUE) && name)
	{
	if ((nhp->name = strdup(name)) == (char*)NULL)
	    {
	    errmsg(E_MEM, MAnomem, pn, strerror(errno));
	    done(1);
	    }
	}

    else
	nhp->name = header[hdrtype].tag;

    nhp->hdrtype = hdrtype;
    return nhp;
}
