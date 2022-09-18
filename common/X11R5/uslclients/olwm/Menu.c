/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Menu.c	1.82"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/keysym.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookI.h>
#include <Xol/DynamicP.h>
#include <Xol/Olg.h>
#include <Xol/FButtons.h>
#include <Xol/Flat.h>
#include <Xol/MenuShell.h>
#include <Xol/WSMcomm.h>

#include <wm.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>
#include <Extern.h>
#include <Xol/Stub.h>
#include <Xol/MenuShellP.h>
#include <X11/RectObj.h>
#include <signal.h>

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */

static void	InitializeGroupList OL_NO_ARGS();
extern void	ConstructGroupList OL_ARGS((WMStepWidget, int, Boolean));

static int	CheckCascade OL_ARGS((WMStepWidget));
static void	CascadeDefault OL_ARGS((Widget, XtPointer, XtPointer));
extern void	CreateGlobalMenus OL_NO_ARGS();
extern void	CreateStepIcon OL_ARGS((WMStepWidget, WMGeometry *));
extern void	DestroyStepIcon OL_ARGS((WMStepWidget));
extern void	FillHelpTitles OL_ARGS((Widget));
static void	FillMenuLabels OL_ARGS((Widget));
static void	FillMotifMenuLabels OL_ARGS((Widget));
static int	FindOwnerAndGroup OL_ARGS((WMStepWidget));
extern void	GetMenuItems OL_ARGS((WMStepWidget));
static void	GetSpecialMenuItems OL_ARGS((WMStepWidget,
						Global_Menu_Info *));
static void	MenuDefault OL_ARGS((Widget, XtPointer, XtPointer));
static void	SpecialMenuDefault OL_ARGS((Widget, XtPointer, XtPointer));
static void	OpenClose OL_ARGS((WMStepWidget, Widget, Widget, int,
					WMGeometry *));

extern void	Menu OL_ARGS((WMStepWidget, XEvent *, WMPiece));
extern void	MenuBack OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuDismiss OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuDismissPopups OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuFullRestore OL_ARGS((Widget, XtPointer, XtPointer));
extern void	Menu_Move OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuMotifMaximize OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuMotifMinimize OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuMotifRestore OL_ARGS((Widget, XtPointer, XtPointer));

extern void	MenuOpenClose OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuOwner OL_ARGS((Widget, XtPointer, XtPointer));
static void	MenuExpose OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));
extern void	MenuQuit OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuRefresh OL_ARGS((Widget, XtPointer, XtPointer));
extern void	Menu_Resize OL_ARGS((Widget, XtPointer, XtPointer));

extern void 	IconEnterLeave OL_ARGS((Widget, XtPointer, XEvent *,
                                                 Boolean *));


/* For new menu initialization...*/
extern void	MenuInitialize OL_ARGS((Widget));
extern void	UpdateMenuItems OL_ARGS((Widget));
static void	MakeCombinedMenu OL_ARGS((Widget));


extern void	ConsumeExcessiveEvents OL_ARGS((Widget, long));
static void	PopdownMenuCB OL_ARGS((Widget));
static void	MBClick2EH OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void	IconClickEH OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void	WmPostPopupWindowMenu OL_ARGS((Widget, Widget,
			OlVirtualName, OlPopupMenuCallbackProc, Position,
			Position, Position, Position, Dimension, Dimension));



			/* MooLIT - for MWM_MENU property, additional
			 * button callbacks.
			 */

extern void OlwmBeep OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmCircle_Down OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmCircle_Up OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmExec OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmFocus_Color OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmFocus_Key OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmKill OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmLower OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmMaximize OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmMenu OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmMinimize OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmMove OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmNext_Cmap OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmNext_Key OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmNext_Prev_Key OL_ARGS((WMStepWidget, WMHelpDef *, int));
extern void OlwmNop OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmNormalize OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmNormalize_And_Raise  OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmPack_Icons OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmPass_Keys OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmPost_Wmenu OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmPrev_Cmap OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmPrev_Key OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmQuit_Mwm OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmRaise OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmRaise_Lower OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmRefresh_Win OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmRefresh OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmResize OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmRestart OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmSend_Msg OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmSeparator OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmSet_Behavior OL_ARGS((Widget, XtPointer, XtPointer));
extern void OlwmTitle OL_ARGS((Widget, XtPointer, XtPointer));


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static Boolean ResetFocus = False;


static char *menuFields [] = {
    XtNlabel, XtNmnemonic, XtNselectProc, XtNuserData, XtNsensitive,
    XtNaccelerator, XtNdefault, XtNpopupMenu,
};
static WMMenuDef dummy_items[] = { "this is a really long label" };

/* In Open Look mode, this is less significant; but in Motif mode,
 * we break the default menu into 2 different menu panes (or as
 * I call them, flatButton panes).  The first pane has a fixed maximum number
 * of items (no more than restore, move, size, minimize, maximize, and lower).
 * The second, unless knocked off, starts with Close; this is modifiable with
 * the MWM_MENU property.  We've got to have some limit on the maximum number
 * of buttons we allow on this menu.
 */
#define MAX_MENU_BUTTONS_1	36
#define MAX_MENU_BUTTONS_2	18

#ifdef STATICMEM
/* Nothing anymore */
#else

/* Don't allocate static memory for titles - use OlGetMessage */

static WMHelpDef dismissHelp = {
    "popup_dismiss", NULL,
};
    
static WMMenuDef dismissButton = {
    NULL, 'D', NULL, &dismissHelp, True,
};

static WMHelpDef cancelHelp = {
    "popup_cancel", NULL,
};

static WMMenuDef cancelButton = {
    NULL, 'C', MenuDismiss, &cancelHelp, True,
};

static WMHelpDef openHelp = {
    "window_open", NULL,
};

static WMMenuDef openButton = {
    NULL, 'O', MenuOpenClose, &openHelp, True, "a<F5>",
};

static WMHelpDef closeHelp = {
    "window_open", NULL,
};

static WMMenuDef closeButton = {
    NULL, 'C', MenuOpenClose, &closeHelp, True, "a<F5>",
};

static WMHelpDef fullHelp = {
    "window_full", NULL,
};

static WMMenuDef fullButton = {
    NULL, 'F', MenuFullRestore, &fullHelp, True, "a<F10>",
};

static WMHelpDef restoreHelp = {
	"window_full", NULL,
};

static WMMenuDef restoreButton = {
    NULL,'R', MenuFullRestore, &restoreHelp, True, "a<F10>",
};

static WMHelpDef backHelp = {
    "window_back", NULL,
};

static WMMenuDef backButton = {
    NULL, 'B', MenuBack, &backHelp, True,
};

static WMHelpDef refreshHelp = {
    "window_refresh", NULL
};

static WMMenuDef refreshButton = {
    NULL, 'e', MenuRefresh, &refreshHelp, True, "a c <l>",
};

static WMHelpDef ownerHelp = {
	"popup_owner", NULL,
};

static WMMenuDef ownerButton = {
    NULL, 'w', MenuOwner, &ownerHelp, True, "s a<F8>",
};

static WMHelpDef moveHelp = {
	"window_move", NULL,
};

static WMMenuDef moveButton = {
    NULL, 'M', Menu_Move, &moveHelp, True,
};

static WMHelpDef resizeHelp = {
    "window_resize", NULL,
};

static WMMenuDef resizeButton = {
	NULL, 's', Menu_Resize, &resizeHelp, True,
};

static WMHelpDef quitHelp = {
    "window_quit", NULL,
};

static WMMenuDef quitButton = {
    NULL, 'Q', MenuQuit, &quitHelp, True,
};

static WMHelpDef dismissThisHelp = {
	"this_window", NULL,
};

static WMMenuDef dismissThisButton = {
	NULL, 'T', MenuDismiss, &dismissThisHelp, True, "a<F9>",
};

static WMHelpDef dismissAllHelp = {
	"all_popups", NULL,
};

static WMMenuDef dismissAllButton = {
	NULL, 'A', MenuDismissPopups, &dismissAllHelp, True,"s a<F9>",
};

/* Motif mode */

static WMHelpDef motifRestoreHelp = {
	"mot_restore", NULL,
};

static WMMenuDef motifRestoreButton = {
    NULL,'R', MenuMotifRestore, &restoreHelp, True, "a<F5>",
};

static WMHelpDef motminHelp = {
	"mot_min", NULL,
};

static WMMenuDef motifMinimizeButton = {
    NULL,'n', MenuMotifMinimize, &motminHelp, True, "a<F9>",
};

static WMHelpDef motmaxHelp = {
	"mot_max", NULL,
};
static WMMenuDef motifMaximizeButton = {
    NULL,'n', MenuMotifMaximize, &motmaxHelp, True, "a<F10>",
};

#endif /* STATICMEM */

WMMenuDef combined_menu[MAX_MENU_BUTTONS_1];
/* Motif mode "second pane" */
WMMenuDef combined_menu2[MAX_MENU_BUTTONS_2];

WMMenuDef cascade_menu [2];

typedef struct {
    char	*name;
    char	**pText;
} AccelDef;

/* Only do those that are common to both OL and motif ...
AccelDef accelerators [] = {
    { XtNwmOpenCloseKey, &openButton.accelerator, },
    { XtNwmOpenCloseKey, &closeButton.accelerator, },
    { XtNwmSizeKey, &fullButton.accelerator, },
    { XtNwmSizeKey, &restoreButton.accelerator, },
    { XtNwmBackKey, &backButton.accelerator, },
    { XtNwmRefreshKey, &refreshButton.accelerator, },
    { XtNwmOwnerKey, &ownerButton.accelerator, },
    { XtNwmMoveKey, &moveButton.accelerator, },
    { XtNwmResizeKey, &resizeButton.accelerator, },
    { XtNwmQuitKey, &quitButton.accelerator, },
    { XtNwmDismissThisKey, &dismissThisButton.accelerator, } ,
    { XtNwmDismissAllKey, &dismissAllButton.accelerator, },
};
 */

/* These are the common bindings - they appear on that property sheet */
AccelDef accelerators [] = {
    { XtNwmBackKey, &backButton.accelerator, },
    { XtNwmMoveKey, &moveButton.accelerator, },
    { XtNwmResizeKey, &resizeButton.accelerator, },
    { XtNwmQuitKey, &quitButton.accelerator, },
};

#define SHOWN(x) \
 ((x->xnormalsize.max_width == x->xnormalsize.min_width && \
   x->xnormalsize.max_height == x->xnormalsize.min_height) ? False : True)

/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 * InitializeGroupList
 * - Called from ConstructGroupList when in "NoAppend" mode
 */
static void
InitializeGroupList()
{
	/* group_list->is the global WidgetBuffer (list of wm widgets in use) */
	group_list->used = 0;
} /* end of InitializeGroupList */

/*
 * ConstructGroupList
 * Called from:
 *  -  RaiseLowerGroup() with append = NoAppend, and flag = Windowlayering;
 * 
 *  -  Also called from , FindOwnerAndGroup(), MenuOpenClose()
 *     CheckCascade(), DragWindowAround()
 * 
 * Find all windows in window group of w's window (the WMStep widget).
 * Do so by searching window_list->WidgetBuffer, and adding group members to
 * group_list->WidgetBuffer.  If in the group, OR in the WMSelected flag in
 * the wmstep.decorations field of each  WMStepWidget in the group.
 * It appears that the WMSelected flag is unused throughout olwm.
 *
 * New value for append is possible, NoAppendAny.
 * If append == NoAppendAny, then construct new list from scratch (as with
 * NoAppend), and use the WMHints->window_group value instead of the
 * window ID.  Note the difference - it is possible for the window_group
 * value to be a window that olwm isn't aware of (isn't decorated) -
 * an unmapped base window. This operation would amount to gathering
 * all windows in the same group.
 * One more possible append value, NestedNoAppendAny.  If passed this value,
 * find highest possible window group.  Use that window group if the
 * window is decorated AND is not a popup window (for our purposes,
 * has a pushpin).  This is currently only called from one place
 * (and rarely, too), MenuOpenClose(), so it insures that when a window is
 * closed to an icon, or about to opened, we get the same window group members
 * each time.
 */
extern void
ConstructGroupList OLARGLIST((w, append, flag))
	OLARG(WMStepWidget,	w)
	OLARG(int,		append)
	OLGRA(Boolean,		flag)
{
int          i = 0;
Window       window_group;
WMStepWidget temp;
Window		highest_group, target_highest_group;
int		wmstep_kids_index = -2;
WMStepWidget	kid_wmstep;

if (!append || append == NoAppendAny || append == NestedNoAppendAny)
   /* Just above this function */
   InitializeGroupList();

if (append == NestedNoAppendAny) {
	int k;
	window_group = find_highest_group(w, (Window) 0);
	if ( (k = IsWMStepChild(window_group)) != -1) {
		kid_wmstep = (WMStepWidget)wmstep_kids[k];
		if (HasPushpin(kid_wmstep))
			/* Don't put a pinned window into a group with
			 * append == NestedNoAppendAny.  Go back to basic
			 * NoAppendAny.
			 */
			append = NoAppendAny;
		else
			/* No pin present, so stick with the found
			 * window_group.
			 */
			i++;
	}
	else
		append = NoAppendAny;
}
if (i == 0) {
	if (append != NoAppendAny)
		window_group = w-> wmstep.window;
	else
		window_group = w-> wmstep.xwmhints-> window_group;
}
if (window_group == (Window) NULL)
	window_group = w->wmstep.window;

AddWidgetToWidgetBuffer(w, group_list);

/* If Windowlayering and THIS widgets window is the window group leader...
 * then you want to gather all FOLLOWERS of this window (e.g., all windows that
 * have their wmstep->window_group field == window_group set above);
 * Any window that has a transient parent == window_group is sufficient to
 * be in the group as a follower.  And the group leader is in the group. 
 * It appears that you can strike the 2nd assignment statement above for this
 * to work.  No nesting YET...
 */
if (flag && 
	(w-> wmstep.window == window_group) || (append == NoAppendAny) ||
						(append == NestedNoAppendAny))
   /* Traverse window_list->WidgetBuffer for group members */
   for (i = 0; i < window_list->used; i++)
      {
      temp = (WMStepWidget)window_list->p[i];
      if (temp != w &&
#ifdef WITHDRAWNSTATE
	temp->wmstep.size != WMWITHDRAWN &&
#endif
	  /* This first case CAN'T happen if append != NoAppendAny -
	   * because only 1 window can equal the actual window_group!!
	   */
          (temp-> wmstep.window == window_group ||
           temp-> wmstep.transient_parent == window_group ||
           temp-> wmstep.xwmhints-> window_group == window_group ||
	   ((highest_group = find_highest_group(temp,window_group)) ==
							 window_group) ) ) {

		/* Found group member, add to group_list->WidgetBuffer.
		 * You know that temp != w, so that can't be added again
		 * inadvertently as a duplicate;  By looking for the windows
		 * that have the highest window group in common, we will
		 * ultimately gather all windows in the same group,
		 * where window_group is the group that we are looking
		 * for. 
		 * If append == NoAppend, then you will set window_group to
		 * the window ID passed in, regardless of whether it has
		 * a window_group other than itself.  The if statement
		 * above with all the ||'s will pick up any followers
		 * in the group (windows that have a window group equal
		 * to window_group; What if the intent is to pick up
		 * children of THAT window - this will work IF
		 * we look up the window group hierarchy and stop when
		 * we get to "window_group" -  if we find it!
		 * We do this by passing the window_group argument to
		 * find_highest_group.  If we don't pass the argument, it
		 * will look up the window hierarchy until it finds a window
		 * with a window group equal to itself OR a window group that
		 * is a non-decorated window.
		 * For the NoAppendAny case, it will work because
		 * find_highest_group() returns the highest mapped or unmapped
		 * window group.  But it is safe to pass window_group anyway.
		 * The intent is to handle NESTING - window groups within
		 * window groups within group, etc, and be able to pick them
		 * all up.
		 */

         AddWidgetToWidgetBuffer(temp, group_list);
         temp-> wmstep.decorations |= WMSelected;
         }
      else
         temp-> wmstep.decorations &= ~WMSelected;
      }
w-> wmstep.decorations |= WMSelected;

} /* end of ConstructGroupList */

/*
 * CheckCascade
 * - Called from Menu() when putting together info. for the limited menu
 */

#ifdef later

static int CheckCascade(wm)
WMStepWidget wm;
{
Window       window           = wm-> wmstep.window;
Window       window_group     = wm-> wmstep.xwmhints-> window_group;
Window       transient_parent = wm-> wmstep.transient_parent;
int          i;
WMStepWidget temp;

FPRINTF((stderr, "CheckCascade %x\n", wm));

/* Run thru window_list-> find the one that this window says is it's group
 * leader (or at least is in the same group
 */
for (i = 0; i < window_list->used; i++)
   {
   temp = (WMStepWidget)window_list->p[i];
   if (temp-> wmstep.window == window_group || /* if group leader */
       temp-> wmstep.window == transient_parent) /* or parent of another */
      break;
   }

if (i == window_list->used)
   {
   /* Ran through list without success */
   FPRINTF((stderr, "can't find owner\n"));
   InitializeGroupList();
   }
else
   /* Put together group_list->- list of group members, including group leader. */
   ConstructGroupList((WMStepWidget)window_list->p[i], NoAppend, TRUE);

/* If the group had more than two windows, returns non-zero;
 * else return 0.
 */
return (group_list->used > 2);

} /* end of CheckCascade */

#endif

/*
 * CascadeDefault - Open Look Mode Only.
 *
 */

static void
CascadeDefault OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMMenuInfo * WMMenu       = (WMMenuInfo *)client_data;
WMStepWidget wm           = (WMStepWidget)WMMenu->w;
int i;

for (i = 0; i < WMMenu-> num_cascade_items; i++)
   if (cascade_menu [i].defaultItem == True)
      {
      wm-> wmstep.cascade_default = i;
      cascade_menu [i].defaultItem = False;
      break;
      }

/*
XtRemoveEventHandler(WMMenu-> CascadeShell, NoEventMask, True, 
   ClientNonMaskable, NULL);
XtAddEventHandler(WMMenu-> CascadeShell, NoEventMask, True, 
   _OlPopupHelpTree, NULL);

XtDestroyWidget(WMMenu-> CascadeShell);
*/

} /* end of CascadeDefault */

/*
 * FindOwnerAndGroup
 *
 */
static int
FindOwnerAndGroup OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	Window       window_group     = wm-> wmstep.xwmhints-> window_group;
	Window       transient_parent = wm-> wmstep.transient_parent;
	WMStepWidget temp;
	int          i;
	
	for (i = 0; i < window_list->used; i++) {
		temp = (WMStepWidget)window_list->p[i];
		if (temp-> wmstep.window == window_group ||
		    temp-> wmstep.window == transient_parent)
		   break;
	}
	
	if (i == window_list->used) {
		FPRINTF((stderr, "can't find owner\n"));
		InitializeGroupList();
	} else {
		ConstructGroupList((WMStepWidget)window_list->p[i],
					NoAppend, TRUE);
	}
	
	return (group_list->used);
	
} /* end of FindOwnerAndGroup */

/*
 * MenuDefault
 *
 */

static void
MenuDefault OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMMenuInfo * WMMenu = (WMMenuInfo *)client_data;
WMStepWidget wm     = (WMStepWidget)WMMenu->w;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

int i;
int default_pane = 0;
Boolean next_default = True;


	 if (currentGUI == OL_OPENLOOK_GUI && HasFullMenu(wm) && mmstate ||
		(currentGUI == OL_MOTIF_GUI &&
			wmstep->decorations & WMMenuButton &&
			wmstep->menu_functions & WMMenuButtonState) )
		FlipMenuMark(wm);

		/* Can only be combined_menu, but which pane? */
		for (i = 0; i < WMMenu->num_menu_items; i++)
   			if (combined_menu[i].defaultItem == True) {
				wmstep->menu_default = i;
				wmstep->default_cb = combined_menu[i].selectProc;
				next_default = False;
				wmstep->menu_pane_default = default_pane;
				break;
			}
		if (currentGUI == OL_MOTIF_GUI && next_default) {
			default_pane++;
			/* Look for a default item on the next menupane */
			for (i = 0; i < WMMenu->num_menu_items2; i++)
				if (combined_menu2[i].defaultItem == True) {
					wmstep->menu_default = i;
					wmstep->default_cb =
						combined_menu2[i].selectProc;
					wmstep->menu_pane_default = default_pane;
					break;
				} /* if */
		} /* if MOTIF && next_default */
	
	/* Presume that this menu shell is the top of any cascade.  At this
	 * point, no other window menu is posted.
	 */
	NumMenushellsPosted = 0;

/*
XtRemoveEventHandler(WMMenu-> MenuShell, NoEventMask, True, 
   ClientNonMaskable, NULL);
XtAddEventHandler(WMMenu-> MenuShell, NoEventMask, True, 
   _OlPopupHelpTree, NULL);
*/

/* For O.L. 4.0, Window menus do XSetInputFocus() and steal focus away
 * from clients.  Try and return focus to a client if possible - don't
 * return it to a non-existent client, though!  Only do this here if
 * ResetFocus flag is true; this is done in ClientFocusChange() when
 * root window gets FocusIn event with detail == NotifyDetailNone.
 * One reason for doing this here may be for security that the desired
 * window is sure to get focus.
 */
#if defined(CHECKFOCUS)
if (ResetFocus) {
	WMStepWidget temp;
fprintf(stderr,"MenuDefault: Got ResetFocus request\n");
	temp = NextFocus((WMStepWidget)NULL, True);
	if (temp != NULL)
		SetFocus((Widget)temp,GainFocus,1);
	else
		SetFocus(Frame,GainFocus,0);
#ifdef PFOCUS
fprintf(stderr,"MenuDef(): ResetFocus is true\n");
#endif
}
else
	ResetFocus = False;

#endif
} /* end of MenuDefault */

/*
 * SpecialMenuDefault.
 *
 */

static void
SpecialMenuDefault OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
Widget MenuShell = (Widget)client_data;
Global_Menu_Info *gmi_ptr = global_menu_info;
int	i, j, k,
	wmap_index,
	flat_index,
	flat_item;
