#ident	"@(#)siserver:os/connection.c	1.7"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/***********************************************************
Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: connection.c,v 1.120 89/11/13 11:53:33 rws Exp $ */

/*****************************************************************
 *  Stuff to create connections --- OS dependent
 *
 *      EstablishNewConnections, CreateWellKnownSockets, ResetWellKnownSockets,
 *      CloseDownConnection, CheckConnections, AddEnabledDevice,
 *	RemoveEnabledDevice, OnlyListToOneClient,
 *      ListenToAllClients,
 *
 *      (WaitForSomething is in its own file)
 *
 *      In this implementation, a client socket table is not kept.
 *      Instead, what would be the index into the table is just the
 *      file descriptor of the socket.  This won't work for if the
 *      socket ids aren't small nums (0 - 2^8)
 *
 *****************************************************************/

#undef NULL
#include "X.h"
#include "Xproto.h"
#include <sys/param.h>
#include <errno.h>
#include "Xos.h"			/* for strings, file, time */

#include <signal.h>
#include <setjmp.h>

#include <stdio.h>
#ifdef SVR4
#include <sys/uio.h>
#endif
#include "osstruct.h"
#include "osdep.h"
#include "opaque.h"
#include "dixstruct.h"

#include <stdio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <string.h>
#if defined(SYSV) || defined(SVR4)
/* These are properly declared in BSD environments */
extern char *index();
extern char *rindex();
#endif /* SYSV */
#include "osstruct.h"
#include "Xstreams.h"
#include "opaque.h"

#include "dixstruct.h"

#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<ctype.h>
#include	<dirent.h>

typedef long CCID;      /* mask of indices into client socket table */

/*  SunRiver work, used to be char *	*/
extern char display[];			/* The display number */

extern int nClients;

int lastfdesc;			/* maximum file descriptor */

long WellKnownConnections;	/* Listener mask */
long EnabledDevices[mskcnt];	/* mask for input devices that are on */
long AllSockets[mskcnt];	/* select on this */
long AllClients[mskcnt];	/* available clients */
long LastSelectMask[mskcnt];	/* mask returned from last select call */
long ClientsWithInput[mskcnt];	/* clients with FULL requests in buffer */
long YieldedClientsWithInput[mskcnt];   /* clients with FULL requests */
     				/* in buffer that have yielded */
long ClientsBlocked[mskcnt];/* clients who cannot receive output */
long MaxClients = MAXSOCKS ;
long OutputBufferSize = BUFSIZ; /* output buffer size (must be > 0) */
long NConnBitArrays = mskcnt;
long FirstClient;

static Bool SendSignal;		/* send SIGUSR1 to parent process */
static int ParentProcess;

static Bool debug_conns = FALSE;

static int SavedAllClients[mskcnt];
static int SavedAllSockets[mskcnt];
static int SavedClientsWithInput[mskcnt];
static int SavedYieldedClientsWithInput[mskcnt];
static int GrabInProgress = 0;

int ConnectionTranslation[MAXSOCKS];
extern ClientPtr NextAvailableClient();

extern char TypeOfStream[];
extern Xstream xstream[]; 

extern int AutoResetServer();
extern void GiveUp();
extern XID CheckAuthorization();
static void CloseDownFileDescriptor(), ErrorConnMax();
extern void FreeOsBuffers(), ResetOsBuffers();

#ifdef XDMCP
void XdmcpOpenDisplay(), XdmcpInit(), XdmcpReset(), XdmcpCloseDisplay();
#endif



/*****************
 * CreateWellKnownSockets
 *    At initialization, create the sockets to listen on for new clients.
 *****************/

