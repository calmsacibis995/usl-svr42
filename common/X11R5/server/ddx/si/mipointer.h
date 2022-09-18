/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipointer.h	1.2"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/* $XConsortium: mipointer.h,v 5.2 89/06/21 11:16:15 rws Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology
*/

/*
 * ** SI **
 *
 * The meaning of DisplayCursor() has been changed.  It now is used
 * only to turn on a given cursor.  If a cursor is to have it's position
 * set, a new function, MoveCursor() is used.  
 */
typedef struct {
    Bool	(*RealizeCursor)();	/* pScreen, pCursor */
    Bool	(*UnrealizeCursor)();	/* pScreen, pCursor */
    void	(*DisplayCursor)();	/* pScreen, pCursor */
    void	(*UndisplayCursor)();	/* pScreen, pCursor */
    void	(*MoveCursor)();	/* pScreen, pCursor, x, y */
} miPointerSpriteFuncRec, *miPointerSpriteFuncPtr;

typedef struct {
    long	(*EventTime)();		/* pScreen */
    Bool	(*CursorOffScreen)();	/* pScreen, x, y */
    void	(*CrossScreen)();	/* pScreen, entering */
    void	(*QueueEvent)();	/* pxE, pPointer, pScreen */
} miPointerCursorFuncRec, *miPointerCursorFuncPtr;

extern void miPointerPosition (),	miRegisterPointerDevice();
extern void miPointerDeltaCursor (),	miPointerMoveCursor();
extern Bool miPointerInitialize ();

