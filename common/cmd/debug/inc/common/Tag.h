/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Tag_h
#define Tag_h
#ident	"@(#)debugger:inc/common/Tag.h	1.1"

#undef DEFTAG
#define DEFTAG(VAL, NAME) VAL,

enum Tag {
#include "Tag1.h"
};

#define	IS_ENTRY(x) ((x) == t_global_sub||(x) == t_subroutine||(x) == t_entry)
#define IS_VARIABLE(x)	((x) == t_global_variable || (x) == t_local_variable \
			|| (x) == t_argument)

#endif