long	num_items;
int	go;
WMStepWidget	wm;
WMStepPart *wmstep;

	/* -Change current value of menushells_posted and
	 * NumMenushellsPosted.
	 *
	 * There can be many menus up, in a cascade, and one can
	 * be popped down; so we must reduce the value of  CurrentMenuPosted
	 * must be reduced by this number.  I need a way to determine how
	 * to change this number - one was may be to keep it's value
	 * in an array as a string - and keep a count of the number of
	 * menus posted.  So the array is a bunch of ptrs to the names of
	 * the menushells, and an extra field tells me how many menus
	 * are up; can also keep a field in the menu structure to tell
	 * me which one is up.
	 * Why do this?  Because if a cascade of menus is up, then
	 * it may be important to track focus events - not for going
	 * in, but for going out of one menu if the arrow keys are used
	 * to cascade (pop down) BACKWARD.  An example is the combined
	 * menu which has the  CascadeShell.
	 */
	while(gmi_ptr) {
		if (gmi_ptr->MenuShell == MenuShell)
			break;
		gmi_ptr = gmi_ptr->next;
	}

	if (!gmi_ptr)
		/* Shouldn't happen */
		return;
	wm = (WMStepWidget)(gmi_ptr->w);
	wmstep = (WMStepPart *)&(wm->wmstep);
	if (NumMenushellsPosted) {
		if (gmi_ptr->MenuShell ==
				menushells_posted[NumMenushellsPosted])
			/* Popped down "tail" menu in cascade */
			NumMenushellsPosted--;
		else
			/* Is it the first */
			if (menushells_posted[0] == gmi_ptr->MenuShell) {
				/* All the menushells in a cascade must
				 * be coming down.
				 */
				NumMenushellsPosted = 0;
			}
		/* else the menus in a cascade are all popped down at once,
		 * and we get this callback out of order (not from last
		 * of cascade to first.  Just leave it as is.
		 */
	}
			
	 if (currentGUI == OL_MOTIF_GUI &&
			wmstep->decorations & WMMenuButton &&
			wmstep->menu_functions & WMMenuButtonState)
		FlipMenuMark(wm);
	/*
	 * Traverse through each menu pane for the menu popped down, see if
	 * the defaultItem field is set to true.  If it is, the fun begins:
	 * find the corresponding menu item, set the gmi_ptr->menu_default
	 * field to gmi_ptr->mbd.  How to find the corresponding item? 
 	 */

	for (i=0, go = 1; i < gmi_ptr->num_flat_panes && go; i++) {
		num_items = gmi_ptr->buttons_per_pane[i];
		for (j = 0; j < num_items && go; j++) {
			if (gmi_ptr->motif_menu[i][j].defaultItem) {
				/* found a match!  Find the item.  Look for
				 * the jth item on the ith flat pane
				 */
				flat_item = -1;
				go = 0; /* stop on first match */
				for (k = 0; k < gmi_ptr->num_mbmap; k++) {
					
				  if (gmi_ptr->mbmap[k].flatpane_index == i) {
					flat_item++;
					if (flat_item == j) {
						/* Found */
						wmstep->mbd_menu_default =
						   gmi_ptr->menu_default =
							gmi_ptr->mbmap[k].mbd;
/* These can't be used because some windows will not have certain buttons
 * defined in the menu.  For example, a window without the resize function
 * will not have a resize button.  Therefore, the buttons array will not
 * match the buttons.  The buttons array on the gmi_ptr is actually the
 * MAXIMUM number of buttons possible on the menu.
 *
						wmstep->menu_default = j;
						wmstep->menu_default_pane = i;
 */
						break;
				  	} /* if flat_item == j */
				  } /* if */
				} /* for k */
			} /* if defaultItem */
		} /* for j */
	} /* for i */

} /* SpecialMenuDefault */

/*
 * OpenClose
 * When called from MenuOpenClose(), new != NULL if going
 * from normal to iconic (new == icon's position on the screen )
 * It's hard to tell whether it matters if the size passed in
 * is ICONICFULL vs. ICONICNORMAL, etc. - how does this come in here?
 * size = the size the client is going to be:
 *	WMNORMALSIZE
 *	WMICONICFULL
 *	WMICONICNORMAL
 *	WMFULLSIZE
 * new = the position of the clients icon (include x, y, width, height).
 */

static void
OpenClose OLARGLIST((wm, parent, child, size, new))
OLARG(WMStepWidget, wm)
OLARG(Widget, parent)
OLARG(Widget, child)
OLARG(int, size)
OLGRA(WMGeometry *, new)
{
Display * display = XtDisplay(wm);
WMStepPart * wmstep = &wm-> wmstep;
int i;
WMStepWidget temp;
Widget w = IsIconic(wm) ? wmstep->icon_widget : (Widget)wm;

/* update size field in wmstep part of WMStepWidget (e.g., to iconic) */
ResetFocus = False;
wmstep-> size = size;

/* Get the corr. metrics associated with the new size */
wmstep-> metrics = GetMetrics(wm);

/* Draw the "Open" or "Close" lines when going to/from an icon */
DrawStreaks(wm, w-> core.x, w-> core.y, w-> core.width, w-> core.height,
                  new-> x, new-> y, new-> width, new-> height);

/* Move wm to highest index in the window_list->group */
   if (size != WMICONICNORMAL && size != WMICONICFULL) {
	MoveWindow(wm, WMRaise);
	XRaiseWindow(display, XtWindow(wm));
	if (currentGUI == OL_MOTIF_GUI)
		/* Release the map position */
		ReleaseMapPosition(wmstep->icon_map_pos_row,
					wmstep->icon_map_pos_col);
   }

/* The windows entire group is in group_list->array */
for (i = 1; i < group_list->used; i++)
   {
   temp = (WMStepWidget)group_list->p[i];
#ifdef WITHDRAWNSTATE
   if (temp->wmstep.size == WMWITHDRAWN)
	continue;
#endif
   if (size == WMICONICNORMAL || size == WMICONICFULL)
      {
      /* if going from normal to iconic... */
	if (temp->wmstep.size == WMICONICNORMAL ||
				 temp->wmstep.size == WMICONICFULL)
		/* This window is (somehow) in iconic state; 
		 * it may be part of another window group that is iconic-
		 * could be a user error, but don't over-do it, continue.
		 */
		continue;
      temp-> wmstep.size = size;
      temp-> wmstep.protocols |= IgnoreUnmap;
      SetWindowState(temp, IconicState, wm->wmstep.window);
      /* unmap client widget (child) and WMStepWidget - is it sufficient to
       * unmap temp (the parent)?
       */
	/* If the window had input focus, then will this rip it away from it?
	 * If so, then we're O.K., because it would RevertToNone.
	 */
      XtUnmapWidget(temp->wmstep.child);
      XtUnmapWidget(temp);
      }
   else
      {
	/* going from iconic to normal. (The group leader may be going to
	 * full, but we can't...  First, make a security check.  A
	 * window in the group, possibly as a result of a user error,
	 * has it's own icon window (and may in turn have it's own group
	 * that it iconized with it).  If we run into this situation,
	 * then don't unmap this window this window
	 */
	if (temp->wmstep.decorations & WMIconWindowMapped)
		continue;
      temp-> wmstep.size = WMNORMALSIZE;
      SetWindowState(temp, NormalState, wm->wmstep.window);
      XtMapWidget(temp-> wmstep.child);
      XtMapWidget(temp);
      }
   /* For each window in the group, move it to the highest index of the array
    * and raise it. Done for going either way- iconic to normal or vice versa.
    */
   MoveWindow(temp, WMRaise);
   XRaiseWindow(display, XtWindow(temp));
   }
if (size == WMICONICNORMAL || size == WMICONICFULL)
   {
   /* going from normal (or full) to iconic -  IGNORE UNMAP because
    * WE are causing it, not the client calling XUnmapWindow().
    */
   wmstep-> protocols |= IgnoreUnmap;
   SetWindowState(wm, IconicState, wm->wmstep.window);
   /* Unmap widget, then change focus - always in openlook, if autoKeyFocus in
    * motif.
    */
   XtUnmapWidget((Widget)wm);
   XtUnmapWidget(wm->wmstep.child);
   if (wm->wmstep.is_current) {
	/* Get next window for focus - ignore icons */
	WMStepWidget nwm;

	if (currentGUI == OL_OPENLOOK_GUI || motwmrcs->autoKeyFocus) {
		nwm = NextFocus(wm, False);
		if (nwm)
			SetFocus((Widget)nwm, GainFocus, True);
	}
   } /* if current */
	

	/* Create icon widget */
	CreateStepIcon(wm, new);

   if ( (!(wmstep-> decorations & WMIconWindowReparented)) &&
	(wmstep->xwmhints->icon_window != (Window)NULL) )
      {
      /*
       * Reparent an icon window to the WMStepWidget core.window.
       */
      XChangeSaveSet(display, wmstep-> xwmhints-> icon_window, SetModeInsert);
	if (currentGUI == OL_OPENLOOK_GUI) {
		int	xoffset,
			yoffset;
      /* Resize the icon window - it may not be necessary , but do it anyway */
      XResizeWindow(display, wmstep->xwmhints->icon_window,
		ol_icon_image_width, ol_icon_image_height);

		xoffset = (icon_width - ol_icon_image_width) / 2;
		yoffset = ICON_BORDER_WIDTH + ICON_IMAGE_PAD;
      XReparentWindow(display, wmstep-> xwmhints-> icon_window, 
		XtWindow(wm->wmstep.icon_widget),
		xoffset, yoffset);

	} /* Open Look */
	else {
		/* Motif */
		ReparentMotifIconWindow(display, wmstep);
	}

      /* Now set the flag */
      wmstep-> decorations |= WMIconWindowReparented;
	} /* end if(icon_window != NULL) && ~WMIconWindowReparented */
   if (!(wmstep-> decorations & WMIconWindowMapped))
      {
	if (wmstep->xwmhints->icon_window != (Window)NULL)
      		XMapWindow(display, wmstep-> xwmhints-> icon_window);
      wmstep-> decorations |= WMIconWindowMapped;
      }
   XtMapWidget(wm->wmstep.icon_widget);
	
   }
else
   {
   /* Going from iconic to normal (or full?? ) */
   if (wmstep-> decorations & WMIconWindowMapped)
      {
	if (wmstep->xwmhints->icon_window != (Window)NULL) {
		XUnmapWindow(display, wmstep-> xwmhints-> icon_window);
		XReparentWindow(display, wmstep->xwmhints->icon_window, 
			RootWindowOfScreen(XtScreen(wm)), wm-> core.x,
			wm-> core.y);
		XChangeSaveSet(display, wmstep->xwmhints->icon_window,
			SetModeDelete);
		wmstep->decorations &= ~WMIconWindowReparented;
	}
	  if (wmstep->icon_widget) {
		/* If this is Motif mode, what happens to the icon pixmap,
		 * if we decide to go with that way to implement it?
		 */
		DestroyStepIcon(wm);
	  }
      wmstep-> decorations &= ~WMIconWindowMapped;
      }
   /* Set window state property */
   SetWindowState(wm, NormalState, wm->wmstep.window);
   XtConfigureWidget((Widget)wm, new-> x, new-> y, new-> width, new-> height,
									0);
	if (currentGUI == OL_OPENLOOK_GUI)
   XtResizeWidget(wm-> wmstep.child,
			ChildWidth(wm), ChildHeight(wm), 0);
	else
   XtResizeWidget(wm->wmstep.child,
			MChildWidth(wm), MChildHeight(wm), 0);
   XtMapWidget((Widget)wm);
   XtMapWidget(child);
   if (!wmrcs->pointerFocus) {
	/* Fool the window manager by setting the CurrentFocus
	 * variables to the window that is ABOUT to get focus.
	 * It's probably not necessary, but call SetFocus()
	 * anyway.
	 */
      CurrentFocusWindow = CurrentFocusApplication =
	PreviousFocusWindow = PreviousFocusApplication = wm;
	/* "Trick" the window manager into thinking this was the
	 * last window to have input focus - then if we got here via
	 * the menu, and the menu pops down, we will later try to
	 * give focus to the client that has the highest focus
	 * count!  For example, suppose the window menu is used to
	 * open this client; and suppose client C is the last client
	 * that had input focus; We call SetFocus() here to give focus
	 * to the client that is being "opened" (this client); when the
	 * window menu pops down, focus will RevertToNone under the
	 * current scheme; olwm will detect this, and attempt to give
	 * focus to the client that has the highest focus ticket
	 * counter (in wmstep->focus).  This can be no higher than
	 * wm_focus_count (the value returned by FocusCount() ).
	 * So, olwm will then try and give focus to that window C,
	 * Assume C is a globally active client, olwm now sends a
	 * message to C (WM_TAKE_FOCUS).  The first message, sent
	 * to this client being opened here, may get processed first,
	 * and it may take focus first; but the second message
	 * is then processed afterward, and it (client C) will get
	 * the focus.  To avoid client C ever getting the WM_TAKE_FOCUS
	 * message sent to it, simply make the focus ticket counter
	 * in this client being opened the highest possible number!
	 * 
	 * There are times when you don't want to do this:
	 * when the window you are opening is a NoInput window, for
	 * example; another thought is not doing this if the wmstep.focus
	 * field is set to 0, but what if the application was started
	 * in iconic mode?  Then it would have the field set to 0, yet
	 * you'd want it to come up with focus anyway.
	 */
	/* If going to iconic, don't make it current; if going to
	 * normal, make it current.
	 */
	if (!(IsIconic(wm))) {
      		wm->wmstep.focus = FocusCount();
		SetFocus((Widget)wm, GainFocus, 1);
	}
     }
   }
	/* Put this in for windows moving from iconic state,
	 * or from normal to full or full to normal
	 */
	if (size != WMICONICNORMAL && size != WMICONICFULL) {
		int usewidth, useheight, usex, usey;
		if (currentGUI == OL_OPENLOOK_GUI) {
			usewidth = ChildWidth(wm);
			useheight = ChildHeight(wm);
			usex = OriginX(wm);
			usey = OriginY(wm);
		}
		else {
			usewidth = MChildWidth(wm);
			useheight = MChildHeight(wm);
			usex = MOriginX(wm);
			usey = MOriginY(wm);
		}
		SendConfigureNotify(XtDisplay(wm->wmstep.child), 
            	  	XtWindow(wm->wmstep.child), wm->core.x + usex,
			wm->core.y + usey, usewidth, useheight,
			0);
	}
} /* end of OpenClose */

extern void
CreateStepIcon OLARGLIST((wm,icon_geometry))
OLARG(WMStepWidget, wm)
OLGRA(WMGeometry *, icon_geometry)
{
Arg args[10];
int k = 0;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

	XtSetArg(args[k], XtNx, (XtArgVal)icon_geometry->x); k++;
	XtSetArg(args[k], XtNy, (XtArgVal)icon_geometry->y); k++;
	XtSetArg(args[k], XtNwidth, (XtArgVal)icon_geometry->width); k++;
	XtSetArg(args[k], XtNheight, (XtArgVal)icon_geometry->height); k++;
	XtSetArg(args[k], XtNmappedWhenManaged, (XtArgVal)False); k++;
	XtSetArg(args[k], XtNborderWidth, (XtArgVal)0); k++;
	if (wmrcs->iconParentRelative == False ||
					currentGUI == OL_MOTIF_GUI) {
		XtSetArg(args[k], XtNbackground,
					(XtArgVal)wmrcs->iconBackground);
	}
	else {
		XtSetArg(args[k], XtNbackgroundPixmap,
					(XtArgVal)ParentRelative);
	};
	k++;
	wm->wmstep.icon_widget = XtAppCreateShell("olwmXYIcon","OlwmXYIcon",
		shellWidgetClass, XtDisplay((Widget)wm),
		args, k);
	XtRealizeWidget(wm->wmstep.icon_widget);
	if (currentGUI == OL_MOTIF_GUI)
		XDefineCursor(XtDisplay(wmstep->icon_widget),
				XtWindow(wmstep->icon_widget),
						 motifWindowCursor);

	XtAddEventHandler(wmstep->icon_widget, ButtonPressMask |
		 ButtonReleaseMask, False, IconButtonPress, (XtPointer) wm);
	XtAddEventHandler(wmstep->icon_widget, ExposureMask, False,
		 IconExpose, (XtPointer)wm);
	if (wmrcs->pointerFocus)
 		XtAddEventHandler(wmstep->icon_widget,
			EnterWindowMask|LeaveWindowMask,
 		 	False, IconEnterLeave, (XtPointer) wm);

#ifdef RAW
	if (currentGUI == OL_MOTIF_GUI)
		XtInsertRawEventHandler(wmstep->icon_widget, ExposureMask,
			False, IconExposeRaw, (XtPointer)wm, XtListHead);
#endif

	/* Do this to get icon help */
	XtRemoveEventHandler(wmstep->icon_widget, NoEventMask, True,
					 _OlPopupHelpTree, NULL);
	XtAddEventHandler(wmstep->icon_widget, NoEventMask, True,
					 ClientNonMaskable, NULL);
} /* CreateStepIcon */

void
DestroyStepIcon OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Display *display = XtDisplay((Widget)wm);
		XtDestroyWidget(wmstep->icon_widget);
		wmstep->icon_widget = (Widget)NULL;
		if (wmstep->icon_pixmap) {
			XFreePixmap(display, wmstep->icon_pixmap);
			wmstep->icon_pixmap = (Pixmap)NULL;
		}
		if (currentGUI == OL_MOTIF_GUI &&
			motwmrcs->iconDecoration&ICON_ACTIVELABEL)
				CheckAIWDown(wm);
					
} /* DestroyStepIcon */

/*
 * MenuInitialize() - Initialize the window menu(s).  There are two menu
 * types, one a full menu (for base windows) and one a limited menu
 * (for popup windows).
 */

extern void
MenuInitialize OLARGLIST((w))
OLGRA(Widget, w)
{
   MakeCombinedMenu(w);
   UpdateMenuItems(w);
}

/*
 * GetAccelerators() - Accelerator strings are based on dynamic resources.
 * Get the accelerators from the resource manager and convert them to the
 * displayed format.  
 */

static void
GetAccelerators OLARGLIST((w))
    OLGRA(Widget, w)
{
    XtResource	accelResources [XtNumber (accelerators)];
    XtResource	*resrc;
    char	*accelStrings [XtNumber (accelerators)];
    char	**str;
    AccelDef	*accel;
    Cardinal	offset;
    char 	*tmp_ptr;

    for (resrc=accelResources, accel=accelerators, str=accelStrings, offset=0;
	 accel < accelerators + XtNumber (accelerators);
	 resrc++, accel++, str++, offset+=sizeof (char *))
    {
	resrc->resource_name	= accel->name;
	resrc->resource_class	= accel->name;
	resrc->resource_type	= XtRString;
	resrc->resource_size	= sizeof(String);
	resrc->resource_offset	= offset;
	resrc->default_type	= XtRString;
	resrc->default_addr	= (XtPointer)NULL;
    }

    XtGetApplicationResources(w, (XtPointer) accelStrings,	
			      accelResources, XtNumber (accelerators),
			      NULL, 0);

    /* Replace the old accelerator strings with the new ones.  Chances are
     * that the strings did not change, but this routine shouldn't be called
     * much, so it's not horribly expensive to do this.
     */

    for (accel=accelerators, str=accelStrings;
	 accel < accelerators + XtNumber (accelerators);
	 accel++, str++)
    {
	tmp_ptr = *str;
	if (tmp_ptr) {
		tmp_ptr = strchr(*str,',');
		if (tmp_ptr)
			*tmp_ptr = '\0';
	}
	*accel->pText = *str;
    }
} /* end of GetAccelerators() */

extern void
GetMenuItems OLARGLIST((wm))
    OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
