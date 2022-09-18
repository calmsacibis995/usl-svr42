/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:data.h	1.2.1.19"
#endif
/*
 data.h (C hdr file)
	Acc: 601052241 Tue Jan 17 09:57:21 1989
	Mod: 601054033 Tue Jan 17 10:27:13 1989
	Sta: 601054033 Tue Jan 17 10:27:13 1989
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

#include <X11/Intrinsic.h>
#include <setjmp.h>

extern XPoint VTbox[];
extern XPoint VTwbox[];
extern Char *bptr;
extern char log_def_name[];
extern char *ptydev;
extern char *ttydev;
extern char *xterm_name;
extern char *title;
extern Char buffer[];
extern int L_flag;
extern int Select_mask;
extern int X_mask;
extern int am_slave;
extern int bcnt;
#ifdef DEBUG
extern int debug;
#endif	/* DEBUG */
extern int errno;
extern int max_plus1;
extern int pty_mask;
extern int switchfb[];
extern jmp_buf VTend;

extern int waitingForTrackInfo;

extern EventMode eventMode;

extern GC visualBellGC;

extern int VTgcFontMask;
extern int MenugcFontMask;

extern Widget toplevel;					/* toplevel shell from OlInitialize */
extern XtermWidget term;
/* FLH dynamic */
extern RubberTileWidget container;   /* container for xterm & scrollbar */
/* FLH dynamic */

#ifdef I18N
extern FooterPanelWidget footerpane;
#endif


/* SS-cut */
extern	int Topline;
/* SS-cut-end */

#ifdef TEK
extern TekLink *TekRefresh;
extern XPoint T_box2[];
extern XPoint T_box3[];
extern XPoint T_boxlarge[];
extern XPoint T_boxsmall[];
extern T_fontsize Tfontsize[];
extern Char *Tbptr;
extern Char *Tbuffer;
extern Char *Tpushb;
extern Char *Tpushback;
extern int T_lastx;
extern int T_lasty;
extern int Tbcnt;
extern int Ttoggled;
extern jmp_buf Tekend;
extern int TEKgcFontMask;
extern TekWidget tekWidget;
#endif /* TEK */

extern	Boolean		This_is_fk, This_is_alt;
extern	Boolean	 	Using_colors, Using_wide; 
extern	Pixel		Pixels[8];
#define PixelValue(n)   Pixels[n]

extern  unsigned long   Get_Mouse_Events;

#define VT_MODE 1		/* vt given last FOCUS IN */
#define TEK_MODE 2	/* tek given last FOCUS IN */ 

extern short focus_switched;
extern short inside_TekExpose;
