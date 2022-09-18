/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)dtm:olwsm/settings.c	1.14"

#include <stdio.h>
#include <errno.h>
#include <signal.h>

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
#include <slider.h>
#include <property.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>
#include "error.h"

	/*
	 * Define the following if you want to support user changing
	 * the mouse acceleration.
	 */
/* #define ACCELERATION /* */

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
#if	defined(ACCELERATION)
static int		GetPointerControl OL_ARGS((
	Display *		display
));
static void		SetPointerControl OL_ARGS((
	Display *		display
));
#endif

/*
 * Convenient macros:
 */

#if	defined(ACCELERATION)
# define GetAccelerationLevel(dpy) (GetPointerControl(dpy), acceleration)
# define SetAccelerationLevel(dpy,val) (acceleration=val, SetPointerControl(dpy))
#endif

#define MCLICK			0
#define DAMPING			1
#define DRAGRIGHT		2
#define MENUMARKR		3

#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

/*
 * Local data:
 */

static Arg			mouse_args[] = {
/* Note: if you change the location of this element you must also */
/* change the index value in CreatePropertyPopup(). */
	{XtNvSpace,		0},
	{XtNlayoutType,		OL_FIXEDCOLS},
	{XtNmeasure,		1},
	{XtNalignCaptions,	True},
	{XtNcenter,		True},
	{XtNsameSize,		OL_NONE}
};

static OlDtHelpInfo MseSettingsHelp = {
	NULL, NULL, "DesktopMgr/moupref.hlp", NULL, NULL
};

