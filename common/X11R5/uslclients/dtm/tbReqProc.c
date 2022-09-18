/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:tbReqProc.c	1.6"
#endif

#include <X11/Intrinsic.h>
#include "X11/StringDefs.h"
#include "X11/Xatom.h"
#include <Xol/OpenLook.h>
#include <Dt/DesktopI.h>
#include "Dtm.h"
#include "extern.h"

static char toolbox_str[] = "TOOLBOX";
static char shortcut_str[] = "SHORTCUT";

static int open_toolbox_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply);
static int create_tbobj_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply);
static int delete_tbobj_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply);
static int query_tbobj_proc (Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply);

/*
 * This array must match with information in DtTBMsg.h.
 */
static int (*(tb_procs[]))() = {
	open_toolbox_proc,
	create_tbobj_proc,
	delete_tbobj_proc,
	query_tbobj_proc
};

void
DmInitTBReqProcs(w)
Widget w;
{
	static DmPropEventData tb_prop_ev_data = {
			(Atom)0,
			Dt__tb_msgtypes,
			tb_procs,
			XtNumber(tb_procs)
	};

	/*
	 * Since _TOOLBOX_QUEUE is not constant,
	 * it cannot be initialized at compile time.
	 */
	tb_prop_ev_data.prop_name = _TOOLBOX_QUEUE;

	DmRegisterReqProcs(w, &tb_prop_ev_data);
}

static int
open_toolbox_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
	DmOpenToolboxWindow(request->open_toolbox.path, 0, NULL, False);

	/* free data in request structure */
	free(request->open_toolbox.path);

	return(0);
}

static int
create_tbobj_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->create_tb_obj
#define REP	reply->create_tb_obj
	char	*toolbox;
	char	*label;

	if (label = strchr(REQ.path, '/')) {
		toolbox = REQ.path;
		*label++ = '\0';
	}
	else {
		toolbox = TOOLBOXROOT;
		label = REQ.path;
	}

	if (!strcmp(REQ.objtype, "TOOLBOX")) {
		DmContainerPtr cp;

		/* assume the worst first */
		REP.status = DT_FAILED;

		/* get parent container */
		if (cp = DmGetToolboxInfo(toolbox)) {
			/* creating a toolbox */
			if (DmCreateToolbox(cp, label, 0, 0)) {
				REP.status = DT_OK;
			}
		}
	}
	else {
		/* creating a shortcut */
		char *realpath;

		/* get realpath from property list */
		if (realpath = DtGetProperty(&(REQ.plist), REAL_PATH, NULL)) {
			int ret;

			switch(DmInstallShortcut(label, realpath, toolbox)) {
			case 4:
				REP.status = DT_DUP;
				break;
			case 0:
				/* success */
				REP.status = DT_OK;

				/* install the other instance properties */
				/* not counting REAL_PATH */
				if (REQ.plist.count > 1) {
					DmObjectPtr op;
					DmContainerPtr cp =
						DmGetToolboxInfo(toolbox);

					op = DmGetObjectInContainer(cp, label);
					if (op) {
					    DtFreePropertyList(&(op->plist));
					    DtCopyPropertyList(&(op->plist),
								&(REQ.plist));
					}
					else
						/* internal error? */
						REP.status = DT_FAILED;
				}
				break;
			default:
				REP.status = DT_FAILED;
			}
		}
		else
			REP.status = DT_FAILED;
	}

	/* free data in request structure */
	XtFree(REQ.path);
	XtFree(REQ.objtype);
	DtFreePropertyList(&(REQ.plist));

	return(1);
#undef REP
#undef REQ
}

static int
delete_tbobj_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->delete_tb_obj
#define REP	reply->delete_tb_obj
	char	*toolbox;
	char	*shortcut;
	DmContainerPtr cp;
	DmObjectPtr op;

	/* assume the worst */
	REP.status = DT_FAILED;

	toolbox = REQ.path;
	if (shortcut = strchr(REQ.path, '/')) {
		*shortcut++ = '\0';

		if ((cp = DmGetToolboxInfo(toolbox))) {
			if (op = DmGetObjectInContainer(cp, shortcut)) {
				if (DmDelShortcut(cp, op) == 0)
					REP.status = DT_OK;
			}
		}
	}
	else {
		/* Deleting a toolbox */
		if ((cp = DmGetToolboxInfo(toolbox))) {
			DmDelToolbox(toolbox);
			REP.status = DT_OK;
		}
	}

	/* free data in request structure */
	free(REQ.path);

	return(1);
#undef REP
#undef REQ
}

static int
query_tbobj_proc(scrn, xevent, request, reply)
Screen *scrn;
XEvent *xevent;
DtRequest *request;
DtReply *reply;
{
#define REQ	request->query_tb_obj
#define REP	reply->query_tb_obj
	char *objname;
	DmContainerPtr cp;


	if (objname = strchr(REQ.path, '/'))
		*objname++ = '\0';

	/* initialize reply structure */
	REP.objtype = NULL;
	REP.plist.count = 0;
	REP.plist.ptr = NULL;

	if ((cp = DmGetToolboxInfo(REQ.path)) == NULL) {
err:
		free(REQ.path);
		REP.status = DT_FAILED;
		return(1);
	}

	if (objname) {
		DmObjectPtr op;

		if (op = DmGetObjectInContainer(cp, objname)) {
			if (op->ftype == DM_FTYPE_TOOLBOX)
				REP.objtype = toolbox_str;
			else
				REP.objtype = shortcut_str;
			if (REQ.options & DT_GET_PROPERTIES)
				REP.plist = op->plist;
		}
		else
			goto err;
	}
	else {
		/* toolbox does not have a property list */
		REP.objtype = toolbox_str;
		REP.plist.count = 0;
		REP.plist.ptr = NULL;
	}

	REP.status = DT_OK;

	/* free data in request structure */
	free(REQ.path);

	return(1);
#undef REP
#undef REQ
}