unsigned long menufuncs = wmstep->menu_functions;
int add_items = 0;
int next = 0; /* Keep track of number of items on pane 1 */
int i,k;
int num_pane2_items = 0;	/* number of items on pane2 it motif mode */
Global_Menu_Info *gmi_ptr = global_menu_info;
int shown_flag = SHOWN(wmstep);
unsigned int	CurrentMenuPosted = 0; /* note that this is LOCAL */

    Arg    menuargs[3];

	if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		while (gmi_ptr)
			if (!strcmp(gmi_ptr->menuname,
						wmstep->csinfo->windowMenu))
				break;
		if (gmi_ptr)
			GetSpecialMenuItems(wm, gmi_ptr);
		return;
	}
	if (currentGUI == OL_MOTIF_GUI) {
		int k = 0;
		num_pane2_items = 0;

		/* In motif mode, set CurrentMenuPosted to FULLMENU, because
		 * there is no additional cascade menu; having the additinoal
		 * value of LIMITEDMENU for CurrentMenuPosted is important
		 * if there is a cascade menu, because it affects menu
		 * focus when popping the cascade down; no problem here.
		 */
		CurrentMenuPosted = FULLMENU;
		menushells_posted[NumMenushellsPosted++] = 
						WMCombinedMenu->MenuShell;

		combined_menu[k] = motifRestoreButton;
		/* first button is always restore */
		switch(wmstep->size) {
			case WMICONICNORMAL:
			case WMFULLSIZE:
			case WMICONICFULL:
				combined_menu[k++].sensitive = True;
				break;
			default:
				combined_menu[k++].sensitive = False;
				break;
		}
		/* Second button may be Move IFF function is present */
		if (menufuncs & WMFuncMove)
			combined_menu[k++] = moveButton;
		/* 3rd button may be resize IFF function */
		if (menufuncs & WMFuncResize) {
			combined_menu[k] = resizeButton;
			if (IsIconic(wm))
				combined_menu[k++].sensitive = False;
			else
				combined_menu[k++].sensitive = True;
		}
		/* Next: minimize, if functinality exists */
		if (menufuncs & WMFuncMinimize) {
			combined_menu[k] = motifMinimizeButton;
			if (IsIconic(wm))
				combined_menu[k++].sensitive = False;
			else
				combined_menu[k++].sensitive = True;
		}
		/* Next: maximize, if functionality permits */
		if (menufuncs & WMFuncMaximize) {
			combined_menu[k] = motifMaximizeButton;
			switch(wm-> wmstep.size) {
   				case WMICONICFULL:
   				case WMFULLSIZE:
					combined_menu[k++].sensitive = False;
					break;
				default:
					combined_menu[k++].sensitive=shown_flag;
					break;
			} /* switch */
		} /* menufuncs & WMFuncMaximize */
		/* everyone gets a "lower" button */
		combined_menu[k++] = backButton;

		/* With at least 2 menu panes, end the first count here */
		WMCombinedMenu->num_menu_items = k;

		/* Only get a quit ("Close") button if functionality permits */
		/* Update: force it to have a close button to be sure that
		 * the second menu pane (under the separator) has at least
		 * one button; then (de)sensitize it based on the
		 * functionality.  This differs from mwm: mwm will just
		 * forget about the lower menu pane area and not draw
		 * the separator.  Before this change, olwm would remove
		 * the close button from the lower menu pane (so it would
		 * have 0 items) if it didn't have close functionality.
		 * This would leave a separator and an empty space below
		 * it on menus that had no other buttons in the second
		 * pane!
		 */
		k = 0;
		combined_menu2[k] = quitButton;
		if (menufuncs & WMFuncClose)
			combined_menu2[k].sensitive = True;
		else
			combined_menu2[k].sensitive = False;
		k++;
		num_pane2_items = k;

		/* next will now stand for the index into the 2nd array.
		 * Any buttons added from here will be additions from MWM_MENU
		 */
		next = k;
	} /* CurrentGUI == OL_MOTIF_GUI */
	else { /* Open Look mode */

    /* In open look mode, better make sure that the  decorations &
     * WMHasFullMenu or WMHasLimitedMenu is set, because EVERY open look
     * menu is either full or limited, and must contain the appropriate
     * labels.
     */


    if (wm-> wmstep.decorations & WMHasFullMenu)
    {
	CurrentMenuPosted = FULLMENU;
	menushells_posted[NumMenushellsPosted++] =
					WMCombinedMenu->MenuShell;
	switch (wm->wmstep.size) {
	case WMNORMALSIZE:
	    combined_menu [0] = closeButton;
	    combined_menu [1] = fullButton;
	    combined_menu[1].sensitive=shown_flag;
	    break;

	case WMFULLSIZE:
	    combined_menu [0] = closeButton;
	    combined_menu [1] = restoreButton;
	    break;

	case WMICONICNORMAL:
	    combined_menu [0] = openButton;
	    combined_menu [1] = fullButton;
	    combined_menu[1].sensitive=shown_flag;
	    break;

	case WMICONICFULL:
	    combined_menu [0] = openButton;
	    combined_menu [1] = restoreButton;
	    break;
	}

	combined_menu [2] = backButton;
	combined_menu [3] = refreshButton;
	combined_menu [4] = moveButton;
	combined_menu [5] = resizeButton;
	combined_menu [6] = quitButton;
	next = 7;
	/* WMCombinedMenu->num_menu_items = 7; */
    }
    else
    {
	int pop_them_all = 0;

	CurrentMenuPosted = LIMITEDMENU;
	menushells_posted[NumMenushellsPosted++] =WMCombinedMenu->MenuShell;
	menushells_posted[NumMenushellsPosted++] =WMCombinedMenu->CascadeShell;

	combined_menu [0] = (wm-> wmstep.decorations & WMCancel) ?
	    cancelButton : dismissButton;
	combined_menu [1] = backButton;
	combined_menu [2] = refreshButton;
	combined_menu [3] = ownerButton;
	combined_menu [4] = moveButton;
	combined_menu [5] = resizeButton;
	/*WMCombinedMenu->num_menu_items = 6; */
	next = 6;

	/* I want a cascade menu if one OR two buttons... */
	combined_menu [0].subMenu = WMCombinedMenu->CascadeShell;

	cascade_menu [0] = dismissThisButton;
	cascade_menu [1] = dismissAllButton;
	if (FindOwnerAndGroup(wm) == 0)
	{
	    Window leader;
	    int idx;
	    leader = find_highest_group(wm, (Window) 0);
	    if ( (idx=IsWMStepChild(leader)) < 0)
	    {
		ConstructGroupList(wm, NoAppendAny,TRUE);
		if (group_list->used > 1) pop_them_all++;
	    }
	}
	else 
	    if (group_list->used > 2)
		pop_them_all++;

	WMCombinedMenu->num_cascade_items = pop_them_all ? 2 : 1;
		

	cascade_menu [wm->wmstep.cascade_default].defaultItem = True;

	XtSetArg(menuargs[0], XtNitems, cascade_menu);
	XtSetArg(menuargs[1], XtNnumItems, WMCombinedMenu->num_cascade_items);
	XtSetValues(WMCombinedMenu->cascade, menuargs, 2);

    }

    /* (De)sensitize labels based on Motif hints */
    if (wmstep->decorations & WMUsesMwmh) {
       combined_menu [4].sensitive =
	(wm->wmstep.menu_functions & WMFuncMove) ?
	    TRUE : FALSE;
	if (wmstep->menu_functions & WMFuncClose) {
		if (CurrentMenuPosted == FULLMENU)
			combined_menu[6].sensitive = True;
		else
			combined_menu[0].sensitive = True;
	}
	else {
		if (CurrentMenuPosted == FULLMENU)
			combined_menu[6].sensitive = False;
		else
			combined_menu[0].sensitive = False;
	}
	if (CurrentMenuPosted == FULLMENU) {
		if(wmstep->menu_functions & WMFuncMinimize)
			combined_menu[0].sensitive = True;
		else
			combined_menu[0].sensitive = False;
		if (wmstep->menu_functions & WMFuncMaximize)
			combined_menu[1].sensitive = True;
		else
			combined_menu[1].sensitive = False;
	}
    } /* UsesMwmh */

    /* (De)sensitize resize label if (not) resizeable. */
    combined_menu[5].sensitive =
	(wm->wmstep.decorations & WMResizable) && !IsIconic(wm) ?
	    TRUE : FALSE;

   } /* else Open Look mode */

    /* Add items on to the end */
	/* Will add items onto the end of pane1 (the only pane) in OL
	 * mode, and pane2 in motif mode
	 */
    for (i=0; i < wmstep->private_buttons_used && next < MAX_MENU_BUTTONS_2;
							 i++) {
	if (wmstep->transient_parent) {
		if (wmstep->private_buttons[i].selectProc == OlwmMinimize ||
		(wmstep->private_buttons[i].selectProc == OlwmMaximize))
			continue;
	}
	/* Check functionality */
	if (wmstep->decorations & WMUsesMwmh) {
		if (!(wmstep->menu_functions & WMFuncClose) && 
		wmstep->private_buttons[i].selectProc == OlwmKill)
			continue;
		if (!(wmstep->menu_functions & WMFuncMinimize) && 
		wmstep->private_buttons[i].selectProc == OlwmMinimize)
			continue;
		if (!(wmstep->menu_functions & WMFuncMaximize) && 
		wmstep->private_buttons[i].selectProc == OlwmMaximize)
			continue;
		if (!(wmstep->menu_functions & WMFuncMove) && 
		wmstep->private_buttons[i].selectProc == OlwmMove)
			continue;
		if (!(wmstep->menu_functions & WMFuncResize) && 
		wmstep->private_buttons[i].selectProc == OlwmResize)
			continue;
	} /* if uses mwmh */

	/* Use combined_menu2 for all these extras */

	combined_menu2[next] = wmstep->private_buttons[i];	
	/* De-sensitize button in some cases */
	if (combined_menu2[next].selectProc == OlwmResize) {
		 if ( (wm->wmstep.decorations & WMResizable) && !IsIconic(wm) &&
					!shown_flag)
			combined_menu2[next].sensitive = True;
		  else
			combined_menu2[next].sensitive = False;
	} /* if selectProc == OlwmResize */
	else
	if (combined_menu2[next].selectProc == OlwmSend_Msg) {
		Boolean sensitive = False;
		WMHelpDef *hd =
			combined_menu2[next].helpData;
		/* Desensitize label if not in my message list */
		if (hd != NULL) {
			long msg_num = (long)hd->menuargs;
			int k;
			if (wmstep->mwm_msgs) {
				for (k=0; k < wmstep->num_mwm_msgs; k++)
					if (wmstep->mwm_msgs[k] == msg_num) {
						sensitive = True;
						break; /* for loop */
					} /* little if */
			} /* if wmstep->mwm_msgs */
		} /* if (hd) */
		combined_menu2[next].sensitive = sensitive;
	} /* if (selectProc == OlwmSend_Msg) */
	else
	  if (combined_menu2[next].selectProc == OlwmMenu) {
		/* Check for menus presence */
		Boolean sensitive = False;
		WMHelpDef *hd = combined_menu2[next].helpData;

		gmi_ptr = global_menu_info;

		/* Desensitize label if not in my message list */
		if (hd != NULL) {
			char *menuname = (char *)hd->menuargs;
			int k;
			while(gmi_ptr) 
				if (gmi_ptr->menuname && !strcmp(gmi_ptr->menuname,
		menuname))
				break;
			else
				gmi_ptr = gmi_ptr->next;
				if (gmi_ptr) {
					sensitive = True;
			combined_menu2[next].subMenu =
					gmi_ptr->MenuShell;
					gmi_ptr->w = (Widget)wm;
				GetSpecialMenuItems(wm,
					gmi_ptr);
				} /* if(gmi_ptr) */
		} /* if (hd) */
		combined_menu2[next].sensitive = sensitive;
		
	  } /* selectProc == OlwmMenu */
	else
	  if (combined_menu2[next].selectProc == OlwmNormalize ||
		combined_menu2[next].selectProc == OlwmNormalize_And_Raise) {
			Boolean sensitive = False;
			switch(wmstep->size) {
				case WMNORMALSIZE:
					break;
				case WMICONICFULL:
				case WMFULLSIZE:
					sensitive = True;
					break;
				case WMICONICNORMAL:
					sensitive = True;
					break;
				default: /* Should't happen */
					sensitive = True;
					break;
			} /* switch */
			combined_menu2[next].sensitive = sensitive;
	  } /* OlwmNormalize */
	else
		combined_menu2[next].sensitive = True;
	/* increment next, then continue with for loop */
	next++;
    } /* for (i < wmstep->private_buttons_used) */

    /* First menu - always one menu (now) for Open Look, >=1 for Motif */
    if (currentGUI == OL_OPENLOOK_GUI) {
	WMCombinedMenu->num_menu_items = next;
    }
    else 
	WMCombinedMenu->num_menu_items2 = next;

	/* Before setting up menu items, set Default Item */
	/* No guarantees which menu pane it's on, so use the
	 * wmstep->menu_pane_default field.
	 */
    if (wm->wmstep.menu_pane_default == 0)
	combined_menu[wm->wmstep.menu_default].defaultItem = True;
    else
	if (currentGUI == OL_MOTIF_GUI && wm->wmstep.menu_pane_default == 1)
		combined_menu2[wm->wmstep.menu_default].defaultItem = True;

    XtSetArg(menuargs[0], XtNitems, combined_menu);
    XtSetArg(menuargs[1], XtNnumItems, WMCombinedMenu->num_menu_items);
    XtSetArg(menuargs[2], XtNitemsTouched, True);
    XtSetValues(WMCombinedMenu->menu, menuargs, 3);

    if (currentGUI == OL_MOTIF_GUI) {
	XtSetArg(menuargs[0], XtNitems, combined_menu2);
	XtSetArg(menuargs[1], XtNnumItems, WMCombinedMenu->num_menu_items2);
	XtSetArg(menuargs[2], XtNitemsTouched, True);
	XtSetValues(WMCombinedMenu->menu2, menuargs, 3);
    }

} /* end of GetMenuItems */

/*
 * GetSpecialMenuItems.
 *
 * Get menu items on a special window menu - in motif mode -
 * from windowMenu client specific resource.
 */
static void
GetSpecialMenuItems OLARGLIST((wm, gmi_ptr))
OLARG(WMStepWidget, wm)
OLGRA(Global_Menu_Info *, gmi_ptr)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
unsigned long menufuncs = wmstep->menu_functions;
int next = 0; /* Keep track of number of items on pane 1 */
int i,k;
int flatindx;
int num_pane2_items = 0;	/* number of items on pane2 it motif mode */
Widget	flat_widget;
int	buttons_per_pane[MAX_FLAT_PANES];
int	flat_index;
int	wmap_index;
WMMenuButtonData	*mbd;
Arg    menuargs[3];

	if (gmi_ptr == NULL)
		/* Shouldn't get here */
		return;

	memset(buttons_per_pane, 0, MAX_FLAT_PANES * sizeof(int));

	gmi_ptr->w = (Widget)wm;

		/* In motif mode, set CurrentMenuPosted to FULLMENU, because
		 * there is no additional cascade menu; having the additinoal
		 * value of LIMITEDMENU for CurrentMenuPosted is important
		 * if there is a cascade menu, because it affects menu
		 * focus when popping the cascade down; no problem here.
		 */
	for (i=0; i < NumMenushellsPosted; i++)
		if (menushells_posted[i] == gmi_ptr->MenuShell)
			/* Looks to be a repeat menu shell in the cascade,
			 * and a potentially recursive menu posting
			 */
			return;

	menushells_posted[NumMenushellsPosted++] = gmi_ptr->MenuShell;

	for (i=0; i < gmi_ptr->num_mbmap; i++) {

			wmap_index = gmi_ptr->mbmap[i].wmap_index;

			if (gmi_ptr->wmap[wmap_index].type == GMI_FLAT) {
				flat_index = 
					gmi_ptr->mbmap[i].flatpane_index;

				/* This is a flat button - what to do?
				 * Check to see if the menu button can
				 * go on the pane - check the functions
				 * for resizable, movable, etc.
				 */
				mbd = gmi_ptr->mbmap[i].mbd;
				if (mbd->menucb == OlwmResize &&
				   !(wmstep->menu_functions & WMFuncResize)
				  || mbd->menucb == OlwmMove &&
				   !(wmstep->menu_functions & WMFuncMove)
				  || mbd->menucb == OlwmMinimize &&
				   !(wmstep->menu_functions & WMFuncMinimize)
				  || mbd->menucb == OlwmMaximize &&
				   !(wmstep->menu_functions & WMFuncMaximize)
				  || mbd->menucb == OlwmKill &&
				   !(wmstep->menu_functions & WMFuncClose))
					continue;

				/* It's safe to put the item in the
				 * WMMenuDef array
				 */

		gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]] =
				gmi_ptr->mbmap[i].menu_button;

		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				defaultItem =
			(gmi_ptr->mbd == gmi_ptr->menu_default) ? True : False;

		if (menufuncs & WMFuncResize && mbd->menucb == OlwmResize) {
		 if (IsIconic(wm))
		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				sensitive = False;
		 else
		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				sensitive = True;
		} /* if OlwmResize */
		/* Next: minimize, if functinality exists */
		if (menufuncs & WMFuncMinimize && mbd->menucb == OlwmMinimize) {
			if (IsIconic(wm))
		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				sensitive = False;
			else
		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				sensitive = True;
		}
		/* Next: maximize, if functionality permits */
		if (menufuncs & WMFuncMaximize && mbd->menucb == OlwmMaximize) {
			switch(wm-> wmstep.size) {
   				case WMICONICFULL:
   				case WMFULLSIZE:
		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				sensitive = False;
					break;
				default:
		  gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
				sensitive = True;
					break;
			} /* switch */
		} /* menufuncs & WMFuncMaximize */

	if (mbd->menucb == OlwmSend_Msg) {
		Boolean sensitive = False;
		int msg_num = (mbd->args ? (int)mbd->args : -1);

		/* Desensitize label if not in my message list */
		if (msg_num != -1) {
			if (wmstep->mwm_msgs) {
				for (k=0; k < wmstep->num_mwm_msgs; k++)
					if (wmstep->mwm_msgs[k] == msg_num) {
						sensitive = True;
						break; /* for loop */
					} /* little if */
			} /* if wmstep->mwm_msgs */
		} /* if (msg_num != -1) */
	gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
						sensitive = sensitive;
	} /* if (selectProc == OlwmSend_Msg) */

	  if (mbd->menucb == OlwmNormalize ||
		mbd->menucb == OlwmNormalize_And_Raise) {
			Boolean sensitive = False;
			switch(wmstep->size) {
				case WMNORMALSIZE:
					break;
				case WMICONICFULL:
				case WMFULLSIZE:
					sensitive = True;
					break;
				case WMICONICNORMAL:
					sensitive = True;
					break;
				default: /* Should't happen */
					sensitive = True;
					break;
			} /* switch */
	gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
						sensitive = sensitive;
	  } /* OlwmNormalize */

	/* Any submenus ?? */
	if (mbd->menucb == OlwmMenu) {
		char *menuname = (mbd->args ? (char *)mbd->args :
				(char *)NULL);
	gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
						sensitive = False;
		if (menuname) {
			/* find the menu */
			Global_Menu_Info *addgmi_ptr = global_menu_info;
			while(addgmi_ptr) {
				if (!strcmp(menuname, addgmi_ptr->menuname))
					break;
				addgmi_ptr = addgmi_ptr->next;
			}
			if (addgmi_ptr) {
	/***********
	 * We have to detect recursion if there is any here - otherwise,
	 * we set ourself up for a crash.
	 **********
	 */
	gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].subMenu =
					addgmi_ptr->MenuShell;
	gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
					sensitive = True;
				/* Do you like recursion?  I do when
				 * it looks opportune - does here
				 */
				GetSpecialMenuItems(wm, addgmi_ptr);
			} /* if gmi_ptr */
		} /* if menuname */
	} /* if mbd->menucb == OlwmMenu */
	if (wmstep->mbd_menu_default && wmstep->mbd_menu_default == mbd)
		gmi_ptr->motif_menu[flat_index][buttons_per_pane[flat_index]].
					defaultItem = True;
			
	buttons_per_pane[flat_index]++;
			
			} /* if FLAT */
	} /* end big for */

	if (!wmstep->mbd_menu_default) {
		/* No default yet, just specify first available one */
		for (i=0; i < MAX_FLAT_PANES; i++)
			if (buttons_per_pane[i] > 0)
		{
			gmi_ptr->motif_menu[i][0].defaultItem = True;
			break;
		}
	}
		
	for (i=0; i < gmi_ptr->num_wmap; i++) {
		if (gmi_ptr->wmap[i].type == GMI_FLAT) {
			flatindx = gmi_ptr->wmap[i].flatpane_index;
			XtSetArg(menuargs[0], XtNitems,
				gmi_ptr->motif_menu[flatindx]);
			XtSetArg(menuargs[1], XtNnumItems,
				buttons_per_pane[flatindx]);
			XtSetArg(menuargs[2], XtNitemsTouched, True);
			XtSetValues(gmi_ptr->wmap[i].widget,
				menuargs, 3);
		}
	}

} /* end of GetSpecialMenuItems */




/*
 * UpdateMenuItems() - routine responsible for insuring that the menu items
 * display the proper accelerator text.
 * Called from MenuInitialize() and from WMDynamic()
 */
extern void
UpdateMenuItems OLARGLIST((w))
	OLGRA(Widget, w)
{
	GetAccelerators (w);

	/* While it's VERY unlikely that the dynamic resources will change
	 * when a menu is posted, accidents will happen.  Force the menu
	 * items to be recalculated.
	 */

	if (NumMenushellsPosted)
	    GetMenuItems((WMStepWidget) WMCombinedMenu->w);

} /* end of UpdateMenuItems() */

static void
MakeCombinedMenu OLARGLIST((parent))
OLGRA(Widget, parent)
{
int i;
Arg    menuargs[10];

	if ( ( WMCombinedMenu = 
		(WMMenuInfo *) XtMalloc((unsigned)sizeof(WMMenuInfo)))
				== (WMMenuInfo *) NULL) {
#if !defined(I18N)
		fprintf(stderr,"Window Manager Error: No Space, exiting...\n");
		exit(1);
#else
	OlVaDisplayErrorMsg(XtDisplay(parent), OleNspace, OleTexit,
OleCOlClientOlwmMsgs, OleMspace_exit, NULL);
#endif	
	}
	WMCombinedMenu->menu_items = combined_menu;
	WMCombinedMenu->menu_items2 = combined_menu2;
	WMCombinedMenu->num_menu_items  = 0;
	WMCombinedMenu->num_menu_items2  = 0;
	WMCombinedMenu->w	      = (Widget) NULL;

	/* For limited menu, cascade_items is the labels and callbacks necessary
	 * for the cascade menu off the limited menu (Dismiss label)
	 */
	WMCombinedMenu->cascade_items = cascade_menu;
	WMCombinedMenu->num_cascade_items = 0;

	i = 0;
	if (currentGUI == OL_OPENLOOK_GUI) {
		XtSetArg(menuargs[i], XtNtitle,
			OlGetMessage(XtDisplay(parent), NULL,  0,
                        	OleNtitle, OleTpopupWindowMenu,
                        	OleCOlClientOlwmMsgs,
				OleMtitle_popupWindowMenu,
                        	(XrmDatabase)NULL) ); i++;
		XtSetArg(menuargs[i], XtNhasTitle,
			True); i++;
	}
	XtSetArg(menuargs[i], XtNpushpin, OL_NONE); i++;
	
	WMCombinedMenu->MenuShell = XtCreatePopupShell("WMC",
			 popupMenuShellWidgetClass, Frame, menuargs, i);

/*
	if (currentGUI == OL_MOTIF_GUI) {
		XtAddEventHandler(WMCombinedMenu->MenuShell,
			MapNotify, False, MenuMapNotify, NULL);
	}
 */
	if (currentGUI == OL_MOTIF_GUI) {
		XtAddEventHandler(WMCombinedMenu->MenuShell,
			ExposureMask, False, MenuExpose,
			&(WMCombinedMenu->MenuShell));
	}

	XtSetArg(menuargs[0], XtNitemFields, menuFields);
	XtSetArg(menuargs[1], XtNnumItemFields, XtNumber (menuFields));
	XtSetArg(menuargs[2], XtNlayoutType, OL_FIXEDCOLS);
	XtSetArg(menuargs[3], XtNmeasure, 1);
	XtSetArg(menuargs[4], XtNclientData, &WMCombinedMenu->w);
	XtSetArg(menuargs[5], XtNitems, dummy_items);
	XtSetArg(menuargs[6], XtNnumItems, XtNumber(dummy_items));
	WMCombinedMenu->menu = XtCreateManagedWidget ("wm_main_menu",
				      flatButtonsWidgetClass,
				      WMCombinedMenu->MenuShell,
				      menuargs, 7);

	if (currentGUI == OL_MOTIF_GUI) {
		/* Create the separator, but don't give a width (not
		 * necessary now.  Add an event handler such that
		 * when the menu pops up, we redraw the separator.
		 */
		WMCombinedMenu->separator = XtVaCreateManagedWidget(
				"separator1", rectObjClass,
				WMCombinedMenu->MenuShell,
				XtNheight, 4,
				(char *)0);

		WMCombinedMenu->menu2 = XtCreateManagedWidget("wm_second_menu",
				      flatButtonsWidgetClass,
				      WMCombinedMenu->MenuShell,
				      menuargs, 7);
	}

   if (currentGUI == OL_OPENLOOK_GUI) {
	XtSetArg(menuargs[0], XtNhasTitle, False);
	XtSetArg(menuargs[1], XtNpushpin, OL_NONE);
	WMCombinedMenu->CascadeShell = XtCreatePopupShell("WMCascadeShell",
			  popupMenuShellWidgetClass, WMCombinedMenu->MenuShell,
			  menuargs, 2);

	XtSetArg(menuargs[0], XtNitemFields, menuFields);
	XtSetArg(menuargs[1], XtNnumItemFields, XtNumber (menuFields));
	XtSetArg(menuargs[2], XtNlayoutType, OL_FIXEDCOLS);
	XtSetArg(menuargs[3], XtNmeasure, 1);
	XtSetArg(menuargs[4], XtNclientData, &WMCombinedMenu->w);
	XtSetArg(menuargs[5], XtNitems, dummy_items);
	XtSetArg(menuargs[6], XtNnumItems, XtNumber(dummy_items));

	WMCombinedMenu->cascade = XtCreateManagedWidget ("wm_cascade_menu",
				      flatButtonsWidgetClass,
				      WMCombinedMenu->CascadeShell,
				      menuargs, 7);
    } /* Open Look */
    else
	WMCombinedMenu->CascadeShell = WMCombinedMenu->cascade = (Widget)NULL;

	/* The WMCombinedMenu struct will have
	 * the WMStepWidget in one of the fields within that was
	 * being worked on at the time this was called.  It must be
	 * set in Menu().  Here, add a popdown callback for getting
	 * and setting the new menu default (if it changes)
	 */
	XtAddCallback(WMCombinedMenu->MenuShell, XtNpopdownCallback,
				MenuDefault, (XtPointer) WMCombinedMenu);
	XtRemoveEventHandler(WMCombinedMenu->MenuShell, NoEventMask, True,
					 _OlPopupHelpTree, NULL);
	XtAddEventHandler(WMCombinedMenu->MenuShell, NoEventMask, True,
					 ClientNonMaskable, NULL);


   if (currentGUI == OL_OPENLOOK_GUI) {
	XtAddCallback(WMCombinedMenu->CascadeShell, XtNpopdownCallback,
		      CascadeDefault, (XtPointer) WMCombinedMenu);
	XtRemoveEventHandler(WMCombinedMenu->CascadeShell, NoEventMask,
			     True, _OlPopupHelpTree, NULL);
	XtAddEventHandler(WMCombinedMenu->CascadeShell, NoEventMask, True,
			  ClientNonMaskable, NULL);
   }

	if (currentGUI == OL_OPENLOOK_GUI)
		FillMenuLabels(parent);
	else {
		FillMotifMenuLabels(parent);
		/* Minor adjustments */
		backHelp.helpfile = XtMalloc(strlen("mot_lower")+1);
		strcpy(backHelp.helpfile, "mot_lower");
		quitHelp.helpfile = XtMalloc(strlen("mot_close") + 1);
		strcpy(quitHelp.helpfile, "mot_close");
	}
} /* MakeCombinedMenu */



