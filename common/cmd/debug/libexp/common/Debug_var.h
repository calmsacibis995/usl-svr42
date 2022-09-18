/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ifndef DEBUG_VAR_H
#define DEGUG_VAR_H

#ident	"@(#)debugger:libexp/common/Debug_var.h	1.2"

//
// Support for debugger variables (predefined and user-defined)
//

#include "Fund_type.h"
#include "TYPE.h"
#include "List.h"

class LWP;
class Frame;

enum debug_var_class {null_var, reg_var, debug_var, user_var};

class Debug_var
// 
// Responsible for maintaining the value of the variable.
//
{
protected:
	LWP		*lwp;
	Frame		*frame;
public:
			Debug_var() { lwp = 0; frame = 0;}
			~Debug_var() {}

	void		set_context(LWP * the_process,
				Frame * the_frame)
				{lwp = the_process; frame = the_frame;};

	// Subclasses may need to override these:
	virtual Fund_type	fund_type(void);
	virtual int	size();
	virtual int	isnull();

	// Subclasses *must* override these:
	virtual debug_var_class	var_class() = 0;
	virtual int	read_value(void * value, int byte_count) = 0;
	virtual int	write_value(void * value, int byte_count) = 0;
};

class Debug_var_table
//
// Responsible for maintaining the list of variables.
//
{
private:
	List	table;
	void	*current;
	void	Enter(const char * name, Debug_var * var); 
		// for any debug variable
		// okay if variable was previously defined
public:
		Debug_var_table();
			// enters predefined variables (including regs)
		~Debug_var_table() {}

		Debug_var * Enter(const char * name);  
			// for new user defined variables
			   // okay if variable was previously defined
			   // name must be a legal user defined 
			   // variable name
	Debug_var *Lookup(const char * name); // for any debug variable
	char *	Lookup(Debug_var * var);
		// mostly useful for error reporting

	Debug_var *First(void);
	Debug_var *Next(void);
};

extern Debug_var_table debug_var_table;

#endif