Property		settingsProperty = {
	"Mouse Settings",
	mouse_args,
	XtNumber (mouse_args),
	&MseSettingsHelp,
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

static Resource		_factory[]	= {
	{ "*multiClickTimeout",	 "500" },	/* milliseconds   */
	{ "*mouseDampingFactor",   "8" },	/* pixels         */
	{ "*dragRightDistance",   "20" },	/* pixels         */
	{ "*menuMarkRegion",      "20" },	/* pixels         */
};
static List		factory		= LIST(Resource, _factory);

static Resource		_current[]	= {
	{ "*multiClickTimeout",	   NULL },	/* milliseconds   */
	{ "*mouseDampingFactor",   NULL },	/* pixels         */
	{ "*dragRightDistance",    NULL },	/* pixels         */
	{ "*menuMarkRegion",       NULL },	/* pixels         */
};
static List		current		= LIST(Resource, _current);

#if	defined(ACCELERATION)
static int		FactoryAccelerationLevel = 100;	/* 1 */
static Slider	Accel	 = SLIDER(
				"acceleration",
				1, 1, 100,
				"Min", "Max",
				1, 1
);
#endif

static Slider	MClick	 = SLIDER(
				"multiClick",
				100, 100, 1000,
				"0.1 sec", "1.0 sec",
				25, 100
);
static Slider	Damping	 = SLIDER(
				"mouseDamping",
				1, 1, 20,
				"1 pixel", "20 pixels",
				1, 2
);
static Slider	DragRight= SLIDER(
				"dragRight",
				1, 1, 40,
				"1 pixel", "40 pixels",
				1, 4
);
static Slider	MenuMarkR= SLIDER(
				"menuMarkR",
				1, 1, 40,
				"1 pixel", "40 pixels",
				1, 4
);

#if	defined(ACCELERATION)
static int		acceleration;
static int		threshold;
#endif

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
	merge_resources(&global_resources, &factory);
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
	CURRENT(MCLICK)		= GLOBAL(MCLICK);
	CURRENT(DAMPING)	= GLOBAL(DAMPING);
	CURRENT(DRAGRIGHT)	= GLOBAL(DRAGRIGHT);
	CURRENT(MENUMARKR)	= GLOBAL(MENUMARKR);
	/*
	 *	Enforce our notion of valid resource values ("re-Import").
	 */
	merge_resources(&global_resources, &current);

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
	Screen *		screen	= XtScreenOfObject(work);
	Display *		dpy	= XtDisplayOfObject(work);
#if	defined(ACCELERATION)
	int			accel;


	/*
	 * See if the server can handle acceleration. If this check
	 * ``succeeds'' but the server realy can't handle acceleration,
	 * we have no way of knowing. In that case, make the slider
	 * insensitive through the resource file.
	 */
	Accel.sensitive = False;
	accel = GetAccelerationLevel(XtDisplayOfObject(work));
	if (accel) {
		accel *= 2;
		SetAccelerationLevel (XtDisplayOfObject(work), accel);
		if (GetAccelerationLevel(XtDisplayOfObject(work)) == accel)
			Accel.sensitive = True;
	}
#endif
	MClick.sensitive	= True;
	Damping.sensitive	= True;
	DragRight.sensitive	= True;
	MenuMarkR.sensitive	= True;

#if	defined(ACCELERATION)
	Accel.string = OLG(mouseAcc,fixedString);
	Accel.slider_value   = GetAccelerationLevel(XtDisplayOfObject(work));
#endif

	MClick.slider_value	= atoi(CURRENT(MCLICK));
	Damping.slider_value	= atoi(CURRENT(DAMPING));
	DragRight.slider_value	= atoi(CURRENT(DRAGRIGHT));
	MenuMarkR.slider_value	= atoi(CURRENT(MENUMARKR));

#if	defined(ACCELERATION)
	Accel.string		= OLG(mouseAcc,fixedString);
	Accel.min_label		= OLG(mouseAcc,minLabel);
	Accel.max_label		= OLG(mouseAcc,maxLabel);
#endif

	MClick.string		= OLG(multiClick,fixedString);
	MClick.min_label	= OLG(multiClick,minLabel);
	MClick.max_label	= OLG(multiClick,maxLabel);
	Damping.string		= OLG(damping,fixedString);
	Damping.min_label	= OLG(damping,minLabel);
	Damping.max_label	= OLG(damping,maxLabel);
	DragRight.string	= OLG(dragRight,fixedString);
	DragRight.min_label	= OLG(dragRight,minLabel);
	DragRight.max_label	= OLG(dragRight,maxLabel);
	MenuMarkR.string	= OLG(menuMarkR,fixedString);
	MenuMarkR.min_label	= OLG(menuMarkR,minLabel);
	MenuMarkR.max_label	= OLG(menuMarkR,maxLabel);

#if	defined(ACCELERATION)
	CreateSlider (work, &Accel, True, False);
#endif

	CreateSlider(work, &MClick, True, False);
	CreateSlider(work, &Damping, True, False);
	CreateSlider(work, &DragRight, True, False);
	CreateSlider(work, &MenuMarkR, True, False);

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


#define _APPLY(slider, index) \
	CURRENT(index) = StringSliderValue(&slider);			\
	_OlSetChangeBarState (slider.w, OL_NONE, OL_PROPAGATE)

#if	defined(ACCELERATION)
	SetAccelerationLevel (XtDisplayOfObject(w), Accel.slider_value);
	_OlSetChangeBarState (Accel.w, OL_NONE, OL_PROPAGATE);
#endif
	_APPLY (MClick,    MCLICK);
	_APPLY (Damping,   DAMPING);
	_APPLY (DragRight, DRAGRIGHT);
	_APPLY (MenuMarkR, MENUMARKR);
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
#define _RESET(slider, value)	SetSlider(&slider, value, OL_NONE)

#if	defined(ACCELERATION)
	_RESET (Accel,     GetAccelerationLevel(XtDisplayOfObject(w)));
#endif
	_RESET (MClick,    atoi(CURRENT(MCLICK)));
	_RESET (Damping,   atoi(CURRENT(DAMPING)));
	_RESET (DragRight, atoi(CURRENT(DRAGRIGHT)));
	_RESET (MenuMarkR, atoi(CURRENT(MENUMARKR)));

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
#define _FACTORY(slider, value) \
	SetSlider (&slider, value, OL_NORMAL)

#if	defined(ACCELERATION)
	_FACTORY (Accel,     FactoryAccelerationLevel);
#endif
	_FACTORY (MClick,    atoi(FACTORY(MCLICK)));
	_FACTORY (Damping,   atoi(FACTORY(DAMPING)));
	_FACTORY (DragRight, atoi(FACTORY(DRAGRIGHT)));
	_FACTORY (MenuMarkR, atoi(FACTORY(MENUMARKR)));

#undef	_FACTORY
	return;
} /* FactoryCB */

/**
 ** GetPointerControl()
 **/

#if	defined(ACCELERATION)

static int
#if	OlNeedFunctionPrototypes
GetPointerControl (
	Display *		display
)
#else
GetPointerControl (display)
	Display *		display;
#endif
{
	int			n;
	int			d;

	XGetPointerControl (display, &n, &d, &threshold);
	acceleration = (100 * n) / d;
} /* GetPointerControl */

#endif

/**
 ** SetPointerControl()
 **/

#if	defined(ACCELERATION)

static void
#if	OlNeedFunctionPrototypes
SetPointerControl (
	Display *		display
)
#else
SetPointerControl (display)
	Display *		display;
#endif
{
	XChangePointerControl (display, True, True, acceleration, 100, threshold);
	return;
} /* SetPointerControl */

#endif
