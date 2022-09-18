/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/__ctdt.c	1.1"
typedef int (*PFV)();
int _STI__src_norm2_c();
extern PFV _ctors[];
PFV _ctors[] = {
	_STI__src_norm2_c,
	0
};
int _STD__src_norm2_c();
extern PFV _dtors[];
PFV _dtors[] = {
	_STD__src_norm2_c,
	0
};