static char *addons[14] = {
"all",
"quit",
"move",
"back",
"full",
"open",
"this",
"close",
"owner",
"cancel",
"resize",
"dismiss",
"restore",
"refresh",
};

/*
 * FillMenuLabels.
 *  Fill in Open Look menu labels and mnemonics
 */
static void
FillMenuLabels OLARGLIST((w))
OLGRA(Widget, w)
{
Display *display = XtDisplay(w);
int i;
char T[12] ;

  char **labelPointers[14] = {
	&dismissAllButton.label,
      	&quitButton.label,
      	&moveButton.label,
      	&backButton.label,
      	&fullButton.label,
	&openButton.label, 
	&dismissThisButton.label,
      	&closeButton.label,
      	&ownerButton.label,
	&cancelButton.label,
      	&resizeButton.label, 
	&dismissButton.label,
      	&restoreButton.label,
      	&refreshButton.label,
  };

  int *mnemonicPointers[14] = {
	(int *)&dismissAllButton.mnemonic,
      	(int *)&quitButton.mnemonic ,
      	(int *)&moveButton.mnemonic,
      	(int *)&backButton.mnemonic,
      	(int *)&fullButton.mnemonic,
	(int *)&openButton.mnemonic, 
	(int *)&dismissThisButton.mnemonic,
      	(int *)&closeButton.mnemonic,
      	(int *)&ownerButton.mnemonic,
	(int *)&cancelButton.mnemonic,
      	(int *)&resizeButton.mnemonic, 
	(int *)&dismissButton.mnemonic,
      	(int *)&restoreButton.mnemonic,
      	(int *)&refreshButton.mnemonic,
  };
/* shortest to longest */

	for (i=0; i<14; i++) {
		sprintf(T,"%s\0",addons[i]);
        	*labelPointers[i] =
			OlGetMessage(display, NULL,  0,
                        	OleNmenuLabel, T,
                        	OleCOlClientOlwmMsgs,
				olwmMenuLabels[i],
                        	(XrmDatabase)NULL);
        	*mnemonicPointers[i] =
			(int)*(OlGetMessage(display, NULL,  0,
                        	OleNmnemonic, T,
                        	OleCOlClientOlwmMsgs,
				menuMnemonics[i],
                        	(XrmDatabase)NULL));
	} /* for */
} /* FillMenuLabels */

/* full is no longer used in Motif mode */

static char *motaddons[8] = {
"quit",
"move",
"back",
"full",
"minimize",
"resize",
"motifrestore",
"maximize",
};


/*
 * FillMotifMenuLabels - fill in labels and mnemonics.
 */

static void
FillMotifMenuLabels OLARGLIST(w)
OLGRA(Widget, w)
{
Display *display = XtDisplay(w);
int i;
char T[15] ;

  char **labelPointers[8] = {
      	&quitButton.label,
      	&moveButton.label,
      	&backButton.label,
      	&fullButton.label,
      	&motifMinimizeButton.label,
      	&resizeButton.label, 
      	&motifRestoreButton.label,
      	&motifMaximizeButton.label,
  };

  int *mnemonicPointers[8] = {
      	(int *)&quitButton.mnemonic ,
      	(int *)&moveButton.mnemonic,
      	(int *)&backButton.mnemonic,
      	(int *)&fullButton.mnemonic,
      	(int *)&motifMinimizeButton.mnemonic,
      	(int *)&resizeButton.mnemonic, 
      	(int *)&motifRestoreButton.mnemonic,
      	(int *)&motifMaximizeButton.mnemonic,
  };

	for (i=0; i < 8; i++) {
		sprintf(T,"%s\0",motaddons[i]);
        	*labelPointers[i] =
			OlGetMessage(display, NULL,  0,
                        	OleNmenuLabel, T,
                        	OleCOlClientOlwmMsgs,
				motMenuLabels[i], 
                        	(XrmDatabase)NULL);
        	*mnemonicPointers[i] =
			(int)*(OlGetMessage(display, NULL,  0,
                        	OleNmnemonic, T,
                        	OleCOlClientOlwmMsgs,
				motMenuMnemonics[i],
                        	(XrmDatabase)NULL));
	} /* for */

} /* FillMotifMenuLabels */


#if !defined(HELP_MESSAGE)
#define HELP_MESSAGE(type, msg) \
			OlGetMessage(display, NULL, 0, OleNtitle, \
				type, OleCOlClientOlwmMsgs, msg, \
				NULL);
#endif


extern void
FillHelpTitles OLARGLIST(w)
OLGRA(Widget, w)
{
Display *display = XtDisplay(w);
int i;
char T[15], M[23], N[23];

  char **labelPointers[14] = {
	&dismissAllHelp.helptitle,
      	&quitHelp.helptitle,
      	&moveHelp.helptitle,
      	&backHelp.helptitle,
      	&fullHelp.helptitle,
	&openHelp.helptitle, 
	&dismissThisHelp.helptitle,
      	&closeHelp.helptitle,
      	&ownerHelp.helptitle,
	&cancelHelp.helptitle,
      	&resizeHelp.helptitle, 
	&dismissHelp.helptitle,
      	&restoreHelp.helptitle,
      	&refreshHelp.helptitle,
  };

  /* for motif mode, do only these */

  char **motlabelPointers[8] = {
	&quitHelp.helptitle,
      	&moveHelp.helptitle,
      	&backHelp.helptitle,
      	&fullHelp.helptitle,
      	&motminHelp.helptitle,
      	&resizeHelp.helptitle,
      	&motifRestoreHelp.helptitle,
      	&motmaxHelp.helptitle,
  };

   if (currentGUI == OL_OPENLOOK_GUI) {
	for (i=0; i<14; i++) {
		sprintf(T,"%s\0",addons[i]);
        	*labelPointers[i] =
			OlGetMessage(display, NULL,  0,
                        	OleNtitle, T,
                        	OleCOlClientOlwmMsgs, helpTitles[i],
                        	(XrmDatabase)NULL);
	}
    }
    else {
	/* Motif - actually only 7 different buttons - will remove the
	 * extra "Full" later.
	 */
	for (i=0; i< 7; i++) {
		sprintf(T,"%s\0",motaddons[i]);
        	*motlabelPointers[i] =
			OlGetMessage(display, NULL,  0,
                        	OleNtitle, T,
                        	OleCOlClientOlwmMsgs, mothelpTitles[i],
                        	(XrmDatabase)NULL);
	}
    }
	called_filled_help++;
}

/* These three are for debugging purpose, they are for the event handlers for
   double click ... */
#undef DBG_MBCLICK2EH		/* MBClick2EH */
#undef DBG_ICONCLICKEH		/* IconClickEH */
#undef DBG_CBE			/* ConsumeExcessiveEvents */

#define EM	(ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define RM_EH	XtRemoveRawEventHandler(w, EM, False, MBClick2EH, NULL)
#define RM_IEH	XtRemoveRawEventHandler(w, EM, False, IconClickEH, NULL)

	/* The following variables, ConsumeExcessiveEvents,
	 * PopdownMenuCB, MBClick2EH, and IconClickEH are
	 * for MOTIF mode only...
	 *
	 * PopdownMenuCB checks "should_rm_eh" to determine
	 * whether it's necessary to remove EH. By default,
	 * it's True but can be overrided by EHs when
	 * "popdown" is performed there...
	 *
	 * PopdownMenuCB checks "icon_br1.type" to determine
	 * which EH (MBClick2EH or IconClickEH) should be
	 * removed. Menu() will initialize "icon_br1" properly.
	 * Basically, if type == ButtonPress then it's a window
	 * menu, if type == ButtonRelease then it's an icon menu.
	 *
	 * "wm_for_icon" is used by IconClickEH because there is
	 * no way to get "wm" from an icon_widget id (I think).
	 */
static Boolean		should_rm_eh = True;
static XButtonEvent	icon_br1 = { 0 };
static Widget		wm_for_icon = NULL;

extern void
ConsumeExcessiveEvents OLARGLIST((w, mask))
	OLARG( Widget,	w)
	OLGRA( long,	mask)
{
	Display *	dpy = XtDisplay(w);
	Window		window = XtWindow(w);
	XEvent		ignore;

		/* flush the event queue first */
	XSync(dpy, False);

		/* peel off EM events manually and consume them right away */
	while (XCheckWindowEvent(dpy, window, mask, &ignore) == True)
	{
#ifdef DBG_CBE
 printf("\t\tConsuming %d\n", ignore.type);
#endif
		;
	}
} /* end of ConsumeExcessiveEvents */

static void
PopdownMenuCB OLARGLIST((w))
	OLGRA( Widget,		w)
{
	if (should_rm_eh)
	{
#ifdef DBG_MBCLICK2EH
 printf("\t\tmenu is going down thru ?Keyboard?, RM_[I]EH, %s\n", XtName(w));
#endif
		(icon_br1.type == ButtonPress) ? RM_EH : RM_IEH;
	}
	else
	{
#ifdef DBG_MBCLICK2EH
 printf("\t\tshould_rm_eh is False...\n");
#endif
		should_rm_eh = True;
		ConsumeExcessiveEvents(w, EM);
	}
} /* end of PopdownMenuCB */

/*
 * MBClick2EH - This EH is used to handle "wMenuButtonClick2" resource.
 *
 */
static void
MBClick2EH OLARGLIST((w, client_data, ev, cont_to_dispatch))
	OLARG( Widget,		w)	/* wm */
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	ev)
	OLGRA( Boolean *,	cont_to_dispatch)
{
#define A	((XMotionEvent *)&prev)
#define B	((XMotionEvent *)ev)
#define MDF	(int)attrs->mouse_damping_factor
#define MCT	(int)attrs->multi_click_timeout
#define SAME_PT	( -MDF < A->x_root-B->x_root && A->x_root-B->x_root < MDF && \
		  -MDF < A->y_root-B->y_root && A->y_root-B->y_root < MDF )

	static XButtonEvent	prev;	/* 1st button release */
	Boolean			stay_up = False;
	Widget			widget = XtWindowToWidget(
						ev->xany.display,
						ev->xany.window);
	_OlAppAttributes *	attrs;

#ifdef DBG_MBCLICK2EH
 if (ev->type != MotionNotify)
 printf("\t(%s, %x) ev->type : %d\n", XtName(w), w, ev->type);
#endif
	*cont_to_dispatch = False;

	if (w != widget)
	{
		Widget shell = _OlGetShellOfWidget(widget);

#ifdef DBG_MBCLICK2EH
 printf("w != widget => %s, shell: %s\n", XtName(widget), XtName(shell));
#endif
		if ( ev->type == ButtonPress && !_OlIsInMenuStack(shell) )
		{
#ifdef DBG_MBCLICK2EH
 printf("\t??RESET STAYUP mode...\n");
#endif
			_OlResetStayupMode(shell);
		}
		else if ( ev->type == ButtonRelease &&
			  !_OlIsEmptyMenuStack(w) && _OlIsNotInStayupMode(w) )
		{
#ifdef DBG_MBCLICK2EH
 printf("\t??POPDN and RM_EH...\n");
#endif
			should_rm_eh = False;
			_OlPopdownCascade(_OlRootOfMenuStack(w), False);
			RM_EH;
		}
		return;
	}

	switch (ev->type)
	{
		case ButtonRelease:
			if ( _OlIsEmptyMenuStack(w) ||
			     (stay_up = _OlIsInStayupMode(w)) )
			{
#ifdef DBG_MBCLICK2EH
 printf("\t\tBR: 2nd click???....\n");
#endif
				if (stay_up)
				{
#ifdef DBG_MBCLICK2EH
 printf("\t\t\tBR: MAY BE fast finger causes MENU consumes a BP(s)???\n");
#endif
					should_rm_eh = False;
					_OlPopdownCascade(
						_OlRootOfMenuStack(w), False);
				}

				if ( ev->xbutton.root == prev.root )
				{
					attrs = _OlGetAppAttributesRef(w);
					if (ev->xbutton.time - prev.time < MCT
					    && SAME_PT)
					{
#ifdef DBG_MBCLICK2EH
 printf("\t\t\tBR: double click, call MenuQuit\n");
#endif
					MenuQuit(w, (XtPointer)&w, NULL);
					}
				}
#ifdef DBG_MBCLICK2EH
 printf("\t\t\tBR: job is done after 2nd release, RM_EH???....\n");
#endif
				RM_EH;
			}
			else
			{
#ifdef DBG_MBCLICK2EH
 printf("\t\t\tBR: 1st click???....\n");
#endif
				prev = ev->xbutton;

				if (_OlIsPendingStayupMode(w))
				{
					_OlQueryResetStayupMode(
						w, XtWindow(w),
						ev->xbutton.x, ev->xbutton.y);
				}
#ifdef DBG_MBCLICK2EH
 printf("\t\t\tBR: is stayup?....: %d\n", _OlGetStayupMode(w));
#endif
				if (_OlIsNotInStayupMode(w))
				{
#ifdef DBG_MBCLICK2EH
 printf("\t\t\t\tBR: no, popdown and RM_EH...\n");
#endif
					should_rm_eh = False;
					_OlPopdownCascade(
						_OlRootOfMenuStack(w), False);

					RM_EH;
				}
				else
				{
#ifdef DBG_MBCLICK2EH
 printf("\t\t\t\tBR: yes, stayup...\n");
#endif
					_OlSetStayupMode(w);
				}
			}
			break;
		case MotionNotify:
			if (!_OlIsEmptyMenuStack(w))
			{
				_OlQueryResetStayupMode(
					w, XtWindow(w),
					ev->xmotion.x, ev->xmotion.y);
			}
#if 0
			else
			{
				attrs = _OlGetAppAttributesRef(w);

				if (!SAME_PT)
				{
#ifdef DBG_MBCLICK2EH
 printf("\t\tMOTION: RM_EH, job done\n");
#endif
					RM_EH;
				}
			}
#endif
			break;
	}

#undef RM_EH
#undef A
#undef B
#undef MDF
#undef DCT
#undef SAME_PT
} /* end of MBClick2EH  */

/*
 * IconClickEH - This EH is used to handle "iconClick" resource.
 *
 */
static void
IconClickEH OLARGLIST((w, client_data, ev, cont_to_dispatch))
	OLARG( Widget,		w)	/* wm */
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	ev)
	OLGRA( Boolean *,	cont_to_dispatch)
{
#define A	((XMotionEvent *)&icon_br1)
#define B	((XMotionEvent *)ev)
#define MDF	(int)attrs->mouse_damping_factor
#define MCT	(int)attrs->multi_click_timeout
#define SAME_PT	( -MDF < A->x_root-B->x_root && A->x_root-B->x_root < MDF && \
		  -MDF < A->y_root-B->y_root && A->y_root-B->y_root < MDF )

	_OlAppAttributes *	attrs;

	Widget			widget = XtWindowToWidget(
						ev->xany.display,
						ev->xany.window);

#ifdef DBG_ICONCLICKEH
 if (ev->type != MotionNotify)
 printf("\t(%s, %x) ev->type : %d\n", XtName(w), w, ev->type);
#endif
	*cont_to_dispatch = False;

	if (w != widget)
	{
		Widget shell = _OlGetShellOfWidget(widget);

#ifdef DBG_ICONCLICKEH
 printf("w != widget => %s, shell: %s\n", XtName(widget), XtName(shell));
#endif
		if ( ev->type == ButtonPress && !_OlIsInMenuStack(shell) )
		{
#ifdef DBG_ICONCLICKEH
 printf("\t??RESET STAYUP mode...\n");
#endif
			_OlResetStayupMode(shell);
		}
		else if ( ev->type == ButtonRelease &&
			  !_OlIsEmptyMenuStack(w) && _OlIsNotInStayupMode(w) )
		{
#ifdef DBG_ICONCLICKEH
 printf("\t??POPDN and RM_IEH...\n");
#endif
			should_rm_eh = False;
			_OlPopdownCascade(_OlRootOfMenuStack(w), False);
			RM_IEH;
		}
		return;
	}

	switch (ev->type)
	{
		case ButtonRelease:
#ifdef DBG_ICONCLICKEH
 printf("\t\tmust be 2nd click???\n");
#endif

			if ( !_OlIsEmptyMenuStack(w) )
			{
#ifdef DBG_ICONCLICKEH
 printf("\t\t\tBR, menu is still in stack, pop it down first...\n");
#endif
				should_rm_eh = False;
				_OlPopdownCascade(_OlRootOfMenuStack(w), False);
			}

			if ( ev->xbutton.root == icon_br1.root )
			{
				attrs = _OlGetAppAttributesRef(w);
				if (ev->xbutton.time - icon_br1.time < MCT &&
				    SAME_PT)
				{
#ifdef DBG_ICONCLICKEH
 printf("\t\t\tBR, double click, OpenClose???\n");
#endif
					MenuOpenClose(
						wm_for_icon,
						(XtPointer)&wm_for_icon, NULL);
					wm_for_icon = NULL;
				}
			}
#ifdef DBG_ICONCLICKEH
 printf("\t\t\tBR, RM_IEH\n");
#endif
			RM_IEH;
			break;
		case MotionNotify:
			if (!_OlIsEmptyMenuStack(w))
                        {
                                _OlQueryResetStayupMode(
                                        w, XtWindow(w),
                                        ev->xmotion.x, ev->xmotion.y);
                        }
			break;
	}

#undef RM_IEH
#undef A
#undef B
#undef MDF
#undef DCT
#undef SAME_PT
} /* end of IconClickEH  */

/*
 * Menu
 * Pop up window menu.
 * If base window , piece == WM_MM. 
 */

