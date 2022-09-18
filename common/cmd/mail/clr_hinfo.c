/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/clr_hinfo.c	1.7.2.3"
#ident "@(#)clr_hinfo.c	2.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	clr_hdrinfo - clean out mail header information

    SYNOPSIS
	void clr_hdrinfo(Hdrinfo *phdrinfo)

    DESCRIPTION
	Clr_hinfo() cleans out hdrlines[] and other associated data
	in preparation for the next message.
*/

void clr_hdrinfo(phdrinfo)
Hdrinfo *phdrinfo;
{
    pophdrlist(phdrinfo, phdrinfo->hdrs[H_FROM]);
    phdrinfo->hdrs[H_FROM] = 0;
    pophdrlist(phdrinfo, phdrinfo->hdrs[H_RFROM]);
    phdrinfo->hdrs[H_RFROM] = 0;
    pophdrlist(phdrinfo, phdrinfo->hdrhead);
    phdrinfo->hdrhead = phdrinfo->hdrtail = 0;
    init_Hdrinfo(phdrinfo);
}
