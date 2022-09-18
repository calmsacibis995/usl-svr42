/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:dtmReqProc.c	1.22" */

/******************************file*header********************************

    Description:
	This file contains the source code for callback functions
	which are shared among components of the desktop manager.
*/
						/* #includes go here	*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Dt/DesktopI.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

#define MSG_PROTO	Screen *scrn, XEvent *xevent, \
			DtRequest *request,  DtReply *reply

static int	open_folder_proc ( MSG_PROTO );
static int	sync_folder_proc ( MSG_PROTO );
static int	create_class_proc ( MSG_PROTO );
static int	delete_class_proc ( MSG_PROTO );
static int	query_class_proc ( MSG_PROTO );
static int	get_property_proc ( MSG_PROTO );
static int	set_property_proc ( MSG_PROTO );
static int	display_prop_sheet_proc ( MSG_PROTO );
static int	display_binder_proc ( MSG_PROTO );
static int	open_fmap_proc ( MSG_PROTO );
static int	dt_shutdown_proc ( MSG_PROTO );

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
/*
 * This array must match with information in DtDTMMsg.h.
 */
static int (*(dtm_procs[]))() = {
	open_folder_proc,
	sync_folder_proc,
	create_class_proc,
	delete_class_proc,
	query_class_proc,
	get_property_proc,
	set_property_proc,
	display_prop_sheet_proc,
	display_binder_proc,
	open_fmap_proc,
	dt_shutdown_proc,
};

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    open_folder_proc-
*/
static int
open_folder_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
	int ret = 0;

#define REQ	request->open_folder
#define REP	reply->open_folder

	if (request->open_folder.path) {
		DmFolderWinPtr fwp;

		fwp = DmOpenFolderWindow(REQ.path, 0, NULL, False);

		if (REQ.options & DT_NOTIFY_ON_CLOSE) {
			if (fwp) {
				DmCloseNotifyPtr np;

				if (np = (DmCloseNotifyPtr)realloc(fwp->notify,
						  sizeof(DmCloseNotifyRec) *
						  (fwp->num_notify + 1))) {
					fwp->notify = np;
					np += fwp->num_notify;
					np->serial = REQ.serial;
					np->version= REQ.version;
					np->client = xevent->xselectionrequest.
						     requestor;
					np->replyq = xevent->xselectionrequest.
						     property;
					fwp->num_notify++;
				}
				else {
					/* realloc failed */
					REP.status = DT_FAILED;
					ret = 1;
				}
			}
			else {
				/* open failed */
				REP.status = DT_FAILED;
				ret = 1;
			}
		}
	}

	/* free data in request structure */
	XtFree(REQ.class_name);
	XtFree(REQ.pattern);
	XtFree(REQ.path);
	XtFree(REQ.title);

	return(ret);

#undef REP
#undef REQ
}

static int
sync_folder_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
	DmFolderWindow folder;

#define REQ	request->sync_folder

	if ( (folder = DmQueryFolderWindow(REQ.path)) != NULL )
	{
	    Dm__RmFromStaleList(folder);
	    Dm__SyncFolder(folder);
	}

	/* free data in request structure */
	XtFree(REQ.path);

	return(0);

#undef REQ
}

static int
create_class_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
	DmFnameKeyPtr new_fnkp;
	DmFnameKeyPtr last_new_fnkp; /* end of the new class list */
	DmFnameKeyPtr last_new_fcfp; /* last fcfp in the new class list */
	DmFnameKeyPtr fnkp;
	DmFnameKeyPtr last_fnkp;     /* end of the current class list */

