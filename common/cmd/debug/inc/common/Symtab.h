/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Symtab_h
#define Symtab_h
#ident	"@(#)debugger:inc/common/Symtab.h	1.3"

// Symtab - the mapping of a Symtable to a base address.
//
// A Symtab contains a base address and a pointer to a Symtable (which see).
// Its mission in life is to convert between the relative addresses in its
// Symtable and the virtual addresses in the LWP.  More than one Symtab
// may point to a given Symtable.  Symtab has no constructor; it is created
// only as a member of a Symnode (see Seglist.C).  Symtab has no destructor,
// since the Symtable it points to is never destroyed.
//
// Symtab supplies all of the member functions of Symtable.  Symtab is visible
// outside of libsymbol; Symtable is not.

#include	"Symbol.h"
#include	"Iaddr.h"
#include	"Fund_type.h"

class Evaluator;
class AddrEntry;
class NameEntry;
class Symtable;

class Symtab {
public:
	Iaddr		ss_base;
	Symtable *	symtable;
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

#endif /* Symtab_h */
