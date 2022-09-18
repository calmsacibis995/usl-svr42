/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:Pointer.c	1.1"
/* $XConsortium: Pointer.c,v 1.4 91/08/21 16:39:18 converse Exp $ */

/********************************************************

Copyright (c) 1988 by Hewlett-Packard Company
Copyright (c) 1987, 1988, 1989 by Digital Equipment Corporation, Maynard, 
              Massachusetts, and the Massachusetts Institute of Technology, 
              Cambridge, Massachusetts



********************************************************/

#include "IntrinsicI.h"
#include "PassivGraI.h"


#define AllButtonsMask (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)

Widget _XtProcessPointerEvent(event, widget, pdi)
    XButtonEvent  	*event;
    Widget 		widget;
    XtPerDisplayInput 	pdi;
{    
    XtDevice		device = &pdi->pointer;
    XtServerGrabPtr	newGrab = NULL, devGrab = &device->grab;
    Widget		dspWidget = NULL;
    Boolean		deactivateGrab = FALSE;

    switch (event->type)
      {
	case ButtonPress:
	  {
	      if (!IsServerGrab(device->grabType))
		{
		    Cardinal		i;

		    for (i = pdi->traceDepth; 
			 i > 0 && !newGrab; 
			 i--)
		      newGrab = _XtCheckServerGrabsOnWidget((XEvent*)event, 
							    pdi->trace[i-1],
							    POINTER);
		}
	      if (newGrab)
		{
		    /* Activate the grab */
		    device->grab = *newGrab;
		    device->grabType = XtPassiveServerGrab;
		}
	  }
	  break;
	  
	case ButtonRelease:
	  {
	      if ((device->grabType == XtPassiveServerGrab) && 
		  !(event->state & ~(Button1Mask << (event->button - 1)) &
		    AllButtonsMask))
		deactivateGrab = TRUE;
	  }
	  break;
      }
    
    if (IsServerGrab(device->grabType) && !(devGrab)->ownerEvents)
      dspWidget = (devGrab)->widget;
    else
      dspWidget = widget;
    
    if (deactivateGrab)
      device->grabType = XtNoServerGrab;

    return dspWidget;
}
