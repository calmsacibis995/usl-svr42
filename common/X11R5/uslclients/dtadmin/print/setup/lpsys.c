/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/lpsys.c	1.10"
#endif

/* LP Subsystem Interface */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <Intrinsic.h>
#include <OpenLook.h>

#include <lp.h>
#include <printers.h>
#include <msgs.h>
#include <access.h>
#include <systems.h>

#include "lpsys.h"
#include "properties.h"
#include "error.h"

#define	DEFAULT_TIMEOUT		-1
#define	DEFAULT_RETRY		10
#define	S5_PROTO		1
#define	BSD_PROTO		2

static char *AllList [] = {
    "all!all",
    (char *) 0,
};

static char *LocalList [] = {
    "all",
    (char *) 0,
};

static void AllowSignals (Boolean);
static void InternalError (char *);

extern Widget IconBox;

/* LpAdmin
 *
 * Do the work of lpadmin; that is, update the printer files and inform the
 * lp scheduler, if it is running, of the changes.  Optionally allow a list of
 * users to use the printer.  If allowBtn is Yes_Button or No_Button, change
 * the user list to allow all user or just local users, repectively.  If
 * allowBtn is OL_NO_ITEM, leave the user list alone.  Return True if
 * changes were made, False on error.
 */
Boolean
LpAdmin (PRINTER *config, Cardinal allowBtn)
{
    char		msg [MSGMAX];
    int			updated;

    AllowSignals (False);

    if (putprinter (config->name, config) != 0)
	updated = False;
    else
    {
	/* Allow users access to the printer. */
	switch (allowBtn) {
	case Yes_Button:
	    allow_user_printer (AllList, config->name);
	    break;

	case No_Button:
	    allow_user_printer (LocalList, config->name);
	    break;

	case OL_NO_ITEM:
	default:
	    /* do nothing */
	    break;
	}

	/* Open a connection to the spooler.  If the open fails, assume the
	 * spooler is not running.
	 */
	if (mopen () == 0)
	{
	    /* Spooler running.  Tell it about the changes. */
	    (void) putmessage (msg, S_LOAD_PRINTER, config->name);

	    /* Send the message and get the response.  Neither of these
	     * should fail, but if disaster strikes, just ignore the error.
	     * Also assume that the operation works correctly.
	     */
	    if (msend (msg) == 0)
		mrecv (msg, MSGMAX);

	    mclose ();
	}
	updated = True;
    }

    AllowSignals (True);
    return (updated);
} /* End of LpAdmin () */

/* LpAcceptEnable
 *
 * Open a connection to the spooler and optionally enable/disable a printer
 * or accept/reject from its queue.  Return flags to indicate success.
 */
unsigned
LpAcceptEnable (char *name, LpState accept, LpState enable, LpWhen when)
{
    char	msg [MSGMAX];
    short	rcvMsg;
    short	status;
    char	*id;
    unsigned	success;

    success = 0;

    if (accept == Lp_No_Change && enable == Lp_No_Change)
	return (Lp_Accept_Flag & Lp_Enable_Flag);

    if (mopen () == 0)
    {
	switch (accept) {
	case Lp_On:
	    (void) putmessage (msg, S_ACCEPT_DEST, name);
	    rcvMsg = R_ACCEPT_DEST;
	    break;
	case Lp_Off:
	    (void) putmessage (msg, S_REJECT_DEST, name, "");
	    rcvMsg = R_REJECT_DEST;
	    break;
	case Lp_No_Change:
	    success |= Lp_Accept_Flag;
	    rcvMsg = 0;
	    break;
	}

	if (rcvMsg && msend (msg) == 0 && mrecv (msg, MSGMAX) != -1)
	{
	    (void) getmessage (msg, rcvMsg, &status);
	    if (status == MOK)
		success |= Lp_Accept_Flag;
	}

	switch (enable) {
	case Lp_On:
	    (void) putmessage (msg, S_ENABLE_DEST, name);
	    rcvMsg = R_ENABLE_DEST;
	    break;
	case Lp_Off:
	    (void) putmessage (msg, S_DISABLE_DEST, name, "", when);
	    rcvMsg = R_DISABLE_DEST;
	    break;
	case Lp_No_Change:
	    success |= Lp_Enable_Flag;
	    rcvMsg = 0;
	    break;
	}

	if (rcvMsg && msend (msg) == 0 && mrecv (msg, MSGMAX) != -1)
	{
	    if (rcvMsg == R_ENABLE_DEST)
		(void) getmessage (msg, rcvMsg, &status);
	    else
		(void) getmessage (msg, rcvMsg, &status, &id);
	    if (status == MOK)
		success |= Lp_Enable_Flag;
	}

	mclose ();
    }

    return (success);
} /* End of LpAcceptEnable () */

