/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:SetWMCW.c	1.1"
/* $XConsortium: SetWMCW.c,v 1.4 91/01/08 15:06:58 converse Exp $ */
/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Chris D. Peterson, MIT X Consortium
 */

#include "IntrinsicI.h"
#include <X11/Xatom.h>

/*	Function Name: XtSetWMColormapWindows
 *
 *	Description: Sets the value of the WM_COLORMAP_WINDOWS
 *                   property on a widget's window.
 *
 *	Arguments:  widget - specifies the widget on whose window the
 *   		           - WM_COLORMAP_WINDOWS property will be stored.
 *
 *                  list - Specifies a list of widgets whose windows are to be
 *		           listed in the WM_COLORMAP_WINDOWS property.
 *                  count - Specifies the number of widgets in list.
 *
 *	Returns: none.
 */

void
XtSetWMColormapWindows(widget, list, count)
Widget widget, *list;
Cardinal count;
{
    Window *data;
    Widget *checked, *top, *temp;
    register Cardinal i, j, checked_count;
    register Boolean match;
    Atom xa_wm_colormap_windows;

    if ( !XtIsRealized(widget) || (count == 0) ) return;

    top = checked = (Widget *) XtMalloc( (Cardinal) sizeof(Widget) * count);


/*
 * The specification calls for only adding the windows that have unique 
 * colormaps to the property to this function, so we will make a pass through
 * the widget list removing all the widgets with non-unique colormaps.
 *
 * We will also remove any unrealized widgets from the list at this time.
 */

    for (checked_count = 0, i = 0; i < count; i++) {
	if (!XtIsRealized(list[i])) continue;
	    
	*checked = list[i];
	match = FALSE;

/*
 * Don't check first element for matching colormap since there is nothing
 * to check it against.
 */

	if (checked != top)	
	    for (j = 0, temp = top; j < checked_count ; j++, temp++)
		if ( (*temp)->core.colormap == (*checked)->core.colormap) {
		    match = TRUE;
		    break;
		}

/*
 * If no colormap was found to match then add this widget to the linked list.
 */

	if (!match) {
	    checked++;
	    checked_count++;
	}
    }

/*
 * Now that we have the list of widgets we need to convert it to a list of
 * windows and set the property.
 */

    data = (Window *) XtMalloc( (Cardinal) sizeof(Window) * checked_count);

    for ( i = 0 ; i < checked_count ; i++)
	data[i] = XtWindow(top[i]);

/*
 * Relax, there's an atom cache in Xlib.
 */

    xa_wm_colormap_windows = XInternAtom(XtDisplay(widget),
					 "WM_COLORMAP_WINDOWS", FALSE);

/*
 * No need to check return from XInternAtom() since the atom will be
 * created if it doesn't exist.
 */

    XChangeProperty(XtDisplay(widget), XtWindow(widget), 
		    xa_wm_colormap_windows, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *) data, (int) i);

    XtFree( (char *) data);
    XtFree( (char *) top);
}
		    
    