void
CreateWellKnownSockets()
{
    int		request, i;
    struct stat sbuf;

    CLEARBITS(AllSockets);
    CLEARBITS(AllClients);
    CLEARBITS(LastSelectMask);
    CLEARBITS(ClientsWithInput);
    CLEARBITS(YieldedClientsWithInput);

    for (i=0; i<MAXSOCKS; i++) {
	ConnectionTranslation[i] = 0;
	TypeOfStream[i] = CLOSED_STREAM;
    }
    
    lastfdesc = ulimit(4, (long)0) - 1;  /* Returns total # of FDs available */

    if (lastfdesc > MAXSOCKS)
    {
	lastfdesc = MAXSOCKS;
	if (debug_conns)
	    ErrorF( "GOT TO END OF SOCKETS %d\n", MAXSOCKS);
    }

    WellKnownConnections = 0;

    if ((request = (*xstream[X_LOCAL_STREAM].SetupTheListener)
					(display, "local")) < 0)
    {
       	if(request == -1)
        	attexit(1);
    }
    else
    {
        WellKnownConnections |= (1<<request);
        TypeOfStream[request] = X_LOCAL_STREAM;
	NetworkInfo->_npeers = ulimit(4, (long)0);
    }

#ifndef SVR4
    {
 	extern char *GetXWINHome ();
	char *dir = GetXWINHome (XNETSPECDIR);
	struct dirent *dentry;
	DIR *dirf;
	int	n;

	if ((dirf = opendir(dir)) == NULL) {
#ifdef DEBUG
		fprintf(stderr, "Cannot access directory %s\n", dir);
#endif
	}
        else {
		while (dentry = readdir(dirf)) 
		{
          	    if (
			dentry->d_name[0]=='.' && 
			(
			 dentry->d_name[1]=='\0' ||
			 dentry->d_name[1]=='.' && dentry->d_name[2]=='\0'
			)
          	       )  /* check for directory items '.', '..', 
                                   *  and items without valid inode-number;
                                   */
          			continue;
          		if(!FileStatusIsOk(dir, dentry->d_name))
          			continue;
                           if(NetworkInfo->_nnets >= MAX_NETS)
                           {
 				fprintf(stderr, 
	"Cannot handle more than %d networks at the same time\n", MAX_NETS);
                                break;
                           }
                           if ((request =
                                (*xstream[X_TLI_STREAM].SetupTheListener)
                                        (display, dentry->d_name)) < 0)
                           {
                                fprintf(stderr,
				"Note: %s connections are not available.\n",
						dentry->d_name);
                           }
                           else
                           {
    				FirstClient = request + 1;
                                WellKnownConnections |= (1<<request);
                                n = strlen(dentry->d_name) +1;
                                NetworkInfo->_net[NetworkInfo->_nnets] =
                                        (char *) xalloc(n);
                                bcopy(dentry->d_name, 
				    NetworkInfo->_net[NetworkInfo->_nnets],n);
                                NetworkInfo->_nnets++;
                           }
                }
          	(void) closedir(dirf);
	}
   }
#else

    if ((request = (*xstream[X_NAMED_STREAM].SetupTheListener)
					(display, "local")) < 0)
    {
       	if(request == -1)	{
	 	fprintf(stderr,"setup named failed\n");
        	attexit(1);
	}
    }
    else
    {
        WellKnownConnections |= (1<<request);
        TypeOfStream[request] = X_NAMED_STREAM;
    }

/* Enhanced Application Compatibility Support */

    if(stat("/dev/spx", &sbuf) !=0)	{
	if(errno == ENOENT)	
	   fprintf(stderr,"/dev/spx not present, no sp listener is setup\n");
	else perror("");
    }
    else	{
      if ((request = (*xstream[X_SP_STREAM].SetupTheListener)
					(display, "local")) < 0)
      {
       	if(request == -1)	
	 	fprintf(stderr,"\n**** setup streams pipe failed,\n\n\
        You won't be able to run SCO X clients\n\n");
      }
      else
      {
        WellKnownConnections |= (1<<request);
        TypeOfStream[request] = X_SP_STREAM;
      }
    
/* End Enhanced Application Compatibility Support */

      if ((request = (*xstream[X_SP2_STREAM].SetupTheListener)
                                          (display, "local")) < 0)
      {
       	  if(request == -1)
                  fprintf(stderr,"***** setup streams pipe failed,\n\n\
	 You won't be able to run ISC X clients\n\n");
      }
      else
      {
	  WellKnownConnections |= (1<<request);
	  TypeOfStream[request] = X_SP2_STREAM;
      }
    }	/* end of /dev/spx */

   do
   {
	if(NetworkInfo->_nnets >= MAX_NETS)
        {
        	fprintf(stderr,
	"Cannot handle more than %d networks at the same time\n", MAX_NETS);
        	request = -1;
        }
	else request =(*xstream[X_TLI_STREAM].SetupTheListener)(display, NULL);

        if(request >= 0)
        {
        	FirstClient = request + 1;
		WellKnownConnections |= (1<<request);
	}
    } while (request > 0);
#endif

    if (openvt() < 0) 	/* Open the console device */
       	  attexit(1);

    if (WellKnownConnections == 0)	{
        FatalError ("Cannot establish any listening sockets");
	GiveUp();
    }

    signal (SIGPIPE, SIG_IGN);

    /*
     * At the moment the PTY subsystem sends a SIGHUP signal
     * every time a client closes its connection to the server.
     * Until this matter is settled, we cannot use SIGHUP to AutoResetServer.
     * (Luckily, AutoResetServer() looks like it's only used when X is run
     * from init.)
     */

/*
    signal (SIGHUP, SIG_IGN);
*/
    signal (SIGHUP, (void (*)())AutoResetServer);
    signal (SIGINT, GiveUp);
    signal (SIGTERM, GiveUp);
    signal (SIGQUIT, GiveUp);
    signal (SIGPWR, GiveUp);

    AllSockets[0] = WellKnownConnections;
    ResetHosts(display);
    /*
     * Magic:  If SIGUSR1 was set to SIG_IGN when
     * the server started, assume that either
     *
     *  a- The parent process is ignoring SIGUSR1
     *
     * or
     *
     *  b- The parent process is expecting a SIGUSR1
     *     when the server is ready to accept connections
     *
     * In the first case, the signal will be harmless,
     * in the second case, the signal will be quite
     * useful
     */
    if (signal (SIGUSR1, SIG_IGN) == SIG_IGN)
	SendSignal = TRUE;
    ParentProcess = getppid ();
    if (SendSignal) {
	if (ParentProcess > 0) {
	    kill (ParentProcess, SIGUSR1);
	}
    }

#ifdef XDMCP
    XdmcpInit ();
#endif


}

