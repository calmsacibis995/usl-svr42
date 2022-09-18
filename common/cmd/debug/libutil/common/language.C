#ident	"@(#)debugger:libutil/common/language.C	1.2"

#include "utility.h"
#include "Language.h"
#include "Interface.h"
#include "LWP.h"
#include "Symbol.h"
#include "Attribute.h"
#include <stdio.h>

static Language language = UnSpec; // %lang is set only by the user

// string names are known only in this file
// enum Language is externally visible

struct langmap {
	const char *name;
	Language lang;
};

static langmap lm[] = {
    	"C",	C,	// debug C exprs.
    	"c",	C,	// debug C exprs.
    	"cc",	C,	// debug C exprs.
    	"C++",	CPLUS,	// debug C++ subset.
    	"c++",	CPLUS,	// debug C++ subset.
    	"CC",	CPLUS,	// debug C++ subset.
	0,	UnSpec,	// end of list
};

Language
current_language(LWP *lwp)
{
	Language return_value = current_user_language();
	if (return_value == UnSpec)
		return_value = current_context_language(lwp);
	return return_value;
}

Language current_user_language()
{
	return language;
}

Language
current_context_language(LWP *lwp)
{
	Symbol		source;
	Attribute	*attr;

	// %db_lang defaults to C if unknown

	if (!lwp) return C;

	source = lwp->current_source();

	if (source.isnull()
		|| (attr = source.attribute(an_language)) == 0
		|| attr->value.language == UnSpec)
		return C;
	return attr->value.language;
}

const char *
language_name( Language lang )
{
	register langmap *p = lm;
	for (p = lm; p->name; p++)
	{
		if ( lang == p->lang )
			return p->name;
	}
	return 0;
}

int
set_language( const char *name )
{
	register langmap *p;

	if (!name || !*name)
	{
		language = UnSpec;
		return 1;
	}

	for (p = lm; p->name; p++)
	{
		if (strcmp( name, p->name ) == 0)
		{
			language = p->lang;
			if (get_ui_type() == ui_gui)
				printm(MSG_set_language, name);
			return 1;
		}
	}
	printe(ERR_unknown_language, E_ERROR, name);
	return 0;
}
