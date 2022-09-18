/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpNet/svChild/svChild.c	1.10.2.7"
#ident  "$Header: svChild.c 1.2 91/06/27 $"

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
*/
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


void
svChild ()
{
	enum {
		lpExec = 0,
		remoteSystem = 1
	} pipeIds;
	int i, nFiles, timeoutValue, nEvents, save;
	char msgbuf [MSGMAX];
	short status;
	pollfd_t pipeEvents [2];
	static char FnName [] = "svChild";


	ENTRYP

	/* Some initialization:        */
	/*   Trap the 'SIGTERM' signal.*/

	(void)	sigset (SIGTERM, SoftwareTerminationTrap);
	(void)	sigset (SIGALRM, AlarmTrap);

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
		Shutdown (False);
	}

	/*  Set defaults for file attributes for files/jobs	*/
	/*  arriving from pre-SVR4ES systems.  			*/
	SetDefaultFileAttributes (DEFAULT_FILE_OWNER,
		(uid_t) -1, (uid_t) -1, DEFAULT_FILE_LID, DEFAULT_FILE_MODE);

	/* This is the main loop of the child.				*/
	/*	o  We always poll the lpExec pipe but we don't		*/
	/*	   always have a connection to the remote system	*/
	/*	   we service.						*/
	/*								*/
	/*	o  The timeoutValue to poll is dependent upon		*/
	/*	   whether we have a connection or not.  If we		*/
	/*	   have a connection then the timeoutValue reflects	*/
	/*	   when we should shutdown the child for inactivity.	*/
	/*	   If we don't have a connection the timeoutValue	*/
	/*	   reflects when we should try to connect to the remote	*/
	/*	   system again.  Until we reach the remote system	*/
	/*	   the child is considered inactive and may timeout	*/
	/*	   anyway.						*/
	/*								*/
	/*	o  The first time through the loop 'timeoutValue' is	*/
	/*	   -1.  This is not a problem since there must be some	*/
	/*	   event there for us to process.			*/
	/*								*/
	/*	o  Master children (those started by the local lpsched)	*/
	/*	   have a higher job priority than Slave children	*/
	/*	   (those started as a result of an incomming job).	*/
	/*	   This is all determined by the parent lpNet process	*/
	/*	   before it starts us.  It is a simple arbitration	*/
	/*	   method in the case of job colisions.			*/

	if (ProcessInfo.processRank == MasterChild)
		SetJobPriority (2);
	else
		SetJobPriority (1);

	pipeEvents [lpExec].fd		= ProcessInfo.lpExec;
	pipeEvents [lpExec].events	= POLLIN;
	nFiles = 1;

	/*  If we are already connected then fine otherwise   	*/
	/*  we have been started to send jobs so make an attempt*/
	/*  to connect.  					*/

	if (Connected (CIP) || ConnectToRemoteChild ())
	{
		pipeEvents [remoteSystem].fd	 = CIP->fd;
		pipeEvents [remoteSystem].events = POLLIN;
		nFiles = 2;
	}
	/*  At first we don't get into timeouts in poll().		*/
	/*  That is, there will be something there shortly so		*/
	/*  lets not worry about it. '-1' says never timeout.		*/
	/*  As for 'SIP->timeout == 0', this is actually too quick	*/
	/*  and the user must be protected from themselves.  So,	*/
	/*  reset it to 1 (minute).  That is good enough.  		*/

	timeoutValue = -1;
	if (! SIP->timeout)
		SIP->timeout = 1;