extern void
Menu OLARGLIST((wm, event, piece))
OLARG(WMStepWidget,	wm)
OLARG(XEvent *,		event)
OLGRA(WMPiece,		piece)
{
    int		i;
    int		cascades = False;
    unsigned int	j;
    Window		junk;
    int		ijunk;
    unsigned int uijunk;
    int		x, y, init_x, init_y;
    WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
    Widget shell_to_pop;

    Dimension	rect_w = 1, rect_h = 1;
    OlPopupMenuCallbackProc	popdn_menu_cb = NULL;
    Widget	menu_owner = (Widget)wm;


    wm_for_icon = NULL;

	if (!HasMenu(wm)) {
		/* No menu, ring bell and return */
		XBell(XtDisplay(wm), 0);

		/*
		* could grab the pointer (QuestionMark) until release
		* OlGrabDragPointer(,OlGetQuestionCursor(screen));
		* OlDragAndDrop()
		* OlUngrabPointer();
		*/
		return;
	}

	/* Turn on the menu mark */
	if (piece == WM_MM) {
		if (currentGUI == OL_OPENLOOK_GUI)
			mmstate = FALSE;
		else
			wmstep->menu_functions &= ~ WMMenuButtonState;
		FlipMenuMark(wm);
	}

	GetMenuItems(wm);

	/* Set wm field of WMMenuInfo struct for button callbacks;  client
	 * data field was set to a POINTER to this field (not the actual
	 * WMStepWidget, so the client_data field must be de-referenced once
	 * in the callback to get the actual widget.
	 */ 
	WMCombinedMenu->w = (Widget)wm;

	if (event->type == ButtonPress || event->type == ButtonRelease)
	{
		x = event->xbutton.x_root;
		y = event->xbutton.y_root;
	}
	else
	{
		(void)XQueryPointer(
			XtDisplay(wm), RootWindowOfScreen(XtScreen(wm)),
			&junk, &junk, &x, &y, &ijunk, &ijunk, &uijunk);
	}

	init_x = x;
	init_y = y;

	if (currentGUI == OL_MOTIF_GUI /*&& event->type != ButtonPress*/) {
		int	menuwidth,
			menuheight;
		int	screenwidth = WidthOfScreen(XtScreen((Widget)wm));
		int	screenheight = HeightOfScreen(XtScreen((Widget)wm));

		menuwidth = WMCombinedMenu->MenuShell->core.width;
		menuheight = WMCombinedMenu->MenuShell->core.height;

			/* This can happen if menu is not realized
			 * at the time, just HACK for now...
			 */
		if (menuwidth == 0 || menuheight == 0)
		{
			menuwidth = 117;
			menuheight = 157;
		}

		if (IsIconic(wm)) {
			int	iconx = wm->wmstep.icon_widget->core.x,
				icony = wm->wmstep.icon_widget->core.y;
			x = iconx;
			y = icony - menuheight;
			if ((int)(x + menuwidth) > WidthOfScreen(
					XtScreen((Widget)wm)) )
				x = iconx + icon_width - menuwidth;
			/* now do y */
			if (y < 0)
				y = icony + icon_height;

				/* The click may come from either
				 * wm->wmstep.icon_widget or
				 * motifAIW (the label when this
				 * icon has "focus".
				 */
			menu_owner = XtWindowToWidget(
					XtDisplay((Widget)wm),
					event->xany.window);
		}
		else if (piece == WM_MM) {
			/* Not iconic, position menu */
			int vwidth, hwidth;
			if (Resizable(wm)) {
			  vwidth = wmstep->metrics->motifVResizeBorderWidth;
			  hwidth = wmstep->metrics->motifHResizeBorderWidth;
			}
			else {
				if (wmstep->decorations & WMBorder) {
		  		vwidth =
				  wmstep->metrics->motifVNResizeBorderWidth;
		  		hwidth =
				  wmstep->metrics->motifHNResizeBorderWidth;
				}
				else /* No border */
					vwidth = hwidth = 1;
			} /* Not resizable */
			x = wm->core.x + hwidth;
			y = wm->core.y + vwidth;
			rect_w = wmstep->metrics->motifButtonWidth;
			rect_h = wmstep->metrics->motifButtonHeight;
			if (x + menuwidth > screenwidth)
				x = screenwidth - menuwidth;
			if (x < 0)
				x = 0;
			if (y + menuheight >  screenheight)
				y = wm->core.y - menuheight;
			if (y < 0)
				y = 0;
		} /* else !iconic */
	}
	/* What Menu shell?
	/* CurrentMenuShellPosted = the "top" menushell of a potential cascade -
	 * all that counts is that we know this top menushell is posted for the
	 * CurrentMenushellPosted variable.
	 */
	if (NumMenushellsPosted && menushells_posted[0])
		/*shell_to_pop = CurrentMenushellPosted;*/
		shell_to_pop = menushells_posted[0];
	else {
		if (currentGUI == OL_MOTIF_GUI) {
			shell_to_pop = WMCombinedMenu->MenuShell;
			if(wmstep->csinfo && wmstep->csinfo->windowMenu) {
				Global_Menu_Info *gmi_ptr = global_menu_info;
				while (gmi_ptr)
		   		if (!strcmp( gmi_ptr->menuname,
						wmstep->csinfo->windowMenu))
					break;
		   		else
					gmi_ptr = gmi_ptr->next;
				if (gmi_ptr)
					shell_to_pop = gmi_ptr->MenuShell;
			}
	
		}
		else {
			/* Open Look */
			if (wmstep->decorations & WMHasFullMenu)
				shell_to_pop = WMCombinedMenu->MenuShell;
			else
				/* I presume that if you got this far, this
				 * window must have SOME menu
				 */
				shell_to_pop = WMCombinedMenu->CascadeShell;
		} /* Open Look */
		/*CurrentMenushellPosted = shell_to_pop;*/
		menushells_posted[NumMenushellsPosted++] = shell_to_pop;
	} /* else CurrentMenushellPosted == NULL */

	if ( currentGUI == OL_MOTIF_GUI && event->type != KeyPress )
	{
		XtEventHandler	EH = NULL;

		if (event->type == ButtonPress)	/* Handle Window Menu */
		{
			if ( piece == WM_MM &&
			     (wmstep->menu_functions & WMFuncClose) &&
			     motwmrcs->wMenuButtonClick2 )
			{
				EH = MBClick2EH;
				icon_br1 = event->xbutton;
			}
		}
		else				/* Handle Icon Menu */
		{
			if (motwmrcs->iconClick)
			{
				EH = IconClickEH;
				icon_br1 = event->xbutton;
				wm_for_icon = (Widget)wm;
			}
		}
		
		if (EH)
		{
			popdn_menu_cb = PopdownMenuCB;
			XtInsertRawEventHandler(
				menu_owner, EM, False, EH, NULL, XtListHead);
#undef EM

				/* Make sure "menu_owner/wm/icon_w" is in Xt
				 * grab list so that both menu and myself
				 * can see the wanted events.
				 */
			_OlMenuLock(menu_owner, menu_owner, NULL);

#ifdef DBG_MBCLICK2EH
 printf("Inserted EH to handle double clicks\n");
#endif
		}
	}

	WmPostPopupWindowMenu(
		menu_owner,
		shell_to_pop,
		event->type == ButtonPress ? OL_MENU : OL_MENUKEY,
		popdn_menu_cb,
		(Position)x, (Position)y,	    /* root_x, root_y	*/
		(Position)init_x, (Position)init_y, /* init_x, init_y	*/
		rect_w, rect_h
	);

} /* end of Menu */

/*
 * MenuChooseDefault
 * The menu for a given window has never been brought up, so the default
 * callback procedure has not yet been set up.  Determine the correct
 * procedure, and yield to it.
 */

extern void
MenuChooseDefault OLARGLIST((w, client_data, call_data))
    OLARG (Widget, w)
    OLARG (XtPointer, client_data)
    OLGRA (XtPointer, call_data)
{
    WMStepWidget wm = *((WMStepWidget *)client_data);

	/* If we choose not to implement the selectDoesPreview in
	 * Motif mode, then this needs no changes; otherwise,
	 * these defaults will not fly all right.  In motif mode,
	 * I would have to check the first reasonable button
	 * for the default on the window menu.  It wouldn't
	 * necessarily be the first (Restore) unless the window
	 * was in iconic or full mode.
	 */
    wm->wmstep.default_cb = (wm->wmstep.decorations & WMHasFullMenu) ?
	MenuOpenClose : MenuDismiss;
    (*wm->wmstep.default_cb) (w, client_data, call_data);
}


/* MenuPopupCB - pops when main menu does - draw separator.
 */

/* In a "title" button, there will be SEPARATOR_BORDER_OFFSET
 * pixels to the double lines, then another TITLE_OFFSET pixels to the
 * title.  Total = Title height + 2 * SEP_B_O + 6 (for the lines) + 2 *
 * TITLE_OFFSET
 */ 
#define SEPARATOR_BORDER_OFFSET	2
#define DIVIDER_OFFSET 3
#define TITLE_OFFSET 4

static void
MenuExpose OLARGLIST((w, client_data, event, cont_to_dispatch))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, call_data)
OLGRA(Boolean *, cont_to_dispatch)
{
Screen	*screen = XtScreenOfObject(w);
Display *display = XtDisplay(w);
WMStepWidget wm;
WMStepPart *wmstep;
Widget separator;
Widget menushell = (Widget)*((Widget *)client_data);
Global_Menu_Info *gmi_ptr;
int i;
XGCValues xgcvalues;
GC	gc;
	
	/* Draw the separator(s) */
	/* w is the menu shell */
	if (menushell == WMCombinedMenu->MenuShell) {
		separator = WMCombinedMenu->separator;
		wm = (WMStepWidget)WMCombinedMenu->w;
		for (i=0; i < num_wmstep_kids; i++){
			if (wm == (WMStepWidget)wmstep_kids[i])
				break;
		}
		if (i == num_wmstep_kids)
			return;
		
		wmstep = (WMStepPart *)&(wm->wmstep);
		OlgDrawLine(screen, XtWindowOfObject(separator),
			wmstep->mdm, SEPARATOR_BORDER_OFFSET,
			separator->core.y + 1,
			w->core.width - 2 * SEPARATOR_BORDER_OFFSET, 2, 0);
	}
	else {
		MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[MENU_COMP]);
		MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
		XSegment	segs[4];
		WMMenuButtonData *mbd;
		OlgAttrs	*motlabel_mdm = (OlgAttrs *)NULL;
		gmi_ptr = global_menu_info;
		while (gmi_ptr) {
			if (gmi_ptr->MenuShell == menushell) {
				/* found it - gather all separators */
				wm = (WMStepWidget)gmi_ptr->w;
				wmstep = (WMStepPart *)&(wm->wmstep);
				for (i=0; i < gmi_ptr->num_wmap; i++) {
					if (gmi_ptr->wmap[i].type ==
					    GMI_SEPARATOR) {
		separator = gmi_ptr->wmap[i].widget;
						OlgDrawLine(screen,
						   XtWindowOfObject(separator),
						   wmstep->mdm,
						   SEPARATOR_BORDER_OFFSET,
						   separator->core.y + 1,
						   w->core.width -
					     2 * SEPARATOR_BORDER_OFFSET,
						   2, 0);
					} /* if separator */
					else
					  if (gmi_ptr->wmap[i].type ==
						GMI_TITLE) {
				int	label_area_wid,
					label_area_ht;
				OlgTextLbl labeldata;
						/* Draw the title */
						/* Top 2, bottom 2 lines */
				separator = gmi_ptr->wmap[i].widget;
				xgcvalues.foreground = mcai->activeForeground;
				gc = XtGetGC(w, GCForeground, &xgcvalues);

					segs[0].x1 = separator->core.x + 1;
					segs[0].y1 = separator->core.y + 1;
					segs[0].x2 = separator->core.x + 
			separator->core.width - 1;
					segs[0].y2 = separator->core.y + 1;

					segs[1].x1 = separator->core.x + 1;
					segs[1].y1 = separator->core.y + 3;
					segs[1].x2 = separator->core.x +
			separator->core.width - 1;
					segs[1].y2 = separator->core.y + 3;

					segs[2].x1 = separator->core.x + 1;
					segs[2].x2 = separator->core.x +
			separator->core.width - 1;
					segs[2].y1 = separator->core.y +
				separator->core.height - 1;
					segs[2].y2 = separator->core.y +
				separator->core.height - 1;

					segs[3].x1 = separator->core.x + 1;
					segs[3].x2 = separator->core.x +
			separator->core.width - 1;
					segs[3].y1 = separator->core.y +
				separator->core.height - 3;
					segs[3].y2 = separator->core.y +
				separator->core.height - 3;
					XDrawSegments(display,
						   XtWindowOfObject(separator),
						   gc,
						   segs, 4);
					/*Draw Window title here, then
					 * release GC
					 */
			mbd = gmi_ptr->wmap[i].mbd;
			label_area_ht = separator->core.height - 6;
			label_area_wid = separator->core.width - 2;

			labeldata.justification = TL_CENTER_JUSTIFY;

	motlabel_mdm = OlgCreateAttrs(screen, mcai->activeForeground,
				(OlgBG *)&(mcai->activeForeground),
			(Boolean)FALSE, (Dimension)12);
	labeldata.normalGC = OlgGetFgGC(motlabel_mdm);
	labeldata.font = _OlGetDefaultFont(w, "OlDefaultBoldFont");
	if (labeldata.font) /* SHOULDN'T BE NON_NULL */
		XSetFont(display, labeldata.normalGC,
				labeldata.font->fid);
	labeldata.inverseGC = labeldata.normalGC;
	if (mcai->fontList == NULL)
		labeldata.font_list = ol_font_list;
	else
		labeldata.font_list = mcai->fontList;
   labeldata.accelerator = NULL;
   labeldata.mnemonic = NULL;
   labeldata.flags = (unsigned char) NULL;
   labeldata.justification = TL_CENTER_JUSTIFY;

  labeldata.label = mbd->label;

			OlgDrawTextLabel(screen, XtWindowOfObject(separator),
				motlabel_mdm, separator->core.x + 1,
				separator->core.y + 6,
		label_area_wid, label_area_ht, &labeldata);
		if (motlabel_mdm)
			OlgDestroyAttrs(motlabel_mdm);
		if (gc)
		{
			XtReleaseGC(w, gc);
			gc = NULL;
		}
					  }
				} /* for */
			} /* if MenuShell == menushell */
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* else */
	XFlush(XtDisplay((Widget)wm));
} /* MenuExpose */




extern void
MenuMotifMinimize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
	/* Quick */
	MenuOpenClose(w, client_data, call_data);
} /* MenuMotifMinimize */

extern void
MenuMotifMaximize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
	/* Quick */
	MenuFullRestore(w, client_data, call_data);
} /* MenuMotifMinimize */

extern void
MenuMotifRestore OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
	WMStepWidget wm     = (WMStepWidget) *((WMStepWidget *)client_data);
	WMStepPart * wmstep = &wm-> wmstep;

	/* Are we restoring from full size or iconic size?? */
	switch(wmstep-> size) {
		default:
			break;
		case WMICONICNORMAL:
			MenuOpenClose(w, client_data, call_data);
			break;
		case WMFULLSIZE:
		case WMICONICFULL:
			MenuFullRestore(w, client_data, call_data);
			break;
	} /* switch size */
} /* MenuMotifRestore */

/*
 * MenuOpenClose
 * - Open (Close) button on the window menu.
 * Also called as function from NonMaskable() to handle
 * XIconifyWindow() client message.
 * Looks like client_data is the name of the WMStepWidget for which the
 * window menu was used.
 * Again, assume client_data is the WMStepWidget for which the window
 * menu is being called up for.
 *
 * wmstep->icon : contains the position of the clients icon - use
 * for lifetime of client, or until we allow client to dynamically
 * change it.
 * wmstep->prev : contains the position of the client prior to it
 * being iconized.
 */

extern void
MenuOpenClose OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm     = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart * wmstep = &wm-> wmstep;
Widget       parent = XtParent(wm);
Widget       child  = wmstep-> child;
Display		*display = XtDisplay(w);
WMGeometry * p;
WMGeometry   n;
int	k;

FPRINTF((stderr, "Open or Close %x\n", (Window) client_data));

/* get together all the windows in wm's group (TRUE argument forces this).
 * group will be in window_group list.
 * A new twist: Suppose you have an application with multiple base windows.
 * Now, this base window (or at least we think it is) may have it's
 * own group, or it may have a window group that is another window,
 * also a base window.  The ICCCM rule says that the window group is
 * the window that has the WM_COMMAND property on it.  We just made a RULE:
 *****RULE ****
 *     A base window can't be iconized unless the window whose window group
 * it is gets iconized, too.  UNLESS, of course, that window_group window is
 * an undecorated window that we don't know about!
 * Why?  Suppose we had an application with 2 base windows, A and B.  The
 * first window, A, had a window group == A (itself).  The second window,
 * B, had a window_group == A.  Iconize B first => unmap B, make an icon for
 * it.  Iconize A second => unmap A AND B (although it is already unmapped)
 * and make an icon for it => 2 icons.  Now bring up (open) A first.  A
 * will result in both windows A and B mapping, and icon A disappearing.
 * However, icon B is still around.  Open B, and you wind up with an
 * extra decoration set.  Bad news.
 */ 
if (wmstep->xwmhints->window_group == wmstep->window)
	/* With NoAppend arg, tells ConstructGroupList to get me all
	 * windows in the same group as wmstep->window BELOW this
	 * window (e.g., that have this window, or a child of this window,
	 * as a group leader - of course the child would have THIS window
	 * as a group leader.
	 */
	ConstructGroupList(wm, NoAppend, TRUE);
else {
	/* window group isn't this window; who is it?? */
	int k = IsWMStepChild(wmstep->xwmhints->window_group);
	if (k < 0)
		/* This window is (probably) a base window, and has a window
		 * group that is an unmapped (undecorated) window.  In that
		 * case, it's O.K. to iconize it alone, and there's no
		 * danger of the above scenario occurring.
		 * When opening the window, the inverse will take place -
		 * it'll find all windows below it in the hierarchy.
		 */
		ConstructGroupList(wm, NoAppend, TRUE);
	else
		/* The window_group window is another decorated window;
		 * It is PROBABLY a base window, or HOPEFULLY.
		 * I think we can make that assumption because otherwise,
		 * this window would be telling us that it is capable of
		 * being iconized, implying it is a base window, and it's
		 * group leader is a mapped window that is a popup window.
		 * find ALL members of it's group, and all SUB-members, too.
		 * That means find all the windows that have a window_group
		 * of wmstep->xwnhints.window_group, and all window
		 * that have THEM as  a window group in addition.  For example:
		 *   Window A = group leader (base window) (group == A).
		 *   Window B = base window, group leader == window A.
		 *   Window C = popup window, group leader == window B
		 * When iconizing, All three get unmapped to one icon, A.
		 * When opening, All three get mapped.
		 */
		ConstructGroupList(wm, NestedNoAppendAny, True);
}


switch (wmstep-> size)
   {
   case WMNORMALSIZE:                                /* normal --> iconic */
      /* Save current position, size in wmstep->prev (last function in wm.c) */
      RecordPosition(wm, &(wmstep->prev));

	/* Get icons position on screen (if not previously allocated, do it
	 * now)
	 */
      p = IconPosition(wm, wmstep);
      if (!p) {
	/* No more room */
	XBell(display, 0);
	break;
      }
      /* Now iconize the thing */
      OpenClose(wm, parent, child, WMICONICNORMAL, p);
      break;
   case WMICONICNORMAL:                              /* iconic --> normal */
      RecordPosition(wm, wmstep->icon);
      /* Open it to a real window */
      OpenClose(wm, parent, child, WMNORMALSIZE,  &(wmstep-> prev));
      break;
   case WMFULLSIZE:                                  /* full   --> iconic */
      p = IconPosition(wm, wmstep);
	if (!p) {
	/* No more room */
	XBell(display, 0);
	break;
	}
      OpenClose(wm, parent, child, WMICONICFULL, p);
      break;
   case WMICONICFULL:                                /* iconic --> full   */
/*
      RecordPosition(wm, wmstep-> icon);
      p = &n;
      RecordPosition((WMStepWidget)parent, p);
      OpenClose(wm, parent, child, WMFULLSIZE, p);
 */
	/* Redo this section - call MenuFullRestore() which will call
	 * OpenClose() after it sets the appropriate geometry.
	 */
	wmstep-> size = WMICONICNORMAL;
	MenuFullRestore(w, client_data, (XtPointer) NULL);
	break;
   default:
      break;
   }

} /* end of MenuOpenClose */

/*
 * MenuFullRestore
 * - Callback for 2nd button on full menu.
 * wm = the step widget (decoration frame of client chosen for full/restore)
 * child = the client widget (likely a stub widget)
 * parent = the step widgets parent.
 *
 * - Warning: : If you choose to use client_data and call_data any more, check
 * in MenuOpenClose, case WMICONFULL, first and make sure that its call
 * to this function remains sane.
 */

extern void
MenuFullRestore OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget	wm     = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart	*wmstep = (WMStepPart *)&(wm->wmstep);

Widget		parent = XtParent((Widget)wm);
Widget		child  = XtWindowToWidget(XtDisplay(wm), wm-> wmstep.window);
Display		*display = XtDisplay((Widget)wm);
WMGeometry	*p      = &wm-> wmstep.prev;
WMGeometry	n;
Screen		*screen		= XtScreen(wm);
int		screenwidth	= WidthOfScreen(screen);
int		screenheight	= HeightOfScreen(screen);
int		AvailScreenWidth, AvailScreenHeight;
Boolean		sendconfigure = False;
int		borderx,
		bordery;

FPRINTF((stderr, "Full or Restore %x\n", (Window) client_data));

if (currentGUI == OL_OPENLOOK_GUI) {
	borderx = BorderX(wm);
	bordery = BorderY(wm);
}
else {
	borderx = MBorderX(wm);
	bordery = MBorderY(wm);
}

ResetFocus = False;

if (wmstep->xwmhints->window_group == wmstep->window)
	ConstructGroupList(wm, NoAppend, TRUE);
else {
	int k = IsWMStepChild(wmstep->xwmhints->window_group);
	if (k < 0)
		ConstructGroupList(wm, NoAppend, TRUE);
	else
		ConstructGroupList(wm, NestedNoAppendAny, True);
}

switch(wm-> wmstep.size)
   {
   /* If in iconic state, save current position in wmstep->icon,
    * restore to whatever the dimensions were before it was closed.
    */
   case WMICONICNORMAL:
      /* current state is iconic, previous state is normal;
       * save icon position, set new size, go to full size.
       * Assume wm->wmstep.prev has the position of the window when
       * it was last in WMNORMALSIZE state.
	*****  MOOLIT ******
	* If there is a -motif option (Motif mode), then we use the
	* restore button for daul purposes in Motif mode:
	* if either iconic or full, regardless of the previous state,
	* we go to normal.  Call MenuOpenClose(wm, &wm, NULL).
	* Only the WMICONICNORMAL case is special for this callback
	* when restoring from iconic.  The others, including restoring
	* from FULL, will be taken care of below.
	*/
      RecordPosition(wm, wmstep->icon);
	/* Now configure widget to max. width and height, or max. width and
	 * height of screen.
	 */
	wm->wmstep.size = WMFULLSIZE;
      /*
       * Redo the metrics so the border functions get the proper values
       */
      wm-> wmstep.metrics = GetMetrics(wm);
        /*
         * Now configure widget to available max. width and height,
         * or max. width and height of screen.
         */
      AvailScreenWidth  = screenwidth - (borderx * 2);
      AvailScreenHeight = screenheight -
	(  (currentGUI == OL_OPENLOOK_GUI ? OriginY(wm) : MOriginY(wm))
						+ bordery);

	n.x = n.y = (Position)0;
	if (wm->wmstep.hints & WMNormalSizeHints) {
		n.width = wm->wmstep.xnormalsize.max_width; 
		n.height = wm->wmstep.xnormalsize.max_height; 
	}
	else
		n.width = n.height = 0;
	n.width = (n.width == 0) ? AvailScreenWidth :
					MIN(AvailScreenWidth,n.width);
	n.height = (n.height == 0) ? AvailScreenHeight:
					MIN(AvailScreenHeight,n.height);
	n.width += borderx * 2;
	n.height += (currentGUI == OL_OPENLOOK_GUI ? OriginY(wm) :
		MOriginY(wm)) + bordery;
	
	OpenClose(wm, XtParent(wm), wm->wmstep.child, WMFULLSIZE, &n);
	break;
   case WMICONICFULL:
      RecordPosition(wm, wmstep->icon);
      OpenClose(wm, parent, child, WMNORMALSIZE, p);
	if (currentGUI == OL_OPENLOOK_GUI) {
		XtResizeWidget(wm->wmstep.child, ChildWidth(wm),
					ChildHeight(wm), 0);
	}
	else {
		XtResizeWidget(wm->wmstep.child, MChildWidth(wm),
					MChildHeight(wm), 0);
	}
      break;
   case WMNORMALSIZE:
      /* Currently normal size, make it full */
      wm-> wmstep.size = WMFULLSIZE;
      RecordPosition(wm, p);
      wm-> wmstep.metrics = GetMetrics(wm);
        /*
         * Now configure widget to available max. width and height,
         * or max. width and height of screen.
         */
      AvailScreenWidth  = screenwidth - borderx * 2;
      AvailScreenHeight = screenheight -
		( (currentGUI == OL_OPENLOOK_GUI ? OriginY(wm) : MOriginY(wm))
		 + bordery );

	n.x = n.y = (Position)0;
	if (wm->wmstep.hints & WMNormalSizeHints) {
		n.width = wm->wmstep.xnormalsize.max_width; 
		n.height = wm->wmstep.xnormalsize.max_height; 
	}
	else {
		n.width = n.height = 0;
	}
	n.width = (n.width == 0) ? AvailScreenWidth :
					MIN(AvailScreenWidth, n.width);
	n.height = (n.height == 0) ? AvailScreenHeight:
					MIN(AvailScreenHeight, n.height);
	n.width += borderx * 2;
	n.height += (currentGUI == OL_OPENLOOK_GUI ? OriginY(wm) :
			MOriginY(wm)) + bordery;
      XtConfigureWidget((Widget)wm, n.x, n.y, n.width, n.height, 0);
	if (currentGUI == OL_OPENLOOK_GUI) {
		XtResizeWidget(wm->wmstep.child, ChildWidth(wm),
					ChildHeight(wm), 0);
	}
	else {
		XtResizeWidget(wm->wmstep.child, MChildWidth(wm),
					MChildHeight(wm), 0);
	}
	sendconfigure = True;
      break;
   case WMFULLSIZE:
      /* Currently full size, make it normal size */
      wm-> wmstep.size = WMNORMALSIZE;
      XtConfigureWidget((Widget)wm, p-> x, p-> y, p-> width, p-> height,
							p-> border_width);
	if (currentGUI == OL_OPENLOOK_GUI) {
		XtResizeWidget(wm->wmstep.child, ChildWidth(wm),
					ChildHeight(wm), 0);
	}
	else {
		XtResizeWidget(wm->wmstep.child, MChildWidth(wm),
					MChildHeight(wm), 0);
	}
	sendconfigure = True;
      break;
   default:
      break;
   }
  if (sendconfigure) {
	int	usewidth, useheight, usex, usey;
	if (currentGUI == OL_OPENLOOK_GUI) {
		usewidth = ChildWidth(wm);
		useheight = ChildHeight(wm);
		usex = OriginX(wm);
		usey = OriginY(wm);
	}
	else {
		usewidth = MChildWidth(wm);
		useheight = MChildHeight(wm);
		usex = MOriginX(wm);
		usey = MOriginY(wm);
	}
		SendConfigureNotify(XtDisplay(wm->wmstep.child),
            	  	XtWindow(wm->wmstep.child), wm->core.x + usex,
			wm->core.y + usey, usewidth, useheight,
			0);
  }

} /* end of MenuFullRestore */

