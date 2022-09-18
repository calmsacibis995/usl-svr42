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
#ifndef CCEVAL_H
#define CCEVAL_H

#ident	"@(#)debugger:libexp/common/CCeval.h	1.4"

#include "Iaddr.h"
#include "Language.h"
#include "TYPE.h"

class	CCtree;
class	Frame;
class	LWP;
class	Rvalue;

class node_eval
{
	Language lang;
	LWP	*lwp;
	Frame   *frame;
	int	isEvent;

	int pushArgs(CCtree *, Iaddr &);
	int setupFcnCall(CCtree *, Rvalue &, Iaddr &);
	int push_val(void *, int , Iaddr &);
	Value *getReturnVal(TYPE );
	Fund_type getOperationFT(TYPE &rsltTYPE);
	Value *eval_ID(CCtree *);
	Value *eval_At(CCtree *);
	Value *eval_reg_user_ID(CCtree *);
	Value *eval_Select(CCtree *);
	Value *eval_MemberSelect(CCtree *);
	Value *eval_Member(CCtree *e);
	Value *eval_Deref(CCtree *);
	Value *eval_Index(CCtree *);
	Value *eval_Constant(CCtree *);
	Value *eval_Sizeof(CCtree *e);
	Value *eval_Type(CCtree *e);
	Value *eval_Addr(CCtree *);
	Value *eval_Minus(CCtree *e);
	Value *eval_Plus(CCtree *e);
	Value *eval_BinOp(CCtree *);
	Value *eval_String(CCtree *);
	Value *eval_AssignOp(CCtree *);
	Value *eval_Assign(CCtree *);
	Value *eval_UnaryOp(CCtree *e);
	Value *eval_PrePostOp(CCtree *e);
	Value *eval_QuestionOp(CCtree *e);
	Value *eval_COMop(CCtree *);
	Value *eval_Cast(CCtree *);
	Value *eval_Call(CCtree *e);
	Value *eval_NotOp(CCtree *e);
	Value *eval_CompareOp(CCtree *e);

	Value *apply_BinOp(Operator, Rvalue *, Rvalue *, TYPE);

    public:
	node_eval(Language l, LWP *p, Frame *f, int evnt)
		{
			lang    = l;
			lwp 	= p;
			frame   = f;
			isEvent = evnt;
		}
	Value *eval(CCtree *);
};

#endif /* CCEVAL_H */
