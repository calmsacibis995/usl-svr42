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
#ident	"@(#)debugger:libsymbol/common/Symbol.C	1.3"
#include	"Symbol.h"
#include	"Source.h"
#include	"Evaluator.h"
#include	"Locdesc.h"
#include	"Interface.h"
#include	"Tag.h"
#include	"TYPE.h"
#include	<string.h>

char *
Symbol::name()
{
	Attribute *	a;

	if ( namep == 0 && evaluator &&
			(a = evaluator->attribute(attrlist, an_name)) )
	{
		namep = a->value.name;
	}
	return namep;
}

char *
Symbol::mangledName()
{
	Attribute *	a;

	if ( namep == 0 && evaluator &&
			(a = evaluator->attribute(attrlist, an_mangledname)) )
	{
		mangledNamep = a->value.name;
	}
	return mangledNamep;
}

Symbol
Symbol::arc( Attr_name attrname )
{
	Symbol		symbol;
	Attribute *	a;

	if ( evaluator && (a = evaluator->arc(attrlist,attrname)) != 0 )
	{
		symbol.attrlist = a->value.symbol;
		symbol.evaluator = evaluator;
		symbol.ss_base = ss_base;
	}
	return symbol;
}

Attribute *
Symbol::attribute( Attr_name attrname )
{
	if ( evaluator == 0 )
	{
		return 0;
	}
	else
	{
		return evaluator->attribute( attrlist, attrname );
	}
}

Iaddr
Symbol::pc( Attr_name attr_name )
{
	Attribute	* a;

	if ( (a = attribute(attr_name)) == 0 )
	{
		return ~0;
	}
	else
	{
		return a->value.addr + ss_base;
	}
}

int
Symbol::source( Source & s )
{
	Attribute	* a;

	if ( (a = attribute(an_lineinfo)) == 0 )
	{
		s.lineinfo = 0;
		s.ss_base = 0;
		return 0;
	}
	else
	{
		s.lineinfo = a->value.lineinfo;
		s.ss_base = ss_base;
		return 1;
	}
}

int
Symbol::type(TYPE& t, Attr_name attr)
{
	Attribute *a;
	Symbol s(*this);

	a = attribute(attr);
	while(a)
	{
		switch (a->form)
		{
		case af_symbol:
			{
			Attribute	*a1, *tag;
			a1 = a->value.symbol;
			if ((tag = evaluator->attribute(a1, an_tag)) == 0)
			{
				t.null();
				return 0;
			}
			if (tag->value.tag == t_typedef)
			{
				// typedef - find real type
				a = evaluator->attribute(a1, attr);
				break;
			}
			else
			{
				s.namep = 0;
				s.attrlist = a->value.symbol;
				t = s;
				return 1;
			}
			break;
			}
		case af_fundamental_type:
			// the symbol has no type information at all (meaning
			// it was probably compiled without -g) - assume
			// function returning int (if in text) or just int
			// if object
			if (a->value.fund_type == ft_none)
			{
				a->value.fund_type = ft_int;
			}
			t = a->value.fund_type;
			return 1;
		default:
			t.null();
			return 0;
		}
	}
	t.null();
	return 0;
}

int
Symbol::locdesc(Locdesc & desc, Attr_name attr)
{
	Attribute *a;

	if ((a = attribute(attr)) == 0)
	{
		desc.clear();
		return 0;
	}
	if (a->form == af_locdesc)
	{
		desc = a->value.loc;
		desc.adjust( ss_base );
		return 1;
	} else {
		return 0;
	}
}

Tag
Symbol::tag(const char *msg)
{
	register Attribute *a = attribute(an_tag);

	if (a != 0 && a->value.tag != 0)
	{
		return a->value.tag;
	}
	if (msg)
	{
#ifdef DEBUG
		printf("No Tag: %s symbol.parent() is %s\n", msg, parent().name());
#endif /* DEBUG */
	}
	return t_none;
}

Symbol &
Symbol::operator=( Symbol &s )
{
	namep = s.namep;
	attrlist = s.attrlist;
	evaluator = s.evaluator;
	ss_base = s.ss_base;
	return *this;
}


void
Symbol::null()
{
	namep = 0;
	mangledNamep = 0;
	attrlist = 0;
	evaluator = 0;
	ss_base = 0;
}

int
Symbol::isSubrtn()
{
	Tag t = tag();
	return (t==t_global_sub || t==t_subroutine);
}

int
Symbol::isUserTypeSym()
{
	Tag t = tag();
	return (t==t_structuretype || t==t_uniontype || 
		            t==t_enumtype || t==t_typedef);
}

int
Symbol::isUserTagName()
{
	Tag t = tag();
	return (t==t_structuretype || t==t_uniontype || 
		            t==t_enumtype);
}

int
Symbol::isVariable()
{
	Tag t = tag();
	return (t==t_global_variable || t==t_local_variable ||
		t==t_argument);
}

int
Symbol::isEntry()
{
	Tag t = tag();
	return (t==t_global_sub || t==t_subroutine || t == t_entry);
}

int
Symbol::isMember()
{
	Tag t = tag();
	return (t==t_structuremem || t==t_unionmem || t==t_bitfield);
}

// Was type of symbol assumed to be int?  (no type information available)
// If true_only is set, we return 0 for not assumed, 1 for assumed.
// If true_only is 0, we return 0 for not assumed, 1 if assumed and
// this is the first such request, else > 1.
int
Symbol::type_assumed(int true_only)
{
	register Attribute *a;

	if ((isEntry()) || ((a = attribute(an_assumed_type)) == 0))
		return 0;
	else if (true_only)
		return 1;
	else 
		return(++(a->value.word));
}