for (;;)
{
	/*  For nEvents:				*/
	/*	==  0:  we timed-out.			*/
	/*	== -1:  an error occurred.		*/
	/*	>   0:  the number of fd's that have	*/
	/*		events.				*/
	/*  Turn off the alarm immediately.  		*/

	TRACEP ("TOP of main loop")
	TRACEd (nFiles)
	TRACEd (timeoutValue)

	nEvents = poll (pipeEvents, nFiles, timeoutValue);
	save = errno;
	(void)	alarm (0);
	errno = save;

	TRACEd (nEvents)
	TRACEd (errno)

	if (nEvents == -1)
	switch (errno) {
	case	EAGAIN:
		continue;

	case	EINTR:
		/*  Since the alarm is only set to retry	*/
		/*  connections, get right to it.  If		*/
		/*  the connect is successful then call		*/
		/*  'LpExecEvent' to process any stacked	*/
		/*  messages we may have (i.e. jobs pending).  	*/
		TRACEP ("poll() interrupted.")
		if (AlarmTrapFlag)
		{
			TRACEP ("Alarm caught in poll.");
			AlarmTrapFlag = False;
			if (ConnectToRemoteChild ())
			{
				/*  Check for pending outgoing		*/
				/*  jobs.  Then determine if we		*/
				/*  should poll on the network		*/
				/*  connection and what our		*/
				/*  timeout value is in milliseconds.   */
				LpExecEvent (NULL);
				if (! SIP->timeout)
					Shutdown (True);
				if (CIP->fd >= 0)
				{
					pipeEvents [remoteSystem].fd
						= CIP->fd;
					pipeEvents [remoteSystem].events
						= POLLIN;
					nFiles = 2;
					timeoutValue = SIP->timeout;
					if (timeoutValue > 0)
					{
						timeoutValue *= 60000;
					}
				}
			}
			else
			/*  We could not connect so schedule another	*/
			/*  retry.  Immediate retry (SIP->retry == 0)	*/
			/*  is massaged into 'retry once a minue'.  	*/
			{
				if (! SIP->retry)
					(void)	alarm (60);
				else
					(void)	alarm (SIP->retry*60);
			}
		}
		continue;

	case	EFAULT:
	case	EINVAL:
	default:
		TrapError (Fatal, Unix, FnName, "poll");
	}
	if (nEvents == 0)
		Shutdown (True);


	for (i=nFiles-1; i >= 0; i--)
	{
		if (pipeEvents [i].revents)
		switch (i) {
		case	lpExec:
			LpExecEvent (&pipeEvents [i]);
			break;

		case	remoteSystem:
			RemoteSystemEvent (&pipeEvents [i]);
			break;
		}
	}
	/*  If we are connected then we are all set,		*/
	/*  otherwise, we may have lost our connection so	*/
	/*  reset nFiles and timeoutValue.  Then,		*/
	/*  schedule a retry.  					*/
	if (Connected (CIP))
	{
		TRACEd (SIP->timeout)
		if (! SIP->timeout)
			Shutdown (True);
		if (CIP->fd >= 0)
		{
			timeoutValue = SIP->timeout;
			if (timeoutValue > 0)
			{
				timeoutValue *= 60000;
			}
		}
		continue;
	}
	FreeConnectionInfo (&CIP);
	nFiles = 1;

	if (ProcessInfo.processRank == SlaveChild)
	{
		Shutdown (True);
	}
	timeoutValue = -1;

	if (SIP->retry < 0)
		Shutdown (True);

	if (SIP->retry == 0)
	{
		if (ConnectToRemoteChild ())
		{
			LpExecEvent (NULL);
		}
		else
		{
			(void)	alarm (60);  /*  immediate retries.  */
		}
		continue;
	}
	else
		(void)	alarm (SIP->retry*60);

} /*  for (;;) {  */
	/*NOTREACHED*/
}


static
boolean
#ifdef	__STDC__
ConnectToRemoteChild (void)
#else
ConnectToRemoteChild ()
#endif
{
	static char FnName [] = "ConnectToRemoteChild";


	ENTRYP
	CIP = ConnectToSystem (SIP);

	if (! CIP)
		goto	errorReturn_2;

#ifdef	NO_CS
	if (! ConnectToService (CIP, LP_SERVICE_CODE))
		goto	errorReturn_1;
#else
#endif

	if (! SendSystemIdMsg (CIP, NULL, 0))
		goto	errorReturn_1;

	WriteLogMsg ("Connected to remote child.");

	EXITP
	return	True;


errorReturn_1:
	FreeConnectionInfo (&CIP);

errorReturn_2:
	WriteLogMsg ("Could not connect to remote child.");
	EXITP
	return	False;
}



