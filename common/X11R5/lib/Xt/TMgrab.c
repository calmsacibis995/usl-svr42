/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:TMgrab.c	1.1"
/* $XConsortium: TMgrab.c,v 1.4 91/04/12 14:02:15 converse Exp $ */
/*LINTLIBRARY*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "IntrinsicI.h"

typedef struct _GrabActionRec {
    struct _GrabActionRec* next;
    XtActionProc action_proc;
    Boolean owner_events;
    unsigned int event_mask;
    int pointer_mode, keyboard_mode;
} GrabActionRec;

static GrabActionRec *grabActionList = NULL;

static void GrabAllCorrectKeys(widget, typeMatch, modMatch, grabP)
    Widget widget;
    TMTypeMatch typeMatch;
    TMModifierMatch modMatch;
    GrabActionRec* grabP;
{
    Display *dpy = XtDisplay(widget);
    KeyCode *keycodes, *keycodeP;
    Cardinal keycount;
    XtKeysymToKeycodeList(
	    dpy,
	    (KeySym)typeMatch->eventCode,
	    &keycodes,
	    &keycount
			 );
    if (keycount == 0) return;
    for (keycodeP = keycodes; keycount--; keycodeP++) {
	if (modMatch->standard) {
	    /* find standard modifiers that produce this keysym */
	    KeySym keysym;
	    int std_mods, least_mod = 1;
	    Modifiers modifiers_return;
	    XtTranslateKeycode( dpy, *keycodeP, (Modifiers)0,
			        &modifiers_return, &keysym );
	    if (keysym == typeMatch->eventCode) {
		XtGrabKey(widget, *keycodeP,
			  (unsigned)modMatch->modifiers,
			  grabP->owner_events,
			  grabP->pointer_mode,
			  grabP->keyboard_mode
			);
		/* continue; */		/* grab all modifier combinations */
	    }
	    while ((least_mod & modifiers_return)==0) least_mod <<= 1;	    
	    for (std_mods = modifiers_return;
		 std_mods >= least_mod; std_mods--) {
		 /* check all useful combinations of modifier bits */
		if (modifiers_return & std_mods) {
		    XtTranslateKeycode( dpy, *keycodeP,
					(Modifiers)std_mods,
					&modifiers_return, &keysym );
		    if (keysym == typeMatch->eventCode) {
			XtGrabKey(widget, *keycodeP,
				  (unsigned)modMatch->modifiers | std_mods,
				  grabP->owner_events,
				  grabP->pointer_mode,
				  grabP->keyboard_mode
				);
			/* break; */	/* grab all modifier combinations */
		    }
		}
	    }
	} else /* !event->standard */ {
	    XtGrabKey(widget, *keycodeP,
		      (unsigned)modMatch->modifiers,
		      grabP->owner_events,
		      grabP->pointer_mode,
		      grabP->keyboard_mode
		    );
	}
    }
    XtFree((char *)keycodes);
}

typedef struct {
    TMShortCard count;
    Widget	widget;
    GrabActionRec *grabP;
}DoGrabRec;

static Boolean DoGrab(state, data)
    StatePtr		state;
    XtPointer		data;
{
    DoGrabRec		*doGrabP = (DoGrabRec *)data;
    GrabActionRec* 	grabP = doGrabP->grabP;
    Widget		widget = doGrabP->widget;
    TMShortCard		count = doGrabP->count;
    TMShortCard		typeIndex = state->typeIndex;
    TMShortCard		modIndex = state->modIndex;
    ActionRec		*action;
    TMTypeMatch		typeMatch = TMGetTypeMatch(typeIndex);
    TMModifierMatch	modMatch = TMGetModifierMatch(modIndex);

    for (action = state->actions; action; action = action->next)
      if (count == action->idx) break;
    if (!action) return False;
    
    switch (typeMatch->eventType) {
      case ButtonPress:
      case ButtonRelease:
	XtGrabButton(
		     widget,
		     (unsigned) typeMatch->eventCode,
		     (unsigned) modMatch->modifiers,
		     grabP->owner_events,
		     grabP->event_mask,
		     grabP->pointer_mode,
		     grabP->keyboard_mode,
		     None,
		     None
		     );
	break;
	
      case KeyPress:
      case KeyRelease:
	GrabAllCorrectKeys(widget, typeMatch, modMatch, grabP);
	break;
	
      case EnterNotify:
	break;
	
      default:
	XtAppWarningMsg(XtWidgetToApplicationContext(widget),
			"invalidPopup","unsupportedOperation",XtCXtToolkitError,
			"Pop-up menu creation is only supported on Button, Key or EnterNotify events.",
			(String *)NULL, (Cardinal *)NULL);
	break;
    }
    return False;
}

