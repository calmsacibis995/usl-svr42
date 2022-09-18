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
#ident	"@(#)debugger:libsymbol/common/TYPE.C	1.7"

#include "Fund_type.h"
#include "TYPE.h"
#include "Tag.h"
#include "Machine.h"
#include "Interface.h"
#include "LWP.h"
#include "Protorec.h"
#include "Locdesc.h"
#include "Place.h"
#include "Attribute.h"
#include "Parser.h"

TYPE::TYPE(const TYPE& t)
{
	_form = t._form;
	if (_form == TF_fund)
		ft = t.ft;
	else
		symbol = t.symbol;
}

TYPE&
TYPE::operator=(const TYPE& t)
{
	_form = t._form;
	if (_form == TF_fund)
		ft = t.ft;
	else
		symbol  = t.symbol;
	return *this;
}

TYPE&
TYPE::operator=(Fund_type ftype)
{
	_form = TF_fund;
	ft = ftype;
	return *this;
}

TYPE&
TYPE::operator=(Symbol& symb)
{
	_form = TF_user;
	symbol = symb;
	return *this;
}

int
TYPE::operator==(TYPE& t)
{
	if( _form == TF_fund )
	{
		return (this->ft == t.ft);
	}

	if( this->symbol == t.symbol )
		return 1;

	TYPE thisDrefType;
	TYPE tDrefType;
	if( this->deref_type(thisDrefType) && t.deref_type(tDrefType) )
		return thisDrefType==tDrefType;;

	return 0;
}

int
TYPE::fund_type(Fund_type& ftype)   // 'read' fundamental type.
{
	if (_form == TF_fund) 
	{
		ftype = ft;
		return 1;
	}
	ftype = ft_none;
	return 0;
}

int
TYPE::user_type(Symbol& symb) // 'read' user defined type.
{
	if (_form == TF_user) 
	{
		symb = symbol;
		return 1;
	}
	symb.null();
	return 0;
}

int
TYPE::modifyUserType(Attribute *typeAttr, LWP *proc)
{
	if( symbol.evaluator == 0 )
	{
		// get an evaluator for the user type 
		if( proc != 0 )
		{
			Symbol s = proc->first_file();
			if( s.isnull() )
				return 0;
			symbol.evaluator = s.evaluator;
		}
	}

	_form = TF_user;
	symbol.attrlist = typeAttr;
	return 1;
}

int
TYPE::size()
{
	if (isnull()) 
		return 0; // don't know.

	if (form() == TF_user) 
	{
		Attribute *a = symbol.attribute(an_bytesize);

		if (a != 0 && a->form == af_int) 
		{
			return a->value.word;
		}

		TYPE elemtype;
		symbol.type(elemtype, an_elemtype);
		if (!elemtype.isnull()) 
		{
			// MORE - this is really C/C++ specific
			int span = elemtype.size();
			a = symbol.attribute(an_lobound);
			if (a != 0 && a->form == af_int) 
			{
				int lo = (int)a->value.word;
				a = symbol.attribute(an_hibound);
				if (a != 0 && a->form == af_int) 
				{
					return span *
						(a->value.word - lo +1);
				}
			}
		}

		return 0;
	}

	switch (ft) 
	{ 
	case ft_char:
	case ft_schar:
	case ft_uchar:
		return CHAR_SIZE;
	case ft_short:
	case ft_sshort:
	case ft_ushort:
		return SHORT_SIZE;
	case ft_int:
	case ft_sint:
	case ft_uint:
		return INT_SIZE;
	case ft_long:
	case ft_slong:
	case ft_ulong:
		return LONG_SIZE;
	case ft_pointer:
		return PTR_SIZE;
	case ft_sfloat:
		return SFLOAT_SIZE;
	case ft_lfloat:
		return LFLOAT_SIZE;
	case ft_xfloat:
		return XFLOAT_SIZE;
	case ft_none:
	default:
		return 0;
	}
}


int
TYPE::deref_type(TYPE& dtype, Tag *tagp)	// ptr or array.
{
	if (form() != TF_user) 
		return 0;

	Tag		tag   = symbol.tag("TYPE::deref_type(TYPE&)");
	Attr_name	attr;

	if (tagp)
		*tagp = tag;

	switch (tag) 
	{
	case t_pointertype:
	attr = an_basetype;
		break;
	case t_arraytype:
		attr = an_elemtype;
		break;
	default:
		return 0;
	}
	return symbol.type(dtype, attr);
}

//===================================================================
// Type Building member functions
//===================================================================

