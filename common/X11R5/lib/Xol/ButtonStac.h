/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)buttonstack:ButtonStac.h	1.11"
#endif

#ifndef _Ol_ButtonStac_h_
#define _Ol_ButtonStac_h_

/*************************************************************************
 *
 * Description:
 *		This is the "public" include file for the ButtonStack
 *	Widget and Gadget.
 *
 *****************************file*header********************************/

/***********************************************************************
 *
 * ButtonStack Widget
 *
 ***********************************************************************/

#include <Xol/MenuButton.h>

extern WidgetClass				buttonStackWidgetClass;
typedef struct _MenuButtonClassRec *		ButtonStackWidgetClass;
typedef struct _MenuButtonRec *			ButtonStackWidget;

extern WidgetClass				buttonStackGadgetClass;
typedef struct _MenuButtonGadgetClassRec *	ButtonStackGadgetClass;
typedef struct _MenuButtonGadgetRec *		ButtonStackGadget;

#endif /* _Ol_ButtonStac_h_ */
