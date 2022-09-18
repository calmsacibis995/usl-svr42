/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/list.c	1.7"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/memutil.h>

#include "misc.h"
#include "list.h"

#define SIZE(p, n)		((p)->size * (n))
#define ELEM(p, i)		((p)->entry + SIZE((p), (i)))
#define ALLOC(n)		(ADDR)MALLOC((n))

List *
alloc_List(size)
	int			size;
{
	List *			new = NULL;

	if (size > 0) {
		new = ELEMENT(List);
		new->entry = NULL;
		new->size = size;
		new->count = 0;
		new->max = 0;
	}
	return new;
}

void
free_List(p)
	List *			p;
{
	if (p) {
		if (p->entry) {
			FREE(p->entry);
		}
		FREE(p);
	}
}

void
compress_List(p)
	List *			p;
{
	int			n;
	ADDR			entry = NULL;

	if (p) {
		if (p->count < p->max) {
			if ((n = SIZE(p, p->count)) > 0) {
				entry = ALLOC(n);
				BCOPY(p->entry, entry, n);
			}
			FREE(p->entry);
			p->entry = entry;
			p->max = p->count;
		}
	}
}

void
expand_List(p, count)
	List *			p;
	int			count;
{
	int			n;
	ADDR			entry;

	if (p && count > 0) {
		if (p->count + count > p->max) {
			p->max = p->count + count;
			entry = ALLOC(SIZE(p, p->max));

			if ((n = SIZE(p, p->count)) > 0) {
				BCOPY(p->entry, entry, n);
			}
			if (p->entry) {
				FREE(p->entry);
			}
			p->entry = entry;
		}
	}
}

void
list_insert(p, i, entry, count)
	List *			p;
	int			i;
	ADDR			entry;
	int			count;
{
	int			n;
	ADDR			q;

	if ((p == NULL)
	||  (i < 0 || i > p->count)
	||  (entry == NULL)
	||  (count <= 0)) {
		return;
	}
	if (p->count + count > p->max) {
		p->max += MAX(p->max, count);
		q = ALLOC(SIZE(p, p->max));

		if ((n = SIZE(p, p->count)) > 0) {
			BCOPY(p->entry, q, n);
		}
		if (p->entry) {
			FREE(p->entry);
		}
		p->entry = q;
	}
	if ((n = SIZE(p, p->count - i)) > 0) {
		BCOPY(ELEM(p, i), ELEM(p, i + count), n);
	}
	p->count += count;
	BCOPY(entry, ELEM(p, i), SIZE(p, count));
}

void
list_delete(p, i, count)
	List *			p;
	int			i;
	int			count;
{
	int			n;

	if ((p == NULL)
	||  (i < 0 || i + count > p->count)
	||  (count <= 0)) {
		return;
	}
	if ((n = SIZE(p, p->count - (i + count))) > 0) {
		BCOPY(ELEM(p, i + count), ELEM(p, i), n);
	}
	p->count -= count;
}

void
list_sort(p, f)
	List *			p;
	int			(*f)();
{
	if (p && f && p->count) {
		qsort((char *)p->entry, p->count, p->size, f);
	}
}

ADDR
list_search(p, entry, f)
	List *			p;
	ADDR			entry;
	int			(*f)();
{
	if (p && entry && f && p->count) {
		return (ADDR)bsearch(
			(char*)entry, (char*)p->entry, p->count, p->size, f
		);
	}
	return NULL;
}

list_ITERATOR
list_iterator(p)
	List *			p;
{
	list_ITERATOR		I;

	I.min = ELEM(p, 0);
	I.max = ELEM(p, p->count);
	I.next = I.min;
	I.prev = I.max;
	I.size = p->size;
	return I;
}
