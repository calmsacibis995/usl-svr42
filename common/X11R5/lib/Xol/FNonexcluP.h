/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)flat:FNonexcluP.h	1.4"
#endif

#ifndef _OL_FNONEXCLUP_H
#define _OL_FNONEXCLUP_H

/************************************************************************
    Description:
	This is the flat non-exclusive container's private header file.

*/

#include <Xol/FButtonsP.h>	/* superclasses' header */
#include <Xol/FNonexclus.h>	/* public header */

/************************************************************************
    Define Widget Class Part and Class Rec

*/

	/* Full class record declaration.  Since this class adds no new
	   fields over its superclass, just use typedefs for this class's
	   structures.
	*/

typedef FlatButtonsClassRec	_FlatNonexclusivesClassRec;
typedef FlatButtonsClassRec	FlatNonexclusivesClassRec;

				/* External class record declaration */

extern FlatNonexclusivesClassRec flatNonexclusivesClassRec;

/************************************************************************
    Define Item Instance Structure

*/

typedef FlatButtonsItemRec FlatNonexclusivesItemRec;

/************************************************************************
    Define Widget Instance Structure

*/

	/* Full instance record declaration.  Since this class adds no new
	   fields over its superclass, just use typedefs for this class's
	   instance structure.
	*/

typedef FlatButtonsRec	_FlatNonexclusivesRec;
typedef FlatButtonsRec	FlatNonexclusivesRec;

#endif /* _OL_FNONEXCLUP_H */
