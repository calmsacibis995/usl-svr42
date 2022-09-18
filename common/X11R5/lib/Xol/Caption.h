/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)caption:Caption.h	2.1"
#endif

#ifndef _CAPTION_H
#define _CAPTION_H

#include "Xol/Manager.h"

extern WidgetClass			captionWidgetClass;

typedef struct _CaptionClassRec *	CaptionWidgetClass;
typedef struct _CaptionRec *		CaptionWidget;

#endif