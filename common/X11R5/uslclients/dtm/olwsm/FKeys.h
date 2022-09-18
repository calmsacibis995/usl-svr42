/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/FKeys.h	1.5"
#endif

#ifndef _OL_FKEYS_H
#define _OL_FKEYS_H

#include <Xol/FRowColumn.h>
#include <Xol/ChangeBar.h>

#define OL_FLATKEY_CHANGING	1100
#define OL_FLATKEY_CHANGED	1101
#define OL_FLATKEY_UNDONE	1102
#define OL_FLATKEY_DELETED	1103
#define OL_FLATKEY_ABORTED	1104

extern char		XtNmodifiers     [];
extern char		XtNkeysym        [];
extern char		XtNcaptionFont   [];
extern char		XtNcaptionGap    [];
extern char		XtNkeyChanged    [];
extern char		XtNisHeader      [];

extern char		XtCModifiers     [];
extern char		XtCKeysym        [];
extern char		XtCCaptionFont   [];
extern char		XtCCaptionGap    [];
extern char		XtCKeyChanged    [];
extern char		XtCIsHeader      [];

typedef struct OlFlatKeyChanged {
	Boolean			ok;
	Cardinal		index;
	Modifiers		modifiers;
	KeySym			keysym;
	OlDefine		change;
}			OlFlatKeyChanged;

extern WidgetClass			flatKeysWidgetClass;
typedef struct _FlatKeysClassRec *	FlatKeysWidgetClass;
typedef struct _FlatKeysRec *		FlatKeysWidget;

#endif /* _OL_FKEYS_H */
