/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)notice:NoticeO.c	1.1"
#endif

/*******************************file*header*******************************
    Description: Notice.c - OPEN LOOK(TM) Notice Widget
*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ControlAre.h>

#include <Xol/NoticeP.h>
#include <Xol/StaticText.h>
#include <Xol/VendorI.h>
#include <RectObj.h>
#include <Xol/RubberTile.h>


#define LeaveParams		LeaveWindowMask, False, LeaveEH, NULL
#define AddLeaveEH(w)		XtAddEventHandler(w, LeaveParams)
#define RemoveLeaveEH(w)	XtRemoveEventHandler(w, LeaveParams)

#define MapParams		StructureNotifyMask, False, MapEH, NULL
#define AddMapEH(w)		XtAddEventHandler(w, MapParams)
#define RemoveMapEH(w)		XtRemoveEventHandler(w, MapParams)

/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */
						/* class procedures */
						/* action procedures */

static void LeaveEH OL_ARGS((NoticeShellWidget, XtPointer, XEvent *,
						 Boolean *));
static void MapEH OL_ARGS((NoticeShellWidget, XtPointer, XEvent *,
						 Boolean *));

						/* public procedures */

extern void _MolNoticeCheckSetValues OL_ARGS((NoticeShellWidget,NoticeShellWidget));
extern void _MolNoticeRemoveEventHandlers OL_ARGS((Widget));
extern void _MolNoticeSetupFrame OL_ARGS((NoticeShellWidget, ArgList, Cardinal *));
static void PostSelectCB();
/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
/******************************function*header****************************
 * SetupNoticeFrame()
 */

#ifdef OlNeedFunctionPrototypes
extern void
_MolNoticeSetupFrame(NoticeShellWidget new, ArgList args, Cardinal *num_args)
#else
extern void
_MolNoticeSetupFrame(new, args, num_args)
    NoticeShellWidget	new;
    ArgList		args;
    Cardinal *		num_args;
#endif
{
    NoticeShellPart *	nPart = &(new->notice_shell);
    Cardinal		m;
    MaskArg		mArgs[20];
    ArgList		mergedArgs;
    Cardinal		mergedCnt;
    Widget		pane;
    int			notice_width,
			top_margin,
			text_margin,
			control_area_height,
			textarea_width;
   Widget		rectobj_top_margin;

/*
    static Arg		pane_args[] = {
				{ XtNcenter, (XtArgVal) True },
				{ XtNlayoutType, (XtArgVal) OL_FIXEDCOLS },
				{ XtNsameSize, (XtArgVal) OL_NONE },
				{ XtNvSpace, (XtArgVal) 0 },
				};
*/

    /* postSelect callbacks must be added at Create time. */
    static XtCallbackRec popdown_cb[] = {
	{ NULL, NULL },		/* filled in below */
        { NULL, NULL }		/* delimiter */
    };

/* defines (in 10s of points) for 12 point scale only */

#define TEXT_MARGIN 200	/* Margin to left and rt. of text area */
#define TOP_MARGIN 360	/* margin on top and bottom of text area */
#define NOTICE_WIDTH 3900	/* notice widget width */
#define CONTROL_AREA_HEIGHT 360	/* control area height */
#define TEXTAREA_WIDTH (NOTICE_WIDTH - (CHUNK_SMALL_WIDTH * 2))

    /* Add event handler to get window-leave notification.  This is used
	to decide if pointer should be unwarped.  Add event handler to
	get map notification.  This is to beep the display.  Beeping the
	display when Popped Up is too early when system is loaded so
	defer till when window is mapped.
	
     */
    AddLeaveEH(new);
    AddMapEH(new);

    /****************************************************************
     * CREATE FIXED CHILDREN
     */

    /* create the sole child widget of this notice/shell widget */
	notice_width = OlScreenPointToPixel(OL_HORIZONTAL,
				NOTICE_WIDTH, XtScreen(new)) / 10;	
	pane = XtVaCreateManagedWidget("pane", rubberTileWidgetClass,
				(Widget) new,		/* parent (self) */
				XtNorientation, OL_VERTICAL, 
				XtNwidth, notice_width,
				(char *)NULL);

	top_margin = OlScreenPointToPixel(OL_VERTICAL,
					TOP_MARGIN, XtScreen(new)) / 10;

	text_margin = OlScreenPointToPixel(OL_HORIZONTAL,
					TEXT_MARGIN, XtScreen(new)) / 10;

	/* Force rubbertile to be notice_width pixels wide */
	rectobj_top_margin = XtVaCreateManagedWidget("top_mrgn", 
				rectObjClass, pane,
				XtNweight, 0,
				XtNwidth, notice_width,
				XtNheight, 1,
				(char *)0);

    /* create the component parts: text area & control area.
     * some resources can be specified/overidden by the appl:
     * OL_OVERRIDE_PAIR: full control of these resources
     * OL_DEFAULT_PAIR: the appl has some say about these resources
     * OL_SOURCE_PAIR: don't care, include them on appl's behalf
     */

	m = 0;
	_OlSetMaskArg(mArgs[m], XtNvSpace, top_margin, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNhSpace, text_margin, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNalignment, OL_CENTER, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNlineSpace, 0, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNstring, NULL, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNstrip, True, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNwrap, True, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNweight, 1, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNrecomputeSize, False, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNwidth, notice_width,
						OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNalignment, OL_CENTER, OL_SOURCE_PAIR); m++;

	_OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
	nPart->text = XtCreateManagedWidget("textarea",
					staticTextWidgetClass, pane,
					mergedArgs, mergedCnt);
	XtFree(mergedArgs);

    /*
     * create control area: will contain application's controls
     */

    popdown_cb[0].callback	= PostSelectCB;
    popdown_cb[0].closure	= (XtPointer)new;

   control_area_height = OlScreenPointToPixel(OL_VERTICAL,
				CONTROL_AREA_HEIGHT, XtScreen(new));	
    m = 0;
    _OlSetMaskArg(mArgs[m], XtNhPad, 0, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNhSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNsameSize, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNvPad, 0, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNvSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNpostSelect, popdown_cb, OL_OVERRIDE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNweight, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNheight, control_area_height,
						 OL_SOURCE_PAIR); m++;
/*
    _OlSetMaskArg(mArgs[m], XtNlayoutType, OL_FIXEDCOLS,
						 OL_DEFAULT_PAIR); m++;
*/
    _OlSetMaskArg(mArgs[m], XtNlayoutType, OL_FIXEDWIDTH,
						 OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNwidth, notice_width,
						 OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNcenter, TRUE, OL_SOURCE_PAIR); m++;

    _OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
    nPart->control = XtCreateManagedWidget("controlarea",
					   controlAreaWidgetClass,
					   pane, mergedArgs, mergedCnt);
    XtFree(mergedArgs);

}