/*
 * MenuBack
 * - Lower the window (or group).  Callback on the menu.
 */

extern void
MenuBack OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);

ResetFocus = False;
RaiseLowerGroup(wm, WMLower);

} /* end of MenuBack */

/*
 * MenuRefresh
 * Menu callback - create window size of window to be refreshed,
 * map it, then destroy it; would it be better (less overhead) to
 * keep a spare "RefreshWindow" around, say, off-screen, and when this is
 * called, resize it and move it over the window being refreshed, then
 * move it off?
 */

extern void
MenuRefresh OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget		widget = (WMStepWidget) *((WMStepWidget *)client_data);
Display *       	display = XtDisplay(widget);

Window		refresh_window;
/* the target Widget */
Widget		tW = ( (IsIconic(widget)) ?  (widget->wmstep.icon_widget) :
				(Widget)widget);

XSetWindowAttributes xswa;
XWindowChanges       xwc;
unsigned long        value_mask;

FPRINTF((stderr, "Refresh %x\n", (Window) client_data));

ResetFocus = False;
xswa.background_pixmap = None;
xswa.override_redirect = True;
value_mask = CWBackPixmap | CWOverrideRedirect;

refresh_window = XCreateWindow(display, RootWindowOfScreen(XtScreen(w)), 
   tW-> core.x, tW-> core.y, tW-> core.width, tW-> core.height,
   tW-> core.border_width, 
   CopyFromParent, CopyFromParent, CopyFromParent,
   value_mask, &xswa);

xwc.sibling = XtWindow(tW);
xwc.stack_mode = Above;
value_mask = CWSibling | CWStackMode;

XConfigureWindow(display, refresh_window, value_mask, &xwc);
XMapWindow(display, refresh_window);
XDestroyWindow(display, refresh_window);

} /* end of MenuRefresh */


/*
 * Menu_Move
 *  Menu button for Moving a window
 */
extern void
Menu_Move OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget parent = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart * wmstep  = &parent-> wmstep;
Display *    display = XtDisplay(parent);
Window       window  = parent-> wmstep.window;
Atom         type;

/*
 * The following will be done for us by the toolkit's menu code
 * AFTER this callback returns. We do it just in case user
 * brought us here via keyboard.
 *
 * It is also especially important in this routine, because the window
 * movement can be controlled by the keyboard, and OlMenuUnpost()
 * removes a needed keyboard grab. Calling OlMenuUnpost() now gets all
 * that over with so that we can proceed.
 */
_OlPopdownCascade(_OlRootOfMenuStack((Widget)parent), False);
if ( (currentGUI == OL_OPENLOOK_GUI && HasFullMenu(parent) && mmstate
	&& (NumMenushellsPosted) ) ||
	(currentGUI == OL_MOTIF_GUI && wmstep->decorations & WMMenuButton &&
	wmstep->menu_functions & WMMenuButtonState) )
   FlipMenuMark(parent);

FPRINTF((stderr, "Move %x\n", (Window) client_data));
  /* The third argument tells us if the button is down */
  DragWindowAround(parent,(XEvent *)NULL,0);
}

/*
 * Menu_Resize
 *
 * Menu button callback for resize operation
 * Assume it is only called if the resize corners are on - when
 * creating menu, sets button state (sensitive if has resize
 * corners, insensitive if not).
 */
extern void
Menu_Resize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget parent = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart * wmstep  = &parent-> wmstep;
Display *    display = XtDisplay(parent);
Window       window  = parent-> wmstep.window;
Atom         type;

/*
 * The following will be done for us by the toolkit's menu code
 * AFTER this callback returns. We do it just in case user
 * brought us here via keyboard.
 *
 * It is also especially important in this routine, because the window
 * resize can be controlled by the keyboard, and OlMenuUnpost()
 * removes a needed keyboard grab. Calling OlMenuUnpost() now gets all
 * that over with so that we can proceed.
 */
_OlPopdownCascade(_OlRootOfMenuStack((Widget)parent), False);
if ( (currentGUI == OL_OPENLOOK_GUI && HasFullMenu(parent) && mmstate
	&& (NumMenushellsPosted ) ) ||
	(currentGUI == OL_MOTIF_GUI && wmstep->decorations & WMMenuButton &&
	wmstep->menu_functions & WMMenuButtonState) )
   FlipMenuMark(parent);

FPRINTF((stderr, "Resize %x\n", (Window) client_data));
/* If we get a resize request called from here, pass the step
 * widget, a NULL event, and any WMPiece for the third
 * argument (it will be ignored anyway).
 */
ResizeWindow(parent, (XEvent *)0, WM_SE);
}



/*
 * MenuQuit
 *
 * Select Quit from window menu, or called from NonMaskable() when
 * Quit-ting all windows at end of session (args would be wm, wm, NULL).
 * Client_data is the WMStepWidget.
 */

extern void
MenuQuit OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget parent = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart * wmstep  = &parent-> wmstep;
Display *    display = XtDisplay(parent);
Window       window  = parent-> wmstep.window;
Atom         type;
WMStepWidget temp = parent;
/*Boolean		they_do_it = OlIsWSMRunning(display,XtScreen(parent));*/
Boolean		they_do_it = False;
static char *s = "WM_SAVE_YOURSELF";
static char *d = "WM_DELETE_WINDOW";

FPRINTF((stderr, "Quit %x\n", (Window) client_data));

/*
 * These are not supposed to be exclusive!!!
 */
if (!they_do_it) {
   if (wmstep-> protocols & SaveYourself) {
	/* What if already on pending list?  Don't add again, that's for
	 * sure.  Maybe they don't send the WM_COMMAND, and we're just
	 * waiting for doomsday (or Dallas to win another Super Bowl).
	 * 
	 * Here is a cautious situation: in the case of a client that
	 * has multiple base windows, only the group leader will
	 * be put on the "pending" list.  That is the window that we
	 * will wait for the WM_COMMAND property on.  See
	 * ClientPropertyChange()
	 */
	Window window_group; 
	int k, use_step_index = 0;

	if (wmstep->xwmhints->window_group == window)
		/* The window group window ID is THIS window - easy case */
		window_group = window;
	else {
		/* Find the step widget that we will wait on for WM_COMMAND.
		 * This should only be necessary when an application has
		 * multiple base window.
		 */
		window_group = find_highest_group(parent,(Window)0);
		if ( (k =IsWMStepChild(window_group)) < 0) {
			/* The highest window in the group isn't decorated.
			 * Can't use this one.
			 */
			if (window_group == wmstep->xwmhints->window_group)
				/* Not good - the window group is a
				 * non-decorated window.  Just look for
				 * the WM_COMMAND on this window.  I have
				 * a feeling that this is a popup window
				 * with a non-decorated window_group,
				 * converted to have a full menu with a
				 * quit button - it shouldn't even have
				 * WM_SAVE_YOURSELF!!
				 */
				window_group = window;
			else {
				/* try and find the next highest window in
				 * the group that is decorated, if not the
				 * absolute highest.
				 */
				window_group = find_highest_group(parent,
					wmstep->xwmhints->window_group);
				if ( (k =IsWMStepChild(window_group)) < 0)
					/* same thing - make window_group
					 * the actual window.
					 */
					window_group = window;
				else
					use_step_index++;
			} /* end else (window_group != hints->window_group) */
		} /* end if (k < 0) */
		else
			use_step_index++;
	} /* end else */
	if (use_step_index) {
		/* Use the window_group to get the wmstep widget, and get
		 * the window to use for the WM_COMMAND from that window.
		 * This may be a little redundant, but I want to be sure.
		 */
		temp = (WMStepWidget)wmstep_kids[k];
		window_group = temp->wmstep.window;
	}
	/* All the above checking must be done in ClientPropertyChange()
	 * on the other side, too.
	 */
	if (IsPending(window_group) == -1) {
		/* temp == parent in most cases - when a windows
		 * window_group == the window.
		 */
		AddDeletePendingList((Widget)temp,(Widget)parent, True);
		SendProtocolMessage(display, window,
					XA_WM_SAVE_YOURSELF(display),
							 CurrentTime);
	}
   }
   /* Now, if save yourself is selected, then don't handle delete_window
    * here, wait until we get the WM_COMMAND written to the window that is
    * the window's window_group - handle it
    * in our function that takes care of property changes.
    */
   else { /* save_yourself not selected */
	if (wmstep-> protocols & DeleteWindow) {
		SendProtocolMessage(display, window,
						XA_WM_DELETE_WINDOW(display),
						CurrentTime);
	}
	else {
		XKillClient(display, window);
	}
  }
} /* end if(do_yourself) */
else { /* olwsm is running, yippie-kai-yaah */
   if (wmstep-> protocols & SaveYourself) {
	wsmr.command = NULL;
	EnqueueWSMRequest(display, window, WSM_SAVE_YOURSELF, &wsmr);
   }
   if (wmstep-> protocols & DeleteWindow) {
	wsmr.command = d;
	EnqueueWSMRequest(display, window, WSM_TERMINATE, &wsmr);
   }
} /* end else */

} /* end of MenuQuit */

/*
 * MenuDismissPopups
 *
 */

extern void 
MenuDismissPopups OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm= (WMStepWidget) *((WMStepWidget *)client_data);
int          i;

FPRINTF((stderr, "DismissPopups %x\n", (Window) client_data));

if (FindOwnerAndGroup(wm) == 0)
   {
   Window leader;
   FPRINTF((stderr, "can't find owner\n"));
   /* Maybe this window has a window_group that isn't decorated?? */
   leader = find_highest_group(wm,(Window) 0);
   if ( (i = IsWMStepChild(leader)) >= 0)
	/* Hmmm?  We found a decorated window that is a leader of this
	 * window - but it isn't the immediate leader - how can this be?
	 * Just dismiss the window only.
	 */
   	MenuDismiss(w, (XtPointer)&wm, call_data);
   else {
	/* this window has a leader that isn't decorated - it could be
	 * one of those invisible base windows, like olwsm has.  Gather
	 * the popups, and dismiss them.
	 */
	ConstructGroupList(wm,NoAppendAny,TRUE);
	for (i = 0; i < group_list->used; i++)
		MenuDismiss(w, (XtPointer)&(group_list->p[i]), call_data);
	}
   }
else
   {
   for (i = 1; i < group_list->used; i++)
      MenuDismiss(w, (XtPointer)&(group_list->p[i]), call_data);
   }

} /* end of MenuDismissPopups */

/*
 * MenuDismiss
 *
 */

extern void
MenuDismiss OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget parent = (WMStepWidget) *((WMStepWidget *)client_data);
Display *    display = XtDisplay(parent);
Window       window  = parent-> wmstep.window;

FPRINTF((stderr, "Dismiss %x\n", (Window) client_data));

if (parent == help_parent)
   {
   help_parent-> wmstep.decorations |= WMPinIn;
   help_posted = FALSE;
   XtPopdown(help_shell);
   XtUnmapWidget(help_parent);
   }
else
   {
	SendLongMessage(display, window, XA_WM_DISMISS(display),

						0L, 0L, 0L, 0L, 0L);
	if (parent->wmstep.protocols & DeleteWindow) {
		SendProtocolMessage(display, window,
					XA_WM_DELETE_WINDOW(display),
					CurrentTime);
	}
	else {
		XtUnmapWidget(parent);
	}
   }

} /* end of MenuDismiss */

/*
 * MenuOwner
 *
 */

extern void
MenuOwner OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget parent = (WMStepWidget) *((WMStepWidget *)client_data);

FPRINTF((stderr, "Owner %x\n", client_data));

ResetFocus = False;
if (FindOwnerAndGroup(parent) == 0)
   FPRINTF((stderr, "can't find owner\n"));
else
   {
   WMStepWidget wm = (WMStepWidget)group_list->p[0];
   XRectangle rect;
   RaiseLowerGroup(wm, WMRaise);
   rect = HeaderRect(wm);
   OlClearRectangle(wm, &rect, False);
   DisplayWM(wm, &rect);
   }

} /* end of MenuOwner */



/* CreateGlobalMenus.
 * Creates menu info from resource file.
 * 	Create the menus just read in with ReadResourceFile.
 * Easy: go through linked list pointed to by global_menu_info; create all
 * the necessary flat panes, rectObjs, and the one menushell.  Create
 * an extra pane every time we hit a flat button that IMMEDIATELY follows
 * either a title or a label (separator).
 * Here is what you have to work with:
 *
 * struct _Global_Menu_Info {
 *	String menuname;
 *	WMMenuButtonData *mbd;
 *	Widget MenuShell;
 *	Widget *menu;	
 *	Widget *separators;
 *	int menu_default;
 *	int *num_menu_items;
 *	WidgetMap	wmap;
 *	int num_wmap;
 *	MenuButtonMap mbmap;
 *	int num_mbmap;
 *	struct _Global_Menu_Info *next;
 * };
 *	#define GMI_FLAT	1
 *	#define GMI_SEPARATOR	2
 *	#define GMI_TITLE	3
 */
