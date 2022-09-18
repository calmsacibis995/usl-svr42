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
#ident	"@(#)debugger:libexp/common/Resolver.C	1.6.1.1"

#include <string.h>
#include <signal.h>
#include "Resolver.h"
#include "global.h"
#include "TYPE.h"
#include "Interface.h"
#include "Symtab.h"
#include "LWP.h"
#include "Tag.h"


Resolver::Resolver(LWP *proc, Iaddr pc)
{
	Symtab *symtab;

	mode    = mFULL_SEARCH;
	lwp = proc;

	if (lwp != 0 && (symtab = lwp->find_symtab(pc)) != 0)
	{
		curScope = symtab->find_scope(pc);
	}
	inGlobalSearch = 0;
}

Resolver::Resolver(Symbol& scope, LWP *proc)
{
	// resolver for members of aggregates 
	// e.g.	(C/C++: struct/union/class, ...).

	mode = mLOCAL_ONLY;  // don't climb scope chain.
	lwp   = proc;
	top_scope = scope;
	inGlobalSearch = 0;
}

int
Resolver::getEnclosingSrc(Symbol &src)
{
	src.null();

	if( !curScope.isnull() )
	{
		src = curScope;
	}
	else if( !top_scope.isnull() )
	{
		src = top_scope;
	}
	else
	{
		return 0;
	}

	for(; !src.isnull() && !src.isSourceFile();
			   src = src.parent())
		;

	return !src.isnull();
}

int
Resolver::getEnclosingFunc(Symbol& func)
{
	Symbol s = curScope;
	while( !s.isnull() && !s.isSubrtn())
	{
		s = s.parent();
	}

	if( !s.isnull() )
	{
		func = s;
		return 1;
	}
	return 0;
}

int
Resolver::find_enumlit(Symbol& enumtype, const char * id, Symbol& litsym)
{
	Symbol s;

	litsym.null();

	for (s = enumtype.child(); ! s.isnull(); s = s.sibling())
	{
		if (!s.isEnumLitType())
		{
			break;
		}
		Attribute *a = s.attribute(an_litvalue);
	
		if (a != 0 && a->form == af_int)
		{
			char *nname = s.name();
	
			if (nname != 0 && strcmp(nname, id) == 0)
			{
				litsym = s;
				return 1;
			}
		}
	}
	return 0;
}

int
Resolver::cmpName(const char* id, Symbol& s)
{
	char *s_name = s.name();
	if( s_name == 0 )
		return 0;

	return (strcmp(s_name, id) == 0 );
}

// base class version does not try to find another symbol with
// same name
int	
Resolver::getNext(const char *, Symbol& sym)
{
	sym.null();
	return 0;
}

int
Resolver::searchSiblingChain(const char* id, Symbol& startSym, 
	Symtype user_def_type)
{
	if( startSym.isnull() )
	{
		curSymbol.null();
		return 0;
	}

	Symbol s;
	for( s = startSym; ! s.isnull(); s = s.sibling() )
	{
		if (interrupt & sigbit(SIGINT))
			break;

		if (s.tag() == t_none)
			continue;

		if( cmpName(id, s) )
		{ 
			// distinguish tag names and typedef names.
			//  .. typedef enum e { a, b, c } E; int e;  is legal C.
			//     but NOT legal C++ - but we shouldn't see conflicts
			// in C++
			if (checkTag(user_def_type, s))
			{
				curSymbol = s;
				return 1;
			}
		}
		
		if( s.isEnumType() )
		{
			if (find_enumlit(s, id, curSymbol))
				return 1;
		}
	}
	curSymbol.null();
	return 0;
}


//
// Search from curScope outward to and includeing the top scope
//
int
Resolver::searchOutwardToTopScope(const char *id, Symbol& bottom, 
	Symbol &top, Symtype user_def_type)
{
	Tag tag;
	Symbol s = bottom;

	for( ;!s.isnull(); s=s.parent())
	{
		if (interrupt & sigbit(SIGINT))
			break;
		tag = s.tag();
		switch (tag)
		{
		case t_subroutine:
		case t_global_sub:
		case t_block:
		case t_entry:
		case t_sourcefile:
		case t_extlabel:
		case t_structuretype:
		case t_uniontype:
		case t_label:
			if( searchScope(id, s, user_def_type) )
			{
				return 1;
			}
			break;
		default:
			printe(ERR_unexpected_tag, E_ERROR, tag);
			continue;
		}
		if( s == top )
		{
			break;
		}
	} 
	return 0;
}

