/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)manager:Manager.h	1.4"
#endif


#ifndef _OlManager_h
#define _OlManager_h

#include <X11/Constraint.h>
					/* Class record constants */

extern WidgetClass			managerWidgetClass;
typedef struct _ManagerClassRec	*	ManagerWidgetClass;
typedef struct _ManagerRec	*	ManagerWidget;

#endif	/* _OlManager_h */
