/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)shlib:OlXlibExt.h	1.3"
#endif


#ifndef __Ol_OlXlibExt_h__
#define __Ol_OlXlibExt_h__

/*
 *************************************************************************
 *
 * Description:
 *		This file declares all the external Xlib functions used by
 *		OPEN LOOK (tm).
 *
 *******************************file*header*******************************
 */

extern Bool XGetFontProperty();
extern Bool XCheckWindowEvent();
extern Status XSendEvent();
extern Bool XCheckWindowEvent();
extern Bool XQueryPointer();
extern Bool XTranslateCoordinates();
extern Status XGetWindowAttributes();
extern KeyCode XKeysymToKeycode();
extern Boolean XtOwnSelection();
extern void XtGetSelectionValue();
#ifndef MEMUTIL
extern char *XtMalloc();
extern void XtFree();
#endif
extern XDeleteProperty();

#endif /* __Ol_OlXlibExt_h__ */
