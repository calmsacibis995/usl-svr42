/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/xinput/XFreeLst.c	1.1"
/* $XConsortium: XFreeLst.c,v 1.2 89/09/25 16:20:15 gms Exp $ */

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved



********************************************************/

/***********************************************************************
 *
 * XFreeDeviceList - free the input device list.
 *
 */

#include <stdio.h>
#include "Xlib.h"

/***********************************************************************
 *
 * Free the list of input devices.
 *
 */

XFreeDeviceList (list)
    XDeviceList *list;
    {
    if (list != NULL) 
	{
        XFree ((list->name) - sizeof(XDeviceList));
        XFree ((XDeviceList *) list);
        }
    }
