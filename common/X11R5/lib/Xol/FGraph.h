/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _OL_FGRAPH_H
#define _OL_FGRAPH_H

#ifndef	NOIDENT
#ident	"@(#)flat:FGraph.h	1.3"
#endif

/*
 ************************************************************************
 * Description:
 *	This is the flattened Graph widget's public header file.
 ************************************************************************
 */

#include <Xol/Flat.h>

/*
 ************************************************************************
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 ************************************************************************
 */

extern WidgetClass			flatGraphWidgetClass;
typedef struct _FlatGraphClassRec *	FlatGraphWidgetClass;
typedef struct _FlatGraphRec *		FlatGraphWidget;

	/*
	 * Define some new error strings
	 */

#define OleTitemSize			"itemSize"
#define OleMinvalidDimension_itemSize	"Widget \"%s\" (class \"%s\"): item\
 %d has a %s of zero (or undefined), setting to 1"

extern void	OlFlatRaiseItem OL_ARGS((Widget, Cardinal, Boolean));

#endif /* _OL_FGRAPH_H */
