/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Extern.h	1.41"
#endif

#ifndef _OL_EXTERN_H_
#define _OL_EXTERN_H_

/*
 ************************************************************************	
 * Description:
 *	This file contains the external data types, variables and
 * values.
 ************************************************************************	
 */

			/* include external function and string
			 * declarations
			 */
#include <ProcDeclar.h>
#include <Strings.h>
			/* include memory utility functions */
#ifdef MEMUTIL
#include <X11/memutil.h>
#endif

#define XA_WM_HELP_QUEUE(dpy) XInternAtom(dpy, "_HELP_QUEUE", False)

			/* Define the macro.  Note: Extern.c uses a
			 * different definition for this macro.
			 */
#ifndef EXTERN
#define EXTERN(t,var,val)	extern t var
#endif

			/*
			 * Entries that can't be included with the
			 * EXTERN macro.
			 */
extern WidgetBuffer *	window_list;
extern WidgetBuffer *	group_list;

extern Widget		menushells_posted[];

extern Cardinal		num_global_resources;
extern XtResource	global_resources[];


/* For motif resource description file menu */
extern Global_Menu_Info *global_menu_info;

extern WMResources *		wmrcs;	/* global olwm resources */

extern MotWMResources *		motwmrcs; /* additional mwm resources*/
extern Cardinal		num_motif_global_resources;
extern XtResource	motif_global_resources[];


 /* The motif component specific resources may be assigned to the
  * following components: menu, icon, client, feedback.
  * For example:
  *	 mwm*icon*background:	red
  * Therefore, the following variable, motcomprcs, points to the resource
  * struct that gets read in from the resource manager (Extern.c).  What we
  * will need is an array of dimension 4 of struct MotCompAppearanceInfo,
  * one for each of menu, icon, client and feedback.  After reading in the
  * resources with an xt function, copy over the fields into each struct
  * for later use.
  */

extern MotCompAppearanceInfo	*motcomprcs; /* mwm component specific */
extern XtResource	motif_comp_resources[];
extern Cardinal		num_motif_comp_resources;

/* This array contains resource information for each component under
 * scrutiny for decoration information, such as colors and pixmaps needed
 * for border colors and backgrounds.
 */
extern MotifCompResourceInfo	motCompRes[];

/* Client specific resources */
/*extern MotCSResources	*	motcsrcs; */  /* mwm client specific */

	/* mlp - try this */
extern MotCSInfo	*	motcsrcs;
extern XtResource	motif_cs_resources[];
extern Cardinal		num_motif_cs_resources;

extern WMMenuDef	combined_menu[];
extern String olwmMenuLabels[];
extern String motMenuLabels[];
extern String menuMnemonics[];
extern String motMenuMnemonics[];
extern String helpTitles[];
extern String mothelpTitles[];


extern WMMenuInfo *	WMCombinedMenu;

			/*
			 * WorkSpaceManager Entry.
			 * --> conditionally include this.
			 */
#ifdef _WSMcomm_h
	EXTERN( WSM_Request,	wsmr, {0});
#endif /* _WSMcomm_h */

			/*
			 * Entries for the application resources
			 */

EXTERN( WidgetList,	wmstep_kids, 0);
EXTERN( Cardinal,	num_wmstep_kids, 0);
EXTERN( Cardinal,	num_wmstep_slots, 0);

EXTERN( PendingInfo *,	pending_kids, 0);
EXTERN( Cardinal,	num_pending_kids, 0);
EXTERN( Cardinal,	num_pending_slots, 0);

EXTERN( Atom,			MWM_HINTS, 0);
EXTERN( Atom,			WM_COLORMAP_WINDOWS, 0);
				/* currently installed colormap
				 */
EXTERN( short,			called_filled_help, 0);
EXTERN( Colormap,		CurrentColormap, 0);
/*
EXTERN( int,			CurrentColormapCount, 0);
*/

				/* window with current cmap focus
				 */
EXTERN( Window,			CurrentColormapWindow, 0);

/* Cursors for motif mode */
EXTERN( Cursor, fleur_cursor, 0);
EXTERN( Cursor, ul_cursor, 0);
EXTERN( Cursor, ur_cursor, 0);
EXTERN( Cursor, ll_cursor, 0);
EXTERN( Cursor, lr_cursor, 0);
EXTERN( Cursor, l_cursor, 0);
EXTERN( Cursor, r_cursor, 0);
EXTERN( Cursor, t_cursor, 0);
EXTERN( Cursor, b_cursor, 0);
EXTERN( Cursor, motifWindowCursor, 0);

/* String declarations for the de-stringing */


/* Menu.c */
extern String OleMspace_exit;
extern String OleMtitle_popupWindowMenu;

