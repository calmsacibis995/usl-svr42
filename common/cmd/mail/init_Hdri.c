/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Hdri.c	1.2.2.3"
#ident "@(#)init_Hdri.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Hdrinfo - initialize a Hdrinfo structure

    SYNOPSIS
	void init_Hdrinfo(Hdrinfo *phdrinfo)

    DESCRIPTION
	init_Hdrinfo initializes a Hdrinfo structure as
	if it had just been created.
*/

void init_Hdrinfo(phdrinfo)
Hdrinfo *phdrinfo;
{
    static const char pn[] = "init_Hdrinfo";
    register int i;
    Dout(pn, 0, "Entered\n");
    phdrinfo->hdrhead = phdrinfo->hdrtail = 0;
    for (i = 0; i < H_MAX; i++)
	phdrinfo->hdrs[i] = 0;
    phdrinfo->orig_aff = 0;
    phdrinfo->orig_rcv = 0;
    phdrinfo->orig_tcopy = 0;
    phdrinfo->affcnt = 0;
    phdrinfo->fnuhdrtype = 0;
    phdrinfo->last_hdr = 0;
}
