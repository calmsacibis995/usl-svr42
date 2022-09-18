/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:error.c	1.2"
#endif

#define ERROR_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "mail.h"
#include <Gizmo/Gizmos.h>
#include <locale.h>

/*
 * Given the errno return a printable message in some language.
 */

char *
GetTextGivenErrno (errno)
int	errno;
{
	char	buf[10];

	/*
	 * Return error message errno from catalog UX.
	 */
	sprintf (buf, "UX:%d", errno+1);
	return (char *)gettxt (buf, strerror (errno));
}

/*
 * Given an error message produced by perror return a printable message in
 * some language.
 */

char *
GetTextGivenText (perror)
char *	perror;
{
	char *		cp;
	int		i;

	/*
	 * Look though all of the system errors for this error message
	 */
	for (i=0; (cp=strerror(i))!=NULL; i++) {
		if (strncmp (perror, cp, strlen(cp)) == 0) {
			/*
			 * String found
			 */
			return GetTextGivenErrno (i);
		}
	}

	return NULL;
}
