/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:setalarm.h	1.2"
#endif

/*
 * setalarm.h
 *
 */

#ifndef _setalarm_h
#define _setalarm_h

extern void AlarmCB(Widget, XtPointer, XtPointer);

extern PopupGizmo AlarmPrompt;

#endif /* _setalarm_h */
