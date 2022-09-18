/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Porthole.h	1.2"
/*
 * $XConsortium: Porthole.h,v 1.1 90/02/28 18:07:31 jim Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XawPorthole_h
#define _XawPorthole_h

#include <X11/Xaw/Reports.h>

/*****************************************************************************
 * 
 * Porthole Widget (subclass of Composite)
 * 
 * This widget is similar to a viewport without scrollbars.  Child movement
 * is done by external panners or scrollbars.
 * 
 * Parameters:
 * 
 *  Name		Class		Type		Default
 *  ----		-----		----		-------
 * 
 *  background		Background	Pixel		XtDefaultBackground
 *  border	        BorderColor	Pixel		XtDefaultForeground
 *  borderWidth		BorderWidth	Dimension	1
 *  height		Height		Dimension	0
 *  reportCallback	ReportCallback	Pointer		NULL
 *  width		Width		Dimension	0
 *  x 			Position	Position	0
 *  y			Position	Position	0
 * 
 *****************************************************************************/

					/* external declarations */

extern WidgetClass portholeWidgetClass;
typedef struct _PortholeClassRec *PortholeWidgetClass;
typedef struct _PortholeRec      *PortholeWidget;

#endif /* _XawPorthole_h */
