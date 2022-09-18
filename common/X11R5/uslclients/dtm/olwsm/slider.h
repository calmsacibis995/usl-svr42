/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/slider.h	1.6"
#endif

#ifndef _WSM_SLIDER_H
#define _WSM_SLIDER_H

typedef struct Slider {
	Boolean		caption;
	char *		name;
	char *		string;
	void		(*f) OL_ARGS(( struct Slider * , XtPointer , Boolean ));
	XtPointer	closure;
	int		slider_value;
	int		slider_min;
	int		slider_max;
	String		min_label;
	String		max_label;
	int		granularity;
	int		ticks;
	Widget		w;
	Boolean		track_changes;
	Boolean		sensitive;
	char		string_slider_value[20];
}			Slider;

#define StringSliderValue(P) ( \
	sprintf((P)->string_slider_value, "%d", (P)->slider_value),	\
	(P)->string_slider_value					\
    )

#define SLIDER(name, value, min, max, minlabel, \
	       maxlabel, gran, ticks) \
       {True, name, NULL, NULL, NULL, value, min, max, \
	minlabel, maxlabel, gran, ticks, NULL, False, True}

#define COLOR_SLIDER(name) \
	{True, name, NULL, NULL, NULL, 1, 1, 256, NULL, NULL, 1, 0, NULL, \
	 False, True }

extern void		CreateSlider OL_ARGS((
	Widget			parent,
	Slider *		slider,
	Boolean			track_changes,
	Boolean			from_color
));
extern void		SetSlider OL_ARGS((
	Slider *		slider,
	int			value,
	OlDefine		change_state
));

#endif
