/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)setmnt:setmnt.c	4.13.1.11"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/setmnt/setmnt.c,v 1.1 91/02/28 19:48:40 ccs Exp $"

/***************************************************************************
 * Command: setmnt
 * Inheritable Privileges: P_MACWRITE,P_DACWRITE,P_MACREAD,P_DACREAD,
 *			   P_SETFLEVEL,P_COMPAT,P_OWNER
 *       Fixed Privileges: None
 * Notes: /sbin/setmnt is generally used only to set the /etc/mnttab
 *	  file when the system is first boot up.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<errno.h>
#include	<sys/mnttab.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/statvfs.h>
#include	<mac.h>

#define	LINESIZ	BUFSIZ
#define	OPTSIZ	64
#define	MNTTAB_OWN	0	/* root       */
#define	MNTTAB_GRP	3	/* sys        */
#define	MNTTAB_MODE	0444	/* -r--r--r-- */

extern char	*fgets();
extern char	*strtok();
extern char	*strrchr();
extern time_t	time();

static char	*opts();
static void	error();

static char	*myname;
static char	line[LINESIZ];
static char	sepstr[] = " \t\n";
static char	mnttab[] = MNTTAB;
static char	parent_dir[] = "/etc";
static char	mnttab_tmp[] = "/etc/mnttab_tmp";

/*
 * Procedure:     main
 *
 * Restrictions:
                 fprintf: None
                 fopen: None
                 fgets: None
                 statvfs(2): None
                 fclose: None
                 unlink(2): None
                 rename(2): None
                 lvlfile(2): None
                 chmod(2): None
                 chown(2): None
 *
 * Notes: /sbin/setmnt creates a temporary file /etc/mnttab_tmp and
 *	  takes from stdin the input of the mnttab entry.  It writes to
 *	  this temp file after stat'ing the mount-points.  Then it
 *	  replace the original /etc/mnttab file (if there is one) with
 *	  the temp file.  It sets the default attributes of the file.
 */	
main(argc, argv)
	char	**argv;
{
	char	*lp = line;
	time_t	date;
	FILE	*fd;
	struct mnttab	mm;
	struct statvfs	sbuf;
	level_t level_lid = 0;	/* MAC level identifier */
	
	myname = strrchr(argv[0], '/');
	if (myname)
		myname++;
	else
		myname = argv[0];

        if (argc > 1) {
                fprintf(stderr, "Usage: %s\n", myname);
                exit(1);
        }

	if ( (fd = fopen(mnttab_tmp, "w")) == NULL )
		error("cannot create temp mnttab file");
	
	time(&date);

	/* write immediately, so errors will be noticed */
	setbuf(fd, NULL);
	while ((lp = fgets(line, LINESIZ, stdin)) != NULL) {
		if ((mm.mnt_special = strtok(lp, sepstr)) != NULL &&
		    (mm.mnt_mountp = strtok(NULL, sepstr)) != NULL &&
		     statvfs(mm.mnt_mountp, &sbuf) == 0) {
			if (fprintf(fd, "%s\t%s\t%s\t%s\t%d\n",
				mm.mnt_special,
				mm.mnt_mountp,
				sbuf.f_basetype ? sbuf.f_basetype : "-",
				opts(sbuf.f_flag),
				date) < 0) {
			    unlink(mnttab_tmp);
			    error("cannot update mnttab");
			}
		}
	}
	fclose(fd);

	(void) unlink(mnttab);
	if ( rename(mnttab_tmp, mnttab) != 0 ) 
		error("cannot rename temp file");

	/* 
	 * set correct attributes: level = SYS_PUBLIC
	 * owner = root, group = sys, mode = 0444
	 */

	/* find lid of SYS_PUBLIC by finding lid of mnttab's parent dir. */
	if ( lvlfile(parent_dir, MAC_GET, &level_lid) < 0) {
		error("cannot get level identifier");
	}

	/* If level_lid is then, set level of mnttab */
	if ( level_lid ) {
		if ( lvlfile(mnttab, MAC_SET, &level_lid) != 0) {
				error("cannot set level");
		}
	}
	
	if ( chmod(mnttab, (mode_t) MNTTAB_MODE) != 0 || 
	     chown(mnttab, (uid_t) MNTTAB_OWN, (gid_t) MNTTAB_GRP) != 0 )
		error("cannot set attributes");
	
	exit(0);
}

static
char *
opts(flag)
	ulong	flag;
{
	static char	mntopts[OPTSIZ];

	sprintf(mntopts, "%s,%s",
		(flag & ST_RDONLY) ? "ro" : "rw",
		(flag & ST_NOSUID) ? "nosuid" : "suid");

	return	mntopts;
}

static
void 
error(string)
	char *string;
{
	fprintf(stderr, "%s: %s\n", myname, string);
	exit(1);
}
