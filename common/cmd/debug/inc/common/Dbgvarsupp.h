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
#ifndef DEBUG_VAR_SUPPORT_H
#define DEBUG_VAR_SUPPORT_H

#ident	"@(#)debugger:inc/common/Dbgvarsupp.h	1.2"

class Frame;
class LWP;
//
// Support outside of expressions for debugger variables
//

//
// This class supports traversing the list of variables, and
// getting the attributes of a specific variable
//
class Debug_var_support 
{
private:
	LWP* lwp;
	Frame* frame;
 	char registers;
	char predefined;
	char user_defined;
	void * current_variable;
public:
	Debug_var_support(
	   LWP* lwp,
	   Frame* frame,
	   char include_registers = 1, 
	   char include_predefined = 1,
	   char include_user_defined = 1)
		{
			this->lwp = lwp;
			this->frame = frame;
			registers = include_registers;
			predefined = include_predefined;
			user_defined = include_user_defined;
			current_variable = 0;
		};
	~Debug_var_support() {}

	// Change current variable
	void Find(const char * name);
	void First();
	void Next();

	// Attributes of current variable
	char * Name();
	char * Value();
};

// Defines a user variable for each evironment variable
void Import_environment(const char ** environment);

// Exports the given user variable to the debugger environment.
// Reports any errors to the user
void Export_variable(const char * name);
#endif
