/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)olwm:wm.h	1.38"
#endif

#ifndef _OL_WM_H
#define _OL_WM_H

/*
 ************************************************************************	
 * Description:
 *	This file contains externs and global information needed by
 *	other window manager files.
 ************************************************************************	
 */

#include <Xol/DynamicP.h>

#include <Xol/buffutil.h>
typedef Bufferof(Widget)	WidgetBuffer;


typedef enum 
   { 
   WMNorthGravity, WMEastGravity, WMSouthGravity, WMWestGravity
   } WMIconGravity;

typedef enum 
   { 
   WMWITHDRAWN, WMICONICFULL, WMICONICNORMAL, WMFULLSIZE, WMNORMALSIZE
   } WMSize;

/* A WMPiece is any decoration on the frame.  It is returned by EventIn()
 * to figure out where the pointer is when (possibly) the help key is
 * pressed (e.g., over what window manager decoration is the pointer).
 * Motif mode: MINB = minimize button, MAXB = maximize button; use MM for
 * the menu button.
 */
typedef enum 
   {
   WM_NW, WM_NE, WM_SE, WM_SW, WM_BANNER, WM_T, WM_R, WM_L, WM_B, WM_PP, WM_MM, WM_BTN, WM_MINB, WM_MAXB, WM_NULL
   } WMPiece;

typedef enum {WM_NOP, WM_RESIZE, WM_MOVE, WM_PIN, WM_MENU } WMAction;


typedef struct _WMHelpDef
   {
   char *	helpfile;
   char *	helptitle;
   XtPointer	menuargs;	/* has nothing to do with help, but it's
				 * convenient to have this extra field.
				 */
   } WMHelpDef;

typedef struct _WMMenuDef
   {
   char *	label;
   int		mnemonic;
   void		(*selectProc)();
   WMHelpDef *	helpData;
   Boolean	sensitive;
   char *	accelerator;
   int		defaultItem;
   Widget	subMenu;
   } WMMenuDef;

typedef struct _WMMenuInfo
   {
   Widget       w;	/* WMStepWidget that caused menu to pop */
   Widget       MenuShell;
   Widget	menu;
   Widget	separator; /* Motif mode - Separates 1st, 2nd pane */
   Widget	menu2;	/* For default motif mode menu */
   WMMenuDef *	menu_items;
   WMMenuDef *	menu_items2; /* For default motif mode menu */
   int          num_menu_items;
   int          num_menu_items2; /* for default motif mode menu */
   Cardinal	menu_default;
   Widget       CascadeShell;
   Widget	cascade;
   WMMenuDef *	cascade_items;
   int          num_cascade_items;
   } WMMenuInfo;

typedef struct _WMMetrics
   {
   int scale;
   int resolution;
   int cornerX;
   int cornerY;
   int cornerx;
   int cornery;
   int linewidX;
   int linewidY;
   int linewidx;
   int linewidy;
   int gapx;
   int gapy;
   int bannerht;
   int hgap1;
   int linewid;
   int hgap2;
   int offset;
   int baseline;
   int markwid;
   int selectwidth;
   int mark_title_gap;
   int compiled;
   int motifButtonWidth;
   int motifButtonHeight;
   int motifHResizeBorderWidth;
   int motifVResizeBorderWidth;
   int motifHNResizeBorderWidth;
   int motifVNResizeBorderWidth;
   } WMMetrics;

typedef struct _WMGeometry
   {
   int x;
   int y;
   int width;
   int height;
   int border_width;
   } WMGeometry;

typedef struct _IconWinDim {
	Dimension width;
	Dimension height;
} IconWinDim;

typedef struct _WMResources {
	Pixel		iconForeground;
	Pixel		iconBackground;
	Pixel		inputFocusColor;
	Pixel		foregroundColor;
	Pixel		backgroundColor;
	Pixel		windowFrameColor;
	String		helpDirectory;
	int		iconGridSize;
	int		scale;
	WMIconGravity	iconGravity;
	Boolean		parentRelative;
	Boolean		moveOpaque;
	Boolean		pointerFocus;
	Boolean		selectDoesPreview;
	Boolean		iconBorder;
	Boolean		iconGrid;
	Boolean		windowLayering;
        OlFontList      *font_list;
	XFontStruct	*font;
	XFontStruct	*iconFont;
        String          xnlLanguage;
	Boolean		iconParentRelative;
	Boolean		pointerColormapFocus;
	Boolean		do_fork;
	Boolean		pass_keys;
} WMResources;

