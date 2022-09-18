/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xsol:XMakeAssoc.c	1.1"
#include "copyright.h"

/* $XConsortium: XMakeAssoc.c,v 10.17 88/09/06 16:09:41 jim Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#include "Xlibint.h"
#include "X10.h"

void insque();
struct qelem {
	struct    qelem *q_forw;
	struct    qelem *q_back;
	char q_data[1];
};
/*
 * XMakeAssoc - Insert data into an XAssocTable keyed on an XId.
 * Data is inserted into the table only once.  Redundant inserts are
 * meaningless (but cause no problems).  The queue in each association
 * bucket is sorted (lowest XId to highest XId).
 */
XMakeAssoc(dpy, table, x_id, data)
	register Display *dpy;
	register XAssocTable *table;
	register XID x_id;
	register caddr_t data;
{
	int hash;
	register XAssoc *bucket;
	register XAssoc *Entry;
	register XAssoc *new_entry;
	
	/* Hash the XId to get the bucket number. */
	hash = x_id & (table->size - 1);
	/* Look up the bucket to get the entries in that bucket. */
	bucket = &table->buckets[hash];
	/* Get the first entry in the bucket. */
	Entry = bucket->next;

	/* If (Entry != bucket), the bucket is empty so make */
	/* the new entry the first entry in the bucket. */
	/* if (Entry == bucket), the we have to search the */
	/* bucket. */
	if (Entry != bucket) {
		/* The bucket isn't empty, begin searching. */
		/* If we leave the for loop then we have either passed */
		/* where the entry should be or hit the end of the bucket. */
		/* In either case we should then insert the new entry */
		/* before the current value of "Entry". */
		for (; Entry != bucket; Entry = Entry->next) {
			if (Entry->x_id == x_id) {
				/* Entry has the same XId... */
				if (Entry->display == dpy) {
					/* Entry has the same Display... */
					/* Therefore there is already an */
					/* entry with this XId and Display, */
					/* reset its data value and return. */
					Entry->data = data;
					return;
				}
				/* We found an association with the right */
				/* id but the wrong display! */
				continue;
			}
			/* If the current entry's XId is greater than the */
			/* XId of the entry to be inserted then we have */
			/* passed the location where the new XId should */
			/* be inserted. */
			if (Entry->x_id > x_id) break;
		}
        }

	/* If we are here then the new entry should be inserted just */
	/* before the current value of "Entry". */
	/* Create a new XAssoc and load it with new provided data. */
	new_entry = (XAssoc *)Xmalloc(sizeof(XAssoc));
	if (new_entry == NULL) {
		/* Malloc failed! */
		errno = ENOMEM;
		(*_XIOErrorFunction)(dpy);
	}
	new_entry->display = dpy;
	new_entry->x_id = x_id;
	new_entry->data = data;

	/* Insert the new entry. */
	insque((struct qelem *)new_entry, (struct qelem *)Entry->prev);
}

