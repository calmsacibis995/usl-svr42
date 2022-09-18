/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:MotifRes.c	1.8"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the code used to support the Motif mode
 *	resources.
 *
 ******************************file*header********************************
 */

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/Flat.h>

#include <wm.h>
#include <WMStepP.h>

#include <Extern.h>
#include <limits.h>


extern void AssignCSGCs OL_ARGS((WMStepWidget));
static void ResolveCompConflicts OL_ARGS((MotCompAppearanceInfo *));
extern void ResolveMotifGlobalResources OL_ARGS((Widget));
extern void ResolveMotifComponentResources OL_ARGS((Widget));
extern void ResolveMotifCSResources OL_ARGS((Widget));
static void ConvertToIconWinDimString OL_ARGS((String, int, int));
extern void GetCSResources OL_ARGS((WMStepWidget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static char dbname[20];
static char resource_str[100];
static char class_str[100];

/*
 * Mwm "Global" resources.
 */

static MotWMResources	motwmrcsRec;
MotWMResources		*motwmrcs = &motwmrcsRec;

#define OFFSET(f)	XtOffsetOf(MotWMResources, f)

	/* motif_global_resources - these are global appearance and
	 * behavior resources common in mwm. (Incomplete list, 12/20/91)
	 * Amended 1/29/92.
	 */
XtResource	motif_global_resources[] = {
   { XtNautoKeyFocus, XtCAutoKeyFocus, XtRBoolean, sizeof(Boolean),
     OFFSET(autoKeyFocus), XtRImmediate, (XtPointer)1},
   /* Used in real-estate focus only, when focusAutoRaise is True */
   { XtNautoRaiseDelay, XtCAutoRaiseDelay, XtRInt, sizeof(int),
     OFFSET(autoRaiseDelay), XtRImmediate, (XtPointer)500},

   { XtNbitmapDirectory, XtCBitmapDirectory, XtRString, sizeof(String),
     OFFSET(bitmapDirectory), XtRImmediate, 
    (XtPointer)"/usr/include/X11/bitmaps" },
   { XtNbuttonBindings, XtCButtonBindings, XtRString, sizeof(String),
     OFFSET(buttonBindings), XtRImmediate, 
    (XtPointer)"DefaultButtonBindings" },

   { XtNcleanText, XtCCleanText, XtRBoolean, sizeof(Boolean),
     OFFSET(cleanText), XtRImmediate, (XtPointer)1},

   { XtNclientAutoPlace, XtCClientAutoPlace, XtRBoolean, sizeof(Boolean),
     OFFSET(clientAutoPlace), XtRImmediate, (XtPointer)1},

	/* Add converter for string to cmap focus policy, but default is
	 * an int.
	 */
   { XtNcolormapFocusPolicy, XtCColormapFocusPolicy, XtRString,
	sizeof(String),
     OFFSET(colormapFocusPolicy), XtRImmediate,
	(XtPointer)CMAP_POLICY_EXPLICIT},

   { XtNconfigFile, XtCConfigFile, XtRString, sizeof(String),
     OFFSET(configFile), XtRImmediate, (XtPointer)".mwmrc"},

   { XtNdeiconifyKeyFocus, XtCDeiconifyKeyFocus, XtRBoolean, sizeof(Boolean),
     OFFSET(deiconifyKeyFocus), XtRImmediate, (XtPointer)1},

   { XtNdoubleClickTime, XtCDoubleClickTime, XtRInt, sizeof(int),
     OFFSET(doubleClickTime), XtRImmediate, (XtPointer)3},

   { XtNenableWarp, XtCEnableWarp, XtRBoolean, sizeof(Boolean),
     OFFSET(enableWarp), XtRImmediate, (XtPointer)1},

   { XtNenforceKeyFocus, XtCEnforceKeyFocus, XtRBoolean, sizeof(Boolean),
     OFFSET(enforceKeyFocus), XtRImmediate, (XtPointer)1},

   /* focusAutoRaise - meaningful only in real-estate focus */
   { XtNfocusAutoRaise, XtCFocusAutoRaise, XtRBoolean, sizeof(Boolean),
     OFFSET(focusAutoRaise), XtRImmediate, (XtPointer)1 },

   /* border width (no resize corners) is in pixels */
   { XtNframeBorderWidth, XtCFrameBorderWidth, XtRInt, sizeof(int),
     OFFSET(frameBorderWidth), XtRImmediate, (XtPointer)5 },

   { XtNiconAutoPlace, XtCIconAutoPlace, XtRBoolean, sizeof(Boolean),
     OFFSET(iconAutoPlace), XtRImmediate, (XtPointer)1 },

   { XtNiconClick, XtCIconClick, XtRBoolean, sizeof(Boolean),
     OFFSET(iconClick), XtRImmediate, (XtPointer)1 },

    /* Need converter to convert to an int - if specify "label", only get
     * a label (fixed size) for the icon.  If supply image, only get an
     * image for the icon (no label at all).  If you want a label when the
     * icon is active, then specify "activelabel".
     * Ex)	activelabel image - gets you an image only when it's not active,
     *	and an image and a non-truncated label when it's active.
     *		image - gets you an image (no label even when active, only
     * the border of the icon changes color).
     *		label - gets you a label only, no image, and fixed size (not
     * extended when active, but filled in with the active color).
     *		image label - combine the two (fixed size label at all times)
     *		activelabel label image - default.
     *		activelabel - by itself, just get the default, because what
     * else can you do???
     *		activelabel label - get regular labels (fixed), extended when
     *  active.
     *		
     * (non-truncated)
     * label, you must qualify the "label" by saying "activelabel label"-
     * the word "activelabel" is a qualifier for label, that's it - alone,
     * it means nothing.
     */

   { XtNiconDecoration, XtCIconDecoration, XtRIDecor, sizeof(int),
     OFFSET(iconDecoration), XtRImmediate,
    (XtPointer)(ICON_ACTIVELABEL|ICON_LABEL|ICON_IMAGE) },

   { XtNiconImageMaximum, XtCIconImageMaximum, XtRIconWinDim,
     sizeof(IconWinDim),
     OFFSET(iconImageMaximum), XtRString, (XtPointer)"50x50" },
   { XtNiconImageMinimum, XtCIconImageMinimum, XtRIconWinDim,
     sizeof(IconWinDim),
     OFFSET(iconImageMinimum), XtRString, (XtPointer)"16x16" },

	/* Define ICONLEFT, ICONBOTOM in wm.h */
   { XtNiconPlacement, XtCIconPlacement, XtRString, sizeof(String),
     OFFSET(iconPlacement), XtRImmediate, (XtPointer)(ICONLEFT|ICONBOTTOM)},
   /* Placement margin - should be >0 - default equals the space between
    * icons on the screen - will be maximized to have greatest number of
    * icons in each row/col.
    */
   { XtNiconPlacementMargin, XtCIconPlacementMargin, XtRInt, sizeof(int),
     OFFSET(iconPlacementMargin), XtRImmediate, (XtPointer)-1 },

   { XtNinteractivePlacement, XtCInteractivePlacement, XtRBoolean,
     sizeof(Boolean), OFFSET(interactivePlacement), XtRImmediate,
     (XtPointer)False },

   { XtNkeyBindings, XtCKeyBindings, XtRString, sizeof(String),
     OFFSET(keyBindings), XtRImmediate, (XtPointer)"DefaultKeyBindings"},

   { XtNkeyboardFocusPolicy, XtCKeyboardFocusPolicy, XtRString, sizeof(String),
     OFFSET(keyboardFocusPolicy),
	XtRImmediate, (XtPointer)KEYBOARD_EXPLICIT},

   { XtNlimitResize, XtCLimitResize, XtRBoolean, sizeof(Boolean),
     OFFSET(limitResize), XtRImmediate, (XtPointer)False},

   { XtNlowerOnIconify, XtCLowerOnIconify, XtRBoolean, sizeof(Boolean),
     OFFSET(lowerOnIconify), XtRImmediate, (XtPointer)True},

   { XtNmaximumMaximumSize, XtCMaximumMaximumSize, XtRIconWinDim,
     sizeof(IconWinDim),
     OFFSET(maximumMaximumSize), XtRString, (XtPointer)NULL},

   { XtNmoveThreshold, XtCMoveThreshold, XtRDimension, sizeof(Dimension),
     OFFSET(moveThreshold), XtRImmediate, (XtPointer)4},

   { XtNmultiScreen, XtCMultiScreen, XtRBoolean, sizeof(Boolean),
     OFFSET(multiScreen), XtRImmediate, (XtPointer)False},

   { XtNpassButtons, XtCPassButtons, XtRBoolean, sizeof(Boolean),
     OFFSET(passButtons), XtRImmediate, (XtPointer)False},
   { XtNpassSelectButton, XtCPassSelectButton, XtRBoolean, sizeof(Boolean),
     OFFSET(passSelectButton), XtRImmediate, (XtPointer)True},

   { XtNpositionIsFrame, XtCPositionIsFrame, XtRBoolean, sizeof(Boolean),
     OFFSET(positionIsFrame), XtRImmediate, (XtPointer)1},

   { XtNpositionOnScreen, XtCPositionOnScreen, XtRBoolean, sizeof(Boolean),
     OFFSET(positionOnScreen), XtRImmediate, (XtPointer)1},

   { XtNquitTimeout, XtCQuitTimeout, XtRDimension, sizeof(Dimension),
     OFFSET(quitTimeout), XtRImmediate, (XtPointer)1000},

   { XtNraiseKeyFocus, XtCRaiseKeyFocus, XtRBoolean, sizeof(Boolean),
     OFFSET(raiseKeyFocus), XtRImmediate, (XtPointer)False},

   { XtNresizeBorderWidth, XtCResizeBorderWidth, XtRInt, sizeof(int),
     OFFSET(resizeBorderWidth), XtRImmediate, (XtPointer)ULONG_MAX}, /* pixels */

   { XtNresizeCursors, XtCResizeCursors, XtRBoolean, sizeof(Boolean),
     OFFSET(resizeCursors), XtRImmediate, (XtPointer)1},

   { XtNscreens, XtCScreens, XtRString, sizeof(String),
     OFFSET(screens), XtRImmediate, (XtPointer)NULL},

   /* XtNshowFeedback is tough - possible values are: all, none, or a comb.
    * of "placement resize behavior restart move kill quit"
    * Default is all; start with move and resize.
    */
   { XtNshowFeedback, XtCShowFeedback, XtRShowFeedback, sizeof(int),
     OFFSET(showFeedback), XtRImmediate, (XtPointer)(FEED_MOVE|FEED_RESIZE) },

   { XtNstartupKeyFocus, XtCStartupKeyFocus, XtRBoolean, sizeof(Boolean),
     OFFSET(startupKeyFocus), XtRImmediate, (XtPointer) 1},

   { XtNtransientDecoration, XtCTransientDecoration, XtRClientDecoration,
	sizeof(unsigned int),
     OFFSET(transientDecoration), XtRImmediate, (XtPointer) ULONG_MAX},

   { XtNtransientFunctions, XtCTransientFunctions, XtRClientFunctions,
	sizeof(unsigned int),
     OFFSET(transientFunctions), XtRImmediate, (XtPointer) ULONG_MAX},

     /* menubuttonclick - if true, then a mouse CLICK over the menu button
      * posts the menu.
      */
   { XtNwMenuButtonClick, XtCWMenuButtonClick, XtRBoolean, sizeof(Boolean),
     OFFSET(wMenuButtonClick), XtRImmediate, (XtPointer) 1},

    /* click2 - if true, do f.kill for the window */
   { XtNwMenuButtonClick2, XtCWMenuButtonClick2, XtRBoolean, sizeof(Boolean),
     OFFSET(wMenuButtonClick2), XtRImmediate, (XtPointer) 1},
}; /* motif_global_resources */
#undef OFFSET

Cardinal num_motif_global_resources = XtNumber(motif_global_resources);


/*
 *************************************************************************
 * ResolveMotifGlobalResources
 ****************************procedure*header*****************************
 */

extern void
ResolveMotifGlobalResources OLARGLIST((Frame))
OLGRA(Widget, Frame)
{
Display *display = XtDisplay(Frame);
int pos;

	XtGetApplicationResources(Frame, (XtPointer)motwmrcs,
		motif_global_resources, num_motif_global_resources, NULL, 0);

	/* Now have compiled resources - the values returned are returned
	 * by XtGetApplicationResources.
	 */

/*
	ConvertToIconWinDimString(str1, motwmrcs->iconImageMaximum.width,
			motwmrcs->iconImageMaximum.height);
	ConvertToIconWinDimString(str2, motwmrcs->iconImageMinimum.width,
			motwmrcs->iconImageMinimum.height);
	ConvertToIconWinDimString(str3, motwmrcs->maximumMaximumSize.width,
			motwmrcs->maximumMaximumSize.height);
	motif_global_resources[ICONIMAGEMAX_INDEX].default_addr =
			(XtPointer)str1;
	motif_global_resources[ICONIMAGEMIN_INDEX].default_addr =
			(XtPointer)str2;
	motif_global_resources[MAXMAXSIZE_INDEX].default_addr =
			(XtPointer)str3;
 */

	pos = DisplayWidth(display, DefaultScreen(display));

	if(motwmrcs->resizeBorderWidth == ULONG_MAX)	
		if(pos >1000)
			motwmrcs->resizeBorderWidth = 10;
		else if (pos >700)
			motwmrcs->resizeBorderWidth = 7;
		else
			motwmrcs->resizeBorderWidth = 5;

/*
	XtDestroyWidget(Frame);
 */
} /* ResolveMotifGlobalResources */


/* Struct for the motif mode "component" resources -
 * for the window manager parts.  
 *
 * For the topShadowColor, bottomShadowColor, activeTopShadowColor,
 * and activeBottomShadowColor, set default to ULONG_MAX (because
 * a Pixel is an unsigned long), and if none is supplied by the user,
 * then use the default GC (bg0, bg3). If the top is supplied and not
 * the bottom, or vice-versa, then use the defaults for both.
 * Same goes for clientspecific resources below.
 */

/*
#define NUM_MOTIF_COMPONENTS	5
 */

/* Each defined number represents a position in the component resource
 * array of values.
 */
/*
#define CLIENT_COMP	0
#define ICON_COMP	1
#define TITLE_COMP	2
#define MENU_COMP	3
#define FEEDBACK_COMP	4
 */

#define OFFSET(f)	XtOffsetOf(MotCompAppearanceInfo, f)
#define FONTLIST	4

static XtResource	motif_comp_resources[] = {
   { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
     OFFSET(background), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNbackgroundPixmap, XtCBackgroundPixmap, XtRPixmap, sizeof(Pixmap),
     OFFSET(backgroundPixmap), XtRImmediate, (XtPointer)NULL},
   { XtNbottomShadowColor, XtCForeground, XtRPixel, sizeof(Pixel),
     OFFSET(bottomShadowColor), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNbottomShadowPixmap, XtCBottomShadowPixmap, XtRPixmap, sizeof(Pixmap),
     OFFSET(bottomShadowPixmap), XtRImmediate, (XtPointer)NULL},

/* Motif calls it the fontList resource, we call it fontGroup.  I'll use
 * XtCFontList in case we pick it up in an app-defaults file
 */
   { XtNfontList, XtCFontList, XtROlFontList, sizeof(OlFontList *),
     OFFSET(fontList), XtRImmediate, (XtPointer)NULL},
/*
   { XtNfont, XtCFont, XtRFont, sizeof(XFontStruct *),
     OFFSET(font), XtRImmediate, (XtPointer)NULL},
 */

   { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
     OFFSET(foreground), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNsaveUnder, XtCSaveUnder, XtRBoolean, sizeof(Boolean),
     OFFSET(saveUnder), XtRImmediate, (XtPointer)False},
   { XtNtopShadowColor, XtCBackground, XtRPixel, sizeof(Pixel),
     OFFSET(topShadowColor), XtRImmediate, (XtPointer)ULONG_MAX},
/* mlp - ***** For the pixmap resources, we will check the depth
 * of the pixmap - if it is > 1, then we will treat is as a real
 * pixmap, and the topShadowColor and bottomShadowColor will be
 * ignored; if the depth is 1, then it is a real bitmap, and the
 *  other shadow colors will not be ignored, but will be used
 * as mwm does.
 */
   { XtNtopShadowPixmap, XtCActiveTopShadowPixmap, XtRPixmap, sizeof(Pixmap),
     OFFSET(topShadowPixmap), XtRImmediate, (XtPointer)NULL},

		/* Following resources apply to icons and window frames */

   { XtNactiveBackground, XtCBackground, XtRPixel, sizeof(Pixel),
     OFFSET(activeBackground), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNactiveBackgroundPixmap, XtCActiveBackgroundPixmap, XtRPixmap,
	sizeof(Pixmap),
     OFFSET(activeBackgroundPixmap), XtRImmediate, (XtPointer)NULL},
   { XtNactiveBottomShadowColor, XtCForeground, XtRPixel, sizeof(Pixel),
     OFFSET(activeBottomShadowColor), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNactiveBottomShadowPixmap, XtCActiveBottomShadowPixmap, XtRPixmap,
	sizeof(Pixmap),
     OFFSET(activeBottomShadowPixmap), XtRImmediate, (XtPointer)NULL},
   { XtNactiveForeground, XtCForeground, XtRPixel, sizeof(Pixel),
     OFFSET(activeForeground), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNactiveTopShadowColor, XtCBackground, XtRPixel, sizeof(Pixel),
     OFFSET(activeTopShadowColor), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNactiveTopShadowPixmap, XtCActiveTopShadowPixmap, XtRPixmap,
	sizeof(Pixmap),
     OFFSET(activeTopShadowPixmap), XtRImmediate, (XtPointer)NULL},
}; /* motif_comp_resources */

#undef OFFSET

MotifCompResourceInfo	motCompRes[NUM_MOTIF_COMPONENTS];

/*
static MotWMCompResources	motwmcomprcsRec;
MotWMCompResources		*motcomprcs = &motwmcomprcsRec;
 */
static MotCompAppearanceInfo	motwmcomprcsRec;
MotCompAppearanceInfo		*motcomprcs = &motwmcomprcsRec;

/* Very important - this is a component resource record that will contain the
 * default pointers and values
 */
static MotCompAppearanceInfo	motcomprcsDefs;

Cardinal num_motif_comp_resources = XtNumber(motif_comp_resources);

#define NUM_COMP_STRINGS	16

static char *compres_str[NUM_COMP_STRINGS] = {
"background",
"backgroundPixmap",
"bottomShadowColor",
"bottomShadowPixmap",
"fontList",
"foreground",
"saveUnder",
"topShadowColor",
"topShadowPixmap",
/* icons, windows only */
"activeBackground",
"activeBackgroundPixmap",
"activeBottomShadowColor",
"activeBottomShadowPixmap",
"activeForeground",
"activeTopShadowColor",
"activeTopShadowPixmap",
};

static char *compclass_str[NUM_COMP_STRINGS] = {
"Background",
"BackgroundPixmap",
"BottomShadowColor",
"BottomShadowPixmap",
"FontList",
"Foreground",
"SaveUnder",
"TopShadowColor",
"TopShadowPixmap",
/* icons, windows only */
"ActiveBackground",
"ActiveBackgroundPixmap",
"ActiveBottomShadowColor",
"ActiveBottomShadowPixmap",
"ActiveForeground",
"ActiveTopShadowColor",
"ActiveTopShadowPixmap",
};


/*
 *************************************************************************
 * ResolveMotifComponentResources.
 * To review:
 * 	[Mwm]*client*resource
 *	[Mwm]*client*title*resource
 *	[Mwm]*icon*resource
 *	[Mwm]*feedback*resource
 *	[Mwm]*menu*resource
 *	
 ****************************procedure*header*****************************
 */
void
ResolveMotifComponentResources OLARGLIST((Frame))
OLGRA(Widget, Frame)
{
Display *display = XtDisplay(Frame);
XrmQuark name, class;
char *return_type;
XrmDatabase db;
XrmValue value, toVal;
unsigned long	toVal_addr;
Boolean		BooltoVal_addr;
OlFontList	*FLtoVal_addr;
Pixmap		PixtoVal_addr;
int		i, k;
Screen		*screen = XtScreen(Frame);

#define NUMCOMPS	16

Arg	args[NUMCOMPS];
Cardinal	numargs;

caddr_t comps[NUMCOMPS] =  {
      (caddr_t) &(motwmcomprcsRec.background),
      (caddr_t) &(motwmcomprcsRec.backgroundPixmap),
      (caddr_t) &(motwmcomprcsRec.bottomShadowColor),
      (caddr_t) &(motwmcomprcsRec.bottomShadowPixmap),
      (caddr_t) &(motwmcomprcsRec.fontList),
      (caddr_t) &(motwmcomprcsRec.foreground),
      (caddr_t) &(motwmcomprcsRec.saveUnder),
      (caddr_t) &(motwmcomprcsRec.topShadowColor),
      (caddr_t) &(motwmcomprcsRec.topShadowPixmap),
/* icons, windows only */
      (caddr_t) &(motwmcomprcsRec.activeBackground),
      (caddr_t) &(motwmcomprcsRec.activeBackgroundPixmap),
      (caddr_t) &(motwmcomprcsRec.activeBottomShadowColor),
      (caddr_t) &(motwmcomprcsRec.activeBottomShadowPixmap),
      (caddr_t) &(motwmcomprcsRec.activeForeground),
      (caddr_t) &(motwmcomprcsRec.activeTopShadowColor),
      (caddr_t) &(motwmcomprcsRec.activeTopShadowPixmap),
}; /* comps pointer declaration */

caddr_t defaultcomps[NUMCOMPS] =  {
      (caddr_t) &(motcomprcsDefs.background),
      (caddr_t) &(motcomprcsDefs.backgroundPixmap),
      (caddr_t) &(motcomprcsDefs.bottomShadowColor),
      (caddr_t) &(motcomprcsDefs.bottomShadowPixmap),
      (caddr_t) &(motcomprcsDefs.fontList),
      (caddr_t) &(motcomprcsDefs.foreground),
      (caddr_t) &(motcomprcsDefs.saveUnder),
      (caddr_t) &(motcomprcsDefs.topShadowColor),
      (caddr_t) &(motcomprcsDefs.topShadowPixmap),
/* icons, windows only */
      (caddr_t) &(motcomprcsDefs.activeBackground),
      (caddr_t) &(motcomprcsDefs.activeBackgroundPixmap),
      (caddr_t) &(motcomprcsDefs.activeBottomShadowColor),
      (caddr_t) &(motcomprcsDefs.activeBottomShadowPixmap),
      (caddr_t) &(motcomprcsDefs.activeForeground),
      (caddr_t) &(motcomprcsDefs.activeTopShadowColor),
      (caddr_t) &(motcomprcsDefs.activeTopShadowPixmap),
}; /* defaultcomps pointer declaration */

caddr_t titlecomps[NUMCOMPS] =  {
      (caddr_t) &(motCompRes[TITLE_COMP].compai.background),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.backgroundPixmap),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.bottomShadowColor),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.bottomShadowPixmap),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.fontList),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.foreground),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.saveUnder),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.topShadowColor),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.topShadowPixmap),
