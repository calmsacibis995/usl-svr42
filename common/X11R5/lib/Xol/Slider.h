/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)slider:Slider.h	1.4"
#endif

#ifndef _Slider_h
#define _Slider_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***********************************************************************
 *
 * Slider Widget (subclass of CompositeClass)
 *
 ***********************************************************************/


/* Class record constants */

extern WidgetClass sliderWidgetClass;


typedef struct _SliderClassRec *SliderWidgetClass;
typedef struct _SliderRec      *SliderWidget;

typedef struct OlSliderVerify {
	int	new_location;
	Boolean	more_cb_pending;
} OlSliderVerify;

#endif /* _Slider_h */
