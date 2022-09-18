/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/selection.h	1.3"

/* $XConsortium: selection.h,v 1.2 88/09/06 15:48:36 jim Exp $ */
#ifndef SELECTION_H
#define SELECTION_H 1

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#include "dixstruct.h"
/*
 *
 *  Selection data structures 
 */

typedef struct _Selection {
    Atom selection;
    TimeStamp lastTimeChanged;
    Window window;
    WindowPtr pWin;
    ClientPtr client;
} Selection;

#endif /* SELECTION_H */