/* LpPrinterStatus
 *
 * Get the current status of a printer.  Return False if unable to get status.
 */
Boolean
LpPrinterStatus (char *name, char **pId, Boolean *pAccepting,
		 Boolean *pEnabled, Boolean *pFaulted)
{
    char	msg [MSGMAX];
    short	status;
    char	*printer;
    char	*form;
    char	*printWheel;
    char	*disableReason;
    char	*rejectReason;
    short	printerState;
    long	disableDate;
    long	rejectDate;
    Boolean	success;

    success = False;
    if (mopen () == 0)
    {
	(void) putmessage (msg, S_INQUIRE_PRINTER_STATUS, name);
	if (msend (msg) == 0 && mrecv (msg, MSGMAX) != -1)
	{
	    (void) getmessage (msg, R_INQUIRE_PRINTER_STATUS, &status,
			       &printer, &form, &printWheel, &disableReason,
			       &rejectReason, &printerState,
			       pId, &disableDate,
			       &rejectDate);
	    if (status == MOK)
	    {
		if (printerState & PS_BUSY)
		    *pId = strdup (*pId);
		else
		    *pId = (char *) 0;
		*pAccepting = printerState & PS_REJECTED ? False : True;
		*pEnabled =
		    printerState & (PS_DISABLED | PS_LATER) ? False : True;
		*pFaulted = printerState & PS_FAULTED ? True : False;
		success = True;
	    }
	    else
	    {
		*pId = (char *) 0;
		*pAccepting = *pEnabled = *pFaulted = False;
	    }
	}
	mclose ();
    }

    return (success);
}	/* End of LpPrinterStatus () */

/* LpDelete
 *
 * Delete a printer from the system.  The operation will fail if there are
 * any jobs queued for the printer.  Return MOK if the printer was deleted,
 * MBUSY if it had jobs, and MTRANSMITERR if we couldn't talk to the
 * scheduler.
 */
int
LpDelete (char *name)
{
    char	msg [MSGMAX];
    short	status;
    Boolean	success;

    /* Open a connection to the scheduler and have it unload the printer. */
    if (mopen () == 0)
    {
	(void) putmessage (msg, S_UNLOAD_PRINTER, name);
	if (msend (msg) == 0 && mrecv (msg, MSGMAX) != -1)
	    (void) getmessage (msg, R_UNLOAD_PRINTER, &status);
	else
	    status = MTRANSMITERR;
	mclose ();
    }
    else
	return (MTRANSMITERR);

    /* Delete the printer from the lp system. */
    if (status == MOK || status == MNODEST)
    {
	/* Even if the spooler does not know about the printer, try and
	 * delete the printer configuration files.  The only reasons the
	 * delprinter functions should fail is because we didn't have
	 * permissions, in which case, we never should have gotten to this
	 * function anyway, or if the printer truely doesn't exist, which
	 * is what we wanted in the first place.
	 */
	AllowSignals (False);
	(void) delprinter (name);
	AllowSignals (True);
	status = MOK;
    }

    return (status);
}	/* End of LpDelete () */

/* LpCancelAll
 *
 * Cancel all jobs for a printer.  Return True if successful, False otherwise.
 */
