/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/lib/new/_new.c	1.2"

typedef void (*PFVV)();

extern PFVV _new_handler;

extern char* _new();

char* _new(size)
long size;
{
	extern char* malloc();
	char* p;

	while ( (p=malloc((unsigned)size))==0 ) {
		if(_new_handler)
			(*_new_handler)();
		else
			return 0;
	}
	return p;
}
