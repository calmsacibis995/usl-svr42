/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:ptyx.h	1.2.1.34"
#endif

/*
 ptyx.h (C hdr file)
	Acc: 601052346 Tue Jan 17 09:59:06 1989
	Mod: 601054122 Tue Jan 17 10:28:42 1989
	Sta: 601054122 Tue Jan 17 10:28:42 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#ifndef _INPTYX
#define _INPTYX 1
#include <X11/copyright.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>		/* FLH mouseless */
#include <Xol/DynamicP.h>		/* FLH mouseless */
#include <Xol/PrimitiveP.h>	/* FLH mouseless must follow OpenLookP.h */
/* FLH dynamic */
#include <Xol/RubberTile.h>
/* FLH dynamic */
#include <Xol/FooterPane.h>
#include <Xol/Stub.h>
/* Extra Xlib definitions */
#define AllButtonsUp(detail, ignore)  (\
		((ignore) == Button1) ? \
				(((detail)&(Button2Mask|Button3Mask)) == 0) \
				: \
		 (((ignore) == Button2) ? \
		  		(((detail)&(Button1Mask|Button3Mask)) == 0) \
				: \
		  		(((detail)&(Button1Mask|Button2Mask)) == 0)) \
		)


/*
#ifndef Max
#define Max(i, j)       ((i) > (j) ? (i) : (j))
#endif  /* Max */

#define MAX_COLS	200
#define MAX_ROWS	128

/*
** System V definitions
*/

#if defined(SYSV) || defined(SVR4)

#ifdef JOBCONTROL
#define	xgetpgrp	getpgrp2
#else	/* !JOBCONTROL */
#define	xgetpgrp(x)	(x)
#endif	/* !JOBCONTROL */

#define	killpg(x,sig)	kill(-x,sig)

#ifndef SVR4
#define	dup2(fd1,fd2)	((fd1 == fd2) ? fd1 : \
				(close(fd2), fcntl(fd1, F_DUPFD, fd2)))
#endif /* SVR4 */

#endif	/* !SYSV */

/*
** allow for mobility of the pty master/slave directories
*/
#ifndef PTYDEV
#ifdef hpux
#define	PTYDEV		"/dev/ptym/ptyxx"
#else	/* !hpux */
#define	PTYDEV		"/dev/ptyxx"
#endif	/* !hpux */
#endif	/* !PTYDEV */

#ifndef TTYDEV
#ifdef hpux
#define TTYDEV		"/dev/pty/ttyxx"
#else	/* !hpux */
#define	TTYDEV		"/dev/ttyxx"
#endif	/* !hpux */
#endif	/* !TTYDEV */

#ifndef PTYCHAR1
#ifdef hpux
#define PTYCHAR1	"zyxwvutsrqp"
#else	/* !hpux */
#define	PTYCHAR1	"pqrstuvwxyz"
#endif	/* !hpux */
#endif	/* !PTYCHAR1 */

#ifndef PTYCHAR2
#ifdef hpux
#define	PTYCHAR2	"fedcba9876543210"
#else	/* !hpux */
#define	PTYCHAR2	"0123456789abcdef"
#endif	/* !hpux */
#endif	/* !PTYCHAR2 */

/* Until the translation manager comes along, I have to do my own translation of
 * mouse events into the proper routines. */

typedef enum {NORMAL, LEFTEXTENSION, RIGHTEXTENSION} EventMode;

/*
 * The origin of a screen is 0, 0.  Therefore, the number of rows
 * on a screen is screen->max_row + 1, and similarly for columns.
 */

/* R3 */
typedef unsigned char Char;             /* to support 8 bit chars */
typedef Char **ScrnBuf;
/* R3-end */

/*
 * ANSI emulation.
 */
#define INQ	0x05
#define	FF	0x0C			/* C0, C1 control names		*/
#define	LS1	0x0E
#define	LS0	0x0F
#define	CAN	0x18
#define	SUB	0x1A
#define	ESC	0x1B
#define US	0x1F
#define	DEL	0x7F
#define HTS     ('H'+0x40)
#define	SS2	0x8E
#define	SS3	0x8F
#define	DCS	0x90
#define	OLDID	0x9A			/* ESC Z			*/
#define	CSI	0x9B
#define	ST	0x9C
#define	OSC	0x9D
#define	PM	0x9E
#define	APC	0x9F
#define	RDEL	0xFF

#define	NBOX	5			/* Number of Points in box	*/
#define	NPARAM	10			/* Max. parameters		*/