/* 12/20/91 - mlp - motif global resources (incomplete) */
typedef struct _MotWMResources {
   Boolean autoKeyFocus;
   int autoRaiseDelay;
   String bitmapDirectory;
   String buttonBindings;
   Boolean cleanText;
   Boolean clientAutoPlace;
   int colormapFocusPolicy;
   String configFile;
   Boolean deiconifyKeyFocus;
   int doubleClickTime;
   Boolean enableWarp;
   Boolean enforceKeyFocus;
   Boolean focusAutoRaise;
   int frameBorderWidth;
   Boolean iconAutoPlace;
   Boolean iconClick;
   int iconDecoration;
   IconWinDim iconImageMaximum;
   IconWinDim iconImageMinimum;
   int iconPlacement;
   int iconPlacementMargin;
   Boolean interactivePlacement;
   String keyBindings;
   int keyboardFocusPolicy;
   Boolean limitResize;
   Boolean lowerOnIconify;
   IconWinDim maximumMaximumSize;
   Dimension moveThreshold;
   Boolean multiScreen;
   Boolean passButtons;
   Boolean passSelectButton;
   Boolean positionIsFrame;
   Boolean positionOnScreen;
   Dimension quitTimeout;
   Boolean raiseKeyFocus;
   int resizeBorderWidth;
   int resizeCursors;
   String  screens;
   int showFeedback;
   Boolean startupKeyFocus;
   int transientDecoration;
   int transientFunctions;
   Boolean wMenuButtonClick;
   Boolean wMenuButtonClick2;
} MotWMResources;

/* More motif global resources, related to component appearance -
 * include all resources related to components, but some don't
 * apply to all components - for example, the active resources
 * don't apply to menus, feedback, title appearance.
 * These resources may be assigned globally to all clients, but
 * the refinement may be on a per-client basis.
 */

typedef struct _MotWMCompResources {
	Pixel	background;
	String	backgroundPixmap;
	Pixel	bottomShadowColor;
	String	bottomShadowPixmap;
	OlFontList	*fontList;
	Pixel	foreground;
	Boolean	saveUnder;
	Pixel	topShadowColor;
	String	topShadowPixmap;
		/* Following apply to icons and window frames */
	Pixel	activeBackground;
	String	activeBackgroundPixmap;
	Pixel	activeBottomShadowColor;
	String	activeBottomShadowPixmap;
	Pixel	activeForeground;
	Pixel	activeTopShadowColor;
	String	activeTopShadowPixmap;
} MotWMCompResources;

/* Struct for final set of motif resources - for client specific
 * resources - not all are accounted for as of 12/21/91 (not
 * the matte resources, and not the windowMenu resource.
 */

typedef struct _MotCSResources {
	int	clientDecoration;
	int	clientFunctions;
	Boolean	focusAutoRaise;
	String	iconImage;
	Pixel	iconImageBackground;
	Pixel	iconImageBottomShadowColor;
	String	iconImageBottomShadowPixmap;
	Pixel	iconImageForeground;
	Pixel	iconImageTopShadowColor;
	String	iconImageTopShadowPixmap;
	/* matte resource fields would go here  - not yet...
	 * Pixel	matteBackground;
	 * Pixel	matteBottomShadowColor;
	 * Pixel	matteBottomShadowPixmap;
	 * Pixel	matteForeground;
	 * Pixel	matteTopShadowColor;
	 * Pixel	matteTopShadowPixmap;
	 * Dimension	matteWidth;
	 */
	IconWinDim	maximumClientSize;
	Boolean	useClientIcon;
	/* windowMenu resource field would go here */
	String	windowMenu;
} MotCSResources;

/* Motif "component appearance" information - one struct for 4
 * different areas of component appearance, based on mwm man page:
 *
 *  Mwm*[menu|icon|client|feedback]*resource_id - the resources
 *
 * are identified in the structure below.  A 5th possible area
 * of component appearance is the title area, specified by
 *	Mwm*client*title*resource_id
 *
 *  The first 9 elements of the structure are resources used by all
 *  window manager components;  The final 7 are used for frames and
 *  icons only; 
 */

typedef struct _MotCompAppearanceInfo {
	Pixel	background;
	Pixmap	backgroundPixmap;
	Pixel	bottomShadowColor;
	Pixmap	bottomShadowPixmap;
	OlFontList	*fontList;
	Pixel	foreground;
	Boolean	saveUnder;
	Pixel	topShadowColor;
	Pixmap	topShadowPixmap;
		/* The following are for frames and icons, only */
	Pixel	activeBackground;
	Pixmap	activeBackgroundPixmap;
	Pixel	activeBottomShadowColor;
	Pixmap	activeBottomShadowPixmap;
	Pixel	activeForeground;
	Pixel	activeTopShadowColor;
	Pixmap	activeTopShadowPixmap;
} MotCompAppearanceInfo;

