/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/WMP.h	1.2"
#endif

#if	!defined(_WMP_H)
#define _WMP_H

#include "Xol/ManagerP.h"

#include "WM.h"

/*
 * Class description:
 */

typedef struct {
	int			unused;
}			WMClassPart;


typedef struct _WMClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	WMClassPart		wm_class;
}			WMClassRec;

/*
 * Instance description:
 */

typedef struct WMmetrics {
	int			scale;
	int			cornerX;
	int			cornerY;
	int			cornerx;
	int			cornery;
	int			linewidX;
	int			linewidY;
	int			linewidx;
	int			linewidy;
	int			gapx;
	int			gapy;
	int			bannerht;
	int			hgap1;
	int			linewid;
	int			hgap2;
	int			offset;
	int			baseline;
	int			markwid;
	int			selectwidth;
}			WMmetrics;

typedef struct WMPart {
	/*
	 * Public:
	 */
	String			title;
	XFontStruct *		font;
	Pixel			foreground;
	Pixel			input_window_header;
	Boolean			pointer_focus;
	XtCallbackList		select;

	/*
	 * Private:
	 */
	WMmetrics *		metrics;
	OlgAttrs *		attrs;
	Dimension		mmWidth;
	Dimension		mmHeight;
	Widget			child;
}			WMPart;

typedef struct _WMRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	WMPart			wm;
}			WMRec;

extern WMClassRec	wmClassRec;

#endif	/* _WMP_H */
