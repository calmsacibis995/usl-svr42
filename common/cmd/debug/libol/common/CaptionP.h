/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_CAPTIONP_H
#define	_CAPTIONP_H
#ident	"@(#)debugger:libol/common/CaptionP.h	1.1"

// toolkit specific members of the Caption class
// included by ../../gui.d/common/Caption.h

// caption is a static text widget; the top level widget is a form
// The predefined caption widget is not being used because it doesn't
// allow for resizing the child widget

private:
	Widget		caption;

	Caption_position position;

#endif	// _CAPTIONP_H
