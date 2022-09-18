/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)textedit:TextUtil.h	1.4"
#endif

/*
 * TextUtil.h
 *
 */

#ifndef _TextUtil_h
#define _TextUtil_h

#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>      /* for BufferElement */

extern Bool _IsGraphicsExpose();
extern Cursor _CreateCursor();
extern Cursor _CreateCursorFromBitmaps();
extern Time   _XtLastTimestampProcessed();

#ifdef I18N
extern int    _mbCopyOfwcString  OL_ARGS((char **, BufferElement *));
extern int    _mbCopyOfwcSegment OL_ARGS((char **, BufferElement *, int));
#endif

#endif
