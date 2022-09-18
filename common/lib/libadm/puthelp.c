/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/puthelp.c	1.1.6.2"
#ident  "$Header: puthelp.c 1.3 91/07/29 $"

#include <stdio.h>
#include <string.h>

extern int	ckwidth;
extern int	ckindent;
extern int	ckquit;
extern int	puttext();
extern void	*calloc(),
		free();

void
puthelp(fp, defmesg, help)
FILE *fp;
char *defmesg, *help;
{
	char	*tmp;
	int	n;

	tmp = NULL;
	if(help == NULL) {
		/* use default message since no help was provided */
		help = defmesg ? defmesg : "No help available.";
	} else if(defmesg != NULL) {
		n = strlen(help);
		if(help[0] == '~') {
			/* prepend default message */
			tmp = calloc(n+strlen(defmesg)+1, sizeof(char));
			(void) strcpy(tmp, defmesg);
			(void) strcat(tmp, "\n");
			(void) strcat(tmp, ++help);
			help = tmp;
		} else if(n && (help[n-1] == '~')) {
			/* append default message */
			tmp = calloc(n+strlen(defmesg), sizeof(char));
			(void) strcpy(tmp, help);
			tmp[n-1] = '\0';
			(void) strcat(tmp, "\n");
			(void) strcat(tmp, defmesg);
			help = tmp;
		}
	}
	(void) puttext(fp, help, ckindent, ckwidth);
	(void) fputc('\n', fp);
	if(tmp)
		free(tmp);
}
