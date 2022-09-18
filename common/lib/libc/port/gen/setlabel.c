/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/setlabel.c	1.2"

/*LINTLIBRARY*/

#ifdef __STDC__
	#pragma weak setlabel = _setlabel
#endif
#include "synonyms.h"
#include <pfmt.h>
#include "pfmt_data.h"
#include <string.h>

int
setlabel(label)
const char *label;
{
	if (!label)
		__pfmt_label[0] = '\0';
	else {
		strncpy(__pfmt_label, label, sizeof __pfmt_label - 1);
		__pfmt_label[sizeof __pfmt_label - 1] = '\0';
	}
	return 0;
}
