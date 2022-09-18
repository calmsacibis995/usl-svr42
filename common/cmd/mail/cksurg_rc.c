/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/cksurg_rc.c	1.6.2.2"
#ident "@(#)cksurg_rc.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	cksurg_rc - check the surrogate return code

    SYNOPSIS
	SendSurgRet cksurg_rc(int surr_num, int rc)

    DESCRIPTION
	Cksurg_rc() looks up the return code in the list
	of return codes for the given surrogate entry.
*/

SendSurgRet cksurg_rc (surr_num, rc)
int	surr_num, rc;
{
    return rc >= 0 ? (SendSurgRet)(surrfile[surr_num].statlist[rc]) : FAILURE;
}
