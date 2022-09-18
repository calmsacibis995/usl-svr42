/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:button.c	1.54"
#endif

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/copyright.h>

#ifndef lint
static char rcs_id[] = "$Header: button.c,v 1.1 88/02/10 13:08:02 jim Exp $";
#endif	/* lint */

#include <X11/Xos.h>

#include <X11/Xlib.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include "error.h"
/* SS-color */
#include "xterm.h"
/* SS-color-end */
/* SS-mouse */
#include "mouse.h"
/* SS-mouse-end */

#include "ptyx.h"
#include "data.h"

/* SS-copy */
#include <X11/Xatom.h>

/* SS-copy-end */

#ifndef MEMUTIL
extern char *malloc();
#endif

/* SS-copy */
static char	*clip_contents = NULL;
Boolean		Have_hilite    = FALSE;
Boolean		Have_to_paste  = FALSE;
/* SS-copy-end */

#define KeyState(x) (((x) & (ShiftMask|ControlMask)) + (((x) & Mod1Mask) ? 2 : 0))
    /* adds together the bits:
        shift key -> 1
        meta key  -> 2
        control key -> 4 */
  
#define TEXTMODES 4
#define NBUTS 3
#define DIRS 2
#define UP 1
#define DOWN 0
#define SHIFTS 8		/* three keys, so eight combinations */
#define	Coordinate(r,c)		((r) * (term->screen.max_col+1) + (c))


#define	SELECT_BUTTON	0
#define	ADJUST_BUTTON	1
#define MENU_BUTTON	2

/* SS-copy */
static void	ReadClipboard();
static void	ReadPrimary();
static Boolean	ConvertSelection();
static void	LosePrimary OL_ARGS((Widget w, Atom *selection));
static void	map_button();
static void	LoseClipboard OL_ARGS((Widget w, Atom *selection));
static char	*SaltTextAway();
/* SS-copy-end */

char *SaveText();
int StartCut();
int StartExtend();
extern XtermMenu();
extern EditorButton();
extern TrackDown();

extern char *xterm_name;
extern Bogus(), Silence();
#ifdef TEK
int	GINbutton0(), GINbutton(), TekMenu();
/* FLH mouseless */
extern TekMenu();
/* FLH mouseless-end */
#endif

/*
	Note: the shifted portion of this table is no longer useful;
	the status of keyboard modifiers is takein into account by
	the Toolkit virtual event translation routines.
	The button event handlers no longer pay attention to the
	status of the keyboard modifiers.
*/
/* due to LK201 limitations, not all of the below are actually possible */
static int (*textfunc[TEXTMODES][SHIFTS][DIRS][NBUTS])() = {
/*	left		middle		right	*/
	StartCut,	StartExtend,	XtermMenu,	/* down |	  */
	Silence,	Silence,	Silence,	/* up	|no shift */

	StartCut,	StartExtend,	XtermMenu,	/* down |	  */
	Silence,	Silence,	Silence,	/* up	|shift	  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|meta	  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|meta shift */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|control  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|ctl shift */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|ctl meta */

	Bogus,		Bogus,		Bogus,		/* down	| control  */
	Silence,	Silence,	Silence,	/* up	|meta shift*/

/* MIT mouse bogus sequence 			*/
/* 	button, shift keys, and direction 	*/
/*	left		middle		right	*/
	EditorButton,	EditorButton,	EditorButton,	/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|no shift */

	StartCut,	StartExtend,	XtermMenu,	/* down |	  */
	Silence,	Silence,	Silence,	/* up	|shift	  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|meta	  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|meta shift */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|control  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|ctl shift */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|ctl meta */

	Bogus,		Bogus,		Bogus,		/* down	| control  */
	Silence,	Silence,	Silence,	/* up	|meta shift*/

/* DEC mouse bogus sequence 			*/
/* 	button, shift keys, and direction 	*/
/*	left		middle		right	*/
	EditorButton,	EditorButton,	EditorButton,	/* down	|	  */
	EditorButton,	EditorButton,	EditorButton,	/* up	|no shift */

	StartCut,	StartExtend,	XtermMenu,	/* down |	  */
	Silence,	Silence,	Silence,	/* up	|shift	  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|meta	  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|meta shift */

	EditorButton,	EditorButton,	EditorButton,	/* down	|	  */
	EditorButton,	EditorButton,	EditorButton,	/* up	|control  */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|ctl shift */

	Bogus,		Bogus,		Bogus,		/* down	|	  */
	Silence,	Silence,	Silence,	/* up	|ctl meta */

	Bogus,		Bogus,		Bogus,		/* down	| control  */
	Silence,	Silence,	Silence,	/* up	|meta shift*/

/* Hilite tracking DEC mouse bogus sequence 	*/
/* 	button, shift keys, and direction 	*/
/*	left		middle		right	*/
	TrackDown,	EditorButton,	EditorButton,	/* down	|	    */
	EditorButton,	EditorButton,	EditorButton,	/* up	|no shift   */

	StartCut,	StartExtend,	XtermMenu,	/* down |	  */
	Silence,	Silence,	Silence,	/* up	|shift	  */

	Bogus,		Bogus,		Bogus,		/* down	|	    */
	Silence,	Silence,	Silence,	/* up	|meta	    */

	Bogus,		Bogus,		Bogus,		/* down	|	    */
	Silence,	Silence,	Silence,	/* up	|meta shift */

	EditorButton,	EditorButton,	EditorButton,	/* down	|	    */
	EditorButton,	EditorButton,	EditorButton,	/* up	|control    */

	Bogus,		Bogus,		Bogus,		/* down	|	    */
	Silence,	Silence,	Silence,	/* up	|ctl shift  */

	Bogus,		Bogus,		Bogus,		/* down	|	    */
	Silence,	Silence,	Silence,	/* up	|ctl meta   */

	Bogus,		Bogus,		Bogus,		/* down	| control   */
	Silence,	Silence,	Silence		/* up	|meta shift */

};

/* SS-virt */
static          char button_map_tbl[3] = {-1, -1, -1};
/* SS-virt-end */

#ifdef TEK
/*
	Note: the shifted portion of this table is no longer useful;
	the status of keyboard modifiers is takein into account by
	the Toolkit virtual event translation routines.
	The button event handlers no longer pay attention to the
	status of the keyboard modifiers.
*/
/* button and shift keys for Tek mode */
static int (*Tbfunc[SHIFTS][NBUTS])() = {
/*	left		middle		right	*/
	GINbutton0,	GINbutton,	TekMenu,	/* down	|no shift   */

	GINbutton,	GINbutton,	TekMenu,	/* down |shift	    */

	Bogus,		Bogus,		Bogus,		/* down	|meta	    */

	Bogus,		Bogus,		Bogus,		/* down	|meta shift */

	Bogus,		Bogus,		Bogus,	/* down	|control    */

	Bogus,		Bogus,		Bogus,	/* down	|ctl shift  */

	Bogus,		Bogus,		Bogus,		/* down	|ctl meta   */

	Bogus,		Bogus,		Bogus,		/* down	| all       */

};	/* button and shift keys */
#endif /* TEK */


