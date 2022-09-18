/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/strerror.c	1.1"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <stdio.h>

extern const char _sys_errs[];
extern const int _sys_index[];
extern int _sys_num_err;

char *
strerror(errnum)
int errnum;
{
	if (errnum < _sys_num_err && errnum >= 0)
		return(&_sys_errs[_sys_index[errnum]]);
	else
		return(NULL);
}
