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

#ident	"@(#)uadmin:uadmin.c	1.4.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/uadmin/uadmin.c,v 1.1 91/02/28 20:16:51 ccs Exp $"

/***************************************************************************
 * Command: uadmin
 *
 * Inheritable Privileges: P_SYSOPS
 *       Fixed Privileges: None
 *
 ***************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <priv.h>
#include <sys/uadmin.h>

char *Usage = "Usage: %s cmd fcn\n";
extern int errno;
int  isnumber(), isdigit();


/*
 * Procedure:     main
 *
 * Restrictions:
 *                uadmin(2): none
*/

main(argc, argv)
char *argv[];
{
	register cmd, fcn;
	sigset_t set, oset;
	int tmperr=0;

	if (argc != 3) {
		fprintf(stderr, Usage, argv[0]);
		exit(1);
	}

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oset);

        if (isnumber(argv[1]) && isnumber(argv[2])) {
                cmd = atoi(argv[1]);
                fcn = atoi(argv[2]);
        }
        else {
                fprintf(stderr, "%s: cmd and fcn must be integers\n",argv[0]);
                exit(1);
        }

	if (uadmin(cmd, fcn, 0) < 0) {
		tmperr=errno;
		perror("uadmin");
	}

	sigprocmask(SIG_BLOCK, &oset, (sigset_t *)0);
	exit(tmperr);

}


/*
 * Procedure:     isnumber
 *
 * Restrictions:  none
*/

isnumber(s)
char *s;
{
        register c;

        while(c = *s++)
                if(!isdigit(c))
                        return(0);
        return(1);
}
