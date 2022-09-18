/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Strings.h	1.24"
#endif

#ifndef _OL_STRINGS_H_
#define _OL_STRINGS_H_

/*
 ************************************************************************	
 * Description:
 *	This file contains the external strings declarations.
 *
 *	Note, there is no need to include this file directly into
 *	source code since Extern.h includes it for us.
 *
 ************************************************************************	
 */

#ifndef EXT_STRING
#define EXT_STRING(var,val)		extern char var[]
#endif

					/* define some new string types	*/

EXT_STRING( XtNpassKeys,		"passKeys");
EXT_STRING( XtCPassKeys,		"PassKeys");
EXT_STRING( XtNdoFork,			"doFork");
EXT_STRING( XtCDoFork,			"DoFork");
EXT_STRING( XtNpointerFocus,		"pointerFocus");
EXT_STRING( XtCPointerFocus,		"PointerFocus");
EXT_STRING( XtNsaveSet,			"saveSet");
EXT_STRING( XtCSaveSet,			"SaveSet");
EXT_STRING( XtNiconFont,		"iconFont");
EXT_STRING( XtCIconFont,		"IconFont");
EXT_STRING( XtNiconGridSize,		"iconGridSize");
EXT_STRING( XtCIconGridSize,		"IconGridSize");
EXT_STRING( XtNiconGrid,		"iconGrid");
EXT_STRING( XtCIconGrid,		"IconGrid");
EXT_STRING( XtNiconParentRelative,	"iconParentRelative");
EXT_STRING( XtCIconParentRelative,	"IconParentRelative");
EXT_STRING( XtNwindowAttributes,	"windowAttributes");
EXT_STRING( XtCWindowAttributes,	"WindowAttributes");
EXT_STRING( XtNwindowFrameColor,	"windowFrameColor");
EXT_STRING( XtCWindowFrameColor,	"WindowFrameColor");
EXT_STRING( XtNwarnings,		"warnings");
EXT_STRING( XtCWarnings,		"Warnings");

EXT_STRING( XtRWMIconGravity,		"WMIconGravity");

/* For window manager input event database (mouseless operations) */

EXT_STRING( XtNwmOpenCloseKey,		"wmOpenCloseKey");
EXT_STRING( XtNwmSizeKey,		"wmSizeKey");
EXT_STRING( XtNwmPropertiesKey,		"wmPropertiesKey");
EXT_STRING( XtNwmRefreshKey,		"wmRefreshKey");
EXT_STRING( XtNwmBackKey,		"wmBackKey");
EXT_STRING( XtNwmQuitKey,		"wmQuitKey");
EXT_STRING( XtNwmDismissThisKey,	"wmDismissThisKey");
EXT_STRING( XtNwmDismissAllKey,		"wmDismissAllKey");
EXT_STRING( XtNwmMoveKey,		"wmMoveKey");
EXT_STRING( XtNwmResizeKey,		"wmResizeKey");
EXT_STRING( XtNwmOwnerKey,		"wmOwnerKey");

EXT_STRING( OleCOlClientOlwmMsgs,	"olwm_msgs");

/* Error names and types */
/* 1st 2: Can't XInternAtom %s */
EXT_STRING( OleNbadAtom,		"BadAtom");
EXT_STRING( OleTinternAtom,		"internAtom");

/* Next 2: warning - same ? and ? colors given, resolving */
EXT_STRING( OleNcolor,			"color");
EXT_STRING( OleTduplicate,		"duplicate");

EXT_STRING( OleNdupColor,			"dupColor");
EXT_STRING( OleTforegroundBackground,		"foregroundBackground");
EXT_STRING( OleTforegroundInputWindowHeader,		"foregroundInputWindowHeader");
EXT_STRING( OleTbackgroundInputWindowHeader,		"backgroundInputWindowHeader");


EXT_STRING( OleTforeground,		"foreground");
EXT_STRING( OleTbackground,		"background");
EXT_STRING( OleTinputWindowHeader,	"inputwindowheader");
EXT_STRING( OleTborder,			"border");

/* For a "should-never-be-gotton" string in ClientFocusChange */
EXT_STRING( OleNfocus,			"focus");
EXT_STRING( OleTfocusNever,		"focusnever");

EXT_STRING( OleNcolormap,		"colormap");
EXT_STRING( OleTtruncate,		"truncate");

EXT_STRING( OleNspace,			"space");
EXT_STRING( OleTmemory,			"memory");
EXT_STRING( OleTexit,			"exit");

EXT_STRING( OleNbadWindowAttr,		"badattr");
EXT_STRING( OleTnotFound,		"notfound");

EXT_STRING( OleNbadBitmap,		"badbitmap");
EXT_STRING( OleTbadRead,		"badRead");