void _XtRegisterGrabs(widget)
    Widget widget;
{
    XtTranslations 	xlations = widget->core.tm.translations;
    TMComplexStateTree 	*stateTreePtr;
    unsigned int 	count;
    TMShortCard		i;
    TMBindData   	bindData = (TMBindData) widget->core.tm.proc_table;
    XtActionProc	*procs;

    if (! XtIsRealized(widget) || widget->core.being_destroyed)
	return;

    /* walk the widget instance action bindings table looking for */
    /* actions registered as grab actions. */
    /* when you find one, do a grab on the triggering event */
    
    if (xlations == NULL) return;
    stateTreePtr = (TMComplexStateTree *) xlations->stateTreeTbl;
    if (*stateTreePtr == NULL) return;
    for (i = 0; i < xlations->numStateTrees; i++, stateTreePtr++) {
	if (bindData->simple.isComplex) 
	  procs = TMGetComplexBindEntry(bindData, i)->procs;
	else 
	  procs = TMGetSimpleBindEntry(bindData, i)->procs;
	for (count=0; count < (*stateTreePtr)->numQuarks; count++) {
	    GrabActionRec* grabP;
	    DoGrabRec      doGrab;
	    for (grabP = grabActionList; grabP != NULL; grabP = grabP->next) {
		if (grabP->action_proc == procs[count]) {
		    /* we've found a "grabber" in the action table. Find the 
		     * states that call this action.  Note that if there is 
		     * more than one "grabber" in the action table, we end 
		     * up searching all of the states multiple times.
		     */
		    doGrab.widget = widget;
		    doGrab.grabP = grabP;
		    doGrab.count = count;
		    _XtTraverseStateTree((TMStateTree)*stateTreePtr,
					 DoGrab,
					 (XtPointer)&doGrab);
		}
	    }
	}
    }
}

#if NeedFunctionPrototypes
void XtRegisterGrabAction(
    XtActionProc action_proc, 
    _XtBoolean owner_events,
    unsigned int event_mask,
    int pointer_mode,
    int keyboard_mode
    )
#else
void XtRegisterGrabAction(action_proc, owner_events, event_mask,
			  pointer_mode, keyboard_mode)
    XtActionProc action_proc;
    Boolean owner_events;
    unsigned int event_mask;
    int pointer_mode, keyboard_mode;
#endif
{
    GrabActionRec* actionP;

    for (actionP = grabActionList; actionP != NULL; actionP = actionP->next) {
	if (actionP->action_proc == action_proc) break;
    }
    if (actionP == NULL) {
	actionP = XtNew(GrabActionRec);
	actionP->action_proc = action_proc;
	actionP->next = grabActionList;
	grabActionList = actionP;
    }
#ifdef DEBUG
    else
	if (   actionP->owner_events != owner_events
	    || actionP->event_mask != event_mask
	    || actionP->pointer_mode != pointer_mode
	    || actionP->keyboard_mode != keyboard_mode) {
	    XtWarningMsg(
		"argsReplaced", "xtRegisterGrabAction", XtCXtToolkitError,
		"XtRegisterGrabAction called on same proc with different args"
			);
	}
#endif /*DEBUG*/

    actionP->owner_events = owner_events;
    actionP->event_mask = event_mask;
    actionP->pointer_mode = pointer_mode;
    actionP->keyboard_mode = keyboard_mode;
}

/*ARGSUSED*/
void _XtGrabInitialize(app)
    XtAppContext	app;
{
    if (grabActionList == NULL)
	XtRegisterGrabAction( XtMenuPopupAction, True,
			      (unsigned)(ButtonPressMask | ButtonReleaseMask),
			      GrabModeAsync,
			      GrabModeAsync
			    );

}