Boolean
LpCancelAll (char *name)
{
    char	msg [MSGMAX];
    Boolean	success;
    char	**jobs;
    int		allocated;
    int		cnt;
    short	status;
    char	*id;
    char	*user;
    long	size;
    long	date;
    short	outcome;
    char	*printer;
    char	*form;
    char	*character_set;
    long	level;

    /* Open connection to scheduler.  We can't cope if the open fails. */
    if (mopen () != 0)
	return (False);

    /* Get the active jobs for the printer */
    success = True;
    cnt = allocated = 0;
    jobs = (char **) 0;
    (void) putmessage (msg, S_INQUIRE_REQUEST, "", name, "", "", "");
    if (msend (msg) == 0)
    {
	do
	{
	    if (mrecv (msg, MSGMAX) == -1)
	    {
		cnt = 0;
		success = False;
		break;
	    }
	    (void) getmessage (msg, R_INQUIRE_REQUEST, &status, &id, &user,
			       &size, &date, &outcome, &printer, &form,
			       &character_set, &level);
	    if (status == MNOINFO)
		break;

	    if (cnt >= allocated)
	    {
		allocated += 10;
		jobs = (char **) XtRealloc ((char *) jobs,
					    allocated * sizeof (*jobs));
	    }
	    jobs [cnt++] = strdup (id);
	} while (status == MOKMORE);
    }
    else
    {
	mclose ();
	return (False);
    }

    /* cancel each job. */
    while (--cnt >= 0)
    {
	(void) putmessage (msg, S_CANCEL_REQUEST, jobs [cnt]);
	XtFree (jobs [cnt]);
	if (msend (msg) != 0 || mrecv (msg, MSGMAX) == -1 ||
	    getmessage (msg, R_CANCEL_REQUEST, &status) == -1)
	{
	    success = False;
	}

	if (status == MNOPERM)
	    InternalError (GetStr (TXT_noPerm));
    }

    mclose ();
    XtFree ((char *) jobs);
    return (success);
}	/* End of LpCancelAll () */

/* LpSystem
 *
 * Setup a remote system.  This function is designed to be called repeatedly--
 * that is, the connection to the spooler is held open until a null value is
 * passed in.  Returns True if operation successful.
 */
Boolean
LpSystem (char *name, int os)
{
    SYSTEM		*sysbuf;
    Boolean		rc;
    char		msg [MSGMAX];
    static		connectionOpen = False;
    static SYSTEM	dfltbuf = {
	NULL, NULL, NULL, S5_PROTO, NULL,
	DEFAULT_TIMEOUT, DEFAULT_RETRY,
	NULL, NULL, NULL,
    }; 

    if (!name)
    {
	if (connectionOpen)
	{
	    connectionOpen = False;
	    mclose ();
	}
	return (True);
    }

    AllowSignals (False);
    rc = True;
    if (os != No_OS)
    {
	sysbuf = getsystem (name);
	if (!sysbuf)
	{
	    sysbuf = &dfltbuf;
	    sysbuf->name = name;
	    sysbuf->protocol = (os == S5_OS) ? S5_PROTO : BSD_PROTO;
	}

	if (putsystem (name, sysbuf))
	    rc = False;
    }
    else
	if (delsystem (name))
	    rc = False;

    AllowSignals (True);
    if (!rc)
	return (False);

    /* Tell the spooler about the changes */
    if (!connectionOpen)
    {
	/* If for some reason we can not tell the spooler about the change,
	 * we'll just ignore the error--after all, the files are correct,
	 * so it's not a complete catastrophe.  If there is a message error,
	 * then close the connection.
	 */
	if (mopen () != 0)
	    return (True);

	connectionOpen = True;
	
	if (os != No_OS)
	    (void) putmessage (msg, S_LOAD_SYSTEM, name);
	else
	    (void) putmessage (msg, S_UNLOAD_SYSTEM, name);
	if (msend (msg) != 0 || mrecv (msg, MSGMAX) == -1)
	{
	    mclose ();
	    connectionOpen = False;
	}
    }

    return (True);
}	/* End of LpSystem () */


/* AllowSignals
 *
 * Allow or disallow signals SIGHUP, SIGINT, SIGQUIT, SIGTERM.  When
 * disallowed, the signals are held until a subsequent call allows them.
 */
static void
AllowSignals (Boolean allow)
{
    if (allow)
    {
	(void) sigrelse (SIGHUP);
	(void) sigrelse (SIGINT);
	(void) sigrelse (SIGQUIT);
	(void) sigrelse (SIGTERM);
    }
    else
    {
	(void) sighold (SIGHUP);
	(void) sighold (SIGINT);
	(void) sighold (SIGQUIT);
	(void) sighold (SIGTERM);
    }
} /* End of AllowSignals () */

/* InternalError
 *
 * Print an error message and die.
 */
static void
InternalError (char *msg)
{
    fprintf (stderr, GetStr (TXT_intrnlErr), msg);
    if (errno)
	fprintf (stderr, GetStr (TXT_errnoEq), errno);
    exit (1);
} /* End of InternalError () */
