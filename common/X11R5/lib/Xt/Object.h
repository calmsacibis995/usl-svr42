/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:Object.h	1.1"
/* $XConsortium: Object.h,v 1.8 89/09/11 17:39:25 swick Exp $ */
/* $oHeader: Object.h,v 1.2 88/08/18 15:55:32 asente Exp $ */
/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtObject_h
#define _XtObject_h

typedef struct _ObjectRec *Object;
typedef struct _ObjectClassRec *ObjectClass;

#ifndef OBJECT
externalref WidgetClass objectClass;
#endif
#endif /* _XtObject_h */
/* DON'T ADD STUFF AFTER THIS #endif */
