/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XGetFProp.c	1.1"
/* $XConsortium: XGetFProp.c,v 11.7 91/01/06 11:45:56 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

Bool XGetFontProperty (fs, name, valuePtr)
    XFontStruct *fs;
    register Atom name;
    unsigned long *valuePtr;
    {
    /* XXX this is a simple linear search for now.  If the
      protocol is changed to sort the property list, this should
      become a binary search. */
    register XFontProp *prop = fs->properties;
    register XFontProp *last = prop + fs->n_properties;
    while (prop != last) {
	if (prop->name == name) {
	    *valuePtr = prop->card32;
	    return (1);
	    }
	prop++;
	}
    return (0);
    }

	
    

      
