/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PtyList_h
#define _PtyList_h
#ident	"@(#)debugger:inc/common/PtyList.h	1.1"

//
// Header for Linked list of Pseudo Terminal records.
//

#include "Link.h"
#include <stdio.h>

class PtyInfo : public Link {
	int	count;
	int	pty; 		   // pty file descriptor
	char	*_name;
public:
		PtyInfo();
		~PtyInfo();

	// Access functions
	int	pt_fd()		{ return pty; }
	int	refcount()	{ return count; }
	char	*name()		{ return _name; }
	void	bump_count()	{ count++; }
	int	dec_count()	{ count--; return count; }
	int	is_null()	{ return (pty < 0); }
};

extern PtyInfo	*first_pty;

#endif // _PtyList_h
