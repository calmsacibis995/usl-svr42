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

#ident	"@(#)debugger:libexp/common/Debug_var.C	1.5.1.1"

#include "Interface.h"
#include "Itype.h"
#include "str.h"
#include "Debug_var.h"
#include "TYPE.h"
#include "Frame.h"
#include "Reg.h"
#include "Language.h"
#include "Parser.h"
#include "utility.h" // %line, %file, %func
#include "global.h" // %num_lines, %num_bytes, %global_path, %follow
#include "Event.h" // %thisevent, %lastevent
#include "Proglist.h" // %program, %proc, %lwp
#include "edit.h" // %mode
#include "Input.h" // %prompt
#include "Program.h"
#include "Vector.h"
#include <string.h>
#include <LWP.h>


#ifndef __cplusplus
// C++ 1.2 does not support pure virtual functions
debug_var_class Debug_var::var_class() { return null_var; };
int Debug_var::read_value(void * , int ) {return 0;};
int Debug_var::write_value(void * , int ) {return 0;};
#endif

#ifdef DEBUG
int debugflag = 0;
#endif

Debug_var_table debug_var_table;

Fund_type
Debug_var::fund_type(void)
{
	return ft_int;
}

int 
Debug_var::size() 
{
	return TYPE(fund_type()).size();
}

int
Debug_var::isnull() 
{
	return frame == 0;
}

//
// Debugger variable flavors:
//

class Debug_var_register: public Debug_var
{
private:
	RegRef the_reg;
public:
	Debug_var_register(RegRef reg) {the_reg = reg;};
	Fund_type fund_type(void) {return regtype(the_reg);};
	// size: use the default

	debug_var_class var_class() {return reg_var;};
	int read_value(void * value, int byte_count);  // see below
	int write_value(void * value, int byte_count); // see below
};

int 
Debug_var_register::read_value(void * value, int )
{
	Stype stype;
	TYPE(fund_type()).get_Stype(stype);
	Itype * iptr = (Itype *)value;
	if (frame->readreg(the_reg, stype, *iptr) == 0)
	{
		printe(ERR_read_reg, E_ERROR, lwp->lwp_name());
		return 0;
	}
	return 1;
};

int 
Debug_var_register::write_value(void * value, int )
{
	Stype stype;
	TYPE(fund_type()).get_Stype(stype);
	if (frame->writereg(the_reg, stype, *(Itype*)value) == 0)
	{
		printe(ERR_write_reg, E_ERROR, lwp->lwp_name());
		return 0;
	}
	return 1;
};

#define Debug_vtype -1

class Debug_var_debug: public Debug_var
{
private:
	dbg_vtype which_var;
	char * value(int report_error); 
		// int values are returned as the pointer itself
public:
	Debug_var_debug(dbg_vtype var) {which_var = var;};

        // See below.
	Fund_type fund_type(void);
	int       size(void);
	int       isnull(void);
	debug_var_class var_class() {return debug_var;};
	int       read_value(void * value, int byte_count);
	int       write_value(void * value, int byte_count);
};

Fund_type 
Debug_var_debug::fund_type(void)
{
	switch (which_var)
	{
	case PLoc:
		return ft_pointer;
	case Debug_vtype: 
	case PResult:
		return ft_int;
	case Lastevent:
	case Lline:
	case Line:
	case Num_bytes:
	case Num_lines:
	case PFrame:
	case Thisevent:
		return ft_uint;
	case Db_lang:
	case File:
	case Follow:
	case Func:
	case Glob_path:
	case Lang:
	case Lfile:
	case Lwp:
	case Mode:
	case Path:
	case Proc:
	case Pprogram:
	case Prompt:
	case PRedir:
	case Verbose:
	case Wait:
		return ft_string;
	}
	return ft_none;
}