void
ResetWellKnownSockets ()
{
    ResetOsBuffers();
    ResetAuthorization ();
    ResetHosts(display);
    /*
     * See above in CreateWellKnownSockets about SIGUSR1
     */
    if (SendSignal) {
	if (ParentProcess > 0) {
	    kill (ParentProcess, SIGUSR1);
	}
    }

    /*
     * restart XDMCP
     */
#ifdef XDMCP
    XdmcpReset ();
#endif


}

/******e**********************************************************
 * ClientAuthorized
 *
 *    Sent by the client at connection setup:
 *                typedef struct _xConnClientPrefix {
 *                   CARD8	byteOrder;
 *                   BYTE	pad;
 *                   CARD16	majorVersion, minorVersion;
 *                   CARD16	nbytesAuthProto;    
 *                   CARD16	nbytesAuthString;   
 *                 } xConnClientPrefix;
 *
 *     	It is hoped that eventually one protocol will be agreed upon.  In the
 *        mean time, a server that implements a different protocol than the
 *        client expects, or a server that only implements the host-based
 *        mechanism, will simply ignore this information.
 *
 *****************************************************************/

char * 
ClientAuthorized(client, proto_n, auth_proto, string_n, auth_string)
    ClientPtr client;
    char *auth_proto, *auth_string;
    unsigned short proto_n, string_n;
{
    register OsCommPtr priv;
    XID	 auth_id;
    int  clientfd;

    priv = (OsCommPtr)client->osPrivate;
    clientfd = priv->fd;

/* TypeOfStream == local */

    if(TypeOfStream[clientfd] < X_TLI_STREAM )
		return((char *)NULL);

    auth_id = CheckAuthorization (proto_n, auth_proto,
				  string_n, auth_string);

    if (auth_id == (XID) ~0L && AuthorizedClient(clientfd))
    {
	auth_id = (XID) 0;
    }

    if (auth_id == (XID) ~0L)
	return "Client is not authorized to connect to Server";

    priv->auth_id = auth_id;
    priv->conn_time = 0;

    /* At this point, if the client is authorized to change the access control
     * list, we should getpeername() information, and add the client to
     * the selfhosts list.  It's not really the host machine, but the
     * true purpose of the selfhosts list is to see who may change the
     * access control list.
     */
    return((char *)NULL);
}

