/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/resource.h	1.3"

/* $XConsortium: resource.h,v 1.11 90/03/27 15:39:20 rws Exp $ */
/***********************************************************
Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#ifndef RESOURCE_H
#define RESOURCE_H 1
#include "misc.h"

/*****************************************************************
 * STUFF FOR RESOURCES 
 *****************************************************************/

/* classes for Resource routines */

typedef unsigned long RESTYPE;

#define RC_VANILLA	((RESTYPE)0)
#define RC_CACHED	((RESTYPE)1<<31)
#define RC_DRAWABLE	((RESTYPE)1<<30)
#define RC_LASTPREDEF	RC_DRAWABLE
#define RC_ANY		(~(RESTYPE)0)

/* types for Resource routines */

#define RT_WINDOW	((RESTYPE)1|RC_CACHED|RC_DRAWABLE)
#define RT_PIXMAP	((RESTYPE)2|RC_CACHED|RC_DRAWABLE)
#define RT_GC		((RESTYPE)3|RC_CACHED)
#define RT_FONT		((RESTYPE)4)
#define RT_CURSOR	((RESTYPE)5)
#define RT_COLORMAP	((RESTYPE)6)
#define RT_CMAPENTRY	((RESTYPE)7)
#define RT_OTHERCLIENT	((RESTYPE)8)
#define RT_PASSIVEGRAB	((RESTYPE)9)
#define RT_LASTPREDEF	RT_PASSIVEGRAB
#define RT_NONE		((RESTYPE)0)

/* bits and fields within a resource id */
#define CLIENTOFFSET 22					/* client field */
#define RESOURCE_ID_MASK	0x3FFFFF		/* low 22 bits */
#define CLIENT_BITS(id) (((unsigned long) id) & 0x1fc00000)		/* hi 7 bits */
#define CLIENT_ID(id) ((unsigned int)(CLIENT_BITS(id) >> CLIENTOFFSET))
#define SERVER_BIT		0x20000000		/* use illegal bit */

/* Invalid resource id */
#define INVALID	(0)

#define BAD_RESOURCE 0xe0000000

RESTYPE CreateNewResourceType();
RESTYPE CreateNewResourceClass();
unsigned long FakeClientID();
Bool AddResource();
void FreeResource();
void FreeClientResources();
Bool LegalNewID();
pointer LookupIDByType();
pointer LookupIDByClass();

#endif /* RESOURCE_H */
