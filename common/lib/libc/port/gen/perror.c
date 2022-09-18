/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/perror.c	1.16"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * perror() - Print the error indicated
 * in the cerror cell.
 */
#include "synonyms.h"
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

extern int _sys_num_err;
extern const char _sys_errs[];
extern const int _sys_index[];

static const char catalog[] = "uxsyserr";

void
perror(s)
const char *s;
{
	register const char *c;
	register const char *colon;
	register int n;

	if (errno < 0 || errno >= _sys_num_err)
		c = __gtxt(catalog, 1, "Unknown error");
	else
		c = __gtxt(catalog, errno + 3, &_sys_errs[_sys_index[errno]]);
	if(s && (n = strlen(s))) {
		colon = __gtxt(catalog, 2, ": ");
		(void) write(2, s, (unsigned)n);
		(void) write(2, colon, strlen(colon));
	}
	(void) write(2, c, (unsigned)strlen(c));
	(void) write(2, (const char *)"\n", 1);
}
