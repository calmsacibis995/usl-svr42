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
#ident	"@(#)debugger:libutil/common/set_val.C	1.3"

#include "Interface.h"
#include "Parser.h"
#include "Expr.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Rvalue.h"
#include "str.h"
#include "utility.h"
#include <string.h>

#include "Buffer.h"
#include "Dbgvarsupp.h"

static char *
expand_name_rhs(Exp *exp)
{
	gbuf1.clear();
	if ((*exp)[1]) 
	{
		printe(ERR_concat, E_ERROR);
		return 0;
	}
	char *p = (*exp)[0];
	if (p[0] == '%' || p[0] == '$' || p[0] == '"')
		gbuf1.add(p);
	else
	{
		gbuf1.add('"');
		gbuf1.add(p);
		gbuf1.add('"');
	}

	return (char*)gbuf1;
}

static char *
expand_string_list_rhs(LWP * lwp, Exp *exp, int report_errors)
{
	gbuf1.clear();
	gbuf1.add('"');
	char * p;
	for ( int j = 0; p = (*exp)[j]; j++) 
	{
		if (p[0] == '%' || p[0] == '$')
		{
			// This is reinstantiated for each variable on the rhs.
			// This is estimated to be less costly than
			// doing it once outside the loop since vars
			// are less often used in the set command.
			Debug_var_support * debug_var_values =
			   new Debug_var_support(
			      lwp, lwp ? lwp->curframe() : 0, 1, 1, 1);
			debug_var_values->Find(p);
			char * value = debug_var_values->Value();
			if (!value) 
			{
		 		if (report_errors)
					printe(ERR_eval_fail_expr, 
						E_ERROR, p);
				return 0;
			}
			gbuf1.add(value);
			delete debug_var_values;
		}
		else if (p[0] == '"')
		{
			int len = strlen(p);
			p[len-1] = 0;
			gbuf1.add(p+1);
			p[len-1] = '"';
		}
		else
		{
			if (report_errors) 
				printe(ERR_string_req, E_ERROR);
			return 0;
		}
	}
	gbuf1.add('"');

	return (char*)gbuf1;
}

static char *
expand_expr_rhs(Exp *exp)
{
	gbuf1.clear();
	char * p;
	for ( int j = 0; p = (*exp)[j]; j++) 
	{
		gbuf1.add(p);
		gbuf1.add(' ');
	}
	return (char*)gbuf1;
}

int
set_val(Proclist * procl, char *lval, Exp *exp)
{
	int 		single = 1;
	LWP		*lwp;
	plist		*list;
	int		ret = 1;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl);
		lwp = list++->p_lwp;
	}
	else
	{
		lwp = proglist.current_lwp();
	}
	do
	{

		char * rhs = (*exp)[0];

		// if lval is predefined string debug var...
		if (lval[0] == '%')
		{
			// name vars
			// This allows the user to omit quotes for
			// names that the expression evaluator would
			// normally choke on.

			dbg_vtype	vtype = dbg_var(lval);
			switch(vtype) 
			{
				case Follow:
				case Func:
				case Lang:
				case Lwp:
				case Mode:
				case Proc:
				case Pprogram:
				case PRedir:
				case Verbose:
				case Wait:
					rhs = expand_name_rhs(exp);
					break;
				case Glob_path:
				case Lfile:
				case Path:
				case Prompt:
					// string_list vars
					rhs = expand_string_list_rhs(lwp,exp,1);
					break;
				default:
					break;
			}
		}
		else if (lval[0] == '$')
		{
			// Allow string_list
			rhs = expand_string_list_rhs(lwp, exp, 0);
			// but, if not, let expr evaluator try
			if (!rhs) rhs = expand_expr_rhs(exp);
		}

		else if ((*exp)[1])
			rhs = expand_expr_rhs(exp);

		if (!rhs) 
		{
			ret = 0;
			continue;
		}

		char	*e;
		if (lval)
			// MORE - for now, C dependent
			e = sf("%s = %s", lval, rhs);
		else
			e = rhs;
		Expr exp(e, lwp);
		Rvalue rval;
		if (!exp.eval(lwp) || !exp.rvalue(rval) )
		{
			ret = 0;
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
