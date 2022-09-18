/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/groupadd.c	1.4.19.2"
#ident  "$Header: groupadd.c 2.0 91/07/13 $"

/*
 * Command:	groupadd
 *
 * Usage:	groupadd [-g gid [-o]] group
 *
 * Inheritable Privileges:	P_MACWRITE,P_AUDIT,P_SETFLEVEL,P_DACWRITE
 *				P_MACREAD,P_DACREAD
 *       Fixed Privileges:	None
 *
 * Notes:	Add (create) a new group definition on the system.
 *
 *		Arguments are:
 *
 *			gid - a gid_t less than MAXUID
 *			group - a string of printable characters excluding
 *				colon(:) and less than MAXGLEN characters long.
 *
 *		P_MACWRITE is required for writing to /etc/group.
 */

/* LINTLIBRARY */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<limits.h>
#include	<userdefs.h>
#include	<string.h>
#include	<users.h>
#include	"messages.h"
#include	<audit.h>
#include	<priv.h>
#include	<pfmt.h>
#include	<locale.h>

extern	char	*optarg,		/* used by getopt */
		*errmsgs[];

extern int optind, opterr;	/* used by getopt */

extern int getopt(), errmsg(), valid_gname();
extern gid_t findnextgid();
extern int valid_gid(), add_group();
extern char	*argvtostr();
extern long strtol();
extern void exit(), adumprec();

char *msg_label = "UX:groupadd";
static char *cmdline = (char *)0;
/*
 * Procedure:     main
 *
 * Restrictions:
 *                printf: none
 *                getopt: none
 */

main(argc, argv)
int argc;
char *argv[];
{
	int ch;				/* return from getopt */
	gid_t gid;			/* group id */
	int oflag = 0;	/* flags */
	register rc;
	char *gidstr = NULL;	/* gid from command line */
	char *grpname;			/* group name from command line */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel(msg_label);
	
	/* save command line arguments */
	if (( cmdline = (char *)argvtostr(argv)) == NULL) {
                printf("failed argvtostr()\n");
		adumprec(ADT_ADD_GRP,1,strlen(argv[0]),argv[0]);
                exit(1);
        }

	while((ch = getopt(argc, argv, "g:o")) != EOF)
		switch(ch) {
			case 'g':
				gidstr = optarg;
				break;
			case 'o':
				oflag++;
				break;
			case '?':
				errmsg( M_AUSAGE );
				adumprec(ADT_ADD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
				exit( EX_SYNTAX );
		}

	if( (oflag && !gidstr) || optind != argc - 1 ) {
		errmsg( M_AUSAGE );
		adumprec(ADT_ADD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
		exit( EX_SYNTAX );
	}


	grpname = argv[optind];

	switch( valid_gname( grpname, NULL ) ) {
	case INVALID:
		errmsg( M_GRP_INVALID, grpname );
		adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
		exit( EX_BADARG );
		/*NOTREACHED*/
	case NOTUNIQUE:
		errmsg( M_GRP_USED, grpname );
		adumprec(ADT_ADD_GRP,EX_NAME_EXISTS,strlen(cmdline),cmdline);
		exit( EX_NAME_EXISTS );
		/*NOTREACHED*/
	}

	if( gidstr ) {
		/* Given a gid string - validate it */
		char *ptr;

		gid = (gid_t) strtol( gidstr, &ptr, 10 );

		if( *ptr ) {
			errmsg( M_GID_INVALID, gidstr );
			adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );
		}

		switch( valid_gid( gid, NULL ) ) {
		case RESERVED:
			(void) pfmt(stderr, MM_WARNING, errmsgs[M_RESERVED], gid);
			break;

		case NOTUNIQUE:
			if( !oflag ) {
				errmsg( M_GRP_USED, gidstr );
				adumprec(ADT_ADD_GRP,EX_ID_EXISTS,strlen(cmdline),cmdline);
				exit( EX_ID_EXISTS );
			}
			break;

		case INVALID:
			errmsg( M_GID_INVALID, gidstr );
			adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );

		case TOOBIG:
			errmsg( M_TOOBIG, gid );
			adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );

		}

	} else {
		if( (gid = findnextgid()) < 0 ) {
			errmsg( M_GID_INVALID, "default id" );
			exit( EX_ID_EXISTS );
		}
	}

	if( (rc = add_group(grpname, gid) ) != EX_SUCCESS )
		errmsg( M_UPDATE, "created" );

	adumprec(ADT_ADD_GRP,rc,strlen(cmdline),cmdline);
	exit( rc );
	/*NOTREACHED*/
}
