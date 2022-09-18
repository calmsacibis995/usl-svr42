/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/spool.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<limits.h>
#include	<string.h>
#include	<Gizmos.h>
#include	"error.h"
#include	"uucp.h"
#include	"dtcopy.h"

int
mkdirs(base, extention, ownerid)
char	*base;
char	*extention;
int	ownerid;
{
	char	*dirp;
	char	*s1;
	char	path[PATH_MAX];
	char	ext[PATH_MAX];
	struct	stat	buf;
	mode_t	um;

	(void)strcpy(path, base);
	(void)strcpy(ext, extention);

	if (*LASTCHAR(path) != '/')
		(void)strcat(path, "/");

	if (*LASTCHAR(ext) != '/')
		(void)strcat(ext, "/");

	s1 = ext;

	um = umask(0022);

	while( (dirp = strtok(s1, "/")) != (char *)NULL ) {
		s1 = (char *)NULL;
		(void)strcat(path, dirp);
		if ( DIRECTORY(path, buf) ) {
			/* LATER need to check group and other if not owner */
			if ( buf.st_uid == ownerid ) {
				(void)strcat(path, "/");
				continue;
			}
			PUTMSG(GGT(string_noAccessNodeDir));
				return(-1);
		}
		if (mkdir(path, MODE) == -1) {
			PUTMSG(GGT(string_noAccessNodeDir));
			return(-1);
		}
		(void)strcat(path, "/");
	}

	(void)umask(um);

	return(0);
}
