/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xterm:xterm_menu.h	1.10"

/* definitions for vt pop-up menu	*/

#define	VT_EDIT			0
#define	VT_REDRAW		(VT_EDIT + 1)
#define VT_SOFT_RESET		(VT_REDRAW + 1)
#define VT_FULL_RESET		(VT_SOFT_RESET + 1)
#define VT_PROPERTIES		(VT_FULL_RESET + 1)
#ifdef TEK
#define VT_HIDE_TEK_WIN  	(VT_PROPERTIES + 1)
#define VT_INTERRUPT		(VT_HIDE_TEK_WIN + 1)
#else /* TEK */
#define VT_INTERRUPT		(VT_PROPERTIES + 1)
#endif /* TEK */
#define VT_HANGUP		(VT_INTERRUPT + 1)
#define VT_TERMINATE		(VT_HANGUP + 1)
#define VT_KILL			(VT_TERMINATE + 1)

/* definitions for vt property window	*/

#define	VISUALBELL_PROP		0
#define	LOGGIN_PROP		(VISUALBELL_PROP + 1)
#define	JUMPSCROLL_PROP		(LOGGIN_PROP + 1)
#define REVERSE_VIDEO_PROP	(JUMPSCROLL_PROP + 1)
#define	WRAPAROUND_PROP		(REVERSE_VIDEO_PROP + 1)
#define REVERSEWRAP_PROP	(WRAPAROUND_PROP + 1)
#define NEWLINEMAP_PROP		(REVERSEWRAP_PROP + 1)
#define APL_CURSOR_PROP		(NEWLINEMAP_PROP + 1)
#define APL_KEYPAD_PROP		(APL_CURSOR_PROP + 1)
#define SCROLLBAR_PROP		(APL_KEYPAD_PROP + 1)
#define MARGINBELL_PROP		(SCROLLBAR_PROP + 1)

#ifdef SECURE_KEYBOARD
#define SECURE_KBD_PROP		(MARGINBELL_PROP + 1)
#define RESIZE_PROP		(SECURE_KBD_PROP +1)
#else 
#define RESIZE_PROP		(MARGINBELL_PROP +1)
#endif

#if defined(TEK) && defined(XTERM_COMPAT)
#define	AUTOREPEAT_PROP		(TEK_WINDOW_PROP + 1)
#define INPUT_SCROLL_PROP	(AUTO_REPEAT_PROP + 1)
#define KEY_SCROLL_PROP		(INPUT_SCROLL_PROP + 1)
#define NUM_PROP		(KEY_SCROLL_PROP + 1)
#else
#define NUM_PROP		(RESIZE_PROP + 1)
#endif /* TEK && XTERM_COMPAT */


#ifdef TEK
/* definitions for Tek pop-up menu	*/

#define TEK_PAGE		0
#define TEK_RESET		(TEK_PAGE + 1)
#define TEK_COPY		(TEK_RESET + 1)
#define	TEK_REDRAW		(TEK_COPY + 1)
#define TEK_PROPERTIES		(TEK_REDRAW + 1)
#define TEK_HIDE_VT_WIN		(TEK_PROPERTIES + 1)
#define TEK_INTERRUPT		(TEK_HIDE_VT_WIN + 1)
#define TEK_HANGUP		(TEK_INTERRUPT + 1)
#define TEK_TERMINATE		(TEK_HANGUP + 1)
#define TEK_KILL		(TEK_TERMINATE + 1)

#endif /* TEK */
