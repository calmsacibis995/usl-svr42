/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Vector_h
#define Vector_h
#ident	"@(#)debugger:inc/common/Vector.h	1.1"

#include <stdlib.h>

class Vector {
	size_t		total_bytes;
	size_t		bytes_used;
	void *		vector;
	void		getmemory(size_t);
	void		check();
public:
			Vector()		{ bytes_used = 0;
						  total_bytes = 0;
						  vector = 0; }
			Vector(Vector &);
			~Vector()		{ if ( vector ) free(vector); }
	Vector &	add(void *, size_t);
	Vector &	drop(size_t i)		{ if ( i <= bytes_used )
							bytes_used -= i;
						  return *this; }
	void *		ptr()			{ return vector;	}
	size_t		size()			{ return bytes_used;	}
	Vector &	operator= (Vector&);
	Vector &	clear()			{ bytes_used = 0; return *this;	}
#ifdef DEBUG
	Vector &	report(char * = 0);
#endif
};

#endif /* Vector_h */