int 
Debug_var_debug::isnull(void)
{
	switch (which_var)
	{
	case Debug_vtype:
	case Db_lang:
	case Follow:
	case Glob_path:
	case Lang:
	case Lastevent:
	case Mode:
	case Num_bytes:
	case Num_lines:
	case PResult:
	case Prompt:
	case PRedir:
	case Thisevent:
	case Verbose:
	case Wait:
		return 0;
	case Lfile:
	case Lline:
	case Lwp:
	case Path:
	case Proc:
	case Pprogram:
		return lwp == 0;
	case File:
	case Func:
	case Line:
	case PFrame:
	case PLoc:
		Execstate state = lwp->get_state();
		return lwp == 0 || frame == 0
		|| state == es_running
		|| state == es_stepping
		|| state == es_none
		|| state == es_dead;
	}
	return 1;
}

int 
Debug_var_debug::size(void)
{
	switch (fund_type())
	{
	case ft_int:
	case ft_uint:
	case ft_pointer:
		return Debug_var::size();
	case ft_string:
		{
		char * s = value(0);
		return strlen(s)+1;
		}
	default:
		return 0;
	}
}

static const char * yes_no_table[]  = {"no", "yes", 0};
static const char * wait_table[]    = {"background", "foreground", 0};
static const char * follow_table[]  = {"none", /*"threads",*/ "procs", "all", 0};
static const char * verbose_table[] = {"quiet", "source", "events", 
					"reason", "all", 0};
static const char * table_to_5[]    = {"0", "1", "2", "3", "4", "5", 0};
// "5" is available only for internal use = high verbosity

inline const char *
debug_enum_string(int value, const char * table[])
{
	// assumes value is in range
	return table[value];
}

static int
debug_enum_value(const char * string, const char * table[])
{
	int entry = 0;
	while (table[entry])
	{
		if (strcmp(table[entry], string) == 0) 
			return entry;
		entry++;
	}
	return -1;
}

inline
char *
null_check(char * s)
{
	if (s) 
		return s;
	else
		return "";
}

char * // int values are returned as the pointer itself
Debug_var_debug::value(int report_error)
{
	char *fname, *filename;
	long line;
	switch(which_var)
	{
#ifdef DEBUG
	case Debug_vtype:
		return (char *) debugflag;
#endif
	case Db_lang:	
		return null_check((char *)language_name(
			current_context_language(lwp)));
	case Func:	
		if (!lwp)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		current_loc(lwp, lwp->curframe(), filename, fname, line);
		return null_check(fname);
	case File:	
		if (!lwp)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		current_loc(lwp, lwp->curframe(), filename, fname, line);
		return null_check(filename);
	case Follow:
		return (char *)debug_enum_string(follow_mode, follow_table);
	case Glob_path: 
		return null_check(global_path);
	case Lang:      
		return null_check((char *)language_name(
			current_user_language()));
	case Lastevent:	
		return (char *)m_event.last_event();
	case Lfile:	
		if (!lwp)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		return null_check(lwp->curr_src());
	case Line:
		if (!lwp)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		current_loc(lwp, lwp->curframe(), filename, fname, line);
		return (char *)line;
	case Lline:
		if (!lwp)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		line = lwp->current_line();
		return (char *)line;
	case Lwp:
	{
		LWP	*l = proglist.current_lwp();
		if (!l)
		{
			if (report_error) printe(ERR_no_proc, E_WARNING);
			return "";
		}
		else 
			return (char *)l->lwp_name();
	}
	case Mode:	
		return null_check((char *)get_mode());
	case Num_bytes:
		return (char *)num_bytes;
	case Num_lines:
		return (char *)num_line;
	case Path:
		{
			if (!lwp)
			{
				if (report_error) printe(ERR_no_proc, E_ERROR);
				return "";
			}
			return null_check((char *)lwp->process()->program()->src_path());
		}
	case PFrame:	
		if (!lwp)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		return (char *)curr_frame(lwp);
	case PLoc:	
		if (!lwp || lwp->get_state() == es_dead)
		{
			if (report_error) printe(ERR_no_proc, E_ERROR);
			return "";
		}
		return (char *)lwp->curframe()->pc_value();
	case Pprogram:
	{
		Program	*p = proglist.current_program();
		if (!p)
		{
			if (report_error) printe(ERR_no_proc, E_WARNING);
			return "";
		}
		else 
			return null_check((char *)p->prog_name());
	}
	case PRedir:	
		return (char *)debug_enum_string(redir_io, yes_no_table);
	case Proc:
	{
		Process	*p = proglist.current_process();
		if (!p)
		{
			if (report_error) printe(ERR_no_proc, E_WARNING);
			return "";
		}
		else 
			return null_check((char *)p->proc_name());
	}
	case Prompt:	
		return (char *)Pprompt;
	case PResult:
		return (char *)cmd_result;
	case Thisevent:
		return (char *)m_event.this_event();
	case Verbose:	
		return (char *)debug_enum_string(vmode, verbose_table);
	case Wait:	
		return (char *)debug_enum_string(wait_4_proc, wait_table);
	default:	
		return "";
	}
}

