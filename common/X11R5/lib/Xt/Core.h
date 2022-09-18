/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:Core.h	1.1"
/*
* $XConsortium: Core.h,v 1.10 89/12/12 19:30:32 swick Exp $
* $oHeader: Core.h,v 1.2 88/08/18 15:54:32 asente Exp $
*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtCore_h
#define _XtCore_h

typedef struct _WidgetClassRec *CoreWidgetClass;
typedef struct _WidgetRec *CoreWidget;
externalref WidgetClass coreWidgetClass;

#ifndef _XT_CORE_C
externalref WidgetClass widgetClass;

#endif

#endif /* _XtCore_h */
/* DON'T ADD STUFF AFTER THIS #endif */
