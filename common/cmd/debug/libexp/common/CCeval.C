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
#ident	"@(#)debugger:libexp/common/CCeval.C	1.14"

#include <errno.h>
#include "Language.h"
#include <string.h>
#include "Interface.h"
#include "CCtree.h"
#include "CC.h"
#include "Value.h"
#include "LWP.h"
#include "Itype.h"
#include "Fund_type.h"
#include "TYPE.h"
#include "Debug_var.h"
#include "List.h"
#include "CCeval.h"
#include "utility.h"
#include "cvt_util.h"
#include "Machine.h"
#include "fpemu.h"
#include "global.h"

Value *
CCeval(Language l, CCtree *etree, LWP *lwp, Frame *frame, int isEvent)
{
	Rvalue rval;

	node_eval ne(l, lwp, frame, isEvent);

	Value *result = ne.eval(etree);	// recursive evaluation.

	delete etree->nodeVal;
	if( !result )
		etree->nodeVal = 0;
	else
		etree->nodeVal = new Value(*result);

	TYPE& type = etree->type;		// top node gives result type.

	// -- get rvalue for all results now so that
	//    lwp motion has little or no effect.

	if (!result || !result->get_rvalue())
	{
		return 0; 
	}
	
	return result;
}

Value *
node_eval::eval_ID(CCtree *e)
{
	Symbol&    idSym = e->entry;
	Attribute *a;
	Execstate lwp_state;

	if( (lwp_state=lwp->get_state())==es_running ||
	                                             lwp_state==es_stepping )
	{
		if( !isEvent )
		{
			printe(ERR_invalid_op_running, E_ERROR,
				lwp->lwp_name());
			return 0;
		}
		else // expr is an event and lwp is running
		{
			return(e->nodeVal); // return previous value
		}
	}

	if( idSym.isEnumLitType() )
	{
		if ((a = e->entry.attribute(an_litvalue)) == 0)
		{
			printe(ERR_enum_val, E_ERROR, e->strval);
			return 0;
		}
		if (a->form != af_int)
		{
			printe(ERR_internal, E_ERROR, "node_eval::eval_ID",
				__LINE__);
			return 0;
		}
		Rvalue rval((void *)&a->value.word, sizeof(a->value.word), 
						                     e->type);
		return new Value(rval);
	}
	
	Obj_info object;

	if (! object.init(lwp, frame, idSym, idSym.base()))
	{
		if( !isEvent )
		{
			printe(ERR_obj_info, E_ERROR, e->strval);
		}
		return 0;
	}

	if( idSym.isSubrtn() || idSym.isLabel() )
	{
		// value of function is its address
		Rvalue rval((void *)&object.loc.addr, sizeof(Iaddr), e->type);
		return new Value(object, rval);
	}

	if( isEvent )
	{
		delete e->nodeVal;
		e->nodeVal = new Value(object); // Event expr, save value
	}

	return new Value(object);
}

Value *
node_eval::eval_At(CCtree *e)
{

	LWP	*exprLwp = lwp;	// save lwp and frame to restore ...
	Frame	*exprFrame = frame;	//  ... after subtree evaluation
	
	if( e->frame != 0 )
	{
		frame = e->frame;
	}

	if( e->lwp != 0 )
	{
		lwp = e->lwp;
		if( e->frame == 0 )
		{
			frame = e->lwp->curframe();
		}
	}

	Value *val = eval(e->operand[0]);

	lwp = exprLwp;
	frame = exprFrame;

	return val;
}

Value *
node_eval::eval_Assign(CCtree *e)
{
	Fund_type ft;
	Symbol ptrUt;
	Rvalue rval;

	Value *lhs = eval(e->operand[0]);
	Value *rhs = eval(e->operand[1]);

	if ( !lhs || !rhs || !rhs->rvalue(rval) )
	{
		delete lhs;
		delete rhs;
		return 0;
	}
	if( e->operand[1]->type.isPtrType(&ft, &ptrUt) &&
		 ptrUt.isArrayType() )
	{ 
		TYPE	rslt_type = ft_ulong;
		// assignment of an array should get its address
		Iaddr addr;
		addr = rhs->object().loc.addr; 
		rval.reinit((void *)&addr, sizeof(long), rslt_type);
	}

	if ( !rval.assign(lhs->object(), lang) )
	{
		delete lhs;
		delete rhs;
		return 0;
	}

	Value *newVal = new Value(lhs->object());
	delete lhs;
	delete rhs;
	return newVal;
}

Value *
node_eval::eval_AssignOp(CCtree *e)
{
	Value *lhs_val = eval(e->operand[0]);
	Value *rhs_val = eval(e->operand[1]);

	if( !lhs_val || !rhs_val )
	{
		delete lhs_val;
		delete rhs_val;
		return 0;
	}

	Rvalue rhs_rval,
	       lhs_rval;

	if ( !lhs_val->rvalue(lhs_rval) ) 
	{
		delete lhs_val;
		delete rhs_val;
		printe(ERR_assign_left_value, E_ERROR);
		return 0;
	}

	if ( !rhs_val->rvalue(rhs_rval) ) 
	{
		printe(ERR_assign_right_value, E_ERROR);
		delete lhs_val;
		delete rhs_val;
		return 0;
	}


	if( !rhs_rval.convert(lhs_rval.type(), C) )
	{
		printe(ERR_incomp_type, E_ERROR, e->opStr());
		delete lhs_val;
		delete rhs_val;
		return 0;
	}

	Operator op;
	switch(e->op)
	{
	case N_ASAND:	op = N_AND; break;
	case N_ASDIV:	op = N_DIV; break;
	case N_ASLS:	op = N_LS; break;
	case N_ASMINUS:	op = N_MINUS; break;
	case N_ASMOD:	op = N_MOD; break;
	case N_ASMUL:	op = N_MUL; break;
	case N_ASOR:	op = N_OR; break;
	case N_ASPLUS:	op = N_PLUS; break;
	case N_ASRS:	op = N_RS; break;
	case N_ASXOR:	op = N_XOR; break;
	}

	Value *result;
	result = apply_BinOp(op, &lhs_rval, &rhs_rval, e->type);

	if( !result || !result->assign(lhs_val->object(), lang) )
	{
		delete lhs_val;
		delete rhs_val;
		return 0;
	}

	delete lhs_val;
	delete rhs_val;
	return result;
}

