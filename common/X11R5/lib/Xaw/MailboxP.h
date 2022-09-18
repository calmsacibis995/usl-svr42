/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:MailboxP.h	1.2"
/*
 * $XConsortium: MailboxP.h,v 1.20 91/07/19 21:52:57 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XawMailboxP_h
#define _XawMailboxP_h

#include <X11/Xaw/Mailbox.h>
#include <X11/Xaw/SimpleP.h>

#ifdef SYSV
#define MAILBOX_DIRECTORY "/usr/mail"
#else
#ifdef SVR4
#define MAILBOX_DIRECTORY "/var/mail"
#else
#define MAILBOX_DIRECTORY "/usr/spool/mail"
#endif
#endif

typedef struct {			/* new fields for mailbox widget */
    /* resources */
    int update;				/* seconds between updates */
    Pixel foreground_pixel;		/* color index of normal state fg */
    String filename;			/* filename to watch */
    String check_command;		/* command to exec for mail check */
    Boolean flipit;			/* do flip of full pixmap */
    int volume;				/* bell volume */
    Boolean once_only;			/* ring bell only once on new mail */
    /* local state */
    GC gc;				/* normal GC to use */
    long last_size;			/* size in bytes of mailboxname */
    XtIntervalId interval_id;		/* time between checks */
    Boolean flag_up;			/* is the flag up? */
    struct _mbimage {
	Pixmap bitmap, mask;		/* depth 1, describing shape */
	Pixmap pixmap;			/* full depth pixmap */
	int width, height;		/* geometry of pixmaps */
    } full, empty;
    Boolean shapeit;			/* do shape extension */
    struct {
	Pixmap mask;
	int x, y;
    } shape_cache;			/* last set of info */
} MailboxPart;

typedef struct _MailboxRec {		/* full instance record */
    CorePart core;
    SimplePart simple;
    MailboxPart mailbox;
} MailboxRec;


typedef struct {			/* new fields for mailbox class */
    int dummy;				/* stupid C compiler */
} MailboxClassPart;

typedef struct _MailboxClassRec {	/* full class record declaration */
    CoreClassPart core_class;
    SimpleClassPart simple_class;
    MailboxClassPart mailbox_class;
} MailboxClassRec;

extern MailboxClassRec mailboxClassRec;	 /* class pointer */

#endif /* _XawMailboxP_h */
