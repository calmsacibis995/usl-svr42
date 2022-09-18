/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:TextGizmo.h	1.3"
#endif

#ifndef _textgizmo_h
#define _textgizmo_h

typedef struct _TextGizmo {
	char *		source;
	int		lines;
	int		width;
	Widget		textField;
} TextGizmo;

extern GizmoClassRec	TextGizmoClass[];
extern void		SetTextFieldValue();
extern char *		GetTextFieldValue();

#endif /* _textgizmo_h */
