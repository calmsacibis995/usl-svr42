/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:floppy/RemoteFilt.c	1.2"
#endif

#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <priv.h>
#include <sys/stat.h>

/* RemoteFilt: This module filters a set of file names created as part of 
 * the backup process.  The resulting legal files are fed to cpio for backup.
 * 1) Lines with the string "BLOCKS=" are removed leaving only file names.
 * 2) Files are checked to see if they are remote.  If they are, privileges
 *    are turned off before checking whether a file is readable.  This is
 *    to make sure we don't accidentally (as root or machine owner) "pass"
 *    unreadable files to cpio (which will choke on them).
 */

main()
{
	char	linebuf[PATH_MAX];
	char	*bp;
	int	cond = 0;
	int	remote;
	struct stat statb;

	for(;;) {
		remote = 0;
		bp = linebuf;
		if (! fgets(bp, PATH_MAX, stdin)) {
			if (!feof(stdin))
				cond = 1;
			break;
		}
		if (*(bp+strlen(bp)-1) != '\n') {
			cond = 1;
			break;
		}

		*(bp + strlen((char *)bp) -1) = (char) 0; /* delete newline */

		if (!strlen(bp))	/* skip empty lines (if any) */
			continue;

		if (strstr(bp,"BLOCKS=") != NULL)	/* skip lines with no file name */
			continue;

		if (stat(bp, &statb) < 0) {
			continue;
		}

		if ((long)statb.st_dev < 0) {
			(void)procprivl(CLRPRV, pm_work(P_DACREAD),
					pm_work(P_DACWRITE), (priv_t)0);
			remote = 1;
		}
		if (access(bp, EFF_ONLY_OK | R_OK) == 0)
			fprintf(stdout, "%s\n", bp);
		if (remote == 1) {
			(void)procprivl(SETPRV, pm_work(P_DACREAD),
					pm_work(P_DACWRITE), (priv_t)0);
		}
	}
	exit(cond);
}

