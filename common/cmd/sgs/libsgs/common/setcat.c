/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/setcat.c	1.2"

#ifdef __STDC__	
	#pragma weak setcat = _setcat
#endif
#include "synonyms.h"
#include <stdio.h>

/* setcat(cat): dummy function.  Returns NULL since there is no
 * message catalog in the cross environment
 */
/*ARGSUSED*/
const char *
setcat(cat)
const char *cat;
{
	return NULL;
}