// NOTE the next four routines will allocate an attribute list that 
// isn't freed (since attribute lists are never freed). 
// This will be fixed at a
// later date.   For now it shouldn't be too big problem because they 
// are small, they are only generated for cast and sizeof operators,
// and they  will be inaccessible after the containing etree node 
// is deleted.

int
TYPE::build_ptrToBase(TYPE &baseTYPE, LWP *proc)
{
	Fund_type	base_ft;
	Symbol		base_ut;
	Protorec	protoUT;	// proto new user type

	protoUT.add_attr(an_tag, af_tag, t_pointertype);

	if( baseTYPE.fund_type(base_ft) )
	{
		protoUT.add_attr(an_basetype, af_fundamental_type,
			base_ft);
	}
	else if( baseTYPE.user_type(base_ut) )
	{
		protoUT.add_attr(an_basetype, af_symbol, 
				(void *) base_ut.attribute(an_count));
	}
	else
	{
		printe(ERR_internal, E_ERROR, "TYPE::build_ptrProto", 
			__LINE__);
		return 0;
	}

	TYPE ptrTYPE = ft_pointer;
	protoUT.add_attr(an_bytesize, af_int, ptrTYPE.size());

	
	Attribute *newUT = protoUT.put_record();
	if( !modifyUserType(newUT, proc) )
	{
		printe(ERR_build_type, E_ERROR);
		return 0;
	}
	return 1;
}

int
TYPE::build_ptrTYPE(TYPE &ptrTYPE, LWP *proc)
{
	TYPE	baseTYPE;

	if( ptrTYPE.isPointer() )
	{
		*this = ptrTYPE;
	}
	else if( !ptrTYPE.isArray() || !ptrTYPE.deref_type(baseTYPE) ||
		 !build_ptrToBase(baseTYPE, proc) )
	{
		this->null();
	}
	
	return !this->isnull();
}

enum array_attrs 
{
	ar_elemtype = 0,
	ar_subtype,
	ar_lobound,
	ar_hibound,
	ar_tagtype
};

static void subscr_list( Attribute *, Attribute *, IntList *);

// build symbol for possibly multi-dimensional arrays
static void
next_item( Attribute * a, Attribute *elem, IntList *list )
{
	Attribute	attr[5];
	Protorec	prototype;
	
	if (list->next())
	{
		subscr_list( attr, elem, list );
		for(int i = 0; i < 5; i++)
			prototype.add_attr( (Attr_name)attr[i].name, 
				(Attr_form)attr[i].form, attr[i].value );
		a[ar_elemtype].value.symbol = prototype.put_record();
		a[ar_elemtype].form = af_symbol;
		a[ar_elemtype].name = an_elemtype;
	}
	else
	{
		a[ar_elemtype] = *elem;
	}
}

static void
subscr_list( Attribute * a, Attribute *elem, IntList *list )
{
	int		subscript;

	subscript = list->val();

	a[ar_subtype].name = an_subscrtype;
	a[ar_subtype].form = af_fundamental_type;
	a[ar_subtype].value.fund_type = ft_int;
	a[ar_lobound].name = an_lobound;
	a[ar_lobound].form = af_int;
	a[ar_lobound].value.word = 0;
	a[ar_hibound].name = an_hibound;
	a[ar_tagtype].value.tag = t_arraytype;
	a[ar_hibound].form = af_int;
	a[ar_hibound].value.word = subscript - 1;
	a[ar_tagtype].name = an_tag;
	a[ar_tagtype].form = af_tag;

	next_item( a, elem, list );
}

// MORE - this is really C/C++ specific
int
TYPE::build_arrayTYPE(TYPE &baseTYPE, LWP *proc, IntList *list)
{
	Fund_type	base_ft;
	Symbol		base_ut;
	Protorec	protoUT;	// proto new user type
	Attribute	elem_type;
	Attribute	attr[5];

	list->first();

	elem_type.name = an_elemtype;
	if( baseTYPE.fund_type(base_ft) )
	{
		elem_type.form = af_fundamental_type;
		elem_type.value.fund_type = base_ft;
	}
	else if( baseTYPE.user_type(base_ut) )
	{
		elem_type.form = af_symbol;
		elem_type.value.word = 
			(long)(void *)base_ut.attribute(an_count);
	}
	else
	{
		printe(ERR_internal, E_ERROR, "TYPE::build_arrayTYPE",
			__LINE__);
		return 0;
	}

	subscr_list( attr, &elem_type, list );
	for(int i = 0; i < 5; i++)
		protoUT.add_attr( (Attr_name)attr[i].name, 
			(Attr_form)attr[i].form, attr[i].value );

	Attribute *newUT = protoUT.put_record();

	// build Symbol and TYPE
	if( !modifyUserType(newUT, proc) )
	{
		printe(ERR_build_type, E_ERROR);
		return 0;
	}

	return 1;
}

