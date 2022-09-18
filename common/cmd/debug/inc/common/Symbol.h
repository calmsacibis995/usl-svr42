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
#ifndef Symbol_h
#define Symbol_h
#ident	"@(#)debugger:inc/common/Symbol.h	1.3"

#include	"Attribute.h"
#include	"Iaddr.h"
#include	"Tag.h"

// class Symbol -- the representation of the entries in a Symtab.
//
// A Symbol is a 4-word structure consisting of three pointers and a long.
// It has no destructor, since the data pointed to is not "owned" by
// the Symbol and should not be destroyed when the Symbol is destroyed.
//
// Symbols are normally passed by value.
//
// The "namep" member is strictly an optimization; the most common operation
// performed on a Symbol is to ask its name.  Note that the comparison
// operator does NOT check for equality of namep's.
//
// The "pc" member function should be used to get the (relocated) virtual
// address for a static or external object.  The value of an attribute with
// form "af_addr" will be a relative address; it must have the ss_base added
// to it (if from a shared object) to make it a relocated address.

class	Evaluator;
class	TYPE;
class	Source;
class	Locdesc;

class Symbol {
	char *		namep;
	char *		mangledNamep;	// null if not C++
	Attribute *	attrlist;
	Evaluator *	evaluator;
	Iaddr		ss_base;
	friend class	Symtab;
	friend class	Symtable;
	friend class	TYPE;
public:
			Symbol()		{ null(); }
			Symbol ( Symbol &s )	{ *this = s; }

	Symbol &	operator=( Symbol &s );
	int		operator==( Symbol &s )	{
				return  attrlist  == s.attrlist &&
					evaluator == s.evaluator &&
					ss_base   == s.ss_base; }
	int		operator!=( Symbol &s )	{
				return  !(*this == s); }
	Symbol		arc( Attr_name );
	Tag		tag( const char * = 0 );
	Attribute *	attribute( Attr_name );
	char *		name();
	char *		mangledName();
	Iaddr		pc( Attr_name );
	int		source( Source & );
	int		type(TYPE&, Attr_name = an_type);
	int		locdesc(Locdesc&, Attr_name = an_location);
	void		null();
	int		isnull(){ return attrlist == 0; }
	void		dump( const char * = 0 );

	Iaddr		base()		{ 	return ss_base; 	}

	Symbol		parent()	{	return arc(an_parent);	}
	Symbol		child()		{	return arc(an_child);	}
	Symbol		sibling()	{	return arc(an_sibling);	}

	// Higher level abstractions (used to hide tags)

	int		isUserTypeSym(); // typedef, enum, union, struct, class
	int		isUserTagName(); // enum, union, struct, class
	int		isSourceFile()	{ return tag()==t_sourcefile; }
	int		isBlock()	{ return tag()==t_block; }
	int		isArgument()	{ return tag()==t_argument; }
	int		isUnspecArgs()	{ return tag()==t_unspecargs; }
	int		isGlobalVar()	{ return tag()==t_global_variable; }
	int		isGlobalSub()	{ return tag()==t_global_sub; }
	int		isSubrtn();
	int		isEntry();
	int		isVariable();
	int		isLabel()	{ return tag()==t_label; }
	int		isSubrtnType()	{ return tag()==t_functiontype; }
	int		isStructType()	{ return tag()==t_structuretype; }
	int		isClassType()	{ return isStructType(); }
	int		isUnionType()	{ return tag()==t_uniontype; }
	int		isArrayType()	{ return tag()==t_arraytype; }
	int		isEnumType()	{ return tag()==t_enumtype; }
	int		isEnumLitType()	{ return tag()==t_enumlittype; }
	int		isTypedef()	{ return tag()==t_typedef; }
	int		isMember();
	int		type_assumed(int true_only);	
			// returns 0 if not assumed,
			// 1 if assumed and this is the first time
			// we are seeing it, else > 1
			// if true_only is set, returns only
			// 0 for not assumed, 1 for assumed and
			// doesn't increment count for number of
			// times seen
};

#endif /* Symbol_h */