/* Each number defined below represents a position in the component resource
 * array of values.  The component appearance array is in MotifRes.c,
 * but will be globally accessed via Extern.h.
 */
#define CLIENT_COMP	0
#define ICON_COMP	1
#define TITLE_COMP	2
#define MENU_COMP	3
#define FEEDBACK_COMP	4

#define NUM_MOTIF_COMPONENTS	5

typedef struct _MotClientSpecificInfo {
	unsigned int	clientDecoration;
	unsigned int	clientFunctions;
	Boolean	focusAutoRaise;
	Pixmap	iconImage;
	Pixel	iconImageBackground;
	Pixel	iconImageBottomShadowColor;
	Pixmap	iconImageBottomShadowPixmap;
	Pixel	iconImageForeground;
	Pixel	iconImageTopShadowColor;
	Pixmap	iconImageTopShadowPixmap;
	/* Not yet with the matte resources...
	 * Pixel	matteBackground;
	 * Pixel	matteBottomShadowColor;
	 * Pixmap	matteBottomShadowPixmap;
	 * Pixel	matteForeground;
	 * Pixel	matteTopShadowColor;
	 * Pixmap	matteTopShadowPixmap;
	 * Dimension	matteWidth;
	 */
	IconWinDim	maximumClientSize;
	Boolean		useClientIcon;
	String	windowMenu;
} MotCSInfo;
	

typedef struct _SubWindowInfo {
	Window	topwindow;
	Colormap colormap;
	Window	window;
	Widget wm;
} SubWindowInfo;

typedef struct _PendingInfo {
	Widget pending;
	Widget delete;
} PendingInfo;

/* Motif mode structure to hold component resource information.  Will
 * have an array that has one MotifResourceInfo struct per component.
 * For example, one slot of the array is for icons;  topGC will
 * be created for the icon shadows, both active and non-active, and
 * the active foreground, active background, and  non-active background.
 *
 * Another slot in the array will be for windows;  There will be slots in
 * the array for feedback objects and menus, but for those, the active GCs
 * are not needed.
 */
typedef struct _MotifCompResourceInfo {
	/*MotWMCompResources	compRes;*/
	MotCompAppearanceInfo	compai;		/* direct corr. to compRes */
	GC	topGC;	/* non-active topShadowColor */
	GC	botGC;	/* non-active bottomShadowColor */
	GC	activefgGC; /* windows, icons only */
	GC	activebgGC; /* window, icons only */
	GC	activetopGC; /* activeTopShadowColor */
	GC	activebotGC; /* activeBottomShadowColor */
	GC	backgroundGC; /* background resources, such as color, pixmap */
} MotifCompResourceInfo;


/* This stuff was formerly in WMStepP.h, but I now need that WMMenuButtonData
 * struct for other uses
 */
typedef void (*Menu_CB)();

typedef struct _WMMenuButtonData
{
	int		labelType;	/* currently, support PL_TEXT */
	String		label;		/* button label */
	OlDefine	mnemonic;
	char		*accelerator;	/* menu button keybd. accelerator */
	Menu_CB		menucb;		/* callback */
	XtPointer	args;		/* arguments on menu spec line */
	OlKeyDef	kd;		/* Key information for accelerator */
	struct _WMMenuButtonData *next;	/* ptr. to next of these structs */
} WMMenuButtonData;

#define WMHints			(1<<0)
#define WMNormalSizeHints	(1<<1)
#define WMZoomSizeHints		(1<<2)

#define TakeFocus		(1L<<0)
#define SaveYourself		(1L<<1)
#define DeleteWindow		(1L<<2)
#define IgnoreUnmap		(1L<<3)
#define HasFocus		(1L<<4)
#define LocallyActive		(1L<<5)
#define GloballyActive		(1L<<6)
#define Passive			(1L<<7)
#define NoInput			(1L<<8)
#define FocusModel	(LocallyActive | GloballyActive | Passive | NoInput)
#define MwmOffset		(1L<<9) /* New for Moolit */

