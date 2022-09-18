/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Wordstack_h
#define Wordstack_h
#ident	"@(#)debugger:inc/common/Wordstack.h	1.1"

#include	"Vector.h"

class Wordstack {
	Vector		vector;
	int		count;
public:
			Wordstack()	{	count = 0;	}
			~Wordstack()	{}
	void		push( unsigned long );
	unsigned long	pop();
	int		not_empty()	{	return count>0;	}
	void		clear()		{	vector.clear(); count = 0; }
};

#endif	/* Wordstack_h */
