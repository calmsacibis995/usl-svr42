/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/misprite.h	1.2"

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

/* $XConsortium: misprite.h,v 5.2 89/08/30 19:24:05 keith Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology
*/

/*
 * misprite.h
 *
 * software-sprite/sprite drawing interface spec
 *
 * mi versions of these routines exist.
 */

/*
 * ** SI **
 *
 * Parts of this interface have changed to better fit the needs of the SI.  
 */
typedef struct {
    Bool	(*RealizeCursor)();	/* pScreen, pCursor */
    Bool	(*UnrealizeCursor)();	/* pScreen, pCursor */
    Bool	(*DisplayCursor)();	/* pScreen, pCursor */
    Bool	(*UndisplayCursor)();	/* pScreen */
    Bool	(*MoveCursor)();	/* pScreen, pCursor, x, y */
} miSpriteCursorFuncRec, *miSpriteCursorFuncPtr;

