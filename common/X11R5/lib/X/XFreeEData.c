/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFreeEData.c	1.1"
/* $XConsortium: XFreeEData.c,v 11.13 91/01/06 11:45:47 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
*/

#include "Xlibint.h"

_XFreeExtData (extension)
     XExtData *extension;
{
	XExtData *temp;
	while (extension) {
		if (extension->free_private) 
		    (*extension->free_private)(extension);
		else Xfree ((char *)extension->private_data);
		temp = extension->next;
		Xfree ((char *)extension);
		extension = temp;
	}
	return;
}