EXT_STRING( OleNnoTitle,		"noTitle");
EXT_STRING( OleTnoTitle,		"noTitle");

EXT_STRING( OleTwinColors,		"winColors");

EXT_STRING( OleNdimension,		"dimension");

EXT_STRING( OleNbadKeyboard,		"badKeyboard");
EXT_STRING( OleTnoGrab,			"noGrab");

EXT_STRING( OleNmenuLabel,		"mlabel");

EXT_STRING( OleNmove,			"move");
EXT_STRING( OleTmove,			"move");

EXT_STRING( OleNresize,			"resize");
EXT_STRING( OleTresize,			"resize");

EXT_STRING( OleNhelp,			"help");
EXT_STRING( OleTdismiss,		"dismiss");
EXT_STRING( OleTcancel,			"cancel");
EXT_STRING( OleTopen,			"open");
EXT_STRING( OleTclose,			"close");
EXT_STRING( OleTfull,			"full");
EXT_STRING( OleTrestore,		"restore");
EXT_STRING( OleTback,			"back");
EXT_STRING( OleTrefresh,		"refresh");
EXT_STRING( OleTquit,			"quit");
EXT_STRING( OleTthis,			"this");
EXT_STRING( OleTall,			"all");
EXT_STRING( OleTowner,			"owner");

EXT_STRING( OleTresizeCorner,		"resizeCorner");
EXT_STRING( OleTresizeHandles,		"resizeHandles");

EXT_STRING( OleTmenu,			"menu");
EXT_STRING( OleTpushpin,		"pushpin");
EXT_STRING( OleTmenuButton,		"menuButton");
EXT_STRING( OleTicon,			"icon");
EXT_STRING( OleThelp,			"help");

EXT_STRING( OleTwinMgr,			"winMgr");

EXT_STRING( OleNtitle,			"title");
EXT_STRING( OleTtitle,			"title");

/* Bad execvp() for olwm restart, in wmm.c */

EXT_STRING( OleNbadExec,		"badExec");
EXT_STRING( OleTexecvp,			"execvp");

EXT_STRING( OleNmnemonic,		"mnemonic");

EXT_STRING( OleTbadSpec,		"badSpec");
EXT_STRING( OleNaccelerator,		"accelr");

EXT_STRING( OleNgroup,		"group");

EXT_STRING( XtNpointerColormapFocus,	"ptrcmapfocus");
EXT_STRING( XtCPointerColormapFocus,	"Ptrcmapfocus");

EXT_STRING( OleNolwm,	"olwm");
EXT_STRING( OleTrunning,	"rng");

EXT_STRING(OleNiconDecor,	"icondecor");
EXT_STRING(OleTbadResSpec,	"badresspec");

EXT_STRING(OleNshowFeedback,	"showfdbk");
/* Here are strings used for motif style global appearance and behavior
 * resources.
 */
EXT_STRING(   XtNautoKeyFocus, "autoKeyFocus");
EXT_STRING(   XtCAutoKeyFocus, "AutoKeyFocus");

EXT_STRING(   XtNautoRaiseDelay, "autoRaiseDelay");
EXT_STRING(   XtCAutoRaiseDelay, "AutoRaiseDelay");

EXT_STRING(   XtNbitmapDirectory, "bitmapDirectory");
EXT_STRING(   XtCBitmapDirectory, "BitmapDirectory");

EXT_STRING(   XtNclientAutoPlace, "clientAutoPlace");
EXT_STRING(   XtCClientAutoPlace, "ClientAutoPlace");

EXT_STRING(   XtNdeiconifyKeyFocus, "deiconifyKeyFocus");
EXT_STRING(   XtCDeiconifyKeyFocus, "DeiconifyKeyFocus");

EXT_STRING(   XtNenforceKeyFocus, "enforceKeyFocus");
EXT_STRING(   XtCEnforceKeyFocus, "EnforceKeyFocus");

EXT_STRING(   XtNfocusAutoRaise, "focusAutoRaise");
EXT_STRING(   XtCFocusAutoRaise, "FocusAutoRaise");

EXT_STRING(   XtNframeBorderWidth, "frameBorderWidth");
EXT_STRING(   XtCFrameBorderWidth, "FrameBorderWidth");

EXT_STRING(   XtNiconAutoPlace, "iconAutoPlace");
EXT_STRING(   XtCIconAutoPlace, "IconAutoPlace");

EXT_STRING(   XtNiconClick, "iconClick");
EXT_STRING(   XtCIconClick, "IconClick");

EXT_STRING(   XtNiconDecoration, "iconDecoration");
EXT_STRING(   XtCIconDecoration, "IconDecoration");

EXT_STRING(   XtNiconImageMaximum, "iconImageMaximum");
EXT_STRING(   XtCIconImageMaximum, "IconImageMaximum");