#define WMNoDecorations		(0)
#define WMHasFullMenu		(1<<0)
#define WMHasLimitedMenu	(1<<1)
#define WMResizable		(1<<2)
#define WMHeader		(1<<3)
#define WMPushpin		(1<<4)
#define WMPinIn			(1<<5)
#define WMCancel		(1<<6)
#define WMSelected		(1<<7)
#define WMNotReparented		(1<<8)
#define WMBusy			(1<<9)
#define WMMarkDepressed		(1<<10)
#define WMIconWindowReparented	(1<<11)
#define WMIconWindowMapped	(1<<12)

/* Added these for MooLIT */

#define WMUsesOlwa		(1<<13)
#define WMUsesWmdh		(1<<14)
#define WMUsesMwmh		(1<<15)
#define WMBorder		(1<<16)
#define WMMinimize		(1<<17)
#define WMMaximize		(1<<18)
#define WMMenuButton		(1<<19)

/* for wmstep->menu_functions field */

#define WMFuncMove		(1<<0)
#define WMFuncResize		(1<<1)
#define WMFuncMinimize		(1<<2)
#define WMFuncMaximize		(1<<3)
#define WMFuncClose		(1<<4)

/* If these are turned on, then the button in the header is selected */
#define WMMenuButtonState	(1<<5)
#define WMMinButtonState	(1<<6)
#define WMMaxButtonState	(1<<7)
#define WMTitleButtonState	(1<<8)

#define WM_CANT_GET_WA  1

#define WMSame		0
#define WMRaise		1
#define WMLower		2

#define LoseFocus	0
#define GainFocus	1

#define NoAppend	0
#define Append		1
#define NoAppendAny	2
#define AppendAny	3
#define NestedNoAppendAny	4

#define PAUSEX		36
#define PAUSEY		36

#define LIST_INCREMENT  5

#define SendProtocolMessage(display,window,protocol,time)	\
   SendLongMessage(display, window, XA_WM_PROTOCOLS(display),	\
				protocol, time, 0L, 0L, 0L)

	/*
	 * Define some macros
	 */

#ifdef DEBUG
static int debugging = 0;
#define FPRINTF(x) if (debugging) fprintf x
#else
#define FPRINTF(x)
#endif
#define DUMPBUFFER(buffer) {int i;					\
 (void) fprintf(stderr,"Buffer (size = %d used = %d esize = %d)\n",	\
           buffer-> size, buffer-> used, buffer-> esize);		\
        for (i = 0; i < buffer-> used; i++)				\
    (void) fprintf(stderr, "%d : %x\n", i, buffer-> p[i]);		\
	}

#define DEFAULT_HELP_PATH  "/usr/X/clients/olwm/help"

#define MIN(x,y)	((x) < (y) ? (x) : (y))
#define MAX(x,y)	((x) > (y) ? (x) : (y))

#define LN		0
#define NW		1
#define NE		2
#define SE		3
#define SW		4
#define BD		5
#define BN		6
#define PP		7
#define MM		8

#define OlClearRectangle(w,r,f) \
   XClearArea(XtDisplay(w), XtWindow(w), \
	(r)-> x, (r)-> y, (r)-> width, (r)-> height, f)

/* Minimum width, height of a window - decorations must be visible */
#define MINIMUM_WIDTH  44
#define MINIMUM_HEIGHT 44

/* Use with CurrentMenuPosted variable */
#define LIMITEDMENU	01
#define FULLMENU	02

/* Used with respect to WM_COLORMAP_WINDOWS property */
#define NEWCMAP		(int)0
#define DESTROYCMAP	(int)1

/* Use with window manager input event database */

#define OL_WM_BASE		1000
#define OL_WM_OPENCLOSE 	OL_WM_BASE+0
#define OL_WM_SIZE 		OL_WM_BASE+1
#define OL_WM_PROPERTIES 	OL_WM_BASE+2
#define OL_WM_REFRESH 		OL_WM_BASE+3
#define OL_WM_BACK 		OL_WM_BASE+4
#define OL_WM_QUIT 		OL_WM_BASE+5
#define OL_WM_DISMISSTHIS 	OL_WM_BASE+6
#define OL_WM_DISMISSALL 	OL_WM_BASE+7
#define OL_WM_MOVE 		OL_WM_BASE+8
#define OL_WM_RESIZE 		OL_WM_BASE+9
#define OL_WM_OWNER 		OL_WM_BASE+10
/* For Motif mode */
#define OL_WM_RESTORE		OL_WM_BASE+11
#define OL_WM_MINIMIZE		OL_WM_BASE+12
#define OL_WM_MAXIMIZE		OL_WM_BASE+13

/* Any further keys in private step widget databases can be added below
 * this, starting at OL_WM_BASE+11
 */
#define OL_WM_PRIVATE_KEYS	OL_WM_BASE+13

