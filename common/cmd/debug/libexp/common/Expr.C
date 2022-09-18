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
#ident	"@(#)debugger:libexp/common/Expr.C	1.7"

#include "Language.h"
#include "Resolver.h"
#include "CC.h"
#include "ParsedRep.h"
#include "CCtree.h"
#include "Expr.h"
#include "Interface.h"
#include "LWP.h"
#include "Proglist.h"
#include "Value.h"
#include "str.h"
#include "Symbol.h"
#include "Tag.h"
#include "Locdesc.h"
#include "Place.h"
#include "utility.h"
#include <string.h>
#include <libgen.h>

#include <cvt_util.h> // for error recovery
#include <setjmp.h>
#include <signal.h>

// flags used in flags field
#define	IS_EVENT	1
#define IS_SYMBOL	2

Expr::Expr(char *e, LWP* lwp, int event)
{
	estring = makesf("%s\n", e);	// add line feed for lexers.
	lang = current_language(lwp);
	etree = 0;
	value = 0;
	if (event)
		flags = IS_EVENT;
	else 
		flags = 0;
}

Expr::~Expr()
{
	delete value;
	delete estring;
	delete etree;
}

int
Expr::parse(Resolver *context)
{
	switch (lang)
	{
	case C:
	case CPLUS:
		if( !etree )
		{
			if( !(etree=(ParsedRep *)CCparse(estring, 
				lang, context)) || 
			    !CCresolve(lang, (CCtree *)etree, 
				context, (flags & IS_EVENT)) )
			{
				return 0;
			}
		}
		else if (!(flags & (IS_EVENT|IS_SYMBOL)))
		{
			if (!CCresolve(lang, (CCtree *)etree, context, 0) )
			{
				return 0;
			}
		}
		break;
	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "Expr::parse", __LINE__);
	}

	return 1;
}

static jmp_buf the_jmp_buf;

static void 
check_for_user_error(int sig)
{
	if (sig == SIGFPE)
	{
		printe(ERR_float_eval, E_ERROR);
		clear_fp_error();
		sigrelse(sig);
		longjmp(the_jmp_buf, 1);
	}
}

int
Expr::eval(LWP *lwp, Iaddr pc, Frame *frame)
{
	Resolver	*context;

	if (frame == 0 && lwp != 0)
	{
		frame = lwp->curframe();
	}

	if (pc == ~0 && lwp != 0)
		pc = frame->pc_value();

	if (lang == CPLUS)
		context = (Resolver *)new CCresolver(lwp, pc);
	else
		context = new Resolver(lwp, pc);

	value = 0;	// make sure this is well defined.

	if( parse(context) )
	{
		if (setjmp(the_jmp_buf) == 0)
		{
			init_fp_error_recovery(check_for_user_error);
			value = etree->eval(lang, lwp, frame, (flags & IS_EVENT));
		}
		clear_fp_error_recovery();
	}
	delete context;
	return (value!=0);
}


int
Expr::lvalue(Place& loc)
{
	loc.null();
	if (value != 0)
	{
		loc = value->object().loc;
	}
	return ! loc.isnull();
}

int
Expr::rvalue(Rvalue& rval)
{
	rval.null();
	if (value != 0) 
	{
		value->rvalue(rval);
	}
	return ! rval.isnull();
}

Expr &
Expr::operator=( Expr &e )
{
	lang = e.lang;
	estring = makestr(e.estring);
	etree = e.etree->clone();
	value = new Value( *e.value );

	return *this;
}

Expr::Expr(Symbol &sym, LWP* lwp)
{
	DPRINT(DBG_EXPR,("Expr::Expr(sym=%s)\n", sym.name()));
	lang = current_language(lwp);
	estring = makestr(sym.name());
	// don't need to resolve symbol exprs
	flags = IS_SYMBOL;
	value = 0;

	switch (lang) 
	{
	case C:
	case CPLUS:
		etree = (ParsedRep *)new CCtree(sym);
		break;
	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "Expr::Expr", __LINE__);
	}
}

Symbol
find_symbol(const char *file, const char *name, LWP *lwp, Iaddr pc, 
	Symtype stype)
{
	Symbol		sym;
	Language	lang = current_language(lwp);
	Resolver	*context;

	if (!file)
	{
		// if no file name was specified, 
		// look it up in the current scope
		if (lang == CPLUS)
			context = (Resolver *)new CCresolver(lwp, pc);
		else
			context = new Resolver(lwp, pc);
	}
	else
	{
		// just this file
		Symbol	scope;
		for (scope = lwp->first_file(); !scope.isnull() ;
			scope = lwp->next_file())
		{
			if ((!strcmp(scope.name(), file)) ||
				(!strcmp(basename(scope.name()), 
					basename((char *)file))))
				break;
		}
		if (scope.isnull())
			return sym;
		if (lang == CPLUS)
			context = (Resolver *)new CCresolver(scope, lwp);
		else
			context = new Resolver(scope, lwp);

	}
	context->lookup((char *)name, sym, stype);
	delete context;
	return sym;
}

int
Expr::triggerList(LWP *lwp, Iaddr pc, List &valueList)
{
	Resolver   	*context;
	int		ret;

	if (lang == CPLUS)
		context = (Resolver *)new CCresolver(lwp, pc);
	else
		context = new Resolver(lwp, pc);
	
	// don't reparse if already done
	if( !etree && !parse(context) )
		return 0;

	ret = etree->triggerList(lang, lwp, context, valueList);
	delete context;
	return ret;
}

Expr *
Expr::copyEventExpr(List& old_tl, List& new_tl, LWP* new_lwp)
{
	Expr *newExpr = new Expr(estring, new_lwp, 1);
	newExpr->etree = etree->copyEventExpr(old_tl, new_tl, new_lwp);

	return newExpr;
}