/*****************
 * EstablishNewConnections
 *    If anyone is waiting on listened sockets, accept them.
 *    Returns a mask with indices of new clients.  Updates AllClients
 *    and AllSockets.
 *****************/

int
EstablishNewConnections()
{
    long readyconnections;     /* mask of listeners that are ready */
    int curconn;                  /* fd of listener that's ready */
    register int newconn;         /* fd of new client */
    long connect_time;
    register int i;
    register ClientPtr client;
    register OsCommPtr oc;
    int nnew = 0;

    readyconnections = (LastSelectMask[0] & WellKnownConnections);
    if (!readyconnections)
	return;
    connect_time = GetTimeInMillis();

#ifdef USE_TIMEOUT

    /* kill off stragglers */
    for (i=1; i<currentMaxClients; i++)
    {
	if (client = clients[i])
	{
	    oc = (OsCommPtr)(client->osPrivate);
	    if (oc && (oc->conn_time != 0) &&
		(connect_time - oc->conn_time) >= TimeOutValue)
		CloseDownClient(client);     
	}
    }
#endif

    while (readyconnections) 
    {
	char MoreConnections;
	curconn = ffs (readyconnections) - 1;

        do
	{
	    MoreConnections = 0;
	    newconn = (*xstream[TypeOfStream[curconn]].ConnectNewClient)
						(curconn, &MoreConnections);
	    if (newconn >= 0)
	    {
		if (newconn >= lastfdesc)
		{
		    if (debug_conns)
ErrorF("Didn't make connection: Out of file descriptors for connections\n");
	(*xstream[TypeOfStream[newconn]].CloseStream)(newconn);
		} 
		else 
	        {
		    fcntl (newconn, F_SETFL, FNDELAY);


		    oc = (OsCommPtr)xalloc(sizeof(OsCommRec));
    		    if (!oc)
		    {
		        xfree(oc);
		        ErrorConnMax(newconn);
		        close(newconn);
		        continue;
		    }
		    if (GrabInProgress)
		    {
		    BITSET(SavedAllClients, newconn);
		    BITSET(SavedAllSockets, newconn);
		    }
		    else
		    {
		    BITSET(AllClients, newconn);
		    BITSET(AllSockets, newconn);
		    }
		    oc->fd = newconn;
	    	    oc->input = (ConnectionInputPtr)NULL;
		    oc->output = (ConnectionOutputPtr)NULL;
		    oc->conn_time = connect_time;
		    if ((newconn < lastfdesc) &&
		        (client = NextAvailableClient((pointer)oc)))
		    {
		        ConnectionTranslation[newconn] = client->index;
/* moved here from dispatch.c	*/
			nClients++;
			nnew++;
		    }
		    else
		    {
		        ErrorConnMax(newconn);
		        CloseDownFileDescriptor(oc);
		    }
		}
	    }
	 }
	 while (MoreConnections);
	 readyconnections &= ~(1 << curconn);
#ifdef XDMCP
        /* indicate to Xdmcp protocol that we've opened new client */
        XdmcpOpenDisplay(newconn);
#endif /* XDMCP */


    }
    return(nnew);
}

#define NOROOM "Maximum number of clients reached"

/************
 *   ErrorConnMax
 *     Fail a connection due to lack of client or file descriptor space
 ************/

