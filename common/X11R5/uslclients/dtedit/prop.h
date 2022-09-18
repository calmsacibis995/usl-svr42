/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:prop.h	1.2"
#endif

/*
 * prop.h
 *
 */

#ifndef _prop_h
#define _prop_h

#define WRAP    "editwrap"
#define NUMB    "editnumb"

typedef enum { WrapWord, WrapChar, WrapNone } WrapMenuItemIndex;
typedef enum { NumbNone, NumbLeft, NumbRight } NumbMenuItemIndex;

extern void PropCB(Widget w, XtPointer client_data, XtPointer call_data);
extern int  DecodeWrapMode(int mode);

extern PopupGizmo PropertiesPrompt;
 
#endif
