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
#ifndef TYPE_h
#define TYPE_h
#ident	"@(#)debugger:inc/common/TYPE.h	1.3"

#include "Symbol.h"
#include "Itype.h"
#include "Fund_type.h"
#include "Attribute.h"

class	Frame;
class	LWP;
class	Place;
class	IntList;

enum Type_form
{
	TF_fund,  // char, short, int, unsigned int, ...
	TF_user   // ptr, array, struct, enum, ...
};

// user-defined TYPES may be built as the result of cast or SIZEOF
// operators;  the attribute lists created are never deleted.
// This should be fixed.
//
// C/C++ routines should be moved to a sub-class derived from the
// base TYPE class

class TYPE
{
	Type_form	_form;
	Fund_type	ft;     // meaningful iff form == TF_fund.
	Symbol		symbol; // meaningful iff form == TF_user.

    public:
			TYPE()  { null(); }
			TYPE(const TYPE&);	
			// bit copy, needed because of cfront bug
			TYPE(Fund_type fundtype) { _form = TF_fund; 
				ft = fundtype; }
			~TYPE() {}

	void		null()  { _form = TF_fund; ft = ft_none; } // make null.
	int		isnull() { return _form == TF_fund && ft == ft_none; }

	TYPE&		operator=(const TYPE&); // bit copy 
				// needed because of cfront bug
	TYPE&		operator=(Fund_type);	// init is a fundamental type.
	TYPE&		operator=(Symbol&);	// init as a user defined type.
	int		operator==(TYPE&);
	int		operator!=(TYPE& t) { return !(this->operator==(t)); }

	int		modifyUserType(Attribute *, LWP *proc=0);

	Type_form	form() { return _form; }
	int		fund_type(Fund_type&);  // return 1 iff form TF_fund.
	int		user_type(Symbol&);     // return 1 iff form TF_user.
	
	int		deref_type(TYPE&, Tag * = 0);
	int		get_Stype(Stype& stype);

	int		size();			  // size in bytes
	int		get_bound(Attr_name, Place &, long &, LWP *, Frame *); 
				// array bounds

	// General type query rtns
	int		isClass()	{ return isStruct(); }
	int		isStruct()
				{ return (_form==TF_user && 
					symbol.tag()==t_structuretype); }
	int		isUnion()
				{ return (_form==TF_user && 
					symbol.tag()==t_uniontype); }
	int		isBitField()
				{ return (_form==TF_user && 
					symbol.tag()==t_bitfield); }
	int		isSubrtn()
				{ return (_form==TF_user && 
					symbol.isSubrtn()); }

	// C/C++ specific routines
	int		isBitFieldSigned();		// machine dependent
	int		isPointer(Fund_type* ft=0, Symbol* ut=0);
	int		isPtrType(Fund_type* ft=0, Symbol* ut=0);
	int		isArray(Symbol* ut=0);
	int		isIntegral(Fund_type* ft=0, Symbol* ut=0);
	int		isFloatType(Fund_type* ft=0);
	int		isArithmetic(Fund_type* ft=0, Symbol* ut=0);
	int		isScalar(Fund_type* ft=0, Symbol* ut=0);

	// member functions used to build types needed by debugger 
	// expression evaluation
	int		build_ptrToBase(TYPE &baseTYPE, LWP *proc);	
	int		build_ptrTYPE(TYPE &baseTYPE, LWP *proc);
				// if baseTYPE is array, build ptr to elemtype
	int		build_arrayTYPE(TYPE &baseTYPE, LWP *proc, IntList *subscr);
	int		build_subrtnTYPE(TYPE &baseTYPE, LWP *proc);

#ifdef DEBUG
	void dumpTYPE();
#endif
};

#ifdef DEBUG
    void dumpAttribute(Attribute *AttrPtr, int indent);
#endif

#endif