static void
ErrorConnMax(fd)
    register int fd;
{
    xConnSetupPrefix csp;
    char pad[3];
    struct iovec iov[3];
    char byteOrder = 0;
    int whichbyte = 1;
    struct timeval waittime;
    long mask[mskcnt];

    /* if these seems like a lot of trouble to go to, it probably is */
    waittime.tv_sec = BOTIMEOUT / MILLI_PER_SECOND;
    waittime.tv_usec = (BOTIMEOUT % MILLI_PER_SECOND) *
		       (1000000 / MILLI_PER_SECOND);
    CLEARBITS(mask);
    BITSET(mask, fd);
    (void)SELECT(fd + 1, (int *) mask, (int *) NULL, (int *) NULL, &waittime);
    /* try to read the byte-order of the connection */
    (void)read(fd, &byteOrder, 1);
    if ((byteOrder == 'l') || (byteOrder == 'B'))
    {
	csp.success = xFalse;
	csp.lengthReason = sizeof(NOROOM) - 1;
	csp.length = (sizeof(NOROOM) + 2) >> 2;
	csp.majorVersion = X_PROTOCOL;
	csp.minorVersion = X_PROTOCOL_REVISION;
	if (((*(char *) &whichbyte) && (byteOrder == 'B')) ||
	    (!(*(char *) &whichbyte) && (byteOrder == 'l')))
	{
	    swaps(&csp.majorVersion, whichbyte);
	    swaps(&csp.minorVersion, whichbyte);
	    swaps(&csp.length, whichbyte);
	}
	iov[0].iov_len = sz_xConnSetupPrefix;
	iov[0].iov_base = (char *) &csp;
	iov[1].iov_len = csp.lengthReason;
	iov[1].iov_base = NOROOM;
	iov[2].iov_len = (4 - (csp.lengthReason & 3)) & 3;
	iov[2].iov_base = pad;
	_writev(fd, iov, 3);
    }
}

/************
 *   CloseDownFileDescriptor:
 *     Remove this file descriptor and it's I/O buffers, etc.
 ************/

static void
CloseDownFileDescriptor(oc)
    register OsCommPtr oc;
{
    int connection = oc->fd;

    FreeOsBuffers(oc);
    FreeClientBuffers(connection);

    /* Close the connection if it hasn't been closed already */
    if (TypeOfStream[connection] != CLOSED_STREAM){
	(*xstream[TypeOfStream[connection]].CloseStream)(connection);
	}
    TypeOfStream[connection] = CLOSED_STREAM;


    BITCLEAR(AllSockets, connection);
    BITCLEAR(AllClients, connection);
    BITCLEAR(ClientsWithInput, connection);
    BITCLEAR(YieldedClientsWithInput, connection);
    if (GrabInProgress)
    {
	BITCLEAR(SavedAllSockets, connection);
	BITCLEAR(SavedAllClients, connection);
	BITCLEAR(SavedClientsWithInput, connection);
	BITCLEAR(SavedYieldedClientsWithInput, connection);
    }

    BITCLEAR(ClientsBlocked, connection);
    BITCLEAR(SavedClientsWithInput,connection);
    BITCLEAR(SavedYieldedClientsWithInput,connection);


    xfree(oc);
}

/*****************
 * CheckConections
 *    Some connection has died, go find which one and shut it down 
 *    The file descriptor has been closed, but is still in AllClients.
 *    If would truly be wonderful if select() would put the bogus
 *    file descriptors in the exception mask, but nooooo.  So we have
 *    to check each and every socket individually.
 *****************/

#ifdef SVR4
void
CheckConnections()
{
    long		mask;
    long		tmask[mskcnt]; 
    register int	curclient, curoff;
    int			i;
    struct timeval	notime;
    int r;

    notime.tv_sec = 0;
    notime.tv_usec = 0;

    for (i=0; i<mskcnt; i++)
    {
	mask = AllClients[i];
        while (mask)
    	{
	    curoff = ffs (mask) - 1;
 	    curclient = curoff + (i << 5);
            CLEARBITS(tmask);
            BITSET(tmask, curclient);
            r = SELECT (curclient + 1, (int *)tmask, (int *)NULL, (int *)NULL, 
			&notime);
            if (r < 0)
	    {
		CloseDownClient(clients[ConnectionTranslation[curclient]]);
	        FreeClientBuffers(curclient);
	    }
	    mask &= ~(1 << curoff);
	}
    }	
}