int
Resolver::find_static(const char *id, Symtype user_def_type)
{
	Symbol curSource;

	getEnclosingSrc(curSource);
	searchScope(id, curSource, user_def_type);

	while( !curSymbol.isnull() )
	{
		if( !curSymbol.isGlobalVar() && !curSymbol.isGlobalSub() )
		{
			// found one that is not a global
			if (checkTag(user_def_type, curSymbol))
				return 1;
		}
		searchSiblingChain(id, curSymbol.sibling(), user_def_type);
	}

	curSymbol.null();
	return 0;
}

int
Resolver::find_global(const char *id)
{
	inGlobalSearch = 1;
	curSymbol = lwp->find_global(id);
	return (!curSymbol.isnull());
}

int
Resolver::lookup(const char *id, Symbol& result, Symtype user_def_type)
{
	inGlobalSearch = 0;
	if ( !lwp )
	{
		return 0;
	}

	sigrelse(SIGINT);
	curSymbol.null();
	result.null();

	switch( mode )
	{
	case mFULL_SEARCH:
	{
		// search for automatics
		getEnclosingFunc(top_scope);
		if( searchOutwardToTopScope(id, curScope, top_scope, 
			user_def_type) )
		{
			result = curSymbol;
			break;
		}
		// search static, search global
		if( find_static(id, user_def_type) || find_global(id) )
		{
			result = curSymbol;
		}
		break;
	}
	case mLOCAL_ONLY :
	{
		if( searchScope(id, top_scope, user_def_type) )
		{
			result = curSymbol;
		}
		break;
	}
	default:
		break;
	}

	sighold(SIGINT);
	return !result.isnull();
}

int
Resolver::checkTag(Symtype user_def_type, Symbol &s)
{
	switch(user_def_type)
	{
	case st_notags:
		if (!s.isUserTagName())
		{
			return 1;
		}
		break;
	case st_tagnames:
		if (s.isUserTagName())
		{
			return 1;
		}
		break;
	case st_usertypes:
		if (s.isUserTypeSym())
		{
			return 1;
		}
		break;
	case st_func:
		if (s.isEntry() || s.isLabel() || s.isBlock())
		{
			return 1;
		}
		break;
	case st_object:
		if (s.isVariable())
		{
			return 1;
		}
		break;
	}
	return 0;
}

// C++ specific functions

int
CCresolver::cmpName(const char* id, Symbol& s)
{
	char *s_name = s.name();
	if( s_name == 0 )
		return 0;

	char* parenPos;
	char *colonPos;
	if( strcmp(s_name, id) == 0 )
	{
		return 1;
	}
	else if( (parenPos=strchr(s_name, '(')) && !strchr(id, '(') )
	{
		// symbol is prototyped function name and the id is not,
		// compare up to the first left paren (i.e., ignore prototype)
		int len = parenPos-s_name;
		if(len==strlen(id)&&strncmp(s_name, id, len)==0)
		{
			return 1;
		}
	}
	// if a class contains inherited members, these member names are
	// prefixed by the inherited class name and the "::" operator.
	// Move past this prefix and do compare.
	else if( s.parent().isClassType() &&
		   (colonPos=strchr(s_name, ':')) && *(colonPos+1)==':'  &&
	  	      strcmp(id, colonPos+2)==0 )
	{
		return 1;
	}

	return 0;
}

char *
CCresolver::buildClassQualifiedName(const char* id, Symbol& classSym)
{
	char *className;
	if( !(className=classSym.name()) )
	{
		printe( ERR_internal, E_ERROR,
			"Resolver::buildClassQualifedName", __LINE__);
		return 0;
	}

	char *qualifiedNm = new char[strlen(className) + strlen(id) + 3];
	sprintf(qualifiedNm, "%s::%s", className, id);

	return qualifiedNm;
}

int
CCresolver::inMemberFunction(Symbol& thisSym, Symbol& classSym)
{
	// is there a "this" argument
	Symbol curFunc;
	if( !getEnclosingFunc(curFunc) )
	{
		return 0;
	}

	if( !searchScope(THIS_NM, curFunc, st_notags) )
	{
		return 0;
	}

	// result of searchScope returned in curSymbol member
	thisSym = curSymbol;

	if( thisSym.tag() != t_argument )
	{
		return 0;
	}

	// does "this" argument have the correct type
	TYPE thisType;
	Fund_type thisFT;
	Symbol thisUT;
	if( !thisSym.type(thisType) || !thisType.isPointer(&thisFT, &thisUT) )
	{
		return 0;
	}

	Symbol classStruct = thisUT.arc(an_basetype);
	if( classStruct.tag() != t_structuretype )
	{
		return 0;
	}

	classSym = classStruct;
	return 1;
}

