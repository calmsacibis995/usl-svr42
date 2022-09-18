/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/cursor.h	1.3"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: cursor.h,v 1.17 88/09/06 15:48:50 jim Exp $ */

#ifndef CURSOR_H
#define CURSOR_H 
#define NullCursor ((CursorPtr)NULL)

typedef struct _Cursor *CursorPtr;
typedef struct _CursorMetric *CursorMetricPtr;

extern CursorPtr rootCursor;
#endif /* CURSOR_H */
