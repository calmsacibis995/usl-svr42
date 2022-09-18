/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:Constraint.h	1.1"
/* $XConsortium: Constraint.h,v 1.9 89/06/16 18:08:57 jim Exp $ */
/* $oHeader: Constraint.h,v 1.2 88/08/18 15:54:18 asente Exp $ */
/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtConstraint_h
#define _XtConstraint_h

typedef struct _ConstraintClassRec *ConstraintWidgetClass;

#ifndef CONSTRAINT
externalref WidgetClass constraintWidgetClass;
#endif

#endif /* _XtConstraint_h */
/* DON'T ADD STUFF AFTER THIS #endif */
