/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:temp.c	1.7.2.5"
#ident "@(#)temp.c	1.13 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"
#include <pwd.h>
#ifdef preSVr4
extern struct passwd *getpwnam();
extern struct passwd *getpwuid();
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Give names to all the temporary files that we will need.
 */

void
tinit()
{
	register pid_t pid = mypid;
	struct passwd *pwd;

	sprintf(tempMail, "/tmp/Rs%-ld", (long)pid);
	sprintf(tempQuit, "/tmp/Rm%-ld", (long)pid);
	sprintf(tempEdit, "/tmp/Re%-ld", (long)pid);
	sprintf(tempSet, "/tmp/Rx%-ld", (long)pid);
	sprintf(tempMesg, "/tmp/Rx%-ld", (long)pid);
	sprintf(tempZedit, "/tmp/Rz%-ld", (long)pid);

	/* get the name associated with this uid */
	pwd = getpwuid(uid = myruid);
	if (!pwd) {
		copy("ubluit", myname);
		if (rcvmode) {
			pfmt(stdout, MM_ERROR, ":336:Who are you!?\n");
			exit(1);
		}
	}
	else
		copy(pwd->pw_name, myname);
	endpwent();
	lockname = myname;

	mylocalname = pcalloc(strlen(myname) + strlen(host) + 2);
	mydomname = pcalloc(strlen(myname) + strlen(domain) + 2);
	sprintf(mylocalname, "%s@%s", myname, host);
	sprintf(mydomname, "%s@%s", myname, domain);

	strcpy(homedir, Getf("HOME"));
	findmail();
	assign("MBOX", Getf("MBOX"));
	assign("MAILRC", Getf("MAILRC"));
	assign("DEAD", Getf("DEAD"));
	assign("save", "");
	assign("asksub", "");
	assign("header", "");
	assign("newmail", "");
	assign("from", "");
	assign("date", "");
	assign("editheaders", "");
}
