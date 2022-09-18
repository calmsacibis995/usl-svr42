/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)dtm:olwsm/icon.c	1.27"

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/Category.h>
#include <Xol/ChangeBar.h>

#include <misc.h>
#include <node.h>
#include <list.h>
#include <exclusive.h>
#include <property.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>
#include "error.h"

/*
 * Local routines:
 */

static void		Import OL_ARGS((
	XtPointer		closure
));
static void		Export OL_ARGS((
	XtPointer		closure
));
static void		Create OL_ARGS((
	Widget			work,
	XtPointer		closure
));
static ApplyReturn *	ApplyCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		ResetCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		FactoryCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));

/*
 * Global data:
 */

static Arg		icon_args[] = {
	{XtNlayoutType,		OL_FIXEDCOLS},
	{XtNalignCaptions,	True}
};

static OlDtHelpInfo IconHelp = {
	NULL, NULL, "DesktopMgr/icnpref.hlp", NULL, NULL
};

Property		iconProperty = {
	"Icons",
	icon_args,
	XtNumber (icon_args),
	&IconHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	0,
	0,
	0,
	0,
};

/*
 * Convenient macros:
 */

#define ITEM(exclusive, addr)	ResourceItem(exclusive, addr)

#define PARKING			0
#define BORDER			1

#define FACTORY(x)     *(ADDR *)&(_factory[x].value)
#define CURRENT(x)     *(ADDR *)&(_current[x].value)
#define GLOBAL(x)      resource_value(&global_resources, _factory[x].name)

/*
 * Local data:
 */

static char		_north   [] = "north";
static char		_south   [] = "south";
static char		_west    [] = "west";
static char		_east    [] = "east";

static char		_show    [] = "true";
static char		_noshow  [] = "false";

static Resource		_factory[] = {
	{ "*iconGravity", _south },
	{ "*iconBorder", _show },
};
static List		factory	   = LIST(Resource, _factory);

static Resource		_current[] = {
	{ "*iconGravity", NULL },
	{ "*iconBorder", NULL },
};
static List		current    = LIST(Resource, _current);

static ExclusiveItem	_parking[] = {
	{ (XtArgVal)"Top",    (XtArgVal)_north },
	{ (XtArgVal)"Bottom", (XtArgVal)_south },
	{ (XtArgVal)"Left",   (XtArgVal)_west  },
	{ (XtArgVal)"Right",  (XtArgVal)_east  },
};
static List		parking    = LIST(ExclusiveItem, _parking);
static Exclusive	Parking    = EXCLUSIVE("location", "Location:", &parking);

static ExclusiveItem	_border[]  = {
	{ (XtArgVal)"Show",       (XtArgVal)_show   },
	{ (XtArgVal)"Don't Show", (XtArgVal)_noshow },
};
static List		border     = LIST(ExclusiveItem, _border);
static Exclusive	Border     = EXCLUSIVE("border", "Border:", &border);

/**
 ** Import()
 **/

static void
#if	OlNeedFunctionPrototypes
Import (
	XtPointer		closure
)
#else
Import (closure)
	XtPointer		closure;
#endif
{
	merge_resources (&global_resources, &factory);
	return;
} /* Import */

/**
 ** Export()
 **/

static void
#if	OlNeedFunctionPrototypes
Export (
	XtPointer		closure
)
#else
Export (closure)
	XtPointer		closure;
#endif
{
	ExclusiveItem *		p;


#define _EXPORT(exclusive, index) \
	p = ITEM(exclusive, GLOBAL(index));				\
	CURRENT(index) = (p? (ADDR)p->addr : FACTORY(index))

	_EXPORT (&Parking, PARKING);
	_EXPORT (&Border,  BORDER);
	/*
	 *	Enforce our notion of valid resource values ("re-Import").
	 */
	merge_resources (&global_resources, &current);

#undef	_EXPORT
	return;
} /* Export */

/**
 ** Create()
 **/

static void
#if	OlNeedFunctionPrototypes
Create (
	Widget			work,
	XtPointer		closure
)
#else
Create (work, closure)
	Widget			work;
	XtPointer		closure;
#endif
{
	Cardinal n = 0;
	Display *dpy = XtDisplay(work);
	
	Parking.current_item = ITEM(&Parking, CURRENT(PARKING));
	Border.current_item  = ITEM(&Border,  CURRENT(BORDER));

	_parking[0].name = (XtArgVal) OLG(top,fixedString);
	_parking[1].name = (XtArgVal) OLG(bottom,fixedString);
	_parking[2].name = (XtArgVal) OLG(left,fixedString);
	_parking[3].name = (XtArgVal) OLG(right,fixedString);
	parking.entry = (ADDR)_parking;
	Parking.items = &parking;
	Parking.string = OLG(location,fixedString);
	
	_border[0].name = (XtArgVal) OLG(show,fixedString);
	_border[1].name = (XtArgVal) OLG(noShow,fixedString);
	border.entry = (ADDR)_border;
	Border.items = &border;
	Border.string = OLG(border,fixedString);

	
	CreateExclusive (work, &Parking, True);
	CreateExclusive (work, &Border,  True);

	return;
} /* Create */

/**
 ** ApplyCB()
 **/

static ApplyReturn *
#if	OlNeedFunctionPrototypes
ApplyCB (
	Widget			w,
	XtPointer		closure
)
#else
ApplyCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	static ApplyReturn	ret = { APPLY_OK };


#define _APPLY(exclusive, index) \
	CURRENT(index) = (ADDR)(exclusive).current_item->addr;		\
	_OlSetChangeBarState ((exclusive).w, OL_NONE, OL_PROPAGATE)

	_APPLY (Parking, PARKING);
	_APPLY (Border,  BORDER);
	merge_resources(&global_resources, &current);

#undef	_APPLY
	return (&ret);
} /* ApplyCB */

/**
 ** ResetCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ResetCB (
	Widget			w,
	XtPointer		closure
)
#else
ResetCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
#define _RESET(exclusive, index) \
	SetExclusive(exclusive, ITEM(exclusive, CURRENT(index)), OL_NONE)

	_RESET (&Parking, PARKING);
	_RESET (&Border,  BORDER);

#undef	_RESET
	return;
} /* ResetCB */

/**
 ** FactoryCB()
 **/

static void
#if	OlNeedFunctionPrototypes
FactoryCB (
	Widget			w,
	XtPointer		closure
)
#else
FactoryCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
#define _FACTORY(exclusive, index) \
	SetExclusive(exclusive, ITEM(exclusive, FACTORY(index)), OL_NORMAL)

	_FACTORY (&Parking, PARKING);
	_FACTORY (&Border,  BORDER);

#undef	_FACTORY
	return;
} /* FactoryCB */
