/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/msgs/mcreate.c	1.11.1.3"
#ident	"$Header: $"

#include	<unistd.h>
#include	<string.h>
#include	<stropts.h>
#include	<errno.h>
#include	<stdlib.h>

#include	"lp.h"
#include	"msgs.h"
#include	"debug.h"

#if	defined(__STDC__)
MESG *
mcreate (char *pathp)
#else
MESG *
mcreate (pathp)

char	*pathp;
#endif
{
	int		fds[2];
	level_t		lid;
	MESG		*md;

	DEFINE_FNNAME(mcreate)

	ENTRYP

	(void)	SetFileLevel (pathp, "SYS_PUBLIC");
	(void)	lvlproc (MAC_GET, &lid); TRACEx (lid)
	(void)	SetProcLevel ("SYS_PUBLIC");
#ifdef	DEBUG
{
	level_t	lid;
	(void)	lvlproc (MAC_GET, &lid); TRACEx (lid)
}
#endif

	if (pipe(fds) != 0)
		return	NULL;

	if (ioctl(fds[1], I_PUSH, "connld") != 0)
		return	NULL;


	if (fattach(fds[1], pathp) != 0)
		return	NULL;

	(void)	lvlproc (MAC_SET, &lid);
#ifdef	DEBUG
{
	level_t	lid;
	(void)	lvlproc (MAC_GET, &lid); TRACEx (lid)
}
#endif

	if ((md = (MESG *) Calloc(1, MDSIZE)) == NULL)
		return	NULL;
	
	md->admin	= 1;
	md->file	= Strdup (pathp);
	md->gid		= getgid ();
	md->mque	= NULL;
	md->on_discon	= NULL;
	md->readfd	= fds[0];
	md->state	= MDS_IDLE;
	md->type	= MD_MASTER;
	md->uid		= getuid ();
	md->writefd	= fds[1];

	EXITP
	return	md;
}
