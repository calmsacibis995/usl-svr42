/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/WM.h	1.1"
#endif

#if	!defined(_WM_H)
#define _WM_H

#include "X11/Shell.h"	/* for XtNtitle */

#include "Xol/Manager.h"

#define WM_PART_NONE		2001
#define WM_PART_HEADER		2002
#define WM_PART_BACKGROUND	2003

extern WidgetClass		wmWidgetClass;

typedef struct _WMClassRec *	WMWidgetClass;
typedef struct _WMRec *		WMWidget;

#endif /* _WM_H */