/* icons, windows only */
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeBackground),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeBackgroundPixmap),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeBottomShadowColor),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeBottomShadowPixmap),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeForeground),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeTopShadowColor),
      (caddr_t) &(motCompRes[TITLE_COMP].compai.activeTopShadowPixmap),
}; /* titlecomps pointer declaration */

Pixel	white = WhitePixelOfScreen(screen),
	black = BlackPixelOfScreen(screen);

	db = XtDatabase(display);

	/* Get *resource, and Mwm*resource.  Place the default resource
	 * values in the record pointed to by motcomprcs.  We want to
	 * save these defaults/ptrs to them, because if we don't get
	 * any more specific, then we want to use them all the way down.
	 */
/*
	GetNewWidget(display);
 */
	XtGetApplicationResources(Frame, (XtPointer)motcomprcs,
		motif_comp_resources, num_motif_comp_resources, NULL, 0);

	if (motcomprcs->foreground == ULONG_MAX)
		motcomprcs->foreground = wmrcs->foregroundColor;
	if (motcomprcs->activeForeground == ULONG_MAX)
		motcomprcs->activeForeground = motcomprcs->foreground;

	if (motcomprcs->background == ULONG_MAX)
		motcomprcs->background = wmrcs->backgroundColor;
	
	if (motcomprcs->activeBackground == ULONG_MAX)
		motcomprcs->activeBackground = wmrcs->inputFocusColor;

	/* What to do about inputFocusColor??? Try this for the momemt */
	if (motcomprcs->activeBackground != wmrcs->inputFocusColor)
		/* Same, so use inputFocusColor */
		motcomprcs->activeBackground = wmrcs->inputFocusColor;

	ResolveCompConflicts(motcomprcs);

	/* Two more */
	if (motcomprcs->activeTopShadowColor == ULONG_MAX)
		motcomprcs->activeTopShadowColor = motcomprcs->background;
	if (motcomprcs->activeBottomShadowColor == ULONG_MAX)
		motcomprcs->activeBottomShadowColor = motcomprcs->foreground;
	if (motcomprcs->activeTopShadowColor ==
				motcomprcs->activeBottomShadowColor) {
		motcomprcs->activeTopShadowColor = white;
		motcomprcs->activeBottomShadowColor = black;
	}

	/*    We now have motcomprcs filled with anything looking like
	 * Mwm*resource, or *resource, it there is a match.
	 *
	 * Next, copy the values JUST RETURNED , back into
	 * the default_addr field of the XtResource record.  These will become
	 * the new defaults for further nested calls.
	 */

	for (i=0; i < num_motif_comp_resources; i++) {
		/* Set the default_addr field to equal the returned value from
		 * the call.  That's what the comps array points to.
		 * The first time through, WE SHOULD SAVE the values for
		 * later, because they may get lost in intermediate
		 * calculations.  But how can they be easily saved?
		 * Most of them are strings - only the ptrs can be easily
		 * saved, and therefore the actual values may get lost.
		 */
		motif_comp_resources[i].default_addr =
			(XtPointer) *(comps[i]);
	}


	/* Now save the "original" resource values - ACTUALLY, we can
	 * use these original resource values as defaults when we later
	 * do the "window", icon, and feedback (and title), BUT we'd
	 * have to manually copy them into the XtResource array (the values,
	 * that is).  By that I mean we'd have to individually assign them all
	 * one at a time.  May be a possibility, rather than have 5 diff.
	 * XtResource arrays as my comment below says.
	 */
	/* Problem is we should really copy in the value here, because if
	 * we have any pointers, then they get wiped out by successive
	 * calls.  For example, if the backgroundPixmap is set to a ptr to
	 * a string, then we are just copying the pointers - but if the
	 * pointer changes for another class of component, such as feedback,
	 * but we are the client class, then we want to conserve the returned
	 * value for client because the pointer would be lost when the feedback
 	 * ptr changes to another string - a string we don't want for client.
	 * First, save the original pointers/values in a safe haven.
	 */
	  memcpy( (MotCompAppearanceInfo *) &(motcomprcsDefs),
		(MotCompAppearanceInfo *) &(motwmcomprcsRec),
		sizeof(MotCompAppearanceInfo));			
	for (i=0; i<5; i++)
	  memcpy( (MotCompAppearanceInfo *) (&(motCompRes[i].compai)),
		(MotCompAppearanceInfo *) &(motcomprcsDefs),
		sizeof(MotCompAppearanceInfo));			

	/* Next, get Mwm*client*resource  - use XtGetSubResources.  After,
	 * get any full pathnames - Mwm*client.  Then repeat this 2 step
	 * process for icon, menu, and feedback - title should be fun...
	 * I wonder if it would just be easier to have 5 different
	 * XtResource arrays, one for each - the ones for the menu and
	 * feedback would be somewhat shorter than the others.
	 */

	XtGetSubresources(Frame, (XtPointer)&(motCompRes[CLIENT_COMP].compai),
		"client", "Client", motif_comp_resources,
		num_motif_comp_resources, NULL, 0);

	/* adjust */
	if (motCompRes[CLIENT_COMP].compai.activeBackground ==
					motCompRes[CLIENT_COMP].compai.background)
		/* Same, so use inputFocusColor */
		motCompRes[CLIENT_COMP].compai.activeBackground = wmrcs->inputFocusColor;

	ResolveCompConflicts(&(motCompRes[CLIENT_COMP].compai));

	XtGetSubresources(Frame, (XtPointer)&(motCompRes[ICON_COMP].compai),
		"icon", "Icon", motif_comp_resources,
		num_motif_comp_resources, NULL, 0);

	if (motCompRes[ICON_COMP].compai.activeBackground ==
					motCompRes[ICON_COMP].compai.background)
		/* Same, so use inputFocusColor */
		motCompRes[ICON_COMP].compai.activeBackground = wmrcs->inputFocusColor;

	ResolveCompConflicts(&(motCompRes[ICON_COMP].compai));

	XtGetSubresources(Frame, (XtPointer)&(motCompRes[MENU_COMP].compai),
		"menu", "Menu", motif_comp_resources,
		num_motif_comp_resources, NULL, 0);

	XtGetSubresources(Frame,
			(XtPointer)&(motCompRes[FEEDBACK_COMP].compai),
			"feedback", "feedback", motif_comp_resources,
			num_motif_comp_resources, NULL, 0);

	/* One more: title component.
	 * Mwm*client*title*resource_id.
	 * (Actually, 2 more, but the other is Mwm*menu_name*resource,
	 * that must be done after we have menu names to query.)
	 *
	 */
	strcpy(resource_str, "mwm.client.title.");
	strcpy(class_str, "Mwm.Client.Title.");
	k = strlen(resource_str);
	numargs = 0;
	/* Start copying at position 16 (k) */
	for (i=0; i < 16; i++) {
		/* First get the full path of the resource */
		resource_str[k] = class_str[k] = (char) '\0';
		strcat(resource_str, compres_str[i]);
		strcat(class_str, compclass_str[i]);
		if ( (XrmGetResource(db, resource_str, class_str,
				&return_type, &value) == True) && value.size
				> 0 && (strcmp((char *)(value.addr), ""))){

			/* We need to call a converter for these, then
			 * place the result in the correct place.
			 */
			switch(i) {
				default:
				*(titlecomps[i]) = *((int *)(value.addr));
					break;
				case 4:
					/* FontList */
				/* FLtoVal_addr is an OlFontList * */
				toVal.addr = (XtPointer)&FLtoVal_addr;
				if (XtConvertAndStore(Frame, XtRString,
				   &value, XtROlFontList, &toVal) == True) {
					/* Is this cast correct - 
					 * OlFontList **, rather than							 * OlFontList * for the return value.	
					 */
					*(titlecomps[i]) = FLtoVal_addr;
				} /* if conversion passes */
				else
fprintf(stderr,"Bad Resource (fontList), value=%s\n",value.addr);
					break;
				case 6:
					/* Boolean */
				/* BooltoVal_addr is a Boolean */
				toVal.addr = (XtPointer)&BooltoVal_addr;
				if (XtConvertAndStore(Frame, XtRString,
				    &value, XtRBoolean, &toVal) == True) {
					*(titlecomps[i]) =
						BooltoVal_addr;
				} /* if conversion passes */
				else
fprintf(stderr,"Bad Resource value=%s\n",value.addr);
					break;
				case 1:
				case 3:
				case 8:
				case 10:
				case 12:
				case 15:
					/* Pixmaps (maybe) */
				/* PixtoVal_addr is a Pixmap variable */
				toVal.addr = (XtPointer)&PixtoVal_addr;
				if (XtConvertAndStore(Frame, XtRString,
					&value, XtRPixmap, &toVal) == True) {
					*(titlecomps[i]) =
						PixtoVal_addr;
				} /* if conversion passes */
				else
fprintf(stderr,"Bad Resource (Pixmap), value=%s\n",value.addr);
					break;
				case 0:
				case 2:
				case 5:
				case 7:
				case 9:
				case 11:
				case 13:
				case 14:
				/* toVal_addr is an unsigned long */
				toVal.addr = (XtPointer)&toVal_addr;
				if (XtConvertAndStore(Frame, XtRString,
					&value, XtRPixel, &toVal) == True) {
					*(titlecomps[i]) =
						toVal_addr;
/*fprintf(stderr,"Pixel value = %d\n", toVal_addr);*/
				} /* if conversion passes */
				else
fprintf(stderr,"Bad Resource (Pixel), value=%s\n",value.addr);
		 	} /* switch */
		} /* if get resource == True */
	} /* for */

	/* Resolve important colors for title */
	if (motCompRes[TITLE_COMP].compai.activeBackground ==
					motCompRes[TITLE_COMP].compai.background)
		/* Same, so use inputFocusColor */
		motCompRes[TITLE_COMP].compai.activeBackground = wmrcs->inputFocusColor;
	ResolveCompConflicts(&(motCompRes[TITLE_COMP].compai));



} /* ResolveMotifComponentResources */


