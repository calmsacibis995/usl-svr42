#ident	"@(#)debugger:libcmd/common/Location.C	1.5"

#include "Location.h"
#include "LWP.h"
#include "Interface.h"
#include "Proglist.h"
#include "Dbgvarsupp.h"
#include "str.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define VAR_MASK	0x7f

int
Location::get_str_val(LWP *l, Frame *f, const char *name, char *&value)
{
	Debug_var_support	*user_var_table;
	char			*var;

	// Get variable textual value.
	user_var_table = new Debug_var_support(l,f,1,1,1);
	user_var_table->Find((char *)name);
	var = user_var_table->Value();
	delete user_var_table;
	if (!var)
	{
		printe(ERR_user_var_defined, E_ERROR, name);
		return 0;
	}
	value = new(char[strlen(var) + 1]);
	strcpy(value, var);
	return 1;
}

int
Location::get_int_val(LWP *l, Frame *f, const char *name, 
	unsigned long &lval)
{
	Debug_var_support	*user_var_table;
	char			*ptr;
	char			*var_value;

	// Get variable textual value.
	user_var_table = new Debug_var_support(l,f,1,1,1);
	user_var_table->Find((char *)name);
	var_value = user_var_table->Value();
	delete user_var_table;
	if (!var_value)
	{
		printe(ERR_user_var_defined, E_ERROR, name);
		return 0;
	}
	lval = strtoul(var_value, &ptr, 0);
	if (*ptr)
	{
		printe(ERR_bad_number, E_ERROR, name);
		return 0;
	}
	return 1;
}

LDiscrim
Location::get_type()
{
	if ((kind & VAR_MASK) == L_UNKNOWN)
	{
		// should have symbolic name - parse
		char	*var_val;

		if (!locn.l_name || !get_str_val(0, 0, locn.l_name, var_val))
			return lk_none;
		if (isdigit(*var_val))
		{
			// line or address
			if (*var_val == '0')
			{
				// assume hex or octal
				if (file_name)
					return lk_none;
				
				kind |= L_ADDR;
				return lk_addr;
			}
			else
			{
				long l = strtol(var_val, 0, 0);
				if ((l > 999999) && !file_name)
				{
					// assume addr
					kind |= L_ADDR;
					return lk_addr;
				}
				// assume decimal
				kind |= L_LINE;
				return lk_stmt;
			}
		}
		else
		{
			kind |= L_FUNC;
			return lk_stmt;
		}
		
	}
	switch(kind & VAR_MASK)
	{
		case L_LINE:
			return lk_stmt;
		case L_ADDR:
			if (file_name)
				return lk_none;
			return lk_addr;
		case L_FUNC:
			return lk_fcn;
		default:
			return lk_none;
	}
}

int
Location::get_lwp(LWP *&l)
{
	if (!lwp)
	{
		l = 0;
		return 1;
	}
	if ((l = proglist.find_lwp(lwp)) == 0)
	{
		if (flags & L_CHECK_LWP)
		{
			// we really have a file name
			file_name = lwp;
			lwp = 0;
		}
		else
		{
			printe(ERR_object_unknown, E_ERROR, lwp);
			return 0;
		}
	}
	flags &= ~L_CHECK_LWP;
	return 1;
}

int
Location::get_file(LWP *l, Frame *f, char *&file)
{
	if (!file_name)
	{
		if (flags & L_CHECK_LWP)
		{
			// LWP may really be a file
			LWP	*l;

			flags &= ~L_CHECK_LWP;
			if ((l = proglist.find_lwp(lwp)) == 0)
			{
				file_name = lwp;
				lwp = 0;
			}
			else
			{
				file = 0;
				return 1;
			}
		}
		else
		{
			file = 0;
			return 1;
		}
	}
	if (!(flags & L_STRING) & (*file_name == '$' || *file_name == '%'))
		return get_str_val(l, f, file_name, file);
	file = file_name;
	return 1;
}