int       
Debug_var_debug::read_value(void * value, int byte_count)
{
	char * s = this->value(1);
	switch (fund_type())
	{
	case ft_int:
	case ft_uint:
	case ft_pointer:
		memcpy(value, &s, byte_count);
		return byte_count;
	case ft_string:
		{
		memcpy(value, s, byte_count);
		return byte_count;
		}
	default:
		return 0;
	}
}

static Vector str_vector;

// values are not null terminated

inline static char *
string_value(void * value, int byte_count)
{
	char	zero = 0;
	str_vector.clear();
	str_vector.add(value, byte_count);
	str_vector.add((void*)&zero, 1);
	return((char *)str_vector.ptr());
}

int       
Debug_var_debug::write_value(void * value, int byte_count)
{
// The string is not null-terminated.
	char *s = 0;
	char	*name;
#define CVALUE string_value(value, byte_count)
#define IVALUE *(int *)value
	switch(which_var) {
#ifdef DEBUG
	case Debug_vtype:
		debugflag = IVALUE;
		break;
#endif
	case Follow:
		{
		int enum_value;
		enum_value = debug_enum_value(CVALUE, follow_table);
		if (enum_value < 0)
			printe(ERR_debug_var_val, E_ERROR, "%follow");
		else
			follow_mode = enum_value;
		break;
		}
	case Func:
		if (!lwp)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		if (lwp->get_state() == es_running
		|| lwp->get_state() == es_stepping)
		{
			printe(ERR_invalid_op_running, E_ERROR, lwp->lwp_name());
			return 0;
		}
		set_curr_func(lwp, CVALUE);
		break;
	case PFrame:
		if (!lwp)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		if (lwp->get_state() == es_running
		|| lwp->get_state() == es_stepping)
		{
			printe(ERR_invalid_op_running, E_ERROR, lwp->lwp_name());
			return 0;
		}
		set_frame(lwp, IVALUE);
		break;
	case Lang:
		set_language(CVALUE);
		break;
	case Mode: 
		(void)set_mode(CVALUE);
		break;
	case Num_bytes:
		if (IVALUE < 0)
			printe(ERR_invalid_num, E_ERROR);
		else
			num_bytes = IVALUE;
		break;
	case Num_lines: 
		if (IVALUE < 0)
			printe(ERR_invalid_num, E_ERROR);
		else
			num_line = IVALUE;
		break;
	case Lfile:
		if (!lwp)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		lwp->set_current_stmt(CVALUE, 1);
		break;
	case Lline:
	{
		char *fname;
		if (!lwp)
		{
			printe(ERR_no_proc, E_ERROR);
			return 0;
		}
		fname = lwp->curr_src();
		lwp->set_current_stmt(fname, IVALUE);
		break;
	}
	case Lwp:
	{
		LWP	*l;

		name = CVALUE;
		if (!name)
			break;
		l = proglist.find_lwp(name);
		if (!l)
			printe(ERR_no_match, E_ERROR, name);
		else
			proglist.set_current(l, 1);
		break;
		
	}
	case Proc:
	{
		Process	*proc;

		name = CVALUE;
		if (!name)
			break;
		proc = proglist.find_proc(name);
		if (!proc)
			printe(ERR_no_match, E_ERROR, name);
		else
			proglist.set_current(proc, 1);
		break;
		
	}
	case Pprogram:
	{
		Program	*prog;

		name = CVALUE;
		if (!name)
			break;
		prog = proglist.find_prog(name);
		if (!prog)
			printe(ERR_no_match, E_ERROR, name);
		else
			proglist.set_current(prog, 1);
		break;
		
	}
	case Path:
		if (!lwp)
		{
			printe(ERR_no_proc, E_ERROR);
				return 0;
		}
		// set_path deletes the current string
		name = CVALUE;
		lwp->process()->program()->set_path(makestr(name));
		break;
	case Glob_path:
		delete global_path;
		name = CVALUE;
		global_path = makestr(name);
		pathage++;	// to invalidate cached source files
		break;
	case Prompt:
		// Should reclaim space, but first value is not 
		// dynamically allocated.
		// delete Pprompt;
		name = CVALUE;
		Pprompt = makestr(name);
		break;
	case Verbose:
		if (get_ui_type() == ui_gui)
		{
			printe(ERR_debug_var_set,E_ERROR,
				debug_var_table.Lookup(this));
			break;
		}
		else
		{
		int enum_value;
		name = CVALUE;
		enum_value = debug_enum_value(name, verbose_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, table_to_5);
		if (enum_value < 0)
			printe(ERR_debug_var_val, E_ERROR, "%verbose");
		else
			vmode = enum_value;
		}
		break;
	case PRedir:
		if (get_ui_type() == ui_gui)
		{
			printe(ERR_debug_var_set,E_ERROR,
				debug_var_table.Lookup(this));
			break;
		}
		else
		{
		int enum_value;
		name = CVALUE;
		enum_value = debug_enum_value(name, yes_no_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, table_to_5);
		if (enum_value < 0 || enum_value > 1)
			printe(ERR_debug_var_val, E_ERROR, "%redir");
		else
			redir_io = enum_value;
		break;
		}
	case Wait:
		int enum_value;
		name = CVALUE;
		enum_value = debug_enum_value(name, yes_no_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, wait_table);
		if (enum_value < 0)
			enum_value = debug_enum_value(name, table_to_5);
		if (enum_value < 0 || enum_value > 1)
			printe(ERR_debug_var_val, E_ERROR, "%wait");
		else
			wait_4_proc = enum_value;
		break;
	default:
		printe(ERR_debug_var_set,E_ERROR,debug_var_table.Lookup(this));
		break;
	}
	return byte_count;
}

