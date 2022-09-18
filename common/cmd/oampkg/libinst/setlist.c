/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/setlist.c	1.3.7.2"
#ident  "$Header: setlist.c 1.2 91/06/27 $"

#include <stdio.h>
#include <string.h>

extern int	errno;
extern void	*calloc(), 
		*realloc();
extern void	progerr(),
		quit();

#define MALSIZ	64
#define ERR_MEMORY	"memory allocation failure, errno=%d"

int
setlist(plist, value)
char ***plist;
char *value;
{
	char	**list;
	char	*pt;
	int	n;

	list = (char **) calloc(MALSIZ, sizeof(char *));
	if(list == NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}

	n = 0;
	pt = strtok(value, " \t\n");
	while(pt) {
		list[n] = pt;
		if((n++ % MALSIZ) == 0) {
			list = (char **) realloc((void *)list, 
				(n+MALSIZ)*sizeof(char *));
			if(list == NULL) {
				progerr(ERR_MEMORY, errno);
				quit(99);
			}
			list[n] = (char *)NULL;
		}
		pt = strtok(NULL, " \t\n");
	}
	*plist = list;
	return(n);
}
