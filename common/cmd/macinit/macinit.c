/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)macinit:macinit.c	1.13.2.2"
#ident  "$Header: macinit.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/secsys.h>
#include <sys/uadmin.h>
#include <stdio.h>
#include <errno.h>
#include <mac.h>

#define	SU		"/etc/sulogin"

/*
 * Command: macinit	- complete MAC initialization
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_SYSOPS,P_MOUNT
 *       Fixed Privileges: None
 *
 * Notes:
 *	1. This command is called by /sbin/bcheckrc.
 *	   It is included in the base system.  If MAC is not installed,
 *	   this command just returns with success.
 *	2. No special signal handling is performed.  If the administrator
 *	   chooses to bypass this command, the MAC policy will be most
 *	   restrictive, i.e., MAC access is only granted through MAC
 *	   override privileges.
 */


/***************************************************************************
 * Command: macinit
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_SYSOPS,P_MOUNT
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

/*
 * Procedure:     main
 *
 * Restrictions:
                 devalloc: None
                 lvlin: None
                 secsys(2): None
                 lvlvalid: None
                 lvlvfs(2): None
*/

main(argc, argv)
	int	argc;
	char	*argv[];
{
	void	administer();
	void 	mac_rootdev();
	level_t	lid_user;
	level_t	lid_adt;
	level_t	lid_priv;
	level_t lid_max;
	level_t lid_pub;
	char	*lid_file = LTF_LID;
	struct dev_alloca	rootddb; /* contains allocation info for root dev */

	extern int devalloc(); /* devalloc function */
	/*
	 * macinit is called as an inittab entry.  If MAC is not
	 * installed, exit successfully.
	 */
	if (lvlproc(MAC_GET, &lid_user) == -1  &&  errno == ENOPKG) {
		if (lvlin(SYS_RANGE_MAX, &lid_max) == -1)
			exit(0);
		else {
			if (secsys(ES_MACSYSLID, (caddr_t)&lid_max) == -1)
				administer("cannot assign SYS_RANGE_MAX to system processes");
			exit(0);
		}
	}


	if (lvlin(SYS_PRIVATE, &lid_priv) == -1)
		administer("LTDB is inaccessible or corrupt");

	if (lvlin(SYS_AUDIT, &lid_adt) == -1)
		administer("LTDB is inaccessible or corrupt");

	if (lvlin(SYS_RANGE_MAX, &lid_max) == -1)
		administer("LTDB is inaccessible or corrupt");

	if (lvlin(SYS_PUBLIC, &lid_pub) == -1)
		administer("LTDB is inaccessible or corrupt");

	if (secsys(ES_MACOPENLID, (caddr_t)lid_file) == -1)
		administer("cannot open LID file for kernel use");

	if (secsys(ES_MACSYSLID, (caddr_t)&lid_priv) == -1)
		administer("cannot assign SYS_PRIVATE to system processes");

	if (secsys(ES_MACADTLID, (caddr_t)&lid_adt) == -1)
		administer("cannot assign SYS_AUDIT to audit daemon");

	if (devalloc("/dev/root", MAC_GET, &rootddb) == -1) {
		fprintf(stderr,"\nmacinit: cannot get level range for root device /dev/root from Device Database\n");
		mac_rootdev(&lid_max, &lid_pub);
	}
	
	if (lvlvalid(&rootddb.hilevel) != 0) {
		fprintf(stderr," \nmacinit: high level from Device Database for root device  /dev/root is invalid");
		mac_rootdev(&lid_max, &lid_pub);
	}

	if (secsys(ES_MACROOTLID, (caddr_t)&lid_pub) == -1)
		administer("cannot assign level to root vnode of root fs");

	/* set level ceiling of root file system, expects fs to support labeling */
	if (lvlvfs("/", MAC_SET, &rootddb.hilevel) == -1)
		administer(" cannot set level ceiling of root file system");
	
	exit(0);
}

/*
 * Procedure:     administer
 *
 * Restrictions:
                 uadmin(2): None
	 Notes:
 * administer	- provide a mechanism to administer the system
 */
void
administer(comment)
	char	*comment;
{

	fprintf(stderr, "\nmacinit: %s\n", comment);
	fprintf(stderr, "macinit: RETURN TO FIRMWARE MODE\n");
	fprintf(stderr, "macinit: reboot with mUNIX to fix the problem\n");
	uadmin(A_SHUTDOWN, AD_IBOOT, 0);
	fprintf(stderr, "macinit: now we're in real trouble\n");
	exit(1);
		
}

/*
 * Procedure:     mac_rootdev
 *
 * Restrictions:
                 secsys(2): None
                 lvlvfs(2): None

 * Notes: initialize root file system if ddb cannot be found 
*/
void 
mac_rootdev(hilevelp, lolevelp)
	level_t *hilevelp;
	level_t *lolevelp;

{
	if (secsys(ES_MACROOTLID, (caddr_t)lolevelp) == -1)
		administer("cannot assign level to root vnode of root fs");

	/* set level ceiling of root file system, expects fs to support labeling */
	if (lvlvfs("/", MAC_SET, hilevelp) == -1)
		administer(" cannot set level ceiling of root file system");

	admin_shell();
	exit(0);
	

}


/*
 * Procedure:     admin_shell
 *
 * Restrictions:
                 fork(2): None
                 execlp: None
                 uadmin(2): None
*/

admin_shell()

{
	int	rfork;
	int	status;
	int	retval;


	if ((rfork = fork()) == -1) {
		return;
	}
	
	if (rfork == 0) {
		execlp(SU, SU, "-", 0);
		fprintf(stderr, "macinit: sulogin failed\n");
		exit(1);
	}

	/* parent */

	while (((retval = wait(&status)) != rfork) && retval != -1)
		;

	fprintf(stderr, "macinit: return to firmware mode\n");
	uadmin(A_SHUTDOWN, AD_IBOOT, 0);
	fprintf(stderr, "macinit: now we're in real trouble\n");
	exit(1);
		
}
