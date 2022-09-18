/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/miscstruct.h	1.3"

/* $XConsortium: miscstruct.h,v 5.2 89/06/09 17:54:23 keith Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#ifndef MISCSTRUCT_H
#define MISCSTRUCT_H 1

#include "misc.h"

extern unsigned long globalSerialNumber;
typedef struct _DDXPoint {
    short x, y;
} DDXPointRec;

typedef struct _Box {
    short x1, y1, x2, y2;
} BoxRec;

typedef union _DevUnion {
    pointer		ptr;
    long		val;
    unsigned long	uval;
} DevUnion;

#endif /* MISCSTRUCT_H */