static void
ResolveCompConflicts OLARGLIST((motcomprcs))
OLGRA(MotCompAppearanceInfo,	*motcomprcs)
{
Pixel	white = WhitePixelOfScreen(XtScreen(Frame)),
	black = BlackPixelOfScreen(XtScreen(Frame));

	if (motcomprcs->activeForeground == motcomprcs->activeBackground) {
		if (motcomprcs->activeForeground == black)
			motcomprcs->activeBackground = white;
		else
		  if (motcomprcs->activeForeground == white)
			motcomprcs->activeForeground = black;
		else
			/* somebody is wrong - make background white */
			motcomprcs->activeBackground = white;
	}
}


/* Struct for the motif window manager "client specific" resources -
 * specify as:
 *	Mwm*client_name_or_class*resource_id
 * Can be specified for all clients (e.g., "globally") as
 *	mwm*resource_id
 * but naming a resource for a specific client name or class takes precedence
 * over this.  It is possible to specify a resource for all clients that
 * have an unknown name/class:
 *	mwm*defaults*resource_id
 */

/*
static MotCSResources	motcsrcsRec;
MotCSResources		*motcsrcs = &motcsrcsRec;
 */
static MotCSInfo	motcsrcsRec;
MotCSInfo		*motcsrcs = &motcsrcsRec;

