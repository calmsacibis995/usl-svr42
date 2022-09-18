/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lpNet/networkMgmt.c	1.12.2.6"
#ident  "$Header: networkMgmt.c 1.2 91/06/27 $"

/*==================================================================*/
/*
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <rpc/rpc.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <pwd.h>
#include <mac.h>
#include "networkMgmt.h"
#include "errorMgmt.h"
#include "lists.h"
#include "memdup.h"
#include "debug.h"

static	uid_t	DefaultUID	= (uid_t) -1;
static	gid_t	DefaultGID	= (uid_t) -1;
static	level_t	DefaultLID	= (level_t) -1;
static	mode_t	DefaultMode	= (mode_t) -1;
static	char *	DefaultOwnerp	= (char *) 0;

boolean		JobPendingFlag	= False;
jobControlMsg	JobControl;

#ifdef	__STDC__

static char *	ReceiveFile_1_1 (connectionInfo *, fileFragmentMsg_1_1 *);
static char *	ReceiveFile_1_2 (connectionInfo *, fileFragmentMsg_1_2 *);
static void	NormalizeFileFragmentMsg_1_1 (void **);
static void	NormalizeFileFragmentMsg_1_2 (void **);
static boolean	EncodeNetworkMsgTag_1_1 (connectionInfo *, networkMsgType);
static boolean	EncodeNetworkMsgTag_1_2 (connectionInfo *, networkMsgType);
static boolean	SendJobControlMsg_1_1 (connectionInfo *, jobControlCode);
static boolean	SendJobControlMsg_1_2 (connectionInfo *, jobControlCode);
static networkMsgTag_1_1 * DecodeNetworkMsg_1_1 (connectionInfo *,
				jobControlMsg **, void **);
static networkMsgTag_1_2 * DecodeNetworkMsg_1_2 (connectionInfo *,
				jobControlMsg **, void **);
#else

static char *	ReceiveFile_1_1 ();
static char *	ReceiveFile_1_2 ();
static void	NormalizeFileFragmentMsg_1_1 ();
static void	NormalizeFileFragmentMsg_1_2 ();
static boolean	EncodeNetworkMsgTag_1_1 ();
static boolean	EncodeNetworkMsgTag_1_2 ();
static boolean	SendJobControlMsg_1_1 ();
static boolean	SendJobControlMsg_1_2 ();
static networkMsgTag * DecodeNetworkMsg_1_1 ();
static networkMsgTag * DecodeNetworkMsg_1_2 ();

#endif
extern int errno;


int
SendJob (cip, dataList_p, srcFileList_p, destFileList_p)
connectionInfo *cip;
list *dataList_p, *srcFileList_p, *destFileList_p;
{
	int i, jobId, listLength;
	boolean	endOfJob = True;
	static	char  FnName []	= "SendJob";


	ENTRYP

	/*  Check our args.  */
	if (! cip)
	{
		errno = EINVAL;
		TRACE (errno)
		EXITP
		return	-1;
	}
	if (! Connected (cip))
	{
		errno = ENOLINK;
		TRACE (errno)
		EXITP
		return	-1;
	}
	if (dataList_p == NULL && srcFileList_p == NULL)
	{
		errno = EINVAL;
		TRACE (errno)
		EXITP
		return	-1;
	}


	if ((jobId = NegotiateJobClearance (cip)) == -1)
	{
		TRACE (errno)
		EXITP
		return	-1;
	}


	/* Send data. */
	endOfJob = False;

	listLength = LengthOfList (dataList_p);

	for (i=0; i < listLength-1; i++) {
		if (! SendData (cip, endOfJob,
		      ListMember (dataList_p, i),
		      SizeofListMember (dataList_p, i)))
		{
			int	save = errno;
			if (Connected (cip))
				SendJobControlMsg (cip, JobAborted);
			errno = save;
			TRACE (errno)
			EXITP
			return	-1;
		}
	}
	if (srcFileList_p == NULL)
	{
		endOfJob = True;
	}
	if (! SendData (cip, endOfJob, ListMember (dataList_p, i),
	      SizeofListMember (dataList_p, i)))
	{
		int	save = errno;
		if (Connected (cip))
			SendJobControlMsg (cip, JobAborted);
		errno = save;
		TRACE (errno)
		EXITP
		return	-1;
	}
	if (endOfJob)
	{
		EXITP
		return	jobId;
	}


	/* Send files. */
	listLength = LengthOfList (srcFileList_p);

	for (i=0; i < listLength-1; i++) {
		if (! SendFile (cip, endOfJob,
		      ListMember (srcFileList_p, i),
		      destFileList_p == NULL ? NULL :
		      ListMember (destFileList_p, i)))
		{
			int	save = errno;
			if (Connected (cip))
				SendJobControlMsg (cip, JobAborted);
			errno = save;
			TRACE (errno)
			EXITP
			return	-1;
		}
	}
	endOfJob = True;

	if (! SendFile (cip, endOfJob,
	      ListMember (srcFileList_p, i),
	      destFileList_p == NULL ? NULL :
	      ListMember (destFileList_p, i)))
	{
		int	save = errno;
		if (Connected (cip))
			SendJobControlMsg (cip, JobAborted);
		errno = save;
		TRACE (errno)
		EXITP
		return	-1;
	}
	EXITP
	return	jobId;
}
int
ReceiveJob (cip, dataList_pp, srcFileList_pp)
connectionInfo *cip;
list **dataList_pp, **srcFileList_pp;
{
	int jobId = -1, msgCount;
	char *fileName_p;
	void *data_p, *networkMsgp;
	boolean	endOfJob;
	networkMsgTag *networkMsgTagp;
	jobControlMsg *jobControlMsgp;
	static char FnName [] = "ReceiveJob";


	ENTRYP

	/* Check our args. */
	if (! cip)
	{
		errno = EINVAL;
		TRACE (errno)
		EXITP
		return	-1;
	}
	if (! Connected (cip))
	{
		errno = ENOLINK;
		TRACE (errno)
		EXITP
		return	-1;
	}
	if (dataList_pp == NULL && srcFileList_pp == NULL)
	{
		errno = EINVAL;
		TRACE (errno)
		EXITP
		return	-1;
	}

	msgCount = 0;
	endOfJob = False;
	do {
		networkMsgTagp =
			ReceiveNetworkMsg (cip, &jobControlMsgp, &networkMsgp);
	
		if (! networkMsgTagp)
		{
			TRACE (errno)
			goto	errorReturn_1;
		}
		msgCount++;
		if (networkMsgTagp->msgType == JobControlMsg)
		{
			switch (jobControlMsgp->controlCode)
			{
			case	RequestToSendJob:
				TRACEP ("Received 'RequestToSendJob'.");
				if (msgCount != 1)
				{
					TrapError (NonFatal, Internal, FnName,
					"Network messages out of sync.");
					goto	errorReturn_1;
				}
				TRACEP ("Sending 'ClearToSendJob'.")
				jobId = jobControlMsgp->jobId;
				if (! SendJobControlMsg (cip, ClearToSendJob))
					goto	errorReturn_1;
				break;
		
			case	JobAborted:
				TRACEP ("Received 'JobAborted'.");
				goto	errorReturn_1;
		
			default:
				TRACE (jobControlMsgp->controlCode)
				TrapError (NonFatal, Internal, FnName,
					"Unexpected network message.");
				errno = EPROTO;
				TRACE (errno)
				EXITP
				return	-1;
			}
			continue;
		}
		endOfJob = jobControlMsgp->endOfJob;
	
		switch (networkMsgTagp->msgType)
		{
		case 	DataPacketMsg:
			if (! *dataList_pp)
			{
				*dataList_pp = NewList (PointerList, 0);
				if (*dataList_pp == NULL)
				{
					TrapError (Fatal, Unix, FnName, "NewList");
				}
			}
			data_p = memdup (
				((dataPacketMsg *) networkMsgp)->data.data_val,
				((dataPacketMsg *) networkMsgp)->data.data_len);
	
			if (! data_p)
			{
				TrapError (Fatal, Unix, FnName, "memdup");
			}
			if (! AppendToList (*dataList_pp, data_p,
		      	((dataPacketMsg *) networkMsgp)->data.data_len))
			{
				TrapError (Fatal, Unix, FnName, "AppendToList");
			}
			FreeNetworkMsg (DataPacketMsg, &networkMsgp);
			break;
	
		case 	FileFragmentMsg:
			if (! *srcFileList_pp)
			{
				*srcFileList_pp = NewList (StringList, 0);
				if (*srcFileList_pp == NULL)
				{
					TrapError (Fatal, Unix, FnName, "NewList");
				}
			}

			/* NOTE:  The networkMsgp is unusable */
			/* after a return from ReceiveFile.   */
			/* Therefore, do not free.            */

			fileName_p = ReceiveFile (cip, networkMsgp);
	
			networkMsgp = NULL;
	
			if (! fileName_p)
			{
				goto	errorReturn_1;
			}
			if (! AppendToList (*srcFileList_pp, fileName_p, 0))
			{
				TrapError (Fatal, Unix, FnName, "AppendToList");
			}
			break;
	
	
		default:
			TRACE (networkMsgTagp->msgType)
			TrapError (NonFatal, Internal, FnName,
				"Unknown message from remote.");
			free (networkMsgp);
			goto	errorReturn_1;
		}
	} while (! endOfJob);
	

	JobPendingFlag = False;
	EXITP
	return	jobId;


errorReturn_1:
	FreeList (dataList_pp);

	if (*srcFileList_pp != NULL)
	{
		(void)
		ApplyToList (*srcFileList_pp, (void *(*) ()) unlink,
			     EmptyList, 0);
		FreeList (srcFileList_pp);
	}
	if (Connected (cip))
		SendJobControlMsg (cip, AbortJob);

	JobPendingFlag = False;
	errno = ECOMM;
	TRACE (errno)
	EXITP
	return	-1;
}

