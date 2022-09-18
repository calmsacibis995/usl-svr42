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

//
// Class: ParsedRep 
//
// Member Functions (public):
//      eval	- pure virtual function called to invoke the appropriate
//		  derived class eval function.
//      triggerList - is called to handle a stop expression.  It traverses the
//                parsed expression represention and returns a list of Value
//                objects.  The value objects provide the the location and
//                (possibly null) value of each of the  expression's data
//                objects.
//	eventIsTrue -
//	clone
//
#ifndef PARSEDREP_H
#define PARSEDREP_H
#ident	"@(#)debugger:inc/common/ParsedRep.h	1.4"

#include "TYPE.h"
#include "List.h"
#include "LWP.h"

class	Place;
class	Value;
class	Resolver;

class ParsedRep
{
public:
		ParsedRep() {};
	virtual	~ParsedRep(void) {};

//
// Interface routines required by all language expression classes
//
	virtual	Value *eval(Language, LWP *, Frame *, int);
	virtual ParsedRep *clone();	// make deep copy
	virtual	int triggerList(Language, LWP *, Resolver *, List &);
	virtual	int exprIsTrue(Language, LWP *, Frame *);
	virtual int getTriggerLvalue(Place&);
	virtual int getTriggerRvalue(Rvalue&);
	virtual ParsedRep* copyEventExpr(List&, List&, LWP*);
};

#endif /* PARSEDREP_H */