#else
void
CheckConnections()
{
    long		rmask[mskcnt]; 
    long		emask[mskcnt]; 
    register int	curclient;
    int			i;
    int           bad;

    COPYBITS(AllClients, rmask);
    COPYBITS(AllClients, emask);
    i = SELECT (MAXSOCKS, rmask, (int *) NULL, emask, NULL);
    if (i <= 0)
	return;

    for (i=0; i<mskcnt; i++)
    {
        while (emask[i])
    	{
	    curclient = ffs (emask[i]) - 1 + (i << 5);
	    FreeClientBuffers(curclient);
	    if (bad = ConnectionTranslation[curclient])
	    {
    		    CloseDownClient(clients[bad]);
	    }
	    BITCLEAR(emask, curclient);
	}
    }	
}
#endif


/*****************
 * CloseDownConnection
 *    Delete client from AllClients and free resources 
 *****************/

CloseDownConnection(client)
    ClientPtr client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;

    ConnectionTranslation[oc->fd] = 0;

#ifdef XDMCP
    XdmcpCloseDisplay(oc->fd);
#endif

    CloseDownFileDescriptor(oc);
    client->osPrivate = (pointer)NULL;
}

#ifndef i386	/* funNotUsedByATT, AddEnabledDevice, RemoveEnabledDevice */

AddEnabledDevice(fd)
    int fd;
{
    BITSET(EnabledDevices, fd);
    BITSET(AllSockets, fd);
}


RemoveEnabledDevice(fd)
    int fd;
{
    BITCLEAR(EnabledDevices, fd);
    BITCLEAR(AllSockets, fd);
}

#endif	/* i386, funNotUsedByATT */

/*****************
 * OnlyListenToOneClient:
 *    Only accept requests from  one client.  Continue to handle new
 *    connections, but don't take any protocol requests from the new
 *    ones.  Note that if GrabInProgress is set, EstablishNewConnections
 *    needs to put new clients into SavedAllSockets and SavedAllClients.
 *    Note also that there is no timeout for this in the protocol.
 *    This routine is "undone" by ListenToAllClients()
 *****************/

OnlyListenToOneClient(client)
    ClientPtr client;
{

#ifdef XTESTEXT1
	extern int		XTestSimulationFlag;
	extern ClientPtr	XTestSimulationClient;
#endif

    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    int connection = oc->fd;

    if (! GrabInProgress)
    {
	COPYBITS (ClientsWithInput, SavedClientsWithInput);
	COPYBITS (YieldedClientsWithInput, SavedYieldedClientsWithInput);
        BITCLEAR (SavedClientsWithInput, connection);
	BITCLEAR (SavedYieldedClientsWithInput, connection);
	if (GETBIT(ClientsWithInput, connection))
	{
	    CLEARBITS(ClientsWithInput);	    
	    BITSET(ClientsWithInput, connection);
	}
	else
        {
	    CLEARBITS(ClientsWithInput);	    
	}

	if (GETBIT(YieldedClientsWithInput, connection))
	{
	    CLEARBITS(YieldedClientsWithInput);	    
	    BITSET(YieldedClientsWithInput, connection);
	}
	else
        {
	    CLEARBITS(YieldedClientsWithInput);	    
	}
	COPYBITS(AllSockets, SavedAllSockets);
	COPYBITS(AllClients, SavedAllClients);

	UNSETBITS(AllSockets, AllClients);
	BITSET(AllSockets, connection);
	CLEARBITS(AllClients);
	BITSET(AllClients, connection);


#ifdef XTESTEXT1
	if ( XTestSimulationFlag )
	{
		BITSET(AllSockets,
		       ((OsCommPtr)XTestSimulationClient->osPrivate)->fd);
		BITSET(AllClients,
		       ((OsCommPtr)XTestSimulationClient->osPrivate)->fd);

	}
#endif


	GrabInProgress = client->index;
    }
}

/****************
 * ListenToAllClients:
 *    Undoes OnlyListentToOneClient()
 ****************/