// Find global symbol.  For C++ functions we may have multiple symbols
// with same name and different prototypes.  If we are asking for
// name with no prototype information, and the name we get back is
// prototyped, it might not be the first alphabetically in the NameEntry
// list.  We search backward to find the first match alphabetically, so
// future forward searches will find all instances.
int
CCresolver::find_global(const char *id)
{
	inGlobalSearch = 1;
	curSymbol = lwp->find_global(id);
	if (curSymbol.isnull())
		return 0;
	if ((strchr(id, '(') == 0) && (strchr(curSymbol.name(), '(') != 0))
	{
		Symbol	sym = lwp->prev_global();
		while(!sym.isnull())
		{
			if (cmpName(id, sym))
			{
				curSymbol = sym;
				sym = lwp->prev_global();
			}
			else
			{
				sym = lwp->next_global();
				break;
			}
		}
	}
	return 1;
}

// find symbol with given name; if user_def_type is set,
// we only return symbols representing struct, class, union or enum
// tags
int
CCresolver::lookup(const char *id, Symbol& result, 
	Symtype user_def_type)
{
	inGlobalSearch = 0;
	if ( !lwp )
	{
		return 0;
	}

	sigrelse(SIGINT);
	curSymbol.null();
	result.null();

	char *colonPos;
	char *localId = 0;
	switch( mode )
	{
	case mFULL_SEARCH:
	{
		// search for automatics
		getEnclosingFunc(top_scope);
		if( searchOutwardToTopScope(id, curScope, top_scope, 
			user_def_type) )
		{
			result = curSymbol;
			break;
		}
		
		Symbol classSym;
		Symbol thisSym;
		if( inMemberFunction(thisSym, classSym) )
		{
			// search class for data members
			if( searchScope(id, classSym, st_notags) )
			{
				result = curSymbol;
				break;
			}

			// if name not qualified, build qualified
			// name and continue static/global search

			if(!((colonPos=strchr(id, ':')) && *(colonPos+1)==':'))
			{
				localId = buildClassQualifiedName(id, classSym);
			}
		}

		// search static, search global
		if( find_static(id, user_def_type) || find_global(id) )
		{
			result = curSymbol;
			break;
		}
		if (localId)
		{
			if( find_static(localId, user_def_type) || 
				find_global(localId) )
			{
				result = curSymbol;
				break;
			}
		}
		
		break;
	}
	case mLOCAL_ONLY :
	{
		if( searchScope(id, top_scope, user_def_type) )
		{
			result = curSymbol;
			break;
		}
		
		// not found, if top scope is a structure,
		// the name could be a class data member item,
		// build a class qualified name search static/global
		if( !top_scope.isClassType() )
		{
			break;
		}

		if( !((colonPos=strchr(id, ':')) && *(colonPos+1)==':') )
		{
			localId = buildClassQualifiedName(id, top_scope);
		}

		if( find_static(localId, user_def_type) || find_global(localId) )
		{
			result = curSymbol;
		}
		break;
	}
	default:
		break;
	}

	if( localId != id )
	{
		delete localId;
	}

	sighold(SIGINT);
	return !result.isnull();
}

int
CCresolver::getNext(const char *id, Symbol& result)
{
	result.null();

	if( curSymbol.isnull() )
		return 0;

	// global function definitions appear as the children of source 
	// files and as global.  Don't pick up the same symbol twice!
	if( !inGlobalSearch && searchSiblingChain(id,curSymbol.sibling(), st_notags) &&
	    lwp->find_global(curSymbol.name()).isnull() )
	{
		//found a static instance of name
		result = curSymbol;
		return 1;
	}

	if( !inGlobalSearch )
	{
		// start global search
		if( !find_global(id) )
			curSymbol.null();
	}
	else
	{
		// continue global search
		curSymbol = lwp->next_global();
		if( curSymbol.isnull() || !cmpName(id, curSymbol) )
		{
			curSymbol.null();
		}
	}
	result = curSymbol;
		
	return !result.isnull();
}