class Debug_var_user: public Debug_var
{
private:
	char * current_value;
	int    current_size;
	int    current_alloc;
public:
	Debug_var_user() {current_value = 0; current_size = current_alloc = 0;};

        // See below.
	Fund_type fund_type(void) 	{return ft_string;};
	int       size(void) 		{return current_size;}
	int       isnull(void) 		{return current_value == 0;};
	debug_var_class var_class()     {return user_var;};
	int       read_value(void * value, int )
			{
				memcpy(value, current_value, current_size);
				return current_size;
			};
	int       write_value(void * value, int byte_count);
};

int       
Debug_var_user::write_value(void * value, int byte_count)
{
	// Ensure value is *null-terminated* string
	if (!current_value 
		|| (byte_count+1) > current_alloc)
	{
		delete current_value;
		current_value = new char[byte_count+1];
		current_alloc = byte_count + 1;
	}
	memcpy(current_value, value, byte_count);
	if (current_value[byte_count-1] != 0)
	{
		current_value[byte_count] = 0;
		current_size = byte_count+1;
	}
	else
		current_size = byte_count;
	return byte_count;
}

// C++ 2.1 workaround
extern RegAttrs regs[];

Debug_var_table::Debug_var_table()
{
#ifdef DEBUG
	Enter("%debug",     new Debug_var_debug(-1));
#endif
	Enter("%db_lang",     new Debug_var_debug(Db_lang));
	Enter("%file",        new Debug_var_debug(File));
	Enter("%follow",      new Debug_var_debug(Follow));
	Enter("%frame",       new Debug_var_debug(PFrame));
	Enter("%func",        new Debug_var_debug(Func));
	Enter("%global_path", new Debug_var_debug(Glob_path));
	Enter("%lang",        new Debug_var_debug(Lang));
	Enter("%lastevent",   new Debug_var_debug(Lastevent));
	Enter("%line",        new Debug_var_debug(Line));
	Enter("%list_file",   new Debug_var_debug(Lfile));
	Enter("%list_line",   new Debug_var_debug(Lline));
	Enter("%loc",         new Debug_var_debug(PLoc));
#if 0
	Enter("%lwp",         new Debug_var_debug(Lwp));
#endif
	Enter("%mode",        new Debug_var_debug(Mode));
	Enter("%num_bytes",   new Debug_var_debug(Num_bytes));
	Enter("%num_lines",   new Debug_var_debug(Num_lines));
	Enter("%path",        new Debug_var_debug(Path));
	Enter("%proc",        new Debug_var_debug(Proc));
	Enter("%program",     new Debug_var_debug(Pprogram));
	Enter("%prompt",      new Debug_var_debug(Prompt));
	Enter("%redir",       new Debug_var_debug(PRedir));
	Enter("%result",      new Debug_var_debug(PResult));
	Enter("%thisevent",   new Debug_var_debug(Thisevent));
	Enter("%verbose",     new Debug_var_debug(Verbose));
	Enter("%wait",        new Debug_var_debug(Wait));

	// Enter register debug variables.
	RegAttrs * a_reg = regs;
	while (a_reg->ref != REG_UNK)
	{
		Enter(a_reg->name, new Debug_var_register(a_reg->ref));
		a_reg++;
	}
};

