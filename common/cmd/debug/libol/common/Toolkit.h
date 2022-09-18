/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TOOLKIT_H
#define	_TOOLKIT_H
#ident	"@(#)debugger:libol/common/Toolkit.h	1.6"

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#define __Ol_OlXlibExt_h__
#include <Xol/OpenLook.h>
#include "Help.h"

#define	PADDING	4
#define	PROPERTIES	"Properties"
#define DISMISS		"Dismiss"
#define DISMISS_MNE	'D'

extern	Widget		base_widget;
extern	OlDefine	gui_mode;

void	register_help(Widget, const char *title, Help_id);
void	display_help(Widget, Help_mode, Help_id);

#endif // _TOOLKIT_H
