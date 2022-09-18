/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Symtable_h
#define Symtable_h
#ident	"@(#)debugger:libsymbol/common/Symtable.h	1.2"

// Symtable - the repository for Symbols.
//
// A Symtable contains only a pointer to its Evaluator.  It provides a set
// of member functions for querying the Symbol graph.
//
// A Symtable differs from a Symtab (which see) in that the Symtable is
// associated with the object, not the process.  The addresses in a Symtable
// are RELATIVE addresses; the Symtab converts them to RELOCATED addresses,
// as appropriate.  Several Symtabs may point to a single Symtable, and the
// various Symtabs may have different base addresses.  The attribute lists
// contain only relative addresses which are relocated as necessary.
//
// Symtables are created only by the Object constructor and are never destroyed.

#include	"Symbol.h"
#include	"Source.h"
#include	"Fund_type.h"

class Evaluator;
class AddrEntry;
class NameEntry;
class Object;

class Symtable {
	Evaluator *	evaluator;
public:
			Symtable( int, Object * );
			~Symtable();
	Symbol		first_symbol();
	Symbol		find_scope ( Iaddr );
	Symbol		find_entry ( Iaddr );
	Symbol		find_symbol ( Iaddr );
	Symbol		find_global( const char * );
	int 		find_source( Iaddr, Symbol & );
	int 		find_source( const char *, Symbol & );
	NameEntry *	first_global();
	NameEntry *	next_global();
	NameEntry *	prev_global();
	Symbol		global_symbol( NameEntry * );
};

#endif /* Symtable_h */