/* lpExec -- sent a message to the child and this is	*/
/* the function that handles it.  			*/
static
void
LpExecEvent (pipeEvents_p)
pollfd_t *pipeEvents_p;
{
	int i, jobId, listLength, save;
	short errstat, fileType, msgsz;
	char tmpbuf[MSGMAX], msgbuf[MSGMAX], *msgp, *reqfile,
	     *remotemsg, *systemname;
	list *dataList_p = NULL, *fileList_p = NULL;
	boolean	jobPending, badEvent = False;
	register uint revents = 0;
	long chk;
	static list *MsgListp = NULL;
	static char FnName [] = "LpExecEvent";


	ENTRYP
	/*  We are most concerned w/ POLLIN.  POLLIN can occur		*/
	/*  w/ POLLHUP and POLLERR.					*/
	/*  If there are no pipe events then we are being called to	*/
	/*  process any stacked messages we may have.  If we are	*/
	/*  not connected then there is nothing we can do.  		*/
	if (! pipeEvents_p)
	{
		if (! Connected (CIP) || ! MsgListp)
		{
			return;
		}
		msgp = (char *)PopListMember (MsgListp);
		if (LengthOfList (MsgListp) == 0)
		{
			FreeList (&MsgListp);
		}
		goto	_switch;
	}
	else
	{
		revents = (unsigned int)  pipeEvents_p->revents;
		TRACE (revents)
	}
_POLLIN:
	if (! (revents & POLLIN))
		goto	_POLLPRI;


	/*  Read the current message off the pipe.  If it is	*/
	/*  S_SHUTDOWN then process immediately.  		*/

	if (mread (ProcessInfo.lpExecMsg_p, msgbuf, sizeof (msgbuf)) == -1)
	{
		TrapError (Fatal, Unix, FnName, "mread");
	}
	if (mtype (msgbuf) == S_SHUTDOWN)
	{
		WriteLogMsg ("Instructed to shutdown.");
		Shutdown (False);
	}
	/*  If we are connected then check for pending messages.	*/
	/*  Put the current message at the end of the list and pop	*/
	/*  the one at the top.  					*/
	if (Connected (CIP))
	{
		if (MsgListp)
		{
			msgp = (char *)memdup (msgbuf, msize (msgbuf));
			if (! msgp)
				TrapError (Fatal, Unix, FnName, "memdup");
			if (! AppendToList (MsgListp, msgp, msize (msgp)))
				TrapError (Fatal, Unix, FnName,
				"AppendToList");
			msgp = (char *) PopListMember (MsgListp);
		}
		else
			msgp = msgbuf;
	}
	else
	/*  ... we are not connected so save the message and	*/
	/*  return.  						*/
	{
		msgp = (char *) memdup (msgbuf, msize (msgbuf));
		if (! MsgListp)
		{
			MsgListp = NewList (PointerList, 0);
			if (MsgListp == NULL)
				TrapError (Fatal, Unix, FnName, "NewList");
		}
		if (! AppendToList (MsgListp, msgp, msize (msgp)))
			TrapError (Fatal, Unix, FnName, "AppendToList");
		return;
	}
	/*  At this point we have a message to service. */ 
	/*  It should be a S_SEND_JOB message.  	*/	
_switch:
	switch (mtype (msgp)) {
	case	S_SEND_JOB:
s_send_job:
		/*  Try to send the job to the remote.  There is some	*/
		/*  underlying job clearance negotiations that occur.	*/
		/*  If the other system has a higher priority than	*/
		/*  we do then we will lose in the negotiations.	*/
		/*  'jobId' == -1 can mean that there is a job pending	*/
		/*  from the other system OR the job transmission failed*/
		/*  OR the file to be printed cannot be accessed.       */
		/*  Also, even though our job was sent we must recognise*/
		/*  that the remote system may have lost in the		*/
		/*  negotiations and that it has a job pending.  	*/
		jobId = SendJobToRemote (msgp);
		save = errno;
		jobPending = JobPending (CIP);
		if (jobId == -1)
		{
			switch (save) {
			/* if the file to be printed on the remote system */
			/* cannot be stat'ed or open'ed , then EACCES is  */
			/* set to be the errno in SendFile().  Want to    */
			/* change the staus to MNOOPEN and then let the   */
			/* scheduler know the job should be canceled.     */
			case EACCES:
				 for (i = 0; i < MSGMAX; i++)
					tmpbuf[i] = msgbuf[i]; 
				 (void)getmessage(tmpbuf,R_PRINT_REQUEST,&errstat,&reqfile,&chk); 
				errstat = MNOOPEN;
				(void) putmessage(tmpbuf,R_PRINT_REQUEST,errstat,reqfile,chk);
				NotifyLpExecOfJobStatus(MOK,tmpbuf);
				break;
			default:
				if (Connected (CIP))
				{
					/*  Our job may have failed because	*/
					/*  we lost out in the job-priority	*/
					/*  negotiations.  Therefore, there	*/
					/*  is an immediate job pending so	*/
					/*  fake the remote system event.	*/
					/*  That is, receive the remote		*/
					/*  system's job.			*/
					if (jobPending)
					{
						RemoteSystemEvent (NULL);
						goto	s_send_job;
					}
				}
				else
				{
					FreeConnectionInfo (&CIP);
				}
				NotifyLpExecOfJobStatus (MTRANSMITERR, 0);
				if (msgp != msgbuf)
				{
					free (msgp);
					msgp = (char *) 0;
				}
				break;
			 } /*end switch */
		break;
		} /*end if JobId == -1 */
		if (msgp != msgbuf)
		{
			free (msgp);
			msgp = (char *) 0;
		}
		jobId = ReceiveJob (CIP, &dataList_p, &fileList_p);

		if (jobId == -1)
		{
			if (!Connected (CIP))
			{
				FreeConnectionInfo (&CIP);
			}
			NotifyLpExecOfJobStatus (MTRANSMITERR, 0);
			break;
		}
		listLength = LengthOfList (dataList_p);

		TRACE (listLength)
		for (i=0; i < listLength-1; i++)
		{
			NotifyLpExecOfJobStatus (MOKMORE,
				(char *)ListMember (dataList_p, i));
		}
		TRACE (i)
		TRACEs (ListMember (dataList_p, i))
		NotifyLpExecOfJobStatus (MOK,
			(char *)ListMember (dataList_p, i));

		FreeList (&dataList_p);
		FreeList (&fileList_p);

		/*  Check and see if we pre-empted a job during	*/
		/*  the underlying job priority negotiations.	*/
		/*  If we did then immediately fake a 		*/
		/*  remote system event.  			*/
		if (jobPending)
		{
			RemoteSystemEvent (NULL);
		}

		/*  If there are still messages on the list then*/
		/*  do them now.  				*/
		if (MsgListp)
		{
			msgp = (char *) PopListMember (MsgListp);
			if (! LengthOfList (MsgListp))
			{
				FreeList (&MsgListp);
			}
			goto	_switch;
		}
		break;

	case	S_SHUTDOWN:
		/*  This is actually an internal error but why*/
		/*  make the situation worse.  Lets just exit.*/
		WriteLogMsg ("Instructed to shutdown.");
		Shutdown (False);
		break;

	default:
		TrapError (Fatal, Internal, FnName, 
			"Received unknown message from lpExec.");
	}
_POLLPRI:
	if (revents & POLLPRI)
	{
		TRACEP ("POLLPRI")
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLPRI) on lpExec pipe.");
	}
