/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/lib/new/_vec.c	1.2"

typedef char* PV;
typedef char (*PF)();
typedef char (*PFI)();

/*
 *	allocate a vector of "n" elements of size "sz"
 *	and initialize each by a call of "f"
 */
PV _vec_new(op, n, sz, f)
PV op; int n; int sz; PV f;
{
	register char* p;
	register int i;
	if (op == 0) op = (PV) _new((long)(n*sz));
	p = op;
	if (f) for (i=0; i<n; i++) (*(PF)f)( (PV)(p+i*sz) );
	return (PV)p;
}

void _vec_delete(op, n, sz, f, del)
PV op; int n; int sz; PV f; int del;
{
	if (op == 0) return;
	if (f) {
		register char* p;
		register int i;
		p = op;
		for (i=0; i<n; i++) (*(PFI)f)( (PV)(p+i*sz), 0 );
	}
	if (del) _delete(op);
}
