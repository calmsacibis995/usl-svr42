/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#ident	"@(#)proto-cmd:bootcntl.c	1.1"

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/bootcntl.h>

#define equal(a,b)      (strcmp(a, b) == 0)

char    *bootname;              /* pointer to boot file name 	*/
char	rfs[B_STRSIZ];		/* input parameter string	*/
char 	options_string[2] = "r:";
int	bootfd;

main(argc,argv)
int argc;
char *argv[];
{
        extern char     *optarg;
        extern int      optind;
        int     	c;
        int     	errflg = 0;
	struct		bootcntl hpbootcntl;
	int		i;
	int		cnt;
	int		bcsize;
	char		param[B_STRSIZ];
	char		*idxp;

        while ((c = getopt(argc, argv, options_string)) != -1) {
                switch (c) {
                case 'r':
			if ((int)strlen(optarg) >= B_STRSIZ) {
				fprintf(stderr,"Root file system string too long.\n");
				exit(15);
			}
			strcpy(rfs,optarg);
			break;
		case '?':
			errflg++;
		}
	}

        if (argc - optind < 1)
                ++errflg;
        if (errflg) {
                giveusage();
                exit(40);
        }

	bootname = argv[optind];
        if ((bootfd = open(bootname, O_RDWR)) == -1) {
		fprintf(stderr, "Unable to open specified boot program.\n");
		exit(10);
	}

/*	boot control block is at the second sector of the boot program	*/
	bcsize = sizeof(struct bootcntl);
	lseek(bootfd, 512, 0);
	if ((cnt = read(bootfd, (char *)&hpbootcntl, bcsize)) != bcsize) {
		fprintf(stderr,"Can't read boot control block.\n");
		exit(30);
	}

/*	check for high performance boot program				*/
	if (hpbootcntl.bc_magic != BPRG_MAGIC) {
		fprintf(stderr,"Invalid boot control block.\n");
		exit(20);
	}

/*	check for current entry for root file system parameter		*/
	for (i=0; i<hpbootcntl.bc_argc; i++) {	
		strncpy(param, (char *)(hpbootcntl.bc_argv[i]), B_STRSIZ);
		if ((idxp = strchr(param, '=')) == (char *)NULL)
			continue;
		*idxp++ = '\0';
		if (equal(param, "rootfs") || equal(param, "rootfstype"))
			break;
	}

/*	if root file system has not been defined, add the specified one	*/
	if (i >= hpbootcntl.bc_argc) {
		if (hpbootcntl.bc_argc == BC_MAXARGS) {
			fprintf(stderr,"Total number of parameter strings exceeds maximum.\n");
			exit(25);
		}
		hpbootcntl.bc_argc++;
	}
	strcpy((char *)(hpbootcntl.bc_argv[i]), rfs);

/*	update the boot control block					*/
	lseek(bootfd, 512, 0);
	if ((cnt = write(bootfd, (char *)&hpbootcntl, bcsize)) != bcsize) {
		fprintf(stderr,"Can't write boot control block.\n");
		exit(31);
	}
	close(bootfd);
}

giveusage()
{
        fprintf(stderr, "Usage: bootcntl -r root-file-system boot-program\n");
}
