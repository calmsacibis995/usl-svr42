/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Sme.h	1.2"
/*
 * $XConsortium: Sme.h,v 1.4 89/12/11 15:20:09 kit Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

/*
 * Sme.h - Public Header file for Sme object.
 *
 * This is the public header file for the Athena Sme object.
 * It is intended to be used with the simple menu widget.  
 *
 * Date:    April 3, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _Sme_h
#define _Sme_h

#include <X11/RectObj.h>

/****************************************************************
 *
 * Sme Object
 *
 ****************************************************************/

/* Simple Menu Entry Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 callback            Callback		Pointer		NULL
 destroyCallback     Callback		Pointer		NULL
 height		     Height		Dimension	0
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	0
 x		     Position		Position	0n
 y		     Position		Position	0

*/

typedef struct _SmeClassRec*	SmeObjectClass;
typedef struct _SmeRec*	        SmeObject;

extern WidgetClass smeObjectClass;

#endif /* _Sme_h */