extern void
CreateGlobalMenus()
{
Global_Menu_Info *gmi_ptr = global_menu_info;
int i, j, k, n;
WMMenuButtonData	*mbd;
int	mbmap_index,
	wmap_index;
int	last_was_rectobj; /* -1 means hasn't been used yet */
int	argcount;
Arg    menuargs[10];
int	flatpane_index,
	last_was_button;
Boolean	HasSeparator	 = False;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[MENU_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);

	/*memset(gmi->buttons_per_pane, 0, MAX_FLAT_PANES * sizeof(short));*/
	while (gmi_ptr) {
	
		/* First create the menu shell */
		gmi_ptr->MenuShell = XtVaCreatePopupShell(gmi_ptr->menuname,
			popupMenuShellWidgetClass, Frame,
			XtNhasTitle, (XtPointer)False,
			XtNpushpin, (XtPointer)OL_NONE,
			(char *)NULL);
		/* If there is a title, then we'll draw it in a rectObj */
		XtAddCallback(gmi_ptr->MenuShell, XtNpopdownCallback,
					SpecialMenuDefault, (XtPointer)
					(gmi_ptr->MenuShell));
		XtRemoveEventHandler(gmi_ptr->MenuShell,
			NoEventMask, True, _OlPopupHelpTree, NULL);
		XtAddEventHandler(gmi_ptr->MenuShell, NoEventMask, True,
					 ClientNonMaskable, NULL);
		
		/* Find out the maximum number of widgets you will need for the
	 	 * menushell (rectobjs or flats) and malloc the map in advance.
		 * Add 1 at the end.  N tells us the maximum number of buttons
		 * (real menu buttons).  Malloc the button map too.
		 */
			mbd = gmi_ptr->mbd;
			if  (mbd == NULL) {
				/* No buttons, just go on */
				gmi_ptr = gmi_ptr->next;
				continue;
			}
		/* New: reset at top of this big loop -
		 * -1 means first time.
		 */
		flatpane_index = last_was_button = -1;
		for (k=0, n = 0; mbd; mbd = mbd->next, k++) {
			if  (mbd == NULL)
				continue;
			if(mbd->menucb == OlwmTitle ||
					mbd->menucb == OlwmSeparator) {
				n++;
				/* Add extra two for titles, for the
				 * double lines around them
				 */
				last_was_button = 0;
			}
			else { /* it is a button */
				if (last_was_button == -1 ||
						last_was_button == 0)
					n++;
				last_was_button = 1;
			}
		} /* for */
		gmi_ptr->num_wmap = n;
		gmi_ptr->num_mbmap = k;

		gmi_ptr->wmap = (WidgetMap *)(XtMalloc(
			gmi_ptr->num_wmap * sizeof(WidgetMap)));

		if (gmi_ptr->wmap == NULL) {
			OlVaDisplayErrorMsg(XtDisplay(Frame), OleNspace,
					OleTexit, OleCOlClientOlwmMsgs,
					OleMspace_exit, NULL);
			return; /* not reached */
		}
		gmi_ptr->mbmap = (MenuButtonMap *) (XtMalloc
			(gmi_ptr->num_mbmap * sizeof(MenuButtonMap)));
		if (gmi_ptr->mbmap == NULL) {
			OlVaDisplayErrorMsg(XtDisplay(Frame), OleNspace,
				OleTexit, OleCOlClientOlwmMsgs,
				OleMspace_exit, NULL);
			return; /* not reached */
		}

		/* Set up the necessary indices that point into each
		 * inportant array.
		 */

		/* mbmap_index - points to the next available position in
		 * the menubutton map array - always increment it after
		 * you use it.
		 */
		mbmap_index = 0;

		/* wmap_index - points to the previous position used in the
		 * widget map array - always increment this before saving
		 * the next widget in the array.
		 */
		wmap_index = -1;

		/* flatpane_index - like mbmap_index - increment it before
		 * using it - index into flatpane array - which flatbutons
		 * widget is a button on?
		 */
		flatpane_index = -1;
		memset(gmi_ptr->buttons_per_pane, 0, MAX_FLAT_PANES * sizeof(short));
				
	mbd = gmi_ptr->mbd;

	    /* first for: set up the menu button and widget maps; second for,
	     * go through the maps and set up the WMMenuDefs.
	     */

	last_was_rectobj = -1;
		/* Now the first for */
	    for (; mbd != NULL;) {
		if (mbd->menucb == OlwmTitle || mbd->menucb == OlwmSeparator){
			long sepwidth = 0, sepheight = 4;

			/* We can create the object, but if it's a title, then
			 * we have to come back to it later to resize the height
			 * for the correct font.  Either way, under all
			 * circumstances, we create an object when we encounter
			 * one of these, ending any previous flat widget that
			 * may bave been started.
			 */

			/* Take care of separator first */
			if (mbd->menucb == OlwmSeparator) {
				wmap_index++;
				gmi_ptr->wmap[wmap_index].widget =
					XtVaCreateManagedWidget(
					"separator", rectObjClass,
					gmi_ptr->MenuShell,
					XtNheight, 4,
					(char *)0);
				gmi_ptr->mbmap[mbmap_index].wmap_index =
							wmap_index;
				gmi_ptr->mbmap[mbmap_index].flatpane_index = -1;
				gmi_ptr->mbmap[mbmap_index].mbd = mbd;
				mbmap_index++;
				mbd = mbd->next;
				last_was_rectobj = 1;
				gmi_ptr->wmap[wmap_index].type =
						GMI_SEPARATOR;
				gmi_ptr->wmap[wmap_index].mbd = mbd;

			/* HasSeparator tells me to add an expose event to the
			 * shell to process separators and titles
			 */
				HasSeparator = True;
				continue;
			} /* Separator */
			{ /* Begin block */
				OlgAttrs	*label_mdm =
						(OlgAttrs *)NULL;
				OlgTextLbl	labeldata;
				Dimension	labelwidth, labelheight;
				char *text = mbd->label ? mbd->label :
			OlGetMessage(XtDisplay(Frame), NULL,  0,
                        	OleNtitle, OleTpopupWindowMenu,
                        	OleCOlClientOlwmMsgs,
				OleMtitle_popupWindowMenu,
                        	(XrmDatabase)NULL) ; 

			labeldata.font = wmrcs->font;
			if (mcai->fontList)
				labeldata.font_list = mcai->fontList;
			else
				labeldata.font_list = ol_font_list;
			labeldata.accelerator = NULL;
			labeldata.mnemonic = NULL;
			labeldata.flags = (unsigned char) NULL;
			labeldata.label = text;
			labeldata.justification = TL_CENTER_JUSTIFY;
			label_mdm = OlgCreateAttrs(
					XtScreen(gmi_ptr->MenuShell),
					mcai->activeForeground,
					(OlgBG *)&(mcai->activeForeground),
					(Boolean)FALSE, (Dimension)12);
			labeldata.normalGC = OlgGetFgGC(label_mdm);
			if (labeldata.font)
				XSetFont(XtDisplay(gmi_ptr->MenuShell),
						labeldata.normalGC,
						labeldata.font->fid);
			OlgSizeTextLabel(
				XtScreen(gmi_ptr->MenuShell),
				label_mdm,
				&labeldata, &labelwidth, &labelheight);
				
			sepwidth = labelwidth + 10;
					/* Just add constant for 5 pixels
					 * on each side.
					 */
/*
 * Dimensions:  Total = Title height + 2 * SEP_B_O + 6 (for the lines) + 2 *
 * TITLE_OFFSET
 *
		sepheight = labelheight + 2 * SEPARATOR_BORDER_OFFSET +
				6 + 2 * TITLE_OFFSET;
 */

		sepheight = labelheight + 2 * SEPARATOR_BORDER_OFFSET +
				6 + 2 * TITLE_OFFSET;

			wmap_index++;

			gmi_ptr->wmap[wmap_index].widget =
				XtVaCreateManagedWidget(
				"Title", rectObjClass,
				gmi_ptr->MenuShell,
				XtNwidth, (XtArgVal)sepwidth,
				XtNheight, (XtArgVal) sepheight,
				(char *)0);

			gmi_ptr->mbmap[mbmap_index].wmap_index = wmap_index;
			gmi_ptr->mbmap[mbmap_index].flatpane_index = -1;
			gmi_ptr->mbmap[mbmap_index].mbd = mbd;
			gmi_ptr->wmap[wmap_index].type = GMI_TITLE;
			gmi_ptr->wmap[wmap_index].mbd = mbd;
			mbmap_index++;
			mbd = mbd->next;
			last_was_rectobj = 1;

			/* HasSeparator tells me to add an expose event to the
			 * shell to process separators and titles
			 */
			HasSeparator = True;
			if (label_mdm)
			   {
				OlgDestroyAttrs(label_mdm);
				label_mdm=NULL;
			   }
			} /* End block for title */

			continue;
		} /* if */
		else {
			/* Real button, possibly */
			if ( (last_was_rectobj == -1 /* first time */ )
			  || (last_was_rectobj == 1) /* last WAS rectobj */) {
				/* Create flat buttons */
				XtSetArg(menuargs[0], XtNitemFields,
							menuFields);
				XtSetArg(menuargs[1], XtNnumItemFields,
							XtNumber (menuFields));
				XtSetArg(menuargs[2], XtNlayoutType,
							OL_FIXEDCOLS);
				XtSetArg(menuargs[3], XtNmeasure, 1);
				XtSetArg(menuargs[4], XtNclientData,
							&(gmi_ptr->w));
				XtSetArg(menuargs[5], XtNitems, dummy_items);
				XtSetArg(menuargs[6], XtNnumItems,
							XtNumber(dummy_items));

				/* Advance to next map entry */
				wmap_index++;
				flatpane_index++;
				gmi_ptr->wmap[wmap_index].widget =
						XtCreateManagedWidget(
						"flat-btn",
				      		flatButtonsWidgetClass,
				      		gmi_ptr->MenuShell,
				      		menuargs, 7);
				gmi_ptr->wmap[wmap_index].type = GMI_FLAT;
				gmi_ptr->mbmap[mbmap_index].wmap_index =
							wmap_index;
				gmi_ptr->mbmap[mbmap_index].mbd = mbd;
				mbd = mbd->next;
				last_was_rectobj = 0;
				gmi_ptr->buttons_per_pane[flatpane_index]++;
				gmi_ptr->mbmap[mbmap_index].flatpane_index =
					flatpane_index;
				gmi_ptr->wmap[wmap_index].flatpane_index = 
					flatpane_index;
				mbmap_index++;
				continue;
			} /* if first time, or last was rectobj */
			else {
				/* last_was rectobj = 0, same flat pane */
				gmi_ptr->mbmap[mbmap_index].wmap_index =
							wmap_index;
				gmi_ptr->mbmap[mbmap_index].mbd = mbd;
				mbd = mbd->next;
				gmi_ptr->buttons_per_pane[flatpane_index]++;
				gmi_ptr->mbmap[mbmap_index].flatpane_index =
					flatpane_index;
				mbmap_index++;
				continue;
			} /* little else */
		} /* else real button, possibly */
	   } /* first for */

	   /* O.K.: Now have the maps set up with all the menushells in
	    * the correct order, with maybe some rectobjs in between.
	    * But now the rest of the story: the buttons must be
	    * converted into WMMenuDefs and keys with possibly real
	    * accelerators.
	    */

		/* The second for: simple: go through the map of buttons
		 * we just made, and create the WMMenuDefs!
		 * mbmap_index is the index in the menu button map that
		 * we are up to - that's the maximum number of WMMenuDefs
		 * needed, but some of those will be labels or titles, so
		 * the number will be less.
		 */
		for (k=0; k < mbmap_index; k++) {
			mbd = gmi_ptr->mbmap[k].mbd;
			if (!mbd || mbd->menucb == OlwmTitle ||
					mbd->menucb == OlwmSeparator)
				continue;

			gmi_ptr->mbmap[k].menu_button.label = mbd->label;
			gmi_ptr->mbmap[k].menu_button.mnemonic = mbd->mnemonic;
			gmi_ptr->mbmap[k].menu_button.selectProc = mbd->menucb;
			gmi_ptr->mbmap[k].menu_button.sensitive = True;
			gmi_ptr->mbmap[k].menu_button.accelerator =
						mbd->accelerator;
			gmi_ptr->mbmap[k].menu_button.defaultItem = False;
			gmi_ptr->mbmap[k].menu_button.subMenu = (Widget)NULL;

			/* Back up a second: the menu_button.subMenu field
			 * is a good one.  If the menu function is
			 * f.menu "menuname", then we set the subMenu
			 * field to the menu widget named.  This widget
			 * was likely created in this function, so we
			 * don't know if it was created yet!  Best bet
			 * is therefore to set up the subMenu field
			 * when the menu is built.  This may be from
			 * another menu pane we build here, or from
			 * a program that uses the MWM_MENU property.
			 */

		/* Watch closely: put any arguments that I may need
		 * over here.  "Share" the helpdata structure for this
		 * purpose.  Don't forget - the args must be freed,
		 * and the helpData struct must be freed, in that order.
		 */
			if (mbd->args) {
				gmi_ptr->mbmap[k].menu_button.helpData =
					(WMHelpDef *)XtMalloc(sizeof(WMHelpDef));
#ifdef DEBUG
			fprintf(stderr,"XtMalloc: helpData=%x\n",
				gmi_ptr->mbmap[k].menu_button.helpData);
#endif
			  gmi_ptr->mbmap[k].menu_button.helpData->menuargs =
								mbd->args;
			} /* if mbd->args */
			else
				gmi_ptr->mbmap[k].menu_button.helpData = NULL;
		} /* End second for */

		gmi_ptr->num_flat_panes = flatpane_index + 1;
		for (i=0; i <= flatpane_index; i++)
			gmi_ptr->motif_menu[i] = (WMMenuDef *)XtMalloc(
				gmi_ptr->buttons_per_pane[i] * sizeof(WMMenuDef) );

		if (HasSeparator)
			XtAddEventHandler(gmi_ptr->MenuShell,
				ExposureMask, False, MenuExpose,
				&(gmi_ptr->MenuShell));


		/* The following code is from NewMenuItems() - for
		 * the virtual keys.  This must be done on a per step widget
		 * basis, but we can gather the nuts here. 
		 * Windows that use this menu can call
		 * OlCreateInpuitEventDB(), and it's business as usual
		 * after that when they get keyboard focus. 
		 */

	/* The number of virtual keys being malloc'd is a little
	 * big, but we can always try to optimize it
	 */

	gmi_ptr->menu_keys = (OlKeyOrBtnRec *)XtMalloc((unsigned)
				(sizeof(OlKeyOrBtnRec) * 
				gmi_ptr->num_mbmap));
	for (j=0, k=0; k < gmi_ptr->num_mbmap; k++) {
		if (gmi_ptr->mbmap[k].mbd->menucb == OlwmTitle ||
			gmi_ptr->mbmap[k].mbd->menucb == OlwmSeparator)
				continue;
		if ( (gmi_ptr->menu_keys[j].default_value =
		      gmi_ptr->mbmap[k].menu_button.accelerator) != NULL) {
			gmi_ptr->menu_keys[j].name =
				 gmi_ptr->mbmap[k].menu_button.label;
			gmi_ptr->menu_keys[j].virtual_name =
				 NextAvailableVKey++;
			j++;
		}
	} /* for */
	gmi_ptr->menu_keys_used = j;

/* Create on a per step widget basis when needed ...
	if ( (wmstep->private_db = OlCreateInputEventDB((Widget)wm,
			wmstep->private_keys,
			j, NULL, 0)) != NULL) {
		OlWidgetSearchIEDB((Widget)wm->wmstep.child, wmstep->private_db);
	}
 */
		gmi_ptr = gmi_ptr->next;
	} /* END while gmi_ptr != NULL */
} /* End CreateGlobalMenu */

void
NewMenuItems OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
WMMenuButtonData *mbd = wmstep->menu_ext;
int j, k;
	if (mbd == NULL)
		return;
	for (k=1; mbd->next; k++, mbd = mbd->next)
		;	
	if ( (wmstep->private_buttons = (WMMenuDef *)XtMalloc(
			    k*sizeof(WMMenuDef)) ) == NULL)
		OlVaDisplayErrorMsg(XtDisplay(wm), OleNspace, OleTexit,
				OleCOlClientOlwmMsgs, OleMspace_exit, NULL);
#ifdef DEBUG
	fprintf(stderr,"Malloc private_buttons = %x num_Bytes=%d\n", wmstep->private_buttons, k*sizeof(WMMenuDef));
#endif
	mbd = wmstep->menu_ext;
	wmstep->private_buttons_avail = k;
	while (mbd) {
		/* I can't handle these now - would require some menu
		 * changes, or some drastic changes to how I do menus,
		 * beyond my time limits at this point.
		 */
		if (mbd->menucb == OlwmTitle || mbd->menucb == OlwmSeparator){
			mbd = mbd->next; continue;
		}
		/* Is it a transient window ? */
		if (wmstep->transient_parent) {
			/* Disallow some buttons */
		}
		k = wmstep->private_buttons_used;
		wmstep->private_buttons[k].label = mbd->label;
		wmstep->private_buttons[k].mnemonic = mbd->mnemonic;
		wmstep->private_buttons[k].selectProc = mbd->menucb;
		wmstep->private_buttons[k].sensitive = True;
		wmstep->private_buttons[k].accelerator =
			mbd->accelerator;
		wmstep->private_buttons[k].defaultItem = False;
		wmstep->private_buttons[k].subMenu = (Widget)NULL;

		/* Watch closely: put any arguments that I may need
		 * over here.  "Share" the helpdata structure for this
		 * purpose.  Don't forget - the args must be freed,
		 * and the helpData struct must be freed, in that order.
		 */
		if (mbd->args) {
			wmstep->private_buttons[k].helpData =
				(WMHelpDef *)XtMalloc(sizeof(WMHelpDef));
#ifdef DEBUG
			fprintf(stderr,"XtMalloc: helpData=%x\n",
				wmstep->private_buttons[k].helpData);
#endif
			wmstep->private_buttons[k].helpData->menuargs =
				mbd->args;
		}
		else
			wmstep->private_buttons[k].helpData = NULL;

		/* Check out f.menu */
		if (mbd->menucb == OlwmMenu) {
			char *menuname =
				wmstep->private_buttons[k].helpData ?
			(char *)(wmstep->private_buttons[k].helpData->menuargs):
				NULL;

			if (menuname) {
				/* does this menu exist */
					Global_Menu_Info *gmi_ptr =
						global_menu_info;
					while (gmi_ptr)
						if (gmi_ptr->menuname && !strcmp(
						  menuname, gmi_ptr->menuname))
						    break;
						else
							gmi_ptr = gmi_ptr->next;
					if (gmi_ptr) {
						/* Use the menu */
				wmstep->private_buttons[k].subMenu =
					gmi_ptr->MenuShell;
				wmstep->private_buttons[k].sensitive = True;
					}
					else
				wmstep->private_buttons[k].sensitive = False;
			} /* if menuname */
		} /* if menucb == OlwmMenu */
			else
				wmstep->private_buttons[k].sensitive = False;
		
	
				
		wmstep->private_buttons_used++;
		mbd = mbd->next;
	} /* while */

	wmstep->private_keys = (OlKeyOrBtnRec *)XtMalloc((unsigned)
				(sizeof(OlKeyOrBtnRec) * 
				wmstep->private_buttons_used));
#ifdef DEBUG
	fprintf(stderr,"Malloc: private_keys=%x num_bytes=%d\n", wmstep->private_keys, wmstep->private_buttons_used * sizeof(OlKeyOrBtnRec));
#endif
	/* Only accept keys for the database that have accelerators */
	for (j=0, k=0; k < wmstep->private_buttons_used; k++) {
		if ( (wmstep->private_keys[j].default_value =
		      wmstep->private_buttons[k].accelerator) != NULL) {
			wmstep->private_keys[j].name =
				 wmstep->private_buttons[k].label;
			wmstep->private_keys[j].virtual_name =
				 NextAvailableVKey++;
			j++;
		} /* if */
		/* For the above, if a buttons doesn't have an accelerator,
		 * then we have extra space at the end.
		 */
	} /* for */
	wmstep->private_keys_used = j;
	if ( (wmstep->private_db = OlCreateInputEventDB((Widget)wm,
			wmstep->private_keys,
			j, NULL, 0)) != NULL) {
		OlWidgetSearchIEDB((Widget)wm->wmstep.child, wmstep->private_db);
	} /* != NULL */
} /* NewMenuItems */




/*
 * WmPopupWindowMenu.  Just copied word for word from MenuShell.c,
 * but I need to call the _ function to get the menu alignment that
 * I want.
 *
 */
static void
WmPostPopupWindowMenu OLARGLIST((menu_owner, popup_menu, activation_type,
		popdown, root_x, root_y, init_x, init_y, rect_w, rect_h))
OLARG(Widget, menu_owner)
OLARG(Widget, popup_menu)
OLARG(OlVirtualName, activation_type)
OLARG(OlPopupMenuCallbackProc, popdown)
OLARG(Position, root_x)
OLARG(Position, root_y)
OLARG(Position, init_x)
OLARG(Position, init_y)
OLARG(Dimension, rect_w)
OLGRA(Dimension, rect_h)
{
	XRectangle	rect;
	Dimension      TWO_POINTS = OlScreenPointToPixel(OL_HORIZONTAL,
					2, XtScreenOfObject(popup_menu));

	rect.x		= root_x + TWO_POINTS;
	rect.y		= root_y + TWO_POINTS;
	rect.width	= rect_w;
	rect.height	= rect_h;

	_OlResetPreviewMode(popup_menu);

	/* Use AbbrevDropDownAlignment (Northwest) alignment for menu */

	_OlPopupMenu(
		popup_menu, menu_owner, popdown, &rect,
		AbbrevDropDownAlignment, True,
		XtWindowOfObject(menu_owner), init_x, init_y);

	if (activation_type == OL_MENUKEY)
	{
		_OlSetStayupMode(popup_menu);
	}
	else
	{
			/* if got Button Release after calling  */
			/* _OlPopupMenu then we have to reset   */
			/* to pending stayup otherwise a menu   */
			/* will never show up if time interval  */
			/* between a Button Press and a Button  */
			/* Release is really short...           */
		if (_OlIsInStayupMode(popup_menu))
			_OlSetStayupModeValue(popup_menu, PENDING_STAYUP);
	}
} /* end of WmPostPopupWindowMenu */



/* The menu function that may appear from the MWM_MENU property follow.
 * Only a handful will take an argument; these will be more complicated
 * to deal with but not much, because we must know whether the function
 * was called from the menu callback or from KeyPressEvent().  Depending
 * on where it was called from, the value of call_data.item_index may
 * represent a different index - if from the menu_callback, then it
 * is the index into combined_menu; if from KeyPressEvent, then it is
 * an index into wmstep->private_buttons;  MwmMenuArgsFlag will tell us so
 * (remember to reset it to 0).
 *	-These 5 were malloc'd space-
 *	OlwmExec().
 *	OlwmMenu().
 *	OlwmLower().
 *	OlwmRaise().
 *	OlwmRaise_Lower().
 *
 *	 -  In there, we used the XtPointer arg structure element, which
 *	  is your basic pointer, as the actual argument, only we just
 *	  interpret the pointer itself as the argument!  if the ptr ==
 *	  NULL (0) then there is no argument, if non-null, interpret it
 *	  as an integer.  For example the argument is either 
 *	  ICON|WINDOW|TRANSIENT for one of these.  We simply make a #define
 *	  for each one.
 *
 *	OlwmCircle_Down()
 *	OlwmCircle_Up().
 */
void
OlwmBeep OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget	wm =		(WMStepWidget) *((WMStepWidget *)client_data);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
	XBell(XtDisplay(w), 0);
#ifdef DEBUG
	fprintf(stderr,"Look at the call data: item_index=%d num_items=%d num_item_fields=%d \n",
		flat_call_data->item_index,
		flat_call_data->num_items,
		flat_call_data->num_item_fields);
#endif

/* This gets us the item index into the combined_menu struct, which is the
 * XtNitems for the flat buttons, but only if we got here
 * via a menu callback, as opposed to via KeyPressEvent(); if we got here
 * by a keypress, then the index isn't really valid, unless we do 
 * something to make it valid, like add an extra field somewhere.
 */
}

/* OlwmCircle_Down() - args == [icon | window] - 
 *	no arg: move window (icon) on top of stack to bottom;
 *	icon: applies only to icons.
 *	window: applies only to windows.
 */
void
OlwmCircle_Down OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart  *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
WMStepWidget tempwm;
char 	*args;
WMHelpDef *hd;
char	*client;
unsigned int which;
int i;
Global_Menu_Info *gmi_ptr = global_menu_info;

	if (MwmMenuArgsFlag) {
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;


		/*hd = combined_menu[flat_call_data->item_index].helpData;*/
	if (hd)
		which  = (unsigned int)(hd->menuargs);
	else
		/* Just use default */
		which = ICON;
	tempwm = (WMStepWidget)window_list->p[window_list->used-1];
	switch(which) {
		case WINDOW:
			RaiseLowerGroup(tempwm, WMLower);
			break;
		case ICON:
			if (!IsIconic(tempwm))
				break;
			/* fall through */
		default:
			RaiseLowerGroup(tempwm, WMLower);
			break;
	} /* switch */
} /* OlwmCircle_Down */

void
OlwmCircle_Up OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
Global_Menu_Info *gmi_ptr = global_menu_info;
char 	*args;
WMHelpDef *hd;
char	*client;
unsigned int which;
WMStepWidget tempwm;
int raise = 1;
int i;

	if (MwmMenuArgsFlag) {
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
		/*hd = combined_menu[flat_call_data->item_index].helpData;*/
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;

	if (hd)
		which  = (unsigned int)(hd->menuargs);
	else
		/* use default */
		which = ICON;
	tempwm = (WMStepWidget)window_list->p[0];
	switch(which) {
		case WINDOW:
			if (!(IsIconic(tempwm)))
				RaiseLowerGroup(tempwm, WMRaise);
			break;
		case ICON:
			if (!(IsIconic(tempwm)))
				break;
			/* If iconic, fall through */
		default:
			RaiseLowerGroup(tempwm, WMRaise);
	} /* switch */
} /* OlwmCircle_Up */

/* OlwmExec() - OR substitute ! -
 * arg = command -
 *	execute command;  which shell to use: check $MWMSHELL, then
 *	$SHELL, the default to /bin/sh.
 */
void
OlwmExec OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
int   status;
int   pid;
int	wait_status;
void (*intfunc)(); /* signal handler function for SIGINT */
void (*quitfunc)(); /* signal handler function for SIGQUIT */
char *shell;
char *shellname;
char 	*args = (char *)NULL;
WMHelpDef *hd;
char	*execstring;
Global_Menu_Info *gmi_ptr = global_menu_info;
int i;

	/* Get the argument, if any  -  the args can be found inside the
 	 * userdata field - in our case, it's the WMHelpDef.
 	 */
	if (MwmMenuArgsFlag) {
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;
	if ( hd  && hd->menuargs)
		execstring = hd->menuargs;
	else
		return;


#ifdef VFORK
	if ((pid = vfork ()) == 0) {
#else
	if ((pid = fork ()) == 0) {
#endif
		/* child - use default signal handling */
		if (signal(SIGINT, SIG_DFL) == SIG_IGN) {
	    		signal(SIGINT, SIG_IGN);
		}
		if (signal (SIGHUP, SIG_DFL) == SIG_IGN) {
	    		signal (SIGHUP, SIG_IGN);
		}
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/*
	 	 * What shell to use:
	 	 * 1) if set, $MWMSHELL; else
	 	 * 2) if set, $SHELL
	 	 * 3) else use /bin/sh.
	 	 */

		if (((shell = getenv("MWMSHELL")) != NULL) ||
				((shell = getenv("SHELL")) != NULL)) {
			shellname = strrchr (shell, '/');
			if (shellname == NULL) {
				shellname = shell;
			}
			else {
				shellname++;
			}
			execl(shell, shellname, "-c", execstring, 0);
		} /* if getenv(MWMSHELL) || getenv(SHELL) */

		 /* no SHELL env var set, or execl failed. - Use /bin/sh */
		execl("/bin/sh", "sh", "-c", execstring, 0);

		/* Returned - obviously it failed */
		_exit(1);
	}

	/* Parent (olwm) - wait for command */

	/* save previous SIGINT, SIGQUIT signal handler functions */
	intfunc = (void (*)())signal(SIGINT, SIG_IGN);
	quitfunc = (void (*)())signal(SIGQUIT, SIG_IGN);

	while ( ((wait_status = wait(&status)) != pid) && (wait_status != -1))
					;
/* if wait_status == -1, then we got a bad return from the command we
 * exec'd.  Should we print a warning, or just continue? test it out.
 */
	if (wait_status == -1) {
		status = -1;
	}

	signal(SIGINT, intfunc);
	signal(SIGQUIT, quitfunc);

} /* OlwmExec */

/* = No-op if colormapFocusPolicy isn't explicit;
 * else, install colormap focus to the window
 */
void
OlwmFocus_Color OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
	if (wmrcs->pointerColormapFocus)
		return;
	if (CurrentColormapWindow != XtWindow((Widget)wm)) {
		CurrentColormapWindow = XtWindow((Widget)wm);
		WMInstallColormaps(wm);
	}
} /* OlwmFocus_Color */

/*
 * OlwmFocus_Key - a no-op if real-estate based focus.
 * Set focus to window.
 */
void
OlwmFocus_Key OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
	if (wmrcs->pointerFocus)
		return;
	SetFocus((Widget)wm, GainFocus, 1);
}

/*
 * Kill the client, not just the window.
 */