#define	MINHILITE	32

typedef struct {
	unsigned char	a_type;
	unsigned char	a_pintro;
	unsigned char	a_final;
	unsigned char	a_inters;
	char	a_nparam;		/* # of parameters		*/
	char	a_dflt[NPARAM];		/* Default value flags		*/
	short	a_param[NPARAM];	/* Parameters			*/
	char	a_nastyf;		/* Error flag			*/
} ANSI;

typedef struct {
	int		row;
	int		col;
	unsigned	flags;	/* Vt100 saves graphics rendition. Ugh! */
	char		curgl;
	char		curgr;
	char		gsets[4];
} SavedCursor;

#ifdef TEK
#define	TEKNUMFONTS	4
/* Actually there are 5 types of lines, but four are non-solid lines */
#define	TEKNUMLINES	4

typedef struct {
	int	x;
	int	y;
	int	fontsize;
	int	linetype;
} Tmodes;

typedef struct {
	int Twidth;
	int Theight;
} T_fontsize;
#endif /* TEK */

typedef struct {
	short *bits;
	int x;
	int y;
	int width;
	int height;
} BitmapBits;

#define	SAVELINES		64      /* default # lines to save      */
#define	SCROLLBARWIDTH		14      /* scroll bar width		*/
#define POINTSIZE		12