_POLLERR:
	if (revents & POLLERR)
	{
		TRACEP ("POLLERR")
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLERR) on lpExec pipe.");
	}
_POLLNVAL:
	if (revents & POLLNVAL)
	{
		TRACEP ("POLLNVAL")
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLNVAL) on lpExec pipe.");
	}
_POLLHUP:
	if (revents & POLLHUP)
	{
		TRACEP ("POLLHUP")
		badEvent = True;
		TrapError (Fatal, Internal, FnName,
		"Hangup (POLLHUP) has occurred.");
	}
	if (badEvent)
		TrapError (Fatal, Internal, FnName, "Cannot recover.");


	EXITP
	return;
}


/* The remote system has sent us a message. */
static
void
RemoteSystemEvent (pipeEvents_p)
pollfd_t *pipeEvents_p;
{
	int jobId;
	list *listOfReplies_p, *dataList_p = NULL, *fileList_p = NULL;
	boolean	badEvent = False;
	register uint revents = 0;
	static char FnName [] = "RemoteSystemEvent";


	ENTRYP

	/* We are most concerned w/ POLLIN.  POLLIN can occur	*/
	/* w/ POLLHUP and POLLERR.  				*/
	if (pipeEvents_p == NULL)
	{
		revents = POLLIN;
	}
	else
	{
		revents = (unsigned int)  pipeEvents_p->revents;
	}
	TRACE (revents)

_POLLIN:
	if (! (revents & POLLIN))
		goto	_POLLPRI;


	/*Before we assume a job is waiting we want to	*/
	/*check the state of our connection.  On some	*/
	/*transports we get POLLIN to signify an event	*/
	/*has occurred - the event being an		*/
	/*'Orderly release indication'.  Therefore, we	*/
	/*want to check for a usable connection not just*/
	/*a POLLIN.  					*/
	TRACEP ("POLLIN")
	if (! Connected (CIP))
	{
		DisconnectSystem (CIP);
		FreeConnectionInfo (&CIP);
		goto	_POLLPRI;
	}
	jobId = ReceiveJob (CIP, &dataList_p, &fileList_p);

	if (jobId == -1)
	{
		if (!Connected (CIP))
		{
			DisconnectSystem (CIP);
			FreeConnectionInfo (&CIP);
		}
		goto	_POLLPRI;
	}
	TRACEs (ListMember (dataList_p, 0))
	TRACEs (ListMember (fileList_p, 0))

#ifdef	DEBUG2
if (getenv ("DEBUG_SVCHILD") == NULL)
{
#endif
	listOfReplies_p = SendJobToLpSched (dataList_p);

	if (listOfReplies_p == NULL)
		goto	freeData;

#ifdef	DEBUG2
}
else
{
	char *p;

	p = strdup ("Hello, world.");

	listOfReplies_p = NewList (StringList, 0);

	(void)	AppendToList (listOfReplies_p, p, 0);
}
#endif

	/* 'jobid' should only be -1 if the connection was lost.*/
	jobId = SendJob (CIP, listOfReplies_p, NULL, NULL);

	if (jobId == -1)
	{
		if (! Connected (CIP))
			FreeConnectionInfo (&CIP);
	}
freeData:
	FreeList (&dataList_p);
	FreeList (&fileList_p);
	FreeList (&listOfReplies_p);


_POLLPRI:
	if (revents & POLLPRI)
	{
		TRACEP ("POLLPRI")
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLPRI) on remote pipe.");
	}