/* Check focus after window menu posts */
#define CHECKFOCUS

#define START_FLAG	1
#define TERMINATE_FLAG	2

/* values for MwmHints.input_mode  - these should go in VendorI.h */
#define MWM_INPUT_MODELESS			0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL	1
#define MWM_INPUT_SYSTEM_MODAL			2
#define MWM_INPUT_FULL_APPLICATION_MODAL	3

/* for some menu functions */
#define ICON 1
#define WINDOW 2
#define TRANSIENT 3
#define APPL	4

/* for iconDecoration resource use (Motif mode) */
#define ICON_LABEL	1<<0
#define ICON_IMAGE	1<<1
#define ICON_ACTIVELABEL	1<<2

/* for showFeedback resource use (Motif mode) */
#define FEED_NONE	0
#define FEED_MOVE	1<<0
#define FEED_RESIZE	1<<1
#define FEED_BEHAVIOR_SWITCH	1<<2
#define FEED_KILL	1<<3
#define FEED_INITPLACEMENT	1<<4
#define FEED_QUIT	1<<5
#define FEED_RESTART	1<<6
#define FEED_ALL	FEED_MOVE|FEED_RESIZE|FEED_BEHAVIOR_SWITCH|\
		FEED_KILL|FEED_INITPLACEMENT|FEED_QUIT|FEED_RESTART

/* Keyboard focus policy */
#define KEYBOARD_EXPLICIT	1
#define KEYBOARD_IMPLICIT	2

/* Motif icon placement  */
#define ICONLEFT	1 << 0
#define ICONRIGHT	1 << 1
#define ICONTOP		1 << 2
#define ICONBOTTOM	1 << 3

#define CMAP_POLICY_EXPLICIT	1
#define CMAP_POLICY_IMPLICIT	2

/* That resource description file from Motif mode */


/* Do we need a "map" that gives us more detail about the shell widget?
 * For example, that widget 1 is a flat, widget 2 is a separator, widget
 * 3 is a Title, etc.
 */
 typedef struct {
	Widget widget;
	int	type;	/* GMI_FLAT, GMI_SEPARATOR, GMI_TITLE,
			 * or GMI_TITLE_BOUNDARY
			 */
	WMMenuButtonData	*mbd;
	int flatpane_index; /* if FLAT, which one? */
} WidgetMap;

typedef struct {
	int wmap_index; /* which entry i the widget map is the button on ? */
	WMMenuButtonData *mbd;
	WMMenuDef menu_button;
	int flatpane_index; /* which flat is the button on ? */
} MenuButtonMap;

/* The maximum number of flat menu panes on any specially defined menu
 * shell is 25 - we have to cut it off somewhere.
 */
#define MAX_FLAT_PANES	25

struct _Global_Menu_Info {
	Widget		w;	/* WMStep Widget that asked for it */
	String		menuname;	/* Malloc, save string */
	WMMenuButtonData *mbd;
	Widget		MenuShell;	/* This popup menu shell */
	WMMenuButtonData	*menu_default; /* The mbd of the button that is
						* the default.
						*/
	int		*num_menu_items;
 	WidgetMap	*wmap;
 	int		num_wmap;
 	MenuButtonMap	*mbmap;
 	int		num_mbmap;
	OlKeyOrBtnRec	*menu_keys; /* For accelerators */
	short		menu_keys_used;
	WMMenuDef	*motif_menu[MAX_FLAT_PANES];
	short		num_flat_panes;
	short		buttons_per_pane[MAX_FLAT_PANES];
	struct _Global_Menu_Info *next;
};

typedef struct _Global_Menu_Info Global_Menu_Info;

#define GMI_FLAT	1
#define GMI_SEPARATOR	2
#define GMI_TITLE	3
#define GMI_TITLE_BOUNDARY	4
/* The boundary is the double lines drawn around the title in a menu */

/* Open Look icons */

#define ICON_BORDER_WIDTH	3
#define	ICON_IMAGE_PAD		1

#define MAX_MENUSHELLS		35

/* Watch out.  These defines are used in wmm.c when we create the toplevel
 * shell, and again in motif mode when we create the OlwmShell;  if these
 * or any one of these change, then change them appropriately in wmm.c
 */
#define APPLICATION_NAME   "olwm"
#define APPLICATION_CLASS  "olwm"

#define MOT_APPLICATION_NAME   "mwm"
#define MOT_APPLICATION_CLASS  "Mwm"

#define NO_ICON_ROOM	-1073

#endif /* _OL_WM_H */