typedef struct {
/* These parameters apply to both windows */
	Display		*display;	/* X display for screen		*/
	int		respond;	/* socket for responses
					   (position report, etc.)	*/
	int		console;	/* fd for SVR4 console logging */
	long		pid;		/* pid of process on far side   */
	int		uid;		/* user id of actual person	*/
	int		gid;		/* group id of actual person	*/
#define	MAX_EUC	4
	GC		normalGC[MAX_EUC];	/* normal painting	*/
	GC		reverseGC[MAX_EUC];	/* reverse painting	*/
	GC		normalboldGC[MAX_EUC];	/* normal painting, bold font*/
	GC		reverseboldGC[MAX_EUC];	/* reverse painting, bold font*/
	GC		cursorGC;	/* normal cursor painting	*/
	GC		reversecursorGC;/* reverse cursor painting	*/
	Pixel		foreground;	/* foreground color		*/
/* SS-color */
	Pixel		background;	/* background color		*/
/* SS-color-end */
	Pixel		cursorcolor;	/* Cursor color			*/
	Pixel		bordercolor;	/* border color			*/
	Pixel		mousecolor;	/* Mouse color			*/
	int		border;		/* inner border			*/
	Pixmap		graybordertile;	/* tile pixmap for border when
						window is unselected	*/
	Cursor		arrow;		/* arrow cursor			*/
	unsigned short	send_mouse_pos;	/* user wants mouse transition  */
					/* and position information	*/
	int		select;		/* xterm selected		*/
	Boolean		visualbell;	/* visual bell mode		*/
	int		logging;	/* logging mode			*/
/* RJK begin (security patch) */
	Boolean		allowSendEvents;/* send event mode 		*/
	Boolean		grabbedKbd;	/* secure keyboard mode		*/
/* RJK end */
	int		logfd;		/* file descriptor of log	*/
	char		*logfile;	/* log file name		*/
	Char		*logstart;	/* current start of log buffer	*/
	int		inhibit;	/* flags for inhibiting changes	*/
/* FLH resize */
	Boolean		in_curses;	/* in curses appl.		*/
/* FLH resize-end */
#ifdef I18N
	OlIm *		im;		/* input method for localized input */
	XRectangle	statusarea;
	StubWidget	statuswidget;
	OlStrSegment  segment;
	int		im_key_index;	/* update cnt for sending global
					 * Open Look keys to input method
					 */
	int		m_index;	/* mnemonic index 
					 * for input method
					 */
	int		a_index;	/* accelerator update index 
					 * for input method
					 */
#endif

/* VT window parameters */
	struct {
		Window	window;		/* X window id			*/
		int	width;		/* width of columns		*/
		int	height;		/* height of rows		*/
		int	fullwidth;	/* full width of window		*/
		int	fullheight;	/* full height of window	*/
		int	f_width;	/* width of fonts in pixels	*/
		int	f_height;	/* height of fonts in pixels	*/
	} fullVwin;
	Cursor		curs;		/* cursor resource from X	*/
	/* Terminal fonts must be of the same size and of fixed width */
	XFontStruct	*fnt_norm[MAX_EUC];	/* normal font of terminal */
	XFontStruct	*fnt_bold[MAX_EUC];	/* bold font of terminal   */
	int		enbolden;	/* overstrike for bold font	*/
	XPoint		*box;		/* draw unselected cursor	*/
	XPoint		*w_box;		/* draw unselected wide cursor	*/
	int		cursor_state;	/* ON or OFF			*/
	int		cursor_set;	/* requested state		*/
	int		cursor_col;	/* previous cursor column	*/
	int		cursor_row;	/* previous cursor row		*/
	int		cur_col;	/* current cursor column	*/
	int		cur_row;	/* current cursor row		*/
	int		max_col;	/* rightmost column		*/
	int		max_row;	/* bottom row			*/
	int		top_marg;	/* top line of scrolling region */
	int		bot_marg;	/* bottom line of  "	    "	*/
	Widget		scrollWidget;	/* pointer to scrollbar struct	*/
	Widget		menuWidget;	/* ponter to xterm menu widget 	*/
	Widget		property;	/* property widget		*/
	int		scrollbar;	/* if > 0, width of scrollbar, and
						scrollbar is showing	*/
	int		topline;	/* line number of top, <= 0	*/
	int		savedlines;     /* number of lines that've been saved */
	int		savelines;	/* number of lines off top to save */
#ifdef XTERM_COMPAT
	Boolean		scrollinput;	/* scroll to bottom on input	*/
	Boolean		scrollkey;	/* scroll to bottom on key	*/
#endif
	ScrnBuf		buf;		/* screen buffer (main)		*/
	ScrnBuf		allbuf;		/* screen buffer (may include
					   lines scrolled off top	*/
	ScrnBuf		altbuf;		/* alternate screen buffer	*/
/* SS-color */
	Pixel	      **fcolor;		/* character foreground color   */
	Pixel	      **bcolor;         /* character background color   */
	Pixel	      **allfcolor;      /* character foreground color   */
	Pixel	      **allbcolor;      /* character background color   */
	Pixel	      **altfcolor;      /* alternate foreground buffer  */
	Pixel	      **altbcolor;      /* alternate background buffer  */
/* SS-color-end */
	Boolean		alternate;	/* true if using alternate buf	*/
	unsigned short	do_wrap;	/* true if cursor in last column
					   and character just output    */
	int		incopy;		/* 0 if no RasterCopy exposure
					   event processed since last
					   RasterCopy			*/
	Boolean		c132;		/* allow change to 132 columns	*/
	Boolean		curses;		/* cludge-ups for more and vi	*/
	Boolean		marginbell;	/* true if margin bell on	*/
	int		nmarginbell;	/* columns from right margin	*/
	int		bellarmed;	/* cursor below bell margin	*/
	Boolean 	multiscroll;	/* true if multi-scroll		*/
	int		scrolls;	/* outstanding scroll count	*/
	SavedCursor	sc;		/* data for restore cursor	*/
	int		save_modes[19];	/* save dec private modes	*/

	/* Improved VT100 emulation stuff.				*/
	char		gsets[4];	/* G0 through G3.		*/
	char		curgl;		/* Current GL setting.		*/
	char		curgr;		/* Current GR setting.		*/
	char		curss;		/* Current single shift.	*/
	int		scroll_amt;	/* amount to scroll		*/
	int		refresh_amt;	/* amount to refresh		*/
	Boolean		jumpscroll;	/* whether we should jumpscroll */
	Boolean         always_highlight; /* whether to highlight cursor */

#ifdef TEK
/* Tektronix window parameters */
	GC		TnormalGC;	/* normal painting		*/
	GC		TcursorGC;	/* normal cursor painting	*/
	Pixel		Tforeground;	/* foreground color		*/
	Pixel		Tbackground;	/* Background color		*/
	Pixel		Tcursorcolor;	/* Cursor color			*/
	Pixmap		Tbgndtile;	/* background tile pixmap	*/
	int		Tcolor;		/* colors used			*/
	Boolean		Vshow;		/* VT window showing		*/
	Boolean		Tshow;		/* Tek window showing		*/
	Boolean		waitrefresh;	/* postpone refresh		*/
	struct {
		Window	window;		/* X window id			*/
		int	width;		/* width of columns		*/
		int	height;		/* height of rows		*/
		int	fullwidth;	/* full width of window		*/
		int	fullheight;	/* full height of window	*/
		double	tekscale;	/* scale factor Tek -> vs100	*/
	} fullTwin;
	XPoint		**Tbox;		/* draw unselected cursor	*/
	int		xorplane;	/* z plane for inverts		*/
	GC		linepat[TEKNUMLINES]; /* line patterns		*/
	XFontStruct 	*Tfont[TEKNUMFONTS]; /* Tek fonts		*/
	int		tobaseline[TEKNUMFONTS]; /* top to baseline for
							each font	*/
	Boolean		TekEmu;		/* true if Tektronix emulation	*/
	int		cur_X;		/* current x			*/
	int		cur_Y;		/* current y			*/
	Tmodes		cur;		/* current tek modes		*/
	Tmodes		page;		/* starting tek modes on page	*/
	int		margin;		/* 0 -> margin 1, 1 -> margin 2	*/
	int		pen;		/* current Tektronix pen 0=up, 1=dn */
	char		*TekGIN;	/* nonzero if Tektronix GIN mode*/
	Widget		TmenuWidget;	/* ponter to Tektronix menu widget   */
	Widget		Tproperty;	/* Tek property widget		*/
#endif /* TEK */
} TScreen;

