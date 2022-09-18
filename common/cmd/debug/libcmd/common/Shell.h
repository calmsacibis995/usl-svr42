/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libcmd/common/Shell.h	1.2"

#ifndef	Shell_h
#define	Shell_h

#include <sys/types.h>

class PtyInfo;

// Invoke the shell to run a UNIX system command
extern	pid_t	Shell( char * cmd, int redir, PtyInfo *& );

// for termination message ---
extern	char *	sig_message[];

#endif	/* Shell_h */
