/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)flat:FButtons.h	1.5"
#endif

#ifndef _FButtons_h
#define _FButtons_h

/*
 ************************************************************************	
 * Description:
 *	This is the flat buttons container's public header file.
 ************************************************************************	
 */

#include <Xol/FRowColumn.h>

/*
 ************************************************************************	
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 ************************************************************************	
 */

extern WidgetClass			flatButtonsWidgetClass;

typedef struct _FlatButtonsClassRec *	FlatButtonsWidgetClass;
typedef struct _FlatButtonsRec *	FlatButtonsWidget;

#endif /* _FButtons_h */
