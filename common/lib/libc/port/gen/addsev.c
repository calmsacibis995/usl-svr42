/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/addsev.c	1.3"

#ifdef __STDC__
	#pragma weak addsev = _addsev
#endif
#include "synonyms.h"
#include "shlib.h"
#include <stdlib.h>
#include <pfmt.h>
#include "pfmt_data.h"
#include <string.h>

addsev(severity, string)
int severity;
const char *string;
{
	register int i, firstfree;
	
	/* Cannot redefine standard severity */
	if (severity <= 4)
		return -1;

	/* Locate severity in table */
	for (i = 0, firstfree = -1 ; i < __pfmt_nsev ; i++){
		if (__pfmt_sev_tab[i].severity == 0 && firstfree == -1)
			firstfree = i;
		if (__pfmt_sev_tab[i].severity == severity)
			break;
	}

	if (i == __pfmt_nsev){
		/* Removing non-existing severity */
		if (!string)
			return 0;
		/* Re-use old entry */
		if (firstfree != -1)
			i = firstfree;
		else {
			/* Allocate new entry */
			if (__pfmt_nsev++ == 0){
				if ((__pfmt_sev_tab = 
					malloc(sizeof (struct sev_tab))) == NULL)
					return -1;
			}
			else {
				if ((__pfmt_sev_tab = realloc(__pfmt_sev_tab,
					sizeof (struct sev_tab) * __pfmt_nsev)) == NULL)
					return -1;
			}
			__pfmt_sev_tab[i].severity = severity;
			__pfmt_sev_tab[i].string = NULL;
		}
	}
	if (!string){
		if (__pfmt_sev_tab[i].string)
			free(__pfmt_sev_tab[i].string);
		__pfmt_sev_tab[i].severity = 0;
		return 0;
	}
	if (__pfmt_sev_tab[i].string){
		if ((__pfmt_sev_tab[i].string = realloc(__pfmt_sev_tab[i].string,
			strlen(string) + 1)) == NULL)
			return -1;
	}
	else
		if ((__pfmt_sev_tab[i].string = malloc(strlen(string) + 1)) == NULL)
			return -1;
	(void) strcpy(__pfmt_sev_tab[i].string, string);
	return 0;
}
