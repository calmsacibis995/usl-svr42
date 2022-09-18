/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/validate.h	1.3"

/* $XConsortium: validate.h,v 5.2 90/03/16 17:17:01 keith Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology
*/

#ifndef VALIDATE_H
#define VALIDATE_H

#include "miscstruct.h"
#include "regionstr.h"

typedef enum { VTOther, VTStack, VTMove, VTUnmap, VTMap } VTKind;

typedef union _Validate {
    struct BeforeValidate {
	DDXPointRec	oldAbsCorner;	/* old window position */
	RegionPtr	borderVisible;	/* visible region of border, */
					/* non-null when size changes */
	Bool		resized;	/* unclipped winSize has changed - */
					/* don't call SaveDoomedAreas */
    } before;
    struct AfterValidate {
	RegionRec	exposed;	/* exposed regions, absolute pos */
	RegionRec	borderExposed;
    } after;
} ValidateRec, *ValidatePtr;

#define UnmapValData ((ValidatePtr)1)

#endif /* VALIDATE_H */
