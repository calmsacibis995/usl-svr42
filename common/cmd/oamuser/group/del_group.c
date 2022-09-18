/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/del_group.c	1.6.19.2"
#ident  "$Header: del_group.c 2.0 91/07/13 $"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include <userdefs.h>
#include <errno.h>
#include <priv.h>
#include <pfmt.h>
#include <mac.h>
#include "users.h"

struct group *getgrnam();
struct group *getgrgid();
struct group *fgetgrent();
void putgrent();

extern	int	strcmp(), unlink(), rename(),
		lckpwdf(), ulckpwdf();

/*
 * Procedure:	del_group
 *
 * Restrictions:
 *		fopen:		none
 *		access:		none
 *		ulckpwdf:	none
 *		lckpwdf:	none
 *		fgetgrent:	none
 *		fclose:		none
 *		unlink(2):	none
 *		lvlfile(2):	none
 *		rename(2):	none
 *		chmod(2):	none
 *		chown(2):	none
*/


/* Delete a group from the GROUP file */
int
del_group(group)
	char	*group;
{
	register	deleted;
	FILE		*e_fptr, *t_fptr;
	struct	group	*grpstruct;
	struct	stat	statbuf;
	char		*tname, *t_suffix = ".tmp";

	if (stat(GROUP, &statbuf) < 0)  /* get file owner and mode */
		return EX_UPDATE;

	/*
	 * lockout anyone trying to write the group file.  This
	 * is done by calling ``lckpwdf()'' which sets a lock
	 * on the file ``/etc/security/ia/.pwd.lock''.
	*/
	if (lckpwdf() != 0) {
		pfmt(stderr, MM_ERROR, ":0:Group file busy.  Try again later\n");
		return EX_UPDATE;
	}
	if ((e_fptr = fopen(GROUP, "r")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	tname = (char *) malloc(strlen(GROUP) + strlen(t_suffix) + 1);

	(void) sprintf(tname, "%s%s", GROUP, t_suffix);

	/* See if temp file exists before continuing */
	if (access(tname, F_OK) == 0) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	errno = 0;

	if ((t_fptr = fopen(tname, "w+")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	/* loop thru GROUP looking for the one to delete */
	for( deleted = 0; (grpstruct = fgetgrent( e_fptr )); ) {
		
		/* check to see if group is one to delete */
		if (!strcmp(grpstruct->gr_name, group))
			deleted = 1;

		else putgrent(grpstruct, t_fptr);
	}

	(void) fclose(e_fptr);
	(void) fclose(t_fptr);

	if (errno == EINVAL) {
		/* GROUP file contains bad entries */
		(void) unlink(tname);
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	/* If deleted, update GROUP file */
	if (deleted) {
		/* Set MAC level */
		if (statbuf.st_level) {
			if (lvlfile(tname, MAC_SET, &statbuf.st_level) < 0) {
				(void) unlink(tname);
				(void) ulckpwdf();
				return EX_UPDATE;
			}
		}
		/*
		 * After that, go ahead and change the mode, owner, 
		 * and group of the file.  When all that has been done
		 * successfully, rename the file.
		 */
		if (chmod(tname, statbuf.st_mode) < 0 ||
		    chown(tname, statbuf.st_uid, statbuf.st_gid) < 0 ||
		    rename(tname, GROUP) < 0) {
			(void) unlink(tname);
			(void) ulckpwdf();
			return EX_UPDATE;
		}
	}
	(void) unlink(tname);
	(void) ulckpwdf();

	return deleted ? EX_SUCCESS : EX_NAME_NOT_EXIST;
}
