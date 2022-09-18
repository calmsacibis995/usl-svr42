/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:Tekproc.c	1.38"
#endif

/*
 * $XConsortium: Tekproc.c,v 1.54 89/03/23 11:59:59 jim Exp $
 *
 * Warning, there be crufty dragons here.
 */


#ifdef TEK
#include <X11/copyright.h>

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include "ptyx.h"
#include "Tekparse.h"
#include <stdio.h>
#include "Strings.h"
#include "messages.h"

#include <sys/termio.h>		/* for TIOCSWINSZ	*/
#ifndef SVR4
#include        <sys/stream.h>  /* for typedef used in ptem.h */
#include        <sys/ptem.h>    /* for struct winsize */
#endif /* SVR4 */

#ifdef umips
# ifndef SYSTYPE_SYSV
# include <sgtty.h>
# endif /* not SYSTYPE_SYSV */
#else
/* # ifndef SYSV */
# if !defined(SYSV) && !defined(SVR4)
# include <sgtty.h>
# endif
#endif /* umips */
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <pwd.h>
#include "data.h"
#include "error.h"

#ifdef macII
#undef FIOCLEX					/* redefined from sgtty.h */
#undef FIONCLEX					/* redefined from sgtty.h */
#include <sys/ioctl.h>				/* to get FIONREAD */
#endif /* macII */

/* #ifdef SYSV */
#if defined(SYSV) || defined(SVR4) 
#include <xterm.h>
#include <poll.h>
#include <unistd.h>		/* for F_OK */
#endif

/* MORE: next line is stolen from R3 Intrinsics */
#define XtRDimension            "Dimension"

#include <Xol/OpenLook.h>

/*
 * USL SVR4: need to define SYSV
 */
#define SYSV YES

extern void exit();
extern long time();

#define TekColormap DefaultColormap( screen->display, \
				    DefaultScreen(screen->display) )
#define DefaultGCID DefaultGC(screen->display, DefaultScreen(screen->display))->gid

/* Tek defines */

#define	BEL		07
#define	CANCEL		030
#define	DOTDASHEDLINE	2
#define	DOTTEDLINE	1
#define	EAST		01
#define	ETX		03
#define	LARGEFONT	0
#define	LARGEFONTNAME	"9x15"
#define	LINEMASK	07
#define	LONGDASHEDLINE	4
#define	MARGIN1		0
#define	MARGIN2		1
#define MAX_PTS		150
#define MAX_VTX		300
#define	NAK		025
#define	NORTH		04
#define	PENDOWN		1
#define	PENUP		0
#define	SHORTDASHEDLINE	3
#define	SMALLFONT	3
#define	SMALLFONTNAME	"6x10"
#define	SOLIDLINE	0
#define	SOUTH		010
#define	TEKBOTTOMPAD	23
#define	TEKDEFHEIGHT	565
#define	TEKDEFWIDTH	750
#define	TEKHEIGHT	3072
#define	TEKHOME		((TekChar[screen->page.fontsize].nlines - 1)\
			 * TekChar[screen->page.fontsize].vsize)
#define	TEKMINHEIGHT	452
#define	TEKMINWIDTH	600
#define	TEKTOPPAD	34
#define	TEKWIDTH	4096
#define	TEXT_BUF_SIZE	256
#define	THREEFONT	2
#define	THREEFONTNAME	"6x13"
#define	TWOFONT		1
#define	TWOFONTNAME	"8x13"
#define	WEST		02

#define	TekMove(x,y)	screen->cur_X = x; screen->cur_Y = y
#define	input()		Tinput()
#define	unput(c)	*Tpushback++ = c

#ifndef lint
static char rcs_id[] = "$XConsortium: Tekproc.c,v 1.54 89/03/23 11:59:59 jim Exp $";
#endif	/* lint */

static XPoint *T_box[TEKNUMFONTS] = {
	T_boxlarge,
	T_box2,
	T_box3,
	T_boxsmall,
};
static struct Tek_Char {
	int hsize;	/* in Tek units */
	int vsize;	/* in Tek units */
	int charsperline;
	int nlines;
} TekChar[TEKNUMFONTS] = {
	{56, 88, 74, 35},	/* large */
	{51, 82, 81, 38},	/* #2 */
	{34, 53, 121, 58},	/* #3 */
	{31, 48, 133, 64},	/* small */
};

static Cursor GINcursor;
static XSegment *line_pt;
static int nplot;
static TekLink Tek0;
static jmp_buf Tekjump;
static TekLink *TekRecord;
static XSegment *Tline;

extern int Talptable[];
extern int Tbestable[];
extern int Tbyptable[];
extern int Tesctable[];
extern int Tipltable[];
extern int Tplttable[];
extern int Tpttable[];
extern int Tspttable[];

static int *curstate = Talptable;
static int *Tparsestate = Talptable;

/* event handlers */
extern void HandleTekKeyPressed(), HandleEightBitKeyPressed();
extern void HandleStringEvent();
extern void HandleEnterWindow();
extern void HandleLeaveWindow();
extern void HandleSecure();
extern void TekButtonPressed();

static Boolean SetValues();
static Boolean AcceptFocus();
static void	FocusHandler OL_ARGS((Widget, OlDefine));

static Dimension defOne = 1;

