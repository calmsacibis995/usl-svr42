/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libexp/common/Const.h	1.2"

#include "Itype.h"

enum ConstKind {
    CK_CHAR, CK_INT, CK_UINT, CK_FLOAT, CK_XFLOAT
};
//MORE: refine ConstKind to handle overload resolution for C++.

struct Const {
    ConstKind const_kind;
    union {
	char          c;
	long          l;
	unsigned long ul;
	double        d;
	Ixfloat	      x;
    };

    Const& init(ConstKind, char*);
};