/* Misc.c */
extern String OleMcolormap_truncate;

extern String OleMspace_memory;

extern String OleMbadWindowAttr_notFound;

extern String OleMbadBitmap_badRead;

extern String OleMnoTitle_noTitle;

extern String OleMcolor_winColors;

extern String OleMcolor_foreground;
extern String OleMcolor_background;
extern String OleMcolor_border;

extern String OleMdimension_border;
extern String OleMshowFeedback_badResSpec;
extern String OleMiconDecor_badResSpec;

extern String OleMclientDecorations_badResSpec;
extern String OleMclientFunctions_badResSpec;

/* Window.c */

extern String OleMmove_move;
extern String OleMresize_resize;
extern String OleMbadKeyboard_noGrab;

/* wm.c */

extern String OleMcolor_windowFrameColor;
extern String OleMbadAtom_internAtom;
extern String OleMcolor_duplicate;
extern String OleMcolor_inputWindowHeader;
extern String OleMolwm_running;

extern String OleMdupColor_foregroundBackground;
extern String OleMdupColor_foregroundInputWindowHeader;
extern String OleMdupColor_backgroundInputWindowHeader;

/* Event.c  - mainly help for titles in ClientNonMaskable() */
extern String OleMtitle_winMgr;
extern String OleMtitle_menu;
extern String OleMtitle_resizeCorner;
extern String OleMtitle_resizeHandles;
extern String OleMtitle_title;
extern String OleMtitle_border;
extern String OleMtitle_pushpin;
extern String OleMtitle_menuButton;
extern String OleMtitle_icon;
extern String OleMtitle_help;
extern String OleMtitle_motifMaximizeButton;
extern String OleMtitle_motifMinimizeButton;

extern String OleMfocus_focusNever;

/* wmm.c */
extern String OleMbadExec_execvp;

/* parse.c */
extern String OleMmnemonic_badSpec;
extern String OleMaccelerator_badSpec;
extern String OleMgroup_badSpec;

#if defined(CHECKFOCUS)
EXTERN( WMStepWidget,			CurrentFocusWindow, 0);
EXTERN( WMStepWidget,			PreviousFocusWindow, 0);
EXTERN( WMStepWidget,			CurrentFocusApplication, 0);
EXTERN( WMStepWidget,			PreviousFocusApplication, 0);
#endif

/* More Motif mode stuff: pixmaps for shadows, background, etc. */
/* First 6 are component resources, global or per application */
EXTERN( Pixmap,			backgroundPixmap, 0);
EXTERN( Pixmap,			bottomShadowPixmap, 0);
EXTERN( Pixmap,			topShadowPixmap, 0);
EXTERN( Pixmap,			activeBackgroundPixmap, 0);
EXTERN( Pixmap,			activeBottomShadowPixmap, 0);
EXTERN( Pixmap,			activeTopShadowPixmap, 0);

EXTERN( Widget,			motifAIW, 0);

EXTERN( int,			CurrentIconX, 0);
EXTERN( int,			CurrentIconY, 0);
/*EXTERN( unsigned int,			CurrentMenuPosted, 0);*/

/* For use with menushells_posted[] */
EXTERN( unsigned int,			NumMenushellsPosted, 0);

EXTERN( int,			CurrentX, 0);
EXTERN( int,			CurrentY, 0);
EXTERN( unsigned long,		DragMask, 0);
EXTERN( Widget,			Frame, 0);

EXTERN( int,			IconGridX, 0);
EXTERN( int,			IconGridY, 0);

			/* If KeyboardOperation == 1, we are moving, resizing
			 * a window via the keyboard.
			 */
EXTERN( int,			KeyboardOperation, 0);
EXTERN( Time,			LastEventTime, CurrentTime);
EXTERN( int,			MaxColormapCount, 0);

/* Motif mode minimum width/height variables - note that the minimums
 * are different depending on whether it has resize corners.
 */
EXTERN( Dimension,		Mresminwidth, 0);
EXTERN( Dimension,		Mnoresminwidth, 0);
EXTERN( Dimension,		Mresminheight, 0);
EXTERN( Dimension,		Mnoresminheight, 0);

EXTERN( int,			MwmMenuArgsFlag, 0);
EXTERN( Position,		NewX, 0);
EXTERN( Position,		NewY, 0);

EXTERN( OlDefine,		NextAvailableVKey, OL_WM_PRIVATE_KEYS);
				/* Flags for moving, resizing window	*/

EXTERN( int,			NowMovingWindow, 0);
EXTERN( int,			NowResizingWindow, 0);