EXT_STRING(   XtNiconImageMinimum, "iconImageMinimum");
EXT_STRING(   XtCIconImageMinimum, "IconImageMinimum");

EXT_STRING(   XtNiconPlacementMargin, "iconPlacementMargin");
EXT_STRING(   XtCIconPlacementMargin, "IconPlacementMargin");

EXT_STRING(   XtNinteractivePlacement, "interactivePlacement");
EXT_STRING(   XtCInteractivePlacement, "InteractivePlacement");

EXT_STRING(   XtNlimitResize, "limitResize");
EXT_STRING(   XtCLimitResize, "LimitResize");

EXT_STRING(   XtNlowerOnIconify, "lowerOnIconify");
EXT_STRING(   XtCLowerOnIconify, "LowerOnIconify");

EXT_STRING(   XtNmaximumMaximumSize, "maximumMaximumSize");
EXT_STRING(   XtCMaximumMaximumSize, "MaximumMaximumSize");

EXT_STRING(   XtNpositionIsFrame, "positionIsFrame");
EXT_STRING(   XtCPositionIsFrame, "PositionIsFrame");

EXT_STRING(   XtNpositionOnScreen, "positionOnScreen");
EXT_STRING(   XtCPositionOnScreen, "PositionOnScreen");

EXT_STRING(   XtNresizeBorderWidth, "resizeBorderWidth");
EXT_STRING(   XtCResizeBorderWidth, "ResizeBorderWidth");

EXT_STRING(   XtNresizeCursors, "resizeCursors");
EXT_STRING(   XtCResizeCursors, "ResizeCursors");

EXT_STRING(   XtNshowFeedback, "showFeedback");
EXT_STRING(   XtCShowFeedback, "ShowFeedback");
EXT_STRING(   XtRShowFeedback, "ShowFeedback");

EXT_STRING(   XtNstartupKeyFocus, "startupKeyFocus");
EXT_STRING(   XtCStartupKeyFocus, "StartupKeyFocus");

EXT_STRING(   XtNwMenuButtonClick, "wMenuButtonClick");
EXT_STRING(   XtCWMenuButtonClick, "WMenuButtonClick");

EXT_STRING(   XtNwMenuButtonClick2, "wMenuButtonClick2");
EXT_STRING(   XtCWMenuButtonClick2, "WMenuButtonClick2");

/* for IconWinDim struct, for use in a converter */
EXT_STRING(   XtRIconWinDim, "IconWinDim");
EXT_STRING(   XtRIDecor, "IDecor");

EXT_STRING(	XtNbottomShadowColor, "bottomShadowColor");

EXT_STRING(	XtNbottomShadowPixmap, "bottomShadowPixmap");

EXT_STRING(	XtNtopShadowColor, "topShadowColor");

EXT_STRING(	XtNfontList, "fontList");
EXT_STRING(	XtCFontList, "FontList");
EXT_STRING(	XtNactiveBackground, "activeBackground");
EXT_STRING(	XtNactiveBackgroundPixmap, "activeBackgroundPixmap");
EXT_STRING(	XtNactiveBottomShadowColor, "activeBottomShadowColor");
EXT_STRING(	XtNactiveBottomShadowPixmap, "activeBottomShadowPixmap");
EXT_STRING(	XtNactiveForeground, "activeForeground");
EXT_STRING(	XtNactiveTopShadowColor, "activeTopShadowColor");
EXT_STRING(	XtNactiveTopShadowPixmap, "activeTopShadowPixmap");
EXT_STRING(	XtNtopShadowPixmap, "topShadowPixmap");

EXT_STRING(  XtCActiveTopShadowPixmap, "ActiveTopShadowPixmap");
EXT_STRING(  XtCActiveBottomShadowPixmap, "ActiveBottomShadowPixmap");
EXT_STRING(  XtCTopShadowPixmap, "TopShadowPixmap");
EXT_STRING(  XtCBottomShadowPixmap, "BottomShadowPixmap");
EXT_STRING(  XtCBackgroundPixmap, "BackgroundPixmap");
EXT_STRING(  XtCActiveBackgroundPixmap, "ActiveBackgroundPixmap");


EXT_STRING( XtNclientDecoration, "clientDecoration");
EXT_STRING( XtCClientDecoration, "ClientDecoration");
EXT_STRING( XtRClientDecoration, "ClientDec");
EXT_STRING( XtNclientFunctions, "clientFunctions");
EXT_STRING( XtCClientFunctions, "ClientFunctions");
EXT_STRING( XtRClientFunctions, "ClientFunc");


EXT_STRING( XtNiconImage, "iconImage");
EXT_STRING( XtCIconImage, "IconImage");

