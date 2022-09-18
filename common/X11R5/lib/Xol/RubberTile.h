/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)rubbertile:RubberTile.h	2.1"
#endif

#ifndef _RUBBERTILE_H
#define _RUBBERTILE_H

#include "Xol/Panes.h"

extern WidgetClass				rubberTileWidgetClass;

typedef struct _RubberTileClassRec *		RubberTileWidgetClass;
typedef struct _RubberTileRec *			RubberTileWidget;
typedef struct _RubberTileConstraintRec *	RubberTileConstraints;

#endif
