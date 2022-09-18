/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/slider.c	1.5"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/Slider.h>
#include <Xol/ChangeBar.h>

#include <misc.h>
#include <slider.h>

static void		SliderMovedCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));

/**
 ** CreateSlider()
 **/

void
#if	OlNeedFunctionPrototypes
CreateSlider (
	Widget			parent,
	Slider *		slider,
	Boolean			track_changes,
	Boolean			from_color
)
#else
CreateSlider (parent, slider, track_changes, from_color)
	Widget			parent;
	Slider *		slider;
	Boolean			track_changes;
	Boolean			from_color;
#endif
{
	Screen *		screen	= XtScreenOfObject(parent);
	int			dist;

	if (slider->caption) {
		parent = CreateCaption(
				slider->name,
				slider->string,
				parent
		);
	}

	/* 2 horz inches */
	dist = OlScreenMMToPixel (OL_HORIZONTAL, 50.8, screen);
	if (from_color == True) {
		/* Do resources for Color*Slider */
		slider->w = XtVaCreateManagedWidget(
			slider->name,
			sliderWidgetClass,
			parent,
			XtNorientation,	(XtArgVal)OL_HORIZONTAL,
			XtNspan,	(XtArgVal)dist,
			XtNstopPosition,(XtArgVal)OL_GRANULARITY,
			XtNdragCBType,	(XtArgVal)OL_GRANULARITY,
			XtNsliderValue,	(XtArgVal)slider->slider_value,
			XtNsliderMin,	(XtArgVal)slider->slider_min,
			XtNsliderMax,	(XtArgVal)slider->slider_max,
			XtNgranularity,	(XtArgVal)slider->granularity,
			(String)0
		);
	}
	else {
		/* Do resources for Mouse Settings*Slider */
		slider->w = XtVaCreateManagedWidget(
			slider->name,
			sliderWidgetClass,
			parent,
			XtNorientation,	(XtArgVal)OL_HORIZONTAL,
			XtNspan,	(XtArgVal)dist,
			XtNleftMargin,	(XtArgVal)
			  OlScreenMMToPixel(OL_HORIZONTAL,7.62,screen),
			XtNstopPosition,(XtArgVal)OL_GRANULARITY,
			XtNdragCBType,	(XtArgVal)OL_GRANULARITY,
			XtNsliderValue,	(XtArgVal)slider->slider_value,
			XtNsliderMin,	(XtArgVal)slider->slider_min,
			XtNsliderMax,	(XtArgVal)slider->slider_max,
			XtNminLabel,	(XtArgVal)slider->min_label,
			XtNmaxLabel,	(XtArgVal)slider->max_label,
			XtNgranularity,	(XtArgVal)slider->granularity,
			XtNticks,	(XtArgVal)slider->ticks,
			(String)0
		);
	}

	/*
	 * (1) Make the internal sensitivity-flag agree with the actual
	 * sensitivity. This covers the case where the sensitivity is
	 * set from a resource file.
	 * (2) Make sure the widget sensitivity agrees with the internal
	 * flag. This covers the case where the client has programmatic-
	 * ally determined that the slider can't work.
	 * (3) If an insensitive slider is captioned, make the caption
	 * insensitive, too.
	 */
	if (!XtIsSensitive(slider->w))
		slider->sensitive = False;
	else if (!slider->sensitive)
		XtSetSensitive (slider->w, False);
	if (!slider->sensitive && slider->caption)
		XtSetSensitive (parent, False);

	/*
	 * Ask the slider widget for the min/max values, as we set
	 * them from the resource file.
	 */
	XtVaGetValues (
		slider->w,
		XtNsliderMin, (XtArgVal)&(slider->slider_min),
		XtNsliderMax, (XtArgVal)&(slider->slider_max),
		(String)0
	);

	/*
	 * Now set the value.
	 */
	if (slider->slider_value < slider->slider_min)
		slider->slider_value = slider->slider_min;
	if (slider->slider_value > slider->slider_max)
		slider->slider_value = slider->slider_max;
	XtVaSetValues (
		slider->w,
		XtNsliderValue, (XtArgVal)slider->slider_value,
		(String)0
	);

	XtAddCallback (slider->w, XtNsliderMoved, SliderMovedCB, (XtPointer)slider);
	slider->track_changes = track_changes;

	return;
} /* CreateSlider */

/**
 ** SetSlider()
 **/

void
#if	OlNeedFunctionPrototypes
SetSlider (
	Slider *		slider,
	int			value,
	OlDefine		change_state
)
#else
SetSlider (slider, value, change_state)
	Slider *		slider;
	int			value;
	OlDefine		change_state;
#endif
{
	if (slider->slider_value < slider->slider_min)
		slider->slider_value = slider->slider_min;
	if (slider->slider_value > slider->slider_max)
		slider->slider_value = slider->slider_max;
	if (slider->slider_value != value) {
		XtVaSetValues (
			slider->w,
			XtNsliderValue,	(XtArgVal)value,
			(String)0
		);
		if (slider->track_changes)
			_OlSetChangeBarState (slider->w, change_state, OL_PROPAGATE);
	} else
		if (slider->track_changes && change_state == OL_NONE)
			_OlSetChangeBarState (slider->w, OL_NONE, OL_PROPAGATE);

	slider->slider_value = value;
	return;
} /* SetSlider */

/**
 ** SliderMovedCB()
 **/

static void
#if	OlNeedFunctionPrototypes
SliderMovedCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
SliderMovedCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Slider *		slider	= (Slider *)client_data;

	OlSliderVerify *	cd	= (OlSliderVerify *)call_data;


	slider->slider_value = cd->new_location;
	if (slider->track_changes)
		_OlSetChangeBarState (slider->w, OL_NORMAL, OL_PROPAGATE);
	if (slider->f)
		(*slider->f) (slider, slider->closure, cd->more_cb_pending);

	return;
} /* SliderMovedCB */
