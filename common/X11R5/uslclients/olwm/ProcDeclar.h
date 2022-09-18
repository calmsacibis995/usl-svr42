/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:ProcDeclar.h	1.41"
#endif

#ifndef _OL_PROCDECLAR_H_
#define _OL_PROCDECLAR_H_

/*
 ************************************************************************	
 * Description:
 *	This file contains the external procedure declarations.
 *
 *	Note, there is no need to include this file directly into
 *	source code since Extern.h includes it for us.
 *
 ************************************************************************	
 */

			/*
			 * Undeclared Toolkit Procedures
			 */

extern void	_XtRegisterWindow();
extern void	_XtUnregisterWindow();

			/*
			 * Window manipulation related procedures
			 */

extern void	DragWindowAround OL_ARGS((WMStepWidget, XEvent *,int));
extern void	LowerAIWGroup OL_ARGS((WMStepWidget, Window));
extern void	MoveWindow OL_ARGS((WMStepWidget, int));
extern void	ResizeWindow OL_ARGS((WMStepWidget, XEvent *, WMPiece));
extern void	RestackWindows OL_ARGS((Display *, WidgetBuffer *, int));
extern void	RaiseAIWGroup OL_ARGS((WMStepWidget, Window));
extern void	RaiseLowerGroup OL_ARGS((WMStepWidget, int));
extern void	Window_List_Used OL_ARGS((WMStepWidget));

			/*
			 * Menu related procedures
			 */
extern void	CheckPostedMenu OL_ARGS((WMStepWidget));
extern void	ConstructGroupList OL_ARGS((WMStepWidget, int, Boolean));
extern void	CreateGlobalMenus OL_NO_ARGS();
extern void	Menu OL_ARGS((WMStepWidget, XEvent *, WMPiece));
extern void	MenuBack OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuDismiss OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuDismissPopups OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuFullRestore OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuInitialize OL_ARGS((Widget));
extern void	Menu_Move OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuMotifMaximize OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuMotifMinimize OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuMotifRestore OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuOpenClose OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuOwner OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuQuit OL_ARGS((Widget, XtPointer, XtPointer));
extern void	MenuRefresh OL_ARGS((Widget, XtPointer, XtPointer));
extern void	Menu_Resize OL_ARGS((Widget, XtPointer, XtPointer));
extern void	UpdateMenuItems OL_ARGS((Widget));
extern void	MenuChooseDefault OL_ARGS((Widget, XtPointer, XtPointer));

			/*
			 * Icon related procedures
			 */
extern void		ConfirmIconPosition OL_ARGS((WMStepWidget,
							WMGeometry **));
extern void		ConsumeIconPosition OL_ARGS((WMStepWidget,
							WMGeometry **));
extern void		CreateDefaultIconMask OL_NO_ARGS();
extern void		CreateStepIcon OL_ARGS((WMStepWidget, WMGeometry *));
extern Boolean		DetermineMapPosFromXY OL_ARGS((Screen *, int, int,
							int *, int *));
extern void		DetermineXYFromMapPos OL_ARGS((Screen *, int, int,
							int *, int *));
extern void		DisplayMotifIcon OL_ARGS((WMStepWidget));
extern void		IconButtonPress OL_ARGS((Widget, XtPointer, 
				XEvent *, Boolean *));
extern void		IconExpose OL_ARGS((Widget, XtPointer,
					XEvent *, Boolean *));
#ifdef RAW
extern void		IconExposeRaw OL_ARGS((Widget, XtPointer, XEvent *,
					Boolean *));
#endif
extern WMGeometry *	IconPosition OL_ARGS((WMStepWidget, WMStepPart *));
extern void		InitIconDims OL_ARGS((Widget)); /* Motif */
extern void		InitIconPlacementStrategy OL_ARGS((Screen *));
extern void		InitMotifIDecorGCs OL_ARGS((Widget));
extern void		MoveAIW OL_ARGS((WMStepWidget));
			/* Motif icon map function */
