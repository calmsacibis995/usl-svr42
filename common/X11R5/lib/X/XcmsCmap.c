/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsCmap.c	1.2"

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Xutil.h"

/*
 *      FORWARD DECLARATIONS
 */
XcmsCmapRec *_XcmsAddCmapRec();
static void _XcmsFreeClientCmaps();


/************************************************************************
 *									*
 *			PRIVATE INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		CmapRecForColormap
 *
 *	SYNOPSIS
 */
static XcmsCmapRec *
CmapRecForColormap(dpy, cmap)
    Display *dpy;
    Colormap cmap;
/*
 *	DESCRIPTION
 *		Find the corresponding XcmsCmapRec for cmap.  In not found
 *		this routines attempts to create one.
 *
 *	RETURNS
 *		Returns NULL if failed; otherwise the address to
 *		the corresponding XcmsCmapRec.
 *
 */
{
    XcmsCmapRec *pRec;
    int nScrn;
    int i, j;
    XVisualInfo visualTemplate;	/* Template of the visual we want */
    XVisualInfo *visualList;	/* List for visuals that match */
    int nVisualsMatched;	/* Number of visuals that match */
    Window tmpWindow;
    Visual *vp;
    _XAsyncHandler async;
    _XAsyncErrorState async_state;

    for (pRec = (XcmsCmapRec *)dpy->cms.clientCmaps; pRec != NULL;
	    pRec = pRec->pNext) {
	if (pRec->cmapID == cmap) {
	    return(pRec);
	}
    }

    /*
     * Can't find an XcmsCmapRec associated with cmap in our records.
     * Let's try to see if its a default colormap
     */
    nScrn = ScreenCount(dpy);
    for (i = 0; i < nScrn; i++) {
	if (cmap == DefaultColormap(dpy, i)) {
	    /* It is ... lets go ahead and store that info */
	    if ((pRec = _XcmsAddCmapRec(dpy, cmap, RootWindow(dpy, i),
		    DefaultVisual(dpy, i))) == NULL) {
		return((XcmsCmapRec *)NULL);
	    }
	    pRec->ccc = XcmsCreateCCC(
		    dpy,
		    i,			/* screenNumber */
		    DefaultVisual(dpy, i),
		    (XcmsColor *)NULL,	/* clientWhitePt */
		    (XcmsCompressionProc)NULL,  /* gamutCompProc */
		    (XPointer)NULL,	/* gamutCompClientData */
		    (XcmsWhiteAdjustProc)NULL,  /* whitePtAdjProc */
		    (XPointer)NULL	/* whitePtAdjClientData */
		    );
	    return(pRec);
	}
    }

    /*
     * Nope, its not a default colormap, so it's probably a foreign color map
     * of which we have no specific details.  Let's go through the
     * rigorous process of finding this colormap:
     *        for each screen
     *            for each screen's visual types
     *                create a window with cmap specified as the colormap
     *                if successful
     *                    Add a CmapRec
     *                    Create an XcmsCCC
     *                    return the CmapRec
     *                else
     *                    continue
     */

    async_state.error_code = 0; /* don't care */
    async_state.major_opcode = X_CreateWindow;
    async_state.minor_opcode = 0;
    for (i = 0; i < nScrn; i++) {
	visualTemplate.screen = i;
	visualList = XGetVisualInfo(dpy, VisualScreenMask, &visualTemplate,
	    &nVisualsMatched);
	if (nVisualsMatched == 0) {
	    continue;
	}

	/*
	 * Attempt to create a window with cmap
	 */
	j = 0;
	do {
	    vp = (visualList+j)->visual;
	    LockDisplay(dpy);
	    {
		register xCreateWindowReq *req;

		GetReq(CreateWindow, req);
		async_state.min_sequence_number = dpy->request;
		async_state.max_sequence_number = dpy->request;
		async_state.error_count = 0;
		async.next = dpy->async_handlers;
		async.handler = _XAsyncErrorHandler;
		async.data = (XPointer)&async_state;
		dpy->async_handlers = &async;
		req->parent = RootWindow(dpy, i);
		req->x = 0;
		req->y = 0;
		req->width = 1;
		req->height = 1;
		req->borderWidth = 0;
		req->depth = (visualList+j)->depth;
		req->class = CopyFromParent;
		req->visual = vp->visualid;
		tmpWindow = req->wid = XAllocID(dpy);
		req->mask = CWColormap;
		req->length++;
		Data32 (dpy, (long *) &cmap, 4);
	    }
	    {
		xGetInputFocusReply rep;
		register xReq *req;

		GetEmptyReq(GetInputFocus, req);
		(void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
	    }
	    DeqAsyncHandler(dpy, &async);
	    UnlockDisplay(dpy);
	    SyncHandle();
	} while (async_state.error_count > 0 && ++j < nVisualsMatched);

	Xfree((char *)visualList);

	/*
	 * if successful
	 */
	if (j < nVisualsMatched) {
	    if ((pRec = _XcmsAddCmapRec(dpy, cmap, tmpWindow, vp)) == NULL)
		return((XcmsCmapRec *)NULL);
	    pRec->ccc = XcmsCreateCCC(
		    dpy,
		    i,			/* screenNumber */
		    vp,
		    (XcmsColor *)NULL,	/* clientWhitePt */
		    (XcmsCompressionProc)NULL,  /* gamutCompProc */
		    (XPointer)NULL,	/* gamutCompClientData */
		    (XcmsWhiteAdjustProc)NULL,  /* whitePtAdjProc */
		    (XPointer)NULL	/* whitePtAdjClientData */
		    );
	    XDestroyWindow(dpy, tmpWindow);
	    return(pRec);
	}
    }

    return(NULL);
}



/************************************************************************
 *									*
 *			API PRIVATE INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsAddCmapRec
 *
 *	SYNOPSIS
 */
XcmsCmapRec *
_XcmsAddCmapRec(dpy, cmap, windowID, visual)
    Display *dpy;
    Colormap cmap;
    Window windowID;
    Visual *visual;
/*
 *	DESCRIPTION
 *		Create an XcmsCmapRec for the specified cmap, windowID,
 *		and visual, then adds it to its list of CmapRec's.
 *
 *	RETURNS
 *		Returns NULL if failed; otherwise the address to
 *		the added XcmsCmapRec.
 *
 */
{
    XcmsCmapRec *pNew;

    if ((pNew = (XcmsCmapRec *) Xcalloc(1, (unsigned) sizeof(XcmsCmapRec)))
	    == NULL) {
	return((XcmsCmapRec *)NULL);
    }

    pNew->cmapID = cmap;
    pNew->dpy = dpy;
    pNew->windowID = windowID;
    pNew->visual = visual;
    pNew->pNext = (XcmsCmapRec *)dpy->cms.clientCmaps;
    dpy->cms.clientCmaps = (XPointer)pNew;
    dpy->free_funcs->clientCmaps = _XcmsFreeClientCmaps;

    /*
     * Note, we don't create the XcmsCCC for pNew->ccc here because
     * it may require the use of XGetWindowAttributes (a round trip request)
     * to determine the screen.
     */
    return(pNew);
}


/*
 *	NAME
 *		_XcmsCopyCmapRecAndFree
 *
 *	SYNOPSIS
 */
XcmsCmapRec *
_XcmsCopyCmapRecAndFree(dpy, src_cmap, copy_cmap)
    Display *dpy;
    Colormap src_cmap;
    Colormap copy_cmap;
/*
 *	DESCRIPTION
 *		Augments Xlib's XCopyColormapAndFree() to copy
 *		XcmsCmapRecs.
 *
 *	RETURNS
 *		Returns NULL if failed; otherwise the address to
 *		the copy XcmsCmapRec.
 *
 */
{
    XcmsCmapRec *pRec_src;
    XcmsCmapRec *pRec_copy;

    if ((pRec_src = CmapRecForColormap(dpy, src_cmap)) != NULL) {
	pRec_copy =_XcmsAddCmapRec(dpy, copy_cmap, pRec_src->windowID,
		pRec_src->visual);
	if (pRec_copy != NULL && pRec_src->ccc) {
	    pRec_copy->ccc = (XcmsCCC)Xcalloc(1, (unsigned) sizeof(XcmsCCCRec));
	    bcopy((char *)pRec_src->ccc, (char *)pRec_copy->ccc,
		    sizeof(XcmsCCCRec));
	}
	return(pRec_copy);
    }
    return((XcmsCmapRec *)NULL);
}


/*
 *	NAME
 *		_XcmsDeleteCmapRec
 *
 *	SYNOPSIS
 */
void
_XcmsDeleteCmapRec(dpy, cmap)
    Display *dpy;
    Colormap cmap;
/*
 *	DESCRIPTION
 *		Removes and frees the specified XcmsCmapRec structure
 *		from the linked list of structures.
 *
 *	RETURNS
 *		void
 *
 */
{
    XcmsCmapRec **pPrevPtr;
    XcmsCmapRec *pRec;
    int scr;

    /* If it is the default cmap for a screen, do not delete it,
     * because the server will not actually free it */
    for (scr = ScreenCount(dpy); --scr >= 0; ) {
	if (cmap == DefaultColormap(dpy, scr))
	    return;
    }

    /* search for it in the list */
    pPrevPtr = (XcmsCmapRec **)&dpy->cms.clientCmaps;
    while ((pRec = *pPrevPtr) && (pRec->cmapID != cmap)) {
	pPrevPtr = &pRec->pNext;
    }

    if (pRec) {
	if (pRec->ccc) {
	    XcmsFreeCCC(pRec->ccc);
	}
	*pPrevPtr = pRec->pNext;
	Xfree((char *)pRec);
    }
}


/*
 *	NAME
 *		_XcmsFreeClientCmaps
 *
 *	SYNOPSIS
 */
static void
_XcmsFreeClientCmaps(dpy)
    Display *dpy;
/*
 *	DESCRIPTION
 *		Frees all XcmsCmapRec structures in the linked list
 *		and sets dpy->cms.clientCmaps to NULL.
 *
 *	RETURNS
 *		void
 *
 */
{
    XcmsCmapRec *pRecNext, *pRecFree;

    pRecNext = (XcmsCmapRec *)dpy->cms.clientCmaps;
    while (pRecNext != NULL) {
	pRecFree = pRecNext;
	pRecNext = pRecNext->pNext;
	if (pRecFree->ccc) {
	    /* Free the XcmsCCC structure */
	    XcmsFreeCCC(pRecFree->ccc);
	}
	/* Now free the XcmsCmapRec structure */
	Xfree((char *)pRecFree);
    }
    dpy->cms.clientCmaps = (XPointer)NULL;
}



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCCCOfColormap
 *
 *	SYNOPSIS
 */
XcmsCCC 
XcmsCCCOfColormap(dpy, cmap)
    Display *dpy;
    Colormap cmap;
/*
 *	DESCRIPTION
 *		Finds the XcmsCCC associated with the specified colormap.
 *
 *	RETURNS
 *		Returns NULL if failed; otherwise the address to
 *		the associated XcmsCCC structure.
 *
 */
{
    XWindowAttributes windowAttr;
    XcmsCmapRec *pRec;
    int nScrn = ScreenCount(dpy);
    int i;

    if ((pRec = CmapRecForColormap(dpy, cmap)) != NULL) {
	if (pRec->ccc) {
	    /* XcmsCmapRec already has a XcmsCCC */
	    return(pRec->ccc);
	}

	/*
	 * The XcmsCmapRec does not have a XcmsCCC yet, so let's create
	 * one.  But first, we need to know the screen associated with
	 * cmap, so use XGetWindowAttributes() to extract that
	 * information.  Unless, of course there is only one screen!!
	 */
	if (nScrn == 1) {
	    /* Assume screenNumber == 0 */
	    return(pRec->ccc = XcmsCreateCCC(
		    dpy,
		    0,			/* screenNumber */
		    pRec->visual,
		    (XcmsColor *)NULL,	/* clientWhitePt */
		    (XcmsCompressionProc)NULL,  /* gamutCompProc */
		    (XPointer)NULL,	/* gamutCompClientData */
		    (XcmsWhiteAdjustProc)NULL,  /* whitePtAdjProc */
		    (XPointer)NULL	/* whitePtAdjClientData */
		    ));
	} else {
	    if (XGetWindowAttributes(dpy, pRec->windowID, &windowAttr)) {
		for (i = 0; i < nScrn; i++) {
		    if (ScreenOfDisplay(dpy, i) == windowAttr.screen) {
			return(pRec->ccc = XcmsCreateCCC(
				dpy,
				i,		   /* screenNumber */
				pRec->visual,
				(XcmsColor *)NULL, /* clientWhitePt */
				(XcmsCompressionProc)NULL, /* gamutCompProc */
				(XPointer)NULL,	   /* gamutCompClientData */
				(XcmsWhiteAdjustProc)NULL, /* whitePtAdjProc */
				(XPointer)NULL	   /* whitePtAdjClientData */
				));
		    }
		}
	    }
	}
    }

    /*
     * No such cmap
     */
    return(NULL);
}
