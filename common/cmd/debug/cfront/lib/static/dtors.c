/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/lib/static/dtors.c	1.1"

typedef void (*PFV)();

void dtors()
{
	extern PFV _dtors[];
	static ddone;
	if (ddone == 0) {	/* once only */
		PFV *pf;
		ddone = 1;
		pf = _dtors;
		while (*pf) pf++;
		while (_dtors < pf) (**--pf)();
	}
}
