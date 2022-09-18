/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)macinit:consalloc.c	1.3.2.2"
#ident  "$Header: consalloc.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <mac.h>
#include <errno.h>


extern errno;

/*
 * consalloc   - allocates  /dev/console securitystate
 *
 * Notes:
 *	1. this command is called as an inittab entry with action sysinit,
 *	   init 2 or init 3
 *	   If MAC is not installed, this command just returns with success.
 *	2. No special signal handling is performed.  If the administrator
 *	   chooses to bypass this command, from boot to sysinit the console will
 *         be in dev_private security state, if this command is not performed
 *         when going to init 2 from init 1, then the console state remains 
 *	   public, if the command was executed for sysinit.
 *	3. This command takes as argument the securtiy state of the device
 *	   1 for DEV_PRIVATE, 2 for DEV_PUBLIC
 *         feature is added
 *      4. This command needs to enforce least privilege. For now, it will run
 *	   with all privileges as inheritable.
 */
 
main(argc, argv)
	int	argc;
	char	*argv[];

{
	struct devstat mybuf;
	level_t lid_user;
	level_t lid_high;
	level_t lid_low;
	int error;

	/* consalloc is called as an inittab entry.  If MAC is not
	 * installed, exit successfully.
	 */

	if (lvlproc(MAC_GET, &lid_user) == -1 && errno == ENOPKG)
		exit(0);
	if (lvlin(SYS_RANGE_MAX, &lid_high) == -1) {
		perror("consalloc: LTDB is inacessible or corrupt");
		exit(1);
	}
	if (lvlin(SYS_PUBLIC, &lid_low) == -1) {
		perror("consalloc: LTDB is inacessible or corrupt");
		exit(1);
	}
	mybuf.dev_relflag= DEV_PERSISTENT;
	mybuf.dev_mode=DEV_STATIC;
	mybuf.dev_hilevel=lid_high;
	mybuf.dev_lolevel=lid_low;
	mybuf.dev_state=atoi(argv[1]);
	if (error = fdevstat(0,DEV_SET, &mybuf)){
		(void)fprintf(stderr, "consalloc:%s:devstat failed on %s\n", strerror(errno), ttyname(0));
		exit(1);
	}
	exit(0);
}
	

