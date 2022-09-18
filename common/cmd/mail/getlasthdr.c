/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/getlasthdr.c	1.2"
#ident "@(#)getlasthdr.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	getlasthdr - get last header of the given type

    SYNOPSIS
	Hdrs *getlasthdr(Hdrs *hdr)

    DESCRIPTION
	Getlasthdr() looks through the list of headers starting
	at the given hdr and finds the last one of that type
	in that particular run of headers of that type.
*/

Hdrs *getlasthdr(hdr)
Hdrs *hdr;
{
    int hdrtype = hdr ? hdr->hdrtype : -1;
    while (hdr && hdr->next && hdr->next->hdrtype == hdrtype)
	hdr = hdr->next;
    return hdr;
}
