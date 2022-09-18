/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xsol:XCrAssoc.c	1.1"
#include "copyright.h"

/* $XConsortium: XCrAssoc.c,v 10.16 88/09/06 16:09:23 jim Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#include "Xlibint.h"
#include "X10.h"

/*
 * XCreateAssocTable - Create an XAssocTable.  The size argument should be
 * a power of two for efficiency reasons.  Some size suggestions: use 32
 * buckets per 100 objects;  a reasonable maximum number of object per
 * buckets is 8.  If there is an error creating the XAssocTable, a NULL
 * pointer is returned.
 */
XAssocTable *XCreateAssocTable(size)
	register int size;		/* Desired size of the table. */
{
	register XAssocTable *table;	/* XAssocTable to be initialized. */
	register XAssoc *buckets;	/* Pointer to the first bucket in */
					/* the bucket array. */
	
	/* XMalloc the XAssocTable. */
	if ((table = (XAssocTable *)Xmalloc(sizeof(XAssocTable))) == NULL) {
		/* XMalloc call failed! */
		errno = ENOMEM;
		return(NULL);
	}
	
	/* XMalloc the buckets (actually just their headers). */
	buckets = (XAssoc *)Xcalloc((unsigned)size, (unsigned)sizeof(XAssoc));
	if (buckets == NULL) {
		/* XCalloc call failed! */
		errno = ENOMEM;
		return(NULL);
	}

	/* Insert table data into the XAssocTable structure. */
	table->buckets = buckets;
	table->size = size;

	while (--size >= 0) {
		/* Initialize each bucket. */
		buckets->prev = buckets;
		buckets->next = buckets;
		buckets++;
	}

	return(table);
}
