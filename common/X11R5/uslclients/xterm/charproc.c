/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:charproc.c	1.64"
#endif

/*
 charproc.c (C source file)
	Acc: 575779325 Wed Mar 30 21:42:05 1988
	Mod: 575733524 Wed Mar 30 08:58:44 1988
	Sta: 575778143 Wed Mar 30 21:22:23 1988
	Owner: 1118
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

#include <X11/copyright.h>

#include <stdio.h>
#include <ctype.h>	/* for isprint()	*/
#include <errno.h>
#include "ptyx.h"  
#include "xterm_menu.h"
#include "VTparse.h"
#include "data.h"
#include "error.h"

/* #ifdef	SYSV */
#if defined(SYSV) || defined(SVR4)
#include	"xterm.h"	/* ehr3 - for EWOULDBLOCK */
#include	<stropts.h>	/* ehr3 - for streams */
#include	<poll.h>	/* ehr3 - for streams */
#include	<sys/utsname.h>	/* ehr3 - for ioctl_reply() */

extern	int	atttable[];
extern	int	ioctltable[];
#include	"xterm_ioctl.h"
char	my_screen_mode = M_C80x25;
#endif /* SYSV */

#include "Strings.h"
#include "messages.h"

extern void exit(), bcopy();

#define	DEFAULT		-1
#define	TEXT_BUF_SIZE	256
#define TRACKTIMESEC	4L
#define TRACKTIMEUSEC	0L


#ifndef lint
static char rcs_id[] = "$Header: charproc.c,v 1.23 88/02/27 15:27:38 rws Exp $";
#endif	/* lint */

/*
 * USL SVR4: need to define SYSV
 */
#define SYSV YES


#ifndef SYSV
static long arg;
#endif

static int nparam;
static ANSI reply;
static int param[NPARAM];

static unsigned long ctotal;
static unsigned long ntotal;
jmp_buf vtjmpbuf;

extern int groundtable[];
extern int csitable[];
extern int dectable[];
extern int eigtable[];
extern int esctable[];
extern int iestable[];
extern int igntable[];
extern int scrtable[];
extern int scstable[];


/* event handlers */
/*
extern void HandleKeyPressed();
extern void HandleEnterWindow();
extern void HandleLeaveWindow();
extern void HandleFocusChange();
extern void VTButtonPressed();
extern void VTMouseMoved();
extern void VTButtonReleased();
*/

/* SS-color */
Pixel	textFG;
Pixel	textBG;
 /* SS-color-end */

/*
 * NOTE: VTInitialize zeros out the entire ".screen" component of the 
 * XtermWidget, so make sure to add an assignment statement in VTInitialize() 
 * for each new ".screen" field added to this resource list.
 */

#ifndef	TIOCVTNAME
#define	VTNAMESZ	32
#endif


/*
 * RESIZE_REQUEST: request that xterm widget grow to (col,row) size
 */
#define RESIZE_REQUEST(col, row) XtMakeResizeRequest((Widget)(term), (screen->fullVwin.f_width * col) + (2 * screen->border), (screen->fullVwin.f_height * row) + (2 * screen->border), &term->core.width, &term->core.height)

static	Boolean	color_display = TRUE;
static	Boolean	local_display = FALSE;
static	Boolean	ega_display   = FALSE;

static	char	w_8[VTNAMESZ];	/* ehr3 - static retains val */

static  char	pending_num = 0;	/* number of pending characters	     */

void	/* cannot be static, called from main() */
get_vtname(screen)
TScreen	*screen;
{
	char	w_1[VTNAMESZ];
	char	w_2[VTNAMESZ];
	char	w_3[VTNAMESZ];
	char	w_4[VTNAMESZ];
	char	w_5[VTNAMESZ];
	char	w_6[VTNAMESZ];
	char	w_7[VTNAMESZ];
	extern	char	*XServerVendor();
	extern  char    *getenv();
	struct	utsname	my_utsname;
	char	display_system[20];
	char	x_display[40];
	int	index;
	char    *ioctl_buf;

        ioctl_buf = XtMalloc(strlen(XServerVendor(screen->display)) +1);
	(void) strcpy(ioctl_buf, XServerVendor(screen->display));

	(void) sscanf(ioctl_buf, "%s %s %s %s %s %s %s %s", w_1, w_2, w_3, w_4, w_5, w_6, w_7, w_8);
	XtFree(ioctl_buf);

	/* Known vendor strings

	  	1	2   3	4       5     	  6       7     8
		"AT&T 6386 WGS with monochrome VDC-400 (using xxxx)"
				    grayscale  VDC-600
					       VGA
				    color      VDC-750
					       EGA
					       VDC-600
					       VGA
	*/

	/* We need to extract the display type (cga/ega/vga) information */
	/* from the vendor string.  If not available, we assume ega for  */
	/* color displays, cga for monochrome and grayscale.  We know	 */
	/* whether display is color or by examining the display depth    */
	/* in VTInitialize().						 */

	/* MORE: currently, there is no requirements to support VGA mode */
	/*       so VGA displays will be treated as EGA			 */

	if ((strcmp(w_6, "VDC-750") == 0) || (strcmp(w_6, "VDC-600") == 0) ||
	    (strcmp(w_6, "EGA") == 0) || (strcmp(w_6, "VGA") == 0))
	    ega_display = 1;
	else if (strcmp(w_6, "VDC-400") == 0)
	    ega_display = 0;
	else if (color_display)
	    ega_display = 1;
	else
	    ega_display = 0;

	w_8[strlen(w_8) - 1] = '\0';    /* drop trailing ')' */

	/* Get display token.  If ${DISPLAY} is: :0, :0.0, unix:0, unix:0.0 */
	/* then we are local. Otherwise if uname != the part before the ':' */
	/* then we are remote.  					    */

       	(void) strcpy(x_display, getenv("DISPLAY"));

	if ((strcmp(x_display, ":0") == 0) ||
	    (strcmp(x_display, ":0.0") == 0) ||
	    (strcmp(x_display, "unix:0") == 0) ||
	    (strcmp(x_display, "unix:0.0") == 0)) {
	    local_display = 1;
	}
	else {
		/* Not a known ${DISPLAY}. Check part of ${DISPLAY} before */
		/* the :						   */

	    for (index = 0; x_display[index] != '\0' &&
			    x_display[index] != ':'; index++)
		display_system[index] = x_display[index];

		display_system[index] = '\0';

		/* Does display_system == uname? If not, then we are remote */

		if (uname(&my_utsname) < 0) {
#if !defined(I18N)
			(void) fprintf(stderr, "uname() failed\n");
			perror("	Reason");
#else
        		perror( OlGetMessage(screen->display, NULL, 0, OleNperror,
					OleTreason, OleCOlClientXtermMsgs,
					OleMperror_reason, NULL));
			OlVaDisplayErrorMsg(screen->display, OleNuname,
					OleTbadUname,
					OleCOlClientXtermMsgs,
					OleMuname_badUname, NULL);
#endif
			exit(1);
		}

		if (strcmp(my_utsname.nodename, display_system) == 0)
		    local_display = 1;
		else
		    local_display = 0;
	}
}