int
TYPE::build_subrtnTYPE(TYPE &baseTYPE, LWP *proc)
{
	Fund_type	base_ft;
	Symbol		base_ut;
	Protorec	protoUT;	// proto new user type

	protoUT.add_attr(an_tag, af_tag, t_functiontype);
	if( baseTYPE.fund_type(base_ft) )
	{
		protoUT.add_attr(an_resulttype,af_fundamental_type,
			base_ft);
	}
	else if( baseTYPE.user_type(base_ut) )
	{
		protoUT.add_attr(an_resulttype, af_symbol, 
				(void *) base_ut.attribute(an_count));
	}
	else
	{
		printe(ERR_internal, E_ERROR, "TYPE::build_subrtnTYPE",
			__LINE__);
		return 0;
	}
	
	Attribute *newUT = protoUT.put_record();

	// build Symbol and TYPE
	if( !modifyUserType(newUT, proc) )
	{
		printe(ERR_build_type, E_ERROR);
		return 0;
	}

	return 1;
}


//====================================================================
// C/C++ specfic member functions
//====================================================================

// C type checking/classification routines

int
TYPE::isPointer(Fund_type* ftype, Symbol* ut)
{
	if (ut)
		*ut = symbol;
	if (ftype)
		*ftype = ft;
	if (_form == TF_user)
	{
		return (symbol.tag()==t_pointertype);
	}
	else if (_form == TF_fund)
	{
		return (ft == ft_pointer);
	}
	return 0;
}

//
// return true if array 
//
int
TYPE::isArray(Symbol* ut)
{
	if (ut)
		*ut = symbol;
	if (_form == TF_user && symbol.tag()==t_arraytype)
	{
		return 1;
	}
	return 0;
}

//
// return true if pointer or array name
//
int
TYPE::isPtrType(Fund_type* ftype, Symbol* ut)
{
	if( isPointer(ftype, ut) )
	{
		return 1;
	}
	else if (_form == TF_user && symbol.tag()==t_arraytype)
	{
		return 1;
	}
	return 0;
}

int
TYPE::isIntegral(Fund_type* ftype, Symbol* ut)
{
	Tag	t;

	if (ut)
		*ut = symbol;
	if (ftype)
		*ftype = ft;
	if ((_form == TF_fund) &&
		((ft >= ft_char && ft <= ft_pointer) ||
		(ft == ft_string)))
	{
		return 1; // assume it is convertible
	}
	else if (_form == TF_user && (((t = symbol.tag()) 
		==t_enumtype) || t==t_enumlittype ||
		t==t_bitfield))
	{
		return 1;
	}
	return 0;
}

int
TYPE::isArithmetic(Fund_type* ftype, Symbol* ut)
{
	return(isIntegral(ftype, ut) || isFloatType(ftype));
}

int
TYPE::isFloatType(Fund_type* ftype)
{
	if (ftype)
		*ftype = ft;
	if (_form == TF_fund && ft >= ft_sfloat && ft <= ft_xfloat)
	{
		return 1;
	}
	return 0;
}

int
TYPE::isScalar(Fund_type* ftype, Symbol* ut)
{
	if( isArithmetic(ftype, ut) || isPtrType(ftype, ut))
	{
		return 1;
	}
	return 0;
}

int
TYPE::get_bound(Attr_name attr, Place &place, long &bnd, 
	LWP *lwp, Frame *frame)
{
	Attribute	*a;
	Locdesc		desc;

	// array bounds - can be constant or a run-time location
	if (_form != TF_user || ((a = symbol.attribute(attr))==0))
	{
		return 0;
	}

	if ( a->form==af_int )
	{
		place.kind = pUnknown;
		bnd = a->value.word;
		return 1;
	}
	else if (a->form != af_locdesc || !lwp)
		return 0;	// not an array type or unknown.
	
	if (!frame)
		frame = lwp->curframe();
	desc = a->value.loc;
	desc.adjust( symbol.base() );
	place = desc.place(lwp, frame);
	return 1;
}

