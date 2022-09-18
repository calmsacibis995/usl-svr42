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
#ifndef CCTREE_H
#define CCTREE_H
#ident	"@(#)debugger:libexp/common/CCtree.h	1.4"

#include "Place.h"
#include "Symbol.h"
#include "TYPE.h"
#include "Resolver.h"
#include "ParsedRep.h"
#include "CC.h"
#include "TriggItem.h"
#include "Machine.h"

struct Value;
struct Const;
class  LWP;

// C/C++ specific expression trees

#undef OP_ENTRY
#define OP_ENTRY(e, s, t, v) e = v,
enum Operator {
#include "CCops.h"
};

#define EMAX_ARITY 3

enum ValKind {
    VT_char, 
    VT_long, 
    VT_ulong, 
    VT_float, 
    VT_xfloat,
    VT_code, 
    VT_strval, 
    VT_none
};

// defintions for flags member
#define eHAS_PLACE   0x1	// eval %r5  - place & value, but NO address.
#define eHAS_ADDRESS 0x2	// eval var  - place, address and value.
#define eHAS_RVALUE  0x4	// eval 17   - value only.

class CCtree : public ParsedRep
{
    public:
	Operator  op;
	int      n_operands;
	CCtree    *operand[EMAX_ARITY];
	int	flags;

	Symbol	entry;
	TYPE	type;

	Value	*nodeVal;
	LWP 	*lwp;		// subtree lwp; set for "@" nodes
	Frame	*frame;		// subtree frame; set for "@" nodes

	ValKind valkind;
	union
	{
		char		charval;
		long		longval;
		unsigned long 	ulongval;
		double		floatval; 
		int		code;     
		char 		*strval;   // string const or ID.
		char		rawbytes[XFLOAT_SIZE];
    	};

	CCtree() {};
	CCtree(CCtree *e);
	CCtree(Symbol &sym);
	CCtree(Operator op, int cnt, CCtree* = 0, CCtree* = 0, CCtree* = 0);
	CCtree(Const&);
	CCtree(Operator, int, ValKind);
	CCtree(Operator, char *);
	~CCtree();	// recursive delete

	CCtree *CCtree_append(CCtree *);
	ParsedRep *clone();
	CCtree& operator=(CCtree&);

	// Member Access methods
	CCtree *operator[](int i) 
		{ return i >= 0 && i < n_operands ? operand[i] : 0; }

	Value *eval(Language lang, LWP *lwp, Frame *frame, int isEvent=0);
	void	re_init(Operator op, int cnt, CCtree* = 0, CCtree* = 0, CCtree* = 0);
	void	re_init(Operator, char *);

	// Event Interface 
	int triggerList(Language, LWP *, Resolver *, List &);
	int getTriggerLvalue(Place&);
	int getTriggerRvalue(Rvalue&);
	int exprIsTrue(Language lang, LWP *lwp, Frame *frame);
	int setIdContext(TriggerItem &, LWP *, Frame *);
	ParsedRep* copyEventExpr(List&, List&, LWP*);

	char *opStr();
#ifdef DEBUG
	char *CCtree_op_string(Operator);
	void dump_CCtree(int = 1); // Might remove in final product.
	void dump_value(int level);
	void dump_etree_flags(int flags);
#endif

    private:
	void buildNewTrigList(List&, List&, CCtree*, LWP*);
	int buildTriggerList(Language, LWP *, Resolver *, List &, int markState);

};

extern char* getCCOperatorStr(Operator op);

#endif /* ETREE_H */
