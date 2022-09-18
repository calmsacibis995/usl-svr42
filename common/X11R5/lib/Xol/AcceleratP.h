/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)mouseless:AcceleratP.h	1.4"
#endif

#if	!defined(_ACCELERATP_H)
#define _ACCELERATP_H

#include "Xol/Accelerate.h"

/*
 * New types:
 */

typedef struct KeyEvent {
	Widget			w;
	XtPointer		data;
	Boolean			is_mnemonic;
	Boolean			grabbed;
	Modifiers		modifiers;
	KeySym			keysym;
	OlVirtualName		name;
}			KeyEvent;

typedef struct OlAcceleratorList {
	KeyEvent *		base;
	Cardinal		nel;
	Boolean			do_grabs;
}			OlAcceleratorList;

/*
 * Macro definitions:
 */

/*
 * LBRA:	left bracket, the character that begins a string KeySym
 * RBRA:	right bracket, the character that ends a string KeySym
 * S_SEPS:	the character(s) that separate modifiers in a string
 * PLUS:	the separator between modifier names in accelerator text
 */
#define LBRA	'<'	/* I18N */
#define RBRA	'>'	/* I18N */
#define S_SEPS	" "	/* I18N */
#define PLUS	"+"	/* I18N */
#endif
