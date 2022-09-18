/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/addgrpmem.c	1.6.16.2"
#ident  "$Header: addgrpmem.c 2.0 91/07/13 $"

/***************************************************************************
 *
 * Command:	addgrpmem
 *
 * Usage:	addgrpmem -g group login [login ...]
 *
 * Inheritable Privileges:	P_MACWRITE,P_SETFLEVEL,P_AUDIT,P_DACWRITE
 *       Fixed Privileges:	None
 *
 * Level:	SYS_PRIVATE
 *
 * Notes:	This command adds logins to the supplementary membership for
 *		the group given.
 *
 *		group - a string of printable characters excluding colon(:)
 *			and less than MAXGLEN characters long.
 *
 ***************************************************************************/

/* LINTLIBRARY */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<limits.h>
#include	<string.h>
#include	<grp.h>
#include	<pwd.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	"users.h"
#include	<userdefs.h>
#include	<audit.h>
#include	<priv.h>
#include	<mac.h>


extern	char	*optarg,
		*argvtostr();

extern	int	optind,
		getopt(),
		unlink(),
		rename();

extern	void	exit();

extern	struct	group	*getgrnam(),
			*getgrgid(),
			*fgetgrent();

extern	struct	passwd	*getpwnam();

extern	void	putgrent(),
		adumprec();

static	char	cmdline[BUFSIZ],
		*cmdname = "addgrpmem",
		*usage = "usage: addgrpmem -g group login [login ...]\n";

/*
 * Procedure:	 main
 *
 * Restrictions:
 *               printf:	none
 *               getopt:	none
 *               getgrnam:	none
 *               stat(2):	none
 *               fopen:		none
 *               sprintf:	none
 *               fgetgrent:	none
 *               getpwnam:	none
 *               fclose:	none
 *               unlink(2):	none
 *               lvlfile(2):	none
 *               rename(2):	none
 *               chmod(2):	none
 *               chown(2):	none
*/

main(argc, argv)
	int	argc;
	char	*argv[];
{
	register gid_t gid;
	register i, modified = 0, ch;
	char *grpname = NULL, *tname, **memptr, *t_suffix = ".tmp";
	FILE *e_fptr, *t_fptr;
	struct group *grpstruct;	/* group structure from fgetgrent */
	struct stat statbuf;
	char *argvp;
	
	/* save command line arguments */
	if ((argvp = argvtostr(argv)) == NULL) {
                (void) printf("failed argvtostr\n");
		adumprec(ADT_ADD_USR_GRP,1,strlen(cmdname),cmdname);
                exit(1);
        }
	(void) strcat(cmdline,argvp);

	while ((ch = getopt(argc, argv, "g:")) != EOF)
		switch(ch) {
			case 'g':
				grpname = optarg;
				break;
			case '?':
				(void) printf(usage);
				adumprec(ADT_ADD_USR_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
				exit(EX_SYNTAX);
		}

	if (!grpname || argc < 4) {
		(void) fprintf(stdout, usage);
		adumprec(ADT_ADD_USR_GRP, EX_SYNTAX, strlen(cmdline), cmdline);
		exit(EX_SYNTAX);
	}


	if ((grpstruct = getgrnam(grpname)) == NULL) {
		(void) fprintf(stdout, "invalid group: %s\n", grpname);
		adumprec(ADT_ADD_USR_GRP,EX_NAME_EXISTS,strlen(cmdline),cmdline);
		exit(EX_NAME_EXISTS);
	}

	gid = grpstruct->gr_gid;

	/* 
	 * get owner grp and mac level of /etc/group
	*/

        if (stat(GROUP, &statbuf) < 0) {
		(void) fprintf(stdout, "unable to stat /etc/group\n");
		exit(EX_UPDATE);
	}

	if ((e_fptr = fopen(GROUP, "r")) == NULL) {
		(void) fprintf(stdout, "unable to open /etc/group\n");
		adumprec(ADT_ADD_USR_GRP,EX_UPDATE,strlen(cmdline),cmdline);
		exit(EX_UPDATE);
	}

	tname = (char *) malloc(strlen(GROUP) + strlen(t_suffix) + 1);

	(void) sprintf(tname, "%s%s", GROUP, t_suffix);

	/* See if temp file exists before continuing */
	if (access(tname, F_OK) == 0)
		exit(EX_UPDATE);

	if ((t_fptr = fopen(tname, "w+")) == NULL) {
		(void) printf("unable to open tmp file needed to modify /etc/group\n");
		adumprec(ADT_ADD_USR_GRP,EX_UPDATE,strlen(cmdline),cmdline);
		exit(EX_UPDATE);
	}

	/* Look for groups matching this gid */
	errno = 0;
	while ((grpstruct = fgetgrent(e_fptr)) != NULL) {
		if (grpstruct->gr_gid == gid) {
			/*
			 * now go through & add logins from command line
			*/
			memptr = grpstruct->gr_mem;
			for (i = optind; i < argc; i++) {
				if (getpwnam(argv[i]) != NULL) {
					/* valid login */
					*memptr++ = argv[i];
					modified++;

				}
				else {
					(void) printf("%s is not a valid login\n",
						argv[i]);
				}
			}
			*memptr = NULL;
		}
		putgrent(grpstruct, t_fptr);
	}

	(void) fclose(e_fptr);
	(void) fclose(t_fptr);

	if (errno == EINVAL) {
		(void) unlink(tname);
		(void) fprintf(stdout,
			"/etc/group contains bad entries -- it was not modified.\n" );
		adumprec(ADT_ADD_USR_GRP, EX_UPDATE, strlen(cmdline), cmdline);
		exit(EX_UPDATE);
	}
	/*
	 * Update GROUP file, if needed
	*/
	if (modified) {
		/*
		 * Set attributes of temporary group file
		*/
		(void) lvlfile(tname, MAC_SET, &statbuf.st_level);
		(void) chmod(tname, statbuf.st_mode);
		(void) chown(tname, statbuf.st_uid, statbuf.st_gid);
		/*
		 * try to rename the temporary file to "/etc/group".
		*/
		if (rename(tname, GROUP) < 0) {
			(void) unlink(tname);
			(void) printf("problem renaming temporary file\n");
			adumprec(ADT_ADD_USR_GRP,EX_UPDATE,strlen(cmdline),cmdline);
			exit(EX_UPDATE);
		}
	}
	(void) unlink(tname);

	adumprec(ADT_ADD_USR_GRP, EX_SUCCESS, strlen(cmdline), cmdline);
	exit(EX_SUCCESS);
	/*NOTREACHED*/
}
