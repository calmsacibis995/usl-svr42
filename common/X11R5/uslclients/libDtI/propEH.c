/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:propEH.c	1.9"
#endif

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "DtI.h"

#define STEP_SIZE	4

#define EATCHAR(P, C)	while (*(P) == C) (P)++

static DmPropEventData **ed_list;
static int ed_list_size = 0;
static int ed_list_used = 0;

static void
DmProcessRequest(edp, idx, w, xevent, str)
DmPropEventData *edp;
int idx;
Widget w;
XEvent *xevent; /* if NULL, it is a simulated request */
char *str;
{
#define EVENT_IN	xevent->xselectionrequest
	DtMsgInfo const *mip = edp->msgtypes + idx;
	DtRequest req;
	DtReply reply;

	memset(&req, 0, sizeof(req));
	req.header.rqtype = mip->type;
	if (Dt__DecodeFromString((char *)&req, mip, str, NULL) == -1)
		return; /* BAD format */

	if (xevent == NULL) {
		/* simulated request */
		req.header.serial = 0;
		req.header.version = DT_VERSION;
	}

	reply.header.status = DT_OK;

	if ((edp->proc_list[idx])(XtScreen(w), xevent, &req, &reply)) {
		if (xevent) {
			/** send a reply **/
			/* fill in the header first */
			reply.header.rptype	= req.header.rqtype;
			reply.header.serial	= req.header.serial;
			reply.header.version	= req.header.version;

			DtSendReply(XtScreen(w), EVENT_IN.property,
			    	    req.header.client, &reply);
		}
	}

	/* free strings in header */
	XtFree(req.header.nodename);

#undef EVENT_IN
}

static
DmSelectionEventHandler(w, client_data, xevent, cont_to_dispatch)
Widget w;
XtPointer client_data;
XEvent *xevent;
Boolean *cont_to_dispatch;
{
#define EVENT_IN	xevent->xselectionrequest
	DmPropEventData *edp;
	DtMsgInfo const *mip;
	char *buffer;
	char *rqname;
	char *p;
	int i;

	if (xevent->type != SelectionRequest)
		return;

	/* check for a selection name match */
	for (i=0, edp=NULL; i < ed_list_used; i++)
		if (EVENT_IN.selection == ed_list[i]->prop_name) {
			edp = ed_list[i];
			break;
		}

	if (edp == NULL)
		return;

	if ((buffer = DtDequeueMsg(XtScreen(w),
		EVENT_IN.property, EVENT_IN.requestor)) == NULL)
		return; /* No msg */

	if (buffer[0] != '@')
		return; /* BAD format */

	rqname = ++buffer; /* skip '@' */
	if ((p = strchr(rqname, ':')) == NULL)
		return; /* BAD format */

	*p++ = '\0'; /* temporary switch */

	/* find request type */
	for (mip=edp->msgtypes, i=0; i < edp->count; i++, mip++)
		if (!strcmp(rqname, mip->name))
			break;

	if (i == edp->count)
		return; /* unknown msg name */
	DmProcessRequest(edp, i, w, xevent, p);

	*cont_to_dispatch = False;
#undef EVENT_IN
}

int
DmDispatchRequest(w, selection, str)
Widget w;
Atom selection;
char *str;
{
	DmPropEventData *edp;
	DtMsgInfo const *mip;
	char *rqname = str;
	char *p;
	int i;
	int j;
	int found = 0;

	EATCHAR(rqname, ' ');
	if (*rqname != '@')
		return(0);

	if ((p = strchr(++rqname, ':')) == NULL)
		return(0);

	*p++ = '\0';

	if (selection != (Atom)None) {
		for (i=0, edp=NULL; i < ed_list_used; i++)
			if (selection == ed_list[i]->prop_name) {
				edp = ed_list[i];
				break;
			}

		/* find request type */
		for (mip=edp->msgtypes, j=0; j < edp->count; j++, mip++)
			if (!strcmp(rqname, mip->name)) {
				found++;
				break;
			}
	}
	else {
		/* try all edps */
		for (i=0, edp=NULL; i < ed_list_used; i++)
			for (mip=edp->msgtypes, j=0; j < edp->count; j++, mip++)
				if (!strcmp(rqname, mip->name)) {
					found++;
					break;
				}
	}

	if (found) {
		DmProcessRequest(edp, j, w, NULL, p);
	}
	else
		return(0);
}

void DmRegisterReqProcs(w, edp)
Widget w;
DmPropEventData *edp;
{
	if (ed_list_size == ed_list_used) {
		if (ed_list_size == 0)
			/* first time */
			XtAddEventHandler(w, (EventMask)NoEventMask, True,
			 	  	  DmSelectionEventHandler, NULL);
		
		ed_list_size += STEP_SIZE;
		ed_list = (DmPropEventData **)realloc(ed_list,
						sizeof(DmPropEventData *) *
						ed_list_size);
		if (ed_list == NULL) {
			fprintf(stderr, "malloc failed\n");
			exit(1);
		}
	}

	ed_list[ed_list_used++] = edp;
}

