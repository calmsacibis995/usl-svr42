#ifndef _IO_IAF_IAF_H	/* wrapper symbol for kernel use */
#define _IO_IAF_IAF_H	/* subject to change without notice */
#ident	"@(#)uts-comm:io/iaf/iaf.h	1.2.2.2"
#ident	"$Header: $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	IAF structures and constants				*/

struct iaf {			/* used to pass AVAs to the module.	*/
	int count;		/* number of strings			*/
	int size;		/* length of strings (including NULLs)	*/
	char data[1];		/* arbitrary character array		*/
};

#define IAFMOD	"iaf"
#define SETAVA	(('i' << 8) | 1)
#define GETAVA	(('i' << 8) | 2)

#endif	/* _IO_IAF_IAF_H */
