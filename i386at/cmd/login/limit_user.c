/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)login:i386at/cmd/login/limit_user.c	1.4"

#include <values.h>
#include <sys/sysi86.h>
#include <stdio.h> 
#include <utmpx.h>
#include <string.h>
#include <sys/proc.h>

int	limit_user();
 
/*
 * Procedure:	limit_user()
 *
 *		Check the number of users on the system
 * 		and if the current number of users is
 * 		= or > the maximum limit, access is denied.
 *
 *		sysi86() 
 */ 
int
limit_user(pshell, ia_uid)
	char	*pshell;
	uid_t	ia_uid;
{ 	
	struct utmpx *tu;
	int 	uucpid = 0,
		users = 0,
		rootin = 0,
		login_limit;

	if ((login_limit = sysi86(SI86LIMUSER, EUA_GET_LIM)) > 0) {
		while ((tu = getutxent()) != NULL)
			if (tu->ut_type == USER_PROCESS) {
				if (strcmp(tu->ut_user, "root") == 0)
					rootin++;
			}
		endutxent();
		/*
		 * If root is logging in, utmp has root written in (on line 386).
		 * Therefore, subtract 1 from rootin if I'm root.
		 */
		if (ia_uid == 0)
			rootin--;

		users = sysi86(SI86LIMUSER, EUA_GET_CUR);
		/*
 		 * Only root can log into the system one time,
		 * once login_limit has been reached.  The uucico
		 * shell has un-unlimited login power.
		 */
		if (strcmp (pshell, "/usr/lib/uucp/uucico") == 0)
			uucpid++;

		if ((users >= login_limit) && !uucpid) {
			if ((ia_uid != 0) ||
				((ia_uid == 0) && rootin)) {

				return login_limit;
			}
		}
	}

	(void) sysi86(SI86LIMUSER, (EUA_ADD_USER + uucpid));

	/* successful enable */

	return 0;
}
