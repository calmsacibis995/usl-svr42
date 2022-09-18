#ident	"@(#)debugger:libsymbol/common/Symtable.C	1.3"
#include	"Symtable.h"
#include	"Evaluator.h"
#include	"Tag.h"
#include	"Object.h"
#include	<string.h>

#define stacktag(X)	((X) == t_entry || (X) == t_subroutine \
			|| (X) == t_global_sub || (X) == t_extlabel)

Symtable::Symtable( int fd, Object *obj )// not inlined so we don't need Evaluator.h
{				// in Symtable.h
	evaluator = new Evaluator( fd, obj );
}

Symtable::~Symtable()
{
	delete evaluator;
}

Symbol
Symtable::find_entry( Iaddr addr )	// nearest enclosing function or label
{
	Symbol		symbol;
	Tag		tag;

	symbol = find_scope( addr );
	tag = symbol.tag();
	while(!symbol.isnull() && !stacktag(tag))
	{
		symbol = symbol.parent();
		tag = symbol.tag();
	}
	if ( symbol.isnull() )
	{
		symbol.attrlist = evaluator->lookup_addr( addr );
	}
	symbol.evaluator = evaluator;
	return symbol;
}

Symbol
Symtable::find_symbol( Iaddr addr )  // symbol corresponding with address
{
	Symbol	symbol;

	symbol.attrlist = evaluator->lookup_addr( addr );
	symbol.evaluator = evaluator;
	return symbol;
}

Symbol
Symtable::find_scope ( Iaddr addr )	// nearest enclosing Symbol
{
	Symbol	i,nearest;

	i = first_symbol();
	while ( !i.isnull() )
	{
		Iaddr	low;
		low = i.pc(an_lopc);
		if (low == (Iaddr)~0 || addr < low)
			i = i.sibling();
		else
		{
			Iaddr	high;
			high = i.pc(an_hipc);
			if (high == (Iaddr)~0 || addr >= high)
				i = i.sibling();
			else
			{
				nearest = i;
				i = i.child();
			}
		}
	}
	return nearest;
}

Symbol
Symtable::first_symbol()
{
	Symbol	symbol;

	symbol.evaluator = evaluator;
	symbol.attrlist = evaluator->first_file();
	return symbol;
}

int
Symtable::find_source( Iaddr pc, Symbol & symbol )
{
	Symbol		x;

	x = first_symbol();
	while ( !x.isnull() )
	{
		if ( pc < x.pc(an_lopc) || pc >= x.pc(an_hipc) )
		{
			x = x.sibling();
		}
		else
		{
			symbol = x;
			return 1;
		}
	}
	return 0;
}

int
Symtable::find_source( const char * name, Symbol & symbol )
{
	Symbol		x;
	char *		s;

	x = first_symbol();
	while ( !x.isnull() )
	{
		if ( strcmp(name,x.name()) == 0 )
		{
			symbol = x;
			return 1;
		}
		else if ( (s = strrchr( x.name(), '/' )) == 0 )
		{
			x = x.sibling();
		}
		else if ( strcmp( s+1, name ) == 0 )
		{
			symbol = x;
			return 1;
		}
		else
		{
			x = x.sibling();
		}
	}
	return 0;
}

Symbol
Symtable::find_global( const char * name )
{
	Symbol		symbol;

	symbol.attrlist = evaluator->find_global( name );
	symbol.evaluator = evaluator;

	return symbol;
}

NameEntry *
Symtable::first_global()	// not inlined so we don't need Evaluator.h
{				// in Symtable.h
	return evaluator->first_global();
}

NameEntry *
Symtable::next_global()		// not inlined so we don't need Evaluator.h
{				// in Symtable.h
	return evaluator->next_global();
}

NameEntry *
Symtable::prev_global()		// not inlined so we don't need Evaluator.h
{				// in Symtable.h
	return evaluator->prev_global();
}

Symbol
Symtable::global_symbol( NameEntry * n )
{
	Symbol	symbol;

	symbol.attrlist = evaluator->evaluate(n);
	symbol.evaluator = evaluator;

	return symbol;
}
