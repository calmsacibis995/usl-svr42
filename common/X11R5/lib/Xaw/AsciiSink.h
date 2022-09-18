/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:AsciiSink.h	1.2"
/*
 * $XConsortium: AsciiSink.h,v 1.7 90/12/01 13:01:31 rws Exp $
 */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XawAsciiSink_h
#define _XawAsciiSink_h

/***********************************************************************
 *
 * AsciiSink Object
 *
 ***********************************************************************/

#include <X11/Xaw/TextSink.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 echo                Output             Boolean         True
 displayNonprinting  Output             Boolean         True

*/

#define XtCOutput "Output"

#define XtNdisplayNonprinting "displayNonprinting"
#define XtNecho "echo"

/* Class record constants */

extern WidgetClass asciiSinkObjectClass;

typedef struct _AsciiSinkClassRec *AsciiSinkObjectClass;
typedef struct _AsciiSinkRec      *AsciiSinkObject;

/************************************************************
 *
 * Public Functions.
 *
 ************************************************************/

#endif /* _XawAsciiSrc_h */
/* DON'T ADD STUFF AFTER THIS #endif */