_POLLERR:
	if (revents & POLLERR) {
		TRACEP ("POLLERR")
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLERR) on remote pipe.");
	}
_POLLNVAL:
	if (revents & POLLNVAL) {
		TRACEP ("POLLNVAL")
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLNVAL) on remote pipe.");
	}
_POLLHUP:
	if (revents & POLLHUP)
	{
		TRACEP ("POLLHUP")
		;
	}

	if (badEvent)
		TrapError (Fatal, Internal, FnName, "Cannot recover.");


	EXITP
	return;
}


/* This function takes the HPI message pointed to by	*/
/* jobMsg_p and sends it to the remote along with	*/
/* any files that are part of the job.			*/
/* It does not wait for an answer to the job.  		*/
static
int
SendJobToRemote (jobMsg_p)
char *jobMsg_p;
{
	int i, jobId, save = 0;
	char *remoteMsg_p, *systemName_p, *requestFileName_p, 
	     **fileList_pp, *srcFilePath_p, *destFilePath_p;
	list *dataList_p = NULL, *srcFileList_p	= NULL, 
	     *destFileList_p = NULL;
	short msgSize, fileType;
	boolean	good;
	static char FnName [] = "SendJobToRemote";

	ENTRYP
	if (getmessage (jobMsg_p, S_SEND_JOB, &systemName_p,
		&fileType, &requestFileName_p, &msgSize,
		&remoteMsg_p) == -1)
		TrapError (Fatal, Unix, FnName, "getmessage");

	if ((remoteMsg_p = (char *)memdup (remoteMsg_p, msgSize)) == NULL)
		TrapError (Fatal, Unix, FnName, "memdup");

	if ((dataList_p = NewList (PointerList, 0)) == NULL)
		TrapError (Fatal, Unix, FnName, "NewList");

	if (! AppendToList (dataList_p, remoteMsg_p, msgSize))
		TrapError (Fatal, Unix, FnName, "AppendToList");


	/* Check to make sure the system name matches with	*/
	/* the remote system we are connected to.  		*/
	if (strcmp (systemName_p, SIP->systemName_p) != 0)
		TrapError (Fatal, Internal, FnName, "lpExec confused.");


	/* If filetype is zero then the 'requestFile' is	*/
	/* a data file that is to be sent uninterpreted.	*/
	/* If the fileType is 1 then the 'requestFile' is	*/
	/* a true request-file and needs to be parsed and	*/
	/* all the files it names need to be sent to the	*/
	/* remote.						*/
	if (strlen (requestFileName_p) == 0)
		goto	sendJob;

	switch (fileType) {
	case	0:
		requestFileName_p = strdup (requestFileName_p);

		if (requestFileName_p == NULL)
			TrapError (Fatal, Unix, FnName, "strdup");

		srcFileList_p  = NewList (StringList, 0);

		if (srcFileList_p == NULL)
			TrapError (Fatal, Unix, FnName, "NewList");

		if (! AppendToList (srcFileList_p, requestFileName_p, 0))
			TrapError (Fatal, Unix, FnName, "AppendToList");

		destFileList_p = NULL;
/*
#ifdef	DEBUG
{
		char	path [128];

		(void)	sprintf (path, "/tmp/lpNet/%s",
			(*requestFileName_p == '/' ?
			(requestFileName_p+1) : requestFileName_p));

		requestFileName_p = strdup (path);

		if (requestFileName_p == NULL)
			TrapError (Fatal, Unix, FnName, "strdup");

		destFileList_p = NewList (StringList, 0);

		if (destFileList_p == NULL)
			TrapError (Fatal, Unix, FnName, "NewList");

		if (! AppendToList (destFileList_p, requestFileName_p, 0))
			TrapError (Fatal, Unix, FnName, "AppendToList");

}
#endif
*/
		break;

	case	1:
		srcFileList_p  = NewList (StringList, 0);
		destFileList_p = NewList (StringList, 0);

		if (srcFileList_p == NULL || destFileList_p == NULL)
			TrapError (Fatal, Unix, FnName, "NewList");
		fileList_pp = getjobfiles (requestFileName_p);

		if (fileList_pp == NULL)
			TrapError (Fatal, Unix, FnName, "getjobfiles");

		for (i=0; fileList_pp [i] != NULL; i++) {
			srcFilePath_p   = fileList_pp [i];
			fileList_pp [i] = NULL;
			destFilePath_p  = strdup (srcFilePath_p + Lp_NTBase);

			if (destFilePath_p == NULL)
			TrapError (Fatal, Unix, FnName, "strdup");

			TRACEs (srcFilePath_p)
			if (!AppendToList (srcFileList_p, srcFilePath_p, 0))
			TrapError (Fatal, Unix, FnName, "AppendToList");

			TRACEs (destFilePath_p)
			if (!AppendToList (destFileList_p, destFilePath_p, 0))
			TrapError (Fatal, Unix, FnName, "AppendToList");
		}
		break;

	default:
		TrapError (NonFatal, Internal, FnName,
			"Unknown file-type in S_SEND_JOB message.");
		EXITP
		return	-1;
	}


sendJob:
	jobId = SendJob (CIP, dataList_p, srcFileList_p, destFileList_p);
	save = errno;
	if (jobId != -1)
	{
		(void)	ApplyToList (srcFileList_p, (void *(*)())unlink,
				     EmptyList, 0);
	}
	FreeList (&srcFileList_p);
	FreeList (&destFileList_p);


	EXITP
	errno = save;
	return	jobId;
}