VTparse()
{
	register TScreen *screen = &term->screen;
	register int *parsestate = groundtable;
	register int c, i;
	register Char *cp;
	register int row, col, top, bot, scstype;
	extern int bitset(), bitclr(), finput(), TrackMouse(), Changetitle(),
		   VTReset();
/* SS-mouse */
	extern void rep_mouse_pos();
/* SS-mouse-end */
/* EHR3-ioctl */
	char ioctl_buf[256];
/* EHR3-ioctl-end */

/* SS-color */
	textFG = screen->foreground;
	textBG = screen->background;
/* SS-color-end */

	if(setjmp(vtjmpbuf))
		parsestate = groundtable;

/* SS added */

	for( ; ; )
	    /* if MSB is on, we cannot use parse tables	*/

	    if ((c = doinput()) & 0x80) {

		char code_set, new_set, code_width;
		OlFontList    *fontl = term->primitive.font_list;

		if (fontl == (OlFontList *) NULL) {
		    c &= ~0x80;
		    goto ascii_char;
		}

		/* find out the code_set we are dealing with	*/
		
		switch (c) {
		   case SS2:	code_set = 2;
				break;
		   case SS3:	code_set = 3;
				break;
		   default:	code_set = 1;
				break;
		}

		if (code_set >= fontl->num) {
		    c &= ~0x80;
		    goto ascii_char;
		}

		code_width = fontl->cswidth[code_set];
		if (code_width == 0) {
		    c &= ~0x80;
		    goto ascii_char;
		}

		if (code_width > 1)
		    Using_wide = TRUE;

		/* take into account SS2 and SS3 */

		if (code_set > 1)
		    code_width++;

		bptr--;
		bcnt++;

		/* if there is not enough bytes in the buffer, save the */
		/* remaining characters, and force another read		*/

		if (bcnt < code_width) {
		    pending_num = (char) bcnt;
		    for (i=0; i<pending_num; i++, bptr++) {
			 buffer[i] = *bptr;
		    }
		    bcnt = -1;
		    continue;
		}

		top = bcnt > TEXT_BUF_SIZE ? TEXT_BUF_SIZE : bcnt;
		cp = bptr;

		while (top >= code_width) {

		    top  -= code_width;
		    bcnt -= code_width;
		    cp   += code_width;

		    /* if the end of input buffer, or next character	*/
		    /* doesn't have to MSB set, break 			*/

		    if (top <= 0 || !(*cp & 0x80))
			break;

		    switch (*cp) {
		       case SS2:    new_set = 2;
				    break;
		       case SS3:    new_set = 3;
				    break;
		       default:	    new_set = 1;
				    break;
		    }
		    if (new_set != code_set)
			break;
		}

		if (bptr < cp) {
		    dotext(screen, term->flags, 'B', bptr, cp, code_set);
		    bptr = cp;
		}
	    }
	    else  {
ascii_char:   switch(parsestate[c &= CHAR]) {
		 case CASE_GROUND_STATE:
			/* exit ignore mode */
			parsestate = groundtable;
			if (This_is_alt)
			    goto into_case_print;
			break;

		 case CASE_PRINT:
			/* printable characters */
into_case_print:
			top = bcnt > TEXT_BUF_SIZE ? TEXT_BUF_SIZE : bcnt;
			cp = bptr;
			*--bptr = c;
			while(top > 0 && isprint(*cp)) {
				top--;
				bcnt--;
				cp++;
			}
			if(screen->curss) {
				dotext(screen, term->flags,
				       screen->gsets[screen->curss],
				       bptr, bptr + 1, 0);
				screen->curss = 0;
				bptr++;
			}
			if(bptr < cp)
				dotext(screen, term->flags,
				 screen->gsets[screen->curgl], bptr, cp, 0);
			bptr = cp;
			break;

		 case CASE_IGNORE_STATE:
			/* Ies: ignore anything else */
			parsestate = igntable;
			break;

		 case CASE_IGNORE_ESC:
			/* Ign: escape */
			parsestate = iestable;
			break;

		 case CASE_IGNORE:
			/* Ignore character */
/* SS-fk */
			if (This_is_fk)
			{
			    This_is_fk = FALSE;
			    parsestate = groundtable;
			}
			if (This_is_alt)
			{
			    This_is_alt = FALSE;
			    parsestate = groundtable;
			}
/* SS-fk-end */
			break;

		 case CASE_BELL:
			/* bell */
			Bell();
			break;

		 case CASE_BS:
			/* backspace */
			CursorBack(screen, 1);
			break;

		 case CASE_CR:
			/* carriage return */
			CarriageReturn(screen);
			break;

		 case CASE_ESC:
			/* escape */
			parsestate = esctable;
			break;

		 case CASE_VMOT:
			/*
			 * form feed, line feed, vertical tab
			 */
			Index(screen, 1);
			if (term->flags & LINEFEED)
				CarriageReturn(screen);
			if (screen->display->qlen > 0 ||
			    GetBytesAvailable (screen->display->fd) > 0)
			    xevents();
			break;

		 case CASE_TAB:
			/* tab */
			screen->cur_col = TabNext(term->tabs, screen->cur_col);
			if (screen->cur_col > screen->max_col)
			    screen->cur_col = screen->max_col;
/* SS-tab */
			if (screen->cur_col == screen->max_col)
			    screen->do_wrap = 1;
/* SS-tab-end */
			break;

		 case CASE_SI:
			screen->curgl = 0;
			break;

		 case CASE_SO:
			screen->curgl = 1;
			break;

		 case CASE_SCR_STATE:
			/* enter scr state */
			parsestate = scrtable;
			break;

		 case CASE_SCS0_STATE:
			/* enter scs state 0 */
			scstype = 0;
			parsestate = scstable;
			break;

		 case CASE_SCS1_STATE:
			/* enter scs state 1 */
			scstype = 1;
			parsestate = scstable;
			break;

		 case CASE_SCS2_STATE:
			/* enter scs state 2 */
			scstype = 2;
			parsestate = scstable;
			break;

		 case CASE_SCS3_STATE:
			/* enter scs state 3 */
			scstype = 3;
			parsestate = scstable;
			break;

		 case CASE_ESC_IGNORE:
			/* unknown escape sequence */
			parsestate = eigtable;
			break;

		 case CASE_ESC_DIGIT:
			/* digit in csi or dec mode */
			if((row = param[nparam - 1]) == DEFAULT)
				row = 0;
			param[nparam - 1] = 10 * row + (c - '0');
			break;

		 case CASE_ESC_SEMI:
			/* semicolon in csi or dec mode */
			param[nparam++] = DEFAULT;
			break;

		 case CASE_DEC_STATE:
			/* enter dec mode */
			parsestate = dectable;
			break;

		 case CASE_ICH:
			/* ICH */
			if((c = param[0]) < 1)
				c = 1;
			InsertChar(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_CUU:
			/* CUU */
			if((c = param[0]) < 1)
				c = 1;
			CursorUp(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_CUD:
			/* CUD */
			if((c = param[0]) < 1)
				c = 1;
			CursorDown(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_CUF:
			/* CUF */
			if((c = param[0]) < 1)
				c = 1;
			CursorForward(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_CUB:
			/* CUB */
			if((c = param[0]) < 1)
				c = 1;
			CursorBack(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_CUP:
			/* CUP | HVP */
			if((row = param[0]) < 1)
				row = 1;
			if(nparam < 2 || (col = param[1]) < 1)
				col = 1;
			CursorSet(screen, row-1, col-1, term->flags);
			parsestate = groundtable;
			break;

		 case CASE_ED:
			/* ED */
			switch (param[0]) {
			 case DEFAULT:
			 case 0:
				ClearBelow(screen);
				break;

			 case 1:
				ClearAbove(screen);
				break;

			 case 2:
				ClearScreen(screen);
/* SS-color */
				if (Using_colors)
				{
				    if (textFG == screen->foreground &&
				  	textBG == screen->background)
					Using_colors = FALSE;
				}
/* SS-color-end */
				/* no need to check if (Using_wide), we're setting it False */
				Using_wide = FALSE;
				break;
			}
			parsestate = groundtable;
			break;

		 case CASE_EL:
			/* EL */
			switch (param[0]) {
			 case DEFAULT:
			 case 0:
				ClearRight(screen);
				break;
			 case 1:
				ClearLeft(screen);
				break;
			 case 2:
				ClearLine(screen);
				break;
			}
			parsestate = groundtable;
			break;

		 case CASE_IL:
			/* IL */
			if((c = param[0]) < 1)
				c = 1;
			InsertLine(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_DL:
			/* DL */
			if((c = param[0]) < 1)
				c = 1;
			DeleteLine(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_DCH:
			/* DCH */
			if((c = param[0]) < 1)
				c = 1;
			DeleteChar(screen, c);
			parsestate = groundtable;
			break;

		 case CASE_TRACK_MOUSE:
		 	/* Track mouse as long as in window and between
			   specified rows */
			TrackMouse(param[0], param[2]-1, param[1]-1,
			 param[3]-1, param[4]-2);
			break;

		 case CASE_DA1:
			/* DA1 */
			if (param[0] <= 0) {	/* less than means DEFAULT */
				reply.a_type   = CSI;
				reply.a_pintro = '?';
				reply.a_nparam = 2;
				reply.a_param[0] = 1;		/* VT102 */
				reply.a_param[1] = 2;		/* VT102 */
				reply.a_inters = 0;
				reply.a_final  = 'c';
				unparseseq(&reply, screen->respond);
			}
			parsestate = groundtable;
			break;

		 case CASE_TBC:
			/* TBC */
			if ((c = param[0]) <= 0) /* less than means default */
				TabClear(term->tabs, screen->cur_col);
			else if (c == 3)
				TabZonk(term->tabs);
			parsestate = groundtable;
			break;

		 case CASE_SET:
			/* SET */
			modes(term, bitset);
			parsestate = groundtable;
			break;

		 case CASE_RST:
			/* RST */
			modes(term, bitclr);
			parsestate = groundtable;
			break;

		 case CASE_SGR:
			/* SGR */
			for (c=0; c<nparam; ++c) {
				switch (param[c]) {
				 case DEFAULT:
				 case 0:
					term->flags &= ~(INVERSE|BOLD|UNDERLINE);
					break;
				 case 1:
				 case 5:	/* Blink, really.	*/
					term->flags |= BOLD;
					break;
				 case 4:	/* Underscore		*/
					term->flags |= UNDERLINE;
					break;
				 case 7:
					term->flags |= INVERSE;
/* SS-color */
					break;
				 case 30:
				 case 31:
				 case 32:
				 case 33:
				 case 34:
				 case 35:
				 case 36:
				 case 37:
					textFG = PixelValue(param[c]-30); 
					Using_colors = TRUE;
					term->flags |= USE_FG_COLOR;
					break;
				 case 40:
				 case 41:
				 case 42:
				 case 43:
				 case 44:
				 case 45:
				 case 46:
				 case 47:
					textBG = PixelValue(param[c]-40);
					Using_colors = TRUE;
					term->flags |= USE_BG_COLOR;
					break;
				 case 100:
					textFG = screen->foreground;
					textBG = screen->background;
					term->flags &= ~(USE_FG_COLOR |
							 USE_BG_COLOR);
					break;
/* SS-color-end */
				}
			}
			parsestate = groundtable;
			break;

		 case CASE_CPR:
			/* CPR */
			if ((c = param[0]) == 5) {
				reply.a_type = CSI;
				reply.a_pintro = 0;
				reply.a_nparam = 1;
				reply.a_param[0] = 0;
				reply.a_inters = 0;
				reply.a_final  = 'n';
				unparseseq(&reply, screen->respond);
			} else if (c == 6) {
				reply.a_type = CSI;
				reply.a_pintro = 0;
				reply.a_nparam = 2;
				reply.a_param[0] = screen->cur_row+1;
				reply.a_param[1] = screen->cur_col+1;
				reply.a_inters = 0;
				reply.a_final  = 'R';
				unparseseq(&reply, screen->respond);
			}
			parsestate = groundtable;
			break;

		 case CASE_DECSTBM:
			/* DECSTBM */
			if((top = param[0]) < 1)
				top = 1;
			if(nparam < 2 || (bot = param[1]) == DEFAULT
			   || bot > screen->max_row + 1
			   || bot == 0)
				bot = screen->max_row+1;
			if (bot > top) {
				if(screen->scroll_amt)
					FlushScroll(screen);
				screen->top_marg = top-1;
				screen->bot_marg = bot-1;
				CursorSet(screen, 0, 0, term->flags);
			}
			parsestate = groundtable;
			break;

		 case CASE_DECREQTPARM:
			/* DECREQTPARM */
			if ((c = param[0]) == DEFAULT)
				c = 0;
			if (c == 0 || c == 1) {
				reply.a_type = CSI;
				reply.a_pintro = 0;
				reply.a_nparam = 7;
				reply.a_param[0] = c + 2;
				reply.a_param[1] = 1;	/* no parity */
				reply.a_param[2] = 1;	/* eight bits */
				reply.a_param[3] = 112;	/* transmit 9600 baud */
				reply.a_param[4] = 112;	/* receive 9600 baud */
				reply.a_param[5] = 1;	/* clock multiplier ? */
				reply.a_param[6] = 0;	/* STP flags ? */
				reply.a_inters = 0;
				reply.a_final  = 'x';
				unparseseq(&reply, screen->respond);
			}
			parsestate = groundtable;
			break;

		 case CASE_DECSET:
			/* DECSET */
			dpmodes(term, bitset);
			parsestate = groundtable;
			break;

		 case CASE_DECRST:
			/* DECRST */
			dpmodes(term, bitclr);
			parsestate = groundtable;
			break;

		 case CASE_DECALN:
			/* DECALN */
			if(screen->cursor_state)
				HideCursor();
			for(row = screen->max_row ; row >= 0 ; row--) {
				bzero(screen->buf[4 * row + 1],
				 col = screen->max_col + 1);
/* SS-color-? */
				for(cp = screen->buf[4 * row] ; col > 0 ; col--)
					*cp++ = 'E';
			}
/* SS-color */
			ScrnRefresh(screen, 0, 0, screen->max_row + 1,
			 			  screen->max_col + 1, FALSE);
/* SS-color-end */
			parsestate = groundtable;
			break;

		 case CASE_GSETS:
			screen->gsets[scstype] = c;
			parsestate = groundtable;
			break;

		 case CASE_DECSC:
			/* DECSC */
			CursorSave(term, &screen->sc);
			parsestate = groundtable;
			break;

		 case CASE_DECRC:
			/* DECRC */
			CursorRestore(term, &screen->sc);
			parsestate = groundtable;
			break;

		 case CASE_IND:
			/* IND */
			Index(screen, 1);
			if (screen->display->qlen > 0 ||
			    GetBytesAvailable (screen->display->fd) > 0)
			    xevents();
			parsestate = groundtable;
			break;

		 case CASE_NEL:
			/* NEL */
			Index(screen, 1);
			CarriageReturn(screen);
			if (screen->display->qlen > 0 ||
			    GetBytesAvailable (screen->display->fd) > 0)
			    xevents();
			parsestate = groundtable;
			break;

		 case CASE_HTS:
			/* HTS */
			TabSet(term->tabs, screen->cur_col);
			parsestate = groundtable;
			break;

		 case CASE_RI:
			/* RI */
			RevIndex(screen, 1);
			parsestate = groundtable;
			break;

		 case CASE_SS2:
			/* SS2 */
/* SS-alt */
			if (This_is_alt)
			{
			    parsestate = groundtable;
			    goto into_case_print;
			}
			else
			{
/* SS-alt-end */
			    screen->curss = 2;
			    parsestate = groundtable;
			    break;
			}

		 case CASE_SS3:
			/* SS3 */
/* SS-fk */
			if (This_is_fk)
			{
			    parsestate = groundtable;
			    goto into_case_print;
			}
			else
			{
/* SS-fk-end */
			    screen->curss = 3;
			    parsestate = groundtable;
			    break;
			}

		 case CASE_CSI_STATE:
			/* enter csi state */
			nparam = 1;
			param[0] = DEFAULT;
			parsestate = csitable;
			break;

		 case CASE_OSC:
			/* do osc escapes */
			do_osc(finput);
			parsestate = groundtable;
			break;

		 case CASE_RIS:
			/* RIS */
			VTReset(TRUE, (Widget) NULL);
			parsestate = groundtable;
			break;

		 case CASE_LS2:
			/* LS2 */
			screen->curgl = 2;
			parsestate = groundtable;
			break;

		 case CASE_LS3:
			/* LS3 */
			screen->curgl = 3;
			parsestate = groundtable;
			break;

		 case CASE_LS3R:
			/* LS3R */
			screen->curgr = 3;
			parsestate = groundtable;
			break;

		 case CASE_LS2R:
			/* LS2R */
			screen->curgr = 2;
			parsestate = groundtable;
			break;

		 case CASE_LS1R:
			/* LS1R */
			screen->curgr = 1;
			parsestate = groundtable;
			break;

		 case CASE_XTERM_SAVE:
			savemodes(term);
			parsestate = groundtable;
			break;

		 case CASE_XTERM_RESTORE:
			restoremodes(term);
			parsestate = groundtable;
			break;

/* SS-add */
		 case CASE_XTERM_TITLE:
			{
			    register int c, i=0;
			    register char *cp;
			    char buf[512];
    
			    cp = buf;
			    while((++i < 512) && isprint(c = finput()))
				    *cp++ = c;
			    *cp = 0;
    			    if (title)
        	    	        free (title);
			    if (i == 1)
#if !defined(I18N)
    			        strcpy(buf, "Untitled");
#else
        			strcpy(buf, OlGetMessage(screen->display, NULL, 0,
					 	OleNtitle, OleTuntitled,
						OleCOlClientXtermMsgs,
						OleMtitle_untitled, NULL));
#endif

    			    title = strdup(buf);
			    if (screen->grabbedKbd)
#if !defined(I18N)
				sprintf(buf, "%s: Secure Keyboard", title);
#else
			sprintf(buf, 
				OlGetMessage(screen->display, NULL, 0,
					OleNtitle, OleTsecureKbd,
					OleCOlClientXtermMsgs, NULL,
					NULL), title);
#endif
			    Changetitle(buf);
			    break;
			}
/* SS-add-end */
/* SS-mouse */
		 case CASE_GET_MOUSE:
			Get_Mouse_Events = (param[0] < 0) ? 0 : param[0];
			parsestate = groundtable;
			break;


		 case CASE_REQ_MOUSE_POS:
			if (param[0] == 492)
			    rep_mouse_pos(screen);
			if (screen->display->qlen > 0 ||
			    GetBytesAvailable (screen->display->fd) > 0)
			    xevents();
			parsestate = groundtable;
			break;
/* SS-mouse-end */

		 /* EHR3 - For AT&T sequences */
		 case CASE_ATT_STATE:
			/* enter AT&T mode */
			parsestate = atttable;
			break;

		 case CASE_KDDISPTYPE_STATE:	/* a */
			/* KDDISPTYPE - kd_disparam 136 bytes */
			RETURN_VALUE(ioctl_buf, 'a', IOCTL_GOOD_RC, '\003');

			/*
				We do not want to return valid
				addresses - Just zeroes.
			*/
			for (i = 9; i < 145; i++)
				ioctl_buf[i] = '\0';

			REPLY_TO_IOCTL(ioctl_buf, 145, "KDDISPTYPE");
			parsestate = groundtable;
			break;

		 case CASE_KDGKBENT_STATE:	/* b */
			/*RETURN_VALUE('b', IOCTL_GOOD_RC, '\001');
			REPLY_TO_IOCTL(ioctl_buf, 9, "KDGKBENT");*/
			kdgkbent (screen, 'b');
			parsestate = groundtable;
			break;

		 case CASE_KDSKBENT_STATE:	/* c */
			/* IOCTL_READ(4);
			RETURN_VALUE(ioctl_buf, 'c', IOCTL_GOOD_RC, '\001');
			REPLY_TO_IOCTL(ioctl_buf, 9, "KDSKBENT"); */
			kdskbent (screen, 'c');
			parsestate = groundtable;
			break;

		 case CASE_KDGKBMODE_STATE:	/* d */
			/* KDGKBMODE - Value 0x01 */
			RETURN_VALUE(ioctl_buf, 'd', IOCTL_GOOD_RC, '\001');
			REPLY_TO_IOCTL(ioctl_buf, 9, "KDGKBMODE");
			parsestate = groundtable;
			break;

		 case CASE_KDSKBMODE_STATE:	/* e */
			/* KDSKBMODE */
			RETURN_VALUE(ioctl_buf, 'e', IOCTL_GOOD_RC, '\001');
			REPLY_TO_IOCTL(ioctl_buf, 9, "KDSKBMODE");
			parsestate = groundtable;
			break;

		 case CASE_GIO_ATTR_STATE:	/* f */
			/* GIO_ATTR */
			RETURN_VALUE(ioctl_buf, 'f', IOCTL_GOOD_RC, '\001');
			REPLY_TO_IOCTL(ioctl_buf, 9, "KDGKBMODE");
			parsestate = groundtable;
			break;

		 case CASE_GIO_COLOR_STATE:	/* g */
			/* GIO_COLOR - Value 1 == color */
			RETURN_VALUE(ioctl_buf, 'g', IOCTL_GOOD_RC, color_display ? '\001' : '\000');
			REPLY_TO_IOCTL(ioctl_buf, 9, "GIO_COLOR");
			parsestate = groundtable;
			break;

		 case CASE_GIO_KEYMAP_STATE:	/* h */
			gio_keymap (screen, 'h');
			parsestate = groundtable;
			break;

		 case CASE_PIO_KEYMAP_STATE:	/* i */
			pio_keymap (screen, 'i');
			parsestate = groundtable;
			break;

		 case CASE_GIO_STRMAP_STATE:	/* j */
			gio_strmap (screen, 'j');
			parsestate = groundtable;
			break;

		 case CASE_PIO_STRMAP_STATE:	/* k */
			pio_strmap (screen, 'k');
			parsestate = groundtable;
			break;

		 case CASE_GETFKEY_STATE:	/* l */
			getfkey (screen, 'l');
			parsestate = groundtable;
			break;

		 case CASE_SETFKEY_STATE:	/* m */
			setfkey (screen, 'm');
			parsestate = groundtable;
			break;

		 case CASE_SW_B40X25_STATE:	/* n */
			/* SW_B40X25 - RC */
			if (color_display == 0) {
			    RESIZE_REQUEST(40, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'n', IOCTL_GOOD_RC);
			    my_screen_mode = M_B40x25;
			}
			else  {
			    RETURN_BUF_HEADER(ioctl_buf, 'n', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_B40X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_C40X25_STATE:	/* o */
			/* SW_C40X25 - RC */
			if (color_display) {
			    RESIZE_REQUEST(40, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'o', IOCTL_GOOD_RC);
			    my_screen_mode = M_C40x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'o', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_C40X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_B80X25_STATE:	/* p */
			/* SW_B80X25 - RC */
			if (color_display == 0) {
			    /* succeed */
			    RESIZE_REQUEST(80, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'p', IOCTL_GOOD_RC);
			    my_screen_mode = M_B80x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'p', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_B80X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_C80X25_STATE:	/* q */
			/* SW_C80X25 - RC */
			if (color_display) {
			    RESIZE_REQUEST(80, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'q', IOCTL_GOOD_RC);
			    my_screen_mode = M_C80x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'q', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_C80X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_EGAMONO80X25_STATE:	/* r */
			/* SW_EGAMONO80X25 - RC */
			if (ega_display) {
			    RESIZE_REQUEST(80, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'r', IOCTL_GOOD_RC);
			    my_screen_mode = M_EGAMONO80x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'r', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "M_EGAMONO80X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_ENHB40X25_STATE:	/* s */
			/* SW_ENHB40X25 */
			if (ega_display) {
			    RESIZE_REQUEST(40, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 's', IOCTL_GOOD_RC);
			    my_screen_mode = M_ENH_B40x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 's', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_ENHB40X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_ENHC40X25_STATE:	/* t */
			/* SW_ENHC40X25 - RC */
			if (color_display && ega_display) {
			    RESIZE_REQUEST(40, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 't', IOCTL_GOOD_RC);
			    my_screen_mode = M_ENH_C40x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 't', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_ENHC40X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_ENHB80X25_STATE:	/* u */
			/* SW_ENHB80X25 - RC */
			if (ega_display) {
			    RESIZE_REQUEST(80, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'u', IOCTL_GOOD_RC);
			    my_screen_mode = M_ENH_B80x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'u', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_ENHB80X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_ENHC80X25_STATE:	/* v */
			/* SW_ENHC80X25 - RC */
			if (color_display && ega_display) {
			    RESIZE_REQUEST(80, 25);
			    RETURN_BUF_HEADER(ioctl_buf, 'v', IOCTL_GOOD_RC);
			    my_screen_mode = M_ENH_C80x25;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'v', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_ENHC80X25");
			parsestate = groundtable;
			break;

		 case CASE_SW_ENHB80X43_STATE:	/* w */
			/* SW_ENHB80X43 - RC */
			if (ega_display) {
			    RESIZE_REQUEST(80, 43);
			    RETURN_BUF_HEADER(ioctl_buf, 'w', IOCTL_GOOD_RC);
			    my_screen_mode = M_ENH_B80x43;
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'w', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_B80X43");
			parsestate = groundtable;
			break;

		 case CASE_SW_ENHC80X43_STATE:	/* x */
			/* SW_ENHC80X43 - RC */
			if (color_display && ega_display) {
			    RESIZE_REQUEST(80, 43);
			    RETURN_BUF_HEADER(ioctl_buf, 'x', IOCTL_GOOD_RC);
			    my_screen_mode = M_ENH_C80x43;
			} else {
			    RETURN_BUF_HEADER(ioctl_buf, 'x', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_ENHC80X43");
			parsestate = groundtable;
			break;

		 case CASE_SW_MCAMODE_STATE:	/* y */
			/* SW_MCAMODE - RC */
			if (color_display == 0) {
			    RETURN_BUF_HEADER(ioctl_buf, 'y', IOCTL_GOOD_RC);
			}
			else {
			    RETURN_BUF_HEADER(ioctl_buf, 'y', IOCTL_BAD_RC);
			}

			REPLY_TO_IOCTL(ioctl_buf, 5, "SW_MCAMODE");
			parsestate = groundtable;
			break;

		 case CASE_CONS_CURRENT_STATE:	/* z */
			/* CONS_CURRENT - Value is EGA | CGA */
			RETURN_VALUE(ioctl_buf, 'z', IOCTL_GOOD_RC, ega_display ? '\004' : '\002');
			REPLY_TO_IOCTL(ioctl_buf, 9, "CONS_CURRENT");
			parsestate = groundtable;
			break;

		 case CASE_CONS_GET_STATE:	/* A */
			/* CONS_GET */
			RETURN_VALUE(ioctl_buf, 'A', IOCTL_GOOD_RC, my_screen_mode);
			REPLY_TO_IOCTL(ioctl_buf, 9, "CONS_GET");
			parsestate = groundtable;
			break;

		 case CASE_TIOCVTNAME_STATE:	/* B */
			/* TIOCVTNAME */
			RETURN_BUF_HEADER(ioctl_buf,'B',IOCTL_GOOD_RC);

			if (local_display == 0) { /* fail */
				/* Build buffer for failure */
				ioctl_buf[4] = IOCTL_BAD_RC;
#if !defined(I18N)
				(void) strcpy(ioctl_buf+5, "TIOCVTNAME Failed: Not local....");
#else
				(void) strcpy(ioctl_buf+5, OlGetMessage(
						screen->display, NULL, 0,
						OleNname, OleTnotLocal,
						OleCOlClientXtermMsgs,
						OleMname_notLocal, NULL));
#endif
			} else {
				/* Build buffer for success */
				int	buf_ind, w_ind;

				ioctl_buf[4] = IOCTL_GOOD_RC;

				/* Load the display name */
				for (buf_ind = 5, w_ind = 0; w_8[w_ind] != '\0'; buf_ind++, w_ind++)
					ioctl_buf[buf_ind] = w_8[w_ind];

				/* Pad the buffer */
				for (; buf_ind < VTNAMESZ + 5; buf_ind++)
					ioctl_buf[buf_ind] = '\0';
			}

			REPLY_TO_IOCTL(ioctl_buf, (VTNAMESZ + 5), "TIOCVTNAME");
			parsestate = groundtable;
			break;

		 case CASE_GIO_SCRNMAP_STATE:  /* C */
			gio_scrnmap (screen, 'C');
			parsestate = groundtable;
			break;

		 case CASE_PIO_SCRNMAP_STATE:  /* D */
			pio_scrnmap (screen, 'D');
			parsestate = groundtable;
			break;

		 case CASE_IOCTLRECV_STATE:
			/* We are receiving an ioctl - EHR3 */
			parsestate = ioctltable;
			break;

		 case CASE_INCURSES_STATE:
			/* FLH resize */
			do_smcup();
			/* FLH resize-end */
			parsestate = groundtable;
			break;
			/* EHR3 - End disable reshape */


		 case CASE_OUTCURSES_STATE:
			/* enable reshape - EHR3 */
			/* FLH resize */
			do_rmcup();
			/* FLH resize-end */
			parsestate = groundtable;
			break;
			/* EHR3 - End enable reshape */

		 /* EHR3 - END AT&T sequences */
		}
	    }
}

finput()
{
	return(doinput());
}

static int select_mask;
static int write_mask;

static char v_buffer[4096];
static char *v_bufstr;
static char *v_bufptr;
static char *v_bufend;

#define	ptymask()	(v_bufptr > v_bufstr ? pty_mask : 0)

v_write(f, d, l)
int f;
char *d;
int l;
{
	register TScreen *screen = &term->screen;
	int r;
	int c = l;

	if (!v_bufstr) {
		v_bufstr = v_buffer;
		v_bufptr = v_buffer;
		v_bufend = &v_buffer[4096];
	}


	if ((1 << f) != pty_mask)
		return(write(f, d, l));

	if (v_bufptr > v_bufstr) {
		if (l) {
			if (v_bufend > v_bufptr + l) {
				bcopy(d, v_bufptr, l);
				v_bufptr += l;
			} else {
				if (v_bufstr != v_buffer) {
					bcopy(v_bufstr, v_buffer,
					      v_bufptr - v_bufstr);
					v_bufptr -= v_bufstr - v_buffer;
					v_bufstr = v_buffer;
				}
				if (v_bufend > v_bufptr + l) {
					bcopy(d, v_bufptr, l);
					v_bufptr += l;
				} else if (v_bufptr < v_bufend) {
#if !defined(I18N)
					fprintf(stderr, "Out of buffer space\n");
#else
					OlVaDisplayWarningMsg(screen->display,
						OleNspace, OleTbuffer,
						OleCOlClientXtermMsgs,
						OleMspace_buffer, NULL);
#endif
					c = v_bufend - v_bufptr;
					bcopy(d, v_bufptr, c);
					v_bufptr = v_bufend;
				} else {
#if !defined(I18N)
					fprintf(stderr, "Out of buffer space\n");
#else
					OlVaDisplayWarningMsg(screen->display,
						OleNspace, OleTbuffer,
						OleCOlClientXtermMsgs,
						OleMspace_buffer, NULL);
#endif
					c = 0;
				}
			}
		}
		if (v_bufptr > v_bufstr) {
			if ((r = write(f, v_bufstr, v_bufptr - v_bufstr)) <= 0)
				return(r);
			if ((v_bufstr += r) >= v_bufptr)
				v_bufstr = v_bufptr = v_buffer;
		}
	} else if (l) {
		errno = 0;
		if ((r = write(f, d, l)) < 0) {
			if (errno == EWOULDBLOCK)
				r = 0;
			else if (errno == EINTR)
				r = 0;
			else
				return(r);
		}
		if (l - r) {
			if (l - r > v_bufend - v_buffer) {
#if !defined(I18N)
				fprintf(stderr, "Truncating to %d\n",
						v_bufend - v_buffer);
#else
				OlVaDisplayWarningMsg(screen->display, OleNspace,
					OleTtruncate,
					OleCOlClientXtermMsgs,
					OleMspace_truncate,
					v_bufend - v_buffer);
#endif
				l = (v_bufend - v_buffer) + r;
			}
			bcopy(d + r, v_buffer, l - r);
			v_bufstr = v_buffer;
			v_bufptr = v_buffer + (l - r);
		}
	}
	return(c);
}


in_put()
{
	register TScreen *screen = &term->screen;
	register Char *cp;
	register int i;
	static struct timeval trackTimeOut;
#ifdef SVR4 
	extern int console_input();
	extern char console_has_input;
	int console_cnt;	
	Char *cptr;
	int buf_size;
#endif

	select_mask = pty_mask;	/* force initial read */
	for( ; ; ) {
		if((select_mask & pty_mask) && (eventMode == NORMAL)) {
			if(screen->logging)
				FlushLog(screen);
			errno = 0;

			/* the first pending_num characters of the buffer */
			/* are reserved					  */

			if((bcnt = read(screen->respond,
					bptr = &buffer[pending_num],
			 		BUF_SIZE-pending_num)) < 0) {
				if(errno == EIO && am_slave)
					exit(0);
				else if(errno != EWOULDBLOCK)

#if !defined(I18N)
					Panic("input: read returned unexpected error (%d)\n", errno);
#else
					Panic(OlGetMessage(screen->display,
							  NULL, 0,
				   			  OleNpanic,
							  OleTpanic_msg7,
				   			  OleCOlClientXtermMsgs,
				   			  OleMpanic7, NULL),
						errno);
#endif
			} else if(bcnt == 0)
#if !defined(I18N)
				Panic("input: read returned zero\n", 0);
#else
				Panic(OlGetMessage(screen->display, NULL, 0,
				   		   OleNpanic, OleTpanic_msg8,
				   		   OleCOlClientXtermMsgs,
				   		   OleMpanic8, NULL), 0);
#endif
			else {
				if (pending_num) {
				    bptr = buffer;
				    bcnt += pending_num;
				    pending_num = 0;
				}
#ifndef SVR4					/* no console logging */

#ifdef XTERM_COMPAT
				if(screen->scrollWidget && screen->scrollinput &&
#else
				if(screen->scrollWidget &&
#endif
				   screen->topline < 0)
					/* Scroll to bottom */
					WindowScroll(screen, 0);
				break;
#endif	/* SVR4 */
			}
		}
				/* get console input */
#ifdef SVR4
		if (console_has_input){
			console_has_input = (char) 0;
			if (bcnt > 0){
				cptr = (Char *) &bptr[bcnt];
				buf_size = BUF_SIZE - bcnt - 1;
			}
			else{
				cptr = bptr;
				buf_size = BUF_SIZE - 1;
			}
			if((console_cnt = console_input(screen->console,cptr,buf_size)) < 0)
					/* read from console log driver failed */
#if !defined(I18N)
				XtError("read from console log driver failed");
#else
				OlVaDisplayErrorMsg(screen->display, OleNread,
					OleTbadConsole,
					OleCOlClientXtermMsgs,
					OleMread_badConsole,
					NULL);

#endif
			else{
				bcnt += console_cnt;
			}
		}
				/* if input found, 
				 * scroll to bottom (if necessary)
				 * and break out of loop (back into parsin loop)
				 */

		if (bcnt > 0){
#ifdef XTERM_COMPAT
			if(screen->scrollWidget && screen->scrollinput &&
#else
			if(screen->scrollWidget &&
#endif
			screen->topline < 0)
					/* Scroll to bottom */
				WindowScroll(screen, 0);
			break;
		}
#endif

		if(screen->scroll_amt)
			FlushScroll(screen);
		if(screen->cursor_set && (screen->cursor_col != screen->cur_col
		 || screen->cursor_row != screen->cur_row)) {
			if(screen->cursor_state)
				HideCursor();
			ShowCursor();
		} else if(screen->cursor_set != screen->cursor_state) {
			if(screen->cursor_set)
				ShowCursor();
			else
				HideCursor();
		}
		
	if (waitingForTrackInfo) {
			trackTimeOut.tv_sec = TRACKTIMESEC;
			trackTimeOut.tv_usec = TRACKTIMEUSEC;
			select_mask = pty_mask;
			errno = 0;
#ifdef USL
#define XSELECT XSelect
#else
#define XSELECT select
#endif
			if ((i = XSELECT(max_plus1, &select_mask,
					(int *)NULL, (int *)NULL,
			 		&trackTimeOut)) < 0) {
			  if (errno != EINTR)
					SysError(ERROR_SELECT);
				continue;
			} else if (i == 0) {
				/* emacs just isn't replying, go on */
				waitingForTrackInfo = 0;
				Bell();
				select_mask = Select_mask;
			}
		} else if (QLength(screen->display))
		{
			select_mask = X_mask;
		}
		else {
			write_mask = ptymask();
			XFlush(screen->display);
			select_mask = Select_mask;
			if (eventMode != NORMAL)
			{
				select_mask = X_mask;
			}
			errno = 0;
#ifdef DEBUG
			printf("\n\nEnter Select: select_mask = %o", select_mask);
#endif /* DEBUG */
			if(XSELECT(max_plus1, &select_mask, &write_mask, 
				(int *)NULL, (struct timeval *) NULL) < 0){
				if (errno != EINTR)
					SysError(ERROR_SELECT);
				continue;
			} 
#undef XSELECT
#ifdef DEBUG
			printf("\n\nExit Select");
#endif /* DEBUG */
		}
		if (write_mask & ptymask())
			v_write(screen->respond, (char *) NULL, 0);	/* flush buffer */
		if(select_mask & X_mask) {
			if (bcnt <= 0) {
				bcnt = 0;
				bptr = buffer;
			}
			xevents();
			if (bcnt > 0)
				break;
		}
	}
 bcnt--;
 return(*bptr++);
}

/*
 * process a string of characters according to the character set indicated
 * by charset.  worry about end of line conditions (wraparound if selected).
 */
dotext(screen, flags, charset, buf, ptr, code_set)
register TScreen	*screen;
unsigned	flags;
char		charset;
Char	*buf;
Char	*ptr;
Char	code_set;
{
	register Char	*s;
	register int	str_len, text_len;
	register int	n, nn;
	register int	next_col;
	OlFontList	*fontl = term->primitive.font_list;
	Boolean		must_wrap = FALSE;
	int		code_width;

	switch (charset) {
	case 'A':	/* United Kingdom set			*/
		for (s=buf; s<ptr; ++s)
			if (*s == '#')
				*s = '\036';	/* UK pound sign*/
		break;

	case 'B':	/* ASCII set				*/
		break;

	case '0':	/* special graphics (line drawing)	*/
		for (s=buf; s<ptr; ++s)
			if (*s>=0x5f && *s<=0x7e)
				*s = *s == 0x5f ? 0x7f : *s - 0x5f;
		break;

	default:	/* any character sets we don't recognize*/
		return;
	}

	str_len = ptr - buf; 
	if (!fontl)
	    code_width = 1;
	else
	    code_width = fontl->cswidth[code_set];

	if (code_set > 1)
	    text_len = (str_len * code_width)/(code_width + 1);
	else
	    text_len = str_len;

	ptr = buf;
	while (str_len > 0) {
		n = screen->max_col-screen->cur_col+1;
		if (n <= code_width) {
			if (screen->do_wrap && (flags&WRAPAROUND)) {
				Index(screen, 1);
				screen->cur_col = 0;
				screen->do_wrap = 0;
				n = screen->max_col+1;
			} else {

			   /* there is no wraparound, and even a single char */
			   /* won't fit on the line: simply break in this case*/

			   if (n < code_width)
			        break;
			}
		}
		if (text_len < n)
			n = text_len;
		else if (code_width > 1 && n < text_len) {
			n -= n%code_width;
			must_wrap = TRUE;
		}
		next_col = screen->cur_col + n;

		/* calculate the length of the string corresponding to the */
		/* text segment to be drawn				   */

		if (code_set > 1)
		    nn = (n * (code_width + 1))/ code_width;
		else
		    nn = n;

		/* if writing on the second byte, erase the first one */

		if (Using_wide && (screen->buf[4 * screen->cur_row + 3]
			                      [screen->cur_col] & SECOND_BYTE)){
		    static char str[] = "  ";

		    screen->cur_col--;
		    dotext(screen, flags, charset, str, str+1, 0);
		}

		WriteText(screen, ptr, n, nn, flags);
		/*
		 * the call to WriteText updates screen->cur_col.
		 * If screen->cur_col != next_col, we must have
		 * hit the right margin, so set the do_wrap flag.
		 */
		if ((screen->cur_col < next_col) || must_wrap) {
		     screen->do_wrap = 1;
		     must_wrap = FALSE;
		}
		str_len  -= nn;
		text_len -= n;
		ptr      += nn;
	}
}
 
/*
 * write a string str of length len onto the screen at
 * the current cursor position.  update cursor position.
 */
WriteText(screen, str, text_len, str_len, flags)
register TScreen	*screen;
register Char	*str;
register int	text_len, str_len;
unsigned	flags;
{
	register int cx, cy;
	register unsigned fgs = flags;
	GC	currentGC;
/* SS-color */
	Pixel savefg, savebg;
/* SS-color-end */
/* SS-copy */
	extern Boolean	Have_hilite;
	extern int      TrackText();
	OlFontList    *fontl = term->primitive.font_list;
/* MORE: must use international staff here */
	Font save_font;

	if (Have_hilite) {
	    TrackText (0, 0, 0, 0);
	    Have_hilite = FALSE;
	}
/* SS-copy-end */
 
			/* if current row is visible on screen, draw it */
   if(screen->cur_row - screen->topline <= screen->max_row) {
	if(screen->cursor_state)
		HideCursor();

	/* select a GC to be used as current.  We could use GC[0], rather */
	/* then GC[codeset]: if there is a font list, then we'll draw	  */
	/* using OlDrawImageString(), which will replace the font in the  */
	/* GC anyway, and if the is no font list, we only have GC[0].	  */

	if (fgs & BOLD)
		if (fgs & INVERSE)
			currentGC = screen->reverseboldGC[0];
		else	currentGC = screen->normalboldGC[0];
	else  /* not bold */
		if (fgs & INVERSE)
			currentGC = screen->reverseGC[0];
		else	currentGC = screen->normalGC[0];

	if (fgs & INSERT)
		InsertChar(screen, text_len);
      if (!(AddToRefresh(screen))) {
		if(screen->scroll_amt)
			FlushScroll(screen);
	cx = CursorX(screen, screen->cur_col);
	cy = CursorY(screen, screen->cur_row)+screen->fnt_norm[0]->ascent;
/* SS-color  */
	if (Using_colors)
	{
	    savefg = GC_Foreground (currentGC);
	    savebg = GC_Background (currentGC);
	    if (fgs & INVERSE)
	    {
	       XSetForeground (screen->display, currentGC, textBG);
	       XSetBackground (screen->display, currentGC, textFG);
	    }
	    else
	    {
	       XSetForeground (screen->display, currentGC, textFG);
	       XSetBackground (screen->display, currentGC, textBG);
	    }
	} 
/* SS-color-end */

/* SS-ioctl */
	if (SCRTrans_table != NULL)
	    REMAP_OUTPUT_STRING(str, text_len);
/* SS-ioctl-end */

	if (!fontl)
 	    XDrawImageString(screen->display, TextWindow(screen),
			currentGC, cx, cy, (char *) str, str_len);
	else
 	    OlDrawImageString(screen->display, TextWindow(screen), fontl,
			currentGC, cx, cy, str, str_len);

	if((fgs & BOLD) && screen->enbolden) 
		if (currentGC == screen->normalGC[0] || 
		    currentGC == screen->reverseGC[0])
		    if (!fontl)
			XDrawString(screen->display, TextWindow(screen),
			      	currentGC, cx + 1, cy, (char *) str, str_len);
		    else
			OlDrawString(screen->display, TextWindow(screen), fontl,
			      	currentGC, cx + 1, cy, str, str_len);

	/* restore GC font, which may have been trampled by OlDraw*String */

	if (fontl)
	    if (fgs & BOLD)
	        XSetFont (screen->display, currentGC, screen->fnt_bold[0]->fid);
	    else
	        XSetFont (screen->display, currentGC, screen->fnt_norm[0]->fid);

	if(fgs & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), currentGC,
			cx, cy+1,
			cx + text_len * FontWidth(screen), cy+1);
	/*
	 * the following statements compile data to compute the average 
	 * number of characters written on each call to XText.  The data
	 * may be examined via the use of a "hidden" escape sequence.
	 */
	ctotal += text_len;
	++ntotal;
/* SS-color */
	if (Using_colors)
	{
	        XSetForeground (screen->display, currentGC, savefg); 
	        XSetBackground (screen->display, currentGC, savebg);
	}
/* SS-color-end */
      }
    }
		/* update screen buffer */
	ScreenWrite(screen, str, flags, textFG, textBG, str_len);
	CursorForward(screen, text_len);
}
 
/*
 * process ANSI modes set, reset
 */
modes(term, func)
XtermWidget	term;
int		(*func)();
{
	register int	i;
	register TScreen	*screen	= &term->screen;
	extern   void UpdateProperty();

	for (i=0; i<nparam; ++i) {
		switch (param[i]) {
		case 4:			/* IRM				*/
			(*func)(&term->flags, INSERT);
			break;

		case 20:		/* LNM				*/
			(*func)(&term->flags, LINEFEED);
			if(screen->property)
                           UpdateProperty(NEWLINEMAP_PROP,
					  ((term->flags & LINEFEED) ?
								TRUE : FALSE));
			break;
		}
	}
}

/*
 * process DEC private modes set, reset
 */
dpmodes(term, func)
XtermWidget	term;
int		(*func)();
{
	register TScreen	*screen	= &term->screen;
	register int	i, j;
	extern int bitset();
	extern   void UpdateProperty();

	for (i=0; i<nparam; ++i) {
		switch (param[i]) {
		case 1:			/* DECCKM			*/
			(*func)(&term->keyboard.flags, CURSOR_APL);
			if(screen->property)
			   UpdateProperty(APL_CURSOR_PROP,
					  ((term->keyboard.flags & CURSOR_APL) ?
					   TRUE : FALSE));
			break;
		case 3:			/* DECCOLM			*/
#ifndef SYSV
			if (screen->in_curses)
				break;

			if(screen->c132) {
				ClearScreen(screen);
				CursorSet(screen, 0, 0, term->flags);

				if((j = func == bitset ? 132 : 80) !=
				 ((term->flags & IN132COLUMNS) ? 132 : 80) ||
				 j != screen->max_col + 1) {
				        Dimension junk;
					(void) XtMakeResizeRequest (
					    (Widget) term, 
					    (unsigned) FontWidth(screen) * j
					        + 2*screen->border,
					    (unsigned) FontHeight(screen)
						* (screen->max_row + 1)
						+ 2 * screen->border,
					    &junk, &junk);

					XSync(screen->display, FALSE);

					if(QLength(screen->display) > 0)
						xevents();
				}

				(*func)(&term->flags, IN132COLUMNS);
			}
#endif
			break;

		case 4:			/* DECSCLM (slow scroll)	*/
			if (screen->in_curses)
				break;

			if (func == bitset) {
				screen->jumpscroll = 0;
				if (screen->scroll_amt)
					FlushScroll(screen);
			} else
				screen->jumpscroll = 1;
			(*func)(&term->flags, SMOOTHSCROLL);
			if(screen->property)
			   UpdateProperty(JUMPSCROLL_PROP, (Boolean) screen->jumpscroll);
			break;
		case 5:			/* DECSCNM			*/
			j = term->flags;
			(*func)(&term->flags, REVERSE_VIDEO);
			if ((term->flags ^ j) & REVERSE_VIDEO)
			{
			    ReverseVideo(term);
			    if (screen->property)
		    		UpdateProperty (REVERSE_VIDEO_PROP,
					        ((term->flags & REVERSE_VIDEO) ?
								TRUE : FALSE));
			}
			break;

		case 6:			/* DECOM			*/
			(*func)(&term->flags, ORIGIN);
			CursorSet(screen, 0, 0, term->flags);
			break;

		case 7:			/* DECAWM			*/
			(*func)(&term->flags, WRAPAROUND);
			if(screen->property)
			   UpdateProperty(WRAPAROUND_PROP,
					  ((term->flags & WRAPAROUND) ?
								TRUE : FALSE));
			break;
		case 8:			/* DECARM			*/
			j = term->flags;
			(*func)(&term->flags, AUTOREPEAT);
			if ((term->flags ^ j) & AUTOREPEAT)
				if(term->flags & AUTOREPEAT)
					XAutoRepeatOn(screen->display);
				else
					XAutoRepeatOff(screen->display);
			break;
		case 9:			/* MIT bogus sequence		*/
			if(func == bitset)
				screen->send_mouse_pos = 1;
			else
				screen->send_mouse_pos = 0;
			break;
#ifdef TEK
		case 38:		/* DECTEK			*/
			if((func == bitset) && !(screen->inhibit & I_TEK)) {

			    /* set focus to the Tek window		      */

			    if (screen->Tshow) {
	    			XWindowAttributes       win_attrs;
	    			extern void		end_vt_mode();
			
	    			XGetWindowAttributes (screen->display,
							TWindow(screen),
				  			&win_attrs);
			
	    			/* see comments in GINbutton0() in button.c */

	    			if (win_attrs.map_state != IsUnviewable)
	    			{
						OlSetInputFocus((Widget) tekWidget,
							RevertToParent, CurrentTime);
				    end_vt_mode (NULL);
	    			}
			    }

			    /* create tek window: re-use the VT menu callback */
			    /* routine which does the same		      */

			    else {
			    	extern void Tmenu_hide_vt();

			    	xmenu_hide_tek_window(NULL, NULL, NULL);
			    }
			}
			break;
#endif
		case 40:		/* 132 column mode		*/
			if (func == bitset)
                            screen->c132 = 1;
                        else
                            screen->c132 = 0;
			break;
		case 41:		/* curses hack			*/
			if (func == bitset)
                            screen->curses = 1;
                        else
                            screen->curses = 0;
			break;
		case 44:		/* margin bell			*/
			if (func == bitset)
                            screen->marginbell = 1;
                        else
                            screen->marginbell = 0;
			if(!screen->marginbell)
				screen->bellarmed = -1;
			if(screen->property)
			   UpdateProperty(MARGINBELL_PROP, (Boolean) screen->marginbell);
			break;
		case 45:		/* reverse wraparound	*/
			(*func)(&term->flags, REVERSEWRAP);
			if(screen->property)
			   UpdateProperty(REVERSEWRAP_PROP,
					  ((term->flags & REVERSEWRAP) ?
								TRUE : FALSE));
			break;
		case 46:		/* logging		*/
			if(func == bitset)
				StartLog(screen);
			else
				CloseLog(screen);
			if(screen->property)
			   UpdateProperty(LOGGIN_PROP, screen->logging);
			break;
		case 47:		/* alternate buffer		*/
			if(func == bitset)
				ToAlternate(screen);
			else
				FromAlternate(screen);
			break;
		case 1000:		/* xtem bogus sequence		*/
			if(func == bitset)
				screen->send_mouse_pos = 2;
			else
				screen->send_mouse_pos = 0;
			break;
		case 1001:		/* xtem sequence w/hilite tracking */
			if(func == bitset)
				screen->send_mouse_pos = 3;
			else
				screen->send_mouse_pos = 0;
			break;
		}
	}
}

/*
 * process xterm private modes save
 */
savemodes(term)
XtermWidget term;
{
	register TScreen	*screen	= &term->screen;
	register int i;

	for (i = 0; i < nparam; i++) {
		switch (param[i]) {
		case 1:			/* DECCKM			*/
			screen->save_modes[0] = term->keyboard.flags &
			 CURSOR_APL;
			break;
		case 3:			/* DECCOLM			*/
			if(screen->c132)
				screen->save_modes[1] = term->flags &
				 IN132COLUMNS;
			break;
		case 4:			/* DECSCLM (slow scroll)	*/
			screen->save_modes[2] = term->flags & SMOOTHSCROLL;
			break;
		case 5:			/* DECSCNM			*/
			screen->save_modes[3] = term->flags & REVERSE_VIDEO;
			break;
		case 6:			/* DECOM			*/
			screen->save_modes[4] = term->flags & ORIGIN;
			break;

		case 7:			/* DECAWM			*/
			screen->save_modes[5] = term->flags & WRAPAROUND;
			break;
		case 8:			/* DECARM			*/
			screen->save_modes[6] = term->flags & AUTOREPEAT;
			break;
		case 9:			/* mouse bogus sequence */
			screen->save_modes[7] = screen->send_mouse_pos;
			break;
		case 40:		/* 132 column mode		*/
			screen->save_modes[8] = screen->c132;
			break;
		case 41:		/* curses hack			*/
			screen->save_modes[9] = screen->curses;
			break;
		case 44:		/* margin bell			*/
			screen->save_modes[12] = screen->marginbell;
			break;
		case 45:		/* reverse wraparound	*/
			screen->save_modes[13] = term->flags & REVERSEWRAP;
			break;
		case 46:		/* logging		*/
			screen->save_modes[14] = screen->logging;
			break;
		case 47:		/* alternate buffer		*/
			screen->save_modes[15] = screen->alternate;
			break;
		case 1000:		/* mouse bogus sequence		*/
		case 1001:
			screen->save_modes[7] = screen->send_mouse_pos;
			break;
		}
	}
}

/*
 * process xterm private modes restore
 */
restoremodes(term)
XtermWidget term;
{
	register TScreen	*screen	= &term->screen;
	register int i;
	extern   void UpdateProperty();

	for (i = 0; i < nparam; i++) {
		switch (param[i]) {
		case 1:			/* DECCKM			*/
			term->keyboard.flags &= ~CURSOR_APL;
			term->keyboard.flags |= screen->save_modes[0] &
						CURSOR_APL;
			if(screen->property)
			   UpdateProperty(APL_CURSOR_PROP,
					  ((term->keyboard.flags & CURSOR_APL) ?
					   TRUE : FALSE));
			break;
		case 3:			/* DECCOLM			*/
#ifndef SYSV
			if(screen->c132) {
				ClearScreen(screen);
				CursorSet(screen, 0, 0, term->flags);
				if((j = (screen->save_modes[1] & IN132COLUMNS)
				 ? 132 : 80) != ((term->flags & IN132COLUMNS)
				 ? 132 : 80) || j != screen->max_col + 1) {
				        Dimension junk;
					(void) XtMakeResizeRequest (
					    (Widget) term,
					    (unsigned) FontWidth(screen) * j 
						+ 2*screen->border,
					    (unsigned) FontHeight(screen)
						* (screen->max_row + 1)
						+ 2*screen->border,
					    &junk, &junk);
					XSync(screen->display, FALSE);	/* synchronize */
					if(QLength(screen->display) > 0)
						xevents();
				}
				term->flags &= ~IN132COLUMNS;
				term->flags |= screen->save_modes[1] &
				 IN132COLUMNS;
			}
#endif
			break;
		case 4:			/* DECSCLM (slow scroll)	*/
			if (screen->save_modes[2] & SMOOTHSCROLL) {
				screen->jumpscroll = 0;
				if (screen->scroll_amt)
					FlushScroll(screen);
			} else
				screen->jumpscroll = 1;
			term->flags &= ~SMOOTHSCROLL;
			term->flags |= screen->save_modes[2] & SMOOTHSCROLL;
			if(screen->property)
                           UpdateProperty(JUMPSCROLL_PROP, (Boolean) screen->jumpscroll);
			break;
		case 5:			/* DECSCNM			*/
			if((screen->save_modes[3] ^ term->flags) &
			 				REVERSE_VIDEO)
			{
			    term->flags &= ~REVERSE_VIDEO;
			    term->flags |= screen->save_modes[3] &
				 				REVERSE_VIDEO;
			    ReverseVideo(term);
			    if (screen->property)
		    		UpdateProperty (REVERSE_VIDEO_PROP,
					        ((term->flags & REVERSE_VIDEO) ?
								TRUE : FALSE));
			}
			break;
		case 6:			/* DECOM			*/
			term->flags &= ~ORIGIN;
			term->flags |= screen->save_modes[4] & ORIGIN;
			CursorSet(screen, 0, 0, term->flags);
			break;

		case 7:			/* DECAWM			*/
			term->flags &= ~WRAPAROUND;
			term->flags |= screen->save_modes[5] & WRAPAROUND;
			if(screen->property)
                           UpdateProperty(WRAPAROUND_PROP,
					  ((term->flags & WRAPAROUND) ?
								TRUE : FALSE));
			break;
		case 8:			/* DECARM			*/
			if((screen->save_modes[6] ^ term->flags) & AUTOREPEAT) {
				term->flags &= ~REVERSE_VIDEO;
				term->flags |= screen->save_modes[6] &
				 REVERSE_VIDEO;
				if(term->flags & AUTOREPEAT)
					XAutoRepeatOn(screen->display);
				else
					XAutoRepeatOff(screen->display);
			}
			break;
		case 9:			/* MIT bogus sequence		*/
			screen->send_mouse_pos = screen->save_modes[7];
			break;
		case 40:		/* 132 column mode		*/
			screen->c132 = screen->save_modes[8];
			break;
		case 41:		/* curses hack			*/
			screen->curses = screen->save_modes[9];
			break;
		case 44:		/* margin bell			*/
			if(!(screen->marginbell = screen->save_modes[12]))
				screen->bellarmed = -1;
			if(screen->property)
                           UpdateProperty(MARGINBELL_PROP, (Boolean) screen->marginbell);
			break;
		case 45:		/* reverse wraparound	*/
			term->flags &= ~REVERSEWRAP;
			term->flags |= screen->save_modes[13] & REVERSEWRAP;
			if(screen->property)
                           UpdateProperty(REVERSEWRAP_PROP,
					  ((term->flags & REVERSEWRAP) ?
								TRUE : FALSE));
			break;
		case 46:		/* logging		*/
			if(screen->save_modes[14])
				StartLog(screen);
			else
				CloseLog(screen);
			if(screen->property)
                           UpdateProperty(LOGGIN_PROP, screen->logging);
			break;
		case 47:		/* alternate buffer		*/
			if(screen->save_modes[15])
				ToAlternate(screen);
			else
				FromAlternate(screen);
			break;
		case 1000:		/* mouse bogus sequence		*/
		case 1001:
			screen->send_mouse_pos = screen->save_modes[7];
			break;
		}
	}
}

/*
 * set a bit in a word given a pointer to the word and a mask.
 */
bitset(p, mask)
int	*p;
{
	*p |= mask;
}

/*
 * clear a bit in a word given a pointer to the word and a mask.
 */
bitclr(p, mask)
int	*p;
{
	*p &= ~mask;
}

unparseseq(ap, fd)
register ANSI	*ap;
{
	register int	c;
	register int	i;
	register int	inters;

	c = ap->a_type;
	if (c>=0x80 && c<=0x9F) {
		unparseputc(ESC, fd);
		c -= 0x40;
	}
	unparseputc(c, fd);
	c = ap->a_type;
	if (c==ESC || c==DCS || c==CSI || c==OSC || c==PM || c==APC) {
		if (ap->a_pintro != 0)
			unparseputc((char) ap->a_pintro, fd);
		for (i=0; i<ap->a_nparam; ++i) {
			if (i != 0)
				unparseputc(';', fd);
			unparseputn((unsigned int) ap->a_param[i], fd);
		}
		inters = ap->a_inters;
		for (i=3; i>=0; --i) {
			c = (inters >> (8*i)) & 0xff;
			if (c != 0)
				unparseputc(c, fd);
		}
		unparseputc((char) ap->a_final, fd);
	}
}

unparseputn(n, fd)
unsigned int	n;
int fd;
{
	unsigned int	q;

	q = n/10;
	if (q != 0)
		unparseputn(q, fd);
	unparseputc((char) ('0' + (n%10)), fd);
}

unparseputc(c, fd)
char c;
int fd;
{
	char	buf[2];
	register i = 1;
	extern XtermWidget term;
	register TScreen *screen = &term->screen;

	if((buf[0] = c) == '\r' && (term->flags & LINEFEED)) {
		buf[1] = '\n';
		i++;
	}

	/* if unparsing backspace, send 2 '\b' for 2-byte chars */

	if (Using_wide && c == '\b') {
	    int x = screen->cur_col;

	    if (x > 1 &&
		(screen->buf[4 * screen->cur_row + 3][x-1] & SECOND_BYTE)) {
	    	buf[1] = '\b';
	    	i++;
	    }
	}

	if (write(fd, buf, i) != i)
#if !defined(I18N)
		Panic("unparseputc: error writing character\n", 0);
#else
		Panic(OlGetMessage(screen->display, NULL, 0,
				   OleNpanic, OleTpanic_msg9,
				   OleCOlClientXtermMsgs,
				   OleMpanic9, NULL), 0);
#endif
}

ToAlternate(screen)
register TScreen *screen;
{
	extern ScrnBuf Allocate();
/* SS-color */
	register int rows = screen->max_row + 1;
	register int cols = screen->max_col + 1;
/* SS-color-end */

	if(screen->alternate)
		return;
	if (screen->altbuf == (ScrnBuf) NULL)
	    screen->altbuf = Allocate(rows, cols);
	SwitchBufs(screen);
	screen->alternate = TRUE;
}

FromAlternate(screen)
register TScreen *screen;
{
	if(!screen->alternate)
		return;
	screen->alternate = FALSE;
	SwitchBufs(screen);
}

SwitchBufs(screen)
register TScreen *screen;
{
	register int rows, top;
	char *save [4 * MAX_ROWS];
/* SS-color */
	register int i, j, cols;
	register Pixel *fptr, *bptr, *altfptr, *altbptr, csave;
/* SS-color-end */

	if(screen->cursor_state)
		HideCursor();
	rows = screen->max_row + 1;
	bcopy((char *)screen->buf, (char *)save, 4 * sizeof(char *) * rows);
	bcopy((char *)screen->altbuf, (char *)screen->buf, 4 * sizeof(char *) *
	 rows);
	bcopy((char *)save, (char *)screen->altbuf, 4 * sizeof(char *) * rows);

	if((top = -screen->topline) <= screen->max_row) {
		if(screen->scroll_amt)
			FlushScroll(screen);
		if(top == 0)
			XClearWindow(screen->display, TextWindow(screen));
		else
			XClearArea(
			    screen->display,
			    TextWindow(screen),
/* SS-scrollbar */
		    	    /* (int) screen->border + screen->scrollbar, */
		    	    (int) screen->border,
/* SS-scrollbar-end */
			    (int) top * FontHeight(screen) + screen->border,
			    (unsigned) Width(screen),
			    (unsigned) (screen->max_row - top + 1)
				* FontHeight(screen),
			    FALSE);
	}
/* SS-color */
	ColorScrnRefresh(screen, 0, 0, rows, screen->max_col + 1);
/* SS-color-end */
}


/*
 * Shows cursor at new cursor position in screen.
 */
ShowCursor()
{
	register TScreen *screen = &term->screen;
	register int x, y;
	Char flags, euc, code_set;
	Char c[2];
	GC	currentGC;
#ifdef I18N
	OlIc *ic = term->primitive.ic;
	Boolean multy_byte = FALSE;
	Boolean restore_GC = FALSE;
#endif

	if (eventMode != NORMAL) return;

	if (screen->cur_row - screen->topline > screen->max_row)
		return;

	screen->cursor_row = screen->cur_row;
	y = 4 * screen->cursor_row;
	x = screen->cursor_col = screen->cur_col;

	euc   = screen->buf[y + 3][x];

	/* if cursor is positioned on the second byte of a 2-byte 	*/
	/* charactes, move the cursor one position back			*/

	if (euc & SECOND_BYTE) {
	    screen->cursor_col--;
	    x--;
	}

	c[0]  = screen->buf[y][x];
	flags = screen->buf[y + 1][x];
	euc   = screen->buf[y + 3][x];
	code_set = Code_set (euc);

	if (euc & FIRST_BYTE) {
	    c[1] = screen->buf[y][x+1];
	    multy_byte = TRUE;
	}
	else if (c[0] == 0)
		 c[0] = ' ';

	if(screen->select) {
		if(flags & INVERSE) { /* is reverse video */
		    if (screen->cursorGC) {
			currentGC = screen->cursorGC;
		    } else {
			if (flags & BOLD) {
				currentGC = screen->normalboldGC[code_set];
			} else {
				currentGC = screen->normalGC[code_set];
			}
		    }
		} else { /* normal video */
		    if (screen->reversecursorGC) {
			currentGC = screen->reversecursorGC;
		    } else {
			if (flags & BOLD) {
				currentGC = screen->reverseboldGC[code_set];
			} else {
				currentGC = screen->reverseGC[code_set];
			}
		    }
		}
	} else { /* not selected */
/* MORE: bug fix taken from kterm
		if(flags & INVERSE) {
			currentGC = screen->reverseGC[code_set];
		} else {
			currentGC = screen->normalGC[code_set];
		}
*/
		if(flags & INVERSE) { /* is reverse video */
			if (screen->reversecursorGC) {
				currentGC = screen->reversecursorGC;
			} else {
				currentGC = screen->reverseGC[code_set];
			}
		} else { /* normal video */
			if (screen->cursorGC) {
				currentGC = screen->cursorGC;
			} else {
				currentGC = screen->normalGC[code_set];
			}
		}
	    
	}

	x = CursorX (screen, screen->cursor_col);
	y=CursorY(screen, screen->cur_row) + screen->fnt_norm[code_set]->ascent;

	/* if necessary, set GC font	*/

	if (code_set > 0 && (currentGC == screen->cursorGC ||
			     currentGC == screen->reversecursorGC)) {
	    XSetFont (screen->display, currentGC,
		      screen->fnt_norm[code_set]->fid);
	    restore_GC = TRUE;
	}

	if (multy_byte) {
	    XDrawImageString16(screen->display, TextWindow(screen), currentGC,
			     x, y, (XChar2b *) &c, 1);

	    if ((flags & BOLD) && screen->enbolden) /* no bold font */
		 XDrawString16(screen->display, TextWindow(screen), currentGC,
			     x + 1, y, (XChar2b *) &c, 1);
	    if(flags & UNDERLINE) 
		 XDrawLine(screen->display, TextWindow(screen), currentGC,
			x, y+1, x + 2*FontWidth(screen), y+1);
	}
	else {
	    XDrawImageString(screen->display, TextWindow(screen), currentGC,
			     x, y, (char *) &c, 1);

	    if ((flags & BOLD) && screen->enbolden) /* no bold font */
		 XDrawString(screen->display, TextWindow(screen), currentGC,
			     x + 1, y, (char *) &c, 1);
	    if(flags & UNDERLINE) 
		 XDrawLine(screen->display, TextWindow(screen), currentGC,
			x, y+1, x + FontWidth(screen), y+1);
	}

	if (restore_GC)
	    XSetFont (screen->display, currentGC, screen->fnt_norm[0]->fid);

	if (!screen->select) {
	    screen->box->x = x /*+ screen->fnt_norm->max_bounds.lbearing*/;
	    screen->box->y = y - screen->fnt_norm[code_set]->ascent;
	    if (multy_byte) {
		XDrawLines(screen->display, TextWindow(screen), currentGC,
			screen->w_box, NBOX, CoordModePrevious);
	    }
	    else {
		XDrawLines(screen->display, TextWindow(screen), currentGC,
			screen->box, NBOX, CoordModePrevious);
	    }
	}
	screen->cursor_state = ON;
#ifdef I18N
	if (ic){
		XPoint spot;
		OlIcValues icvalues[2];
			/*
			 *	Notify input method that cursor position has changed
			 */
		icvalues[0].attr_name = OlNspotLocation;
		icvalues[0].attr_value = &spot;
		icvalues[1].attr_name = NULL;
		icvalues[1].attr_value = NULL;
		spot.x = x;
		spot.y = y;
		OlSetIcValues(ic, icvalues);
	}
#endif
}

/*
 * hide cursor at previous cursor position in screen.
 */
HideCursor()
{
	register TScreen *screen = &term->screen;
	GC	currentGC;
	register int x, y;
	Char     flags, euc, code_set;
	Char c[2];
/* SS-color */
	Pixel fg, bg, savefg, savebg;
	Boolean multy_byte = FALSE;

	if (Using_colors) {
	    fg = PixelValue (FGColor(*(screen->buf[4*screen->cursor_row+2]
                  	  	     + screen->cursor_col)));
	    bg = PixelValue (BGColor(*(screen->buf[4*screen->cursor_row+2]
                  	  	     + screen->cursor_col)));
	}
	else {
	    fg = screen->foreground;
	    bg = screen->background;
	}
/* SS-color-end */

	if(screen->cursor_row - screen->topline > screen->max_row)
		return;

	y = 4 * screen->cursor_row;
	x = screen->cursor_col;

	euc   = screen->buf[y + 3][x];

	/* if cursor is positioned on the second byte of a 2-byte 	*/
	/* charactes, move the cursor one position back			*/

	if (euc & SECOND_BYTE)
	    --x;

	c[0]  = screen->buf[y][x];
	flags = screen->buf[y + 1][x];
	euc   = screen->buf[y + 3][x];
	code_set = Code_set (euc);

	if (euc & FIRST_BYTE) {
	    c[1] = screen->buf[y][x+1];
	    multy_byte = TRUE;
	}
	else if (c[0] == 0)
		 c[0] = ' ';

	if(flags & INVERSE) {
		if(flags & BOLD) {
			currentGC = screen->reverseboldGC[code_set];
		} else {
			currentGC = screen->reverseGC[code_set];
		}
	} else {
		if(flags & BOLD) {
			currentGC = screen->normalboldGC[code_set];
		} else {
			currentGC = screen->normalGC[code_set];
		}
	}

	x = CursorX (screen, screen->cursor_col);
	y = CursorY(screen, screen->cursor_row) + screen->fnt_norm[code_set]->ascent;

/* SS-color */
	if (flags & USE_FG_COLOR)
	{
	    savefg = GC_Foreground (currentGC);
	    if(flags & INVERSE)
	    {
	        XSetBackground (screen->display, currentGC, fg); 
	    }
	    else
	    {
	        XSetForeground (screen->display, currentGC, fg); 
	    }
	}
	if (flags & USE_BG_COLOR)
	{
	    savebg = GC_Background (currentGC);
	    if(flags & INVERSE)
	    {
	        XSetForeground (screen->display, currentGC, bg);
	    }
	    else
	    {
	        XSetBackground (screen->display, currentGC, bg);
	    }
	}
/* SS-color-end */
	if (multy_byte) {
	    XDrawImageString16(screen->display, TextWindow(screen), currentGC,
		    x, y, (XChar2b *) c, 1);
	    if((flags & BOLD) && screen->enbolden)
		XDrawString16(screen->display, TextWindow(screen), currentGC,
			x + 1, y, (XChar2b *) c, 1);
	    if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), currentGC,
			x, y+1, x + 2*FontWidth(screen), y+1);
	}
	else {
	    XDrawImageString(screen->display, TextWindow(screen), currentGC,
		    x, y, (char *) c, 1);
	    if((flags & BOLD) && screen->enbolden)
		XDrawString(screen->display, TextWindow(screen), currentGC,
			x + 1, y, (char *) c, 1);
	    if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), currentGC,
			x, y+1, x + FontWidth(screen), y+1);
	}
	screen->cursor_state = OFF;
/* SS-color */
	if (flags & USE_FG_COLOR)
		XSetForeground (screen->display, currentGC, savefg); 
	if (flags & USE_BG_COLOR)
		XSetBackground (screen->display, currentGC, savebg);
/* SS-color-end */
}


int GetBytesAvailable (fd)
    int fd;
{
#ifdef FIONREAD
    static long arg;
    ioctl (fd, FIONREAD, (char *) &arg);
    return (int) arg;
#else
    struct pollfd pollfds[1];

    pollfds[0].fd = fd;
    pollfds[0].events = POLLIN;
    return poll (pollfds, 1, 0);
#endif
}
