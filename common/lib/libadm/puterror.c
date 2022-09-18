/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/puterror.c	1.1.7.3"
#ident  "$Header: puterror.c 1.4 91/07/29 $"

#include <stdio.h>
#include <string.h>

extern int	ckwidth;
extern int	ckindent;
extern int	ckquit;
extern int	puttext();
extern void	*calloc(),
		free();

#define DEFMSG	"ERROR: "
#define MS	sizeof(DEFMSG)

void
puterror(fp, defmesg, error)
FILE *fp;
char *defmesg, *error;
{
	char	*tmp=NULL;
	int	n;
	
	if(error == NULL) {
		/* use default message since no error was provided */
		tmp = calloc(MS+strlen(defmesg)+1, sizeof(char));
		(void) strcpy(tmp, DEFMSG);
		(void) strcat(tmp, defmesg ? defmesg : "invalid input");
	} else if(defmesg != NULL) {
		n = strlen(error);
		if(error[0] == '~') {
			/* prepend default message */
			tmp = calloc(MS+n+strlen(defmesg)+1, sizeof(char));
			(void) strcpy(tmp, DEFMSG);
			(void) strcat(tmp, defmesg);
			(void) strcat(tmp, "\n");
			(void) strcat(tmp, ++error);
		} else if(n && (error[n-1] == '~')) {
			/* append default message */
			tmp = calloc(MS+n+strlen(defmesg), sizeof(char));
			(void) strcpy(tmp, DEFMSG);
			(void) strcat(tmp, error);
			tmp[(MS-1)+(n-1)] = '\0';
			(void) strcat(tmp, "\n");
			(void) strcat(tmp, defmesg);
		} else {
			tmp = calloc(MS+n+1, sizeof(char));
			(void) strcpy(tmp, DEFMSG);
			(void) strcat(tmp, error);
		}
	}

	if (tmp){	/* <--just in case (error != NULL && defmesg == NULL) */
		(void) puttext(fp, tmp, ckindent, ckwidth);
		(void) fputc('\n', fp);
		free(tmp);
	}
}
