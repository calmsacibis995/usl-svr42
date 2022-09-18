/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ifndef RVALUE_H
#define RVALUE_H
#ident	"@(#)debugger:inc/common/Rvalue.h	1.4"

#include "Vector.h"
#include "TYPE.h"
#include "Itype.h"
#include "Language.h"

class Obj_info;
class LWP;

#define DEBUGGER_FORMAT 'z'
#define DEBUGGER_BRIEF_FORMAT ((char) -1)
#define PRINT_SIZE	1024	// max width of single formatted print
				// expression
class Rvalue
{
	Vector  raw_bytes;
	TYPE    _type;
	LWP	*_lwp;

    public:
	Rvalue() {}
	Rvalue(void *, int, TYPE&, LWP *lwp=0);
	Rvalue(Stype, Itype&, LWP *lwp=0);
	Rvalue(Rvalue&);
	~Rvalue() {}

	Rvalue& operator=(Rvalue&);
	void reinit(void *, int, TYPE&, LWP *lwp=0); // faster than operator=
	int operator==(Rvalue&);
	int operator!=(Rvalue& v)
		{ return !(*this == v); }
	int assign(Obj_info&, Language);

	void null() 
		{ raw_bytes.clear(); _type.null(); _lwp = 0;}
	int  isnull()
		{ return raw_bytes.size() == 0;   }

	Stype get_Itype(Itype&);  // SINVALID if can't get as Itype member.
	void *dataPtr()
		{ return raw_bytes.ptr(); }
	TYPE& type()
		{ return _type; }
	void  set_type(TYPE &t)
		{ _type = t; }
	int convert(TYPE&, Language);

	char *print(LWP * lwp = 0, char format = 0,
		char *format_str = 0);

};

#endif