/* meaning of bits in screen.select flag */
#define	INWINDOW	01	/* the mouse is in one of the windows */
#define	FOCUS		02	/* one of the windows is the focus window */

typedef struct
{
	unsigned	flags;
} TKeyboard;

typedef struct _Misc {
    char *T_geometry;
    char *f_n;
    char *f_b;
    char *curs_shape;
    Boolean log_on;
    Boolean login_shell;
    Boolean re_verse;
    Boolean reverseWrap;
    Boolean logInhibit;
    Boolean signalInhibit;
/* FLH resize */
	 Boolean allow_resize;		/* allow window resizing in curses */
/* FLH resize-end */
#ifdef TEK
    Boolean tekInhibit;
    Boolean tekSmall;   /* start tek window in small size */
#endif
    Boolean scrollbar;
    Boolean console_on;
/* FLH dynamic */
    unsigned char   dyn_flags;
/* FLH dynamic */
    Boolean mouseless;
    OlKeyDef help_key;
} Misc;

typedef struct _TMisc {
	unsigned char dyn_flags;	/* FLH dynamic */
} TMisc;

typedef struct {int foo;} XtermClassPart, TekClassPart;

typedef struct _XtermClassRec {
    CoreClassPart  core_class;
    PrimitiveClassPart   primitive_class;	/* FLH mouseless */
    XtermClassPart xterm_class;
} XtermClassRec;

#ifdef TEK
typedef struct _TekClassRec {
    CoreClassPart core_class;
    PrimitiveClassPart   primitive_class;	/* FLH mouseless */
    TekClassPart tek_class;
} TekClassRec;
#endif

/* define masks for flags */
#define CAPS_LOCK	0x01
#define KYPD_APL	0x02
#define CURSOR_APL	0x04


#define N_MARGINBELL	10
#define MAX_TABS	320
#define TAB_ARRAY_SIZE	10	/* number of ints to provide MAX_TABS bits */

typedef unsigned Tabs [TAB_ARRAY_SIZE];

typedef struct _XtermWidgetRec {
    CorePart	core;
    PrimitivePart     primitive;/* FLH mouseless		*/
    TKeyboard	keyboard;	/* terminal keyboard		*/
    TScreen	screen;		/* terminal screen		*/
    unsigned	flags;		/* mode flags			*/
    unsigned	initflags;	/* initial mode flags		*/
    Tabs	tabs;		/* tabstops of the terminal	*/
    Misc	misc;		/* miscelaneous parameters	*/
} XtermWidgetRec, *XtermWidget;

#ifdef TEK
typedef struct _TekWidgetRec {
    CorePart core;
    PrimitivePart     primitive;	/* FLH mouseless */
	 TMisc misc;
} TekWidgetRec, *TekWidget;
#endif /* TEK */

#define BUF_SIZE 4096

/* masks for terminal flags */

#define INVERSE		0x01	/* invert the characters to be output */
#define UNDERLINE	0x02	/* true if underlining */
#define BOLD		0x04
#define USE_FG_COLOR	0x08	/* true when colors are used	*/
#define USE_BG_COLOR	0x10	/* true if screen white on black */
#define HILITED		0x20	/* true if in origin mode */
#define ORIGIN		0x40	/* true if in origin mode */
#define INSERT		0x80	/* true if in insert mode */
#define SMOOTHSCROLL	0x100	/* true if in smooth scroll mode */
#define AUTOREPEAT	0x200	/* true if in autorepeat mode */
#define IN132COLUMNS	0x400	/* true if in 132 column mode */
#define LINEFEED	0x800
#define	REVERSEWRAP	0x1000	/* true if reverse wraparound mode */
#define WRAPAROUND	0x2000
#define REVERSE_VIDEO	0x4000	/* true if screen white on black */

