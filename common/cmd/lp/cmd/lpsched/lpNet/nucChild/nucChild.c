/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpNet/nucChild/nucChild.c	1.1"

/*=================================================================*/
/*
*/
#include	<unistd.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<errno.h>
#include	<string.h>
#include	"lpNet.h"
#include	"networkMgmt.h"
#include	"logMgmt.h"
#include	"errorMgmt.h"
#include	"boolean.h"
#include	"lists.h"
#include	"memdup.h"
#include	"debug.h"

#ifndef	_POLLFD_T
typedef	struct	pollfd		pollfd_t;
#endif

static	boolean	AlarmTrapFlag	= False;
extern	int	errno;
extern	int	Lp_NTBase;

/*-----------------------------------------------------------------*/
/*
*	Local functions.
#ifdef	__STDC__

static	int	SendJobToRemote (char *);
static	void	LpExecEvent (pollfd_t *);
static	void	RemoteSystemEvent (pollfd_t *);
static	void	Shutdown (boolean);
static	void	NotifyLpExecOfJobStatus (int, char *);
static	void	SoftwareTerminationTrap (int);
static	void	AlarmTrap (int);
static	list	*SendJobToLpSched (list *);
static	boolean	ConnectToRemoteChild (void);

extern	char	**getjobfiles (char *);

#else

static	int	SendJobToRemote ();
static	void	LpExecEvent ();
static	void	RemoteSystemEvent ();
static	void	Shutdown ();
static	void	NotifyLpExecOfJobStatus ();
static	void	SoftwareTerminationTrap ();
static	void	AlarmTrap ();
static	list	*SendJobToLpSched ();
static	boolean	ConnectToRemoteChild ();

extern	char	**getjobfiles ();

#endif
*/
/*=================================================================*/

/*=================================================================*/
/*
*/
void
nucChild ()
{
	/*----------------------------------------------------------*/
	/*
	*/
		enum {
			lpExec		= 0,
			remoteSystem	= 1
		} pipeIds;

		int	i,
			nFiles,
			timeoutValue,
			nEvents,
			save;
		char	msgbuf [MSGMAX];
		short	status;
		pollfd_t	pipeEvents [2];
	static	char	FnName []	= "nucChild";


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	Some initialization stuff:
	**
	**	o  Trap the 'SIGTERM' signal.
	*/

	WriteLogMsg ("Starting.");

	if (mread (ProcessInfo.lpExecMsg_p, msgbuf, sizeof (msgbuf)) < 0)
	{
		TrapError (Fatal, Unix, FnName, "mread");
	}
	TRACE (mtype (msgbuf))
	if (mtype (msgbuf) != S_CHILD_SYNC)
	{
		TrapError (Fatal, Internal, FnName, 
			"Bad message from lpExec.  Expected S_CHILD_SYNC.");
	}
	if (getmessage (msgbuf, S_CHILD_SYNC, &status) < 0)
	{
		TrapError (Fatal, Unix, FnName, "getmessage");
	}
	if (status != MOK)
	{
		WriteLogMsg ("Child services aborted.");
		return ;
	}

	/*---------------------------------------------------------*/
	/*
	**	This is the main loop of the child.
	*/

	pipeEvents [lpExec].fd		= ProcessInfo.lpExec;
	pipeEvents [lpExec].events	= POLLIN;
	nFiles = 1;

	timeoutValue = -1;
	if (! SIP->timeout)
		SIP->timeout = 1;

	for (;;)
	{
		/*-------------------------------------------------*/
		sleep (20);
	}
}