/* For resources specified as Mwm*defaults*resource */
static MotCSInfo	motcsDefRec;
MotCSInfo		*motcsdefs = &motcsDefRec;

/*#define OFFSET(f)	XtOffsetOf(MotCSResources, f)*/
#define OFFSET(f)	XtOffsetOf(MotCSInfo, f)

XtResource	motif_cs_resources[] = {
   { XtNclientDecoration, XtCClientDecoration, XtRClientDecoration,
	sizeof(unsigned int),
     OFFSET(clientDecoration), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNclientFunctions, XtCClientFunctions, XtRClientFunctions,
	sizeof(unsigned int),
    OFFSET(clientFunctions), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNfocusAutoRaise, XtCFocusAutoRaise, XtRBoolean, sizeof(Boolean),
     OFFSET(focusAutoRaise), XtRImmediate, (XtPointer)1},
   { XtNiconImage, XtCIconImage, XtRPixmap, sizeof(Pixmap),
     OFFSET(iconImage), XtRImmediate, (XtPointer)NULL},
   { XtNiconImageBackground, XtCBackground, XtRPixel, sizeof(Pixel),
    OFFSET(iconImageBackground), XtRImmediate, (XtPointer)0},
   { XtNiconImageBottomShadowColor, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(iconImageBottomShadowColor), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNiconImageBottomShadowPixmap, XtCIIBSP, XtRPixmap, sizeof(Pixmap),
    OFFSET(iconImageBottomShadowPixmap), XtRImmediate, (XtPointer)NULL},
   { XtNiconImageForeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(iconImageForeground), XtRImmediate, (XtPointer)1},
   { XtNiconImageTopShadowColor, XtCBackground, XtRPixel, sizeof(Pixel),
    OFFSET(iconImageTopShadowColor), XtRImmediate, (XtPointer)ULONG_MAX},
   { XtNiconImageTopShadowPixmap, XtCIITSP, XtRPixmap, sizeof(Pixmap),
    OFFSET(iconImageTopShadowPixmap), XtRImmediate, (XtPointer)NULL},

/* Resources for matte colors and dimensions would go here */

   { XtNmaximumClientSize, XtCMaximumClientSize, XtRIconWinDim,
    sizeof(IconWinDim),
    OFFSET(maximumClientSize), XtRImmediate, (XtPointer)NULL},
   { XtNuseClientIcon, XtCUseClientIcon, XtRBoolean, sizeof(Boolean),
     OFFSET(useClientIcon), XtRImmediate, (XtPointer)0},
   { XtNwindowMenu, XtCWindowMenu, XtRString, sizeof(String),
     OFFSET(windowMenu), XtRImmediate, (XtPointer)NULL},

}; /* motif_cs_resources */

