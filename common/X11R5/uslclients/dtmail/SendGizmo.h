/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:SendGizmo.h	1.6"
#endif

#ifndef _send_gizmo_h
#define _send_gizmo_h

typedef struct _SendGizmo {
	char *		name;
	Widget		subject;
	Widget		to;
	Widget		bcc;
	Widget		cc;
	Widget		control;
	Widget		displayArea;
	Widget		mailForm;
} SendGizmo;

extern GizmoClassRec	SendGizmoClass[];

extern char *		OlGetWrappedLine ();
extern Widget		GetSendTextWidget();
extern void		SetSendTextAndHeader();

#endif /* _send_gizmo_h */
