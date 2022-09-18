/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:input.c	1.2.1.25"
#endif
/*
 input.c (C source file)
	Acc: 601052282 Tue Jan 17 09:58:02 1989
	Mod: 601054060 Tue Jan 17 10:27:40 1989
	Sta: 601054060 Tue Jan 17 10:27:40 1989
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

#ifndef lint
static char *rcsid_input_c = "$Header: input.c,v 1.1 88/02/10 13:08:06 jim Exp $";
#endif	/* lint */

#include <X11/copyright.h>

#ifndef lint
static char rcs_id[] = "$Header: input.c,v 1.1 88/02/10 13:08:06 jim Exp $";
#endif	/* lint */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "ptyx.h"
/* SS-ioctl */
#include "xterm.h"
#include "data.h"
#include "xterm_ioctl.h"
/* SS-ioctl-end */

#include "Strings.h"
#include "messages.h"

static XComposeStatus compose_status = {NULL, 0};
static char *kypd_num = " XXXXXXXX\tXXX\rXXXxxxxXXXXXXXXXXXXXXXXXXXXX*+,-.\\0123456789XXX=";
static char *kypd_apl = " ABCDEFGHIJKLMNOPQRSTUVWXYZ??????abcdefghijklmnopqrstuvwxyzXXX";
static char *cur = "DACB";