/* This function sends the job to lpSched and gets the reply.  */
static
list *
SendJobToLpSched (msgs_p)
list *msgs_p;
{
	int i, listLength, msgType, msgSize;
	char msgBuffer [512], *member_p, *fooChar_p;
	list *listOfReplies_p;
	long fooLong;
	short statusCode, fooShort;
	boolean	done;
	static boolean PipeNotOpened = True;
	static char FnName [] = "SendJobToLpSched";


	ENTRYP
	if (PipeNotOpened) {
		if (mopen () == -1) {
			TrapError (NonFatal, Unix, FnName, "mopen");
			EXITP
			return	NULL;
		}

		PipeNotOpened = False;
	}
	listLength = LengthOfList (msgs_p);  TRACE (listLength)
	for (i=0; i < listLength; i++)
	{
		
		if (msend (ListMember (msgs_p, i)) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "msend");
			goto	errorReturn_1;
		}
	}


	listOfReplies_p = NewList (PointerList, 0);

	if (listOfReplies_p == NULL)
		TrapError (Fatal, Unix, FnName, "NewList");

	done = False;

	do
	{
		msgType = mrecv (msgBuffer, sizeof (msgBuffer));

		if (msgType == -1)
		{
			TrapError (NonFatal, Unix, FnName, "mrecv");
			goto	errorReturn_2;
		}

		msgSize  = msize (msgBuffer);
		member_p = (char *)  memdup (msgBuffer, msgSize);

		if (member_p == NULL)
			TrapError (Fatal, Unix, FnName, "memdup");

		if (! AppendToList (listOfReplies_p, member_p, msgSize))
			TrapError (Fatal, Unix, FnName, "AppendToList");

		switch (msgType) {
		case	R_PRINT_REQUEST:
			done = True;
			break;

		case	R_JOB_COMPLETED:
			done = True;
			break;

		case	R_CANCEL:
			/* Can have more than one reply. */
			if (getmessage (msgBuffer, R_CANCEL,
			    &statusCode, &fooLong, &fooChar_p) == -1)
			{
				TrapError (Fatal, Unix, FnName, "getmessage");
			}
			if (statusCode != MOKMORE)
				done = True;
			break;

		case	R_INQUIRE_PRINTER_STATUS:
			/* Can have more than one reply.  */
			if (getmessage (msgBuffer, R_INQUIRE_PRINTER_STATUS,
			    &statusCode,
			    &fooChar_p, &fooChar_p, &fooChar_p, &fooChar_p,
			    &fooChar_p, &fooShort, &fooChar_p, &fooLong,
			    &fooLong) == -1)
			{
				TrapError (Fatal, Unix, FnName, "getmessage");
			}
			if (statusCode != MOKMORE)
				done = True;
			break;

		case	R_GET_STATUS:
			done = True;
			break;

		case	R_INQUIRE_REQUEST:
			if (getmessage (msgBuffer, R_INQUIRE_REQUEST,
			    &statusCode, &fooChar_p, &fooChar_p, &fooLong,
			    &fooLong, &fooShort, &fooChar_p, &fooChar_p,
			    &fooChar_p, &fooLong) == -1)
			{
				TrapError (Fatal, Unix, FnName, "getmessage");
			}
			if (statusCode != MOKMORE)
				done = True;
			break;

		case	R_INQUIRE_REQUEST_RANK:
			if (getmessage (msgBuffer, R_INQUIRE_REQUEST_RANK,
			    &statusCode, &fooChar_p, &fooChar_p, &fooLong,
			    &fooLong, &fooShort, &fooChar_p, &fooChar_p,
			    &fooChar_p, &fooShort, &fooLong) == -1)
			{
				TrapError (Fatal, Unix, FnName, "getmessage");
			}
			if (statusCode != MOKMORE)
				done = True;
			break;

		default:
			TRACEP ("Bad message from lpsched.")
			TRACE (msgType)
			TrapError (Fatal, Internal, FnName,
			"Unknown message received from lpSched.");
		}
	} while (! done);


	EXITP
	return	listOfReplies_p;


