/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/osstruct.h	1.3"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

/* $XConsortium: osstruct.h,v 1.7 88/09/06 15:49:01 jim Exp $ */

#ifndef OSSTRUCT_H
#define OSSTRUCT_H

#include "os.h"
#include "misc.h"

typedef struct _FontPathRec {
    int npaths;		/* number of valid paths */
    int size;		/* how big length and paths arrays are */
    int *length;
    char **paths;
    pointer *osPrivate;
} FontPathRec;

#endif /* OSSTRUCT_H */