#define	ATTRIBUTES	0x03F	/* attributes mask */
#define CHAR		0177

/* SS-color */
#define FGColor(n) ((n)>>3)
#define BGColor(n) ((n)&0x7)
#define SetFGColor(p, n) ((p)&0x07 | (n<<3))
#define SetBGColor(p, n) ((p)&0x38 | (n))
/* SS-color-end */

/* SS-inter */
#define	EUC0		0
#define EUC1		1
#define EUC2		2
#define EUC3		3

#define EUC_MASK	0x3
#define FIRST_BYTE	0x4
#define SECOND_BYTE	0x8
#define MULTY_BYTE	0xc	/* FIRST_BYTE | SECOND_BYTE */

#define Code_set(n)	((n) & EUC_MASK)
#define Multy_byte(n)	((n) & MULTY_BYTE)
/* SS-inter-end */

#define VWindow(screen)		(screen->fullVwin.window)
#define VShellWindow		toplevel->core.window
#define VShellWidget		toplevel


/* FLH dynamic */

#define TextWindow(screen)      (screen->fullVwin.window)
#define Width(screen)		(screen->fullVwin.width)
#define Height(screen)		(screen->fullVwin.height)
#define FullWidth(screen)	(screen->fullVwin.fullwidth)
#define FullHeight(screen)	(screen->fullVwin.fullheight)
#define FontWidth(screen)	(screen->fullVwin.f_width)
#define FontHeight(screen)	(screen->fullVwin.f_height)

#ifdef TEK
#define TWindow(screen)         (screen->fullTwin.window)
#define TShellWindow            tekWidget->core.parent->core.window
#define TWidth(screen)          (screen->fullTwin.width)
#define THeight(screen)         (screen->fullTwin.height)
#define TFullWidth(screen)      (screen->fullTwin.fullwidth)
#define TFullHeight(screen)     (screen->fullTwin.fullheight)
#define TekScale(screen)        (screen->fullTwin.tekscale)
#endif /* TEK */

/* SS-scrollbar
#define CursorX(screen,col) ((col) * FontWidth(screen) + screen->border \
			+ screen->scrollbar)
   SS-scrollbar-end */
#define CursorX(screen,col) ((col) * FontWidth(screen) + screen->border)
#define CursorY(screen,row) ((((row) - screen->topline) * FontHeight(screen)) \
			+ screen->border)

#define	TWINDOWEVENTS	(KeyPressMask | ExposureMask | ButtonPressMask |\
			 ButtonReleaseMask | StructureNotifyMask |\
			 EnterWindowMask | LeaveWindowMask | FocusChangeMask)

#define	WINDOWEVENTS	(TWINDOWEVENTS | PointerMotionMask)

#ifdef TEK
#define TEK_LINK_BLOCK_SIZE 1024

typedef struct Tek_Link
{
        struct Tek_Link *next;  /* pointer to next TekLink in list
                                   NULL <=> this is last TekLink */
        short count;
        char *ptr;
        char data [TEK_LINK_BLOCK_SIZE];
} TekLink;
#endif /* TEK */
/* flags for cursors */
#define	OFF		0
#define	ON		1
#define	CLEAR		0
#define	TOGGLE		1

/* flags for inhibit */
#define	I_LOG		0x01
#define	I_SIGNAL	0x02

#ifdef TEK
#define I_TEK		0x04

extern Cursor make_tcross();
extern Cursor make_wait();
#endif /* TEK */

extern Cursor make_xterm();
extern Cursor make_arrow();
#endif

/* FLH dynamic */
/* dynamics resources bit masks 
 *
 *	used for both the VT100 and Tek widgets
 */
#define OL_B_XTERM_BG      		(1 << 0)
#define OL_B_XTERM_FONTCOLOR		(1 << 1)
#define OL_B_XTERM_CURSORCOLOR	(1 << 2)
/* FLH dynamic */

extern Widget CreateScrollBar OL_ARGS((XtermWidget));
extern void HandleHelpMessage OL_ARGS((Widget 	 w,
				       XtPointer client_data,
				       XEvent * event,
				       Boolean * 	continue_to_dispatch
));
