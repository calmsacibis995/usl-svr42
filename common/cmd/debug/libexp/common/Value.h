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

#ifndef VALUE_H
#define VALUE_H

#ident	"@(#)debugger:libexp/common/Value.h	1.6"

#include "Language.h"	// enum Language;
#include "Place.h"
#include "Rvalue.h"
#include "Symbol.h"

class Frame;
class LWP;

// Buff provides a dynamically sized array
class Buff
{
	char	*basep;
	char	*nextch;
	int	length;
public:
		Buff() { basep = nextch = 0; length = 0;}

	void	min_size(int n)
		{	
			if (n < 100)	// minimum malloc amount
				n = 100;
			if (length < n)
			{
				delete basep;
				basep = new char[length = n];
			}
			nextch = basep;
		}
	void	reset() { nextch = basep; }
	char	*ptr() { return basep; }
	int	size() { return nextch - basep; }
	void	mark_end() { *nextch = '\0'; }
	void	save(int ch) { *nextch++ = ch; } // no overflow check required,
						// handled by min_size
};

struct Obj_info
{
	LWP	*lwp;
	Frame   *frame;
	TYPE    type;
	Symbol  entry;
	Place   loc;

		Obj_info();
		Obj_info(LWP *p, Frame *f, TYPE& t, Symbol& s, Place& l);
		Obj_info(LWP *p, TYPE& t, Iaddr& addr);
		Obj_info(Obj_info& obj) { *this = obj; }
		Obj_info& operator=(Obj_info& rhs)
		{
			lwp = rhs.lwp;
			frame   = rhs.frame;
			type    = rhs.type;
			entry   = rhs.entry;
			loc     = rhs.loc;
			return *this;
		}
		~Obj_info() {}

	void	null();
	int	isnull();
	int	get_rvalue(Rvalue&);
	int	init(LWP *, Frame *, Symbol&, Iaddr = 0);
	int	init(LWP *, Frame *, Iaddr, TYPE&);
	int	extractBitFldValue(Buff &buff, int size); // Machine Dependent
	int	insertBitFldValue(Rvalue &newRval);
	int	truncateBitFldValue(Rvalue &newRval);
	int	getBitFldDesc( int *bitFldSz, int *bitFldOffset );
};

class Value
{
	Obj_info	_obj;
	Rvalue		_val;
public:
			Value() {}
			Value(Obj_info& obj) { _obj = obj; }
			Value(Rvalue& r) { _val = r; }
			Value(Obj_info& obj, Rvalue& r) { _obj = obj; 
				_val = r; }
			Value(Value& v) { _obj = v._obj; _val = v._val; }
			~Value() {}

	Value&		operator=(Value& v)
			{
				_obj = v._obj;
				_val = v._val;
				return *this;
			}
	Value&		operator=(Obj_info& obj)
			{	
				_obj = obj;
				_val.null();
				return *this;
			}
	Value&		operator=(Rvalue& r)
			{
				_obj.null();
				_val = r;
				return *this;
			}
	Obj_info&	object() { return _obj; }
	int		rvalue(Rvalue& rval)
			{
				if (_val.isnull())
					_obj.get_rvalue(_val);
				rval = _val;
				return ! rval.isnull();
			}
	int		get_rvalue() { return !_val.isnull() || 
				_obj.get_rvalue(_val); }

	int		deref(LWP *, Frame *, TYPE * = 0);
	int		assign(Obj_info&, Language);
};

#endif /*VALUE_H*/
