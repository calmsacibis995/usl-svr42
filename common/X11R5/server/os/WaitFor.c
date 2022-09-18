/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:os/WaitFor.c	1.6"
/*copyright   "%c%"*/

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/*****************************************************************
 * OS Depedent input routines:
 *
 *  WaitForSomething,  GetEvent
 *
 *****************************************************************/

#include "Xos.h"			/* for strings, fcntl, time */

#include <errno.h>
#include <stdio.h>
#include "X.h"
#include "misc.h"

#include <sys/param.h>
#include <signal.h>
#include "osdep.h"
#include "dixstruct.h"
#include "opaque.h"

/* #include <sys/types.h> rjk */
#ifdef SVR4
#include <sys/time.h>
#endif /* SVR4 */
#include <sys/xque.h>
#include <Xstreams.h>

extern Xstream xstream[];
extern char TypeOfStream[];

char                    ServerAsleep = 0;
extern char             PendingInput;
extern xqEventQueue     *queue; 

#ifdef REPLAY
extern char     replayflag;

#define STORING_EVENTS          1
#define REPLAYING_EVENTS        2
#endif

extern long AllSockets[];
extern long AllClients[];
extern long LastSelectMask[];
extern long WellKnownConnections;
extern long EnabledDevices[];
extern long ClientsWithInput[];
extern long YieldedClientsWithInput[];
extern long ClientsBlocked[];
extern WorkQueuePtr workQueue;
extern void ResetOsBuffers();
extern void ProcessInputEvents();
extern void BlockHandler();
extern void WakeupHandler();

extern long ScreenSaverTime;               /* milliseconds */
extern long ScreenSaverInterval;               /* milliseconds */
extern int ConnectionTranslation[];

extern void CheckConnections();
extern int EstablishNewConnections();
extern void SaveScreens();

extern int errno;


#ifdef apollo
extern long apInputMask[];

static long LastWriteMask[mskcnt];
#endif

#ifdef XTESTEXT1
/*
 * defined in xtestext1dd.c
 */
extern int playback_on;
extern void XTestComputeWaitTime();
#endif /* XTESTEXT1 */

/*****************
 * WaitForSomething:
 *     Make the server suspend until there is
 *	1. data from clients or
 *	2. input events available or
 *	3. ddx notices something of interest (graphics
 *	   queue ready, etc.) or
 *	4. clients that have buffered replies/events are ready
 *
 *     If the time between INPUT events is
 *     greater than ScreenSaverTime, the display is turned off (or
 *     saved, depending on the hardware).  So, WaitForSomething()
 *     has to handle this also (that's why the select() has a timeout.
 *     For more info on ClientsWithInput, see ReadRequestFromClient().
 *     pClientsReady is an array to store ready client->index values into.
 *****************/

static long timeTilFrob = 0;		/* while screen saving */

#if (mskcnt>4)
/*
 * This is a macro if mskcnt <= 4
 */
ANYSET(src)
    long	*src;
{
    int i;

    for (i=0; i<mskcnt; i++)
	if (src[ i ])
	    return (TRUE);
    return (FALSE);
}
#endif