#ifdef DEBUG
#include <stdio.h>
static char *ftStr[] =
{
	"ft_none",
	"ft_char",
	"ft_schar",
	"ft_uchar",
	"ft_short",
	"ft_sshort",
	"ft_ushort",
	"ft_int",
	"ft_sint",
	"ft_uint",
	"ft_long",
	"ft_slong",
	"ft_ulong",
	"ft_pointer",
	"ft_sfloat",
	"ft_lfloat",
	"ft_xfloat",
	"ft_scomplex",
	"ft_lcomplex",
	"ft_set",
	"ft_void",
	"ft_string"
};

#undef DEFTAG
#define DEFTAG(x,y)	y,
char *tagstr[] =
{
#include "Tag1.h"
};

char *Attrform[] =
{
	"af_none",
	"af_tag",				 // value.tag
	"af_int",				 // value.word
	"af_locdesc",			 // value.loc
	"af_stringndx",		   // value.name
	"af_coffrecord",		  // value.word
	"af_coffline",			// value.word
	"af_coffpc",			  // unused
	"af_spidoffs",			// unused
	"af_fundamental_type",	// value.fund_type
	"af_symndx",			  // unused
	"af_reg",				 // unused
	"af_addr",				// value.addr
	"af_local",			   // unused
	"af_visibility",		  // unused
	"af_lineinfo",			// value.lineinfo
	"af_attrlist",			// unused
	"af_cofffile",			// value.word
	"af_symbol",			  // value.symbol
	"af_dwarfoffs",		   // value.word
	"af_dwarfline",		   // value.word
	"af_elfoffs"			 // value.word
};

char *Attrname[] =
{
	"an_nomore",	
	"an_tag",		
	"an_name",	
	"an_child",	
	"an_sibling",	
	"an_parent",	
	"an_count",	
	"an_type",	
	"an_elemtype",	
	"an_elemspan",	
	"an_subscrtype",	
	"an_lobound",	
	"an_hibound",	
	"an_basetype",	
	"an_resulttype",	
	"an_argtype",	
	"an_bytesize",	
	"an_bitsize",	
	"an_bitoffs",	
	"an_litvalue",	
	"an_stringlen",	
	"an_lineinfo",	
	"an_location",	
	"an_lopc",	
	"an_hipc",	
	"an_visibility",	
	"an_scansize"	
};

void
dumpSymType(Attribute *attrPtr, int indent)
{
	int	i,
		j,
	   	attrCnt;

	if(attrPtr == 0)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr,"Attribute list empty\n");
		return;
	}
	
	for(j=0; j<indent; j++)
		fprintf(stderr,"  ");
	fprintf(stderr, "Attrlist @ %x\n", attrPtr);

	for(attrCnt=attrPtr->value.word, i=0; i < attrCnt; i++, attrPtr++)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr, "attr.name=%s, attr.form=%s, ", 
			Attrname[attrPtr->name], Attrform[attrPtr->form] );

		switch(attrPtr->form)
		{
		case af_tag:				 // value.tag
			fprintf(stderr,"attr.value.tag =%s", 
					tagstr[attrPtr->value.tag]);
			break;
		case af_locdesc:			 // value.loc
			fprintf(stderr,"attr.value.loc=<dump not implemented>");
			break;
		case af_stringndx:		   // value.name
			fprintf(stderr,"attr.value.name =%s",
							 attrPtr->value.name);
			break;
		case af_fundamental_type:	// value.fund_type
			fprintf(stderr,"attr.value.fund_type=%s", 
					ftStr[attrPtr->value.fund_type]);
			break;
		case af_addr:				// value.addr
			fprintf(stderr,"attr.value.addr =%x", 
						attrPtr->value.addr);
			break;
		case af_lineinfo:			// value.lineinfo
			fprintf(stderr,
				"attr.value.lineinfo=<dump not implemented>");
			break;
		case af_symbol:			  // value.symbol
			if( attrPtr->name==an_type ||
				attrPtr->name==an_elemtype ||
				attrPtr->name==an_basetype ||
				attrPtr->name==an_resulttype ||
				attrPtr->name==an_resulttype )
			{
				fprintf(stderr,"attr.value.symbol=\n");
				dumpSymType(attrPtr->value.symbol, indent+1);
			}
			else
			{
				fprintf(stderr,"attr.value.symbol =%x",
							 attrPtr->value.symbol);
			}
			break;
		case af_coffrecord:		  // value.word
		case af_coffline:			// value.word
		case af_cofffile:			// value.word
		case af_int:				 // value.word
		case af_dwarfoffs:		   // value.word
		case af_dwarfline:		   // value.word
		case af_elfoffs:			 // value.word
			fprintf(stderr,"attr.value.word =%x",
							attrPtr->value.word);
			break;
		case af_none:
		case af_coffpc:			  // unused
		case af_spidoffs:			// unused
		case af_symndx:			  // unused
		case af_reg:				 // unused
		case af_local:			   // unused
		case af_visibility:		  // unused
		case af_attrlist:			// unused
			fprintf(stderr,"attr.value=unused");
			break;
		}
		fprintf(stderr,"\n");
	}
}

