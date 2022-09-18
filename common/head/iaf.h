/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1989 AT&T */
/*	  All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IAF_H
#define _IAF_H

#ident	"@(#)head.usr:iaf.h	1.1.2.2"
#ident  "$Header: iaf.h 1.3 91/06/21 $"
/*
 *	IAF function declarations
 */

#include <sys/iaf.h>

int    invoke(int, char *);	/* invoke the named scheme/command	*/

char  *getava(char *, char **);	/* local interrogation of the AVA list	*/
char **putava(char *, char **);	/* local manipulation of the AVA list	*/

char **retava(int);		/* retrieves AVA list from module on fd	*/
int    setava(int, char **);	/* sends AVA list to module on fd	*/

char **strtoargv(char *);	/* changes cmd string to argv type list	*/
char  *argvtostr(char **);	/* changes argv type list to cmd string	*/

/* moved from ia.h	*/
int	set_id(char *);		/* set the user identity 	*/
int	set_env(void);		/* set user env.		*/

#define IAFDIR "/usr/lib/iaf/"	/* where the schemes live	*/
#define AVASIZ 1024		/* default size for AVA list	*/
#define NULL 0

#endif	/* _IAF_H */
