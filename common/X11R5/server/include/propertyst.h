/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/propertyst.h	1.3"

/* $XConsortium: propertyst.h,v 1.2 88/09/06 15:49:12 jim Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#ifndef PROPERTYSTRUCT_H
#define PROPERTYSTRUCT_H 
#include "misc.h"
#include "property.h"
/* 
 *   PROPERTY -- property element
 */

typedef struct _Property {
        struct _Property       *next;
	ATOM 		propertyName;
	ATOM		type;       /* ignored by server */
	short		format;     /* format of data for swapping - 8,16,32 */
	long		size;       /* size of data in (format/8) bytes */
	pointer         data;       /* private to client */
} PropertyRec;

extern int ProcGetProperty();
extern int ProcListProperties();
extern int ProcChangeProperty();

#endif /* PROPERTYSTRUCT_H */