// All debugger IDs could be handled identically, but they are different
// nodes to help the special cases.

Value *
node_eval::eval_reg_user_ID(CCtree *e)
{
	Obj_info object;

	// This is the second lookup!
        object.loc.var  = debug_var_table.Lookup(e->strval);

	object.type     = e->type;
        object.loc.kind = pDebugvar;

	if (e->op != N_USER_ID)
	{
		object.lwp  = lwp;
		object.frame    = frame;
		((Debug_var*)object.loc.var)->set_context(lwp, frame);
	}
	if (e->op == N_REG_ID)
	{
		if( isEvent )
		{
			// Event expr, save value
			delete e->nodeVal;
			e->nodeVal = new Value(object); 
		}
	}
	return new Value(object);
}

Value *
node_eval::eval_Select(CCtree *e)
{
	Value *lhs         = eval(e->operand[0]);
	char  *member_name = e->operand[1]->strval;

	if (lhs == 0) return 0;

	if (e->op == N_REF)
	{
		if (! lhs->deref(lwp, frame))
		{
			if( !isEvent )
			{
				printe(ERR_ptr_dereference, E_ERROR);
			}
			delete lhs;
			return 0;
		}
	}
	Obj_info& lhs_obj = lhs->object();

	if (lhs_obj.isnull())
	{
		if( !isEvent )
		{
			printe(ERR_select_lvalue, E_ERROR);
		}
		delete lhs;
		return 0;
	}
	if (lhs_obj.loc.kind != pAddress)
	{
		printe(ERR_select_register, E_ERROR);
		delete lhs;
		return 0;
	}
	Symbol& member     = e->operand[1]->entry;
	Obj_info      result_obj;
	if (! result_obj.init(lwp, frame, member, lhs_obj.loc.addr))
	{
		if( !isEvent )
		{
			printe(ERR_select_member, E_ERROR, e->operand[1]->strval);
		}
		delete lhs;
		return 0;
	}
	
	
	if( isEvent )
	{
		delete e->nodeVal;
		e->nodeVal = new Value(result_obj);
	}

	delete lhs;
	return new Value(result_obj);
}

Value *
node_eval::eval_MemberSelect(CCtree *e)
{
	// x.*y, x->*y

	// get base from left hand side
	Value *lhs = eval(e->operand[0]);
	if (lhs == 0)
	{
		return 0;
	}

	TYPE classType = e->operand[0]->type; 

	if (e->op == N_REFREF)
	{
		e->operand[0]->type.deref_type(classType);
		if (! lhs->deref(lwp, frame))
		{
			if( !isEvent )
			{
				printe(ERR_ptr_dereference, E_ERROR);
			}
			delete lhs;
			return 0;
		}
	}
	Obj_info& lhs_obj = lhs->object();

	// get offset from rhs
	Value *member = eval(e->operand[1]);
	Rvalue memberRval;
	if( !member->rvalue(memberRval) )
	{
		if( !isEvent )
		{
			printe(ERR_ref_ref_val, E_ERROR);
		}
		delete lhs;
		delete member;
		return 0;
	}
	long memberOffset;
	cvtTo_and_return_LONG(&memberRval, memberOffset);
	
	// validate offset (really offset + 1)
	// Insure that the member pointer points into the class.  To
	// do this validation correctly, we would have to insure the
	// pointer points to the start of a field and that the field
	// has the same type as the pointer.
	
	if( classType.size() < memberOffset )
	{
		printe(ERR_not_member, E_ERROR);
		delete lhs;
		delete member;
		return 0;
	}
	
	Obj_info result_obj;
	if( !result_obj.init(lwp, frame,
				lhs_obj.loc.addr+memberOffset-1, e->type) )
	{
		if( !isEvent )
		{
			printe(ERR_select_member, E_ERROR,
				e->operand[1]->strval);
		}
		delete lhs;
		delete member;
		return 0;
	}
	
	if( isEvent )
	{
		delete e->nodeVal;
		e->nodeVal = new Value(result_obj);
	}

	delete lhs;
	delete member;
	return new Value(result_obj);
}

Value *
node_eval::eval_Member(CCtree *e)
{
	// x::y
	// only x::nonStaticDataMembers get here, all others
	// rewritten in CCresovle
	TYPE	t(ft_ulong);
	Obj_info memberInfo;
	Iaddr ZeroBaseAddr = 0;

	if( !e->operand[1] )
	{
		// ::y (no class specified==>get global value)
		return eval(e->operand[0]);
	}

	// get offset of member in structure
	Symbol& member = e->operand[0]->entry;
	if (!memberInfo.init(lwp, frame, member, ZeroBaseAddr))
	{
		printe(ERR_internal,E_ERROR,"node_eval::eval_Member",__LINE__);
		return 0;
	}

	// rvalue is member offset in structure
	Rvalue rval((void *)&memberInfo.loc.addr, sizeof(Iaddr), t);

	if( isEvent )
	{
		delete e->nodeVal;
		e->nodeVal = new Value(rval);
	}

	return new Value(rval);
}

Value *
node_eval::eval_Deref(CCtree *e)
{
	Place result_loc;

	Value *value = eval(e->operand[0]);

	if (value == 0) return 0;

	// For *p these cases are allowed:
	//    1. p is a pointer          -- result lvalue is p's rvalue.
	//    2. p is an array           -- result lvalue is p's lvalue.
	//    3. p is an integer literal -- treat as a pointer to unsigned long.

	if (! value->deref(lwp, frame))
	{ // deref in place.
		if( !isEvent )
		{
			printe(ERR_ptr_dereference, E_ERROR);
		}
		delete value;
		return 0;
	}

	if( isEvent )
	{
		delete e->nodeVal;
		e->nodeVal = new Value(*value); // Event expression, save value
	}

	return value;
}

