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

#ident	"@(#)libiaf:common/lib/libiaf/iaf/putava.c	1.2.1.2"
#ident  "$Header: putava.c 1.2 91/06/25 $"

/*	putenv - change environment variables
 *
 *	input - char *newava = a pointer to a string of the form
 *			       "name=value"
 *
 *	output - 0, if successful
 *		 1, otherwise
 */

#include <iaf.h>
#include <string.h>
#include <stdlib.h>

static int find_ava(), attr_match();

char **
putava(newava, avap)
char *newava;
char **avap;
{
	register int which;	    /* index of variable to replace */

	if (avap == NULL) {
		avap = (char **)malloc(2 * sizeof(char *));
		if (avap != NULL) {
			avap[0] = newava;
			avap[1] = NULL;
		}
		return(avap);
	}

	if ((which = find_ava(newava, avap)) < 0)  {
		/* for new ava which = -(size of(avap table)) */
		which = (-which) - 1;
		/* NOTE the assumption that the original space	*/
		/*	was malloc()'ed				*/
		avap = (char **)realloc(avap, (which + 2) * sizeof(char *));
		if (avap != NULL) {
			avap[which] = newava;
			avap[which+1] = NULL;
		}
	} else {
		/* we are replacing an old variable */
		avap[which] = newava;
	}
	return(avap);
}

/*
 *	find_ava - find where 'wanted' is in 'avap'
 *	input - wanted = string of form name=value
 *		avap = current ava list.
 *	output - index of name in environ that matches "name",
 *		 or -size of table, if none exists
 */
static int
find_ava(wanted, avap)
register char *wanted;
char **avap;
{
	register int count = 0;	/* index into environ */

	while(avap[count] != NULL)   {
		if (attr_match(avap[count], wanted)  != 0)
			return(count);
		count++;
	}
	return -(++count);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of 1,
 *	else return 0
 */

static int
attr_match(s1, s2)
register char *s1, *s2;
{
	while(*s1 == *s2)
		if ( (*s1++ == '\0') || (*s2++ == '=') )
			return(1);
	if ( ((*s2 == '\0') && (*s1 == '=')) ||
		((*s1 == '\0') && (*s2++ == '=')) )
		return(1);
	
	return(0);
}