/* In Motif mode, we need to keep around an application shell created with
 * name and class olwm for (currently) 2 reasons:
 *	- To register dynamic resources (only the open look resources are
 *	  dynamic).
 *	- To get help - getting the help path for help files leads to
 *	  a help directory - we have one set up now for olwm, not mwm -so
 *	  proceed as is.
 *
 */
/* EXTERN( Widget,			OlwmShell, 0); */

	/* PassKeysThrough - should I interpret keypress events
	 * in olwm, or pass them on to the client?
	 */
EXTERN(Boolean,			PassKeysThrough, False);

EXTERN( int,			PermScale, 0);
EXTERN( Pixel,			PreviousBackgroundColor, 0);
EXTERN( String,			PreviousBasicLocale, NULL);
EXTERN( Boolean,		PreviousFocusPolicy, FALSE);
EXTERN( XFontStruct *,		PreviousFont, NULL);
EXTERN( OlFontList *,		PreviousFontList, NULL);
EXTERN( Pixel,			PreviousForegroundColor, 0);
EXTERN( Pixel,			PreviousIconBackground, NULL);
EXTERN( Boolean,		PreviousIconBorder, 0);
EXTERN( XFontStruct *,		PreviousIconFont, NULL);
EXTERN( Pixel,			PreviousIconForeground, NULL);
EXTERN( WMIconGravity,		PreviousIconGravity, 0);
EXTERN( Boolean,		PreviousIconParentRelative, False);
EXTERN( Pixel,			PreviousInputFocusColor, 0);
EXTERN( Boolean,		PreviousParentRelative, False);
EXTERN( Boolean,		PreviousUsingThreeD, False);
EXTERN( Pixel,			PreviousWindowFrameColor, 0);
EXTERN( Colormap,		Root_Colormap, 0);

EXTERN( int,			ScreenResolution, 0);
EXTERN( Widget,			Shell, 0);
EXTERN( WMStepWidget,		TargetStepwidget, 0);
/* Make two menus now...
EXTERN( WMMenuInfo,		WMMenu, {0});
*/

EXTERN( int,		WMmaxx, 0);
EXTERN( int,		WMmaxy, 0);
EXTERN( int,		WMminx, 0);
EXTERN( int,		WMminy, 0);
EXTERN( int,			XIconIncrement, 0);
EXTERN( int,			Xincrement, 30);
EXTERN( int,			YIconIncrement, 0);
EXTERN( int,			Yincrement, 30);
EXTERN( OlDefine,		currentGUI, 0);
EXTERN( Pixmap,			default_icon, 0);
EXTERN( Pixmap,			default_mask, 0);
EXTERN( int,			default_icon_width, 0);
EXTERN( int,			default_icon_height, 0);

#ifdef RAW
EXTERN( Boolean,		dispatch_expose, True);
EXTERN( Widget,		dispatch_widget, NULL);
#endif

EXTERN( WMPiece,		first_horizontal, WM_SE);
EXTERN( WMPiece,		first_vertical, WM_SE);
EXTERN( int,			headcase, 0);
EXTERN( WMStepWidget,		help_parent, 0);
EXTERN( Boolean,		help_posted, 0);
EXTERN( Widget,			help_shell, 0);
EXTERN( unsigned int,		icon_height, 0);
EXTERN( unsigned int,		icon_width, 0);

/* NEW - Open Look only */
EXTERN( OlFontList *,		ol_font_list, 0);
EXTERN( unsigned int,		ol_icon_image_height, 0);
EXTERN( unsigned int,		ol_icon_image_width, 0);


/* Motif icons only */
EXTERN( unsigned int,		mot_icon_iheight, 0);
EXTERN( unsigned int,		mot_icon_iwidth, 0);
EXTERN( unsigned int,		mot_icon_label_height, 0);

EXTERN( int,			icon_x, 0);
EXTERN( int,			icon_y, 0);
EXTERN( Boolean,		inmm, 0);
EXTERN( Boolean,		inpp, 0);
EXTERN( int,			key_x, 0);
EXTERN( int,			key_y, 0);
EXTERN( Dimension,		mmHeight, 0);
EXTERN( Dimension,		mmWidth, 0);
EXTERN( unsigned long,		mmstate, 0);
EXTERN( Position,		offsetx, 0);
EXTERN( Position,		offsety, 0);

EXTERN( Dimension,		ppHeight, 0);
EXTERN( Dimension,		ppWidth, 0);
EXTERN( unsigned long,		ppstate, 0);
EXTERN( int,			start_or_terminate, 0);
EXTERN( Boolean,		wm_help_win_mapped, FALSE);
EXTERN( XContext,		wm_unique_context, 0);
/*EXTERN( unsigned long,		wm_focus_count, 0); */
#undef EXTERN
#endif /* _OL_EXTERN_H_ */
