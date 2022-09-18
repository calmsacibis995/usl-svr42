/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)dtm:olwsm/property.h	1.23"

#ifndef _PROPERTY_H
#define _PROPERTY_H

typedef enum ApplyReturnType {
	APPLY_OK,
	APPLY_RESTART,
	APPLY_REFRESH,
	APPLY_ERROR,
	APPLY_NOTICE
}			ApplyReturnType;

typedef struct ApplyReturn {
	ApplyReturnType		reason;
	union {
#if	defined(_NOTICE_H)
		Notice *		notice;
#else
		XtPointer		notice;
#endif
		String			message;
	}			u;
	struct Property *	bad_sheet;
}			ApplyReturn;

typedef struct Property {
	char *		name;
	ArgList		args;	/* Resources set in Widget */
	Cardinal	num_args;
	OlDtHelpInfo *help;
	char		mnemonic;
	void		(*import)  OL_ARGS(( XtPointer ));
	void		(*export)  OL_ARGS(( XtPointer ));
	void		(*create)  OL_ARGS(( Widget , XtPointer ));
	ApplyReturn *	(*apply)   OL_ARGS(( Widget , XtPointer ));
	void		(*reset)   OL_ARGS(( Widget , XtPointer ));
	void		(*factory) OL_ARGS(( Widget , XtPointer ));
	void		(*popdown) OL_ARGS(( Widget , XtPointer ));
	XtPointer	closure;
	String		footer;
	String		pLabel;	/* page label for category widget */
	Widget		w;
}			Property;
 
extern List		global_resources;

extern Property *	*PropertyList;
extern int		numKbdSheets;

extern Property		desktopProperty;
extern Property		iconProperty;
extern Property		mouseProperty;
extern Property		settingsProperty;
extern Property		miscProperty;
extern Property		colorProperty;
extern Property		localeProperty;

extern void		InitProperty OL_ARGS((
	Display* dpy
));
extern void		UpdateResources OL_ARGS((
	void
));
extern void		MergeResources OL_ARGS((
	char *			str
));
extern void		DeleteResources OL_ARGS((
	char *			str
));
extern void		PropertySheetByName OL_ARGS((
	String			name
));
extern void		PropertyCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
extern void		PropertyApplyCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
extern void		DestroyPropertyPopup OL_ARGS((
	void
));
extern void		HelpCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
extern void		PopupMenuCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
extern void		PropertySheetStatus OL_ARGS((
	Widget *		window,
	String *		sheet
));

extern void		setUpKbdCaptions OL_ARGS((
        Display *		dpy
));						 

#if	defined(_EXCLUSIVE_H)
extern ExclusiveItem *	ResourceItem OL_ARGS((
	Exclusive *		exclusive,
	char *			name
));
#endif

#if	defined(_NONEXCLU_H)
extern NonexclusiveItem * NonexclusiveResourceItem OL_ARGS((
	Nonexclusive *		nonexclusive,
	char *			name
));
#endif

#endif
