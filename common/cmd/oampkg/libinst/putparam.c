/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/putparam.c	1.2.8.3"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>

#define ERR_MEMORY	"memory allocation failure, errno=%d"

extern char	**environ;
extern int	errno;

extern void	*calloc(), 
		*realloc();
extern void	progerr(),
		quit(),
		free();

#define MALSIZ	64

void
putparam(param, tmp_value)
char	*param;
char	*tmp_value;
{
	char	*pt, *value;
	int	i, n;

	/* this code is added so that the value for basedir is not
	 * overwritten when freeing up space
	 */
	if((value = (char *)malloc(strlen(tmp_value)+1)) == (char *)NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}
	(void) strcpy(value, tmp_value);
	
	if(environ == NULL) {
		environ = (char **) calloc(MALSIZ, sizeof(char *));
		if(environ == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
	}
	n = strlen(param);
	for(i=0; environ[i]; i++) {
		if(!strncmp(environ[i], param, n) && (environ[i][n] == '=')) {
			free(environ[i]);
			break;
		}
	}

	if(value)
		pt = (char *) calloc(strlen(param)+strlen(value)+2, sizeof(char));
	else
		pt = (char *) calloc(strlen(param)+2, sizeof(char));
	if(pt == NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}
	if(value)
		(void) sprintf(pt, "%s=%s", param, value);
	else
		(void) sprintf(pt, "%s=", param);
	if(environ[i]) {
		environ[i] = pt;
		return;
	}

	environ[i++] = pt;
	if((i % MALSIZ) == 0) {
		environ = (char **) realloc((void *)environ, 
			(i+MALSIZ)*sizeof(char *));
		if(environ == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(1);
		}
	}
	environ[i] = (char *)NULL;
}

