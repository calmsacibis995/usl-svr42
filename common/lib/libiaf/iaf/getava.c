/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*      Copyright (c) 1989 AT&T */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T     */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */

#ident	"@(#)libiaf:common/lib/libiaf/iaf/getava.c	1.1.1.2"
#ident  "$Header: getava.c 1.2 91/06/25 $"

/*
 *	getava(name, ptr)
 *	returns ptr to value associated with name, if any, else NULL
 */

#include <iaf.h>
static char *avmatch();

char *
getava(register char *name, char **avap)
{
	register char *v;

	if(avap == NULL)
		return(NULL);
	while(*avap != NULL)
		if((v = avmatch(name, *avap++)) != NULL)
			return(v);
	return(NULL);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of s2, else NULL
 *	used for environment searching: see getenv
 */

static char *
avmatch(s1, s2)
register char *s1, *s2;
{
	while(*s1 == *s2)
		if ((*s1++ == '\0') || (*s2++ == '='))
			return(s2);
	if ( ((*s2 == '\0') && (*s1 == '=')) ||
		((*s1 == '\0') && (*s2++ == '=')) )
		return(s2);
	return(NULL);
}
