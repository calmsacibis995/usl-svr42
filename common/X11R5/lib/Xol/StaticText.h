/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)statictext:StaticText.h	1.6"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        StaticText.h
 **
 **   Project:     X Widgets
 **
 **   Description: Public include file for StaticText class widgets
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 *****************************************************************************
 *************************************<+>*************************************/

#ifndef _XtStaticText_h
#define _XtStaticText_h

#include <Xol/Primitive.h>		/* include superclasses' header */

#define	StaticTextSelect	0
#define	StaticTextAdjust	1
#define	StaticTextEnd		2

/***********************************************************************
 *
 * StaticText Widget
 *
 ***********************************************************************/

extern WidgetClass staticTextWidgetClass;

typedef struct _StaticTextClassRec	*StaticTextWidgetClass;
typedef struct _StaticTextRec     	*StaticTextWidget;

#endif
/* DON'T ADD STUFF AFTER THIS */
