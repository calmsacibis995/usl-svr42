/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)flat:FRowColumn.h	1.1"
#endif

#ifndef _OL_FROWCOLUMN_H
#define _OL_FROWCOLUMN_H

/*
 ************************************************************************	
 * Description:
 *	This is the flat row/column container's public header file.
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

extern WidgetClass			flatRowColumnWidgetClass;
typedef struct _FlatRowColumnClassRec *	FlatRowColumnWidgetClass;
typedef struct _FlatRowColumnRec *	FlatRowColumnWidget;

#endif /* _OL_FROWCOLUMN_H */
