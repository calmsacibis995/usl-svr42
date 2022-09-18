/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olhelp:Mag.h	1.4"
#endif

#ifndef _Mag_h_
#define _Mag_h_

/*
 *************************************************************************
 *
 * Date:	March 1989
 *
 * Description:
 *		Public header file for the Magnifier widget
 *
 *******************************file*header*******************************
 */

#include <Xol/Primitive.h>	/* include superclasses' header */

extern WidgetClass magWidgetClass;

typedef struct _MagClassRec	*MagWidgetClass;
typedef struct _MagRec	*MagWidget;


#endif /* _Mag_h_ */
