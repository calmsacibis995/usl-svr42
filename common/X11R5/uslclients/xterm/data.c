/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:data.c	1.2.1.20"
#endif
/*
 data.c (C source file)
	Acc: 601052232 Tue Jan 17 09:57:12 1989
	Mod: 601054032 Tue Jan 17 10:27:12 1989
	Sta: 601054032 Tue Jan 17 10:27:12 1989
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

#include <X11/copyright.h>

#include <setjmp.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include "ptyx.h"
#include "data.h"
#include	"xterm_ioctl.h"

#ifndef lint
static char rcs_is[] = "$Header: data.c,v 1.2 88/02/16 14:59:48 jim Exp $";
#endif	/* lint */

int bcnt = 0;
Char buffer[BUF_SIZE];
Char *bptr = buffer;
jmp_buf VTend;
XPoint VTbox[NBOX] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};

XPoint VTwbox[NBOX] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};

#ifdef DEBUG
int debug = 0; 		/* true causes error messages to be displayed */
#endif	/* DEBUG */
Widget toplevel;
XtermWidget term;		/* master data structure for client */
/* FLH dynamic */
RubberTileWidget container;	/* container widget for xterm & scrollbar */
/* FLH dynamic */
#ifdef I18N
FooterPanelWidget	footerpane;
#endif
char *xterm_name;	/* argv[0] */
char *title;
int am_slave = 0;	/* set to 1 if running as a slave process */
int L_flag;
int max_plus1;
int pty_mask;
int Select_mask;
int X_mask;
char *ptydev = PTYDEV;
char *ttydev = TTYDEV;
char log_def_name[] = "XtermLog.XXXXX";

int waitingForTrackInfo = 0;
EventMode eventMode = NORMAL;

GC visualBellGC;

int VTgcFontMask = GCFont;

int MenugcFontMask = GCFont;

/* SS-cut */
int Topline = 0;
/* SS-cut-end */

#ifdef TEK
XPoint T_boxlarge[NBOX] = {
	{0, 0},
	{8, 0},
	{0, 14},
	{-8, 0},
	{0, -14},
};
XPoint T_box2[NBOX] = {
	{0, 0},
	{7, 0},
	{0, 12},
	{-7, 0},
	{0, -12},
};
XPoint T_box3[NBOX] = {
	{0, 0},
	{5, 0},
	{0, 12},
	{-5, 0},
	{0, -12},
};
XPoint T_boxsmall[NBOX] = {
	{0, 0},
	{5, 0},
	{0, 9},
	{-5, 0},
	{0, -9},
};

int Tbcnt = 0;
Char *Tbuffer;
Char *Tbptr;

Char *Tpushb;
Char *Tpushback;
int Ttoggled = 0;
jmp_buf Tekend;
TekLink *TekRefresh;
T_fontsize Tfontsize[TEKNUMFONTS] = {
        {9, 15},        /* large */
        {8, 13},        /* #2 */
        {6, 13},        /* #3 */
        {6, 10},        /* small */
};
int T_lastx = -1;
int T_lasty = -1;
int TEKgcFontMask = GCFont;
TekWidget tekWidget;
#endif /* TEK */

/* SS-color */
Pixel	Pixels[8];
Boolean	Using_colors = FALSE;
Boolean	Using_wide   = FALSE;
/* SS-color-end */
/* SS-fk */
Boolean		This_is_fk  = FALSE;
Boolean		This_is_alt = FALSE;
/* SS-fk-end */

/* SS-monocrome */
Boolean		Mono_change = FALSE;
/* SS-monocrome-end */

/* SS-mouse */
unsigned long Get_Mouse_Events = 0;
/* SS-mouse-end */

/* SS: sometime xterm starts up unfocused.  This will force the very */
/*     first button press in a window to set focus to the window.    */
Boolean		Xterm_startup = TRUE;

/* SS-ioctl */
Trans_table	*KBTrans_table = (Trans_table *) NULL;
char		*FKTrans_table[NUMBER_OF_FK];
unsigned char	*SCRTrans_table = NULL;
Boolean		KBTranslation = FALSE;
/* SS-ioctl-end */

short focus_switched = 0;	/* indicates window last given FOCUS IN */
									/* used to determine if the emulation mode */
									/* should be switched to match the window */
									/* with focus.  The value is set in each  */
									/* widget's AcceptFocus proc					*/

	
short inside_TekExpose = 0;	/* flag used to indicate that the Tek Expose 
	 									 * routine is on the call stack. This
	 									 * will prevent a longjmp() into oblivion 
										 * in Tinput() (Tekproc.c)
	 									 */
