/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:RectObj.h	1.1"
/* $XConsortium: RectObj.h,v 1.8 89/09/11 17:39:45 swick Exp $ */
/* $oHeader: RectObj.h,v 1.2 88/08/18 17:39:17 asente Exp $ */
/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtRect_h
#define _XtRect_h

typedef struct _RectObjRec *RectObj;
typedef struct _RectObjClassRec *RectObjClass;

#ifndef RECTOBJ
externalref WidgetClass rectObjClass;
#endif
#endif /* _XtRect_h */
/* DON'T ADD STUFF AFTER THIS #endif */