boolean
SendFile (cip, lastFileOfJob, srcPathp, destPathp)
connectionInfo *cip;
boolean lastFileOfJob;
char *srcPathp, *destPathp;
{
	int fd, nBytes, endOfJob = False;
	struct stat statInfo;
	struct passwd *pwp;
	fileFragmentMsg	fragMsg;
	static	size_t FragbufSize = 32768;
	static	caddr_t *Fragbufp = NULL;
	static	char FnName [] = "SendFile";


	ENTRYP
	TRACE  (cip)
	TRACEs (srcPathp);
	TRACEs (destPathp);


	if (! cip || ! srcPathp)
	{
		errno = EINVAL;
		TRACE (errno);
		EXITP
		return	False;
	}


	if (Fragbufp == NULL)
	{
		Fragbufp = malloc (FragbufSize);

		if (Fragbufp == NULL)
		{
			TrapError (Fatal, Unix, FnName, "malloc");
		}
	}
	(void)	memset (&fragMsg, sizeof (fragMsg), 0);


	if (stat (srcPathp, &statInfo) == -1)
	{
		TRACE (errno)
		EXITP
		errno = EACCES;
		return	False;
	}
	fragMsg.sizeOfFile = statInfo.st_size;
	fragMsg.uid = statInfo.st_uid;
	fragMsg.gid = statInfo.st_gid;
	fragMsg.lid = statInfo.st_level;
	fragMsg.mode = statInfo.st_mode;
	pwp = getpwuid (statInfo.st_uid);
	if (pwp)
	{
		fragMsg.ownerp = strdup (pwp->pw_name);
	}
	endpwent ();

	TRACEd (fragMsg.sizeOfFile)
	TRACEd (fragMsg.uid)
	TRACEd (fragMsg.gid)
	TRACEd (fragMsg.lid)
	TRACEo (fragMsg.mode)
	TRACEs (fragMsg.ownerp)

	if ((fd = open (srcPathp, O_RDONLY)) == -1)
	{
		TRACE (errno)
		EXITP
		errno = EACCES;
		return	False;
	}


	fragMsg.fragp = Fragbufp;
	fragMsg.destPathp = destPathp == NULL ? srcPathp : destPathp;

	nBytes = 0;
	do {
		fragMsg.fraglen = read (fd, Fragbufp, FragbufSize);

		if (fragMsg.fraglen == -1)
		{
			int	save = errno;
			(void) close (fd);
			errno = save;
			TRACE (errno)
			EXITP
			return	False;
		}
		nBytes += fragMsg.fraglen;

		if (nBytes < statInfo.st_size)
		{
			fragMsg.endOfFile = False;
		}
		else
		{
			fragMsg.endOfFile = True;
			if (lastFileOfJob)
			{
				endOfJob = True;
			}
		}
		if (! SendFileFragmentMsg (cip, endOfJob, &fragMsg))
		{
			int	save = errno;
			(void)	close (fd);
			errno = save;
			TRACE (errno)
			EXITP
			return	False;
		}
	} while (! fragMsg.endOfFile);

	(void)	close (fd);
	EXITP
	return	True;
}


