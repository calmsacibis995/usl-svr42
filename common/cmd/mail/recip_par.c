/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/recip_par.c	1.1.2.2"
#ident "@(#)recip_par.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	recip_parent - return parent of translated name

    SYNOPSIS
	Recip *recip_parent(Recip *r)

    DESCRIPTION
	 recip_parent() will traverse the parents of a given recipient name
	 to find the parent.
*/

Recip *recip_parent(r)
register Recip *r;
{
    while (r->parent != (Recip*)NULL)
	r = r->parent;
    return r;
}
