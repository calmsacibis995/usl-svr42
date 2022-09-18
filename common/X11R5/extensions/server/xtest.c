/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xtest.c	1.1"

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "windowstr.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "xteststr.h"

static unsigned char XTestReqCode;
static int ProcXTestDispatch(), SProcXTestDispatch();
static void XTestResetProc();
static int XTestSwapFakeInput();
CursorPtr GetSpriteCursor();
WindowPtr GetCurrentRootWindow();

void
XTestExtensionInit()
{
    ExtensionEntry *extEntry, *AddExtension();

    if (extEntry = AddExtension(XTESTNAME, 0, 0,
				 ProcXTestDispatch, SProcXTestDispatch,
				 XTestResetProc, StandardMinorOpcode))
	XTestReqCode = (unsigned char)extEntry->base;
}

/*ARGSUSED*/
static void
XTestResetProc (extEntry)
ExtensionEntry	*extEntry;
{
}

static int
ProcXTestGetVersion(client)
    register ClientPtr client;
{
    REQUEST(xXTestGetVersionReq);
    xXTestGetVersionReply rep;
    register int n;

    REQUEST_SIZE_MATCH(xXTestGetVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = XTEST_MAJOR;
    rep.minorVersion = XTEST_MINOR;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
	swaps(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xXTestGetVersionReply), (char *)&rep);
    return(client->noClientException);
}

static int
ProcXTestCompareCursor(client)
    register ClientPtr client;
{
    REQUEST(xXTestCompareCursorReq);
    xXTestCompareCursorReply rep;
    WindowPtr pWin;
    CursorPtr pCursor;
    register int n;

    REQUEST_SIZE_MATCH(xXTestCompareCursorReq);
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);
    if (stuff->cursor == None)
	pCursor = NullCursor;
    else if (stuff->cursor == XTestCurrentCursor)
	pCursor = GetSpriteCursor();
    else {
	pCursor = (CursorPtr)LookupIDByType(stuff->cursor, RT_CURSOR);
	if (!pCursor) 
	{
	    client->errorValue = stuff->cursor;
	    return (BadCursor);
	}
    }
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.same = (wCursor(pWin) == pCursor);
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    }
    WriteToClient(client, sizeof(xXTestCompareCursorReply), (char *)&rep);
    return(client->noClientException);
}

