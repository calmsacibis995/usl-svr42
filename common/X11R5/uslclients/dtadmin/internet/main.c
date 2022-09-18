/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/main.c	1.23"
#endif

#include <sys/utsname.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <Shell.h>
#include "../dtamlib/dtamlib.h"
#include "inet.h"
#include "error.h"
#include <Xol/Error.h>

extern void	WindowManagerEventHandler();
extern char *	GetUser();
extern char *	getenv();

char *		ApplicationName;
char *		Program;

static Widget	TopLevel;

Arg arg[50];

void
CreateSystemFile(filename)
char *filename;
{
	struct utsname	sname;
	char		text[128];
	char		login[UNAMESIZE];

	hf->toplevel = XtAppCreateShell(
		Program,
		ApplicationName,
		topLevelShellWidgetClass,
		XtDisplay(TopLevel),
		arg, 0
	);

	XtVaSetValues (
		hf->toplevel,
		XtNtitle,                       (XtArgVal) ApplicationName,
		XtNmappedWhenManaged,           (XtArgVal) False,
		XtNwmProtocolInterested,        (XtArgVal) OL_WM_DELETE_WINDOW,
		0
	);

	OlAddCallback (
		hf->toplevel,
		XtNwmProtocol, WindowManagerEventHandler,
		(XtPointer) 0
	);

	new			= (FlatList *) NULL;
	local			= (FlatList *) NULL;
	hf->popupMenuItems	= (Items *)NULL;
	hf->propPrompt		= (PopupGizmo *)NULL;
	hf->appendPrompt	= (ModalGizmo *)NULL;
	hf->expandPrompt	= (ModalGizmo *)NULL;
	hf->flatItems		= (FlatList *)NULL;
        hf->scrollingList	= (Widget)NULL;
        hf->findPopup		= (Widget)NULL;
        hf->propPopup		= (Widget)NULL;
        hf->localPopup		= (Widget)NULL;
        hf->appendPopup		= (Widget)NULL;
	hf->expandPopup		= (Widget)NULL;
        hf->quitNotice		= (Widget)NULL;
	hf->cancelNotice	= (Widget)NULL;
        hf->findTextField	= (Widget)NULL;
	hf->footer		= (Widget)NULL;

	hf->numFlatItems	= 0;
	hf->currentItem		= -1;
	hf->numAllocated	= 0;
	hf->uucpItems		= (UucpEntry *)NULL;
	hf->numUucp		= 0;
	hf->Lines		= (char **)NULL;
	hf->numLines		= 0;
	hf->numLinesAllocated	= 0;
	hf->update 		= _DtamIsOwner(OWN_NETADM); /* able to update? */
	hf->address		= False;
	hf->changesMade		= False;/* Indicates data was changed and */
					/* the file must be updated. */
	if (GetUser(login) == (char *)NULL)
		FooterMsg(hf->footer, "%s", GGT(string_noLogin));
	hf->userName = strdup (login);
	hf->userHome = getenv("HOME");
	if (!hf->userHome)
		hf->userHome = "";
	uname(&sname);
	hf->nodeName = strdup (sname.nodename);

	hf->filename = NULL;
	if (filename != NULL) {
		hf->filename = strdup(filename);
	}

	initialize ();

	if (!local) {
		if (hf->update) {
		/* Create a new entry for local entry */
			CreateBlankEntry ();
			LocalPopupCB((Widget)0, 0, 0);
		} else
			rexit (1, GGT(string_noLocal), "");
	} else
		XtPopup(hf->toplevel, XtGrabNone);
} /* CreateSystemFile */

static void
Usage()
{
} /* Usage */

#define	TYPE			"RAISE_MYSELF"
#define	APP_ID			"INTERNETMGR"
#define XA_RAISE_MYSELF(d)	XInternAtom(d, TYPE, False)
#define XA_APP_ID(d)		XInternAtom(d, APP_ID, False)