#define REQ	request->create_fclass
#define REP	reply->create_fclass
#define FCFP(P)	((DmFclassFilePtr)(P))

	/* check if file is already in the database */
	if (DmCheckFileClassDB(REQ.file_name)) {
		REP.status = DT_DUP;
		goto bye;
	}

	/* read the database */
	new_fnkp = fnkp = DmReadFileClassDB(REQ.file_name);

	if (new_fnkp) {
		last_new_fcfp = new_fnkp;

		/*
		 * Turn on the REPLACED bit, so that new classes are recognized.
		 */
		while (fnkp) {
			fnkp->attrs |= DM_B_REPLACED;
			fnkp->level++; /* included in toplevel file */

			if (fnkp->attrs & DM_B_CLASSFILE)
				last_new_fcfp = fnkp;
			else
				DmInitFileClass(fnkp);

			if (fnkp->next)
				fnkp = fnkp->next;
			else {
				last_new_fnkp = fnkp;
				break;
			}
		}

		/* the first entry is the file header */
		new_fnkp->level--;

		/* insert new class list into the existing list */
		fnkp = DESKTOP_FNKP(Desktop); /* get the current list */
		if (REQ.options & DT_APPEND) {
			/* insert the file at the end */
			DmFnameKeyPtr last_fcfp = fnkp;

			/* assume DESKTOP_FNKP can never be NULL */
			for (; fnkp->next; fnkp=fnkp->next) {
				if (fnkp->attrs & DM_B_CLASSFILE)
					last_fcfp = fnkp;
			}

			fnkp->next = new_fnkp;
			new_fnkp->prev = fnkp;
			FCFP(last_fcfp)->next_file = FCFP(new_fnkp);
		}
		else {
			/* insert the file in front */
			last_new_fnkp->next = fnkp->next;
			if (fnkp->next)
				fnkp->next->prev = last_new_fnkp;
			fnkp->next = new_fnkp;
			new_fnkp->prev = fnkp;
			FCFP(last_new_fcfp)->next_file = FCFP(fnkp)->next_file;
			FCFP(fnkp)->next_file = FCFP(new_fnkp);
		}

		/* sync all windows */
		DmSyncWindows(DESKTOP_FNKP(Desktop), NULL);

		/* turn off the replaced bit */
		for (fnkp=new_fnkp; fnkp->next; fnkp=fnkp->next)
			fnkp->attrs &= ~DM_B_REPLACED;
		/* flush the change to disk */
		DESKTOP_FNKP(Desktop)->attrs |= DM_B_WRITE_FILE;
		DmWriteFileClassDBList(DESKTOP_FNKP(Desktop));
	}
	else
		REP.status = DT_FAILED;

bye:
	/* free data in request structure */
	XtFree(REQ.file_name);

	return(1);
#undef REP
#undef REQ
}

static int
delete_class_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
	DmFnameKeyPtr fcfp;
	DmFnameKeyPtr prev_fcfp;

#define REQ	request->delete_fclass
#define REP	reply->delete_fclass

	/* find the file in the database */
	prev_fcfp = NULL;
	fcfp = DESKTOP_FNKP(Desktop);
	while (fcfp) {
		if (!strcmp(fcfp->name, REQ.file_name))
			break;
		prev_fcfp = fcfp;
		fcfp = (DmFnameKeyPtr)(FCFP(fcfp)->next_file);
	}

	/* check if file is already in the database */
	if (fcfp) {
		DmFnameKeyPtr fnkp;
		DmFnameKeyPtr last_fcfp;
		unsigned short ilevel; /* initial file level */
		/* must not remove the toplevel file */
		if (fcfp == DESKTOP_FNKP(Desktop)) {
			REP.status = DT_NOENT;
			goto bye;
		}

		/* find the end of the list to be deleted */
		for (fnkp=fcfp->next, ilevel=fnkp->level, last_fcfp=fcfp;
		     fnkp; fnkp=fnkp->next){
			if (fnkp->level < ilevel) {
				fnkp = fnkp->prev; /* go back one */
				break;
			}

			if (fnkp->attrs & DM_B_CLASSFILE)
				last_fcfp = fnkp;
		}

		/* remove the deleted classes from the standard list */
		FCFP(prev_fcfp)->next_file = FCFP(last_fcfp)->next_file;
		fcfp->prev->next = fnkp->next;
		if (fnkp->next)
			fnkp->next->prev = fcfp->prev;

		/* sync all windows */
		DmSyncWindows(DESKTOP_FNKP(Desktop), fcfp);

		/* free classes */
		/*
		 * There are problems freeing the class info here, because
		 * fnkp or fcp may still be used by property sheets or
		 * wastebasket. Without maintaining a usage count, the deleted
		 * cannot be freed. Sigh.
		 */

		/* flush the change to disk */
		DESKTOP_FNKP(Desktop)->attrs |= DM_B_WRITE_FILE;
		DmWriteFileClassDBList(DESKTOP_FNKP(Desktop));
	}
	else
		REP.status = DT_NOENT;

