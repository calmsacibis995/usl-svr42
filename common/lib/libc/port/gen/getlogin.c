/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getlogin.c	1.17"
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getlogin = _getlogin
#endif
#include "synonyms.h"
#include <string.h>
#include <sys/types.h>
#include "utmp.h"
#include <unistd.h>

#define NULL 0

extern int open(), read(), close();


char *
getlogin()
{
	register uf;
	pid_t sid;
	struct utmp ubuf ;
	static char answer[sizeof(ubuf.ut_user)+1] ;

	if((uf = open((const char *)UTMP_FILE, 0)) < 0)
		return(NULL);

	if((sid = getsid(0)) < 0)
		return(NULL);

	while(read(uf, (char*)&ubuf, sizeof(ubuf)) == sizeof(ubuf)) {
		if(    (ubuf.ut_type == INIT_PROCESS ||
			ubuf.ut_type == LOGIN_PROCESS ||
			ubuf.ut_type == USER_PROCESS ||
			ubuf.ut_type == DEAD_PROCESS ) &&
			ubuf.ut_pid == sid) {
			(void) close(uf);
			goto found;
		}
	}
	(void) close(uf);
	return (NULL);

found:
	if(ubuf.ut_user[0] == '\0')
		return(NULL);
	strncpy(&answer[0],&ubuf.ut_user[0],sizeof(ubuf.ut_user)) ;
	answer[sizeof(ubuf.ut_user)] = '\0' ;
	return(&answer[0]);
}