void
OlwmKill OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
Window leader;
int idx;

	/* Find the owner and group of the window - we want to kill
	 * the whole client, not just this window, if, say, it's a
	 * transient window.  Find the highest group.
	 */
	if (wm->wmstep.xwmhints->window_group == wm->wmstep.window) {
		MenuQuit(w, client_data, call_data);
		return;
	}
	/* transient or has different window group */
	leader = find_highest_group(wm, (Window) 0);
	if ( (idx=IsWMStepChild(leader)) > 0) {
		/* quit it */
		MenuQuit(w, client_data, call_data);
	}
	else {
		/* no mapped leader - can they be popups (transients)?
		 * Can probably get away with using Dismiss for
		 * all windows in the group.
		 */
		MenuDismissPopups(w, client_data, call_data);
	}
} /* OlwmKill */


/* OlwmLower args = [-client] (optional, but need '-').
 * Move window to bottom of stack.  Client arg =
 * name or class of client to lower.
 */
void
OlwmLower OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
char 	*args;
WMHelpDef *hd;
char	*client;
Global_Menu_Info *gmi_ptr;
int i;

/* Get the argument, if any  -  the args can be found inside the
 * userdata field - in our case, it's the WMHelpDef.
 */
	if (MwmMenuArgsFlag) {
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;
	if ( hd  && hd->menuargs)

	if (!(hd) || !(hd->menuargs))
		/* Raise this client */
		RaiseLowerGroup(wm, WMLower);
	else {
		/* What client is it? Search window names (xclasshint.res_name)
		 * and xclasshint.res_class, both strings, for a match to
		 * the named client.  Raise all clients that match.
		 */
		int i;

		client = hd->menuargs;

		for (i=0; i < num_wmstep_kids; i++) {
			WMStepWidget wm = (WMStepWidget)wmstep_kids[i];
			WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

#ifdef WITHDRAWNSTATE
			if (wmstep->size == WMWITHDRAWN)
				continue;
#endif
			if ((!(strcmp(client, wmstep->classhint.res_name))) ||
			  (!(strcmp(client, wmstep->classhint.res_class)))) {
				RaiseLowerGroup(wm, WMLower);
			}
		 } /* for */
	} /* else */

} /* OlwmLower */

/* OlwmMaximize: make full size */
void
OlwmMaximize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	/* Check to see if this is a no-op */
	if (wmstep->size & WMFULLSIZE || wmstep->size & WMICONICFULL)
		return;
	if (wmstep->decorations & WMUsesMwmh) {
		if (!(wmstep->menu_functions & WMFuncMaximize))
			return;
	}

		MenuFullRestore(w, client_data, call_data);
} /* OlwmMaximize */

/* OlwmMenu -  Set up the popup menu in the subMenu.
 */
void
OlwmMenu OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
char *menuname;
WMHelpDef *hd;
Window junk;
int x, y, ijunk;
unsigned int uijunk;
Boolean via_key;
Global_Menu_Info *gmi_ptr;
int i;

/* Get the argument, if any  -  the args can be found inside the
 * userdata field - in our case, it's the WMHelpDef.
 */
	if (MwmMenuArgsFlag) {
		MwmMenuArgsFlag = 0;
		via_key = True; /* got here by keypress */
		/* Are you sure about this - that private_buttons
		 * is non-null??
		 */
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else {
		via_key = False;
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;


		via_key = False;
	}
	if (hd && (hd->menuargs)) {
		menuname = hd->menuargs;

			if (menuname) {
				/* does this menu exist */
					Global_Menu_Info *gmi_ptr =
						global_menu_info;
					while (gmi_ptr)
						if (!strcmp(
						  menuname, gmi_ptr->menuname))
						    break;
					if (gmi_ptr) {
						/* Use the menu */
					  GetSpecialMenuItems(wm, gmi_ptr);

#if 0 /* SAMC */
	WMCombinedMenu->w = (Widget)wm;

	(void) XQueryPointer(XtDisplay(wm), RootWindowOfScreen(XtScreen(wm)),
		&junk, &junk, &x, &y, &ijunk, &ijunk, &uijunk);

	if (currentGUI == OL_MOTIF_GUI) {
		int	menuwidth,
			menuheight;
		int	screenwidth = WidthOfScreen(XtScreen((Widget)wm));
		int	screenheight = HeightOfScreen(XtScreen((Widget)wm));

		menuwidth = WMCombinedMenu->MenuShell->core.width;
		menuheight = WMCombinedMenu->MenuShell->core.height;
		if (IsIconic(wm)) {
			int	iconx = wm->wmstep.icon_widget->core.x,
				icony = wm->wmstep.icon_widget->core.y;
			x = iconx;
			y = icony - menuheight;
			if ((int)(x + menuwidth) > WidthOfScreen(
					XtScreen((Widget)wm)) )
				x = iconx + icon_width - menuwidth;
			/* now do y */
			if (y < 0)
				y = icony + icon_height;
		}
		else {
			/* Not iconic, position menu */
			int vwidth, hwidth;
			if (Resizable(wm)) {
			  vwidth = wmstep->metrics->motifVResizeBorderWidth;
			  hwidth = wmstep->metrics->motifHResizeBorderWidth;
			}
			else {
				if (wmstep->decorations & WMBorder) {
		  		vwidth =
				  wmstep->metrics->motifVNResizeBorderWidth;
		  		hwidth =
				  wmstep->metrics->motifHNResizeBorderWidth;
				}
				else /* No border */
					vwidth = hwidth = 1;
			} /* Not resizable */
			x = wm->core.x + hwidth;
			y = wm->core.y + vwidth;
			if (x + menuwidth > screenwidth)
				x = screenwidth - menuwidth;
			if (x < 0)
				x = 0;
			if (y + menuheight >  screenheight)
				y = wm->core.y - menuheight;
			if (y < 0)
				y = 0;
		} /* else !iconic */
	}
	WmPostPopupWindowMenu(
		XtParent(gmi_ptr->MenuShell),
		gmi_ptr->MenuShell,
		via_key == False ? OL_MENU : OL_MENUKEY,
		(OlPopupMenuCallbackProc)NULL,
		(Position)x, (Position)y,	/* root_x, root_y	*/
		(Position)x, (Position)y,	/* init_x, init_y	*/
		1, 1				/* rect_w, rect_h	*/
	);
#endif /* SAMC */

					}
			} /* if menuname */
		} /* if hd */
}

/* OlwmMinimize - Iconify. Note in motif: minimized windows are placed 
 * on bottom of window stack...
 */
void
OlwmMinimize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	/* Check to see if this is a no-op */
	if (IsIconic(wm))
		return;
	if (wmstep->decorations & WMUsesMwmh) {
		if (!(wmstep->menu_functions & WMFuncMinimize))
			return;
	}
		MenuOpenClose(w, client_data, call_data);
}

/* 
 * OlwmMove
 */
void
OlwmMove OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
int moveit = 1;
	if ( ( (wmstep->decorations & WMUsesMwmh) &&
			(wmstep->mwmh.flags & MWM_HINTS_FUNCTIONS) &&
			(wmstep->menu_functions & WMFuncMove) ) || /* ORRR */
			!(wmstep->decorations & WMUsesMwmh) )
		Menu_Move((Widget)wm, client_data, call_data);
}

/* 
 * OlwmNext_Cmap.
 * For window with colormap focus, install next colormap (WM_COLORMAP_WINDOWS).
 * If window doesn't have a WM_COLORMAP_WINDOWS property, then no work needed.
 * Else check out the list of colormaps.
 */
void
OlwmNext_Cmap OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
}

/* OlwmNext_Key args: [icon|window|transient]. 
 * F.nop if keyboardFocusPolicy not explicit.
 * no args: move focus to next window/icon in the set. 
 * icon: applies to icons only.
 * window: applies to windows only.
 * transient: transient windows traversed.
 */

void
OlwmNext_Key OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
WMHelpDef *hd;
Global_Menu_Info *gmi_ptr = global_menu_info;
int i;

	/* Find the arg, if any */
	if (MwmMenuArgsFlag) {
		/* Use button on the private_buttons list */
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;

	OlwmNext_Prev_Key(wm, hd, 1);
}

/* f.nop if keyboard focus policy != explicit;
 * Sets keyboard focus to client window/icon.
 */
void
OlwmNop OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
/* Do nothing */
}

/* OlwmNormalize : 
 * Restore client to normal size (and it's transients) 
 */
void
OlwmNormalize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;

	switch(wmstep->size) {
		case WMNORMALSIZE:
			break;
		case WMICONICFULL:
		case WMFULLSIZE:
			MenuFullRestore(w, (XtPointer)wm, call_data);
			break;
		case WMICONICNORMAL:
			MenuOpenClose(w, (XtPointer)wm, call_data);
	}
} /* OlwmNormalize */


/* OlwmNormalize_And_Raise
 * Restore client to normal size (and it's transients),
 * and raise it in stack 
 */
void
OlwmNormalize_And_Raise  OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;

	switch(wmstep->size) {
		case WMNORMALSIZE:
			break;
		case WMICONICFULL:
		case WMFULLSIZE:
			MenuFullRestore(w, (XtPointer)wm, call_data);
			break;
		case WMICONICNORMAL:
			MenuOpenClose(w, (XtPointer)wm, call_data);
	} /* switch */
} /* OlwmNormalize_And_Raise */


/* OlwmPack_Icons -
 * pack them in: relayout icons on root window (or icon box if you
 * have one).
 */
void
OlwmPack_Icons OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
/* We don't have an icon box, so just pack the icons in the order
 * according to the resource. (north, south, east or west)
 */
	PackIcons(w);
}

/* OlwmPass_Keys -
 * Toggle enabling of key  bindings for olwm function.
 * Disabled: pass key events on to window that has focus.
 * Enable: normal processing of window menu operations.
 */

void
OlwmPass_Keys OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
	PassKeysThrough = !PassKeysThrough;
}

/* OlwmPost_Wmenu - Post the window menu */
void
OlwmPost_Wmenu OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;

	/*if (HasMenu(wm) && !CurrentMenuPosted) {*/
	if (HasMenu(wm) && !NumMenushellsPosted) {
		/*
		 * You need an event type to pass to Menu().
		 */
			XEvent ev;

			ev.type = KeyPress;
			OlwmPostMenu(wm, &ev);
	} /* if */
} /* OlwmPost_Wmenu */

/* Install previous colormap from WM_COLORMAP_WINDOWS for window with
 * colormap focus.
 */
void
OlwmPrev_Cmap OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
}

/* OlwmPrev_Key - 
 * args: [icon|window|transient].  Nop if keybdFocusPolicy != explicit.
 * no args: set focus to previous window/icon.
 * ***Don't move to a window that has a transient window that is
 * APPLICATION MODAL.
 * icon: icons only.
 * window: applies to windows.
 * transient: traverse transient windows.
 */
void
OlwmPrev_Key OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
WMHelpDef *hd = (WMHelpDef *)NULL;
Global_Menu_Info *gmi_ptr = global_menu_info;
int i;

	/* find the argument, if any */
	if (MwmMenuArgsFlag) {
		/* Use button on the private_buttons list */
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;

	OlwmNext_Prev_Key(wm, hd, 0);
}

/* OlwmQuit_Mwm -
 * Quit window mgr (obviously olwm in my case).
 */
void
OlwmQuit_Mwm OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
/*
	TrapSignal();
 */
	/* No return from TrapSignal */
}

/* OlwmRaise -
 * arg: [-client] optional.
 * Raise to top of window stack.
 */
void
OlwmRaise OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
char 	*args;
WMHelpDef *hd;
char	*client;
Global_Menu_Info *gmi_ptr = global_menu_info;
int i;

/* Get the argument, if any  -  the args can be found inside the
 * userdata field - in our case, it's the WMHelpDef.
 */
	if (MwmMenuArgsFlag) {
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;
/*		hd = combined_menu[flat_call_data->item_index].helpData;*/
	if (!(hd->menuargs))
		/* Raise this client */
		RaiseLowerGroup(wm, WMRaise);
	else {
		/* What client is it? Search window names (xclasshint.res_name)
		 * and xclasshint.res_class, both strings, for a match to
		 * the named client.  Raise all clients that match.
		 */
		int i;

		client = hd->menuargs;

		for (i=0; i < num_wmstep_kids; i++) {
			WMStepWidget wm = (WMStepWidget)wmstep_kids[i];
			WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

#ifdef WITHDRAWNSTATE
			if (wmstep->size == WMWITHDRAWN)
				continue;
#endif

			if ((!(strcmp(client, wmstep->classhint.res_name))) ||
			  (!(strcmp(client, wmstep->classhint.res_class)))) {
				RaiseLowerGroup(wm, WMRaise);
			}
		 } /* for */
	} /* else */
		
} /* OlwmRaise */

/* OlwmRaise_Lower -
 * Interesting: raise to top of window stack IFF it's partially
 * obscured by another window; else lower it.
 * Check for a window or icon argument. (For now, though, don't act on it)
 */
void
OlwmRaise_Lower OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
WMStepWidget tempwm;
int	x,
	y,
	width,
	height;
int	lower = 1, /* lower by default */
	i, j, k;
WMStepWidget checkwm;
Window	high_window_group,
	target_window_group;

	tempwm = (WMStepWidget)window_list->p[window_list->used-1];
	if (wm == tempwm)
		/* on top of stack, lower it */
		lower++;
	else {
	/* *** Check for iconic?? */
		ConstructGroupList(wm, NestedNoAppendAny, True);
		target_window_group = find_highest_group(wm,
						(Window)NULL);
		high_window_group = find_highest_group(tempwm,
						(Window)NULL);
		/* target_window_group == the group of the target window;
		 * high_window_group == the group of the window on top of
		 * the stack.
		 */
		
		/* Are any windows in the group on top of the stack? */
		for (i=1; i < group_list->used; i++) {
			tempwm = (WMStepWidget)group_list->p[i];
			if (high_window_group ==
				  find_highest_group(tempwm, (Window)NULL)) {
				lower++;
				break;
			} /* if */
		} /* for */
	} /* else */
	/* If lower == 2, then we must lower it because the client is on
	 * top of the stack; but if lower == 1, then do nothing yet, but
	 * we must check to see the client needs to be raised.  If so,
	 * set lower to 0.
	 */

	if (lower == 1) {
		/* check individual windows.  Find the set of windows that
		 * aren't in the window group, check each one against
		 * the windows in the group; if any cover a window in the
		 * group, then lower the group set, and break out of the
		 * loop.  O(n squared).
		 */
		if (IsIconic(wm))
			group_list->used = 1;

		for (i=0; i < window_list->used && lower; i++) {
			tempwm = (WMStepWidget)window_list->p[i];
			if (!(IsIconic(wm)) || IsIconic(wm) &&
				  wm->wmstep.icon_widget) {
				/* horizontal case */
				if (find_highest_group(tempwm, (Window)NULL)
						== target_window_group)
					/* same client window group */
					continue;
				/* Window or icon in different group */
			for (j=0; j < group_list->used; j++) {
				checkwm = (WMStepWidget)group_list->p[j];
				x = checkwm->core.x; y = checkwm->core.y;
				width = checkwm->core.width;
				height  = checkwm->core.height;

				if ((int)(tempwm->core.x + tempwm->core.width) <
				   (int)x || x + width < (int)tempwm->core.x)
					continue;
				if ((int)(tempwm->core.y + tempwm->core.height) < (int)y ||
						(int)(y + height) < (int)tempwm->core.y)
					continue;
				/* The windows overlap; but do we
				 * decrement lower (e.g., raise the window)?
				 * Get the index in window_list; Which is
				 * higher?  That is the window that's above.
				 * i == the index in the window_list that
				 * we are checking; j == the window in the
				 * window group that we are checking; j must
				 * be converted to the index in the window
				 * list.
				 */
				k = Window_List_Position(checkwm);
				if (k < i) {
					/* Raise it */
					lower--;
					break; /* from the for */
				} /* if k < i */
			} /* for j */
			} /* if */
		} /* for i */
	} /* if (lower == 1) */
	if (lower)
		RaiseLowerGroup(wm, WMLower);
	else
		RaiseLowerGroup(wm, WMRaise);
} /* OlwmRaise_Lower */

/* OlwmRefresh_Win -
 * Redraw 1 window
 */
void
OlwmRefresh_Win OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
	MenuRefresh(w, client_data, call_data);
}

/* OlwmRefresh -
 * Redraw all windows.
 */
void
OlwmRefresh OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
	int i;
	/* Best way to do this: loop through all windows,
	 * call MenuRefresh() for each.
	 */
	for (i=0; i < num_wmstep_kids; i++)
#ifdef WITHDRAWNSTATE
		if (wm->wmstep.size != WMWITHDRAWN)
#endif
			MenuRefresh(w, (XtPointer)wmstep_kids[i], (XtPointer)NULL);
	
}

/* OlwmResize -
 * Start resize operation.  Several checks here - if the step widget
 * is iconic, or is in any way not resizable, then earnings fall short,
 * and we just don't do it.  Of course if that is the case, then
 * the label should be insensitive.
 */
void
OlwmResize OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
	if (Resizable(wm) && !(IsIconic(wm)))
		Menu_Resize(w, client_data, call_data);
}

/* OlwmRestart -
 * restart win mgr
 */
void
OlwmRestart OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
	RestartWindowMgr(XtDisplay(w));
}

/* OlwmSend_Msg -
 * arg: msg_num (mandatory) - send client msg (type _MOTIF_WM_MESSAGES).
 * with message_type indicated by the msg # arg.  However, the msg is
 * sent ONLY if msg # is included in client MESSAGES property.  A menu item
 * label is "grayed out" if the menu item is used to do f.send_msg of
 * a message NOT included in the MESSAGES property. < In other words,
 * it sounds like it gets grayed out because nothing will get done,
 * right?? >
 */
void
OlwmSend_Msg OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
OlFlatCallData	*flat_call_data = (OlFlatCallData *) call_data;
int flx = flat_call_data->item_index;
long msg_num;
int i;
WMHelpDef *hd;
Atom MWM_MSGS;
Global_Menu_Info *gmi_ptr = global_menu_info;

#define _XA_MOTIF_WM_MESSAGES	"_MOTIF_WM_MESSAGES"
#define _XA_MWM_MESSAGES	_XA_MOTIF_WM_MESSAGES

	MWM_MSGS = XInternAtom(XtDisplay(wm), _XA_MWM_MESSAGES, False);

	/* First get the argument */
	if (MwmMenuArgsFlag) {
		/* Use button on the private_buttons list */
		MwmMenuArgsFlag = 0;
		hd = wm->wmstep.private_buttons[
			flat_call_data->item_index].helpData;
	}
	else 
	  if (currentGUI == OL_MOTIF_GUI &&
			wmstep->csinfo && wmstep->csinfo->windowMenu != NULL) {
		/* Unfortunate, but not lethal:  In order to get the
		 * flat button pane that this callback came from,
		 * I must do a linear search of all flat widget IDs!
		 */
		Widget target_flat = (Widget)NULL;
		WMMenuButtonData	*mbd;

		while (gmi_ptr) {
		
			for (i=0; i < gmi_ptr->num_wmap; i++)
				if (gmi_ptr->wmap[i].widget == w) 
					break;	
			if (i < gmi_ptr->num_wmap) {
				/* found - get flat item index into this
				 * pane.
				 */
				int fp_index = gmi_ptr->wmap[i].flatpane_index;
				hd = gmi_ptr->motif_menu[
					fp_index][flx].helpData;
				break;
			}
			gmi_ptr = gmi_ptr->next;
		} /* while */
	} /* if */
	else /* try the combined_menu2 */
		if (WMCombinedMenu->num_menu_items2 >
					flat_call_data->item_index)
			hd =
			  combined_menu2[flat_call_data->item_index].helpData;
		else
			hd = NULL;
	if (!hd)
		return;
	/*	hd = combined_menu[flat_call_data->item_index].helpData;*/
	/* The message number is simply the XtPointer (no mallocd space)*/
	msg_num = (long)hd->menuargs;
#ifdef DEBUG
fprintf(stderr,"The message number is %ld\n", msg_num);
#endif


	if (wmstep->mwm_msgs) {
		for (i=0; i < wmstep->num_mwm_msgs; i++)
			if (wmstep->mwm_msgs[i] == msg_num)
				break;
		if (i < wmstep->num_mwm_msgs)
			/* Send the client message - its all right */
			SendLongMessage(XtDisplay(wm), wmstep->window,
				MWM_MSGS, msg_num, CurrentTime,
				0, 0, 0);
	} /* if wmstep->mwm_msgs */
} /* OlwmSend_Msg */

/* No function is actually needed */
void
OlwmSeparator OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
}

void
OlwmSet_Behavior OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
}



/* OlwmTitle -
	Insert title in menu pane at specified location. 
 */
void
OlwmTitle OLARGLIST((w, client_data, call_data))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
WMStepWidget wm = (WMStepWidget) *((WMStepWidget *)client_data);
}

void
OlwmNext_Prev_Key OLARGLIST((wm, help_data, next))
OLARG(WMStepWidget, wm)
OLARG(WMHelpDef *, help_data)
OLGRA(int, next)
{
unsigned int which;
long msg_num;
int i, k, n;
OlVirtualName virtual_name;
Display *display = XtDisplay((Widget)wm);
WMHelpDef *hd = help_data;

	if ( (n = FindCurrentWindow(True) ) == -1) {
		XBell(display, 0);
		return; /* consume */
	}
	if (CheckMotifFocusStops(wm, virtual_name)) {
		XBell(display, 0);
		return;
	}
	if (hd)
		which = (long)hd->menuargs;
	else
		/* Just provide a default as next or prev app */
		which = ICON;
	switch(which) {
		case ICON:
		default:
			virtual_name = (next ? OL_NEXTAPP : OL_PREVAPP);
			break;
		case WINDOW:
		case TRANSIENT:
			virtual_name = (next ? OL_NEXTWINDOW : OL_PREVWINDOW);
			break;
	} /* which */
	wm = (WMStepWidget)wmstep_kids[n];
	
	if (virtual_name == OL_NEXTAPP || virtual_name == OL_PREVAPP)
		k = Next_Prev_Application(wm, virtual_name, n, which);
	else
		k = Next_Prev_Window(wm, virtual_name, n, which);
	wm = (WMStepWidget)wmstep_kids[k];
	RaiseLowerGroup(wm, WMRaise);
	if (IsIconic(wm)) {
		if (!(wmrcs->pointerFocus))
			XSetInputFocus(display, 
				RootWindowOfScreen(XtScreen(wm)), 
				RevertToNone, LastEventTime);
		SetCurrent(wm);
	}
	else
		SetFocus((Widget)wm, GainFocus, 1);
} /* OlwmNext_Prev_Key */