bye:
	/* free data in request structure */
	XtFree(REQ.file_name);

	return(1);
#undef REP
#undef REQ
}

static int
query_class_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->query_fclass
#define REP	reply->query_fclass

	if (REQ.class_name) {
		register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

		REP.class_name = NULL;
		REP.plist.count = 0;
		REP.plist.ptr = NULL;

		for (; fnkp; fnkp = fnkp->next) {
			if (!(fnkp->attrs & DM_B_CLASSFILE) &&
			    !strcmp(fnkp->name, REQ.class_name)) {
				REP.status = DT_OK;
				if (REQ.options & DT_GET_PROPERTIES) {
					REP.class_name = fnkp->name;
					REP.plist = fnkp->fcp->plist;
				}
				break;
			}
		}

		if (!fnkp) {
			REP.status = DT_FAILED;
		}

		/* free data in request structure */
		free(REQ.class_name);
	}
	else
		REP.status = DT_BAD_INPUT;

	return(1);

#undef REP
#undef REQ
}

static int
get_property_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->get_property
#define REP	reply->get_property

	if (REQ.name) {
		REP.value = DmGetDTProperty(REQ.name, &(REP.attrs));
		REP.status = DT_OK;
	}
	else
		REP.status = DT_BAD_INPUT;
		
	/* free data in request structure */
	XtFree(REQ.name);

	return(1);

#undef REP
#undef REQ
}

static int
set_property_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->set_property

	if (REQ.name) {
		DtSetProperty(&DESKTOP_PROPS(Desktop), REQ.name, REQ.value,
			 	REQ.attrs);
		DmSaveDesktopProperties(Desktop);
	}

	/* free data in request structure */
	XtFree(REQ.name);
	XtFree(REQ.value);

	return(0);
#undef REQ
}

/****************************procedure*header*****************************
    display_prop_sheet_proc- popup Desktop property sheet.
	This must mimic a button press on the Properties button of olwsm.
	The request contains the name of the sheet (currently unused).
*/
static int
display_prop_sheet_proc( MSG_PROTO )
{
#define REQ	request->display_prop_sheet

    extern void PropertySheetByName();

    PropertySheetByName( (REQ.prop_name == NULL) ? "Desktop" : REQ.prop_name );

    XtFree(REQ.prop_name);	/* free data in request structure */
    return(0);

#undef REQ
}				/* end of display_prop_sheet_proc */

static int
display_binder_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->display_binder

	DmDisplayBinder();
	return(0);
#undef REQ
}

static int
open_fmap_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->open_fmap

	DmFolderOpenTreeCB(NULL, NULL, NULL);
	return(0);
#undef REQ
}

static int
dt_shutdown_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->dt_shutdown

	DmShutdownPrompt();
	return(0);
#undef REQ
}

/***************************public*procedures*****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmInitDTMReqProcs-
*/
void
DmInitDTMReqProcs(w)
Widget w;
{
	static DmPropEventData dtm_prop_ev_data = {
				(Atom)0,
				Dt__dtm_msgtypes,
				dtm_procs,
				XtNumber(dtm_procs)
	};

	/*
	 * Since _DT_QUEUE is not a constant,
	 * it cannot be initialized at compile time.
	 */
	dtm_prop_ev_data.prop_name = _DT_QUEUE(XtDisplay(w));

	DmRegisterReqProcs(w, &dtm_prop_ev_data);
}
