/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/gettxt.c	1.2"

/*	gettxt()
 *	returns the default message
 */
#ifdef __STDC__
	#pragma weak gettxt = _gettxt
#endif
#include "synonyms.h"

/*ARGSUSED*/
char *
gettxt(msgid, dflt)
const char *msgid, *dflt;
{
	return(dflt);
}
