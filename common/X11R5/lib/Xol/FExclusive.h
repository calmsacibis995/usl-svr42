/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)flat:FExclusive.h	1.5"
#endif

#ifndef _OL_FEXCLUSIVE_H
#define _OL_FEXCLUSIVE_H

/************************************************************************
    Description:
	This is the flat exclusives container's public header file.
*/

#include <Xol/FButtons.h>		/* superclasses' header */

/************************************************************************
    Define class and instance pointers:
	- extern pointer to class data/procedures
	- typedef pointer to widget's class structure
	- typedef pointer to widget's instance structure
*/

extern WidgetClass				flatExclusivesWidgetClass;
typedef struct _FlatExclusivesClassRec *	FlatExclusivesWidgetClass;
typedef struct _FlatExclusivesRec *		FlatExclusivesWidget;

#endif /* _OL_FEXCLUSIVE_H */