Value *
node_eval::eval_Index(CCtree *e)
{
	// handle a[1] or 1[a]
	Value	*lhs_val, *index_val;
	TYPE	t;

	if (e->operand[0]->type.deref_type(t))
	{
		lhs_val   = eval(e->operand[0]);
		index_val = eval(e->operand[1]);
	}
	else
	{
		index_val = eval(e->operand[0]);
		lhs_val   = eval(e->operand[1]);
	}

	if (lhs_val == 0 || index_val == 0)
	{
		delete lhs_val;
		delete index_val;
		return 0;
	}

	// For a[i] these cases are allowed:
	//    1. a is an array   -- result.loc = lhs.loc  + index*sizeof(a[0])
	//    2. a is a  pointer -- result.loc = lhs.rval + index*sizeof(*a)
	//
	Symbol lhs_type_sym;

	if (! lhs_val->deref(lwp, frame))
	{  // deref in place.
		if( !isEvent )
		{
			printe(ERR_ptr_dereference, E_ERROR);
		}
		delete lhs_val;
		delete index_val;
		return 0;
	}
	Obj_info elem_obj = lhs_val->object();  // setup for 1st element.
	long member_incr = e->type.size();

	if (member_incr <= 0)
	{
		printe(ERR_element_size, E_ERROR);
		delete lhs_val;
		delete index_val;
		return 0;
	}
	long hi_bound;
	Place p;
	if (!e->operand[0]->type.get_bound(an_hibound, p, hi_bound, 0, 0))
		hi_bound = -1;

	Rvalue rval;
	if (! index_val->rvalue(rval))
	{
		if( !isEvent )
		{
			printe(ERR_index_value, E_ERROR);
		}
		delete lhs_val;
		delete index_val;
		return 0;
	}

	long  index;
	if( !cvtTo_and_return_LONG(&rval, index) )
	{
		delete lhs_val;
		delete index_val;
		return 0;	// internal error (already reported)
	}
	
	if( hi_bound>0 && index>hi_bound )
	{
		if( !isEvent )
		{
			printe(ERR_array_bounds, E_WARNING, index, hi_bound);
		}
	}
	elem_obj.loc.addr += index * member_incr;

	if( isEvent )
	{
		delete e->nodeVal;
		e->nodeVal = new Value(elem_obj); // Event expr, save value
	}

	delete lhs_val;
	delete index_val;
	return new Value(elem_obj);
}

Value *
node_eval::eval_Constant(CCtree *e)
{
	void *base;
	int   size;

	switch (e->valkind)
	{
	case VT_char:
		base = (void *)&e->charval;
		size = sizeof(e->charval);
		break;
	case VT_long:
		base = (void *)&e->longval;
		size = sizeof(e->longval);
		break;
	case VT_ulong:
		base = (void *)&e->ulongval;
		size = sizeof(e->ulongval);
		break;
	case VT_float:
		base = (void *)&e->floatval;
		size = sizeof(e->floatval);
		break;
	case VT_xfloat:
		base = (void *)&e->rawbytes[0];
		size = XFLOAT_SIZE;
		break;

	case VT_strval:
	case VT_code:
	case VT_none:
		default:
		printe(ERR_internal, E_ERROR, "node_eval::eval_Constant", __LINE__);

	}
	if ((e->flags & (eHAS_PLACE|eHAS_RVALUE|eHAS_ADDRESS)) ==
		(eHAS_PLACE|eHAS_RVALUE|eHAS_ADDRESS) &&
		(e->valkind == VT_long || e->valkind == VT_ulong))
	{
		// constant address in event expression
		Execstate lwp_state;
		Obj_info	obj(lwp, e->type, e->ulongval);

		if( (lwp_state=lwp->get_state())==es_running ||
			lwp_state==es_stepping )
		{
			if( !isEvent )
			{
				printe(ERR_invalid_op_running, E_ERROR,
					lwp->lwp_name());
				return 0;
			}
			else // expr is an event and lwp is running
			{
				return(e->nodeVal); // return previous value
			}
		}
		return new Value(obj);
	}
	else
	{
		Rvalue rval(base, size, e->type);

		return new Value(rval);
	}
}

Value *
node_eval::eval_Sizeof(CCtree *e)
{
	TYPE& key_type = e->operand[0]->type;
	
	int result = key_type.isnull() ? 0 : key_type.size();

	if( result == 0 )
	{
		if( !isEvent )
		{
			printe(ERR_sizeof, E_ERROR);
		}
		return 0;
	}

	Rvalue rval((char *)&result, sizeof(result), e->type);
	return new Value(rval);
}

Value *
node_eval::eval_Addr(CCtree *e)
{
	Value *value = eval(e->operand[0]);
	if ( value == 0 ) return 0;

	Obj_info &obj = value->object();

	if ( obj.loc.kind == pRegister )
	{
		printe(ERR_reg_addr, E_ERROR);
		delete value;
		return 0;
	}
	else if ( obj.loc.kind != pAddress )
	{
		printe(ERR_internal, E_ERROR, "node_eval::eval_Addr", __LINE__);
		delete value;
		return 0;
	}

	Iaddr   result = (Iaddr)obj.loc.addr;

	delete value;

	Rvalue rval((char *)&result, sizeof(Iaddr), e->type);
	return new Value(rval);
}