/* Owner... */
static Boolean
CvtSelectionProc(
Widget		w,
Atom *		selection,
Atom *		target,
Atom *		type_rtn,
XtPointer *	val_rtn,
unsigned long *	length_rtn,
int *		format_rtn)
{
#ifdef debug
	(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
	(void)fprintf(stderr, "Enter CvtSelectionProc \n");
	(void)fprintf(stderr, "w = 0x%x, selection = 0x%x, target = 0x%x\n",
				w, *selection, *target);
#endif
	if (*target == XA_RAISE_MYSELF(XtDisplay(w)) &&
	    *selection == XA_APP_ID(XtDisplay(w)))
	{
		/* map and raise the systems base window 		*/
		/* do we need to raise the device base window? how?	*/
		XMapRaised(XtDisplay(TopLevel), XtWindow(hf->toplevel));
		*format_rtn = 8;
		*length_rtn = 0;
		*val_rtn = 0;
		*type_rtn = *target;
		return(True); /* False */
	}
} /* end of CvtSelectionProc */

/* Requestor... */
static void
RaiseSelectionCB(
Widget		w,
XtPointer	client_data,
Atom *		selection,
Atom *		type,
XtPointer	value,
unsigned long * length,
int *		format)
{
#ifdef debug
	(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
	(void)fprintf(stderr,"Enter RaiseSelectionCB \n");
	(void)fprintf(stderr,"w = 0x%x, selection = 0x%x, type = 0x%x\n",
				w, *selection, *type);
#endif
	/* conversion fail message..., then do recovery...   */
	/* in the future, we may need to run the application */
	if (*type == XT_CONVERT_FAIL) {
		exit(0);
	} else {
		if (*type == XA_RAISE_MYSELF(XtDisplay(w)) &&
		    *selection == XA_APP_ID(XtDisplay(w))) {
			exit(0);
		}
		exit(0);
	}
} /* end of RaiseSelectionCB */

void
main(argc, argv)
int argc;
char *argv[];
{
	Window		another_window;
	XtAppContext	app_con;
	char *p, *q;

#ifdef MEMUTIL
	InitializeMemutil();
#endif

	/* Get rid of embedded slashes */
	/* Since XtName uses basename  */
	q = p = argv[0];
	while ((p = strstr(p, "/")) != NULL)
		q = ++p;
	Program = q;

	OlToolkitInitialize(&argc, argv, NULL);

	TopLevel = XtAppInitialize(
			&app_con,		/* app_context_return	*/
			Program,		/* application_class	*/
			(XrmOptionDescList)NULL,/* options		*/
			(Cardinal)0,		/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String *)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
	);
		
	ApplicationName = GGT (string_appName);
	if (argc > 2) Usage();

	XtVaSetValues (
		TopLevel,
		XtNtitle,			(XtArgVal) ApplicationName,
		XtNmappedWhenManaged,           (XtArgVal) False,
		XtNwidth,			(XtArgVal) 1,
		XtNheight,			(XtArgVal) 1,
		0
	);

	XtRealizeWidget(TopLevel);
	another_window = DtSetAppId (
				XtDisplay(TopLevel),
				XtWindow (TopLevel),
				APP_ID);
	if (another_window != None) {

#ifdef debug
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		(void)fprintf(stderr,"Initiate a selection resuest \n");
		(void)fprintf(stderr,"w = 0x%x, selection = 0x%x, type = 0x%x\n",
				TopLevel,
				XA_APP_ID(XtDisplay(TopLevel)),
				XA_RAISE_MYSELF(XtDisplay(TopLevel)));
#endif
		XtGetSelectionValue(
			TopLevel,
			XA_APP_ID(XtDisplay(TopLevel)),
			XA_RAISE_MYSELF(XtDisplay(TopLevel)) ,
			RaiseSelectionCB, (XtPointer)NULL, 0
		);

		/* bug? if one wants to do recover, how can we break? */
		XtAppMainLoop(app_con);
	} else {
#ifdef debug
		(void)fprintf(stderr,"claim a selection owner \n");
		(void)fprintf(stderr,"w = 0x%x, selection = 0x%x\n",
				TopLevel,
				XA_APP_ID(XtDisplay(TopLevel)));
#endif
	XtOwnSelection(TopLevel, XA_APP_ID(XtDisplay(TopLevel)), CurrentTime,
			CvtSelectionProc, NULL, NULL);
	}

	if (argc > 2) Usage();

	DtInitialize (TopLevel);

	ApplicationName = GGT(string_appName);
	hf = (SystemFile *)XtMalloc (sizeof(SystemFile));
	if (argc == 2)
		CreateSystemFile (argv[1]);
	else
		CreateSystemFile (system_path);

	XtAppMainLoop(app_con);
} /* main */