void
TYPE::dumpTYPE()
{
	if(_form == TF_fund)
		fprintf(stderr,"fund_type=%s\n", ftStr[ft]);
	else
	{
		fprintf(stderr,"User_type=\n");
		dumpSymType(symbol.attribute(an_count), 1);
	}
}

// dump all siblings and chilren of root
void
dumpAttr( Attribute *root, int indent)
{
	int	i,
		j,
	   	attrCnt;
	Attribute *attrPtr = root;
	Attribute *siblingAttr = 0;
	Attribute *childAttr = 0;

	if(attrPtr == 0)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr,"Attribute list empty\n");
		return;
	}
	
	for(j=0; j<indent; j++)
		fprintf(stderr,"  ");
	fprintf(stderr, "Attrlist @ %x\n", attrPtr);

	for(attrCnt=attrPtr->value.word, i=0; i < attrCnt; i++, attrPtr++)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr, "attr.name=%s, attr.form=%s, ", 
			Attrname[attrPtr->name], Attrform[attrPtr->form] );

		switch(attrPtr->form)
		{
		case af_tag:				 // value.tag
			fprintf(stderr,"attr.value.tag =%s", 
					tagstr[attrPtr->value.tag]);
			break;
		case af_locdesc:			 // value.loc
			fprintf(stderr,"attr.value.loc=<dump not implemented>");
			break;
		case af_stringndx:		   // value.name
			fprintf(stderr,"attr.value.name =%s",
							 attrPtr->value.name);
			break;
		case af_fundamental_type:	// value.fund_type
			fprintf(stderr,"attr.value.fund_type=%s", 
					ftStr[attrPtr->value.fund_type]);
			break;
		case af_addr:				// value.addr
			fprintf(stderr,"attr.value.addr=%#x", 
					attrPtr->value.addr);
			break;
		case af_lineinfo:			// value.lineinfo
			fprintf(stderr,
				"attr.value.lineinfo=<dump not implemented>");
			break;
		case af_symbol:			  // value.symbol
			if( attrPtr->name==an_parent )
			{
				fprintf(stderr,"attr.value.symbol=%x", 
							 attrPtr->value.symbol);
			}
			else if( attrPtr->name==an_child )
			{
				fprintf(stderr,"attr.value.symbol=%x (below)", 
							 attrPtr->value.symbol);
				childAttr = attrPtr->value.symbol;
			}
			else if( attrPtr->name==an_sibling )
			{
				fprintf(stderr,"attr.value.symbol=%x (below)",
							 attrPtr->value.symbol);
				siblingAttr = attrPtr->value.symbol;
			}
			else
			{
				fprintf(stderr,"attr.value.symbol=\n");
				dumpAttr(attrPtr->value.symbol, indent+1);
			}
			break;
		case af_coffrecord:		  // value.word
		case af_coffline:			// value.word
		case af_cofffile:			// value.word
		case af_int:				 // value.word
		case af_dwarfoffs:		   // value.word
		case af_dwarfline:		   // value.word
		case af_elfoffs:			 // value.word
		// unused (so they say) dump as long word, value may be meanless
		case af_none:
		case af_coffpc:			  // unused
		case af_spidoffs:			// unused
		case af_symndx:			  // unused
		case af_reg:				 // unused
		case af_local:			   // unused
		case af_visibility:		  // unused
		case af_attrlist:			// unused
			fprintf(stderr,"attr.value.word =%x",
							attrPtr->value.word);
			break;
		}
		fprintf(stderr,"\n");
	}
	if( childAttr )
	{
		fprintf(stderr,"\n");
		if( childAttr == root )
		{
			fprintf(stderr,"LOOP: child has same address as root\n");
			return;
		}
		dumpAttr(childAttr, indent+1);
	}
	if( siblingAttr )
	{
		fprintf(stderr,"\n");
		if( siblingAttr == root )
		{
			fprintf(stderr,"LOOP: sibling has same address as root\n");
			return;
		}
		dumpAttr(childAttr, indent+1);
	}
}

#endif
