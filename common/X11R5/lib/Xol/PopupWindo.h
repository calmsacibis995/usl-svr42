/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)popupwindo:PopupWindo.h	1.4"
#endif

#ifndef _PopupWindow_h
#define _PopupWindow_h

#include <X11/Shell.h>

typedef struct _PopupWindowShellClassRec	*PopupWindowShellWidgetClass;
typedef struct _PopupWindowShellRec		*PopupWindowShellWidget;

extern WidgetClass popupWindowShellWidgetClass;

#endif