EXT_STRING(   XtNiconImageBackground, "iconImageBackground");

EXT_STRING(   XtNiconImageBottomShadowColor, "iconImageBottomShadowColor");
EXT_STRING(   XtNiconImageBottomShadowPixmap, "iconImageBottomShadowPixmap");

EXT_STRING(   XtNiconImageForeground, "iconImageForeground");
EXT_STRING(   XtNiconImageTopShadowColor, "iconImageTopShadowColor");
EXT_STRING(   XtNiconImageTopShadowPixmap, "iconImageTopShadowPixmap");

EXT_STRING(   XtNmaximumClientSize, "maximumClientSize");
EXT_STRING(   XtCMaximumClientSize, "MaximumClientSize");

EXT_STRING(   XtNuseClientIcon, "useClientIcon");
EXT_STRING(   XtCUseClientIcon, "UseClientIcon");

EXT_STRING(   OleTminimize, "minimize");
EXT_STRING(   OleTmotifRestore, "motifrestore");
EXT_STRING(   OleTmaximize, "maximize");

EXT_STRING( XtNwmRestoreKey,	"restorekey");
EXT_STRING( XtNwmMinimizeKey,	"minimizekey");
EXT_STRING( XtNwmMaximizeKey,	"minimizekey");

EXT_STRING( XtNwindowMenu,	"windowMenu");
EXT_STRING( XtCWindowMenu,	"WindowMenu");

EXT_STRING( XtCIIBSP, "IconImageBottomShadowPixmap");
EXT_STRING( XtCIITSP, "IconImageTopShadowPixmap");


EXT_STRING( XtNbuttonBindings, "buttonBindings");
EXT_STRING( XtCButtonBindings, "ButtonBindings");

EXT_STRING( XtNcleanText, "cleanText");
EXT_STRING( XtCCleanText, "CleanText");

EXT_STRING( XtNcolormapFocusPolicy, "colormapFocusPolicy");
EXT_STRING( XtCColormapFocusPolicy, "ColormapFocusPolicy");

EXT_STRING( XtNconfigFile, "configFile");
EXT_STRING( XtCConfigFile, "ConfigFile");

EXT_STRING( XtNdoubleClickTime, "doubleClickTime");
EXT_STRING( XtCDoubleClickTime, "DoubleClickTime");


EXT_STRING( XtNenableWarp, "enableWarp");
EXT_STRING( XtCEnableWarp, "EnableWarp");



EXT_STRING( XtNiconPlacement, "iconPlacement");
EXT_STRING( XtCIconPlacement, "IconPlacement");



EXT_STRING( XtNkeyBindings, "keyBindings");
EXT_STRING( XtCKeyBindings, "KeyBindings");

EXT_STRING( XtNkeyboardFocusPolicy, "keyboardFocusPolicy");
EXT_STRING( XtCKeyboardFocusPolicy, "KeyboardFocusPolicy");



EXT_STRING( XtNmoveThreshold, "moveThreshold");
EXT_STRING( XtCMoveThreshold, "MoveThreshold");

EXT_STRING( XtNpassSelectButton, "passSelectButton");
EXT_STRING( XtCPassSelectButton, "PassSelectButton");

EXT_STRING( XtNpassButtons, "passButtons");
EXT_STRING( XtCPassButtons, "PassButtons");

EXT_STRING( XtNmultiScreen, "multiScreen");
EXT_STRING( XtCMultiScreen, "MultiScreen");

EXT_STRING( XtNquitTimeout, "quitTimeout");
EXT_STRING( XtCQuitTimeout, "QuitTimeout");


EXT_STRING( XtNraiseKeyFocus, "raiseKeyFocus");
EXT_STRING( XtCRaiseKeyFocus, "RaiseKeyFocus");

EXT_STRING( XtNscreens, "screens");
EXT_STRING( XtCScreens, "Screens");

EXT_STRING( XtNtransientDecoration, "transientDecoration");
EXT_STRING( XtCTransientDecoration, "TransientDecoration");
EXT_STRING( XtRTransientDecoration, "RTransientDecoration");

EXT_STRING( XtNtransientFunctions, "transientFunctions");
EXT_STRING( XtCTransientFunctions, "TransientFunctions");
EXT_STRING( XtRTransientFunctions, "RTransientFunctions");

EXT_STRING( OleNclientDecorations, "clientDecorations");
EXT_STRING( OleNclientFunctions, "clientFunctions");

EXT_STRING( OleTpopupWindowMenu, "popupWindowMenu");

EXT_STRING( OleTmotifMaximizeButton, "motifMaximizeButton");
EXT_STRING( OleTmotifMinimizeButton, "motifMinimizeButton");

#endif /* _OL_STRINGS_H_ */
