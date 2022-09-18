/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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
#ifndef EXPR_H
#define EXPR_H
#ident	"@(#)debugger:inc/common/Expr.h	1.4"


// Class: Expr
//	Expr is a language independent container class for language expressions.
//
// Public Member Functions:
//	Expr 	- constructors; non-copy constructor is used to instantiate
//		  an expr object.  This constructor initializes the string
//		  expression representation and the langauge discriminate.
//	eval	- evaluates an expression by invoking the eval member function
//		  of the object pointed to by ParsedRep.  
//		  A return value of zero 
//		  indicates the evaluation failed, while non-zero indicates
//		  the evaluation was sucessful.  Upon sucessful return, the 
//		  result of the evaluation is available through calls to rvalue
//		  and lvalue.
//	rvalue	- returns, in its parameter rval, the rvalue of a previous 
//		  expression evaluation.  If no
//		  rvalue is avaiable, then the function call returns zero.
//		  Otherwise, non-zero is returned.
//	lvalue	- returns, in its parameter lval, the lvalue of a previous 
//		  expression evaluation. If no
//		  lvalue is avaiable, then the function call returns zero.
//		  Otherwise, non-zero is returned.
//	string	- returns a pointer to the character string representation
//		  of the expression.
//	triggerList - traverses the parsed expression represention and 
//		  returns a list of TriggerItem objects.  The objects provide 
//		  the location and (possibibly null) value of each of the
//		  data items that can affect the value of the expression.
//	exprIsTrue - evaluates the expression and returns a non-zero 
//		  value if the expression value constitutes a true value in
//		  the expression's source langauge.  Otherwise, the function
//		  returns zero.
//


#include "Iaddr.h"
#include "Language.h"
#include "ParsedRep.h"

class  Frame;
class  Rvalue;
struct Place;
class  Symbol;
class  Value;
class  LWP;
class  Resolver;

class Expr
{
    private:
	char		*estring;	// expression text
	Language	lang;		
	ParsedRep	*etree;		// parsed representation of expression
	Value		*value;
	int		flags;		// event or no-resolve expr

    public:
			Expr(char *e, LWP* lwp, int isEvent = 0);
			Expr(Symbol&, LWP* lwp);
			~Expr();
			Expr & operator=(Expr &e);

	// Member access methods
	char 		*string() { return estring; }

	// General expression evaluation methods
	int 		parse(Resolver*);
	int 		eval(LWP *lwp=0,Iaddr pc=~0,Frame *frame=0);
	int		rvalue(Rvalue& rval);
	int		lvalue(Place &lval);

	// Event interface methods
	int		triggerList(LWP *lwp, Iaddr pc, List &valueList);
	int		exprIsTrue(LWP *lwp, Frame *frame)
				{ return etree->exprIsTrue(lang, lwp, frame); }
	Expr*		copyEventExpr(List&, List&,  LWP*);
};

extern const char *print_type(Symbol&, Language);
#endif
