/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rmdir:rmdir.c	1.12.2.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/rmdir/rmdir.c,v 1.1 91/02/28 19:29:02 ccs Exp $"
/*
** Rmdir(1) removes directory.
** If -p option is used, rmdir(1) tries to remove the directory
** and it's parent directories.  It exits with code 0 if the WHOLE
** given path is removed and 2 if part of path remains.  
** Results are printed except when -s is used.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <pfmt.h>

extern int opterr, optind, rmdirp();
extern char *optarg;

static const char nonempty[] = "Directory not empty";
static const char nonemptyid[] = "uxsyserr:96";

main(argc,argv)
int argc;
char **argv;
{

	int c, pflag, sflag, errflg, rc, local_errno=0;
        char *ptr, *remain, *msg, *path;
	unsigned int pathlen;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:rmdir");

	pflag = sflag = 0;
	errflg = 0;
	/* set effective uid, euid, to be same as real	*/
	/* uid, ruid.  Rmdir(2) checks search & write	*/
	/* permissions for euid, but for compatibility	*/
	/* the check must be done using ruid.		*/
	if(setuid(getuid()) == -1) {
		pfmt(stderr, MM_ERROR, ":441:setuid() failed: %s\n",
			strerror(errno));
		exit(1);
	}

	while ((c = getopt(argc, argv, "ps")) != EOF)
		switch (c) {
			case 'p':
				pflag++;
				break;
			case 's':
				sflag++;
				break;
			case '?':
				errflg++;
				break;
		}
        if(argc < 2 || errflg) {
        	if (!errflg)
        		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
                pfmt(stderr, MM_ACTION, ":442:Usage: rmdir [-ps] dirname ...\n");
                exit(2);
        }
	argc -= optind;
	argv = &argv[optind];
        while (argc--) {
		ptr = *argv++;
  					/* -p option. Remove directory and parents.
					** Prints results of removing */
		if (pflag) {
			pathlen = (unsigned)strlen(ptr);
			if ((path = (char *)malloc(pathlen + 4)) == NULL ||
			    (remain = (char *)malloc(pathlen + 4)) == NULL) {
				pfmt(stderr, MM_ERROR, ":312:Out of memory: %s\n",
					strerror(errno));
				exit(2);
			}
			strcpy(path,ptr);

				/* rmdirp() removes directory and parents */
				/* rc != 0 implies only part of path removed */

			if ((rc = rmdirp(path, remain)) == 0) {
				if (!sflag) 
					pfmt(stdout, MM_INFO,
					":443:%s: Whole path removed.\n",ptr);
			}
			else {
				if (!sflag) {
					local_errno = errno;
					switch (rc) {
					case -1:
						if (local_errno == EEXIST)
							msg=gettxt(nonemptyid, nonempty);
						else
							msg = strerror(local_errno);
						break;
					case -2:
						msg=gettxt(":444", "Cannot remove . or ..");
						break;
					case -3:
						msg=gettxt(":445", "Cannot remove current directory");
						break;
					}
					pfmt(stderr, MM_ERROR,
						":446:%s: %s not removed: %s\n",ptr, remain, msg); 
				}
			}
			free(path);
			free(remain);
			continue;
		}

			/* No -p option. Remove only one directory */

		if (rmdir(ptr) == -1) {
			local_errno = errno;
			switch(local_errno) {
			case EEXIST:	msg = gettxt(nonemptyid, nonempty);
					break;
			case ENOTDIR:	msg = gettxt(":447", "Path component not a directory");
					break;
			case ENOENT:	msg = gettxt(":448", "Directory does not exist");
					break;
			case EACCES:	msg = gettxt(":449", "Search or write permission needed");
					break;
			case EBUSY:	msg = gettxt(":450", "Directory is a mount point or in use");
					break;
			case EROFS:	msg = gettxt("uxsyserr:33", "Read-only file system");
					break;
			case EIO:	msg = gettxt(":451", "I/O error accessing file system");
					break;
			case EINVAL:	msg = gettxt(":452", "Cannot remove current directory or ..");
					break;
			case EFAULT:
			default:	msg = strerror(errno);
					break;
			}
			pfmt(stderr, MM_ERROR, ":12:%s: %s\n", ptr, msg);
		continue;
		}
        }
        exit(local_errno);
}