/* FLH dynamic */
#define BYTE_OFFSET  XtOffset(TekWidget, misc.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_XTERM_BG, NULL },
{ { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0,
XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_XTERM_FONTCOLOR, NULL },
{ {XtNinputFocusColor, XtCForeground, XtRPixel, sizeof(Pixel), 0,
XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_XTERM_CURSORCOLOR, NULL},
};
#undef BYTE_OFFSET
/* FLH dynamic */

static XtResource resources[] = {
    {XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
	 XtOffset(Widget, core.width), XtRDimension, (XtPointer)&defOne},
    {XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
	 XtOffset(Widget, core.height), XtRDimension, (XtPointer)&defOne},
	
/* FLH dynamic resources */
	
	{XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
	XtOffset(TekWidget, core.background_pixel), XtRString, XtDefaultBackground},

	{XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
	XtOffset(XtermWidget, primitive.font_color), XtRString, XtDefaultForeground},
	
	/* input_focus_color is defined in the Primitive widget */
};

static void TekInitialize(), TekRealize(), TekConfigure();
void TekExpose();

/* FLH mouseless */
static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void	Key OL_ARGS((Widget, OlVirtualEvent));

static OlEventHandlerRec
event_procs[] = {
   { KeyPress, Key},
   { ButtonPress, TekButtonPressed}
};
/* FLH mouseless-end */

XtermClassRec tekClassRec = {
  {
/* core_class fields */		/* FLH mouseless */
    /* superclass	  */	(WidgetClass) &primitiveClassRec,
    /* class_name	  */	"Tek4014",
    /* widget_size	  */	sizeof(TekWidgetRec),
    /* class_initialize   */    NULL,
    /* class_part_initialize */ NULL,
    /* class_inited       */	FALSE,
    /* initialize	  */	TekInitialize,
    /* initialize_hook    */    NULL,				
    /* realize		  */	TekRealize,
/* MORE: SS changed the next two lines */
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave */   TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	NULL,
    /* resize		  */	TekConfigure,
    /* expose		  */	TekExpose,
    /* set_values	  */	SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    NULL,
    /* get_values_hook    */    NULL,
    /* accept_focus	  */	AcceptFocus,	/* FLH mouseless */
    /* version            */    XtVersion,
    /* callback_offsets   */    NULL,
/* MORE: SS changed next line */
    /* tm_table           */    XtInheritTranslations,	/* FLH mouseless */
    /* tm_table             	NULL, */
#ifdef X11R3
    /* query_geometry     */    XtInheritQueryGeometry,
    /* display_accelerator*/    XtInheritDisplayAccelerator,
    /* extension          */    NULL
#endif
  },
  {
	/* primitive class */	/* FLH mouseless */
    /* focus_on_select    */	True,
    /* highlight_handler  */	FocusHandler,
    /* traversal_handler  */	NULL,
    /* register_focus	  */	NULL,
    /* activate		  */	ActivateWidget,
    /* event_procs	  */	event_procs,
    /* num_event_procs	  */	XtNumber(event_procs),
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{dyn_res, XtNumber(dyn_res)}, /* FLH dynamic */	
    /* transparent_proc	  */	NULL,
 },
 {
  	/* tek fields */
	0,
 },
				/* FLH mouseless-end */
};
#define tekWidgetClass ((WidgetClass)&tekClassRec)

static Boolean Tfailed = FALSE;

TekWidget CreateTekWidget ()
{
    Widget tekshellwidget;
    extern Arg ourTopLevelShellArgs[];
    extern int number_ourTopLevelShellArgs;

    /* this causes the Initialize method to be called */
    tekshellwidget = XtCreateApplicationShell ("tektronix",
					       topLevelShellWidgetClass,
					       ourTopLevelShellArgs, 
					       number_ourTopLevelShellArgs);
    /* this causes the Realize method to be called */
    tekWidget = (TekWidget) XtCreateManagedWidget ("tek4014", tekWidgetClass,
						   tekshellwidget, NULL, 0);
    return (tekWidget);
}


int TekInit ()
{
    if (Tfailed) return (0);
    if (tekWidget) return (1);
    if (CreateTekWidget()) {
	return (1);
    }
    return (0);
}

Tekparse()
{
	register TScreen *screen = &term->screen;
	register int c, x, y;
	char ch;
	int Tinput();

	for( ; ; )
		switch(Tparsestate[c = input()]) {
		 case CASE_REPORT:
			/* report address */
			if(screen->TekGIN) {
				TekGINoff();
				TekEnqMouse(0);
			} else {
				c = 064;	/* has hard copy unit */
				if(screen->margin == MARGIN2)
					c |= 02;
				TekEnq(c, screen->cur_X, screen->cur_Y);
			}
			TekRecord->ptr[-1] = NAK; /* remove from recording */
			Tparsestate = curstate;
			break;

		 case CASE_VT_MODE:
			/* special return to vt102 mode */
			Tparsestate = curstate;
			TekRecord->ptr[-1] = NAK; /* remove from recording */
			if(screen->logging) {
				FlushLog(screen);
				screen->logstart = buffer;
			}
			return;

		 case CASE_SPT_STATE:
			/* Enter Special Point Plot mode */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate = Tspttable;
			break;

		 case CASE_GIN:
			/* Do Tek GIN mode */
			screen->TekGIN = &TekRecord->ptr[-1];
				/* Set cross-hair cursor raster array */
			if(GINcursor = make_tcross(term,
				 screen->mousecolor,
			    term->core.background_pixel))
				XDefineCursor(
				    screen->display,
				    TShellWindow,
				    GINcursor);
			Tparsestate = Tbyptable;	/* Bypass mode */
			break;

		 case CASE_BEL:
			/* BEL */
			if(screen->TekGIN)
				TekGINoff();
			if(!TekRefresh)
				Bell();
			Tparsestate = curstate;	/* clear bypass condition */
			break;

		 case CASE_BS:
			/* BS */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate;	/* clear bypass condition */
			TCursorBack();
			break;

		 case CASE_PT_STATE:
			/* Enter Tek Point Plot mode */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate = Tpttable;
			break;

		 case CASE_PLT_STATE:
			/* Enter Tek Plot mode */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate = Tplttable;
			if((c = input()) == BEL)
				screen->pen = PENDOWN;
			else {
				unput(c);
				screen->pen = PENUP;
			}
			break;

		 case CASE_TAB:
			/* HT */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate;	/* clear bypass condition */
			TCursorForward();
			break;

		 case CASE_IPL_STATE:
			/* Enter Tek Incremental Plot mode */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate = Tipltable;
			break;

		 case CASE_ALP_STATE:
			/* Enter Tek Alpha mode from any other mode */
			if(screen->TekGIN)
				TekGINoff();
			/* if in one of graphics states, move alpha cursor */
			if(nplot > 0)	/* flush line Tbuffer */
				TekFlush();
			Tparsestate = curstate = Talptable;
			break;

		 case CASE_UP:
			/* cursor up */
			if(screen->TekGIN)
				TekGINoff();
			Tparsestate = curstate;	/* clear bypass condition */
			TCursorUp();
			break;

		 case CASE_COPY:
			/* make copy */
			if(screen->TekGIN)
				TekGINoff();
			TekCopy();
			TekRecord->ptr[-1] = NAK; /* remove from recording */
			Tparsestate = curstate;	/* clear bypass condition */
			break;

		 case CASE_PAGE:
			/* Page Function */
			if(screen->TekGIN)
				TekGINoff();
			XClearWindow(screen->display, TWindow(screen));
			TekPage();	/* clear bypass condition */
			break;

		 case CASE_BES_STATE:
			/* Byp: an escape char */
			Tparsestate = Tbestable;
			break;

		 case CASE_BYP_STATE:
			/* set bypass condition */
			Tparsestate = Tbyptable;
			break;

		 case CASE_IGNORE:
			/* Esc: totally ignore CR, ESC, LF, ~ */
			break;

		 case CASE_ASCII:
			/* Select ASCII char set */
			/* ignore for now */
			Tparsestate = curstate;
			break;

		 case CASE_APL:
			/* Select APL char set */
			/* ignore for now */
			Tparsestate = curstate;
			break;

		 case CASE_CHAR_SIZE: 
			/* character size selector */
			{
			   extern   void TUpdateProperty();
			   int      new_font = (c & 03);

			   if (screen->Tproperty)
			       TUpdateProperty(screen->cur.fontsize, new_font);
		           TekSetGCFont (screen->cur.fontsize = new_font);
			   Tparsestate = curstate;
			   break;
			}

		 case CASE_BEAM_VEC:
			/* beam and vector selector */
			/* only line types */
			if((c &= LINEMASK) != screen->cur.linetype) {
				if(nplot > 0)
					TekFlush();
				screen->cur.linetype = c;
			}
			Tparsestate = curstate;
			break;

		 case CASE_CURSTATE:
			Tparsestate = curstate;
			break;

		 case CASE_PENUP:
			/* Ipl: penup */
			screen->pen = PENUP;
			break;

		 case CASE_PENDOWN:
			/* Ipl: pendown */
			screen->pen = PENDOWN;
			break;

		 case CASE_IPL_POINT:
			/* Ipl: point */
			x = screen->cur_X;
			y = screen->cur_Y;
			if(c & NORTH)
				y++;
			else if(c & SOUTH)
				y--;
			if(c & EAST)
				x++;
			else if(c & WEST)
				x--;
			if(screen->pen == PENDOWN)
				TekDraw(x, y);
			else
				TekMove(x, y);
			break;

		 case CASE_PLT_VEC:
			/* Plt: vector */
			unput(c);
			if(getpoint()) {
				if(screen->pen == PENDOWN)
					TekDraw(screen->cur.x, screen->cur.y);
				else
					TekMove(screen->cur.x, screen->cur.y);
				screen->pen = PENDOWN;
			}
			break;

		 case CASE_PT_POINT:
			/* Pt: point */
			unput(c);
			if(getpoint()) {
				TekMove(screen->cur.x, screen->cur.y);
				TekDraw(screen->cur.x, screen->cur.y);
			}
			break;

		 case CASE_SPT_POINT:
			/* Spt: point */
			/* ignore intensity character in c */
			if(getpoint()) {
				TekMove(screen->cur.x, screen->cur.y);
				TekDraw(screen->cur.x, screen->cur.y);
			}
			break;

		 case CASE_CR:
			/* CR */
			if(screen->TekGIN)
				TekGINoff();
			if(nplot > 0)	/* flush line Tbuffer */
				TekFlush();
			screen->cur_X = screen->margin == MARGIN1 ? 0 :
			 TEKWIDTH / 2;
			Tparsestate = curstate = Talptable;
			break;

		 case CASE_ESC_STATE:
			/* ESC */
			Tparsestate = Tesctable;
			break;

		 case CASE_LF:
			/* LF */
			if(screen->TekGIN)
				TekGINoff();
			TCursorDown();
			if (!TekRefresh &&
				(screen->display->qlen > 0 ||
			  	 GetBytesAvailable (screen->display->fd) > 0))
			    xevents();
			break;

		 case CASE_SP:
			/* SP */
			TCursorForward();
			break;

		 case CASE_PRINT:
			/* printable character */
			ch = c;
			c = screen->cur.fontsize;

			XDrawString(
			    screen->display,
			    TWindow(screen), 
			    screen->TnormalGC,
			    (int)(screen->cur_X * TekScale(screen)) + screen->border,
			    (int)((TEKHEIGHT + TEKTOPPAD - screen->cur_Y) * TekScale(screen)) + screen->border,
			    &ch,
			    1);
			TCursorForward();
			break;
		 case CASE_OSC:
			/* do osc escape */
			do_osc(Tinput);
			Tparsestate = curstate;
			break;
/* FLH TEK-VT switch */
       case CASE_VT_SWITCH:
			{
				void rem_esc_seq();

         	/* remove tek window and switch to vt mode */
         Tparsestate = curstate;
				/* remove tek escape sequence from input buffer */
				/* and current Tek buffer */	
			rem_esc_seq();
         xmenu_hide_tek_window(NULL,NULL,NULL);
         break;
			}
/* FLH TEK-VT switch */
		}
}			


/* FLH TEK-VT switch */
/* remove ESC7 (tek to vt mode) escape sequence from buffers */
/* to avoid replaying sequence when reentering tek mode */
/* and avoid having sequence show up in vt input buffer window */
static void rem_esc_seq()
{
	Char *lptr, *rptr;
	int move_cnt;

			/* remove ESC7 sequence from current Tek Record */
	TekRecord->count -= 2;
	*(TekRecord->ptr)-- = '\0';
	*(TekRecord->ptr)-- = '\0';
			/* remove ESC7 sequence from input buffer */
			/* Tbptr points to first char beyond ESC7 in input buf */
			/* Tbcnt is number of chars following ESC7 in input buf*/
/* UNNECESSARY
	move_cnt = Tbcnt;
	rptr = Tbptr;
	Tbptr--;
	Tbptr--;
	lptr = Tbptr;
	while (move_cnt-- > 0)
		*lptr++ = *rptr++;
*/
}
/* FLH TEK-VT switch */
	

TekSetGCFont (size)
    int size;  /* one of SMALLFONT, TWOFONT, THREEFONT, or LARGEFONT */
{
    register TScreen *screen = &term->screen;
    Font fid = screen->Tfont[size]->fid;

#ifdef TIOCSWINSZ
    struct  winsize ws;
#endif

    if (fid == DefaultGCID) 
       /* we didn't succeed in opening a real font
	  for this size.  Instead, use server default. */
       XCopyGC (screen->display,
	  DefaultGC(screen->display, DefaultScreen(screen->display)),
	  GCFont, screen->TnormalGC);
   else
       XSetFont (screen->display, screen->TnormalGC, fid);

#ifdef TIOCSWINSZ
    ws.ws_row = TekChar[size].nlines;
    ws.ws_col = TekChar[size].charsperline;
    ws.ws_xpixel = TFullWidth(screen);
    ws.ws_ypixel = TFullHeight(screen);

    if (ioctl (screen->respond, TIOCSWINSZ, &ws) == -1) {
#if !defined(I18N)
        (void) fprintf(stderr, "TIOCSWINSZ failed in TekSetGCFont\n"); 
        perror("        Reason");
#else
	OlVaDisplayWarningMsg(screen->display, OleNioctl, OleTwinSz,
		OleCOlClientXtermMsgs, OleMioctl_winSz, NULL);
        perror(OlGetMessage(screen->display, NULL, 0, OleNperror,
				OleTreason, OleCOlClientXtermMsgs,
				OleMperror_reason, NULL) );

#endif
    }
#endif
}

static int rcnt;
static char *rptr;
static int Tselect_mask;

Tinput()
{
	register TScreen *screen = &term->screen;
	register int i;
	register TekLink *tek;
#ifndef MEMUTIL	
	extern char *malloc();
#endif	

	if(Tpushback > Tpushb)
		return(*--Tpushback);
	if(TekRefresh) {
		if(rcnt-- > 0)
			return(*rptr++);
		if(tek = TekRefresh->next) {
			TekRefresh = tek;
			rcnt = tek->count - 1;
			rptr = tek->data;
			return(*rptr++);
		}
		TekRefresh = (TekLink *)0;
				/* 
				 * We only want to call longjmp if dorefresh is still
				 * on the stack.  This will not be true if a Focus in
				 * event directly followed an Expose event.  In this
				 * case, the xevents loop (misc.c) will have called
				 * end_vt_mode (menu.c), thus doing a longjmp() to
				 * VTEnd, and removing dorefresh from the stack.	
				 */
		if (inside_TekExpose)
			longjmp(Tekjump, 1);
		else
			XDefineCursor(screen->display, TShellWindow,
	 			(screen->TekGIN && GINcursor) ? GINcursor : screen->arrow);
	}
again:
	if(Tbcnt-- <= 0) {
		if(nplot > 0)	/* flush line Tbuffer */
			TekFlush();
		Tselect_mask = pty_mask;	/* force a read */
		for( ; ; ) {
			if(Tselect_mask & pty_mask) {
				if(screen->logging)
					FlushLog(screen);
				if((Tbcnt = read(screen->respond,
				 Tbptr = Tbuffer, BUF_SIZE)) < 0) {
					if(errno == EIO)
						Exit (0);
					else if(errno != EWOULDBLOCK)
						Panic(
#if !defined(I18N)
				 "Tinput:read returned unexpected error (%d)\n",
#else
				 OlGetMessage(screen->display, NULL, 0,
				   		OleNpanic, OleTpanic_msg11,
				   		OleCOlClientXtermMsgs,
				   		OleMpanic11, NULL),
#endif
						 errno);
				} else if(Tbcnt == 0)
					Panic(
#if !defined(I18N)
					      "Tinput: read returned zero\n",
#else
					      OlGetMessage(screen->display,
							   NULL, 0,
				   			   OleNpanic,
							   OleTpanic_msg12,
				   			  OleCOlClientXtermMsgs,
				   			   OleMpanic12, NULL),
#endif
						0);
				else {
					break;
				}
			}
			if (Ttoggled && curstate == Talptable) {
				TCursorToggle(TOGGLE);
				Ttoggled = FALSE;
			}
			if(QLength(screen->display))
				Tselect_mask = X_mask;
			else {
				XFlush(screen->display);
				Tselect_mask = Select_mask;
#ifdef SYSV
				if((i = XSelect(max_plus1, &Tselect_mask,
					(int *)NULL, (int *)NULL,
					(struct timeval *)NULL)) < 0){
#else
				if((i = select(max_plus1, &Tselect_mask,
					(int *)NULL, (int *)NULL,
					(struct timeval *)NULL)) < 0){
#endif
					if (errno != EINTR)
						SysError(ERROR_TSELECT);
					continue;
				}
			}
			if(Tselect_mask & X_mask) {
				xevents();
				if(Tbcnt > 0)
					goto again;
			}
		}
		Tbcnt--;
		if (!Ttoggled && curstate == Talptable) {
			TCursorToggle(TOGGLE);
			Ttoggled = TRUE;
		}
	}
	if((tek = TekRecord)->count >= TEK_LINK_BLOCK_SIZE) {
		if((TekRecord = tek->next = (TekLink *)malloc(sizeof(TekLink)))
		 == (TekLink *)0)
#if !defined(I18N)
			Panic("Tinput: malloc error (%d)\n", errno);
#else
			Panic(OlGetMessage(term->screen.display, NULL, 0,
				   	OleNpanic, OleTpanic_msg13,
				   	OleCOlClientXtermMsgs,
				   	OleMpanic13, NULL), 0);
#endif
		tek = tek->next;
		tek->next = (TekLink *)0;
		tek->count = 0;
		tek->ptr = tek->data;
	}
	tek->count++;
	return(*tek->ptr++ = *Tbptr++);
}

/* this should become the Tek Widget's Resize proc */
static void TekConfigure(w)
    Widget w;
{
    register TScreen *screen = &term->screen;
    register int border = 2 * screen->border;
    register double d;

    if (TWindow(screen)) XClearWindow(screen->display, TWindow(screen));
    TWidth(screen) = w->core.width - border;
    THeight(screen) = w->core.height - border;
    TekScale(screen) = (double)TWidth(screen) / TEKWIDTH;
    if((d = (double)THeight(screen) / (TEKHEIGHT + TEKTOPPAD + TEKBOTTOMPAD))
       < TekScale(screen))
      TekScale(screen) = d;
    TFullWidth(screen) = w->core.width;
    TFullHeight(screen) = w->core.height;
}

/* this should become the Tek Widget's Expose proc */
/* need to use compress_events = TRUE so you don't need to 
   look at the "count" in the exposure event ! */
/*ARGSUSED*/
void TekExpose(w, event, region)
Widget w;
XExposeEvent *event;
Region region;
{
	register TScreen *screen = &term->screen;

#ifdef lint
	region = region;
#endif

		/*
		 *	Set flag to indicate that the Tek Expose routine
		 * is on the call stack.  See Tinput().
		 */
	inside_TekExpose = 1;

	if(!Ttoggled)
	    TCursorToggle(CLEAR);
	Ttoggled = TRUE;
	Tpushback = Tpushb;
	screen->cur_X = 0;
	screen->cur_Y = TEKHOME;
	screen->cur = screen->page;
        TekSetGCFont (screen->cur.fontsize);
	screen->margin = MARGIN1;
	if(screen->TekGIN) {
		screen->TekGIN = NULL;
		TekGINoff();
	}
	TekRefresh = &Tek0;
	rptr = TekRefresh->data;
	rcnt = TekRefresh->count;
	Tparsestate = curstate = Talptable;
	if(!screen->waitrefresh)
		dorefresh();
		/* set flag to indicate that Tek Expose routine is no
		 * longer on the call stack.  See Tinput().
		 */
	inside_TekExpose = 0;
}

dorefresh()
{
	register TScreen *screen = &term->screen;
	static Cursor wait_cursor = None;

	if (wait_cursor == None)
            wait_cursor = make_wait(term, screen->mousecolor, term->core.background_pixel);
        XDefineCursor(screen->display, TShellWindow, wait_cursor);
	XFlush(screen->display);
	if(!setjmp(Tekjump))
		Tekparse();
	XDefineCursor(screen->display, TShellWindow,
	 (screen->TekGIN && GINcursor) ? GINcursor : screen->arrow);
}

TekPage()
{
	register TScreen *screen = &term->screen;
	register TekLink *tek, *tek2;

	screen->cur_X = 0;
	screen->cur_Y = TEKHOME;
	screen->margin = MARGIN1;
	screen->page = screen->cur;
	if(screen->TekGIN)
		TekGINoff();
	tek = TekRecord = &Tek0;
	tek->count = 0;
	tek->ptr = tek->data;
	if(tek = tek->next)
		do {
			tek2 = tek->next;
			free((char *)tek);
		} while(tek = tek2);
	TekRecord->next = (TekLink *)0;
	TekRefresh = (TekLink *)0;
	Ttoggled = TRUE;
	Tparsestate = curstate = Talptable;	/* Tek Alpha mode */
	TekSetGCFont (screen->cur.fontsize);
}

#define	EXTRABITS	017
#define	FIVEBITS	037
#define	HIBITS		(FIVEBITS << SHIFTHI)
#define	LOBITS		(FIVEBITS << SHIFTLO)
#define	SHIFTHI		7
#define	SHIFTLO		2
#define	TWOBITS		03

getpoint()
{
	register int c, x, y, e, lo_y = 0;
	register TScreen *screen = &term->screen;

	x = screen->cur.x;
	y = screen->cur.y;
	for( ; ; ) {
		if((c = input()) < ' ') {	/* control character */
			unput(c);
			return(0);
		}
		if(c < '@') {	/* Hi X or Hi Y */
			if(lo_y) {	/* seen a Lo Y, so this must be Hi X */
				x &= ~HIBITS;
				x |= (c & FIVEBITS) << SHIFTHI;
				continue;
			}
			/* else Hi Y */
			y &= ~HIBITS;
			y |= (c & FIVEBITS) << SHIFTHI;
			continue;
		}
		if(c < '`') {	/* Lo X */
			x &= ~LOBITS;
			x |= (c & FIVEBITS) << SHIFTLO;
			screen->cur.x = x;
			screen->cur.y = y;
			return(1);	/* OK */
		}
		/* else Lo Y */
		if(lo_y) {	/* seen a Lo Y, so other must be extra bits */
			e = (y >> SHIFTLO) & EXTRABITS;
			x &= ~TWOBITS;
			x |= e & TWOBITS;
			y &= ~TWOBITS;
			y |= (e >> SHIFTLO) & TWOBITS;
		}
		y &= ~LOBITS;
		y |= (c & FIVEBITS) << SHIFTLO;
		lo_y++;
	}
}

TCursorBack()
{
	register TScreen *screen = &term->screen;
	register struct Tek_Char *t;
	register int x, l;

	x = ( screen->cur_X -=
		(t = &TekChar[screen->cur.fontsize])->hsize
	    );

	if(screen->margin == MARGIN1 && x < 0 || screen->margin == MARGIN2
	 && x < TEKWIDTH / 2) {
		if((l = (screen->cur_Y + (t->vsize - 1)) / t->vsize + 1) >=
		 t->nlines) {
			screen->margin = !screen->margin;
			l = 0;
		}
		screen->cur_Y = l * t->vsize;
		screen->cur_X = (t->charsperline - 1) * t->hsize;
	}
}

TCursorForward()
{
	register TScreen *screen = &term->screen;
	register struct Tek_Char *t;
	register int l;

	if( ( screen->cur_X +=
		( t = &TekChar[screen->cur.fontsize])->hsize
	    ) > TEKWIDTH
	  ) {
		if((l = screen->cur_Y / t->vsize - 1) < 0) {
			screen->margin = !screen->margin;
			l = t->nlines - 1;
		}
		screen->cur_Y = l * t->vsize;
		screen->cur_X = screen->margin == MARGIN1 ? 0 : TEKWIDTH / 2;
	}
}

TCursorUp()
{
	register TScreen *screen = &term->screen;
	register struct Tek_Char *t;
	register int l;

	t = &TekChar[screen->cur.fontsize];

	if((l = (screen->cur_Y + (t->vsize - 1)) / t->vsize + 1) >= t->nlines) {
		l = 0;
		if((screen->margin = !screen->margin) != MARGIN1) {
			if(screen->cur_X < TEKWIDTH / 2)
				screen->cur_X += TEKWIDTH / 2;
		} else if(screen->cur_X >= TEKWIDTH / 2)
			screen->cur_X -= TEKWIDTH / 2;
	}
	screen->cur_Y = l * t->vsize;
}

TCursorDown()
{
	register TScreen *screen = &term->screen;
	register struct Tek_Char *t;
	register int l;

	t = &TekChar[screen->cur.fontsize];

	if((l = screen->cur_Y / t->vsize - 1) < 0) {
		l = t->nlines - 1;
		if((screen->margin = !screen->margin) != MARGIN1) {
			if(screen->cur_X < TEKWIDTH / 2)
				screen->cur_X += TEKWIDTH / 2;
		} else if(screen->cur_X >= TEKWIDTH / 2)
			screen->cur_X -= TEKWIDTH / 2;
	}
	screen->cur_Y = l * t->vsize;
}

TekDraw (x, y)
int x, y;
{
	register TScreen *screen = &term->screen;

	if(nplot == 0 || T_lastx != screen->cur_X || T_lasty != screen->cur_Y) {
		/*
		 * We flush on each unconnected line segment if the line
		 * type is not solid.  This solves a bug in X when drawing
		 * points while the line type is not solid.
		 */
		if(nplot > 0 && screen->cur.linetype != SOLIDLINE)
			TekFlush();
	}
	AddToDraw(screen->cur_X, screen->cur_Y, x, y);
	T_lastx = screen->cur_X = x;
	T_lasty = screen->cur_Y = y;
}

AddToDraw(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	register TScreen *screen = &term->screen;
	register XSegment *lp;

	if(nplot >= MAX_PTS) {
		TekFlush();
	}
	lp = line_pt++;
	lp->x1 = x1 = x1 * TekScale(screen) + screen->border;
	lp->y1 = y1 = (TEKHEIGHT + TEKTOPPAD - y1) * TekScale(screen) +
	 screen->border;
	lp->x2 = x2 = x2 * TekScale(screen) + screen->border;
	lp->y2 = y2 = (TEKHEIGHT + TEKTOPPAD - y2) * TekScale(screen) +
	 screen->border;
	nplot++;
}

TekFlush ()
{
	register TScreen *screen = &term->screen;

	XDrawSegments(screen->display, TWindow(screen), 
		((screen->cur.linetype == SOLIDLINE)?  screen->TnormalGC :
		 screen->linepat[screen->cur.linetype - 1]),
		 Tline, nplot);
	nplot = 0;
	line_pt = Tline;
}

TekGINoff()
{
	register TScreen *screen = &term->screen;
	
	XDefineCursor(screen->display, TShellWindow, screen->arrow);
	if(GINcursor)
		XFreeCursor(screen->display, GINcursor);
	if(screen->TekGIN) {
		*screen->TekGIN = CANCEL;	/* modify recording */
		screen->TekGIN = NULL;
	}
}

TekEnqMouse(c)
int c;
{
	register TScreen *screen = &term->screen;
	int mousex, mousey, rootx, rooty;
	unsigned int mask; /* XQueryPointer */
	Window root, subw;

	XQueryPointer(
	    screen->display, TWindow(screen), 
	    &root, &subw,
	    &rootx, &rooty,
	    &mousex, &mousey,
	    &mask);
	if((mousex = (mousex - screen->border) / TekScale(screen)) < 0)
		mousex = 0;
	else if(mousex >= TEKWIDTH)
		mousex = TEKWIDTH - 1;
	if((mousey = TEKHEIGHT + TEKTOPPAD - (mousey - screen->border) /
	     TekScale(screen)) < 0)
		mousey = 0;
	else if(mousey >= TEKHEIGHT)
		mousey = TEKHEIGHT - 1;
	TekEnq(c, mousex, mousey);
}

TekEnq (status, x, y)
int status;
register int x, y;
{
	register TScreen *screen = &term->screen;
	int pty = screen->respond;
	char cplot [5];

	/* Translate x and y to Tektronix code */
	cplot[1] = 040 | ((x >> SHIFTHI) & FIVEBITS);
	cplot[2] = 040 | ((x >> SHIFTLO) & FIVEBITS);
	cplot[3] = 040 | ((y >> SHIFTHI) & FIVEBITS);
	cplot[4] = 040 | ((y >> SHIFTLO) & FIVEBITS);
	if(cplot[0] = status)
		write (pty, cplot, 5);
	else
		write (pty, &cplot[1], 4);
}

TekRun()
{
	register TScreen *screen = &term->screen;
	register int i;
	
	if(!TWindow(screen) && !TekInit()) {
		if(VWindow(screen)) {
			screen->TekEmu = FALSE;
			return;
		}
		Exit(ERROR_TINIT);
	}
	if(!screen->Tshow) {
	    XtRealizeWidget (tekWidget->core.parent);
	    set_tek_visibility (TRUE);
	} 

	if(screen->select || screen->always_highlight)
		TekSelect();
	Tpushback = Tpushb;
	Tbptr = Tbuffer;
	for(i = Tbcnt = bcnt ; i > 0 ; i--)
		*Tbptr++ = *bptr++;
	Tbptr = Tbuffer;
	Ttoggled = TRUE;
	if(!setjmp(Tekend))
		Tekparse();
	if(!Ttoggled) {
		TCursorToggle(TOGGLE);
		Ttoggled = TRUE;
	}
	screen->TekEmu = FALSE;
	if (!screen->always_highlight)
	    TekUnselect ();
	reselectwindow (screen);
}

#define DOTTED_LENGTH 2
#define DOT_DASHED_LENGTH 4
#define SHORT_DASHED_LENGTH 2
#define LONG_DASHED_LENGTH 2

static int dash_length[TEKNUMLINES] = {
	DOTTED_LENGTH,
	DOT_DASHED_LENGTH,
	SHORT_DASHED_LENGTH,
	LONG_DASHED_LENGTH,
};

static char dotted[DOTTED_LENGTH] = {3, 1};
static char dot_dashed[DOT_DASHED_LENGTH] = {3, 4, 3, 1};
static char short_dashed[SHORT_DASHED_LENGTH] = {4, 4};
static char long_dashed[LONG_DASHED_LENGTH] = {4, 7};

static char *dashes[TEKNUMLINES] = {
	dotted,
	dot_dashed,
	short_dashed,
	long_dashed,
};



/*
 * The following is called the create the tekWidget
 */

static void TekInitialize(request, new)
    Widget request, new;
{
   /* look for focus related events on the shell, because we need
    * to care about the shell's border being part of our focus.
    */

/* FLH dynamic
 *
 *		Replace these event handlers for real-estate based input focus	
 *
    XtAddEventHandler(XtParent(new), EnterWindowMask, FALSE,
		      HandleEnterWindow, (XtPointer)NULL);
    XtAddEventHandler(XtParent(new), LeaveWindowMask, FALSE,
		      HandleLeaveWindow, (XtPointer)NULL);
*/
    putenv("TERM=tek4014");
}


static void TekRealize (gw, valuemaskp, values)
    Widget gw;
    XtValueMask *valuemaskp;
    XSetWindowAttributes *values;
{
    TekWidget tw = (TekWidget) gw;
    register TScreen *screen = &term->screen;
    register int i;
    register TekLink *tek;
    register double d;
    register int border = 2 * screen->border;
    int pr;
    XGCValues gcv;
    int winX, winY;
    unsigned int width, height;
    char Tdefault[32];
#ifndef MEMUTIL    
    extern char *malloc();
#endif    

	 Cardinal n = 0;
	 Arg args[8];

    tw->core.border_pixel = term->core.border_pixel;

    if (!(screen->Tfont[SMALLFONT] = XLoadQueryFont (screen->display,
						     SMALLFONTNAME))) {
#if !defined(I18N)
	fprintf(stderr, "%s: Could not get font %s; using server default\n",
		xterm_name, SMALLFONTNAME);
#else
	OlVaDisplayWarningMsg(screen->display, OleNfont, OleTbadFont,
		OleCOlClientXtermMsgs, OleMfont_badFont, NULL);
#endif
	screen->Tfont[SMALLFONT] = XQueryFont (screen->display, DefaultGCID);
    }
    if((Tbuffer = (Char *)malloc(BUF_SIZE)) == NULL ||
       (Tpushb = (Char *)malloc(10)) == NULL ||
       (Tline = (XSegment *)malloc(MAX_VTX * sizeof(XSegment))) == NULL) {
#if !defined(I18N)
	fprintf (stderr, "%s: Not enough core for Tek mode\n", xterm_name);
#else
	OlVaDisplayWarningMsg(screen->display, OleNspace, OleTtekMode,
		OleCOlClientXtermMsgs, OleMspace_tekMode, NULL);
#endif
	goto mallocfailed;
    }

    screen->xorplane = 1;

    screen->Tbackground = screen->background;
    screen->Tforeground = screen->foreground;
    screen->Tcursorcolor = screen->foreground;

    if (term->misc.T_geometry == NULL) {
	int defwidth, defheight;

	if (term->misc.tekSmall) {
	    defwidth = TEKMINWIDTH;
	    defheight = TEKMINHEIGHT;
	} else {
	    defwidth = TEKDEFWIDTH;
	    defheight = TEKDEFHEIGHT;
	}
	sprintf (Tdefault, "=%dx%d", defwidth + border, defheight + border);
	term->misc.T_geometry = Tdefault;
    }

    winX = 1;
    winY = 1;
    width = TEKDEFWIDTH + border;
    height = TEKDEFHEIGHT + border;

    pr = XParseGeometry(term->misc.T_geometry, &winX, &winY, &width, &height);
    if ((pr & XValue) && (pr & XNegative))
/* FLH dynamic
 *
 *		term is now 2 levels below the shell widget
 *		must include borders of shell and container
 */
      winX += DisplayWidth(screen->display, DefaultScreen(screen->display))
              	- width
					- (term->core.parent->core.border_width * 2)
					- (term->core.parent->core.parent->core.border_width * 2);
/* FLH dynamic */

    if ((pr & YValue) && (pr & YNegative))
/* FLH dynamic
 *
 *		term is now 2 levels below the shell widget
 *		must include borders of shell and container
 */
      winY += DisplayHeight(screen->display, DefaultScreen(screen->display))
						- height 
						- (term->core.parent->core.border_width * 2)
						- (term->core.parent->core.parent->core.border_width * 2);
/* FLH dynamic */
  
    /* set up size hints */
	XtSetArg(args[n],XtNminWidth, (int) (TEKMINWIDTH + border));n++;	
	XtSetArg(args[n],XtNminHeight, (int) (TEKMINHEIGHT + border));n++;
	XtSetArg(args[n],XtNwidthInc, 1);n++; 
	XtSetArg(args[n],XtNheightInc, 1);n++; 
	XtSetArg(args[n],XtNx, (Position) winX);n++; 
	XtSetArg(args[n],XtNy, (Position) winY);n++; 
	XtSetArg(args[n],XtNwidth, (Dimension) width);n++; 
	XtSetArg(args[n],XtNheight, (Dimension) height);n++; 

    (void) XtMakeResizeRequest ((Widget) tw, width, height,
				&tw->core.width, &tw->core.height);

    /* XXX This is bogus.  We are parsing geometries too late.  This
     * is information that the shell widget ought to have before we get
     * realized, so that it can do the right thing.
     */
	if ((XValue&pr) || (YValue&pr))
      XMoveWindow (XtDisplay(tw), tw->core.parent->core.window,
		   winX, winY);

	/* call to XSetWMNormalHints has been replaced by a call
	 * to XtSetValues of the appropriate resource to prevent
	 *	the intrinsics from overwriting the WM resource values
	 *	set by XSetWMNormalHints
	 *	with the (zero) values cached by libXt
    */
	XtSetValues(tw->core.parent,args,n);

    values->win_gravity = NorthWestGravity;
    values->background_pixel = screen->Tbackground;

    if((tw->core.window = TWindow(screen) = 
	XCreateWindow (screen->display,
		       tw->core.parent->core.window,
		       tw->core.x, tw->core.y,
		       tw->core.width, tw->core.height, tw->core.border_width,
		       (int) tw->core.depth,
		       InputOutput, CopyFromParent,
		       ((*valuemaskp)|CWBackPixel|CWWinGravity),
		       values)) == NULL) {
#if !defined(I18N)
	fprintf(stderr, "%s: Can't create Tek window\n", xterm_name);
#else
	OlVaDisplayWarningMsg(screen->display, OleNcreate, OleTbadWindow1,
		OleCOlClientXtermMsgs, OleMcreate_badWindow1, NULL);
#endif
	free((char *)Tline);
      mallocfailed:
	if(Tpushb) free((char *)Tpushb);
	if(Tbuffer) free((char *)Tbuffer);
	XFreeFont(screen->display, screen->Tfont[SMALLFONT]);
	Tfailed = TRUE;
	return;
    }

    screen->Tbox = T_box;

    TFullWidth(screen) = width;
    TFullHeight(screen) = height;
    TWidth(screen) = width - border;
    THeight(screen) = height - border;
    TekScale(screen) = (double)TWidth(screen) / TEKWIDTH;
    if((d = (double)THeight(screen) / (TEKHEIGHT + TEKTOPPAD +
				       TEKBOTTOMPAD)) < TekScale(screen))
      TekScale(screen) = d;
    
    screen->tobaseline[SMALLFONT] = screen->Tfont[SMALLFONT]->ascent;

    if (!(screen->Tfont[THREEFONT] = XLoadQueryFont(screen->display,
						    THREEFONTNAME)))
      screen->Tfont[THREEFONT] = screen->Tfont[SMALLFONT];
    screen->tobaseline[THREEFONT] = screen->Tfont[THREEFONT]->ascent;

    if (!(screen->Tfont[TWOFONT] = XLoadQueryFont(screen->display,
						  TWOFONTNAME)))
      screen->Tfont[TWOFONT] = screen->Tfont[THREEFONT];
    screen->tobaseline[TWOFONT] = screen->Tfont[TWOFONT]->ascent;

    if (!(screen->Tfont[LARGEFONT] = XLoadQueryFont(screen->display,
						    LARGEFONTNAME)))
      screen->Tfont[LARGEFONT] = screen->Tfont[TWOFONT];
    screen->tobaseline[LARGEFONT] = screen->Tfont[LARGEFONT]->ascent;

    screen->cur.fontsize = LARGEFONT;	/* set large font	*/

    gcv.graphics_exposures = TRUE;	/* default */
    gcv.font = screen->Tfont[screen->cur.fontsize]->fid;
    gcv.foreground = screen->Tforeground;
    gcv.background = screen->Tbackground;
    
    /* if font wasn't successfully opened, then gcv.font will contain
       the Default GC's ID, meaning that we must use the server default font. 
     */
    TEKgcFontMask = (gcv.font == DefaultGCID) ? 0 : GCFont;
    screen->TnormalGC = XCreateGC (screen->display, TWindow(screen), 
				   (TEKgcFontMask|GCGraphicsExposures|
				    GCForeground|GCBackground), &gcv);

    gcv.function = GXinvert;
    gcv.plane_mask = screen->xorplane = (screen->Tbackground ^
					 screen->Tcursorcolor);
    screen->TcursorGC = XCreateGC (screen->display, TWindow(screen), 
				   (GCFunction|GCPlaneMask), &gcv);

    gcv.foreground = screen->Tforeground;
    gcv.line_style = LineOnOffDash;
    for(i = 0 ; i < TEKNUMLINES ; i++) {
	screen->linepat[i] = XCreateGC (screen->display, TWindow(screen),
					(GCForeground|GCLineStyle), &gcv); 
	XSetDashes (screen->display, screen->linepat[i], 0,
		    dashes[i], dash_length[i]);
    }

    TekBackground(screen);

    gcv.function = GXcopyInverted;

    screen->margin = MARGIN1;		/* Margin 1		*/
    screen->TekGIN = FALSE;			/* GIN off		*/


    XDefineCursor(screen->display, TShellWindow, screen->curs);
    TekUnselect ();

    {	/* there's gotta be a better way... */
	static Arg args[] = {
	    {XtNtitle, NULL},
	    {XtNiconName, NULL},
	};
	char *icon_name, *title, *tek_icon_name, *tek_title;

	args[0].value = (XtArgVal)&icon_name;
	args[1].value = (XtArgVal)&title;
	if (term != NULL)
/*		FLH dynamic
 *
 *		term is now 2 levels below the shell
 */
	    XtGetValues (VShellWidget, args, 2);
/* FLH dynamic */
	else
	    XtGetValues (tw->core.parent, args, 2);
	tek_icon_name = XtMalloc(strlen(icon_name)+7);
	strcpy(tek_icon_name, icon_name);
#if !defined(I18N)
	strcat(tek_icon_name, "(Tek)");
#else
	strcat(tek_icon_name,
			OlGetMessage(screen->display, NULL, 0, OleNtitle,
				OleTtek, OleCOlClientXtermMsgs, OleMtitle_tek,
				NULL));
#endif

	tek_title = XtMalloc(strlen(title)+7);
	strcpy(tek_title, title);
#if !defined(I18N)
	strcat(tek_title, "(Tek)");
#else
	strcat(tek_title,
			OlGetMessage(screen->display, NULL, 0, OleNtitle,
				OleTtek, OleCOlClientXtermMsgs, OleMtitle_tek,
				NULL));
#endif
	args[0].value = (XtArgVal)tek_icon_name;
	args[1].value = (XtArgVal)tek_title;
	XtSetValues (tw->core.parent, args, 2);
	XtFree( tek_icon_name );
	XtFree( tek_title );
    }

    tek = TekRecord = &Tek0;
    tek->next = (TekLink *)0;
    tek->count = 0;
    tek->ptr = tek->data;
    Tpushback = Tpushb;
    Tbptr = Tbuffer;
    screen->cur_X = 0;
    screen->cur_Y = TEKHOME;
    line_pt = Tline;
    Ttoggled = TRUE;
}

TekReverseVideo(screen)
register TScreen *screen;
{
	register int i;
	XGCValues gcv;
	 

	i = screen->Tbackground;
	screen->Tbackground = screen->Tforeground;
	screen->Tforeground = i;
	
	XSetForeground(screen->display, screen->TnormalGC, 
	 screen->Tforeground);
	XSetBackground(screen->display, screen->TnormalGC, 
	 screen->Tbackground);

	if (tekWidget) {
	    if (tekWidget->core.border_pixel == screen->Tbackground) {
		tekWidget->core.border_pixel = screen->Tforeground;
		tekWidget->core.parent->core.border_pixel =
		  screen->Tforeground;
		if (tekWidget->core.parent->core.window)
		  XSetWindowBorder (screen->display,
				    tekWidget->core.parent->core.window,
				    tekWidget->core.border_pixel);
	    }
	}

	for(i = 0 ; i < TEKNUMLINES ; i++) {
		XSetForeground(screen->display, screen->linepat[i], 
		 screen->Tforeground);
	}

	screen->Tcursorcolor = screen->Tforeground;

        gcv.foreground = screen->Tforeground;
	gcv.plane_mask = screen->xorplane = (screen->Tbackground ^
					     screen->Tcursorcolor);
	XChangeGC (screen->display, screen->TcursorGC, 
		   (GCForeground|GCPlaneMask), &gcv);
	TekBackground(screen);
}

TekBackground(screen)
register TScreen *screen;
{
	if(TWindow(screen))
		XSetWindowBackground(screen->display, TWindow(screen), 
		 screen->Tbackground);
}

/*
 * Toggles cursor on or off at cursor position in screen.
 */
TCursorToggle(toggle)
int toggle;
{
	register TScreen *screen = &term->screen;
	register int c, x, y;
	register T_fontsize *Tf;

	if (!screen->Tshow) return;

	c = screen->cur.fontsize;
	Tf = &Tfontsize[c];

	x = (screen->cur_X * TekScale(screen)) + screen->border;
	y = ((TEKHEIGHT + TEKTOPPAD - screen->cur_Y) * TekScale(screen)) +
	 screen->border - screen->tobaseline[c];
	if (toggle) {
	   if (screen->select || screen->always_highlight) 
		XFillRectangle(
		    screen->display, TWindow(screen), screen->TcursorGC,
		    x, y,
		    (unsigned) Tf->Twidth, (unsigned) Tf->Theight);
	  else { /* fix to use different GC! */
		  screen->Tbox[c]->x = x;
		  screen->Tbox[c]->y = y ;
		  XDrawLines(screen->display, TWindow(screen), 
		    screen->TcursorGC,
		    screen->Tbox[c], NBOX, CoordModePrevious);
	   }
	} else {
	   if (screen->select || screen->always_highlight) 
		XClearArea(screen->display, TWindow(screen), x, y,
		    (unsigned) Tf->Twidth, (unsigned) Tf->Theight, FALSE);
	   else { 
		  XGCValues gcv;

		  screen->Tbox[c]->x = x;
		  screen->Tbox[c]->y = y ;
		  gcv.function = GXcopyInverted;
		  XChangeGC (screen->display, screen->TcursorGC,
			     GCFunction, &gcv);
		  XDrawLines(screen->display, TWindow(screen), 
		    screen->TcursorGC,
		    screen->Tbox[c], NBOX, CoordModePrevious);
		  gcv.function = GXinvert;
		  XChangeGC (screen->display, screen->TcursorGC,
			     GCFunction, &gcv);
	   }
	}
}

TekSelect()
{
	register TScreen *screen = &term->screen;

	if (tekWidget && TShellWindow)
	  XSetWindowBorder (screen->display, TShellWindow,
			    tekWidget->core.border_pixel);
}

TekUnselect()
{
	register TScreen *screen = &term->screen;

	if (tekWidget && TShellWindow && !screen->always_highlight)
	  XSetWindowBorderPixmap (screen->display, TShellWindow,
				  screen->graybordertile);
}

TekCopy()
{
	register TekLink *Tp;
	register int fd;
	register TScreen *screen = &term->screen;
	register struct tm *tp;
	long l;
	char buf[32];

	time(&l);
	tp = localtime(&l);
	sprintf(buf, "COPY%02d%02d%02d%02d%02d", tp->tm_year,
	 	tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min);
	if(access(buf, F_OK) >= 0) {	/* file exists */
		if(access(buf, W_OK) < 0) {
			Bell();
			return;
		}
	} else if(access(".", W_OK) < 0) {	/* can't write in directory */
		Bell();
		return;
	}
	if((fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		Bell();
		return;
	}
	chown(buf, screen->uid, screen->gid);
	sprintf(buf, "\033%c\033%c", screen->page.fontsize + '8',
	 screen->page.linetype + '`');
	write(fd, buf, 4);
	Tp = &Tek0; 
	do
		write(fd, (char *)Tp->data, Tp->count);
	while(Tp = Tp->next);
	close(fd);
}


/*ARGSUSED*/
void HandleSecure(w, event, params, num_params)
Widget w;
XEvent *event;			/* must be XMotionEvent */
String *params;			/* unused */
Cardinal *num_params;		/* unused */
{
}

/* FLH mouseless */
/* ActivateWidget: ignores all events */
static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
OLARG( Widget,	  w)
OLARG( OlVirtualName,	type)
OLGRA( XtPointer, call_data)
{
	Boolean consumed = False;

	switch (type){
		default:
			break;
	}
	return (consumed);
} /* end of ActivateWidget */

/*
 * Key
 *
 * The \fIKey\fR procedure is called whenever a key press event occurs
 * within the Tek window.	The procedure is called indirectly by
 * OlAction().  If a Meta key is *not* present, \fIKey\fR calls 
 * HandleTekKeyPressed() to handle the event.  If a Meta key *is*
 * present, \fIKey\fR calls HandleEightBitKeyPressed() to handle
 * the event.
 *
 */

static void Key(w, ve)
Widget w;
OlVirtualEvent ve;
{

		/* For Now, always call HandleTekKeyPressed */
	HandleTekKeyPressed(w,ve);
		/* NO Meta --> HandleTekKeyPressed() */
		/* Meta --> HandleEightBitKeyPressed() */
}
/* FLH mouseless-end */

/* FLH dynamic */
/*
 * FocusHandler
 */
static void
FocusHandler OLARGLIST((w, highlight_type))
   OLARG( Widget,    w)
   OLGRA( OlDefine,  highlight_type)
{
   register TScreen *screen = &term->screen;

#ifdef DEBUG 
	printf("TEK highlight handler\007\n");
#endif
   switch((int) highlight_type){
  
      case OL_IN:
            /* focus in, highlight cursor */
#ifdef DEBUG
			printf("FOCUS IN\n");
#endif
         selectwindow(screen,FOCUS);
         break;
      case OL_OUT:
            /* focus out, make cursor hollow */
#ifdef DEBUG
         printf("FOCUS OUT\n");
#endif
         unselectwindow(screen,FOCUS);
         break;
      default:
#ifdef DEBUG
         OlWarning("Xterm: Unidefined highlight_type\n");
#endif
         break;
   }

#ifdef DEBUG
   printf("^G");
   printf("focus widget is %x\n",w);
   printf("term is %x\n",term);
   printf("Shell is %x\n",VShellWidget);
   printf("scrollbar is %x\n",term->screen.scrollWidget);
#endif
}
/* FLH dynamic */

/*************************************************************************
 * AcceptFocus - If this widget can accept focus then it is set here
 *		 FALSE is returned if focus could not be set
 ***************************function*header*******************************/

static Boolean
AcceptFocus(w, time)
Widget	w;
Time *	time;
{
	register TScreen *screen = &term->screen;

#ifdef DEBUG
	printf("TEK AcceptFocus\007\n");
#endif

   if (OlCanAcceptFocus(w, *time)){
		OlSetInputFocus(w, RevertToNone, *time);
		focus_switched = TEK_MODE;
		return (True);
	}
	else
		return (False);
} /* AcceptFocus() */

/*
 *	SetValues: for dynamic modification of foreground
 *														background
 *														fontcolor
 */
static Boolean
SetValues(current, request, new, args, num_args)
Widget current;
Widget request;
Widget new;
ArgList args;
Cardinal *num_args;
{
		TekWidget current_tek	= (TekWidget) current;
		TekWidget new_tek			= (TekWidget) new;
		Pixel	fontcolor			= new_tek->primitive.font_color;
		Pixel background			= new_tek->core.background_pixel;
		Pixel cursorcolor			= new_tek->primitive.input_focus_color;
		Boolean realized			= XtIsRealized(current_tek);
		Boolean redisplay			= FALSE;
		TScreen *screen			= &(term->screen);
		Display *dpy				= screen->display;
		int i;
		XGCValues gcv;
		XColor cursor_colors[2];

#ifdef DEBUG
		printf("Tek SetValues called\n");
#endif

			/* check if foreground has changed */
		if (current_tek->primitive.font_color != fontcolor){
			/*
			 *	update tek font color and cursor color
			 */
			if (fontcolor != background){
					/*
					 * update text drawing color
					 */
				XSetForeground(dpy, screen->TnormalGC, fontcolor);
					/*
					 * update cursor planemask
					 */
				gcv.foreground = fontcolor;
   			gcv.plane_mask = screen->xorplane = (background ^ fontcolor);
   			XChangeGC (dpy, screen->TcursorGC, (GCForeground|GCPlaneMask), &gcv);
		
				for (i=0; i<TEKNUMLINES; i++)
					XSetForeground(dpy, screen->linepat[i], fontcolor);
				
					/* update local copy of color data */
				screen->Tforeground = fontcolor;
				screen->Tcursorcolor = fontcolor;
				redisplay = TRUE;
			}
			else
					/*
					 * restore original fontcolor
					 */
				new_tek->primitive.font_color = current_tek->primitive.font_color;
		}
			/* check if background has changed */
		if (current_tek->core.background_pixel != background){
			/*
			 *	update tek font color and cursor color
			 */
			if (background != screen->Tforeground){
				XSetBackground(dpy, screen->TnormalGC, background);
				XSetForeground(dpy, screen->TcursorGC, background);
				for (i=0; i<TEKNUMLINES; i++)
					XSetBackground(dpy, screen->linepat[i], background);
				/* 	
				 * update local copy of color data 
				 */
				screen->Tbackground = background;
				
				/* 
				 * mouse may be one of several pointers -- update them all
				 */
         	if (screen->mousecolor == screen->Tbackground)
					cursor_colors[0].pixel = screen->Tforeground;
				else 
					cursor_colors[0].pixel = screen->mousecolor;
				cursor_colors[1].pixel = screen->Tbackground;
				XQueryColors (dpy, DefaultColormap (dpy, DefaultScreen (dpy)),
									cursor_colors, 2);

				XRecolorCursor(dpy, screen->curs, cursor_colors, cursor_colors+1);
				if (GINcursor)
					XRecolorCursor(dpy, GINcursor, cursor_colors, cursor_colors+1);
				if (screen->arrow)
					XRecolorCursor(dpy, screen->arrow, cursor_colors, cursor_colors+1);
				redisplay = TRUE;
			}
			else
					/*
					 * restore original background color
					 */
				new_tek->core.background_pixel = current_tek->core.background_pixel;
		}
			/* check if fontcolor has changed */
		if (current_tek->primitive.input_focus_color != cursorcolor){
				/*
				 *	Do nothing, because the Tek cursor is the same color as
				 * the tek font
				 */
		}
		return (redisplay);
}

#endif /* TEK */
