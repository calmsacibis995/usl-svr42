/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tfm:err.h	1.2.2.2"
#ident  "$Header: err.h 1.2 91/06/27 $"
#ifndef ERR
#define	ERR	1	/*Flag to say we have been here*/

#define	ERR_NONE	-1
#define	ERR_WARN	MM_WARNING
#define	ERR_ERR		MM_ERROR

#define	ERR_UNKNOWN	0
#define	ERR_CONTINUE	1
#define	ERR_QUIT	2

#define	ERRMAX	1024

struct	msg {
	int	sev;
	int	act;
	char	text[ERRMAX];
	char	args[4][1024];
};

#endif
