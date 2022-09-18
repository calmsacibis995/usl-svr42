/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/setlabel.c	1.2"

#ifdef __STDC__
	#pragma weak setlabel = _setlabel
#endif
#include "synonyms.h"

/* setlabel -- does notthing -- a dummy function for the cross envrionment
 * since the cross environment does not have message catalogs
 */

/*ARGSUSED*/
int
setlabel(label)
const char *label;
{
	return 0;
}
