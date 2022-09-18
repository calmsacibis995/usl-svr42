/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/setlocale.c	1.2"
/*
* setlocale - dummy function for the cross environment
*/
#include "synonyms.h"
#include <stdio.h>

/*ARGSUSED*/
char *
setlocale(cat, loc)
int cat;
const char *loc;
{
	return (char *)NULL;
}
