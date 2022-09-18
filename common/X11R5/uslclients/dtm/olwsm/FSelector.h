/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/FSelector.h	1.3"
#endif

#ifndef _OL_FSELECTOR_H
#define _OL_FSELECTOR_H

#include <Xol/FButtons.h>	/* superclass' header */

extern char		XtNchipHeight [];
extern char		XtNchipWidth  [];
extern char		XtNchipSpace  [];
extern char		XtNchipColor  [];

extern char		XtCChipHeight [];
extern char		XtCChipWidth  [];
extern char		XtCChipSpace  [];
extern char		XtCChipColor  [];

extern WidgetClass			flatSelectorWidgetClass;

typedef struct _FlatSelectorClassRec *	FlatSelectorWidgetClass;
typedef struct _FlatSelectorRec *	FlatSelectorWidget;

#endif /* _OL_FSELECTOR_H */
