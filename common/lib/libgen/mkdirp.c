/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:mkdirp.c	1.7"

#ifdef __STDC__
	#pragma weak mkdirp = _mkdirp
#endif
#include "synonyms.h"

/* Creates directory and its parents if the parents do not
** exist yet.
**
** Returns -1 if fails for reasons other than non-existing
** parents.
** Does NOT compress pathnames with . or .. in them.
*/
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static char *compress();
VOID *malloc();
void free();
extern int	access();
extern int	mkdir();

int
mkdirp(d, mode)
const char *d;
mode_t mode;
{
	char  *endptr, *ptr, *slash, *str, *savestr;

	str=compress(d);

	/* If space couldn't be allocated for the compressed names, return. */

	if ( str == NULL )
		return(-1);
	ptr = str;


	/* Try to make the directory */

	if (mkdir(str, mode) == 0){
		free(str);
		return(0);
	}
	if (errno != ENOENT) {
		free(str);
		return(-1);
	}
	endptr = strrchr(str, '\0');
	ptr = endptr;
	slash = strrchr(str, '/');

	/* Search upward for the non-existing parent */

	/* The mode of each directory created is initially set */
	/* to 0777 so the subdirectories can be created - the */
	/* pathname of the parent is saved in savestr and then */
	/* the mode is changed to what the caller requested */
	/* after the subdirectory is created */

	savestr=(char *)malloc(strlen(str)+1);
	*savestr = '\0';
	while (slash != NULL) {

		ptr = slash;
		*ptr = '\0';

		/* If reached an existing parent, break */

		if (access(str, 00) ==0)
			break;

		/* If non-existing parent*/

		else {
			slash = strrchr(str,'/');

			/* If under / or current directory, make it. */

			if (slash  == NULL || slash== str) {
				if (mkdir(str, 0777)) {
					free(str);
					free(savestr);
					return(-1);
				}
				strcpy(savestr,str);
				break;
			}
		}
	}
	/* Create directories starting from upmost non-existing parent*/

	while ((ptr = strchr(str, '\0')) != endptr){
		*ptr = '/';
		if (mkdir(str, 0777)) {
			if ((*savestr != '\0') && (mode != 0777)) 
				chmod(savestr, mode);
			free(str);
			free(savestr);
			return(-1);
		}
		if ((*savestr != '\0') && (mode != 0777))
			chmod(savestr, mode);
		strcpy(savestr,str);
	}
	if ((*savestr != '\0') && (mode != 0777))
		chmod(savestr, mode);
	free(str);
	free(savestr);
	return(0);
}

static char *
compress(str)
char *str;
{

	char *tmp;
	char *front;

	tmp=(char *)malloc(strlen(str)+1);
	if ( tmp == NULL )
		return(NULL);
	front = tmp;
	while ( *str != '\0' ) {
		if ( *str == '/' ) {
			*tmp++ = *str++;
			while ( *str == '/' )
				str++;
		}
		*tmp++ = *str++;
	}
	*tmp = '\0';
	return(front);
}
