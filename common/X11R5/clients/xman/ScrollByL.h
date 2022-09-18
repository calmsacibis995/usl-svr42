/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xman:ScrollByL.h	1.1"
/*
 * xman - X window system manual page display program.
 *
 * $XConsortium: ScrollByL.h,v 1.6 91/07/12 15:49:38 dave Exp $
 * $Athena: ScrollByL.h,v 4.0 88/08/31 22:11:16 kit Exp $
 *
 * Copyright 1987, 1988 Massachusetts Institute of Technology
 *
 *
 * Author:    Chris D. Peterson, MIT Project Athena
 * Created:   December 5, 1987
 */

#ifndef _XtScrollByLine_h
#define _XtScrollByLine_h

/***********************************************************************
 *
 * ScrollByLine Widget (subclass of Simple)
 *
 ***********************************************************************/

/*
 * The default fonts.
 */

#ifdef ATHENA
#define MANPAGE_NORMAL   "fixed"
#define MANPAGE_BOLD     "helvetica-bold12"
#define MANPAGE_ITALIC   "helvetica-boldoblique12"
#define MANPAGE_SYMBOL   "symbol-medium12"
#else
#define MANPAGE_NORMAL   "*-new century schoolbook-medium-r-normal--*-120-*"
#define MANPAGE_BOLD     "*-new century schoolbook-bold-r-normal--*-120-*"
#define MANPAGE_ITALIC   "*-new century schoolbook-bold-i-normal--*-120-*"
#define MANPAGE_SYMBOL   "*-symbol-medium-r-normal--*-120-*"
#endif /* ATHENA */

#define XtNindent           "indent"
#define XtNforceVert        "forceVert"
#define XtNmanualFontNormal "manualFontNormal"
#define XtNmanualFontBold   "manualFontBold"
#define XtNmanualFontItalic "manualFontItalic"
#define XtNmanualFontSymbol "manualFontSymbol"

#define XtCIndent           "Indent"

/* Class record constants */

extern WidgetClass scrollByLineWidgetClass;

typedef struct _ScrollByLineClassRec *ScrollByLineWidgetClass;
typedef struct _ScrollByLineRec      *ScrollByLineWidget;

#endif /* _XtScrollByLine_h --- DON'T ADD STUFF AFTER THIS LINE */