extern void		MoveMapPosition OL_ARGS((WMStepWidget, int, int));
extern void		PackIcons OL_ARGS((Widget));
extern void		RecordPosition OL_ARGS((WMStepWidget, WMGeometry *));
extern void		RectObjExpose OL_ARGS((Widget, XtPointer,
					XEvent *, Boolean *));
extern void		ReleaseMapPosition OL_ARGS((int, int));
extern void		ResetIconPosition OL_ARGS((Screen *));

			/*
			 * Miscellaneous procedures
			 */
extern void	AddDeletePendingList OL_ARGS((Widget, Widget, Boolean));
extern int	CmapInWMList OL_ARGS((Colormap));
extern void	CreateMotifFeedbackWindow OL_ARGS((Display *, Screen *));
extern int	FindCurrentWindow OL_ARGS((Boolean));
extern Window	find_highest_group OL_ARGS((WMStepWidget,Window));
extern Cardinal	find_leader OL_ARGS((Window));
extern unsigned long	FocusCount OL_NO_ARGS();
extern void	GetWMHints OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetNormalHints OL_ARGS((WMStepPart *, Display *,
					Window, Screen *));
extern void	GetBusyState OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetAllDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetAddDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetDelDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetWindowName OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetIconName OL_ARGS((WMStepPart *, Display *, Window));
extern WMMenuButtonData *GetMWMMenu OL_ARGS((WMStepWidget));
extern int	GetWindowState OL_ARGS((Display *, Window));
extern int	IsPending OL_ARGS((Window));
extern int	IsWMStepChild OL_ARGS((Window));

extern int	MBorderX	OL_ARGS((WMStepWidget));
extern int	MBorderY	OL_ARGS((WMStepWidget));
extern int	MChildHeight	OL_ARGS((WMStepWidget));
extern int	MChildWidth	OL_ARGS((WMStepWidget));
extern int	MNewChildHeight OL_ARGS((WMStepWidget, int));
extern int	MNewChildWidth	OL_ARGS((WMStepWidget, int));
extern int	MOriginY	OL_ARGS((WMStepWidget));
extern int	MOriginX	OL_ARGS((WMStepWidget));
extern int 	MParentHeight	OL_ARGS((WMStepWidget, int));
extern int 	MParentWidth 	OL_ARGS((WMStepWidget, int));
extern void	MotifModeStartup OL_ARGS((Boolean));
extern String	NextToken OL_ARGS((String *));

extern int	OlQueryPointer OL_ARGS((Widget, Window *, Window *, int *,
					int *, int *, int *));
extern void	ParseResourceFile OL_NO_ARGS();
extern int	ReadBitmapFile OL_ARGS((Widget, String, Pixmap *));
extern Boolean	ReadyToDie OL_NO_ARGS();
extern void	ResetConstrainPoints OL_ARGS((XEvent *));

extern void	RemoveOldGrabs OL_ARGS((WMStepWidget));
extern void	ReparentMotifIconWindow OL_ARGS((Display *, WMStepPart *));
extern void	ResolveColors OL_ARGS((Pixel *, Pixel *,Pixel *));
extern void	RestartWindowMgr OL_ARGS((Display *));
extern int	ReworkColormapInfo OL_ARGS((Display *, Window, int));
extern void	SendWindowMovedEvent OL_ARGS((WMStepWidget));
extern void	SetColors OL_ARGS((WMStepWidget, Display *, Window));
extern void	SetMotifIconProperty OL_ARGS((Display *, Window));
extern void	SetMotifMins OL_NO_ARGS();
extern void	SetWindowState OL_ARGS((WMStepWidget, int, Window));
extern void	SetPushpinState OL_ARGS((WMStepWidget, unsigned long, int));
extern void	SetFocus OL_ARGS((Widget, int, int));
extern void	SetCurrent OL_ARGS((WMStepWidget));
extern void	WMClientDecorConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMClientFunctionsConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMIconDecorConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMIconWinDimConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMShowFeedbackConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMTransientDecorConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMTransientFunctionsConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMInstallColormaps OL_ARGS((WMStepWidget));

			/*
			 * Drawing related routines
			 */
