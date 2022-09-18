/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*      Portions Copyright (c) 1988, Sun Microsystems, Inc.     */
/*      All Rights Reserved.                                    */

#ident	"@(#)id:id.c	1.8.1.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/id/id.c,v 1.1 91/02/28 17:34:20 ccs Exp $"
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include <pfmt.h>
#include <locale.h>

main(argc, argv)
int argc;
char **argv;
{
	uid_t uid, euid;
	gid_t gid, egid;
	static char stdbuf[BUFSIZ];
	int c, aflag=0;
	
	setlocale(LC_ALL, "");
	setcat("uxcore.abi");
	setlabel("UX:id");
	
	while ((c = getopt(argc, argv, "a")) != EOF) {
		switch(c) {
			case 'a': 
				aflag++;
				break;
			default: 

				pfmt(stderr,MM_ERROR, ":1013:Usage: id [-a]\n");
				exit(1);
		}
	}
	setbuf (stdout, stdbuf);

	uid = getuid();
	gid = getgid();
	euid = geteuid();
	egid = getegid();

	puid ("uid", uid);
	pgid (" gid", gid);
	if (uid != euid)
		puid (" euid", euid);
	if (gid != egid)
		pgid (" egid", egid);
	if (aflag)
		pgroups ();
	putchar ('\n');

	exit(0);
}

puid (s, id)
	char *s;
	uid_t id;
{
	struct passwd *pw;

	printf ("%s=%ld", s, id);
	setpwent();
	pw = getpwuid(id);
	if (pw)
		printf ("(%s)", pw->pw_name);
}

pgid (s, id)
	char *s;
	gid_t id;
{
	struct group *gr;

	printf ("%s=%ld", s, id);
	setgrent();
	gr = getgrgid(id);
	if (gr)
		printf ("(%s)", gr->gr_name);
}

pgroups ()
{
	gid_t groupids[NGROUPS_UMAX];
	gid_t *idp;
	struct group *gr;
	int i;

	i = getgroups(NGROUPS_UMAX, groupids);
	if (i > 0) {
		pfmt (stdout, MM_NOSTD, ":1014: groups=");
		for (idp = groupids; i--; idp++) {
			printf ("%ld", *idp);
			setgrent();
			gr = getgrgid(*idp);
			if (gr)
				printf ("(%s)", gr->gr_name);
			if (i)
				putchar (',');
		}
	}
}