#undef OFFSET

Cardinal num_motif_cs_resources = XtNumber(motif_cs_resources);
#define ICON_IMAGE_BACKGROUND	4
#define ICON_IMAGE_BOT_SHADOW_COLOR	5
#define ICON_IMAGE_BOT_SHADOW_PIXMAP	6
#define ICON_IMAGE_FOREGROUND	7
#define ICON_IMAGE_TOP_SHADOW_COLOR	8
#define ICON_IMAGE_TOP_SHADOW_PIXMAP	9

/*
 *************************************************************************
 * ResolveMotifCSResources - motif client specific resources.
 ****************************procedure*header*****************************
 */
void
ResolveMotifCSResources OLARGLIST((Frame))
OLGRA(Widget, Frame)
{
int i;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
					&(motCompRes[ICON_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
					&(mcri->compai);
Display	*display = XtDisplay(Frame);

#define NUMCSRCS	13
caddr_t defaultcsrcs[NUMCSRCS] =  {
      (caddr_t) &(motcsDefRec.clientDecoration),
      (caddr_t) &(motcsDefRec.clientFunctions),
      (caddr_t) &(motcsDefRec.focusAutoRaise),
      (caddr_t) &(motcsDefRec.iconImage),
      (caddr_t) &(motcsDefRec.iconImageBackground),
      (caddr_t) &(motcsDefRec.iconImageBottomShadowColor),
      (caddr_t) &(motcsDefRec.iconImageBottomShadowPixmap),
      (caddr_t) &(motcsDefRec.iconImageForeground),
      (caddr_t) &(motcsDefRec.iconImageTopShadowColor),
      (caddr_t) &(motcsDefRec.iconImageTopShadowPixmap),
      (caddr_t) &(motcsDefRec.maximumClientSize),
      (caddr_t) &(motcsDefRec.useClientIcon),
      (caddr_t) &(motcsDefRec.windowMenu),
};
/*
	Frame = XtAppCreateShell("mwm","Mwm", applicationShellWidgetClass,
						display, NULL, 0);
 */

	/* Just some notes:
	 * The default to use for some of these:
	 *	For iconImageBackground, use
	 *		Mwm*background or Mwm*icon*background;
	 * 	For iconImageBottomShadowPixmap, use
	 *		Mwm*bottomShadowPixmap or Mwm*icon*bottomShadowPixmap.
	 *	For iconImageTopShadowColor, use
	 *		Mwm*topShadowColor or Mwm*icon*topShadowColor.
	 *	For iconImageTopShadowPixmap, use
	 *		Mwm*topShadowPixmap, or Mwm*icon*topShadowPixmap.
	 *	Default for windowMenu is the value of DefaultWindowMenu.
	 * 
	 * With that in mind,
	 * Get *resource, or Mwm*resource, put into motcsrcsRec.
	 */

	motif_cs_resources[ICON_IMAGE_BACKGROUND].default_addr = 
						(XtPointer)mcai->background;
	motif_cs_resources[ICON_IMAGE_FOREGROUND].default_addr = 
						(XtPointer)mcai->foreground;
	motif_cs_resources[ICON_IMAGE_BOT_SHADOW_COLOR].default_addr = 
					(XtPointer)mcai->bottomShadowColor;
	motif_cs_resources[ICON_IMAGE_BOT_SHADOW_PIXMAP].default_addr = 
					(XtPointer)mcai->bottomShadowPixmap;
	motif_cs_resources[ICON_IMAGE_TOP_SHADOW_COLOR].default_addr = 
					(XtPointer)mcai->topShadowColor;
	motif_cs_resources[ICON_IMAGE_TOP_SHADOW_PIXMAP].default_addr = 
					(XtPointer)mcai->topShadowPixmap;
	XtGetApplicationResources(Frame, (XtPointer)motcsrcs,
		motif_cs_resources, num_motif_cs_resources, NULL, 0);

	/* copy the record pointed to by motcsrcs to the record pointed
	 * to by motcsdefs (motcsDefRec).
	 */
	  memcpy( (MotCSInfo *) (motcsdefs),
		(MotCSInfo *) (motcsrcs),
		sizeof(MotCSInfo));			

	/* Copy over the values retrieved into the defaults */
	for (i=0; i < num_motif_cs_resources; i++) {
		motif_cs_resources[i].default_addr =
			(XtPointer) *(defaultcsrcs[i]);
	 } /* for */
	XtGetSubresources(Frame, (XtPointer)motcsdefs, "defaults", "Defaults",
		motif_cs_resources, num_motif_cs_resources, NULL, 0);
} /* ResolveMotifCSResources */

/*
 *************************************************************************
 * GetCSResources.
 * Try to find the client specific resources for the client name or class
 * identified by the WM_CLASS property of the client.  Identified in the
 * resource file by
 *	Mwm*client_name*resource 
 * If class not found, then use the defaults.
 * The class name is found in the wmstep->classhint structure, containing the
 * classhint.res_name and classhint.res_class fields.
 ****************************procedure*header*****************************
 */
void
GetCSResources OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Display *display = XtDisplay((Widget)wm);
Widget		Frame;
MotCSInfo		motclsprcs;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
					&(motCompRes[ICON_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
					&(mcri->compai);


	if (!(wmstep->csinfo))
		return;
	Frame = XtAppCreateShell("mwm","Mwm", applicationShellWidgetClass,
						display, NULL, 0);
	motif_cs_resources[ICON_IMAGE_BACKGROUND].default_addr = 
					(XtPointer) motcomprcs->background;
	motif_cs_resources[ICON_IMAGE_FOREGROUND].default_addr = 
					(XtPointer) motcomprcs->foreground;
	motif_cs_resources[ICON_IMAGE_BOT_SHADOW_COLOR].default_addr = 
				(XtPointer)motcomprcs->bottomShadowColor;
	motif_cs_resources[ICON_IMAGE_BOT_SHADOW_PIXMAP].default_addr = 
				(XtPointer) motcomprcs->bottomShadowPixmap;
	motif_cs_resources[ICON_IMAGE_TOP_SHADOW_COLOR].default_addr = 
				(XtPointer) motcomprcs->topShadowColor;
	motif_cs_resources[ICON_IMAGE_TOP_SHADOW_PIXMAP].default_addr = 
				(XtPointer) motcomprcs->topShadowPixmap;
	if (wmstep->classhint.res_name) {
/*
		XtGetSubresources(Frame, (XtPointer)(&motclsprcs),
mlp - Change to use pointer to csinfo.
 */
		XtGetSubresources(Frame, (XtPointer)(wmstep->csinfo),
			wmstep->classhint.res_name,
			wmstep->classhint.res_class,
			motif_cs_resources, num_motif_cs_resources,
			NULL, 0);
	}
	else {
		/* no class hint, use the defaults we retrieved.
		 * Memcpy the defaults to our wmstep csinfo struct
		 */
		memcpy( (MotCSInfo *) (wmstep->csinfo),
			(MotCSInfo *) (motcsdefs),
			sizeof(MotCSInfo));			
	}
/*
	if (motclsprcs.iconImage) {
		Window iconwindow1;
		iconwindow1 = XCreateSimpleWindow(display, RootWindowOfScreen(
				XtScreen(Frame)), 200, 200, 70, 70,
				 2, 0, 1);
		XSetWindowBackgroundPixmap(display, iconwindow1,
						motclsprcs.iconImage);
		XMapWindow(display, iconwindow1);
		XRaiseWindow(display, iconwindow1);
	}
 */
	AssignCSGCs(wm);
	XtDestroyWidget(Frame);
} /* GetCSResources */

/*
 *************************************************************************
 * AssignCSGCs - get the GCs associated with the client specific resources.
 * Will use XtGetGC, because it's likely that we'll have a lot of duplicate
 * GCs.
 ****************************procedure*header*****************************
 */

extern void
AssignCSGCs OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	XGCValues gcvals, gcvalstop, gcvalsbot;
	Display *display = XtDisplay((Widget)wm);
	Window root = RootWindowOfScreen(XtScreen((Widget)wm));
	unsigned int width, height, depth, bw;
	int	status;
	unsigned long mask, topmask, botmask;
	GC	brightGC, darkGC;
	Pixel	brightfgcolor, darkfgcolor;
	MotCSInfo	*csinfo = (MotCSInfo *)(wmstep->csinfo);

	MotifCompResourceInfo	*icon_mcri = (MotifCompResourceInfo *)
					&(motCompRes[ICON_COMP]);
	MotCompAppearanceInfo	*icon_mcs = (MotCompAppearanceInfo *)
					&(icon_mcri->compai);
	unsigned long	cdecor, cfunc;
	Window		root_ret;
	int		x, y;


	brightGC = OlgGetBrightGC(wmstep->mdm);
	darkGC = OlgGetBg3GC(wmstep->mdm);
	mask = GCForeground;

	/* Ignore status for now */
	status = XGetGCValues(display, brightGC, mask, &gcvals);
	brightfgcolor = gcvals.foreground;

	/* Ignore status for now */
	status = XGetGCValues(display, darkGC, mask, &gcvals);
	darkfgcolor = gcvals.foreground;


	topmask = botmask = GCForeground ;
	if (csinfo->iconImageTopShadowColor == ULONG_MAX)
		csinfo->iconImageTopShadowColor =
				icon_mcs->topShadowColor;
	if (csinfo->iconImageTopShadowColor != ULONG_MAX) {
		if (csinfo->iconImageBottomShadowColor == ULONG_MAX)
			csinfo->iconImageBottomShadowColor =
		icon_mcs->bottomShadowColor;
	}
	if (csinfo->iconImageTopShadowColor == ULONG_MAX ||
			(csinfo->iconImageBottomShadowColor == ULONG_MAX) ) {
		gcvalstop.foreground = brightfgcolor;
		gcvalsbot.foreground = darkfgcolor;
	}
	else {
		gcvalstop.foreground = csinfo->iconImageTopShadowColor;
		gcvalsbot.foreground = csinfo->iconImageBottomShadowColor;
	}
	if (csinfo->iconImageTopShadowPixmap) {
		gcvalstop.stipple = csinfo->iconImageTopShadowPixmap;
		gcvalstop.fill_style = FillStippled;
		topmask |= (GCStipple|GCFillStyle);
	}
	if (csinfo->iconImageBottomShadowPixmap) {
		gcvalsbot.stipple = csinfo->iconImageBottomShadowPixmap;
		gcvalsbot.fill_style = FillStippled;
		botmask |= (GCStipple|GCFillStyle);
	}

	wmstep->iconimagetopGC = XtGetGC((Widget)wm, topmask, &gcvalstop);
	wmstep->iconimagebotGC = XtGetGC((Widget)wm, botmask, &gcvalsbot);

	
	topmask = botmask = GCForeground;

	/* icon image foreground, background GCs */

	/* Skip iconimagefgGC for now - we'll save the color below;
	 * it is used for drawing the title, anyway.  For that, a
	 * OlgAttrs struct is needed.
	 *		***************************
	 * For the iconimagebgGC, create the GC when needed in the display
	 * icon function.
	 */


	/* Save remaining values - don't save the pixmaps, because we already
	 * made GCs from them
	 */

	/* Check validity of iconImage if !useClientIcon */
	if ( !(csinfo->useClientIcon) && (!(csinfo->iconImage) ||
		(XGetGeometry(display, csinfo->iconImage,
	&root_ret, &x, &y, &width, &height, &bw, &depth) == 0) ) )
		csinfo->useClientIcon = True;

	/* Road work:  if clientDecorations or clientFunctions are
	 * supplied by the user, then by all means use them.
	 * Keep in mind, that we have to comply with these decorations
	 * and/or functions, because we are running in motif mode, and
	 * these clients are being specified for decorating under
	 * mwm.  Even if the client is an open look client, it must
	 * follow this.
	 */
	if (csinfo->clientFunctions != ULONG_MAX) {
		cfunc = csinfo->clientFunctions;
		if (wmstep->transient_parent)
			cfunc &= ~(WMFuncMinimize | WMFuncMaximize);
		wmstep->menu_functions = cfunc;
	}
	else
		cfunc = wmstep->menu_functions;
	if (csinfo->clientDecoration != ULONG_MAX) {
		/* is it legal ? */
		cdecor = csinfo->clientDecoration;
		if (wmstep->transient_parent) {
			/* can't have minimize or maximize */
			cdecor &= ~(WMMinimize| WMMaximize);
		}
		if (cdecor != ULONG_MAX) {
			if (!(cfunc & WMFuncResize))
				cdecor &= ~WMResizable;
			if (!(cfunc & WMFuncMinimize))
				cdecor &= ~WMMinimize;
			if (!(cfunc & WMFuncMaximize))
				cdecor &= ~WMMaximize;
		}
		if (cdecor &
		   (WMMinimize|WMMaximize|WMHasFullMenu|WMHasLimitedMenu))
			cdecor |= WMHeader;
		if (cdecor & (MWM_DECOR_RESIZEH) )
			cdecor |= WMBorder;
		wmstep->decorations = cdecor;
		/* In motif mode, we've got to force olwm to display
		 * a window menu in mouseless mode (shift esc) when
		 * there is even 1 menu function.  Unchecked here is
		 * the case where a menu has no items, but items are
		 * added on by MEWM_MENU!! - for now just call it
		 * a limited or full menu just so we pop it up
		 */
		if (! (cdecor& (WMHasFullMenu | WMHasLimitedMenu))) {
			if (cfunc)
				wmstep->decorations |=
			  (wmstep->transient_parent ? WMHasLimitedMenu :
						WMHasFullMenu);
		}
	}

	/* More road work.  Check csinfo->windowMenu.  If this value is
	 * non-NULL, we have work to do.  If it is NULL, then we
	 * use the olwm-supplied menus.
	 */
	if (csinfo->windowMenu && (int)strlen(csinfo->windowMenu) > (int)0) {
		Global_Menu_Info *gmi_ptr = global_menu_info;
		while (gmi_ptr) {
			if (!strcmp(gmi_ptr->menuname, csinfo->windowMenu))
				break;
			gmi_ptr = gmi_ptr->next;
		}
		if (!gmi_ptr)
			csinfo->windowMenu = (char *)NULL;
	} /* if csinfo->windowMenu && strlen() */

} /* AssignCSGCs */
