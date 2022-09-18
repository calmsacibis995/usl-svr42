/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/lib/static/_main.c	1.2"

extern void _main();

void _main()
{
	typedef void (*PFV)();
	extern PFV _ctors[];
	static PFV *pf;
	for (pf=_ctors; *pf; pf++) {
		(**pf)();
		*pf = 0; /* permits main to be called recursively */
	}
}
