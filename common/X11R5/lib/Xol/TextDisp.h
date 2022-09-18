/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)textedit:TextDisp.h	1.2"
#endif

/*
 * TextDisp.h
 *
 */

#ifndef _TextDisp_h
#define _TextDisp_h

extern void     _DisplayText();
extern void     _ChangeTextCursor();
extern int      _DrawTextCursor();
extern void     _TurnTextCursorOff();

#endif
