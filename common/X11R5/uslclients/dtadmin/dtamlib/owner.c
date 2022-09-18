/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:dtamlib/owner.c	1.6"
#endif
/*
 *	owner.c:	validate owner privileges
 *
 */
#include <stdio.h>
#include <string.h>
#include <Xt/Intrinsic.h>
#include "owner.h"


Boolean	_DtamIsOwner(char *adm_name)
{
	char	buf[BUFSIZ];

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", adm_name);
	return (system(buf)==0);
}
