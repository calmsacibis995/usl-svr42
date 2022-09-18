/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Tmp.c	1.3.2.2"
#ident "@(#)fini_Tmp.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Msg - delete the resources used by a temp file

    SYNOPSIS
	void fini_Tmpfile(Tmpfile *ptmpfile)
*/

void fini_Tmpfile(ptmpfile)
Tmpfile *ptmpfile;
{
    Tmpfile *p;

    /* Remove all traces of the tmpfile */
    if (ptmpfile->tmpf)
	{
	fclose(ptmpfile->tmpf);
	ptmpfile->tmpf = 0;
	}

    if (ptmpfile->lettmp)
	{
	unlink(ptmpfile->lettmp);
	free(ptmpfile->lettmp);
	ptmpfile->lettmp = 0;
	}

    /* Remove the link for this tmpfile. */
    if (toptmpfile == ptmpfile)
	toptmpfile = ptmpfile->next;

    else
	for (p = toptmpfile; p; p = p->next)
	    if (p->next == ptmpfile)
		{
		p->next = ptmpfile->next;
		break;
		}
}
