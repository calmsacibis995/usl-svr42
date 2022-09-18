/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/alloc.c	1.2"
#include "cfront.h"
#include "size.h"
#include <malloc.h>

int Nchunk = 0;

void print_free()
{
	fprintf(stderr, "free store: %d bytes alloc()=%d free()=%d\n",
		Nfree_store, Nalloc, Nfree);
	fprintf(stderr, "%d chunks: %d(%d)\n", Nchunk, CHUNK, Nchunk * CHUNK);
}


/* get memory that is not to be freed */
char *chunk(i)
int i;
{
	register char *cp;

	cp = (char *) malloc(i * CHUNK - 8);
	if (cp == 0){			/* no space */
		free(gtbl);		/* get space for error message */
		if (Nspy) print_free();
		errorFIPC('i', "free store exhausted", ea0, ea0, ea0, ea0);
	}

	Nchunk += i;
	Nfree_store += (i * CHUNK);
	return cp;
}

/* get memory that might be freed */
char *new(sz)
int sz;
{
	char *p;

	p = (char *)calloc(sz, 1);

	if (p == 0){			/* no space */
		free(gtbl);		/* get space for error message */
		if (Nspy) print_free();
		errorFIPC('i', "free store exhausted", ea0, ea0, ea0, ea0);
	}

	Nalloc++;
	Nfree_store += (sz + sizeof(int *));
	return p;
}

int NFn = 0, NFtn = 0, NFbt = 0, NFpv = 0, NFf = 0, NFe = 0, NFs = 0, NFc = 0;

void delete(p)
char *p;
{
	if (p == 0)return;

	if (Nspy){
		Pname pp;
		TOK t;

		pp = (Pname)p;
		t = pp->base;
		Nfree++;
		Nfree_store -= ((int *)p)[-1] -(int)p - 1 + sizeof(int *);
		switch (t){	/* can be fooled by character strings */
		case INT: case CHAR: case TYPE: case VOID: case SHORT: case LONG:
		case FLOAT: case DOUBLE: case COBJ: case EOBJ: case FIELD:
			NFbt++;
			break;

		case PTR: case VEC:
			NFpv++;
			break;

		case FCT:
			NFf++;
			break;

		case ICON: case CCON: case STRING: case FCON: case THIS:
			NFc++;
			break;
		}
	}
	free(p);
}