extern void		CreateGC OL_ARGS((WMStepWidget, WMStepPart *, int));
extern void		DisplayWM OL_ARGS((WMStepWidget, XRectangle *));
extern void		DrawBN OL_ARGS((WMStepWidget));

extern void		DrawMotifMaximizeButton	OL_ARGS((Widget, int));
extern void		DrawMotifMinimizeButton	OL_ARGS((Widget, int));
extern void		DrawMotifMenuButton	OL_ARGS((Widget, int ));
extern void		DrawMotifResizeCorners	OL_ARGS((WMStepWidget));
extern void		DrawMotifTitleShadow	OL_ARGS((Widget, int ));

extern void		DrawSelectBorder OL_ARGS((WMStepWidget));
extern void		DrawStreaks OL_ARGS((WMStepWidget, int, int, int,
					int, int, int, int, int));
extern void		EraseBN OL_ARGS((WMStepWidget));
extern void		EraseSelectBorder OL_ARGS((WMStepWidget));
extern void		FlipPushpin OL_ARGS((WMStepWidget));
extern void		FlipMenuMark OL_ARGS((WMStepWidget));
extern WMMetrics *	GetMetrics OL_ARGS((WMStepWidget));
extern XRectangle	HeaderRect OL_ARGS((WMStepWidget));
extern void		ShowResizeFeedback OL_ARGS((WMStepWidget, WMPiece,
							int));

			/*
			 * Event handling related procedures
			 */
extern void	AddButtonGrabs OL_ARGS((Widget));
extern void	AddRemoveEventHandlers OL_ARGS((WMStepWidget, Boolean));
extern void	ClientFocusChange OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientNonMaskable OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientStructureNotify OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientSubstructureRedirect OL_ARGS((Widget, XtPointer,
						XEvent *, Boolean *));
extern void	MoveFromWithdrawn OL_ARGS((WMStepWidget));
extern void	MoveToWithdrawn OL_ARGS((WMStepWidget));
extern void	Select OL_ARGS((WMStepWidget, XEvent *, WMPiece));
extern void	StartWindowMappingProcess OL_ARGS((WMStepWidget));

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
			 * Other routines
			 */
extern void	AddWidgetToBuffer OL_ARGS((WMStepWidget, WidgetBuffer *));
extern void	RemoveWidgetFromBuffer OL_ARGS((WMStepWidget, WidgetBuffer *));
extern void	Button OL_ARGS((Widget, XEvent *, String *, Cardinal *));
extern void	ButtonUp OL_ARGS((Widget, XEvent *, String *, Cardinal *));
extern void	CreateHelpTree OL_ARGS((Display *));
extern void	FillHelpTitles OL_ARGS((Widget));
extern void	HandleHelpKeyPress OL_ARGS((Widget, XEvent *));
extern void	DestroyParent OL_ARGS((Widget, XtPointer, XtPointer));
extern WMPiece	EventIn OL_ARGS((WMStepWidget, Position, Position));
extern WMStepWidget NextFocus OL_ARGS((WMStepWidget, Boolean));
extern void	NewMenuItems OL_ARGS((WMStepWidget));
extern void		ReadProtocols OL_ARGS((WMStepWidget, Widget, Window));
extern void	SendLongMessage OL_ARGS((Display *, Window, Atom, unsigned long,
					unsigned long, unsigned long,
					unsigned long, unsigned long));
extern void	SendConfigureNotify OL_ARGS((Display *, Window, int, int,
					int, int, int));
extern void	SetHelpMsg OL_ARGS((Widget, XEvent*, char *, char *, char *));
extern void	SetSignals OL_ARGS((Display *, Window, Screen *));

extern void	SetupWindowManager();
extern void	StartWindowManager();
extern int	IgnoreClientErrors();
extern int	CatchRedirectErrors();
extern void	WMReparent();
extern void	GetWMDesktopHelp OL_ARGS((Display *, Window, XEvent *,
			char *, char *, char *));

#endif /* _OL_PROCDECLAR_H_ */
