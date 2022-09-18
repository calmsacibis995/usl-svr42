/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)basewindow:BaseWindow.h	1.5"
#endif

#ifndef _OlBaseWindow_h_
#define _OlBaseWindow_h_

/*************************************************************************
 *
 * Description:
 *		This is the "public" include file for the BaseWindow Widget.
 *	This baseWindow widget belongs to the OPEN LOOK (Tm - AT&T) Toolkit.
 *
 *****************************file*header********************************/

#include <X11/Shell.h>

extern WidgetClass				baseWindowShellWidgetClass;
typedef struct _BaseWindowShellClassRec *	BaseWindowShellWidgetClass;
typedef struct _BaseWindowShellRec *		BaseWindowShellWidget;

#endif /* _OlBaseWindow_h_ */