class DV_table_entry
{
public:
	char * name;
	Debug_var * var;
};

void
Debug_var_table::Enter(const char * name, Debug_var * var)
{
	// Assumes that name has not already been entered.
	DV_table_entry * new_entry = new DV_table_entry;
	new_entry->name = str(name);
	new_entry->var = var;
	DV_table_entry * insert_before = (DV_table_entry*)table.first();
	while (insert_before && strcmp(name, insert_before->name) > 0)
		insert_before = (DV_table_entry*)table.next();
	table.insert((void *)new_entry);
};

Debug_var *
Debug_var_table::Enter(const char * name)
{
	// Assumes that name has not already been entered.
	// create a new user defined variable
	// name[0] = $ (else error: illegal name)
	if (name[0] != '$')
	{
		printe(ERR_internal, E_ERROR, "Debug_var_table::Enter",
			__LINE__);
		return 0;
	};

	Debug_var_user * user_var = new Debug_var_user();
	Enter(name, user_var); 
	return user_var;
};

Debug_var *
Debug_var_table::Lookup(const char * name)
{
	// This adds all names to the hash table, even if the lookup fails.
	char * name_ptr = str(name);
	DV_table_entry * entry = (DV_table_entry *) table.first();
	while (entry)
	{
		if (entry->name == name_ptr) return entry->var;
		if (strcmp(name, entry->name) < 0 ) break;
		entry = (DV_table_entry *) table.next();
	}
	// Add $ names on lookup; the value is null until assignment
	if (name[0] == '$')
		return Enter(name);
	else
		return 0;
};

char *
Debug_var_table::Lookup(Debug_var * var)
{
	DV_table_entry * entry = (DV_table_entry *) table.first();
	while (entry)
	{
		if (var == entry->var) return entry->name;
		entry = (DV_table_entry *) table.next();
	}
	return 0;
};

Debug_var *
Debug_var_table::First()
{
	DV_table_entry * entry = (DV_table_entry*)table.first();
	if (!entry) 
		return 0;
	else if (entry->var->var_class() == user_var && entry->var->isnull()) 
		return Next();
	else 
		return entry->var;
}

Debug_var *
Debug_var_table::Next()
{
	DV_table_entry * entry = (DV_table_entry*)table.next();
	while (entry && entry->var->var_class() == user_var 
	&& entry->var->isnull())
		entry = (DV_table_entry*)table.next();
	if (entry) return entry->var;
	else       return 0;
}