Input (keyboard, screen, ve)
register TKeyboard	*keyboard;
register TScreen	*screen;
OlVirtualEvent ve;
{

#define STRBUFSIZE 100

	register XKeyEvent *key_event = &ve->xevent->xkey;
	KeySym 	key_sym = ve->keysym;
	int		nbytes = ve->length; 
	int 		i = nbytes + 1;
	char 		strbuf[STRBUFSIZE];
	register char 	*string = strbuf;
	register 	int col, key = FALSE;
	int	pty	= screen->respond;
	ANSI	reply;

/* SS-ioctl */
	extern void i386LookupSymbol();
/* SS-ioctl-end */

		/*
		 * make modifiable copy of the returned string 
		 */ 
	while (i--)
		strbuf[i] = ve->buffer[i];

	i = 0;
	switch(key_sym) {
		case 0xff50:	/* HOME */
			if (write(pty, "\033[H", 3) != 3)
			     i = 1;
			else i = 2;
			break;

		case 0xff55:	/* PAGE UP */
			if (write(pty, "\033[V", 3) != 3)
			     i = 1;
			else i = 2;
			break;

		case 0xff56:	/* PAGE DOWN */
			if (write(pty, "\033[U", 3) != 3)
			     i = 1;
			else i = 2;
			break;

		case 0xff57:	/* END */
			if (write(pty, "\033[Y", 3) != 3)
			     i = 1;
			else i = 2;
			break;
	}

	if (i != 0) {
	    if (i == 1)
		Panic(
#if !defined(I18N)
			"Error: Input() Write failed!\n",
#else
			OlGetMessage(screen->display, NULL, 0,
				     OleNpanic, OleTpanic_msg10,
				     OleCOlClientXtermMsgs,
				     OleMpanic10, NULL),	
#endif
					0);
	    else
		return;
	}

/* SS-ioctl */
    	if (KBTranslation && nbytes == 1)
	{
	     i386LookupSymbol (key_event, strbuf);
	}
/* SS-ioctl-end */

	string = &strbuf[0];
	reply.a_pintro = 0;
	reply.a_final = 0;
	reply.a_nparam = 0;
	reply.a_inters = 0;

	if (IsPFKey(key_sym)) {
		reply.a_type = SS3;
		unparseseq(&reply, pty);
		unparseputc((char)(key_sym-XK_KP_F1+'P'), pty);
		key = TRUE;
	} else if (IsKeypadKey(key_sym)) {
	  	if (keyboard->flags & KYPD_APL)	{
			reply.a_type   = SS3;
			unparseseq(&reply, pty);
			unparseputc(kypd_apl[key_sym-XK_KP_Space], pty);
		} else
			unparseputc(kypd_num[key_sym-XK_KP_Space], pty);
		key = TRUE;
        } else if (IsCursorKey(key_sym)) {
       		if (keyboard->flags & CURSOR_APL) {
			reply.a_type = SS3;
			unparseseq(&reply, pty);
			unparseputc(cur[key_sym-XK_Left], pty);
		} else {
			reply.a_type = CSI;
			reply.a_final = cur[key_sym-XK_Left];
			unparseseq(&reply, pty);
		}
		key = TRUE;
/* SS-ioctl : don't treat function keys as special case */
	 /* } else if (IsFunctionKey(key_sym)) { */
	 } else if (IsMiscFunctionKey(key_sym)) {
/* SS-ioctl-end */
		reply.a_type = CSI;
		reply.a_nparam = 1;
		reply.a_param[0] = funcvalue(key_sym);
		reply.a_final = '~';
		if (reply.a_param[0] > 0)
			unparseseq(&reply, pty);
		key = TRUE;
	} else if (nbytes > 0) {
#ifdef TEK
		if(screen->TekGIN) {
			TekEnqMouse(*string++);
			TekGINoff();
			nbytes--;
		}
#endif

/* SS-alt: Mod1Mask is used for Alt key.  If Xenix translation is not on */
/*         transmit Esc N before the character.  Set This_is_alt flag    */
/*	   in order not to work correct when getting to SS2 case	 */

		if((nbytes == 1) && (key_event->state & Mod1Mask) &&
		   (string[0] >= ' ' && string[0] < '\177'))
		{
			This_is_alt = TRUE;
			unparseputc(033, pty);
			if (string[0] == ' ')
			{   unparseputc('Z', pty);
			    unparseputc('U', pty);
			    goto _over;
			}
			unparseputc('N', pty);
		}
/* SS-alt-end */
		while (nbytes-- > 0)
			unparseputc(*string++, pty);
_over:
		key = TRUE;
/* SS-ioctl */
	 	if (IsFunctionKey(key_sym))
		    This_is_fk = TRUE;
/* SS-ioctl-end */

/* SS-new: this is for dealing with shifted function keys	*/

	} else if (nbytes == 0 && IsFunctionKey(key_sym)) {
		nbytes = 3;
		string[0] = 033;
		string[1] = 'O';

		/* we can get in here under two conditions: shift key is */
		/* pressed, in which case key_sym=0xffca, or either Caps */
		/* Lock or Num Lock is pressed, in which case key_sym=   */
		/* 0xffbe.  In the first case we want the lower case     */
		/* chars. in the string[2], otherwise the upper case.    */
		/* By the way, this code would not be needed if XLookup- */
		/* Sring() would return a string corresponding to the    */
		/* modified FK, just as console does.			 */
			
		if (key_sym >= 0xffca) {
		    string[2] = 'p' + key_sym - 0xffca;
		    if (string[2] > 'z')
		        string[2] = 'a';
		} else {
		    string[2] = 'P' + key_sym - 0xffbe;
		    if (string[2] > 'Z')
		        string[2] = 'A';
		}
		while (nbytes-- > 0)
			unparseputc(*string++, pty);
	}
#ifdef TEK
	if(key && !screen->TekEmu) {
#else
	if(key) {
#endif /* TEK */
#ifdef XTERM_COMPAT
		if (screen->scrollkey && screen->topline != 0)
#else
		if (screen->topline != 0)
#endif
			WindowScroll(screen, 0);
		if(screen->marginbell) {
			col = screen->max_col - screen->nmarginbell;
			if (screen->bellarmed >= 0) {
			    if (screen->bellarmed == screen->cur_row) {
				if ((screen->cur_col >= col) ||
				    (strbuf[0] == '\t' &&
				     screen->cur_col < col &&
				     TabNext(term->tabs, screen->cur_col) >=
									col)) {
						Bell();
						screen->bellarmed = -1;
				}
			    } else
				screen->bellarmed = screen->cur_col <
					 col ? screen->cur_row : -1;
			} else if(screen->cur_col < col)
				screen->bellarmed = screen->cur_row;
		}
	}
#ifdef TEK
#ifdef ENABLE_PRINT
        if (key_sym == XK_F2) TekPrint();
#endif
#endif /* TEK */
	return;
}


#ifdef TEK
void
AdjustAfterInput (screen)
register TScreen *screen;
{
#ifdef XTERM_COMPAT
	if(screen->scrollkey && screen->topline != 0)
		WindowScroll(screen, 0);
#endif
	if(screen->marginbell) {
		int col = screen->max_col - screen->nmarginbell;
		if(screen->bellarmed >= 0) {
			if(screen->bellarmed == screen->cur_row) {
				if(screen->cur_col >= col) {
					if(screen->cur_col == col)
						Bell();
					screen->bellarmed = -1;
				}
			} else
				screen->bellarmed = screen->cur_col <
				 col ? screen->cur_row : -1;
		} else if(screen->cur_col < col)
			screen->bellarmed = screen->cur_row;
	}
}


StringInput (screen, string)
register TScreen	*screen;
register char *string;
{
	int	pty	= screen->respond;
	int	nbytes;

	nbytes = strlen(string);
	if(nbytes && screen->TekGIN) {
		TekEnqMouse(*string++);
		TekGINoff();
		nbytes--;
	}
	while (nbytes-- > 0)
		unparseputc(*string++, pty);
	if(!screen->TekEmu)
	        AdjustAfterInput(screen);
}
#endif /* TEK */


funcvalue(key_sym)
{
	switch (key_sym) {
		case XK_Help:	return(28);
		case XK_Menu:	return(29);
		case XK_Find :	return(1);
		case XK_Insert:	return(2);
		case XK_Delete:	return(3);
		case XK_Select:	return(4);
		case XK_Prior:	return(5);
		case XK_Next:	return(6);
		default:	return(-1);
	}
}
