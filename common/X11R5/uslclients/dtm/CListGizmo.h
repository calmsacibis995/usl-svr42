/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _clistgizmo_h
#define _clistgizmo_h

#ifndef NOIDENT
#ident	"@(#)dtm:CListGizmo.h	1.9"
#endif

typedef struct _ClistGizmo {
	char		*name;		/* gizmo name */
	int		width;		/* width of view in number of icons */
	char		*req_prop;	/* required property */
	Boolean		file;		/* include entries for file (hidden) */
	Boolean		sys_class;	/* include glyphs for sys classes */
	Boolean         xenix_class;    /* include glyphs for xenix classes */
	Boolean		usr_class;	/* include glyphs for user classes */
	Boolean		overridden;	/* include overridden classes */
	Boolean		exclusives;	/* exclusives behavior */
	Boolean		noneset;	/* noneset behavior */
	void		(*selectProc)();/* select proc for items */
	DmContainerPtr	cp;		/* container ptr */
	DmItemPtr	itp;		/* item list ptr */
	Widget		swinWidget;	/* scrolled window widget */
	Widget		boxWidget;	/* FIconBox widget */
} CListGizmo;

extern GizmoClassRec	CListGizmoClass[];

/* public routines */
extern void LayoutCListGizmo(CListGizmo *g, Boolean new_list);
extern void ChangeCListItemLabel(CListGizmo *g, int idx, char *label);
extern void ChangeCListItemGlyph(CListGizmo *g, int idx);

#endif /* _clistgizmo_h */
