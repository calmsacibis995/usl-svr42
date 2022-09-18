/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.ftpd/logwtmp.c	1.2.8.1"
#ident  "$Header: logwtmp.c 1.2 91/06/26 $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <utmp.h>
#ifdef SYSV
#include <utmpx.h>
#include <sac.h>        /* for SC_WILDC */
#endif /* SYSV */
#include <fcntl.h>

static int fd;

#ifdef SYSV
logwtmp(line, name, host)
	char *line, *name, *host;
{
	struct utmpx ut;

	memset ((char *) &ut, (char) 0, sizeof(ut));
	(void) strncpy(ut.ut_user, name, sizeof(ut.ut_user));
	ut.ut_id[0] = 'f';
	ut.ut_id[1] = 't';
	ut.ut_id[2] = 'p';
	ut.ut_id[3] = SC_WILDC;
	(void) strncpy(ut.ut_line, line, sizeof(ut.ut_line));
	ut.ut_pid = getpid();
	ut.ut_type = USER_PROCESS;
	ut.ut_exit.e_termination = 0;
	ut.ut_exit.e_exit = 0;
	(void) time (&ut.ut_tv.tv_sec);
	(void)strncpy(ut.ut_host, host, sizeof(ut.ut_host));	
	ut.ut_syslen = strlen(host);
	(void) updwtmpx(WTMPX_FILE, &ut);
}
#else
logwtmp(line, name, host)
	char *line, *name, *host;
{
	struct utmp ut;
	struct stat buf;
	time_t time();
	char *strncpy();

	if (!fd && (fd = open(WTMP_FILE, O_WRONLY|O_APPEND, 0)) < 0)
		return;
	if (!fstat(fd, &buf)) {
		(void)strncpy(ut.ut_line, line, sizeof(ut.ut_line));
		(void)strncpy(ut.ut_name, name, sizeof(ut.ut_name));
		(void)strncpy(ut.ut_host, host, sizeof(ut.ut_host));
		(void)time(&ut.ut_time);
		if (write(fd, (char *)&ut, sizeof(struct utmp)) !=
		    sizeof(struct utmp))
			(void)ftruncate(fd, buf.st_size);
	}
}
#endif /* SYSV */
