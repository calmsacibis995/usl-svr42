/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:i386/cmd/ttymon/tmmach.c	1.4"
#ident "$Header: tmmach.c 1.2 91/04/23 $"

/*
 *
 * remote_log(sev, msg, a1, [...], [a9])
 *
 * 	write a message to console
 *
 * Parameters:
 *
 *	sev: 		message state flag
 *	msg: 		message number
 *	a1:  		the message default string
 *	a2, ..., a9:	message parameters
 *
 * Return Values:
 *
 *  	remote_log() returns 0 if the message is written to either
 *	/dev/sysmsg or /dev/syscon or both, else 1.
 *
 */

#include 	<stdio.h>
#include 	"ttymon.h"
#include 	<sys/stat.h>
#include 	<sys/sysmsg.h>

int
remote_log(sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
int sev;
char *msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	register FILE *fp;
	int 	 write_to_syscon;
	struct 	 smsg_flags flags;
	struct 	 stat console_stat_buf, syscon_stat_buf, tty00_stat_buf;
	int 	 retval;	/* return value */

	/*
	 * if /dev/syscon and /dev/console are the same, 
	 * only write to /dev/sysmsg else if /dev/syscon and 
	 * /dev/tty00 are the same and both RCEF and
	 * RCMF are set, only write to /dev/sysmsg
	 * else write to both /dev/sysmsg and /dev/syscon
	 */

	write_to_syscon = retval = 1;
	(void)stat("/dev/console", &console_stat_buf);
	(void)stat(CONSOLE, &syscon_stat_buf);
	/* if these system calls fail, not sensible to do anything */

	if ((fp = fopen("/dev/sysmsg", "w")) != NULL)
	{
		retval = 0;
		(void) pfmt(fp, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', fp);
	}

	if (console_stat_buf.st_rdev == syscon_stat_buf.st_rdev)
		write_to_syscon = 0;
	else if (stat("/dev/tty00", &tty00_stat_buf) != -1)
	{
		/* get the remote console flags */
		if (ioctl(fileno(fp), SMSG_GETFLAGS, &flags) != -1) {
			if (syscon_stat_buf.st_rdev == tty00_stat_buf.st_rdev
				&& flags.dynamic_rcmf && flags.rcef)
				write_to_syscon = 0;
		}
	}

	(void) fclose(fp);
	if (write_to_syscon && (fp = fopen(CONSOLE, "w")) != NULL)
	{
		retval = 0;
		(void) pfmt(fp, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', fp);
		(void) fclose(fp);
	}
	return (retval);
}

