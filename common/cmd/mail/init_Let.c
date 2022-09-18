/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Let.c	1.2.2.2"
#ident "@(#)init_Let.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Letinfo - initialize a Letinfo structure

    SYNOPSIS
	void init_Letinfo(Letinfo *pmsg)

    DESCRIPTION
	init_Letinfo initializes a Letinfo structure as if it had just been created
*/

void init_Letinfo(pletinfo)
register Letinfo *pletinfo;
{
    static const char pn[] = "init_Letinfo";
    Dout(pn, 0, "Entered\n");
    init_Hdrinfo(&pletinfo->hdrinfo);
    pletinfo->let[0].adr = 0;
    pletinfo->let[1].adr = 0;
    pletinfo->changed = 0;
    pletinfo->nlet = 0;
    pletinfo->onlet = 0;
    init_Tmpfile(&pletinfo->tmpfile);
}