Value *
node_eval::apply_BinOp(Operator op, Rvalue *lhs_rval, Rvalue *rhs_rval, 
							    TYPE result_type)
{
	// Evaluate all binary operators with arithmetic operands 
	// (i.e., doesn't handle add and subtract involving pointers)
	// Also supports relationals with debugger strings

	Fund_type operationFundType = getOperationFT(result_type);
	TYPE operationTYPE = operationFundType;

	Fund_type lhs_fund_type;
	Fund_type rhs_fund_type;

	Rvalue *rvalRslt;
	if( operationFundType == ft_ulong )
	{
		unsigned long ul_lhs,
		     	      ul_rhs;
		unsigned long ulRslt;
		if( !cvtTo_and_return_ULONG(lhs_rval, ul_lhs ) ||
			     !cvtTo_and_return_ULONG(rhs_rval, ul_rhs ) )
		{
			return 0;	// internal error (already reported)
		}

		switch( op )
		{
		case N_PLUS:	ulRslt = ul_lhs + ul_rhs; break;
		case N_MINUS:	ulRslt = ul_lhs - ul_rhs; break;
		case N_MOD:	ulRslt = ul_lhs % ul_rhs; break;
		case N_MUL:	ulRslt = ul_lhs * ul_rhs; break;
		case N_DIV:	ulRslt = ul_lhs / ul_rhs; break;
		case N_AND:	ulRslt = ul_lhs & ul_rhs; break;
		case N_OR:	ulRslt = ul_lhs | ul_rhs; break;
		case N_XOR:	ulRslt = ul_lhs ^ ul_rhs; break;
		case N_LS:	ulRslt = ul_lhs << ul_rhs; break;
		case N_RS:	ulRslt = ul_lhs >> ul_rhs; break;
		case N_LT:	ulRslt = ul_lhs < ul_rhs; break;
		case N_GT:	ulRslt = ul_lhs > ul_rhs; break;
		case N_LE:	ulRslt = ul_lhs <= ul_rhs; break;
		case N_GE:	ulRslt = ul_lhs >= ul_rhs; break;
		case N_EQ:	ulRslt = ul_lhs == ul_rhs; break;
		case N_NE:	ulRslt = ul_lhs != ul_rhs; break;
		case N_ANDAND:	ulRslt = ul_lhs && ul_rhs; break;
		case N_OROR:	ulRslt = ul_lhs || ul_rhs; break;
		}

		rvalRslt = new Rvalue((void *)&ulRslt,sizeof(ulRslt),
							operationTYPE );
	}
	else if( operationFundType == ft_long 
	&& lhs_rval->type().fund_type(lhs_fund_type) 
	&& lhs_fund_type == ft_string
	&& rhs_rval->type().fund_type(rhs_fund_type) 
	&& rhs_fund_type == ft_string)
	{
		int result = 
			strcmp((char*)lhs_rval->dataPtr(), 
				(char*)rhs_rval->dataPtr());
		switch( op )
		{
		case N_LT:	result = result  < 0; break;
		case N_GT:	result = result  > 0; break;
		case N_LE:	result = result <= 0; break;
		case N_GE:	result = result >= 0; break;
		case N_EQ:	result = result == 0; break;
		case N_NE:	result = result != 0; break;
		}
		rvalRslt = new Rvalue((void *)&result,sizeof(result),
							operationTYPE );
	}
	else if( operationFundType == ft_long )
	{
		long l_lhs,
		     l_rhs;
		long lRslt;
		if( !cvtTo_and_return_LONG(lhs_rval, l_lhs ) ||
		    		!cvtTo_and_return_LONG(rhs_rval, l_rhs ) )
		{
			return 0;	// internal error, already reported
		}

		switch( op )
		{
		case N_PLUS:	lRslt = l_lhs + l_rhs; break;
		case N_MINUS:	lRslt = l_lhs - l_rhs; break;
		case N_MOD:	lRslt = l_lhs % l_rhs; break;
		case N_MUL:	lRslt = l_lhs * l_rhs; break;
		case N_DIV:	lRslt = l_lhs / l_rhs; break;
		case N_AND:	lRslt = l_lhs & l_rhs; break;
		case N_OR:	lRslt = l_lhs | l_rhs; break;
		case N_XOR:	lRslt = l_lhs ^ l_rhs; break;
		case N_LS:	lRslt = l_lhs << l_rhs; break;
		case N_RS:	lRslt = l_lhs >> l_rhs; break;
		case N_LT:	lRslt = l_lhs < l_rhs; break;
		case N_GT:	lRslt = l_lhs > l_rhs; break;
		case N_LE:	lRslt = l_lhs <= l_rhs; break;
		case N_GE:	lRslt = l_lhs >= l_rhs; break;
		case N_EQ:	lRslt = l_lhs == l_rhs; break;
		case N_NE:	lRslt = l_lhs != l_rhs; break;
		case N_ANDAND:	lRslt = l_lhs && l_rhs; break;
		case N_OROR:	lRslt = l_lhs || l_rhs; break;
		}

		rvalRslt = new Rvalue((void *)&lRslt,sizeof(lRslt),
							operationTYPE );
	}
	else if( operationFundType == ft_lfloat )
	{
		double d_lhs,
		       d_rhs;
		double dRslt;

		if( !cvtTo_and_return_DOUBLE(lhs_rval, d_lhs ) ||
			    !cvtTo_and_return_DOUBLE(rhs_rval, d_rhs ) )
		{
			return 0;	// internal error, already reported
		}

		switch( op )
		{
		case N_PLUS:	dRslt = d_lhs + d_rhs; break;
		case N_MINUS:	dRslt = d_lhs - d_rhs; break;
		case N_MUL:	dRslt = d_lhs * d_rhs; break;
		case N_DIV:	dRslt = d_lhs / d_rhs; break;
		case N_LT:	dRslt = d_lhs < d_rhs; break;
		case N_GT:	dRslt = d_lhs > d_rhs; break;
		case N_LE:	dRslt = d_lhs <= d_rhs; break;
		case N_GE:	dRslt = d_lhs >= d_rhs; break;
		case N_EQ:	dRslt = d_lhs == d_rhs; break;
		case N_NE:	dRslt = d_lhs != d_rhs; break;
		case N_ANDAND:	dRslt = d_lhs && d_rhs; break;
		case N_OROR:	dRslt = d_lhs || d_rhs; break;
		case N_LS:
		case N_RS:
		case N_AND:
		case N_MOD:
		case N_OR:
		case N_XOR:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		rvalRslt = new Rvalue((void *)&dRslt, sizeof(dRslt),
							operationTYPE);
	}
	else if( operationFundType == ft_xfloat )
	{
		// cfront 1.2 doesn't recognize long double
		// use floating-point emulation package

		fp_x_t ld_lhs,
		       ld_rhs,
		       ld_tmp;
		fp_x_t ldRslt;
		int    cmp;
		Itype  itype;

		if( !cvtTo_and_return_LDOUBLE(lhs_rval, ld_lhs ) ||
			    !cvtTo_and_return_LDOUBLE(rhs_rval, ld_rhs ) )
		{
			return 0;	// internal error, already reported
		}

		errno = 0;
		switch( op )
		{
		case N_PLUS:	ldRslt = fp_add(ld_lhs, ld_rhs); 
				break;
		case N_MINUS:	ld_tmp = fp_neg(ld_rhs);
				ldRslt = fp_add(ld_lhs, ld_tmp);
				break;
		case N_MUL:	ldRslt = fp_mul(ld_lhs,ld_rhs);
				break;
		case N_DIV:	ldRslt = fp_div(ld_lhs,ld_rhs); 
				break;
		case N_ANDAND:	if (!fp_iszero(ld_lhs) && 
					(!fp_iszero(ld_rhs)))
					ldRslt = fp_one;
				else
					ldRslt = fp_zero;
				break;
				
		case N_OROR:	if (!fp_iszero(ld_lhs) ||
					(!fp_iszero(ld_rhs)))
					ldRslt = fp_one;
				else
					ldRslt = fp_zero;
				break;
		case N_EQ:
		case N_NE:
		case N_GT:
		case N_LT:
		case N_GE:
		case N_LE:	// relational operators
				cmp = fp_compare(ld_lhs, ld_rhs);
				if (errno)
					break;
				if (cmp == 0)
				{
					if (op == N_LE || op == N_GE ||
						op == N_EQ)
						ldRslt = fp_one;
					else
						ldRslt = fp_zero;
				}
				else if (cmp > 0)
				{
					if (op == N_GT || op == N_NE ||
						op == N_GE)
						ldRslt = fp_one;
					else
						ldRslt = fp_zero;
				}
				else
				{
					if (op == N_LT || op == N_NE ||
						op == N_LE)
						ldRslt = fp_one;
					else
						ldRslt = fp_zero;
				}
				break;
		case N_LS:
		case N_RS:
		case N_AND:
		case N_MOD:
		case N_OR:
		case N_XOR:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (errno)
		{
			printe(ERR_float_eval, E_ERROR);
			return 0;
		}
		cvt_to_internal(itype.rawbytes, ldRslt);
		rvalRslt = new Rvalue((void *)&(itype.rawbytes),
			XFLOAT_SIZE, operationTYPE);
	}
	switch(op)
	{
		default:
			break;
		case N_EQ:
		case N_NE:
		case N_GT:
		case N_LT:
		case N_GE:
		case N_LE:	
			result_type = ft_int;
			break;
	}
	if( !CCconvert(rvalRslt, result_type) )
		return 0;

	Value *val = new Value(*rvalRslt);
	delete rvalRslt;
	return val;
}


Value *
node_eval::eval_BinOp(CCtree *e)
{
	Value *lhs = eval(e->operand[0]);
	if ( !lhs ) 
		return 0;

	Value *rhs = eval(e->operand[1]);
	if ( !rhs )
	{
		delete lhs;
		return 0;
	}

	Rvalue lhs_rval;
	if ( !lhs->rvalue(lhs_rval) )
	{
		if( !isEvent )
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}

	Rvalue rhs_rval;
	if ( !rhs->rvalue(rhs_rval) )
	{
		if( !isEvent )
		{
			printe(ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}

	Value *result;
	result = apply_BinOp(e->op, &lhs_rval, &rhs_rval, e->type);

	delete lhs;
	delete rhs;

	return result;
}

unsigned long
ptrValToAddr(Symbol &ptrUt, Value *ptrVal, Iaddr &addr)
{
	if( ptrUt.isArrayType() )
	{ 
		addr = ptrVal->object().loc.addr; 
	}
	else
	{
		Rvalue ptr_rval;
		if ( !ptrVal->rvalue(ptr_rval) )
		{
			return 0;
		}
	
		Itype ptrIval;
		(void) ptr_rval.get_Itype(ptrIval);
		addr = ptrIval.iaddr; 
	}

	return 1;
}

// handle addition involving a pointer, eval_BinOp is called to handle
// addition of arithmetic operands
Value *
node_eval::eval_Plus(CCtree *e)
{
	Fund_type ft;
	Symbol ptrUt;
	CCtree *pointerOprnd,
	      *integralOprnd;

	if( e->operand[0]->type.isPtrType(&ft, &ptrUt) )
	{
		pointerOprnd = e->operand[0];
		integralOprnd = e->operand[1];
	}
	else if( e->operand[1]->type.isPtrType(&ft, &ptrUt) )
	{
		pointerOprnd = e->operand[1];
		integralOprnd = e->operand[0];
	}
	else
	{
		return eval_BinOp(e);
	}

	// debug variable pointer types have no user type
	if (ptrUt.isnull())
		return eval_BinOp(e);

	Value *ptrVal = eval(pointerOprnd);
	if ( !ptrVal ) 
	{
		return 0;
	}

	Iaddr ptr;
	if( !ptrValToAddr(ptrUt, ptrVal, ptr) )
	{
		if( !isEvent )
		{
			printe((pointerOprnd==e->operand[0]) ? ERR_op_left_side :
				ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete ptrVal;
		return 0;
	}

	Value *intVal = eval(integralOprnd);
	if ( !intVal ) 
	{
		delete ptrVal;
		return 0;
	}

	Rvalue int_rval;
	if ( !intVal->rvalue(int_rval) )
	{
		if( !isEvent )
		{
			printe((pointerOprnd==e->operand[0]) ? ERR_op_left_side :
				ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete ptrVal;
		delete intVal;
		return 0;
	}

	long int_l;
	if( !cvtTo_and_return_LONG(&int_rval, int_l ) )
	{
		delete ptrVal;
		delete intVal;
		return 0;	// internal error (already reported)
	}

	TYPE baseTYPE;
	if( !ptrUt.type(baseTYPE, an_basetype) )
	{
		if( !ptrUt.type(baseTYPE, an_elemtype) )
		{
			printe(ERR_internal, E_ERROR, "node_eval::eval_Plus", 
								     __LINE__);
			delete ptrVal;
			delete intVal;
			return 0;
		}
	}
	delete ptrVal;
	delete intVal;
	int incrUnit = baseTYPE.size();
	if( !incrUnit )
	{
		printe(ERR_pointer_arith, E_ERROR);
		return 0;
	}

	Iaddr rslt = ptr + int_l * incrUnit;

	Rvalue rsltRval((void *)&rslt, sizeof(long), e->type);

	return new Value(rsltRval);
}

Value *
node_eval::eval_Minus(CCtree *e)
{
	int rsltIsPtrdiff_t = 0;
	int decrUnit = 1;

	Fund_type lhs_ft;
	Symbol lhsUt;
	if( e->operand[0]->type.isPtrType(&lhs_ft, &lhsUt) && (!lhsUt.isnull()))
	{
		TYPE baseTYPE;
		if( !lhsUt.type(baseTYPE, an_basetype) )
		{
			if( !lhsUt.type(baseTYPE, an_elemtype) )
			{
				printe(ERR_internal, E_ERROR, 
					"node_eval::eval_Minus", __LINE__);
				return 0;
			}
		}
		decrUnit = baseTYPE.size();
	}
	else
	{
		return eval_BinOp(e);
	}

	// debug variable pointer types have no user type
	if (lhsUt.isnull())
		return eval_BinOp(e);

	Fund_type rhs_ft;
	Symbol rhsUt;
	if( e->operand[1]->type.isPtrType(&rhs_ft, &lhsUt) )
	{	// both operands are pointers;
		rsltIsPtrdiff_t = 1;
	}

	Value *lhs = eval(e->operand[0]);
	if( !lhs )
	{
		return 0;
	}

	unsigned long lhs_ul;
	if( !ptrValToAddr(lhsUt, lhs, lhs_ul) )
	{
		if( !isEvent )
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		return 0;
	}

	Value *rhs = eval(e->operand[1]);

	Rvalue 	*rsltRval;
	if( rsltIsPtrdiff_t )
	{	// both operands are pointers
		Iaddr rhs_ul;
		if( !ptrValToAddr(rhsUt, rhs, rhs_ul) )
		{
			if( !isEvent )
			{
				printe(ERR_op_right_side, E_ERROR, e->opStr());
			}
			delete lhs;
			delete rhs;
			return 0;
		}
		
		PTRDIFF rslt = (PTRDIFF)((lhs_ul - rhs_ul)/decrUnit);

		TYPE	iTYPE;
		iTYPE = ft_int;
		rsltRval = new Rvalue((void *)&rslt, sizeof(int), iTYPE);
	}
	else	// lhs is pointer, rhs is integral
	{	
		Rvalue rhs_rval;
		rhs->rvalue(rhs_rval);

		long rhs_l;
		if( !cvtTo_and_return_LONG(&rhs_rval, rhs_l) )
		{
			delete lhs;
			delete rhs;
			return 0;	// internal error (already reported)
		}

		Iaddr rslt = lhs_ul - rhs_l * decrUnit;

		rsltRval = new Rvalue((void *)&rslt, sizeof(Iaddr), e->type);
	}
	
	delete rhs;
	delete lhs;
	Value *val = new Value(*rsltRval);
	delete rsltRval;
	return(val);
}

Value *
node_eval::eval_CompareOp(CCtree *e)
{
	Fund_type lhs_ft, rhs_ft;
	Symbol	  lhs_ut, rhs_ut;
	Iaddr	  lhs_ul, rhs_ul;
	Value	  *lhs, *rhs = 0;

	if (!e->operand[0]->type.isPtrType(&lhs_ft, &lhs_ut) ||
		(lhs_ut.isnull()))
		return(eval_BinOp(e));
	if ((!e->operand[1]->type.isPtrType(&rhs_ft, &rhs_ut)) &&
		(e->operand[1]->op != N_ICON || 
			e->operand[1]->ulongval != 0))
	{
		// checked by CCresolve
		printe(ERR_internal, E_ERROR, 
			"node_eval::eval_CompareOp", __LINE__);
		return 0;
	}
	if ((lhs = eval(e->operand[0])) == 0)
		return 0;
	if ((e->operand[1]->op != N_ICON) &&
		((rhs = eval(e->operand[1])) == 0))
	{
		delete lhs;
		return 0;
	}
	if( !ptrValToAddr(lhs_ut, lhs, lhs_ul) )
	{
		if( !isEvent )
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}
	if (e->operand[1]->op == N_ICON)
	{
		// special case of comparison with 0
		rhs_ul = 0;
	}
	else if( !ptrValToAddr(rhs_ut, rhs, rhs_ul) )
	{
		if( !isEvent )
		{
			printe(ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}
	unsigned long ulRslt;
	delete lhs;
	delete rhs;
	switch(e->op)
	{
		case N_LT:	ulRslt = lhs_ul < rhs_ul; break;
		case N_GT:	ulRslt = lhs_ul > rhs_ul; break;
		case N_LE:	ulRslt = lhs_ul <= rhs_ul; break;
		case N_GE:	ulRslt = lhs_ul >= rhs_ul; break;
		case N_EQ:	ulRslt = lhs_ul == rhs_ul; break;
		case N_NE:	ulRslt = lhs_ul != rhs_ul; break;
		default:
				printe(ERR_internal, E_ERROR,
				"node_eval::eval_CompareOP",
				__LINE__);
				return 0;
	}
	TYPE itype;
	itype = ft_ulong;
	Rvalue rvalRslt((void *)&ulRslt, sizeof(ulRslt),
		itype);
	if (!CCconvert(&rvalRslt, e->type))
		return 0;

	return new Value(rvalRslt);
}

Value *
node_eval::eval_UnaryOp(CCtree *e)
{
	Value *oprnd = eval(e->operand[0]);
	if ( !oprnd ) 
		return 0;

	Rvalue oprnd_rval;
	if ( !oprnd->rvalue(oprnd_rval) )
	{
		if( !isEvent )
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
		}
		delete oprnd;
		return 0;
	}

	Fund_type operationFundType = getOperationFT(e->type);
	TYPE operationTYPE = operationFundType;

	Rvalue *rvalRslt;
	if( operationFundType == ft_ulong)
	{
		unsigned long ulOprnd;
		if( !cvtTo_and_return_ULONG(&oprnd_rval, ulOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_TILDE:	ulOprnd = ~ulOprnd; break;
		case N_UMINUS:	ulOprnd = -ulOprnd; break;
		case N_NOT:	ulOprnd = !ulOprnd; break;
		}
		rvalRslt = new Rvalue((void *)&ulOprnd,sizeof(ulOprnd),
							operationTYPE );
	}
	else if( operationFundType == ft_long)
	{
		long lOprnd;
		if( !cvtTo_and_return_LONG(&oprnd_rval, lOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_TILDE:	lOprnd = ~lOprnd; break;
		case N_UMINUS:	lOprnd = -lOprnd; break;
		case N_NOT:	lOprnd = !lOprnd; break;
		}
		rvalRslt=new Rvalue((void *)&lOprnd,sizeof(lOprnd),
							operationTYPE);
	}
	else if( operationFundType == ft_lfloat )
	{
		double dOprnd;
		if( !cvtTo_and_return_DOUBLE(&oprnd_rval, dOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_UMINUS:	dOprnd = -dOprnd; break;
		case N_NOT:	dOprnd = !dOprnd; break;
		case N_TILDE:	
			printe(ERR_internal, E_ERROR,
					"node_eval::eval_UnaryOp", __LINE__);
			delete oprnd;
			return 0;
		}
		rvalRslt = new Rvalue((void *)&dOprnd,sizeof(dOprnd),
							operationTYPE);
	}
	else if( operationFundType == ft_xfloat )
	{
		// cfront 1.2 doesn't recognize long double
		// use floating-point emulation
		fp_x_t ldOprnd, ldRslt ;
		Itype	itype;
		if( !cvtTo_and_return_LDOUBLE(&oprnd_rval, ldOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		errno = 0;
		switch(e->op)
		{
		case N_UMINUS:	ldRslt = fp_neg(ldOprnd); break;
		case N_NOT:	if (fp_iszero(ldOprnd))
					ldRslt = fp_one;
				else
					ldRslt = fp_zero;
				break;
		case N_TILDE:	
			printe(ERR_internal, E_ERROR,
					"node_eval::eval_UnaryOp", __LINE__);
			delete oprnd;
			return 0;
		}
		if (errno)
		{
			printe(ERR_float_eval, E_ERROR);
			delete oprnd;
			return 0;
		}
		cvt_to_internal(itype.rawbytes, ldRslt);
		rvalRslt = new Rvalue((void *)&(itype.rawbytes),
			XFLOAT_SIZE, operationTYPE);
	}

	delete oprnd;
	if( !CCconvert(rvalRslt, e->type) )
	{
		return 0;
	}

	Value *val = new Value(*rvalRslt);
	delete rvalRslt;
	return val;
}

Value *
node_eval::eval_NotOp(CCtree *e)
{
	Fund_type	ft;
	Symbol		ptrUt;

	if (!e->operand[0]->type.isPtrType(&ft, &ptrUt) ||
		ptrUt.isnull())
		return eval_UnaryOp(e);
	Value *ptrVal = eval(e->operand[0]);
	if (!ptrVal)
		return 0;
	
	Iaddr ptr;
	if( !ptrValToAddr(ptrUt, ptrVal, ptr) )
	{
		if( !isEvent )
		{
			printe( ERR_op_unary, E_ERROR, e->opStr());
		}
		return 0;
	}
	int result = !ptr;
	TYPE iTYPE;
	iTYPE = ft_int;
	Rvalue rsltRval((void *)&result, sizeof(result), iTYPE);
	return new Value(rsltRval);
}

Value *
node_eval::eval_PrePostOp(CCtree *e)
{
	Iaddr rslt;
	Fund_type ft;
	Symbol ut;
	Rvalue oprnd_rval;
	Value *oprnd = eval(e->operand[0]);
	if ( !oprnd ) 
		return 0;

	int inc_decUnit = 1;

	if( e->operand[0]->type.isPtrType(&ft, &ut) && !ut.isnull())
	{
		if( !ptrValToAddr(ut, oprnd, rslt) )
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
			delete oprnd;
			return 0;
		}
		oprnd_rval = Rvalue((void *)&rslt,
			e->operand[0]->type.size(), e->operand[0]->type);

		TYPE baseTYPE;
		if( !ut.type(baseTYPE, an_basetype) )
		{
			printe(ERR_internal, E_ERROR,
				"node_eval::eval_PrePostOp", __LINE__);
			return 0;
		}
		inc_decUnit = baseTYPE.size();
	}
	else
	{
		if ( !oprnd->rvalue(oprnd_rval) )
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
			delete oprnd;
			return 0;
		}
	}

	Rvalue *rsltRval;
	if( e->op == N_POSTMIMI || e->op == N_POSTPLPL )
	{
		rsltRval = &oprnd_rval;
	}
	
	Fund_type operationFundType = getOperationFT(e->type);
	TYPE operationTYPE = operationFundType;

	// apply operation
	Rvalue *tmpRval;
	if( operationFundType == ft_ulong)
	{
		unsigned long ulOprnd;
		if( !cvtTo_and_return_ULONG(&oprnd_rval, ulOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		switch(e->op)
		{
		case N_PREPLPL:	ulOprnd += inc_decUnit; break;
		case N_POSTPLPL:ulOprnd += inc_decUnit; break;
		case N_PREMIMI:	ulOprnd -= inc_decUnit; break;
		case N_POSTMIMI:ulOprnd -= inc_decUnit; break;
		}
		tmpRval = new Rvalue((void *)&ulOprnd,sizeof(ulOprnd),
							operationTYPE);
	}
	else if( operationFundType == ft_long)
	{
		long lOprnd;
		if( !cvtTo_and_return_LONG(&oprnd_rval, lOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		switch(e->op)
		{
		case N_PREPLPL:	lOprnd += inc_decUnit; break;
		case N_POSTPLPL:lOprnd += inc_decUnit; break;
		case N_PREMIMI:	lOprnd -= inc_decUnit; break;
		case N_POSTMIMI:lOprnd -= inc_decUnit; break;
		}
		tmpRval = new Rvalue((void *)&lOprnd,sizeof(lOprnd),
							operationTYPE);
	}
	else if( operationFundType == ft_lfloat )
	{
		double dOprnd;
		if( !cvtTo_and_return_DOUBLE(&oprnd_rval, dOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_PREPLPL:	dOprnd += 1; break;
		case N_POSTPLPL:dOprnd += 1; break;
		case N_PREMIMI:	dOprnd -= 1; break;
		case N_POSTMIMI:dOprnd -= 1; break;
		}
		tmpRval = new Rvalue((void *)&dOprnd,sizeof(dOprnd),
							operationTYPE);
	}
	else if( operationFundType == ft_xfloat )
	{
		// cfront 1.2 doesn't support long double
		// use floating point emulation
		fp_x_t ldOprnd, ldRslt;
		Itype	itype;
		if( !cvtTo_and_return_LDOUBLE(&oprnd_rval, ldOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		errno = 0;
		switch(e->op)
		{
		case N_PREPLPL:	 
		case N_POSTPLPL:
				ldRslt = fp_add(ldOprnd, fp_one);
				break;
		case N_PREMIMI:	
		case N_POSTMIMI:
				ldRslt = fp_add(ldOprnd, fp_neg_one);
				break;
		}
		if (errno)
		{
			printe(ERR_float_eval, E_ERROR);
			return 0;
		}
		cvt_to_internal(itype.rawbytes, ldRslt);
		tmpRval = new Rvalue((void *)&(itype.rawbytes),
			XFLOAT_SIZE, operationTYPE);
	}

	if( e->op ==  N_PREMIMI || e->op == N_PREPLPL )
	{
		rsltRval = tmpRval;
	}
	
	// save operand's new value
	if( !tmpRval->assign(oprnd->object(), lang) )
	{
		delete oprnd;
		return 0;
	}
	delete oprnd;
	
	// convert and return result value
	if( !CCconvert(rsltRval, e->type) )
		return 0;

	Value *val =  new Value(*rsltRval);
	delete rsltRval;
	return(val);
}

Value *
node_eval::eval_QuestionOp(CCtree *e)
{
	Value *cond = eval(e->operand[0]);
	if ( !cond ) 
		return 0;

	Rvalue cond_rval;
	if ( !cond->rvalue(cond_rval) )
	{
		if( !isEvent )
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
		}
		delete cond;
		return 0;
	}
	delete cond;

	unsigned long ulCond;
	if( !cvtTo_and_return_ULONG(&cond_rval, ulCond ) )
	{
		return 0;
	}

	if( ulCond )
	{
		return eval(e->operand[1]);
	}
	else
	{
		return eval(e->operand[2]);
	}
}

Value *
node_eval::eval_String(CCtree *e)
{
	Rvalue rval(e->strval, strlen(e->strval)+1, e->type);

	return new Value(rval);
}

Value *
node_eval::eval_COMop(CCtree *e)
{
	if( eval(e->operand[0]) == 0 )	// don't care what lhs value is ...
		return 0;		//    ... but it must exist

	Value *v = eval(e->operand[1]);
	return v;
}

Value *
node_eval::eval_Cast(CCtree *e)
{
	Value *v;
	Rvalue rval;

	if( (v = eval(e->operand[1])) == 0 )
		return 0;
	v->rvalue(rval);
	delete v; // don't need it, use copy of rval to build new Value
	if( rval.isnull() || !rval.convert(e->operand[0]->type, C) )
	{
		return 0;
	}

	Value *newVal = new Value(rval);
	return newVal;
}

Value *
node_eval::eval_Call(CCtree *e)
{

	if (!lwp->state_check(E_DEAD|E_CORE|E_RUNNING))
		return 0;

	Value *fcnVal;
	if( !(fcnVal=eval(e->operand[0])) ) 
	{
		return 0;
	}
	
	Rvalue fcnAddr;
	if( !fcnVal->rvalue(fcnAddr) )
	{
		printe(ERR_internal, E_ERROR, "node_eval:eval_Call", __LINE__);
		delete fcnVal;
		return 0;
	}

	Iaddr retAddr;
	if( !setupFcnCall(e, fcnAddr, retAddr) )
	{
		delete fcnVal;
		return 0;
	}

	lwp->set_ignore();			// disable events
	lwp->run(1, retAddr, V_QUIET);	// start it running

	// wait for it
	lwp->set_wait();
	waitlist.add(lwp);
	wait_process();

	lwp->clear_ignore();		// re-enable events
	if (get_ui_type() == ui_gui)
		printm(MSG_proc_stop_fcall, (Iaddr)lwp);

	delete fcnVal;

	// pick up return value 
	return getReturnVal(e->type);
}

Value *
node_eval::eval(CCtree *e)
{
	switch (e->op)
	{
	case N_AS:
		return eval_Assign(e);
	case N_ASAND:
	case N_ASDIV:
	case N_ASLS:
	case N_ASMINUS:
	case N_ASMOD:
	case N_ASMUL:
	case N_ASOR:
	case N_ASPLUS:
	case N_ASRS:
	case N_ASXOR:
		return eval_AssignOp(e);
	case N_ADDR:
		return eval_Addr(e);
	case N_DOT:
	case N_REF:
		return eval_Select(e);
	case N_DEREF:
		return eval_Deref(e);
	case N_INDEX:
		return eval_Index(e);
	case N_ID:
		return eval_ID(e);
	case N_AT:
		return eval_At(e);
	case N_REG_ID:
	case N_DEBUG_ID:
	case N_USER_ID:
		return eval_reg_user_ID(e);
	case N_ICON:
	case N_FCON:
	case N_CCON:
		return eval_Constant(e);
	case N_SIZEOF:
		return eval_Sizeof(e);
	case N_PLUS:
		return eval_Plus(e);
	case N_MINUS:
		return eval_Minus(e);
	case N_MUL:
	case N_MOD:
	case N_DIV:
	case N_AND:
	case N_OR:
	case N_XOR:
	case N_LS:
	case N_RS:
	case N_ANDAND:
	case N_OROR:
		return eval_BinOp(e);
	case N_LT:
	case N_GT:
	case N_LE:
	case N_GE:
	case N_EQ:
	case N_NE:
		return eval_CompareOp(e);
	case N_STRING:
		return eval_String(e);
	case N_COM:
		return eval_COMop(e);
	case N_TILDE:
	case N_UMINUS:
		return eval_UnaryOp(e);
	case N_NOT:
		return eval_NotOp(e);
	case N_UPLUS:
		return eval(e->operand[0]);
	case N_POSTMIMI:
	case N_POSTPLPL:
	case N_PREMIMI:
	case N_PREPLPL:
		return eval_PrePostOp(e);
	case N_QUEST:
		return eval_QuestionOp(e);
	case N_CAST:
		return eval_Cast(e);
	case N_TYPE:
	case N_AGGR:
	case N_DT_ARY:
	case N_DT_FCN:
	case N_DT_PTR:
	case N_DT_REF:
	case N_ENUM:
		// These operators are handled in CCresolve
		printe(ERR_internal, E_ERROR, "node_eval:eval", __LINE__);
		return 0;
	case N_CALL:
		return eval_Call(e);
	case N_DOTREF:
	case N_REFREF:
		return eval_MemberSelect(e);
	case N_MEM:
		return eval_Member(e);
	case N_ZCONS:
	case N_OPERATOR:
	case N_DELETE:
	case N_NEW:
	case N_NTYPE:
	case N_DT_MPTR:
	case N_ELLIPSIS:
	default:
		printe(ERR_opr_not_supported, E_ERROR, e->opStr());
	}
	return 0;
}