static int
ProcXTestFakeInput(client)
    register ClientPtr client;
{
    REQUEST(xReq);
    int nev;
    int	n;
    xEvent *ev;
    DeviceIntPtr dev;
    WindowPtr root;

    nev = (stuff->length << 2) - sizeof(xReq);
    if (nev % sizeof(xEvent))
	return BadLength;
    nev /= sizeof(xEvent);
    if (nev != 1)
	return BadLength; /* for now */
    UpdateCurrentTime();
    ev = (xEvent *)&stuff[1];
    switch (ev->u.u.type & 0177)
    {
    case KeyPress:
    case KeyRelease:
    case MotionNotify:
    case ButtonPress:
    case ButtonRelease:
	break;
    default:
	client->errorValue = ev->u.u.type;
	return BadValue;
    }
    if (ev->u.keyButtonPointer.time)
    {
	TimeStamp activateTime;
	CARD32 ms;

	activateTime = currentTime;
	ms = activateTime.milliseconds + ev->u.keyButtonPointer.time;
	if (ms < activateTime.milliseconds)
	    activateTime.months++;
	activateTime.milliseconds = ms;
	ev->u.keyButtonPointer.time = 0;
	/* swap the request back so we can simply re-execute it */
	if (client->swapped)
	{
    	    (void) XTestSwapFakeInput(client, stuff);
	    swaps(&stuff->length, n);
	}
	ResetCurrentRequest (client);
	client->sequence--;
	if (!ClientSleepUntil(client, &activateTime, NULL, NULL))
	{
	    /* 
	     * flush this request - must be in this order because
	     * ResetCurrentRequest adds the client back to 
	     * clientsWithInput which will cause the client to
	     * keep going, instead of waiting for the timeout.
	     */
	    (void) ReadRequestFromClient (client);
	    client->sequence++;
	    return BadAlloc;
	}
	return Success;
    }
    switch (ev->u.u.type & 0177)
    {
    case KeyPress:
    case KeyRelease:
	dev = (DeviceIntPtr)LookupKeyboardDevice();
	if (ev->u.u.detail < dev->key->curKeySyms.minKeyCode ||
	    ev->u.u.detail > dev->key->curKeySyms.maxKeyCode)
	{
	    client->errorValue = ev->u.u.detail;
	    return BadValue;
	}
	break;
    case MotionNotify:
	dev = (DeviceIntPtr)LookupPointerDevice();
	root = LookupWindow(ev->u.keyButtonPointer.root, client);
	if (!root)
	    return BadWindow;
	if (root->parent)
	{
	    client->errorValue = ev->u.keyButtonPointer.root;
	    return BadValue;
	}
	if (ev->u.keyButtonPointer.rootX < 0 ||
	    ev->u.keyButtonPointer.rootX > root->drawable.width)
	{
	    client->errorValue = ev->u.keyButtonPointer.rootX;
	    return BadValue;
	}
	if (ev->u.keyButtonPointer.rootY < 0 ||
	    ev->u.keyButtonPointer.rootY > root->drawable.height)
	{
	    client->errorValue = ev->u.keyButtonPointer.rootY;
	    return BadValue;
	}
	if (root != GetCurrentRootWindow())
	{
	    NewCurrentScreen(root->drawable.pScreen,
			     ev->u.keyButtonPointer.rootX,
			     ev->u.keyButtonPointer.rootY);
	    return client->noClientException;
	}
	(*root->drawable.pScreen->SetCursorPosition)
	    (root->drawable.pScreen,
	     ev->u.keyButtonPointer.rootX,
	     ev->u.keyButtonPointer.rootY, FALSE);
	break;
    case ButtonPress:
    case ButtonRelease:
	dev = (DeviceIntPtr)LookupPointerDevice();
	for (n = 1; n <= dev->button->numButtons; n++)
	    if (dev->button->map[n] == ev->u.u.detail)
		break;
	if (n > dev->button->numButtons)
	{
	    client->errorValue = ev->u.u.detail;
	    return BadValue;
	}
	ev->u.u.detail = n;
	break;
    }
    ev->u.keyButtonPointer.time = currentTime.milliseconds;
    (*dev->public.processInputProc)(ev, (DevicePtr)dev, 1); 
    return client->noClientException;
}

static int
ProcXTestDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XTestGetVersion:
	return ProcXTestGetVersion(client);
    case X_XTestCompareCursor:
	return ProcXTestCompareCursor(client);
    case X_XTestFakeInput:
	return ProcXTestFakeInput(client);
    default:
	return BadRequest;
    }
}

static int
SProcXTestGetVersion(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xXTestGetVersionReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXTestGetVersionReq);
    swaps(&stuff->minorVersion, n);
    return ProcXTestGetVersion(client);
}

static int
SProcXTestCompareCursor(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xXTestCompareCursorReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXTestCompareCursorReq);
    swapl(&stuff->window, n);
    swapl(&stuff->cursor, n);
    return ProcXTestCompareCursor(client);
}

static int
XTestSwapFakeInput(client, req)
    register ClientPtr	client;
    xReq *req;
{
    register int nev;
    register xEvent *ev;
    xEvent sev;
    void (*proc)(), NotImplemented();

    nev = ((req->length << 2) - sizeof(xReq)) / sizeof(xEvent);
    for (ev = (xEvent *)&req[1]; --nev >= 0; ev++)
    {
    	/* Swap event */
    	proc = EventSwapVector[ev->u.u.type & 0177];
	/* no swapping proc; invalid event type? */
    	if (!proc || (int (*)()) proc == (int (*)()) NotImplemented) {
	    client->errorValue = ev->u.u.type;
	    return BadValue;
	}
    	(*proc)(ev, &sev);
	*ev = sev;
    }
    return Success;
}

static int
SProcXTestFakeInput(client)
    register ClientPtr	client;
{
    register int n;
    REQUEST(xReq);

    swaps(&stuff->length, n);
    n = XTestSwapFakeInput(client, stuff);
    if (n != Success)
	return n;
    return ProcXTestFakeInput(client);
}

static int
SProcXTestDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_XTestGetVersion:
	return SProcXTestGetVersion(client);
    case X_XTestCompareCursor:
	return SProcXTestCompareCursor(client);
    case X_XTestFakeInput:
	return SProcXTestFakeInput(client);
    default:
	return BadRequest;
    }
}
