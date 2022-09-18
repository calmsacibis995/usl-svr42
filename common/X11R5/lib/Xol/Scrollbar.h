/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)scrollbar:Scrollbar.h	1.8"
#endif

#ifndef	_Scrollbar_h
#define	_Scrollbar_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***********************************************************************
 *
 * Scrollbar Widget
 *
 ***********************************************************************/


/* Class record	constants */

extern WidgetClass scrollbarWidgetClass;

typedef	struct _ScrollbarClassRec *ScrollbarWidgetClass;
typedef	struct _ScrollbarRec	  *ScrollbarWidget;

typedef	struct OlScrollbarVerify {
	int	new_location;
	int	new_page;
	Boolean	ok;
	int	slidermin;
	int	slidermax;
	int	delta;
	Boolean	more_cb_pending;
} OlScrollbarVerify;

#endif /* _Scrollbar_h */