char *
ReceiveFile (cip, fragMsgp)
connectionInfo *cip;
fileFragmentMsg *fragMsgp;
{
	int n, fd, nBytes;
	char *destPathp, *basenamep, *dirnamep, inbuf [128], outbuf [128];
	uid_t local_uid = (uid_t) -1, local_gid = (uid_t) -1;
	level_t local_lid = (level_t) -1;
	mode_t local_mode = (mode_t) -1;
	struct passwd *pwp;
	networkMsgTag *networkMsgTagp;
	jobControlMsg *jobControlMsgp;
	static char FnName [] = "ReceiveFile";


	/*  Check the file system to make */
	/*  sure we have enough space to  */
	/*  hold the entire file.	  */


	TRACEs (fragMsgp->destPathp)
	TRACEd (fragMsgp->sizeOfFile)

	if ((destPathp = strdup (fragMsgp->destPathp)) == NULL)
	{
		TrapError (Fatal, Unix, FnName, "strdup");
	}
	basenamep = basename (destPathp);
	TRACEs (basenamep)

	if (basenamep == destPathp)
		goto	_open;

	dirnamep = dirname (destPathp);	/*  dirname() can destroy path  */
	TRACEs (dirnamep)

	if (mkdirp (dirnamep, 0755) < 0 && errno != EEXIST)
	{
		TrapError (NonFatal, Unix, FnName, "mkdirp");
		goto	errorReturn_1;
	}
	free (destPathp);
	if ((destPathp = strdup (fragMsgp->destPathp)) == NULL)
	{
		TrapError (Fatal, Unix, FnName, "strdup");
	}

_open:
	TRACEP ("_open")
	TRACEs (destPathp)
	if ((fd = open (destPathp, O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0)
	{
		TrapError (NonFatal, Unix, FnName, "open");
		goto	errorReturn_1;
	}


	for (;;)
	{
		nBytes = write (fd, fragMsgp->fragp, fragMsgp->fraglen);

		if (nBytes == -1)
		{
			TrapError (NonFatal, Unix, FnName, "write");
			goto	errorReturn_2;
		}
		if (nBytes != fragMsgp->fraglen)
		{
			TrapError (NonFatal, Unix, FnName, "write");
			goto	errorReturn_2;
		}
		if (fragMsgp->endOfFile)
			break;
	
		FreeNetworkMsg (FileFragmentMsg, (void **)&fragMsgp);

		networkMsgTagp =
		ReceiveNetworkMsg (cip, &jobControlMsgp, (void **)&fragMsgp);

		if (networkMsgTagp == NULL ||
		    networkMsgTagp->msgType != FileFragmentMsg)
		{
			goto	errorReturn_2;
		}
	}


	/*  File attribute mapping. */
	(void)	close (fd);

	TRACEs (fragMsgp->ownerp)
	TRACEd (fragMsgp->uid)
	TRACEd (fragMsgp->gid)
	TRACEd (fragMsgp->lid)
	TRACEo (fragMsgp->mode)

	/*  Mapping owner (namemap) takes precedence over uid         */
	/*  mapping.  If something is not specified then it           */
	/*  cannot be mapped and remains at its current setting.      */
	/*  Things that are specified but fail to map cause an error. */

	if (fragMsgp->ownerp)
	{
		TRACEP ("Map logname.")
		(void)	sprintf (outbuf, "%s@%s",
			fragMsgp->ownerp, cip->sip->systemName_p);
		TRACEs (outbuf)
		if (namemap (DEFAULT_SCHEME, outbuf, inbuf) < 0)
		{
			goto _DefaultOwner;
		}
		pwp = getpwnam (inbuf);
		if (! pwp)
		{
			TrapError (NonFatal, Unix, FnName, "getpwnam");
			goto    errorReturn_2;
                }
		endpwent ();
		local_uid = pwp->pw_uid;
		local_gid = pwp->pw_gid;
		goto	_lid;
	}
_DefaultOwner:
	if (DefaultOwnerp)
	{
		local_uid = DefaultUID;
		local_gid = DefaultGID;
		goto	_lid;
	}
	if (fragMsgp->uid > (uid_t) -1)
	{
		TRACEP ("Map uid.")
		(void)	sprintf (outbuf, "%d@%s",
			fragMsgp->uid, cip->sip->systemName_p);
		TRACEs (outbuf)
		if (attrmap ("UID", outbuf, inbuf) < 0)
		{
			goto	_DefaultUID;
		}
		(void)	sscanf (inbuf, "%d", &local_gid);
		if (fragMsgp->gid != (uid_t) -1)
		{
			TRACEP ("Map uid.")
			(void)	sprintf (outbuf, "%d@%s",
				fragMsgp->uid, cip->sip->systemName_p);
			TRACEs (outbuf)
			if (attrmap ("GID", outbuf, inbuf) < 0)
			{
				goto	_DefaultGID;
			}
			(void)	sscanf (inbuf, "%d", &local_gid);
		}
		goto	_lid;
	}
_DefaultUID:
	if (DefaultUID != (uid_t) -1)
	{
		local_uid = DefaultUID;
	}
_DefaultGID:
	if (DefaultGID != (uid_t) -1)
	{
		local_gid = DefaultGID;
		goto	_lid;
	}
	goto	errorReturn_2;
_lid:
	if (fragMsgp->lid != (level_t) -1)
	{
		TRACEP ("Map lid.")
		(void)	sprintf (outbuf, "%d@%s",
			fragMsgp->lid, cip->sip->systemName_p);
		TRACEs (outbuf)
		if (attrmap ("LID", outbuf, inbuf) < 0)
		{
			goto	_DefaultLID;
                }
		TRACEs (inbuf)
		(void)	sscanf (inbuf, "%d", &local_lid);
		TRACEd (local_lid)

		if (lvlvalid (&local_lid) < 0)
		{
			goto	_DefaultLID;
                }
		goto	_mode;
	}
_DefaultLID:
	if (DefaultLID != (level_t) -1)
	{
		local_lid = DefaultLID;

		while ((n=lvlfile (destPathp, MAC_SET, &local_lid)) < 0
			&& errno == EINTR);

        	if (n < 0 && errno != ENOSYS)
		{
                        TrapError (NonFatal, Unix, FnName, "lvlfile");
                        goto    errorReturn_2;
                }
		goto	_mode;
	}
	goto	errorReturn_2;
_mode:
	if (fragMsgp->mode != (mode_t) -1)
	{
		local_mode = fragMsgp->mode;
	}
	else
	if (DefaultMode != (mode_t) -1)
	{
		local_mode = DefaultMode;
	}

	/* Do our file attribute changes. */
	if (local_mode != (mode_t) -1)
	{
		while ((n=chmod (destPathp, local_mode)) < 0 &&
			errno == EINTR);
		if (n < 0)
		{
                       	TrapError (NonFatal, Unix, FnName, "chmod");
                       	goto    errorReturn_2;
               	}
	}
	if (local_lid != (level_t) -1)
	{
		while ((n=lvlfile (destPathp, MAC_SET, &local_lid)) < 0
			&& errno == EINTR);

        	if (n < 0 && errno != ENOSYS)
		{
                        TrapError (NonFatal, Unix, FnName, "lvlfile");
                        goto    errorReturn_2;
                }
	}
	if (local_uid != (uid_t) -1 || local_gid != -1)
	{
		while ((n=chown (destPathp, local_uid, local_gid)) < 0
			&& errno == EINTR);
		if (n < 0)
		{
			TrapError (NonFatal, Unix, FnName, "chown");
			goto    errorReturn_2;
        	}
	}

	/* Normal cleanup and return */
	FreeNetworkMsg (FileFragmentMsg, (void **)&fragMsgp);
	return	destPathp;


	/*  Error clean-up and return. */
errorReturn_2:
	(void)	close (fd);
	(void)	unlink (destPathp);

errorReturn_1:
	free (destPathp);
	FreeNetworkMsg (FileFragmentMsg, (void **)&fragMsgp);
	return	NULL;
}



boolean
SendData (cip, endOfJob, data_p, sizeOfData)
connectionInfo *cip;
boolean endOfJob;
void *data_p;
int sizeOfData;
{
	dataPacketMsg msg;
	static char FnName [] = "SendData";


	ENTRYP
	TRACEs (data_p)
	TRACE  (sizeOfData)

	msg.endOfPacket   = (int) True;
	msg.data.data_len = sizeOfData;
	msg.data.data_val = (char *) data_p;


	JobControl.controlCode = NormalJobMsg;
	JobControl.endOfJob    = endOfJob;

	if (! EncodeNetworkMsgTag (cip, DataPacketMsg))
	{
		return	False;
	}
	if (! PutIntoXdrStream (cip, xdr_dataPacketMsg, &msg)) 
	{
		return	False;
	}
	if (! _SendNetworkMsg (cip))
	{
		return	False;
	}
	return	True;
}



void
SetJobPriority (priority)
int priority;
{
	JobControl.priority = priority;
	return;
}



static int MsgId = 0;


boolean
EncodeNetworkMsgTag (cip, msgType)
connectionInfo *cip;
networkMsgType msgType;
{
	static char FnName [] = "EncodeNetworkMsgTag";


	ENTRYP

	/*  The first message to be sent on this connection is always   */
	/*  a version 1.1 message for backward compatability.		*/
	/*  Until we recieve a message which should be a       		*/
	/*  SystemId message or a JobControl message we must send       */
	/*  version 1.1 messages.  					*/

	if (! cip->receiveCount)
	{
		cip->logicalMsgVersion [0] = MSGS_VERSION_MAJOR;
		cip->logicalMsgVersion [1] = MSGS_VERSION_MINOR;

		TRACEP ("Encoding first (version 1.1) message-tag.")
		return	EncodeNetworkMsgTag_1_1 (cip, msgType);
	}
	else
	switch (cip->logicalMsgVersion [1]) {
	case	1:
		TRACEP ("Encoding version 1.1 message-tag.")
		return	EncodeNetworkMsgTag_1_1 (cip, msgType);
		break;

	case	2:
		TRACEP ("Encoding version 1.2 message-tag.")
		return	EncodeNetworkMsgTag_1_2 (cip, msgType);
		break;

	default:
		return	False;
	}
	/*NOTREACHED*/
}

static boolean
EncodeNetworkMsgTag_1_1 (cip, msgType)
connectionInfo *cip;
networkMsgType msgType;
{
	static char FnName [] = "EncodeNetworkMsgTag_1_1";
	static networkMsgTag_1_1 NetworkMsgTag;
	static jobControl_1_1 JobControl_1_1;


	ENTRYP

	NetworkMsgTag.versionMajor = cip->logicalMsgVersion [0];
	NetworkMsgTag.versionMinor = cip->logicalMsgVersion [1];
	NetworkMsgTag.routeControl.sysId = 0;
	NetworkMsgTag.routeControl.msgId = ++MsgId;
	NetworkMsgTag.msgType = msgType;

	if (msgType == SystemIdMsg)
		NetworkMsgTag.jobControlp = NULL;
	else
	{
		JobControl_1_1.controlCode = JobControl.controlCode;
		JobControl_1_1.priority = JobControl.priority;
		JobControl_1_1.endOfJob = JobControl.endOfJob;
		JobControl_1_1.jobId  = JobControl.jobId;
		JobControl_1_1.timeStamp = JobControl.timeStamp;
		NetworkMsgTag.jobControlp = &JobControl_1_1;
	}
		
	ResetXdrStream (cip, XDR_ENCODE);
	if (! PutIntoXdrStream (cip, xdr_networkMsgTag_1_1, &NetworkMsgTag))
	{
		TRACEP ("PutIntoXdrStream (NetworkMsgTag) failed.")
		EXITP
		return	False;
	}
	EXITP
	return	True;
}

static boolean
EncodeNetworkMsgTag_1_2 (cip, msgType)
connectionInfo*cip;
networkMsgType msgType;
{
	static char FnName [] = "EncodeNetworkMsgTag_1_2";
	static networkMsgTag_1_2 NetworkMsgTag;


	ENTRYP

	NetworkMsgTag.versionMajor = cip->logicalMsgVersion [0];
	NetworkMsgTag.versionMinor = cip->logicalMsgVersion [1];
	NetworkMsgTag.msgType = msgType;

	NetworkMsgTag.routeControl.sysId = 0;
	NetworkMsgTag.routeControl.msgId = ++MsgId;

	ResetXdrStream (cip, XDR_ENCODE);
	if (! PutIntoXdrStream (cip, xdr_networkMsgTag_1_2, &NetworkMsgTag))
	{
		TRACEP ("PutIntoXdrStream (NetworkMsgTag) failed.")
		EXITP
		return	False;
	}
	if (msgType != SystemIdMsg)
		if (! PutIntoXdrStream (cip, xdr_jobControlMsg, &JobControl))
		{
			TRACEP ("PutIntoXdrStream (JobControl) failed.")
			EXITP
			return	False;
		}
		
	EXITP
	return	True;
}

boolean
JobPending (cip)
connectionInfo *cip;
{
	if (! cip)
	{
		errno = EINVAL;
		return	False;
	}
	if (cip->fd < 0)
	{
		errno = ENOLINK;
		return	False;
	}
	return	JobPendingFlag;
}



int
NegotiateJobClearance (cip)
connectionInfo*cip;
{
	void *netMsgp;
	networkMsgTag *netMsgTagp;
	jobControlMsg *jobControlMsgp;

	static int JobId = 0;
	static char FnName [] = "NegotiateJobClearance";

	ENTRYP

	JobControl.endOfJob = False;

	TRACEP ("Sending 'RequestToSendJob'.")
	if (! SendJobControlMsg (cip, RequestToSendJob))
	{
		EXITP
		return	-1;
	}
receiveNetworkMsg:
	netMsgTagp = ReceiveNetworkMsg (cip, &jobControlMsgp, &netMsgp);
	if (! netMsgTagp)
	{
		EXITP
		return	-1;
	}
	if (netMsgTagp->msgType != JobControlMsg)
	{
		EXITP
		return	-1;
	}
	switch (jobControlMsgp->controlCode)
	{
	case	ClearToSendJob:
		TRACEP ("Received 'ClearToSendJob'.")
		break;

	case	RequestDenied:
		TRACEP ("Received 'RequestDenied'.");
		errno = EBUSY;
		EXITP
		return	-1;

	case	RequestToSendJob:
		JobPendingFlag = True;
		TRACEP ("Received 'RequestToSendJob'.")
		if (jobControlMsgp->priority > JobControl.priority)
		{
			TRACEP ("Sending 'ClearToSendJob'.")
			JobControl.controlCode = ClearToSendJob;
			(void)	EncodeNetworkMsgTag (cip, JobControlMsg);
			(void)	_SendNetworkMsg (cip);
			goto	receiveNetworkMsg;
		}
		else
		{
			TRACEP ("Sending 'RequestDenied'.")
			JobControl.controlCode = RequestDenied;
			(void)	EncodeNetworkMsgTag (cip, JobControlMsg);
			(void)	_SendNetworkMsg (cip);
			goto	receiveNetworkMsg;
		}
		break;

	default:
		TRACE (jobControlMsgp->controlCode)
		TrapError (NonFatal, Internal, FnName,
		"Network messages out of sync.");
		EXITP
		return	-1;
	}
	EXITP
	return	++JobId;
}



boolean
SendJobControlMsg (cip, controlCode)
connectionInfo *cip;
jobControlCode controlCode;
{
	static char FnName [] = "SendJobControlMsg";

	ENTRYP

	switch (cip->logicalMsgVersion [1]) {
	case	1:
		return	SendJobControlMsg_1_1 (cip, controlCode);

	case	2:
		return	SendJobControlMsg_1_2 (cip, controlCode);

	default:
		errno = EINVAL;
		return	False;
	}
	/*NOTREACHED*/
}


static boolean
SendJobControlMsg_1_1 (cip, controlCode)
connectionInfo *cip;
jobControlCode controlCode;
{
	static char FnName [] = "SendJobControlMsg_1_1";

	ENTRYP

	JobControl.controlCode = controlCode;

	if (! EncodeNetworkMsgTag (cip, JobControlMsg))
	{
		return	False;
	}
	if (! _SendNetworkMsg (cip))
	{
		return	False;
	}
	return	True;
}


static boolean
SendJobControlMsg_1_2 (cip, controlCode)
connectionInfo *cip;
jobControlCode controlCode;
{
	static char FnName [] = "SendJobControlMsg_1_2";

	ENTRYP

	JobControl.controlCode = controlCode;

	if (! EncodeNetworkMsgTag (cip, JobControlMsg))
	{
		return	False;
	}
	if (! _SendNetworkMsg (cip))
	{
		return	False;
	}
	return	True;
}



boolean
SendSystemIdMsg (cip, data_p, sizeOfData)
connectionInfo *cip;
void *data_p;
int sizeOfData;
{
	int jobId;
	systemIdMsg msg;
	struct utsname utsName;
	static char FnName[] = "SendSystemIdMsg";


	(void)	uname (&utsName);
	msg.systemNamep = utsName.nodename;
	msg.data.data_val = (char *) data_p;
	msg.data.data_len = sizeOfData;

	if (! EncodeNetworkMsgTag (cip, SystemIdMsg))
	{
		return	False;
	}
	if (! PutIntoXdrStream (cip, xdr_systemIdMsg, &msg)) 
	{
		return	False;
	}
	if (! _SendNetworkMsg (cip))
	{
		return	False;
	}
	return	True;
}


boolean
SendFileFragmentMsg (cip, endOfJob, fileFragp)
connectionInfo *cip;
boolean endOfJob;
fileFragmentMsg *fileFragp;
{
	fileFragmentMsg_1_1 f11;
	fileFragmentMsg_1_2 f12;
	static char FnName [] = "SendFileFragmentMsg";


	JobControl.controlCode = NormalJobMsg;
	JobControl.endOfJob = endOfJob;

	if (! EncodeNetworkMsgTag (cip, FileFragmentMsg))
	{
		return	False;
	}
	switch (cip->logicalMsgVersion [1]) {
	case	1:
		TRACEP ("Sending file-fragment 1.1")
		f11.endOfFile	= fileFragp->endOfFile;
		f11.sizeOfFile	= fileFragp->sizeOfFile;
		f11.destPathp	= fileFragp->destPathp;
		f11.fragment.fragment_len = fileFragp->fraglen;
		f11.fragment.fragment_val = (char *) fileFragp->fragp;
		if (! PutIntoXdrStream (cip, xdr_fileFragmentMsg_1_1, &f11)) 
		{
			return	False;
		}
		break;
	case	2:
		TRACEP ("Sending file-fragment 1.2")
		f12.endOfFile	= fileFragp->endOfFile;
		f12.uid		= fileFragp->uid;
		f12.gid		= fileFragp->gid;
		f12.lid		= fileFragp->lid;
		f12.mode	= fileFragp->mode;
		f12.sizeOfFile	= fileFragp->sizeOfFile;
		f12.ownerp	= fileFragp->ownerp;
		f12.destPathp	= fileFragp->destPathp;
		f12.fragment.fragment_len = fileFragp->fraglen;
		f12.fragment.fragment_val = (char *) fileFragp->fragp;
		if (! PutIntoXdrStream (cip, xdr_fileFragmentMsg_1_2, &f12)) 
		{
			return	False;
		}
		break;
	default:
		errno = EINVAL;
		return	False;
	}
	if (! _SendNetworkMsg (cip))
	{
		return	False;
	}
	return	True;
}

networkMsgTag *
ReceiveNetworkMsg (cip, jobControlMsgpp, networkMsgpp)
connectionInfo*cip;
jobControlMsg **jobControlMsgpp;
void **	networkMsgpp;
{
	static char FnName [] = "ReceiveNetworkMsg";


	if (! _ReceiveNetworkMsg (cip))
	{
		return	NULL;
	}
	return	DecodeNetworkMsg (cip, jobControlMsgpp, networkMsgpp);
}

networkMsgTag *
DecodeNetworkMsg (cip, jobControlMsgpp, networkMsgpp)
connectionInfo *cip;
jobControlMsg **jobControlMsgpp;
void **networkMsgpp;
{
	networkMsgTag *tp;
	networkMsgTag_1_1 *t11p;
	networkMsgTag_1_2 *t12p;
	static char FnName [] = "DecodeNetworkMsg";


	ENTRYP

	/*  The first message received is always a version 1.1  */
	/*  message even if it isn't tagged version 1.1.  This  */
	/*  is because the first message specifies the version  */
	/*  for all subsequent messages.  			*/

	if (cip->receiveCount == 1)
	{
		TRACEP ("Received first message.")
		cip->logicalMsgVersion [0] = 1;
		cip->logicalMsgVersion [1] = 1;

		t11p =
		DecodeNetworkMsg_1_1 (cip, jobControlMsgpp, networkMsgpp);

		tp = (networkMsgTag *) calloc (1, sizeof (networkMsgTag));
		tp->msgType = t11p->msgType;
		tp->versionMajor = t11p->versionMajor;
		tp->versionMinor = t11p->versionMinor;

		cip->logicalMsgVersion [0] = t11p->versionMajor;
		cip->logicalMsgVersion [1] = t11p->versionMinor;
		TRACEd (cip->logicalMsgVersion [0])
		TRACEd (cip->logicalMsgVersion [1])
	}
	else
	switch (cip->logicalMsgVersion [1]) {
	case	1:
		TRACEP ("Received version 1.1 message.")
		t11p =
		DecodeNetworkMsg_1_1 (cip, jobControlMsgpp, networkMsgpp);
		tp = (networkMsgTag *) calloc (1, sizeof (networkMsgTag));
		tp->msgType = t11p->msgType;
		tp->versionMajor = t11p->versionMajor;
		tp->versionMinor = t11p->versionMinor;
		break;

	case	2:
		TRACEP ("Received version 1.2 message.")
		t12p =
		DecodeNetworkMsg_1_2 (cip, jobControlMsgpp, networkMsgpp);
		tp = (networkMsgTag *) calloc (1, sizeof (networkMsgTag));
		tp->msgType = t12p->msgType;
		tp->versionMajor = t12p->versionMajor;
		tp->versionMinor = t12p->versionMinor;
		break;

	default:
		EXITP
		errno = EINVAL;
		return	NULL;
	}
	EXITP
	return	tp;
}
static
#ifdef __STDC__
networkMsgTag_1_1 *
#else
networkMsgTag *
#endif	/* __STDC__ */
DecodeNetworkMsg_1_1 (cip, jobControlMsgpp, networkMsgpp)
connectionInfo *cip;
jobControlMsg **jobControlMsgpp;
void **networkMsgpp;
{
	#define	ALLOCATE_NETWORK_MSG(msgType)	\
		*networkMsgpp = (void *)  calloc (1, sizeof (msgType));	\
		if (! *networkMsgpp) \
			TrapError (Fatal, Unix, FnName, "calloc")
	
	#define	DECODE_NETWORK_MSG(xdrFnName)	\
		if (! GetFromXdrStream (cip, xdrFnName, *networkMsgpp))\
			return	NULL
		

	static char FnName [] = "DecodeNetworkMsg_1_1";
	static networkMsgTag_1_1 NetworkMsgTag;


	ENTRYP

	if (NetworkMsgTag.jobControlp)
	{
		free (NetworkMsgTag.jobControlp);
		NetworkMsgTag.jobControlp = NULL;
	}
	ResetXdrStream (cip, XDR_DECODE);
	if (! GetFromXdrStream (cip, xdr_networkMsgTag_1_1, &NetworkMsgTag))
	{
		TRACEP ("GetFromXdrStream (NetworkMsgTag) failed.")
		EXITP
		return	NULL;
	}
	TRACE (NetworkMsgTag.msgType)
	if (NetworkMsgTag.msgType == SystemIdMsg)
	{
		*jobControlMsgpp = NULL;
	}
	else
	{
                *jobControlMsgpp = (jobControlMsg *)
			calloc (1, sizeof (jobControlMsg));
		if (! *jobControlMsgpp)
                	TrapError (Fatal, Unix, FnName, "calloc");
                (*jobControlMsgpp)->controlCode =
			NetworkMsgTag.jobControlp->controlCode;
                (*jobControlMsgpp)->priority =
                        NetworkMsgTag.jobControlp->priority;
                (*jobControlMsgpp)->jobId =
                        NetworkMsgTag.jobControlp->jobId;
                (*jobControlMsgpp)->timeStamp =
                        NetworkMsgTag.jobControlp->timeStamp;
                (*jobControlMsgpp)->endOfJob =
                        NetworkMsgTag.jobControlp->endOfJob;
	}
	switch (NetworkMsgTag.msgType) {
	case	JobControlMsg:
		*networkMsgpp = NULL;
		break;

	case	SystemIdMsg:
		TRACEP ("Decoding 'SystemIdMsg'.")
		ALLOCATE_NETWORK_MSG (systemIdMsg);
		DECODE_NETWORK_MSG (xdr_systemIdMsg);
		break;

	case	PacketBundleMsg:
		TRACEP ("Decoding 'PacketBundleMsg'.")
		ALLOCATE_NETWORK_MSG (packetBundleMsg);
		DECODE_NETWORK_MSG (xdr_packetBundleMsg);
		break;

	case	DataPacketMsg:
		TRACEP ("Decoding 'DataPacketMsg'.")
		ALLOCATE_NETWORK_MSG (dataPacketMsg);
		DECODE_NETWORK_MSG (xdr_dataPacketMsg);
		break;

	case	FileFragmentMsg:
		TRACEP ("Decoding 'FileFragmentMsg'.")
		ALLOCATE_NETWORK_MSG (fileFragmentMsg_1_1);
		DECODE_NETWORK_MSG (xdr_fileFragmentMsg_1_1);
		NormalizeFileFragmentMsg_1_1 (networkMsgpp);
		break;

	default:
		TrapError (NonFatal, Internal, FnName,
			"Unknown network message.  Could not decode.");
		return	NULL;
	}
	EXITP
	return	&NetworkMsgTag;
}

#undef	DECODE_NETWORK_MSG
#undef	ALLOCATE_NETWORK_MSG
static
#ifdef __STDC__
networkMsgTag_1_2 *
#else
networkMsgTag *
#endif	/* __STDC__ */
DecodeNetworkMsg_1_2 (cip, jobControlMsgpp, networkMsgpp)
connectionInfo *cip;
jobControlMsg **jobControlMsgpp;
void **networkMsgpp;
{
	#define	ALLOCATE_NETWORK_MSG(msgType)	\
		*networkMsgpp = (void *)  calloc (1, sizeof (msgType));	\
		if (! *networkMsgpp) \
			TrapError (Fatal, Unix, FnName, "calloc")

	#define	DECODE_NETWORK_MSG(xdrFnName)	\
		if (! GetFromXdrStream (cip, xdrFnName, *networkMsgpp))\
			return	NULL
			

	static char FnName [] = "DecodeNetworkMsg_1_2";
	static networkMsgTag_1_2 NetworkMsgTag;


	EXITP

	ResetXdrStream (cip, XDR_DECODE);
	if (! GetFromXdrStream (cip, xdr_networkMsgTag_1_2, &NetworkMsgTag))
	{
		TRACEP ("GetFromXdrStream (NetworkMsgTag) failed.")
		return	NULL;
	}
	TRACE (NetworkMsgTag.msgType)
	if (NetworkMsgTag.msgType == SystemIdMsg)
	{
		*jobControlMsgpp = NULL;
	}
	else
	{
                *jobControlMsgpp = (jobControlMsg *)
			calloc (1, sizeof (jobControlMsg));
		if (! *jobControlMsgpp)
                	TrapError (Fatal, Unix, FnName, "calloc");
		if (! GetFromXdrStream (cip, xdr_jobControlMsg,
			*jobControlMsgpp))
			return	NULL;
	}
	switch (NetworkMsgTag.msgType) {
	case	JobControlMsg:
		*networkMsgpp = NULL;
		break;

	case	SystemIdMsg:
		TRACEP ("Decoding 'SystemIdMsg'.")
		ALLOCATE_NETWORK_MSG (systemIdMsg);
		DECODE_NETWORK_MSG (xdr_systemIdMsg);
		break;

	case	PacketBundleMsg:
		TRACEP ("Decoding 'PacketBundleMsg'.")
		ALLOCATE_NETWORK_MSG (packetBundleMsg);
		DECODE_NETWORK_MSG (xdr_packetBundleMsg);
		break;

	case	DataPacketMsg:
		TRACEP ("Decoding 'DataPacketMsg'.")
		ALLOCATE_NETWORK_MSG (dataPacketMsg);
		DECODE_NETWORK_MSG (xdr_dataPacketMsg);
		break;

	case	FileFragmentMsg:
		TRACEP ("Decoding 'FileFragmentMsg'.")
		ALLOCATE_NETWORK_MSG (fileFragmentMsg_1_2);
		DECODE_NETWORK_MSG (xdr_fileFragmentMsg_1_2);
		NormalizeFileFragmentMsg_1_2 (networkMsgpp);
		break;

	default:
		TrapError (NonFatal, Internal, FnName,
			"Unknown network message.  Could not decode.");
		return	NULL;
	}
	EXITP
	return	&NetworkMsgTag;
}

#undef	DECODE_NETWORK_MSG
#undef	ALLOCATE_NETWORK_MSG
#ifdef	__STDC__
static void
NormalizeFileFragmentMsg_1_1 (void **p)
#else
static void
NormalizeFileFragmentMsg_1_1 (p)
void **p;
#endif
{
	fileFragmentMsg_1_1 *f11p = (fileFragmentMsg_1_1 *) *p;
	fileFragmentMsg *fp;
	static char FnName [] = "NormalizeFileFragmentMsg_1_1";


	ENTRYP
	fp = (fileFragmentMsg *) calloc (1, sizeof (fileFragmentMsg));
	if (! fp)
		TrapError (Fatal, Unix, FnName, "calloc");

	fp->endOfFile	= f11p->endOfFile;
	fp->sizeOfFile	= f11p->sizeOfFile;
	fp->destPathp	= f11p->destPathp;
	fp->fraglen	= f11p->fragment.fragment_len;
	fp->fragp	= (caddr_t *) f11p->fragment.fragment_val;

	fp->uid = (uid_t) -1;
	fp->gid = (uid_t) -1;
	fp->lid = (level_t) -1;
	fp->mode = (mode_t) -1;
	fp->ownerp = NULL;;

	free (f11p);
	*p = (void *) fp;
	EXITP
	return;
}


#ifdef	__STDC__
static void
NormalizeFileFragmentMsg_1_2 (void **p)
#else
static void
NormalizeFileFragmentMsg_1_2 (p)
void **p;
#endif
{
	fileFragmentMsg_1_2 *f12p = (fileFragmentMsg_1_2 *) *p;
	fileFragmentMsg *fp;
	static char FnName [] = "NormalizeFileFragmentMsg_1_2";


	ENTRYP
	fp = (fileFragmentMsg *) calloc (1, sizeof (fileFragmentMsg));
	if (! fp)
		TrapError (Fatal, Unix, FnName, "calloc");

	fp->endOfFile	= f12p->endOfFile;
	fp->uid		= f12p->uid;
	fp->gid		= f12p->gid;
	fp->lid		= f12p->lid;
	fp->mode	= f12p->mode;
	fp->sizeOfFile	= f12p->sizeOfFile;
	fp->ownerp	= f12p->ownerp;
	fp->destPathp	= f12p->destPathp;
	fp->fraglen	= f12p->fragment.fragment_len;
	fp->fragp	= (caddr_t *) f12p->fragment.fragment_val;

	free (f12p);
	*p = (void *) fp;
	EXITP
	return;
}


#ifdef	__STDC__
boolean
SetDefaultFileAttributes (char *ownerp, uid_t uid, uid_t gid,
	level_t lid, mode_t mode)
#else
boolean
SetDefaultFileAttributes (ownerp, uid, gid, lid, mode)
char *ownerp;
uid_t uid, gid;
level_t lid;
mode_t mode;
#endif
{
	struct passwd *pwp;
	level_t	chkmac;
	static char FnName [] = "SetDefaultFileAttributes";

	ENTRYP

	/* 'ownerp' and 'uid/gid' are mutually exclusive.  */
	if (ownerp)
	{
		pwp = getpwnam (ownerp);
		endpwent ();
		if (pwp)
		{
			if (DefaultOwnerp)
				free (DefaultOwnerp);
			DefaultOwnerp = strdup (pwp->pw_name);
			if (! DefaultOwnerp)
				TrapError (Fatal, Unix, FnName, "strdup");
			DefaultUID = pwp->pw_uid;
			DefaultGID = pwp->pw_gid;
		}
		else
		{
			EXITP
			return	False;
		}
	}
	else

	/*  If uid is specified then set the default to what is */
	/*  specified if it is a valid uid.  			*/
	if (uid != (uid_t) -1)
	{
		pwp = getpwuid (uid);
		if (pwp)
		{
			if (DefaultOwnerp)
				free (DefaultOwnerp);
			DefaultOwnerp = strdup (pwp->pw_name);
			if (! DefaultOwnerp)
				TrapError (Fatal, Unix, FnName, "strdup");
			DefaultUID = uid;
			DefaultGID = gid;
		}
		else
		{
			endpwent ();
			EXITP
			return	False;
		}
		endpwent ();
	}
	else
	/* Reset the uid & gid.*/
	{
		if (DefaultOwnerp)
			free (DefaultOwnerp);
		DefaultOwnerp = (char *) 0;
		DefaultUID = (uid_t) -1;
		DefaultGID = (uid_t) -1;
	}
	TRACEs (DefaultOwnerp)
	TRACEd (DefaultUID)
	TRACEd (DefaultGID)
	if (lid != (level_t) -1)
	{
		/* Check whether MAC id installed */

		if (lvlproc (MAC_GET, &chkmac) == 0)
		{	
			if (lvlvalid (&lid) < 0)
			{
				EXITP
				return	False;
			}
		}
		else
		{
			chkmac = 0;
			if (errno != ENOPKG)
			{
				EXITP
				return  False;
			}
		}
		DefaultLID = lid;
	}
	else
		DefaultLID = (level_t) -1;
	TRACEd (DefaultLID)

	if (mode != (mode_t) -1)
	{
		if (mode > 0777 || mode < 0)
		{
			EXITP
			return	False;
		}
		DefaultMode = mode;
	}
	else
		DefaultMode = (mode_t) -1;
	TRACEo (DefaultMode)

	EXITP
	return;
}



void
FreeNetworkMsg (msgType, networkMsgpp)
networkMsgType msgType;
void **networkMsgpp;
{
	static char FnName[] = "FreeNetworkMsg";


	if (networkMsgpp == NULL || *networkMsgpp == NULL)
		return;

	free (*networkMsgpp);

	*networkMsgpp = NULL;

	return;
}

dataPacket *
NewDataPacket (size)
int size;
{
	register dataPacket *dataPacket_p;
	static char FnName [] = "NewDataPacket";


	dataPacket_p = (dataPacket *)  calloc (1, sizeof (dataPacket));

	if (dataPacket_p == NULL)
		TrapError (Fatal, Unix, FnName, "calloc");

	if (size <= 0)
		return	dataPacket_p;

	dataPacket_p->data_p = (void *) calloc (size, sizeof (char));

	if (dataPacket_p->data_p == NULL)
		TrapError (Fatal, Unix, FnName, "calloc");

	dataPacket_p->size = size;

	return	dataPacket_p;
}



void
FreeDataPacket (dataPacket_pp)
register dataPacket **dataPacket_pp;
{
	if (dataPacket_pp == NULL || *dataPacket_pp == NULL)
		return;

	if ((*dataPacket_pp)->data_p != NULL) {
		free ((*dataPacket_pp)->data_p);
		(*dataPacket_pp)->data_p = NULL;
	}
	free (*dataPacket_pp);

	*dataPacket_pp = NULL;

	return;
}
