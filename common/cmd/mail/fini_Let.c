/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Let.c	1.1.2.2"
#ident "@(#)fini_Let.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Letinfo - delete the resources used by a message

    SYNOPSIS
	void fini_Letinfo (Letinfo *pletinfo)

    DESCRIPTION
	Free the space used by a letter.
*/

void fini_Letinfo(pletinfo)
Letinfo *pletinfo;
{
    fini_Tmpfile(&pletinfo->tmpfile);
}
