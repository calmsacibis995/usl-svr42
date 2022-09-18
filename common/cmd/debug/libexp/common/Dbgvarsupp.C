/*
 * $Copyright: $
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

#ident	"@(#)debugger:libexp/common/Dbgvarsupp.C	1.2"

#include "Debug_var.h"
#include "Dbgvarsupp.h"
#include <stdlib.h> // for putenv
#include <string.h> // for strlen
#include "Interface.h"

//
// Support outside of expressions for debugger variables
//

//
// This class supports traversing the list of variables, and
// getting the attributes of a specific variable
//

static int
Included(Debug_var * var, char registers, char predefined, char user_defined)
{
	if (!var) 
		return 0;
	switch (var->var_class())
	{
	case reg_var: return registers;
	case debug_var: return predefined;
	case user_var: return user_defined;
	}
	return 0;
}

void
Debug_var_support::Find(const char * name)
{
	Debug_var * var = debug_var_table.Lookup(name);
	if (Included(var, registers, predefined, user_defined))
		current_variable = var;
	else
		current_variable = 0;
}

void
Debug_var_support::First()
{
	Debug_var * var;
	for (var = debug_var_table.First();
	     var && !Included(var, registers, predefined, user_defined);
	     var = debug_var_table.Next());
	current_variable = var;
}

void
Debug_var_support::Next()
{
	Debug_var * var;
	for (var = debug_var_table.Next();
	     var && !Included(var, registers, predefined, user_defined);
	     var = debug_var_table.Next());
	current_variable = var;
}

char * 
Debug_var_support::Name()
{
	if (current_variable)
		return debug_var_table.Lookup((Debug_var*)current_variable);
	else
		return 0;
}

char * value_buffer;

char * 
Debug_var_support::Value()
{
	delete value_buffer;
	if (!current_variable)
		return 0;

	Debug_var * var = (Debug_var*)current_variable;
		// to ease readability
	var->set_context(lwp, frame);
	if (var->isnull()) return 0;
	int size = var->size();
	long value;
	switch (var->fund_type())
	{
	case ft_string:
		value_buffer = new char[size];
		var->read_value(value_buffer, size);
		break;
	case ft_uint:
		var->read_value(&value, size);
		value_buffer = new char[16];
		sprintf(value_buffer, "%u", value);
		break;
	case ft_int:
	case ft_slong:
		var->read_value(&value, size);
		value_buffer = new char[16];
		sprintf(value_buffer, "%d", value);
		break;
	case ft_pointer:
		var->read_value(&value, size);
		value_buffer = new char[18];
		sprintf(value_buffer, "%#x", value);
		break;
	// No support for float registers
	default:
		return 0; // internal error
	}
	return value_buffer;
}

void Import_environment(const char ** environment)
{
	for ( ; *environment; environment++)
	{
		// name=value\0
		const char *entry = *environment;
		const char *cur = entry;
		int name_len = 0;
		for ( ; *cur && *cur != '='; cur++)
			;
		if (!*cur) 
			continue;
		name_len = cur - entry;
		// $name\0
		char *name = new char[1+name_len+1];
		name[0] = '$';
		memcpy(&name[1], entry, name_len);
		name[name_len+1] = 0;
		// Assumption: name is not already defined (how could it be?)
		Debug_var * var = debug_var_table.Enter(name);
		delete name;
		var->write_value((void *)(cur + 1), strlen(cur + 1) + 1);
	}
}

void Export_variable(const char * name)
{
	Debug_var * var = debug_var_table.Lookup(name);
	if (!name)
	{
		printe(ERR_user_var_defined, E_ERROR, name);
		return;
	}
	if (var->var_class() != user_var)
	{
		printe(ERR_export_type, E_ERROR, name);
		return;
	}
	// name=value\0
	name++; // omit the $
	int name_len = strlen(name);
	int value_len = var->size();
	// This space is never freed; putenv doesn't copy, but it doesn't
	// free the space either.
	char *s = new char[name_len+1+value_len];
	memcpy(s, name, name_len);
	s[name_len] = '=';
	var->read_value(&s[name_len+1], value_len);
	if (putenv(s) != 0)
	{
		printe(ERR_putenv, E_ERROR, s);
	}
}