int
WaitForSomething(pClientsReady)
    int *pClientsReady;
{
    int i;
    struct timeval waittime, *wt;
    long timeout;
    long clientsReadable[mskcnt];
    long clientsWritable[mskcnt];
    long curclient;
    int selecterr;
    int nready;
    long devicesReadable[mskcnt];
    extern Bool sdbMouse;

    CLEARBITS(clientsReadable);
    CLEARBITS(clientsWritable);
    if (! (ANYSET(ClientsWithInput)))
    {
     	if (! (ANYSET(YieldedClientsWithInput)))
	{
	    /* We need a while loop here to handle 
	       crashed connections and the screen saver timeout */
	    while (1)
	    {
	/* deal with any blocked jobs */
        if (workQueue)
            ProcessWorkQueue();

#ifdef REPLAY
		if(replayflag == REPLAYING_EVENTS)
		{
		    waittime.tv_sec = 0;
		    waittime.tv_usec = 0;
		    wt = &waittime;
		}
		else
#endif
		if (ScreenSaverTime)
		{
		    timeout = ScreenSaverTime - TimeSinceLastInputEvent();
		    if (timeout <= 0) /* may be forced by AutoResetServer() */
		    {
			long timeSinceSave;

			timeSinceSave = -timeout;
			if ((timeSinceSave >= timeTilFrob) && (timeTilFrob >= 0))
			{
			    SaveScreens(SCREEN_SAVER_ON, ScreenSaverActive);
			    if (ScreenSaverInterval)
				/* round up to the next ScreenSaverInterval */
				timeTilFrob = ScreenSaverInterval *
					((timeSinceSave + ScreenSaverInterval) /
						ScreenSaverInterval);
			    else
				timeTilFrob = -1;
			}
			timeout = timeTilFrob - timeSinceSave;
		    }
		    else
		    {
			if (timeout > ScreenSaverTime)
			    timeout = ScreenSaverTime;
			timeTilFrob = 0;
		    }

		    COPYBITS(AllSockets, LastSelectMask);
		    BlockHandler((pointer)&wt, (pointer)LastSelectMask);
		    ServerAsleep = 1;
		    /*
		     * If mouse/keyboard input is pending, we don't want to sleep
		     * in select(), so instead wait for 0 seconds.  Note that this
		     * test/assignment must come after setting ServerAsleep to TRUE
		     * so that we won't ever sleep if there is pending input (avoid
		     * critical sections).  If we were to do this test before
		     * setting ServerAsleep, and then get the driver interrupt
		     * immediately thereafter, then we would not longjmp, but
		     * instead sleep until select returns whilst the pending input
		     * remains unprocessed.
		     */
		    if (PendingInput)
			waittime.tv_sec = 0;
		    else
			waittime.tv_sec = timeout / MILLI_PER_SECOND;
		    waittime.tv_usec = 0;
		    wt = &waittime;
		}
		else {
		    COPYBITS(AllSockets, LastSelectMask);
		    BlockHandler((pointer)&wt, (pointer)LastSelectMask);
		    ServerAsleep = 1;
		    /*
		     * Same circumstances and conditions as above apply here.
		     */
		    if (PendingInput) {
			waittime.tv_sec = 0;
			waittime.tv_usec = 0;
			wt = &waittime;
		    }
		    else
		    {
			wt = NULL;
		    }
		}

#ifdef SDB_MOUSE
		if (sdbMouse) {
			waittime.tv_sec = 0;
			waittime.tv_usec = 0;
		}
#endif
#ifdef XTESTEXT1
		/* XXX how does this interact with new write block handling? */
		if (playback_on) {
		    wt = &waittime;
		    XTestComputeWaitTime (&waittime);
		}
#endif /* XTESTEXT1 */
		errno = 0;

		/* keep this check close to select() call to minimize race */
		if (dispatchException)
		    i = -1;
		else if (ANYSET(ClientsBlocked))
		{
		    COPYBITS(ClientsBlocked, clientsWritable);
		    i = SELECT (MAXSOCKS, (int *)LastSelectMask,
				(int *)clientsWritable, (int *) NULL, wt);
		}
		else
		    i = SELECT (MAXSOCKS, (int *)LastSelectMask,
				(int *) NULL, (int *) NULL, wt);
		queue->xq_sigenable = 0;
		ServerAsleep = 0;
		selecterr = errno;

		if (ANYSET (clientsWritable))
		{
		    int	i;
		    for (i=0; i<mskcnt; i++)
		    {
			while (clientsWritable[i])
			{
			    curclient = ffs(clientsWritable[i]) 
					    - 1 + 32 * i;
			    if (TryToWriteAgain(curclient) < 0)
				    ioFatalError(curclient);
			    clientsWritable[i] &= ~(1 << curclient);
			}
		    }
		}

		WakeupHandler((unsigned long)i, (pointer)LastSelectMask);
#ifdef XTESTEXT1
		if (playback_on) {
		    i = XTestProcessInputAction (i, &waittime);
		}
#endif /* XTESTEXT1 */
		if (i <= 0) /* An error or timeout occurred */
		{
		    if (dispatchException) {
		    	queue->xq_sigenable = 0;
			return 0;
		    }
		    if (i < 0) 
			if (selecterr == EBADF)    /* Some client disconnected */
			{
			    CheckConnections ();
			}
			else if (selecterr != EINTR)
			    ErrorF("WaitForSomething(): select: errno=%d\n",
				selecterr);
		    /*
		     * Common break: if we lost a client, if we had an error or
		     * interrupt, or if we timed out.  We need to break to get
		     * back to Dispatch().  (If we lost a client, we want
		     * Dispatch() to check nClients for us.  If we're here due
		     * to an interrupt, it's probably due to a request for a
		     * switch of VTs, and Dispatch()'ll handle that.  If we timed
		     * out, it is likely because there is pending input, so we
		     * want Dispatch to process the input events.)
		     */
		    break;
		}
		else
		{
		    int	nnew = 0;

		    MASKANDSETBITS(devicesReadable, LastSelectMask, EnabledDevices);
			    /* r4 added above line*/
		    MASKANDSETBITS(clientsReadable, LastSelectMask, AllClients); 
		    if (LastSelectMask[0] & WellKnownConnections) 
			nnew = EstablishNewConnections();
		    if (nnew || (LastSelectMask[0] & EnabledDevices[0])
			|| (ANYSET(devicesReadable))
			|| (ANYSET (clientsReadable)) || ANYSET(ClientsBlocked))
				break;

		}
		/* Typically not reached */
		/*
		 * mouse interrupts are a pain while using sdb, so to get around
		 * the mouse interrupts, make queue->sigenable = 0 and set
		 * wait time = 0; The side effect is that server continuously
		 * loops, but this is only for debugging ......
		 *
		 * sdbMouse is set to TRUE while running the server in sdb.
		 * Also look in dix/dispatch.c, os/sysV/WaitFor.c,
		 * ddx/io/init.c and ddx/io/xwin_io.c
		 */
		if (sdbMouse)
		    queue->xq_sigenable = 0;
		else
		    queue->xq_sigenable = 1;

		if (PendingInput)
		    ProcessInputEvents(1);
	    }
	}
        else {
            /* Before going back to select look for clients */
            /* that have yielded their input after MAX_TIMES_PER */
            /* requests. */
            COPYBITS(YieldedClientsWithInput, ClientsWithInput);
            COPYBITS(YieldedClientsWithInput, clientsReadable);
            CLEARBITS(YieldedClientsWithInput);
	}
    }
    else
    {
       COPYBITS(ClientsWithInput, clientsReadable);
    }

    nready = 0;
    queue->xq_sigenable = 0;
    if (ANYSET(clientsReadable))
    {
	for (i=0; i<mskcnt; i++)
	{
	    while (clientsReadable[i])
	    {
		curclient = ffs (clientsReadable[i]) - 1;
		pClientsReady[nready++] = 
			ConnectionTranslation[curclient + (i << 5)];
		clientsReadable[i] &= ~(1 << curclient);
	    }
	}	
    }
    return nready;
}
