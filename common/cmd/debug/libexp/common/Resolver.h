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

#ifndef RESOLVER_H
#define RESOLVER_H

#ident	"@(#)debugger:libexp/common/Resolver.h	1.4"


#include "Iaddr.h"
#include "Symbol.h"
#include "Fund_type.h"
#include "utility.h"

class LWP;
class TYPE;

//-- Resolver provides a map between identifiers and symbol table
//   entries.  It is attempts to hide all language dependent scope
//   rules and various details of symbol table structure.
//   Languages that need their own versions derive them from Resolver

enum searchType {mFULL_SEARCH, mLOCAL_ONLY};

class Resolver
{
protected:
	searchType	mode;
	LWP		*lwp;
	Symbol		curScope;
	Symbol		top_scope;
	Symbol		curSymbol;
	int		inGlobalSearch;

	virtual int	find_global(const char *id);
	virtual int	cmpName(const char*, Symbol&);
	int		find_static(const char *id, Symtype );
	int		searchOutwardToTopScope(const char*, Symbol&, 
				Symbol&, Symtype);
	int		searchScope(const char* id, Symbol& scope, 
				Symtype user_def_type)
			{ return searchSiblingChain(id, scope.child(),
				user_def_type); 
			}
	int		searchSiblingChain(const char*, Symbol&, 
				Symtype);
	int		find_enumlit(Symbol& enumtype, const char * id, 
				Symbol& litsym);
	int		getEnclosingFunc(Symbol&);
	int		checkTag(Symtype user_def_type, Symbol &);
public:
			Resolver(LWP *, Iaddr);
			Resolver(Symbol&, LWP *);
	virtual		~Resolver() {}
			Resolver& operator=(Resolver &r)
			{ mode = r.mode;
			  lwp = r.lwp;
			  top_scope = r.top_scope;
			  curScope = r.curScope;
			  inGlobalSearch = 0;
			  return *this;
			};

	LWP 		*proc() { return this->lwp; };

	int		getEnclosingSrc(Symbol &srcfile);
	virtual	int	getNext(const char *, Symbol&);
	virtual int 	lookup(const char *, Symbol&, 
				Symtype user_def_type=st_notags);

};

// C++ specific Resolver class

#define THIS_NM "this"

// cfront 2.1 requires base class name in derived class constructor,
// 1.2 forbids it
#ifdef __cplusplus
#define RESOLVER	Resolver
#else
#define RESOLVER
#endif

class CCresolver : public Resolver {
	int		find_global(const char *id);
	int		cmpName(const char*, Symbol&);
	char		*buildClassQualifiedName(const char*, Symbol &);
public:
			CCresolver(LWP *lwp, Iaddr addr) : 
				RESOLVER(lwp, addr) {}
			CCresolver(Symbol&sym, LWP *lwp) : 
				RESOLVER(sym, lwp) {}
			~CCresolver() {}
			CCresolver& operator=(CCresolver &r)
			{ mode = r.mode;
			  lwp = r.lwp;
			  top_scope = r.top_scope;
			  curScope = r.curScope;
			  inGlobalSearch = 0;
			  return *this;
			};
	int		inMemberFunction(Symbol&, Symbol&);
	int		getNext(const char *, Symbol&);
	int 		lookup(const char *, Symbol&,
				Symtype user_def_type=st_notags);
};

#endif
