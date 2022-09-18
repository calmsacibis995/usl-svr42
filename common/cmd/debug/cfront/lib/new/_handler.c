/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/lib/new/_handler.c	1.1"

typedef void (*PFVV)();

extern PFVV _new_handler;
extern PFVV set_new_handler();

PFVV _new_handler = 0;

PFVV set_new_handler(handler)
PFVV handler;
{
	PFVV rr;
	rr = _new_handler;
	_new_handler = handler;
	return rr;
}