errorReturn_2:
	FreeList (&listOfReplies_p);

errorReturn_1:
	(void)	mclose ();
	PipeNotOpened = True;


	EXITP
	return	NULL;
}



static
void
Shutdown (notifyLpExec)
boolean notifyLpExec;
{
	pollfd_t pollfd;
	static char FnName [] = "Shutdown";

	ENTRYP
	if (Connected (CIP))
	{
		DisconnectSystem (CIP);
		FreeConnectionInfo (&CIP);
	}
	/*  If we notify lpsched/lpexec then wait for hangup to		*/
	/*  ensure that it doesn't lose our message and in our hangup.	*/
	/*  We do a POLLIN to get a POLLHUP.  That is, we can't do	*/
	/*  a poll() for a hangup.  					*/
	if (notifyLpExec)
	{
		if (mputm (ProcessInfo.lpExecMsg_p, S_SHUTDOWN, 0) < 0)
		{
			TrapError (NonFatal, Unix, FnName, "mputm");
		}
		pollfd.fd	= ProcessInfo.lpExec;
		pollfd.events	= POLLIN;
		pollfd.revents	= 0;
		(void)	poll (&pollfd, 1, -1);
	}
	EXITP
	Exit (0);
}



static
void
NotifyLpExecOfJobStatus (status, msg_p)
int status;
char *msg_p;
{
	static char FnName [] = "NotifyLpExecOfJobStatus";


	ENTRYP

#ifdef	DEBUG
if (getenv ("DEBUG_SVCHILD") == NULL) {
#endif
	if (mputm (ProcessInfo.lpExecMsg_p, R_SEND_JOB,
	    SIP->systemName_p, (short) status,
	    msg_p == NULL ? 0 : (short) msize (msg_p),
	    msg_p == NULL ? "" : msg_p) == -1)
	{
		TrapError (Fatal, Unix, FnName, "mputm");
	}
#ifdef	DEBUG
}
else
	if (mputm (ProcessInfo.lpExecMsg_p, R_SEND_JOB,
	    SIP->systemName_p, (short) status,
	    msg_p == NULL ? 0 : strlen(msg_p)+1,
	    msg_p == NULL ? "" : msg_p) == -1)
	{
		TrapError (Fatal, Unix, FnName, "mputm");
	}
#endif

	EXITP
	return;
}



/* This function is used to trap the SIGTERM signal.  	*/
/* An lpNet child may receive it from the lpNet parent  */
/* if the parent can't send lpExec the file-descriptor  */
/* to talk with the child.  				*/
static
void
SoftwareTerminationTrap (signalNumber)
int signalNumber;
{
	signalNumber = signalNumber;

	WriteLogMsg ("Received 'SIGTERM' signal, exiting.");

	Shutdown (True);	/*  Does not return.  */
}



static
void
AlarmTrap (signal)
int signal;
{
	AlarmTrapFlag = True;
	return;
}
