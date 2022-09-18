/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:common/arena.c	1.5"
#include "arena.h"
#include "manifest.h"
#include <malloc.h>
#ifdef NODBG
#ifndef NDEBUG
#define NDEBUG /* for assert */
#endif
#endif
#include <assert.h>

#define MALLOC(cnt,type,p) \
	{ \
	p = (type *)malloc(cnt *  sizeof(type)); \
	if (!p) \
		cerror("storage failure"); \
	}


#define DEFAULT_ALLOC 8192
#define DEFAULT_THRESH DEFAULT_ALLOC/4
#define EXTENTS 500
#define MAGIC -987654321

struct Extent {
	char *extent; /* block of storage */
	struct Extent *next_extent;
};

struct Arena_struct {
	char * curr_byte;
	char * last_byte;
	struct Extent *extent_list;	
	int magic;
};

#ifndef NODBG
	static extent_count, high_water;
int
get_high_water()
{
	return(high_water);
}

void
init_high_water()
{
	high_water = 0;
}
#endif

struct Arena_struct *
arena_init()
{
	struct Arena_struct *a;
	MALLOC(1,struct Arena_struct, a)
	a->extent_list = 0;
	a->magic = MAGIC;
	a->curr_byte = a->last_byte=0;
	return a;
}
void
arena_term(a)
struct Arena_struct * a;
{
	assert(a->magic == MAGIC);
	while(a->extent_list) {
		struct Extent *e = a->extent_list;
		free((myVOID *)e->extent);
		a->extent_list = e->next_extent;
#ifndef NODBG
		extent_count--;
#endif
		free((myVOID *)e);
	}
	free((myVOID *)a);
}

static struct  Align_s {
	char x;

	union Align_u  {
		short s;
		long l;		

#ifdef __STDC__
		long double ld;
		long double *ldp;
#endif
		double d;
		double *dp;
		float f;
	

		/* assume every pointer type has alignment no stricter
		   then one of the following.. May not be the case in
		   general, but is the case in practice
		*/
		char *cp;
		short *sp;
		long *lp;
		float *fp;
		int (*fup)();
		char **cpp;
	} u;
};
#define ALLOC_ALIGN (sizeof(struct Align_s) - sizeof (union Align_u) )
		

myVOID *
arena_alloc(a,size)
struct Arena_struct *a;
int size;
{
	char *p;
	int alloc_size;
	struct Extent *new_extent;
	size = ((size+ALLOC_ALIGN - 1 ) / ALLOC_ALIGN ) * ALLOC_ALIGN;
	assert(a->magic == MAGIC);
	if (a->curr_byte + size < a->last_byte) {
		p = a->curr_byte;
		a->curr_byte += size;
		return (myVOID *)p;
	}
		/* Need new extent */
	if (size < DEFAULT_THRESH) 
		alloc_size = DEFAULT_ALLOC;
	else
		alloc_size = size;
#ifndef NODBG
	++extent_count;
	if(extent_count > high_water) high_water = extent_count;
#endif
	MALLOC(alloc_size, char, p)
	MALLOC(1, struct Extent, new_extent);
	new_extent->extent = p;
	new_extent->next_extent = a->extent_list;
	a->extent_list = new_extent;
	a->curr_byte = p+size;
	a->last_byte = p+alloc_size;
	return (myVOID *)p;
}
