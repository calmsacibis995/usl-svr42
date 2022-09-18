/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olhelp:Help.h	1.6"
#endif

#ifndef _Ol_Help_h_
#define _Ol_Help_h_

/*
 *************************************************************************
 *
 * Description:
 *		This is the "public" include file for the Help Widget
 *
 *****************************file*header********************************
 */

#include <Xol/ControlAre.h>

typedef struct _HelpClassRec *HelpWidgetClass;
typedef struct _HelpRec      *HelpWidget;

extern WidgetClass helpWidgetClass;
#endif /* _Ol_Help_h_ */
