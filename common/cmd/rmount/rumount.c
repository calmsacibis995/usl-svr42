/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
	rumount -- dequeue a mount request

	usage: umount resource ...

	exit codes:
		0  one or more resources were removed from the queue
		1: no resources were removed from the queue
		2: syntax or functional error
*/

#ident	"@(#)rmount:rumount.c	1.1.11.3"
#ident  "$Header: rumount.c 1.2 91/06/27 $"
#include <sys/types.h>
#include <nserve.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mnttab.h>
#include <unistd.h>
#include <string.h>
#include "rmount.h"
#include "rmnttab.h"

char	*cmd;

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	int		chg = 0;	/* flag if rmnttab is altered */
	int		i;		/* argv index */
	char		*fqn();		/* fully qualified names */
	char		fdev[MAXDNAME];	/* fully qualified names */
	char		fres[MAXDNAME];	/* fully qualified names */
	struct rmnttab	rmtab;
/*	struct stat	stbuf; */ /* <= never initialized */
	FILE		*tfp;
	int		ret;
	int		tflag = 1;	/* the flag to write on temp file */
	FILE		*rp;
	struct stat	stbuf;

	cmd = argv[0];			/* set command name for diagnostics */

#ifdef   OLDSEC
	if (geteuid() != 0) {
		Fprintf(stderr, "%s: must be super-user\n", cmd);
		return 2;
	}
#endif /*OLDSEC*/

	if (argc < 2) {
		Fprintf(stderr, "usage: %s resource ...\n", cmd);
		return 2;
	}

	lock();
	switch(ret = rd_rmnttab(&rp, &stbuf)) {
		case 1:   /* rmnttab does not exist */
			unlock();
			Fprintf(stderr, "%s: resources not queued:", cmd);
			for (i=1; i < argc; ++i)
				if (*argv[i])
					Fprintf(stderr, " %s", argv[i]);
			Fputc('\n', stderr);
			return ret;
		case 2:
			unlock();
			return ret;
		case 0:
			break;
		default:
			unlock();
			Fprintf(stderr, "%s: error in reading rmnttab\n", cmd);
			return 0;
	}

	if ((tfp = fopen(TMPRMNT, "w")) == NULL) {
		perror(cmd);
		unlock();
		Fprintf(stderr, "%s: cannot creat temp rmnttab\n", cmd);
		return 2;
	}

	/* run through the table, looking for the entries */
	while (getrmntent(rp, &rmtab) == 0) {
		(void)fqn(rmtab.rmnt_special, fdev);
		for (i = 1; i < argc; i++)
			if (*argv[i] && !strcmp(fdev, fqn(argv[i], fres))) {
				*argv[i] = '\0';
				chg++;
				tflag=0;
			}
		if (tflag) 
			putrmntent(tfp, &rmtab);
		tflag = 1;
	}


	/* some resources may be not queued. If so, list them out */
	if (chg != argc - 1) {
		Fprintf(stderr, "%s: resources not queued:", cmd);
		for (i=1; i < argc; ++i)
			if (*argv[i])
				Fprintf(stderr, " %s", argv[i]);
		Fputc('\n', stderr);
	}
	Fclose(tfp);
	Fclose(rp);

	/* if all resources are not queued, don't rewrite the file */
	if (!chg) {
		Unlink(TMPRMNT);
		unlock();
		return 1;
	}
		/* ``write'' rmnttab */
	else {
		Rename(TMPRMNT, RMNTTAB);
		Chmod (RMNTTAB, MASK);
		Chown (RMNTTAB, stbuf.st_uid, stbuf.st_gid);
		unlock();
		return 0;
	}
}