int 
Location::get_line(LWP *l, Frame *f, unsigned long &line)
{
	if (!(kind & L_LINE))
	{
		printe(ERR_internal, E_ERROR, "Location::get_line",
			__LINE__);
		return 0;
	}
	if (!(kind & L_VAR))
	{
		line = locn.l_val;
		return 1;
	}
	return get_int_val(l, f, locn.l_name, line);
}

int 
Location::get_addr(LWP *l, Frame *f, unsigned long &addr)
{
	if (!(kind & L_ADDR))
	{
		printe(ERR_internal, E_ERROR, "Location::get_addr",
			__LINE__);
		return 0;
	}
	if (!(kind & L_VAR))
	{
		addr = locn.l_val;
		return 1;
	}
	return get_int_val(l, f, locn.l_name, addr);
}

int 
Location::get_func(LWP *l, Frame *f, char *&name)
{
	if (!(kind & L_FUNC))
	{
		printe(ERR_internal, E_ERROR, "Location::get_func",
			__LINE__);
		return 0;
	}
	if (!(kind & L_VAR))
	{
		name = locn.l_name;
		return 1;
	}
	return get_str_val(l, f, locn.l_name, name);
}

int 
Location::get_offset(long &offval)
{
	unsigned long	ul;

	if (!(flags & (L_PLUS_OFF|L_MINUS_OFF)))
	{
		// one of these flags is set for symbolic offsets
		offval = off.o_val;
		return 1;
	}
	if (!get_int_val(0, 0, off.o_name, ul))
		return 0;

	offval = (long)ul;
	if (flags & L_MINUS_OFF)
		offval = -offval;
	return 1;
}

char *
Location::print()
{
	char	buf[BUFSIZ];
	char	*cur = buf;

	if (lwp)
		cur += sprintf(cur, "%s@", lwp);
	if (file_name)
		cur += sprintf(cur, "%s@", file_name);
	if ((kind & L_VAR) || (kind & L_FUNC))
		cur += sprintf(cur, "%s", locn.l_name);
	else if (kind & L_LINE)
		cur += sprintf(cur, "%d", locn.l_val);
	else
		cur += sprintf(cur, "%#x", locn.l_val);
	if (flags & L_PLUS_OFF)
		sprintf(cur, "+%s", off.o_name);
	else if (flags & L_MINUS_OFF)
		sprintf(cur, "-%s", off.o_name);
	else if (off.o_val != 0)
		sprintf(cur, "%+d", off.o_val);
	cur = new(char[strlen(buf) + 1]);
	strcpy(cur, buf);
	return cur;
}

char *
Location::print(LWP *l, Frame *f)
{
	char		buf[BUFSIZ];
	char		*cur = buf;
	LDiscrim	ltype = get_type();
	LWP		*ll;
	char		*name;
	unsigned long	num;
	long		off;

	if (ltype == lk_none)
		return 0;
	if (lwp)
	{
		get_lwp(ll);	
		if (ll)
			cur += sprintf(cur, "%s@", ll->lwp_name());
	}
	if (ltype != lk_addr)
	{
		if (file_name)
		{
			get_file(l, f, name);
			if (name)
				cur += sprintf(cur, "%s@", name);
		}
	}
	switch(ltype)
	{
		case lk_fcn:
			get_func(l, f, name);
			if (name)
				cur += sprintf(cur, "%s", name);
			break;
		case lk_stmt:
			get_line(l, f, num);
			cur += sprintf(cur, "%d", num);
			break;
		case lk_addr:
			get_addr(l, f, num);
			cur += sprintf(cur, "%#x", num);
			break;
		case lk_none:
		default:
			break;
	}
	if (ltype != lk_stmt)
	{
		// get offset, if any
		get_offset(off);
		if (off)
			sprintf(cur, "%+d", off);
	}
	cur = new(char[strlen(buf) + 1]);
	strcpy(cur, buf);
	return cur;
}
