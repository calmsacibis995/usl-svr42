/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Mailbox.h	1.2"
/*
 * $XConsortium: Mailbox.h,v 1.20 91/05/04 18:58:42 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XawMailbox_h
#define _XawMailbox_h

/*
 * Mailbox widget; looks a lot like the clock widget, don't it...
 */

/* resource names used by mailbox widget that aren't defined in StringDefs.h */

#ifndef _XtStringDefs_h_
#define XtNupdate "update"
#endif

/* command to exec */
#define XtNcheckCommand "checkCommand"
#define XtNonceOnly "onceOnly"

/* Int: volume for bell */
#define XtNvolume "volume"
#define XtNfullPixmap "fullPixmap"
#define XtNfullPixmapMask "fullPixmapMask"
#define XtNemptyPixmap "emptyPixmap"
#define XtNemptyPixmapMask "emptyPixmapMask"
#define XtNflip "flip"
#define XtNshapeWindow "shapeWindow"

#define XtCCheckCommand "CheckCommand"
#define XtCVolume "Volume"
#define XtCPixmapMask "PixmapMask"
#define XtCFlip "Flip"
#define XtCShapeWindow "ShapeWindow"


/* structures */

typedef struct _MailboxRec *MailboxWidget;  /* see MailboxP.h */
typedef struct _MailboxClassRec *MailboxWidgetClass;  /* see MailboxP.h */


extern WidgetClass mailboxWidgetClass;

#endif /* _XawMailbox_h */
/* DON'T ADD STUFF AFTER THIS #endif */
