/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/lvlvalid.c	1.4"

#ifdef __STDC__
	#pragma weak lvlvalid = _lvlvalid
#endif

#include "synonyms.h"
#include <sys/types.h>
#include <mac.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/*
 * lvlvalid	- check the validity of a level
 *
 * Return:
 *	0	- level is valid active
 *	1	- level is valid inactive
 *	-1	- EINVAL: level is invalid
 *		- EACCES: cannot open LTDB
 */
int
lvlvalid(levelp)
	const level_t	*levelp;		/* lid ptr */
{
	int	rfd;				/* read file descriptor */
	struct mac_level buf;		/* static level buffer */
	register struct mac_level *lvlp = &buf;	/* level structure ptr */

	/* 0 reserved */
	if (*levelp == (level_t)0) {
		errno = EINVAL;
		return(-1);
	}

	/* open LTDB */
	if ((rfd = open(LTF_LID, O_RDONLY, 0)) == -1) {
		errno = EACCES;
		return(-1);
	}

	/* retrieve appropriate internal level structure */
	if (lseek(rfd, (*levelp)*sizeof(struct mac_level), 0) == -1
	||  read(rfd, lvlp, sizeof(struct mac_level))
		!= sizeof(struct mac_level)) {
		(void)close(rfd);
		errno = EINVAL;
		return(-1);
	}

	(void)close(rfd);

	/* check if active */
	if (lvlp->lvl_valid == LVL_ACTIVE)
		return(0);

	/* check if inactive */
	if (lvlp->lvl_valid == LVL_INACTIVE)
		return(1);

	/* must be invalid */
	errno = EINVAL;
	return(-1);
}