#if OlNeedFunctionPrototypes
static void
LeaveEH(NoticeShellWidget nw, XtPointer client_data, XEvent *event,
	Boolean *cont_to_dispatch)
#else
static void
LeaveEH(nw, client_data, event, cont_to_dispatch)
    NoticeShellWidget	nw;
    XtPointer		client_data;
    XEvent *		event;
    Boolean *		cont_to_dispatch;
#endif
{
    nw->notice_shell.do_unwarp = False;
}

/******************************function*header****************************
 * MapEH- 
 */

/* ARGSUSED */
#if OlNeedFunctionPrototypes
static void
MapEH(NoticeShellWidget nw, XtPointer client_data, XEvent *event,
	Boolean *cont_to_dispatch)
#else
static void
MapEH(nw, client_data, event, cont_to_dispatch)
    NoticeShellWidget	nw;
    XtPointer		client_data;
    XEvent *		event;
    Boolean *		cont_to_dispatch;
#endif
{
    if (event->type == MapNotify)
	_OlBeepDisplay((Widget)nw, 1);	/* figures out if beep is needed */
	
}

#if OlNeedFunctionPrototypes
extern void
_MolNoticeRemoveEventHandlers(Widget w)
#else
_MolNoticeRemoveEventHandlers(w)
Widget w;
#endif
{
    RemoveLeaveEH(w);
    RemoveMapEH(w);
}

#if OlNeedFunctionPrototypes
extern void
_MolNoticeCheckSetValues(NoticeShellWidget current,NoticeShellWidget new)
#else
extern void
_MolNoticeCheckSetValues(current,new)
NoticeShellWidget current, new;
#endif
{
/* empty */
}

/******************************function*header****************************
 * PostSelectCB():  called when button pressed in Notice
 */

static void
PostSelectCB(nw, closure, call_data)
    Widget nw;
    XtPointer closure;
    XtPointer call_data;
{
    XtPopdown((Widget)closure);
}