/* Selection/extension variables */

/* Raw char position where the selection started */
static int rawRow, rawCol;

/* Hilited area */
static int startHRow, startHCol, endHRow, endHCol, startHCoord, endHCoord = 0;

/* Selected area before CHAR, WORD, LINE selectUnit processing */
static int startRRow, startRCol, endRRow, endRCol = 0;

/* Selected area after CHAR, WORD, LINE selectUnit processing */
static int startSRow, startSCol, endSRow, endSCol = 0;

/* Valid rows for selection clipping */
static int firstValidRow, lastValidRow;

/* Start, end of extension */
static int startERow, startECol, endERow, endECol;

/* Saved values of raw selection for extend to restore to */
static int saveStartRRow, saveStartRCol, saveEndRRow, saveEndRCol;

/* SS-copy */
static int lastRow, lastCol;
/* SS-copy-end */

/* Multi-click handling */
static int numberOfClicks = 0;
static long int lastButtonUpTime = 0;
typedef enum {SELECTCHAR, SELECTWORD, SELECTLINE} SelectUnit;
static SelectUnit selectUnit;

/* Send emacs escape code when done selecting or extending? */
static int replyToEmacs;


/* FLH mouseless */
void VTButtonPressed(w,ve)
Widget w;
OlVirtualEvent ve;
{
/* SS-focus */
	register XEvent *event = ve->xevent;
	register TScreen *screen = &term->screen;
	extern Boolean	Xterm_startup;
	int button;
	int virt_button = -1;
	Boolean primary_paste = False;

	ve->consumed = True;		/* always consume mouse events */
	
	switch(ve->virtual_name){
		case OL_SELECT:
			virt_button = SELECT_BUTTON;
			break;
		case OL_ADJUST:
			virt_button = ADJUST_BUTTON;
			break;
		case OL_MENU:
			virt_button = MENU_BUTTON;
			break;
		    case OLM_BPrimaryPaste:
			primary_paste = True;
			break;
		default:
			break;
	}

#ifdef TEK
	if ((screen->TekEmu || screen->select != FOCUS || Xterm_startup) &&
#else
	if ((screen->select != FOCUS || Xterm_startup) &&
#endif /* TEK */
			virt_button == SELECT_BUTTON)
	{
		XWindowAttributes	    win_attrs;
#ifdef TEK
	   extern void		    end_tek_mode();
#endif /* TEK */

		Xterm_startup = FALSE;
		XGetWindowAttributes (screen->display, VWindow(screen),
                                  &win_attrs);

            /* we may have a race condition: window is not mapped */
            /* yet, but we already trying to set focus set to that*/
            /* window, which will produce X protocol error.	  */
            /* To avoid this, we chech first if the window has	  */
            /* been mapped.  If not, we go away (we could have	  */
            /* waited for an Expose event and then set Focus)	  */

		if (win_attrs.map_state != IsUnviewable)
		{
/* FLH dynamic */
			OlSetInputFocus ((Widget) term,
					RevertToParent, CurrentTime);
#ifdef TEK
			end_tek_mode(NULL);
#endif /* TEK */
/* FLH dynamic */
		}
		else
			return;
	}
	
/* SS-focus-end */
/* SS-mouse */
	button = event->xbutton.button - 1;
	if (Get_Mouse_Events & (1 << (5*button + 1)))
	{
		proc_mouse_button_event (&(event->xbutton), DOWN);
		return;
	}
/* SS-mouse-end */
	if (eventMode != NORMAL)
	{
		return;
	}
	if (screen->incopy)
	    CopyWait (screen);	
	    /* the virtual mapping for this button may still be not known */
	    /* (for example, it was pressed with the modifier, so	      */
	    /* LookupOlInputEvent() didn't map it to one of SELECT,		*/
	    /* ADJUST, or MENU. In this case, ignore it */

	if (virt_button !=  -1)
	    textfunc[screen->send_mouse_pos][0][0][virt_button](&(event->xbutton));
	if (primary_paste)
	    PrimaryPaste();
	  

}
/* FLH mouseless-end */

/* FLH mouseless */
void VTMouseMoved(w,ve)
Widget w;
OlVirtualEvent ve;
{
/* SS-focus */
	register XEvent *event = ve->xevent;

	ve->consumed = False;			
	
	switch (eventMode) {
		case LEFTEXTENSION :
		case RIGHTEXTENSION :
			ve->consumed = True;
			ExtendExtend(event->xbutton.x, event->xbutton.y);
			break;
		default :
			ve->consumed = False;
        		/* Should get here rarely when everything
           		fixed with windows and the event mgr */
/*			fprintf(stderr, "Race mouse motion\n");
*/			break;
	}
}
/* FLH mouseless-end */

/* FLH mouseless */
void VTButtonReleased(w,ve)
Widget w;
OlVirtualEvent ve;
{
/* SS-focus */
	register XEvent *event = ve->xevent;
	register TScreen *screen = &term->screen;
	int button;
	int virt_button = -1;

	ve->consumed = True;			/* always consume mouse events */
	
	switch(ve->virtual_name){
		case OL_SELECT:
			virt_button = SELECT_BUTTON;
			break;
		case OL_ADJUST:
			virt_button = ADJUST_BUTTON;
			break;
		case OL_MENU:
			virt_button = MENU_BUTTON;
			break;
		default:
			break;
	}

	button = event->xbutton.button - 1;
/* SS-mouse-end */
	if (Get_Mouse_Events & (1 << (5*button)))
	{
		proc_mouse_button_event (&(event->xbutton), UP);
		return;
	}
/* SS-mouse-end */
	switch (eventMode) {
		case NORMAL :
			/* the virtual mapping for this button may still be */
			/* not known. (for example, it was pressed with the */
			/* modifier, so LookupOlInputEvent() didn't map it  */
			/* to one of SELECT, ADJUST, or MENU. In this case, */
			/* ignore it													 */ 

			if (virt_button !=  -1)
				textfunc[screen->send_mouse_pos][0][1][virt_button](&(event->xbutton));
			break;
		case LEFTEXTENSION :
		case RIGHTEXTENSION :
			eventMode = NORMAL;
			if AllButtonsUp(event->xbutton.state, event->xbutton.button)
				EndExtend(&(event->xbutton));
			break;
	}
}
/* FLH mouseless-end */
	
#define MULTICLICKTIME 250

SetSelectUnit(buttonDownTime, defaultUnit)
unsigned long buttonDownTime;
SelectUnit defaultUnit;
{
/* Do arithmetic as integers, but compare as unsigned solves clock wraparound */
	if ((long unsigned)((long int)buttonDownTime - lastButtonUpTime)
	 > MULTICLICKTIME) {
		numberOfClicks = 1;
		selectUnit = defaultUnit;
	} else {
		++numberOfClicks;
		/* Don't bitch.  This is only temporary. */
		selectUnit = (SelectUnit) (((int) selectUnit + 1) % 3);
	}
}

StartCut(event)
register XButtonEvent *event;
{
	register TScreen *screen = &term->screen;
	int startrow, startcol;

	firstValidRow = 0;
	lastValidRow  = screen->max_row;

/* SS-copy */
	if (!XtOwnSelection((Widget) term,
	                    XA_PRIMARY,
	                    CurrentTime,
	                    ConvertSelection,
	                    LosePrimary,
	                    NULL)) {
#ifdef DEBUG
        	printf("xterm:   We didn't get the selection!\n");
#endif /* DEBUG */
		return;
	}
/* SS-copy-end */

	Have_hilite = TRUE;
	SetSelectUnit(event->time, SELECTCHAR);
	PointToRowCol(event->y, event->x, &startrow, &startcol);
	replyToEmacs = FALSE;
	StartSelect(startrow, startcol);
}


TrackDown(event)
register XButtonEvent *event;
{
	int startrow, startcol;

	SetSelectUnit(event->time, SELECTCHAR);
	if (numberOfClicks > 1 ) {
		PointToRowCol(event->y, event->x, &startrow, &startcol);
		replyToEmacs = TRUE;
		StartSelect(startrow, startcol);
	} else {
		waitingForTrackInfo = 1;
		EditorButton(event);
	}
}


TrackMouse(func, startrow, startcol, firstrow, lastrow)
int func, startrow, startcol, firstrow, lastrow;
{
	if (!waitingForTrackInfo) {	/* Timed out, so ignore */
		return;
	}
	waitingForTrackInfo = 0;
	if (func == 0) return;

	firstValidRow = firstrow;
	lastValidRow  = lastrow;
	replyToEmacs = TRUE;
	StartSelect(startrow, startcol);
}

StartSelect(startrow, startcol)
int startrow, startcol;
{
	TScreen *screen = &term->screen;
	Char	euc;

	if (screen->cursor_state)
	    HideCursor ();

	if ((numberOfClicks%3) == 1) {
	    if (screen->buf[4*(startrow+screen->topline)+3][startcol] &
	        SECOND_BYTE)
		--startcol;

	    if (numberOfClicks == 1) {
		/* set start of selection */
		rawRow = startrow;
		rawCol = startcol;
	    }
		
	} /* else use old values in rawRow, Col */

	saveStartRRow = startERow = rawRow;
	saveStartRCol = startECol = rawCol;
	saveEndRRow   = endERow   = rawRow;
	saveEndRCol   = endECol   = rawCol;
	if (Coordinate(startrow, startcol) < Coordinate(rawRow, rawCol)) {
		eventMode = LEFTEXTENSION;
		startERow = startrow;
		startECol = startcol;
	} else {
		eventMode = RIGHTEXTENSION;
		endERow = startrow;
		endECol = startcol;
	}
	ComputeSelect(startERow, startECol, endERow, endECol);

}

EndExtend(event)
XButtonEvent *event;
{
	ExtendExtend(event->x, event->y);

	lastButtonUpTime = event->time;
	PointToRowCol(event->y, event->x, &lastRow, &lastCol);
}

#define Abs(x)		((x) < 0 ? -(x) : (x))

StartExtend(event)
XButtonEvent *event;
{
	TScreen *screen = &term->screen;
	int row, col, coord;
	register int topline = screen->topline;

	firstValidRow = 0;
	lastValidRow  = screen->max_row;

	SetSelectUnit(event->time, selectUnit);
	replyToEmacs = FALSE;

	if (numberOfClicks == 1) {
		/* Save existing selection so we can reestablish it if the guy
		   extends past the other end of the selection */
		saveStartRRow = startERow = startRRow;
		saveStartRCol = startECol = startRCol;
		saveEndRRow   = endERow   = endRRow;
		saveEndRCol   = endECol   = endRCol;
	} else {
		/* He just needed the selection mode changed, use old values. */
		startERow = startRRow = saveStartRRow;
		startECol = startRCol = saveStartRCol;
		endERow   = endRRow   = saveEndRRow;
		endECol   = endRCol   = saveEndRCol;

	}
	PointToRowCol(event->y, event->x, &row, &col);
	coord = Coordinate(row, col);

	if (Abs(coord - Coordinate(startSRow, startSCol))
	     < Abs(coord - Coordinate(endSRow, endSCol))
	    || coord < Coordinate(startSRow, startSCol)) {
	 	/* point is close to left side of selection */
		eventMode = LEFTEXTENSION;
		startERow = row;
		if (screen->buf[4*(row+topline)+3][col] & SECOND_BYTE)
		    startECol = col-1;
		else
		    startECol = col;
	} else {
	 	/* point is close to left side of selection */
		eventMode = RIGHTEXTENSION;
		endERow = row;
		if (screen->buf[4*(row+topline)+3][col] & SECOND_BYTE)
		    endECol = col+1;
		else
		    endECol = col;
	}
	Have_hilite = TRUE;
	ComputeSelect(startERow, startECol, endERow, endECol);
}

ExtendExtend(x, y)
int x, y;
{
	TScreen *screen = &term->screen;
	int row, col, coord;
	register int topline = screen->topline;

	PointToRowCol(y, x, &row, &col);
	coord = Coordinate(row, col);
	Have_hilite = TRUE;
	
	if (eventMode == LEFTEXTENSION 
	 && (coord + (selectUnit!=SELECTCHAR)) > Coordinate(endSRow, endSCol)) {
		/* Whoops, he's changed his mind.  Do RIGHTEXTENSION */
		eventMode = RIGHTEXTENSION;
		startERow = saveStartRRow;
		startECol = saveStartRCol;
#ifdef I18N
		if(screen->buf[4*(startERow+topline)+3][startECol]&SECOND_BYTE)
    		   startECol--;
#endif
	} else if (eventMode == RIGHTEXTENSION
	 && coord < Coordinate(startSRow, startSCol)) {
	 	/* Whoops, he's changed his mind.  Do LEFTEXTENSION */
		eventMode = LEFTEXTENSION;
		endERow   = saveEndRRow;
		endECol   = saveEndRCol;
#ifdef I18N
		if (screen->buf[4*(endERow+topline)+3][endECol] & SECOND_BYTE)
    		    endECol++;
#endif
	}
	if (eventMode == LEFTEXTENSION) {
		startERow = row;
		startECol = col;
#ifdef I18N
		if(screen->buf[4*(startERow+topline)+3][startECol]&SECOND_BYTE)
    		    startECol--;
#endif
	} else if (eventMode == RIGHTEXTENSION) {
		endERow = row;
		endECol = col;
#ifdef I18N
	       if (screen->buf[4*(endERow+topline)+3][endECol] & SECOND_BYTE)
    		   endECol++;
#endif
	}
	ComputeSelect(startERow, startECol, endERow, endECol);
}


ScrollSelection(amount)
int amount;
{
	/* Sent by scrollbar stuff, so amount never takes selection out of
	   saved text */
startRRow += amount; endRRow += amount; startSRow += amount; endSRow += amount;
rawRow += amount;
}


PointToRowCol(y, x, r, c)
register int y, x;
int *r, *c;
/* Convert pixel coordinates to character coordinates.
   Rows are clipped between firstValidRow and lastValidRow.
   Columns are clipped between to be 0 or greater, but are not clipped to some
       maximum value. */
{
	register TScreen *screen = &term->screen;
	register row, col;

	row = (y - screen->border) / FontHeight(screen);
	if(row < firstValidRow)
		row = firstValidRow;
	else if(row > lastValidRow)
		row = lastValidRow;
/* SS-scrollbar */
	/* col = (x - screen->border - screen->scrollbar) / FontWidth(screen); */
	col = (x - screen->border) / FontWidth(screen);
/* SS-scrollbar-end */
	if(col < 0)
		col = 0;
	else if(col > screen->max_col+1) {
		col = screen->max_col+1;
	}
	*r = row;
	*c = col;
}


MousePointToRowCol(y, x, r, c)
register int y, x;
int *r, *c;
/*
   Convert pixel coordinates to character coordinates.
   Rows are clipped between 0 and screen->max_row.
   Cols are clipped between 0 and screen->max_col.
*/
{
	register TScreen *screen = &term->screen;
	register row, col;

	row = (y - screen->border) / FontHeight(screen);
	if(row < 0)
		row = 0;
	else if(row > screen->max_row)
		row = screen->max_row;
	col = (x - screen->border) / FontWidth(screen);
	if(col < 0)
		col = 0;
	else if(col > screen->max_col)
		col = screen->max_col;
	*r = row;
	*c = col;
}


int LastTextCol(row)
register int row;
{
	register TScreen *screen =  &term->screen;
	register int i;
	register Char *ch;

	for(i = screen->max_col,
	 ch = screen->buf[4 * (row + screen->topline)] + i ;
	 i > 0 && (*ch == 0 || *ch == ' ') ; ch--, i--);
	return(i);
}	

static int charClass[128] = {
/* NUL  SOH  STX  ETX  EOT  ENQ  ACK  BEL */
    32,   1,   1,   1,   1,   1,   1,   1,
/*  BS   HT   NL   VT   NP   CR   SO   SI */
     1,  32,   1,   1,   1,   1,   1,   1,
/* DLE  DC1  DC2  DC3  DC4  NAK  SYN  ETB */
     1,   1,   1,   1,   1,   1,   1,   1,
/* CAN   EM  SUB  ESC   FS   GS   RS   US */
     1,   1,   1,   1,   1,   1,   1,   1,
/*  SP    !    "    #    $    %    &    ' */
    32,  33,  34,  35,  36,  37,  38,  39,
/*   (    )    *    +    ,    -    .    / */
    40,  41,  42,  43,  44,  45,  46,  47,
/*   0    1    2    3    4    5    6    7 */
    48,  48,  48,  48,  48,  48,  48,  48,
/*   8    9    :    ;    <    =    >    ? */
    48,  48,  58,  59,  60,  61,  62,  63,
/*   @    A    B    C    D    E    F    G */
    64,  48,  48,  48,  48,  48,  48,  48,
/*   H    I    J    K    L    M    N    O */
    48,  48,  48,  48,  48,  48,  48,  48,
/*   P    Q    R    S    T    U    V    W */ 
    48,  48,  48,  48,  48,  48,  48,  48,
/*   X    Y    Z    [    \    ]    ^    _ */
    48,  48,  48,  91,  92,  93,  94,  48,
/*   `    a    b    c    d    e    f    g */
    96,  48,  48,  48,  48,  48,  48,  48,
/*   h    i    j    k    l    m    n    o */
    48,  48,  48,  48,  48,  48,  48,  48,
/*   p    q    r    s    t    u    v    w */
    48,  48,  48,  48,  48,  48,  48,  48,
/*   x    y    z    {    |    }    ~  DEL */
    48,  48,  48, 123, 124, 125, 126,   1};


ComputeSelect(startRow, startCol, endRow, endCol)
int startRow, startCol, endRow, endCol;
{
	register TScreen *screen = &term->screen;
	register Char *ptr, *euc;
	register int length;
	register int class;
	register Char code_set;
	int 	 i;

	if (Coordinate(startRow, startCol) <= Coordinate(endRow, endCol)) {
		startSRow = startRRow = startRow;
		startSCol = startRCol = startCol;
		endSRow   = endRRow   = endRow;
		endSCol   = endRCol   = endCol;
	} else {	/* Swap them */
		startSRow = startRRow = endRow;
		startSCol = startRCol = endCol;
		endSRow   = endRRow   = startRow;
		endSCol   = endRCol   = startCol;
	}	

	switch (selectUnit) {
		case SELECTCHAR :
			if (startSCol > (LastTextCol(startSRow) + 1)) {
				startSCol = 0;
				startSRow++;
			}
			if (endSCol > (LastTextCol(endSRow) + 1)) {
				endSCol = 0;
				endSRow++;
			}
			break;
		case SELECTWORD :
			if (startSCol > (LastTextCol(startSRow) + 1)) {
				startSCol = 0;
				startSRow++;
			} else {
				ptr = screen->buf[i=4*(startSRow+screen->topline)]
				 		 + startSCol;
				euc = screen->buf[i+3] + startSCol;
				if ((code_set = Code_set (*euc)) == 0) {
				    class = charClass[*ptr];
				    do {
					--startSCol;
					--ptr;
					--euc;
				    } while (startSCol >= 0 &&
					     Code_set (*euc) == 0 &&
					     charClass[*ptr] == class);
				}
				else {
				    do {
					--startSCol;
					--ptr;
					--euc;
				    } while (startSCol >= 0 &&
					     Code_set(*euc) == code_set);
				}
				++startSCol;
			}
			if (endSCol > (LastTextCol(endSRow) + 1)) {
				endSCol = 0;
				endSRow++;
			} else {
				length = LastTextCol(endSRow);
				ptr = screen->buf[i=4*(endSRow+screen->topline)]
				 		 + endSCol;
				euc = screen->buf[i + 3] + endSCol;
				if ((code_set = Code_set (*euc)) == 0) {
				    class = charClass[*ptr];
				    do {
					++endSCol;
					++ptr;
					++euc;
				    } while (endSCol <= length &&
					     Code_set (*euc) == 0 &&
					     charClass[*ptr] == class);
				}
				else {
				    do {
					++endSCol;
					++ptr;
					++euc;
				    } while (endSCol <= length &&
					     Code_set(*euc) == code_set);
				}

				/* Word select selects if pointing to any char
				   in "word", especially in that it includes
				   the last character in a word.  So no --endSCol
				   and do special eol handling */
				if (endSCol > length+1) {
					endSCol = 0;
					++endSRow;
				}
			}
			break;
		case SELECTLINE :
			startSCol = 0;
			endSCol = 0;
			++endSRow;
			break;
	}
	TrackText(startSRow, startSCol, endSRow, endSCol);
	return;
}


TrackText(frow, fcol, trow, tcol)
register int frow, fcol, trow, tcol;
/* Guaranteed (frow, fcol) <= (trow, tcol) */
{
	register int from, to;
	register TScreen *screen = &term->screen;

	/* (frow, fcol) may have been scrolled off top of display */
	if (frow < 0)
		frow = fcol = 0;
	/* (trow, tcol) may have been scrolled off bottom of display */
	if (trow > screen->max_row+1) {
		trow = screen->max_row+1;
		tcol = 0;
	}
	from = Coordinate(frow, fcol);
	to = Coordinate(trow, tcol);
	if (to <= startHCoord || from > endHCoord) {
		/* No overlap whatsoever between old and new hilite */
		HiliteText(startHRow, startHCol, endHRow, endHCol, FALSE);
		HiliteText(frow, fcol, trow, tcol, TRUE);
	} else {
		if (from < startHCoord) {
			/* Extend left end */
			HiliteText(frow, fcol, startHRow, startHCol, TRUE); 
		} else if (from > startHCoord) {
			/* Shorten left end */
			HiliteText(startHRow, startHCol, frow, fcol, FALSE);
		}
		if (to > endHCoord) {
			/* Extend right end */
			HiliteText(endHRow, endHCol, trow, tcol, TRUE); 
		} else if (to < endHCoord) {
			/* Shorten right end */
			HiliteText(trow, tcol, endHRow, endHCol, FALSE);
		}
	}
	startHRow = frow;
	startHCol = fcol;
	endHRow   = trow;
	endHCol   = tcol;
	startHCoord = from;
	endHCoord = to;
}

HiliteText(frow, fcol, trow, tcol, hilite)
register int frow, fcol, trow, tcol;
Boolean  hilite;
/* Guaranteed that (frow, fcol) <= (trow, tcol) */
{
	register Char *attr;
	register TScreen *screen = &term->screen;
	register int k, n, i, j;
	register int topline = screen->topline;

	if (frow == trow && fcol == tcol)
		return;

	/* more than one row need to be (un)hilited	*/

	if (frow != trow) {
	    if ((i = screen->max_col - fcol + 1) > 0) {	/* first row */
		 XClearArea(screen->display,
			    VWindow(screen),
			    (int) CursorX(screen, fcol),
			    (int) frow * FontHeight(screen) + screen->border,
			    (unsigned) i*FontWidth(screen),
			    (unsigned) FontHeight(screen),
			    FALSE);
		 attr = screen->buf [4 * (frow + topline) + 1];
		 if (hilite)
		     for (k=fcol; k <= screen->max_col; k++)
		       attr[k] |= HILITED;
		 else
		     for (k=fcol; k <= screen->max_col; k++)
		       attr[k] &= ~HILITED;
	   	 ScrnRefresh(screen, frow, fcol, 1, i, hilite);
	    }
	    if ((i = trow - frow - 1) > 0) {			/* middle rows*/
	         j = screen->max_col + 1;
	         XClearArea (screen->display,
			     VWindow(screen),
/* SS-scrollbar */
		    	     /* (int) screen->border + screen->scrollbar, */
		    	     (int) screen->border,
/* SS-scrollbar-end */
			     (int) (frow+1)*FontHeight(screen) + screen->border,
			     (unsigned) j * FontWidth(screen),
			     (unsigned) i * FontHeight(screen),
			     FALSE);
	         for (n=frow+1; n < trow; n++) {
		      attr = screen->buf [4 * (n + topline) + 1];
		      if (hilite)
		          for (k=0; k <= screen->max_col; k++)
		            attr[k] |= HILITED;
		      else
		          for (k=0; k <= screen->max_col; k++)
		            attr[k] &= ~HILITED;
	         }
	         ScrnRefresh(screen, frow + 1, 0, i, j, hilite);
	    }
	    if (tcol > 0 && trow <= screen->max_row) {	/* last row */
	        XClearArea(screen->display,
		           VWindow(screen),
/* SS-scrollbar */
		           /* (int) screen->border + screen->scrollbar, */
		           (int) screen->border,
/* SS-scrollbar-end */
		           (int) trow * FontHeight(screen) + screen->border,
		           (unsigned) tcol * FontWidth(screen),
		           (unsigned) FontHeight(screen),
		           FALSE);
	        attr = screen->buf [4 * (trow + topline) + 1];
		if (hilite)
		    for (k=0; k < tcol; k++)
		      attr[k] |= HILITED;
		else
		    for (k=0; k < tcol; k++)
		      attr[k] &= ~HILITED;
	        ScrnRefresh(screen, trow, 0, 1, tcol, hilite);
	    }
	 } else {		/* do single row */
		i = tcol - fcol;
		XClearArea(
		    screen->display,
		    VWindow(screen), 
		    (int) CursorX(screen, fcol),
		    (int) frow * FontHeight(screen) + screen->border,
		    (unsigned) i * FontWidth(screen),
		    (unsigned) FontHeight(screen),
		    FALSE);
		attr = screen->buf [4 * (frow + topline) + 1];
		if (hilite)
		    for (k=fcol; k < tcol; k++)
		      attr[k] |= HILITED;
		else
		    for (k=fcol; k < tcol; k++)
		      attr[k] &= ~HILITED;
		
		ScrnRefresh(screen, frow, fcol, 1, tcol - fcol, hilite);
	}
}


/* returns number of chars in line from scol to ecol out */
int Length(screen, row, scol, ecol)
register int row, scol, ecol;
register TScreen *screen;
{
	register Char *ch;
	/* MORE: remove register TScreen *screen = &term->screen; */

	ch = screen->buf[4 * (row + screen->topline)];
	if (ecol == screen->max_col) {
	    while (ecol >= scol && (ch[ecol] == 0 || ch[ecol] == ' '))
	        ecol--;
	}
	else {
	    while (ecol >= scol && ch[ecol] == 0)
	        ecol--;
	}
	return (ecol - scol + 1);
}

/* copies text into line, preallocated */
char *SaveText(screen, row, scol, ecol, lp)
int row;
int scol, ecol;
TScreen *screen;
register char *lp;		/* pointer to where to put the text */
{
	register int i, j;
	register Char *ch  = screen->buf[i = (4*(row + screen->topline))];
	register Char *euc = screen->buf[i + 3];
	OlFontList    *fontl = term->primitive.font_list;
	register char code_set, code_width;
	register int c;

	if ((i = Length(screen, row, scol, ecol)) == 0)
	    return(lp);

	ecol = scol + i;

	/* we must re-create the EUC string from the information saved	*/
	/* in screen->buf						*/

	for (i = scol; i < ecol; ) {
	     code_set   = Code_set (euc[i]);
	     if (fontl != (OlFontList *) NULL)
		 code_width = fontl->cswidth[code_set];	
	     else
		 code_width = 1;
	     switch (code_set) {
		case 0:				/* ASCII: simply copy */
		   if ((c = ch[i++]) == 0)
			c = ' ';
		   else if(c < ' ') {
			if(c == '\036')
				c = '#';
			else
				c += 0x5f;
		   } else if(c == 0x7f)
			c = 0x5f;
		   *lp++ = c;
		   break;

		/* supplementary codesets: make sure high order bit is 1 */
		/* we really only need to do it for codesets with code   */
		/* width greater then 1 (because only in these codesets  */
		/* the MSB could be cleared.  But the code will work 	 */
		/* faster if we simply do it all the time.		 */

		case 1:
		   for (j=0; j<code_width; j++) 
		        *lp++ = (ch[i++] | 0x80);
		   break;
		case 2:
		   *lp++ = SS2;
		   for (j=0; j<code_width; j++) 
		        *lp++ = (ch[i++] | 0x80);
		   break;
		case 3:
		   *lp++ = SS3;
		   for (j=0; j<code_width; j++) 
		        *lp++ = (ch[i++] | 0x80);
		   break;
	     }
	}
	return(lp);
}

EditorButton(event)
register XButtonEvent *event;
{
	register TScreen *screen = &term->screen;
	int pty = screen->respond;
	char line[6];
	register unsigned row, col;
	int button; 

	button = event->button - 1; 

	row = (event->y - screen->border) 
	 / FontHeight(screen);
/* SS-scrollbar */
	/* col = (event->x - screen->border - screen->scrollbar)
	 / FontWidth(screen); */
	col = (event->x - screen->border) / FontWidth(screen);
/* SS-scrollbar-end */
	(void) strcpy(line, "\033[M");
	if (screen->send_mouse_pos == 1) {
		line[3] = ' ' + button;
	} else {
		line[3] = ' ' + (KeyState(event->state) << 2) + 
			((event->type == ButtonPress)? button:3);
	}
	line[4] = ' ' + col + 1;
	line[5] = ' ' + row + 1;
	v_write(pty, line, 6);
}

#ifdef TEK
GINbutton0(event)
XButtonEvent *event;
{
	int button = event->button - 1; 
	register TScreen *screen = &term->screen;
	extern Boolean	Xterm_startup;

	if ((!screen->TekEmu || screen->select != FOCUS || Xterm_startup) &&
	    button_map_tbl[button] == SELECT_BUTTON)
	{
	    XWindowAttributes       win_attrs;
	    extern void		    end_vt_mode();

	    Xterm_startup = FALSE;
	    XGetWindowAttributes (screen->display, TWindow(screen),
				  &win_attrs);

	    /* we may have a race condition: window is not mapped */
	    /* yet, but we already trying to set focus set to that*/
	    /* window, which will produce X protocol error.       */
	    /* To avoid this, we chech first if the window has    */
	    /* been mapped.  If not, we go away (we could have    */
	    /* waited for an Expose event and then set Focus)	  */

	    if (win_attrs.map_state != IsUnviewable)
	    {
			OlSetInputFocus ((Widget) tekWidget,
					RevertToParent, CurrentTime);
		end_vt_mode (NULL);
	    }
	    else
	        return;
	}
	else
	    Bell();
}
#endif /* #ifdef TEK */

GINbutton(event)
XButtonEvent *event;
{
	Bell();
}

/*ARGSUSED*/
Bogus(event)
XButtonEvent *event;
{
	Bell();
}

/*ARGSUSED*/
Silence(event)
XButtonEvent *event;
{
}


/* SS-mouse */
proc_mouse_button_event (event, press)
XButtonEvent *event;
int  press;
{
	int row, col, i;
	char buff[32];
	extern int  make_myx_format();

	buff[0] = 033;
	buff[1] = '[';
	buff[2] = CTRL('_');
	buff[3] = '0' + event->button;
	buff[4] = (press == DOWN) ? 'P' : 'R';
	MousePointToRowCol(event->y, event->x, &row, &col);
	i = make_myx_format (&buff[5], row, col);
	Send_mouse_event (buff, i+5);
}



void rep_mouse_pos (screen)
register TScreen *screen;
{
	/* write mouse pointer coordinates using myx format */

	int row, col, i, root_x, root_y, x, y;
	Window win, root;
	unsigned int /*XtGCMask*/		mask;
	char buff[32];
	extern int  make_myx_format();

	buff[0] = 033;
	buff[1] = '[';
	buff[2] = CTRL('_');
	buff[3] = '0';

	if(XQueryPointer(
	    screen->display, VWindow(screen),
	    &root, &win,
	    &root_x, &root_y,
	    &x, &y,
	    &mask) == 0)
	{
	        i = make_myx_format (&buff[4], -1, -1);
	}
	else
	{
		MousePointToRowCol(y, x, &row, &col);
	        i = make_myx_format (&buff[4], row, col);
	}
	Send_mouse_event (buff, i+4);
}
/* SS-mouse-end */


XtermMenu(event)
XButtonEvent *event;
{
	register TScreen *screen = &term->screen;
	extern   void SetUpMenu();

	/* if this is the first call, create menu widget	*/

	if (screen->menuWidget == NULL)
	    SetUpMenu();

	/* pop up the xterm menu				*/

      	flatMenuPost(screen->menuWidget, event->window, (XEvent *) event);
}

#ifdef TEK

#include <error.h>

/* FLH mouseless */
void TekButtonPressed(w,ve)
Widget w;
OlVirtualEvent ve;
{
/* SS-focus */
	register XEvent *event = ve->xevent;
	register TScreen *screen = &term->screen;
	extern Boolean	Xterm_startup;
	int button;
	int virt_button = -1;

	ve->consumed = True;		/* always consume mouse events */
	
	switch(ve->virtual_name){
		case OL_SELECT:
			virt_button = SELECT_BUTTON;
			break;
		case OL_ADJUST:
			virt_button = ADJUST_BUTTON;
			break;
		case OL_MENU:
			virt_button = MENU_BUTTON;
			break;
		default:
			break;
	}
		/* so table above will be nice, we index from 0 */
	button = event->xbutton.button - 1;
		/* set up button_map_tbl[] for current button */
		/* in case GINButton0 gets called here */
	map_button(w,ve,button);

	if (screen->incopy)
		CopyWait (screen);	
		/* the virtual mapping for this button may still be not known */
		/* (for example, it was pressed with the modifier, so	      */
		/* LookupOlInputEvent() didn't map it to one of SELECT,		*/
		/* ADJUST, or MENU. In this case ignore it						*/

	if (virt_button !=  -1)
		(*(Tbfunc[0][virt_button]))(&(event->xbutton));
}
/* FLH mouseless-end */

TekMenu(event)
XButtonEvent *event;
{
	register TScreen *screen = &term->screen;
	extern   void TSetUpMenu();

	/* if this is the first call, create menu widget	*/

	if (screen->TmenuWidget == NULL)
	    TSetUpMenu();

	/* pop up the xterm menu				*/

        flatMenuPost(screen->TmenuWidget, event->window, (XEvent *) event); 
}
#endif /* TEK */

/* SS-copy */

static void
ReadClipboard(w, client_data, selection, type, value, length, format)
Widget	w;
XtPointer	client_data;
Atom	*selection;
Atom	*type;
XtPointer	value;
unsigned long	*length;
int	*format;
{
	int pty = term->screen.respond;	/* file descriptor of pty */
	register char *lag, *cp, *end;

#ifdef DEBUG
	printf("\nReadClipboard: received value=%x", (char *) value);
#endif /* DEBUG */

	if (*length == 0)
	    return;
	end = (char *) value + *length;
	lag = (char *) value;
	for (cp = (char *) value; cp != end; cp++)
	{
		if (*cp != '\n') continue;
		*cp = '\r';
		v_write(pty, lag, cp - lag + 1);
		lag = cp + 1;
	}
	if (lag != end)
		v_write(pty, lag, end - lag);
}


static void
ReadPrimary(w, client_data, selection, type, value, length, format)
Widget	w;
XtPointer	client_data;
Atom	*selection;
Atom	*type;
XtPointer	value;
unsigned long	*length;
int	*format;
{
#ifdef DEBUG
	printf("\nReadPrimary: received value=%x", value);
#endif /* DEBUG */

	if (*length == 0)
	    goto out;

	if (clip_contents != NULL)
	{
#ifdef DEBUG
		printf("frees the clip_content=%x and ", clip_contents);
#endif /* DEBUG */
	        XtFree(clip_contents);
	}

	clip_contents = value;
	if (!XtOwnSelection((Widget) term,
	                    XA_CLIPBOARD(XtDisplay(term)),
	                    CurrentTime,
	                    ConvertSelection,
	                    LoseClipboard,
	                    NULL)) {
#ifdef DEBUG
        	printf("xterm: ReadPrimary - We didn't get CLIPBOARD selection!\n");
#endif /* DEBUG */
	}
out:
	if (Have_to_paste) {
	    PasteText();
	    Have_to_paste = FALSE;
	}
}
/* SS-copy-end */


/* unhighlight text and free memory used for selection */

static void
LosePrimary OLARGLIST((w, selection))
    OLARG(Widget, w)
    OLGRA(Atom *, selection)
{
	
#ifdef DEBUG
	printf("\nLosePrimary ");
#endif /* DEBUG */
	Have_hilite = FALSE;
	TrackText(0, 0, 0, 0);
}


static void 
LoseClipboard OLARGLIST((w, selection))
    OLARG(Widget, w)
    OLGRA(Atom *, selection)
{
	
#ifdef DEBUG
	printf("\nLoseClipboard ");
#endif /* DEBUG */
	if (clip_contents != NULL)
	{
#ifdef DEBUG
		printf("frees the clip_content=%x ", clip_contents);
#endif /* DEBUG */
	        XtFree(clip_contents);
		clip_contents = NULL;
	}
}



static Boolean	
ConvertSelection(wid, selection, target, type_return, value_return, length_return, format_return)
Widget	wid;
Atom	*selection, *target, *type_return;
XtPointer	*value_return;
unsigned long	*length_return;
int	*format_return;
{
	int	i;
	char	*buffer;
	Display *dpy = XtDisplay(wid);

	if (*selection != XA_PRIMARY &&
	    *selection != XA_CLIPBOARD(dpy))
		return (False);

	if (*target == XA_OL_COPY(dpy)) {
		CopyText();
        }

#ifdef XTERM_CUT
        else if (*target == XA_OL_CUT(dpy)) {
		 CutText();
	}
#endif

	else if (*target == XA_STRING && *selection == XA_PRIMARY) {

		/* Only do select stuff if non-null select */

		if (startHRow != endHRow || startHCol != endHCol) {
		    *value_return = SaltTextAway(startSRow, startSCol,
						 endSRow, endSCol);
		    *length_return = strlen(*value_return);
		}
		else {
		    *value_return = (char *) NULL;
		    *length_return = 0;
		}
		*format_return = 8;
		*type_return = XA_STRING;
		return (True);
	}
	else if (*target == XA_STRING &&
		 *selection == XA_CLIPBOARD(dpy)) {
		i = strlen (clip_contents);
		buffer = XtMalloc(1 + i);
#ifdef DEBUG
		printf("copies clip_content (%x) into buffer (%x)",
			clip_contents, buffer);
#endif /* DEBUG */
		strncpy (buffer, clip_contents, i);
		buffer[i] = '\0';
		*value_return = buffer;
		*length_return = i;
		*format_return = 8;
		*type_return = XA_STRING;
		return (True);
	}
	return (False);
}



static char *SaltTextAway(crow, ccol, row, col)
register crow, ccol, row, col;
/* Guaranteed that (crow, ccol) <= (row, col), and that both points are valid
   (may have row = screen->max_row+1, col = 0) */
{
	register TScreen *screen = &term->screen;
	register int i, j = 0;
	char *str, *lp;

	--col;
	/* first we need to know how long the string is before we can save it*/

	if (row == crow)
	    j = Length(screen, crow, ccol, col);
	else {	/* two cases, cut is on same line, cut spans multiple lines */
		j += Length(screen, crow, ccol, screen->max_col) + 1;
		for(i = crow + 1; i < row; i++) 
			j += Length(screen, i, 0, screen->max_col) + 1;
		if (col >= 0)
			j += Length(screen, row, 0, col);
	}
	
	/* now get some memory to save it in: we are going to re-create */
	/* the string from screen->buf.  This may involve inserting the */
	/* axilary characters SS2 and SS3.  At most we'll need 2*j char */

	str = XtMalloc(2*j+1);

	lp = str;		/* lp points to where to save the text */
	if ( row == crow ) lp = SaveText(screen, row, ccol, col, lp);
	else {
		lp = SaveText(screen, crow, ccol, screen->max_col, lp);
		*lp ++ = '\n';	/* put in newline at end of line */
		for(i = crow +1; i < row; i++) {
			lp = SaveText(screen, i, 0, screen->max_col, lp);
			*lp ++ = '\n';
			}
		if (col >= 0)
			lp = SaveText(screen, row, 0, col, lp);
	}
	*lp = '\0';		/* make sure we have end marked */
	return (str);
}


/* SS-copy */
PasteText()
{
	XtGetSelectionValue((Widget) term,
                            XA_CLIPBOARD(XtDisplay(term)),
                            XA_STRING,
                            ReadClipboard,
                            NULL,
                            CurrentTime);
}

PrimaryPaste()
{
	XtGetSelectionValue((Widget) term,
                            XA_PRIMARY,
                            XA_STRING,
                            ReadClipboard,
                            NULL,
                            CurrentTime);
}


CopyText()
{
	XtGetSelectionValue((Widget) term,
                            XA_PRIMARY,
                            XA_STRING,
                            ReadPrimary,
                            NULL,
                            CurrentTime);

	/* take the primary selection: this way if we copy from another	*/
	/* application, the selection will get un-hilited		*/

	XtOwnSelection((Widget) term, XA_PRIMARY,
	                    CurrentTime,
	                    ConvertSelection,
	                    LosePrimary,
	                    NULL);

	Have_hilite = FALSE;
	TrackText(0, 0, 0, 0);
	eventMode = NORMAL;
}


#ifdef XTERM_CUT
CutText()
{
	TScreen *screen = &term->screen;
	int save_cur_row, save_cur_col;

	XtGetSelectionValue((Widget) term,
                            XA_PRIMARY,
                            XA_STRING,
                            ReadPrimary,
                            NULL,
                            CurrentTime);

	/* take the primary selection: this way if we cut from another	*/
	/* application, the selection will get un-hilited		*/

	XtOwnSelection((Widget) term, XA_PRIMARY,
	                    CurrentTime,
	                    ConvertSelection,
	                    LosePrimary,
	                    NULL);

	Have_hilite = FALSE;
	TrackText(0, 0, 0, 0);
	eventMode = NORMAL;

	/* now remove the selected text from the screen.	    */
	/* save old cursor positions.  It will be restored after we */
	/* finish manipulating the content of the screen	    */

	save_cur_row = screen->cur_row;
	save_cur_col = screen->cur_col;
	Topline      = screen->topline;

	screen->cur_row = startSRow;
	screen->cur_col = startSCol;
	screen->topline = 0;

	if (startSRow == endSRow)
	    DeleteChar (screen, endSCol - startSCol);
	else
	{
	    register int del_lines = 0;

	    if (startSCol != 0)
	    {
	        register int width = screen->max_col - startSCol + 1;
	        register int len;

	    	DeleteChar (screen, width);
	
	    	if ((len = Length(screen,endSRow,endSCol,screen->max_col)) != 0)
	        {
		     ScrnBuf sb = screen->buf;
		     int     ii = 4 * (startSRow + Topline);
		     int     jj = 4 * (endSRow   + Topline);
    
		     register Char *schar = sb[ii]     + startSCol;
		     register Char *sattr = sb[ii + 1] + startSCol;
		     register Char *echar = sb[jj]     + endSCol;
		     register Char *eattr = sb[jj + 1] + endSCol;
    
		     register int sx = CursorX (screen, startSCol);
		     register int sy = CursorY (screen, startSRow);
		     register int ex = CursorX (screen, endSCol);
		     register int ey = CursorY (screen, endSRow);
    
		     /* calculate width of the area to be copied */

		     if (len <= width)
		         width = len;
    
		     XCopyArea(screen->display,
			       TextWindow(screen), TextWindow(screen),
			       screen->normalGC[0],
			       ex, ey,
			       width*FontWidth(screen), FontHeight(screen),
			       sx, sy);

		     /* adjust the screen buffer	*/

		     bcopy(echar, schar, width);
		     bcopy(eattr, sattr, width);
	          }

	          /* if there is no characters left on the last line  */
	          /* delete lines between start+1 and end inclusive.  */
	          /* if there are characters left, delete lines       */
	          /* start+1 and end-1				     */
    
	          screen->cur_row = startSRow + 1;
	          screen->cur_col = 0;
    
	          if (len <= width)
	          {
		      del_lines = endSRow - startSRow;
	              DeleteLine (screen, del_lines);
	          }
	          else
	          {
	      	      if (startSRow != endSRow - 1)
		      {
		          del_lines = endSRow - startSRow - 1;
		          DeleteLine (screen, del_lines);
	              }
	      	      DeleteChar(screen, endSCol + width);
	          }
	     }
	     else	/* startSCol == 0 */
	     {
		  if (endSCol == 0)
		  {
		      del_lines = endSRow - startSRow;
		      DeleteLine (screen, del_lines);
		  }
		  else
		  {
		      del_lines = endSRow - startSRow - 1;
		      DeleteLine (screen, del_lines);
		      DeleteChar (screen, endSCol);
		  }
	     }

	     /* if the screen was scrolled and some lines has been */
	     /* deleted, we must refresh the bottom of the screen  */

	     if (Topline != 0 && del_lines != 0)
	     {
		 if ((-Topline) < del_lines)
		     del_lines = -Topline;
		 ScrnRefresh (screen, screen->max_row - del_lines + 1, 0,
				      del_lines, screen->max_col + 1, FALSE);
	     }

	}
	screen->cur_row = save_cur_row;
	screen->cur_col = save_cur_col;
	screen->topline = Topline;
	Topline = 0;
	startSRow = startSCol = endSRow = endSCol = 0;
	eventMode = NORMAL;
}
#endif /* XTERM_CUT */


StoreText()
{
	TScreen *screen = &term->screen;
	char line[9];

	if (replyToEmacs) {
		if (rawRow == startSRow && rawCol == startSCol 
		 && lastRow == endSRow && lastCol == endSCol) {
		 	/* Use short-form emacs select */
			strcpy(line, "\033[t");
			line[3] = ' ' + endSCol + 1;
			line[4] = ' ' + endSRow + 1;
			v_write(screen->respond, line, 5);
		} else {
			/* long-form, specify everything */
			strcpy(line, "\033[T");
			line[3] = ' ' + startSCol + 1;
			line[4] = ' ' + startSRow + 1;
			line[5] = ' ' + endSCol + 1;
			line[6] = ' ' + endSRow + 1;
			line[7] = ' ' + lastCol + 1;
			line[8] = ' ' + lastRow + 1;
			v_write(screen->respond, line, 9);
		}
	}
	if (clip_contents != NULL)
	{
#ifdef DEBUG
		printf("frees the clip_content=%x and ", clip_contents);
#endif /* DEBUG */
	        XtFree(clip_contents);
	}

	clip_contents = SaltTextAway(startSRow, startSCol, endSRow, endSCol);
	if (!XtOwnSelection((Widget) term,
	                    XA_CLIPBOARD(XtDisplay(term)),
	                    CurrentTime,
	                    ConvertSelection,
	                    LoseClipboard,
	                    NULL)) {
#ifdef DEBUG
        	printf("xterm:   We didn't get the selection!\n");
#endif /* DEBUG */
	}
}
/* SS-copy-end */


/* SS-virt */
/* FLH mouseless */
/* map_button: an obsolete function to record the mouse button mapping
 * for the currently pressed mouse button.  It is made obsolete by
 * dynamic resouces of the toolkit.  The mapping is still needed
 * by the GINButton0() function (Tek mode).  map_button is called for every
 * Tek modebutton press, and the first thing that is done is to erase the
 * old settings.  The function has been changed to take an XEvent
 * rather than a XButtonEvent
 */

static void
map_button(w,ve, button)
Widget w;
OlVirtualEvent ve;
register int button;
{
	XEvent *event = ve->xevent;
	
	button_map_tbl[0] = button_map_tbl[1] = button_map_tbl[2] = (char) -1;

	switch(ve->virtual_name){
		case OL_SELECT:
			button_map_tbl[button] = SELECT_BUTTON;
			break;
		case OL_ADJUST:
			button_map_tbl[button] = ADJUST_BUTTON;
			break;
		case OL_MENU:
			button_map_tbl[button] = MENU_BUTTON;
			break;
		default:
			break;
	}
}
/* FLH mouseless-end */
/* SS-virt-end */
