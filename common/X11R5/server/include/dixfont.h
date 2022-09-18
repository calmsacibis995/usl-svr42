/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/dixfont.h	1.4"

/* $XConsortium: dixfont.h,v 1.7 91/05/11 11:45:47 rws Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
******************************************************************/

#ifndef DIXFONT_H
#define DIXFONT_H 1

#include    <font.h>

#define NullDIXFontProp ((DIXFontPropPtr)0)

typedef struct _DIXFontProp *DIXFontPropPtr;

#ifndef R4_FONT_STRUCTURES

extern void UseFPE();
extern void FreeFPE();
extern void QueueFontWakeup();
extern void RemoveFontWakeup();
extern int FontWakeup();

#else

typedef struct _R4Font *FontPtr;

/*
 * this type is for people who want to talk about the font encoding
 */

typedef enum {
    Linear8Bit, TwoD8Bit, Linear16Bit, TwoD16Bit
}           FontEncoding;

#endif

#endif				/* DIXFONT_H */