ListenToAllClients()
{
    if (GrabInProgress)
    {
	ORBITS(AllSockets, AllSockets, SavedAllSockets);
	ORBITS(AllClients, AllClients, SavedAllClients);
	ORBITS(ClientsWithInput, ClientsWithInput, SavedClientsWithInput);
	ORBITS(YieldedClientsWithInput, YieldedClientsWithInput, SavedYieldedClientsWithInput);
	GrabInProgress = 0;
    }	
}

#ifndef i386	/* funNotUsedByATT, IgnoreClient, AttendClient */

/****************
 * IgnoreClient
 *    Removes one client from input masks.
 *    Must have cooresponding call to AttendClient.
 ****************/

static long IgnoredClientsWithInput[mskcnt];
static long IgnoredYieldedClientsWithInput[mskcnt];
static long IgnoredSavedClientsWithInput[mskcnt];
static long IgnoredSavedYieldedClientsWithInput[mskcnt];

IgnoreClient (client)
    ClientPtr	client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    int connection = oc->fd;

    if (GETBIT (ClientsWithInput, connection))
	BITSET(IgnoredClientsWithInput, connection);
    else
	BITCLEAR(IgnoredClientsWithInput, connection);
    if (GETBIT (YieldedClientsWithInput, connection))
	BITSET(IgnoredYieldedClientsWithInput, connection);
    else
        BITCLEAR(IgnoredYieldedClientsWithInput, connection);
    BITCLEAR(ClientsWithInput, connection);
    BITCLEAR(YieldedClientsWithInput, connection);
    BITCLEAR(AllSockets, connection);
    BITCLEAR(AllClients, connection);
    if (GrabInProgress)
    {
    	if (GETBIT (SavedClientsWithInput, connection))
	    BITSET(IgnoredSavedClientsWithInput, connection);
    	else
	    BITCLEAR(IgnoredSavedClientsWithInput, connection);
	if (GETBIT (SavedYieldedClientsWithInput, connection))
            BITSET(IgnoredSavedYieldedClientsWithInput, connection);
	else
            BITCLEAR(IgnoredSavedYieldedClientsWithInput, connection);
	BITCLEAR(SavedClientsWithInput, connection);
	BITCLEAR(SavedYieldedClientsWithInput, connection);
	BITCLEAR(SavedAllSockets, connection);
	BITCLEAR(SavedAllClients, connection);
    }
}

/****************
 * AttendClient
 *    Adds one client back into the input masks.
 ****************/

AttendClient (client)
    ClientPtr	client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    int connection = oc->fd;

    if (!GrabInProgress || GrabInProgress == client->index)
    {
    	BITSET(AllClients, connection);
    	BITSET(AllSockets, connection);
    	if (GETBIT (IgnoredClientsWithInput, connection))
	    BITSET(ClientsWithInput, connection);
        if (GETBIT (IgnoredYieldedClientsWithInput, connection))
            BITSET(YieldedClientsWithInput, connection);
    }
    else
    {
	BITSET(SavedAllClients, connection);
	BITSET(SavedAllSockets, connection);
	if (GETBIT(IgnoredSavedClientsWithInput, connection))
	    BITSET(SavedClientsWithInput, connection);
	if (GETBIT(IgnoredSavedYieldedClientsWithInput, connection))
            BITSET(SavedYieldedClientsWithInput, connection);
    }
}

#endif	/* i386, funNotUsedByATT */

FileStatusIsOk(dir, file)
char *dir, *file;
{
	struct stat statb;
	char	*path;
	int	n;

	n = strlen(dir) + strlen(file)+2;

	path = (char *) Xalloc(n);

	sprintf(path, "%s/%s", dir, file);

	
	if (stat(path, &statb)<0) {
		perror(file);
		n = 0;
	}
        else {
	       	switch(statb.st_mode&S_IFMT) 
		{
	       		case S_IFDIR:
	       		case S_IFREG:
				n = 1;
				break;
			default     :
				n = 0;
				break;
		}
	}
	
	Xfree(path);
	return(n);
}
