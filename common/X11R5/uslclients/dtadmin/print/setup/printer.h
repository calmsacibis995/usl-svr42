/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/printer.h	1.3"
#endif

#ifndef PRINTER_H
#define PRINTER_H

#define ICONBOX_WIDTH	(5*BOGUS_WIDTH)
#define ICONBOX_HEIGHT	(3*BOGUS_HEIGHT)
#define BOGUS_WIDTH	70
#define BOGUS_HEIGHT	50

typedef struct {
    XtArgVal	lbl;
    XtArgVal	glyph;
    XtArgVal	x;
    XtArgVal	y;
    XtArgVal	width;
    XtArgVal	height;
    XtArgVal	selected;
    XtArgVal	properties;
} IconItem;

extern void	InitSupportedPrinters (Widget);
extern Widget	GetPrinterList(Widget, PropertyCntl *);
extern Printer	*GetDefaultPrinter(Widget);
extern void	GetActivePrinters (Widget, IconItem **, int *,
				   Dimension, Dimension);

extern void	AddPrinter (PropertyData *);
extern void	DelPrinters (void);

#endif /* PRINTER_H */
