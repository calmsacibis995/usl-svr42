/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)textedit:TextEPos.h	1.2"
#endif

/*
 * TextPos.h
 *
 */

#ifndef _TextPos_h
#define _TextPos_h

extern void       _SetDisplayLocation();
extern void       _SetTextXOffset();
extern void       _CalculateCursorRowAndXOffset();
extern int        _MoveDisplayPosition();
extern void       _MoveDisplay();
extern void       _MoveDisplayLaterally();
extern void       _MoveCursorPosition();
extern void       _MoveCursorPositionGlyph();
extern Boolean    _MoveSelection();
extern int        _TextEditOwnPrimary();

extern int        _PositionFromXY();
extern XRectangle _RectFromPositions();

#endif
