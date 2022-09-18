/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xfd:fontgrid.h	1.1"
#ifndef _FontGrid_h_
#define _FontGrid_h_

typedef struct _FontGridRec *FontGridWidget;
extern WidgetClass fontgridWidgetClass;

#define XtNcellRows "cellRows"
#define XtCCellRows "CellRows"
#define XtNcellColumns "cellColumns"
#define XtCCellColumns "CellColumns"
#define XtNcellWidth "cellWidth"
#define XtCCellWidth "CellWidth"
#define XtNcellHeight "cellHeight"
#define XtCCellHeight "CellHeight"

#define XtNcenterChars "centerChars"
#define XtCCenterChars "CenterChars"

#define XtNboxChars "boxChars"
#define XtCBoxChars "BoxChars"

#define XtNboxColor "boxColor"
#define XtCBoxColor "BoxColor"

#define XtNstartChar "startChar"
#define XtCStartChar "StartChar"

#define XtNinternalPad "internalPad"
#define XtCInternalPad "InternalPad"

#define XtNgridWidth "gridWidth"
#define XtCGridWidth "GridWidth"

typedef struct _FontGridCharRec {
    XFontStruct *	thefont;
    XChar2b		thechar;
} FontGridCharRec;

extern void GetFontGridCellDimensions(
#if NeedFunctionPrototypes
   Widget,
   Dimension *,
   int *,
   int *
#endif
);

extern void GetPrevNextStates(
#if NeedFunctionPrototypes
    Widget,
    Bool *,
    Bool *
#endif
);

#endif /* _FontGrid_h_ */
