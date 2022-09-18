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
#ident	"@(#)debugger:libexp/i386/CCevalMach.C	1.6"

#include <string.h>
#include "Language.h"
#include "Interface.h"
#include "CCtree.h"
#include "Value.h"
#include "LWP.h"
#include "Itype.h"
#include "Fund_type.h"
#include "TYPE.h"
#include "Reg.h"
#include "CCeval.h"

// Machine Dependent eval_node methods 
#define ALIGN4(X) ((X) & ~3)
#define ROUND4(X) (((X) + 3) & ~3)

int
node_eval::pushArgs(CCtree *zNode, Iaddr &wrk_sp)
{
	// on first pass save string literals in stack area
	CCtree* arg = zNode->operand[0];
	if( arg && arg->op==N_STRING )
	{
		// push string literal onto stack, replace N_STRING node
		// with a CONSTANT node that contains the string address
		push_val(arg->strval, strlen(arg->strval)+1, wrk_sp);
		arg = new CCtree(N_ICON,(int)wrk_sp,VT_ulong);
		arg->type = zNode->type;
	}

	// push args in reverse order 
	if( zNode->operand[1] != 0 )
	{
		if( !pushArgs(zNode->operand[1], wrk_sp) )
		{
			return 0;
		}
	}

	Value *argVal;
	if( !(argVal=eval(arg)) )
	{
		return 0;
	}

	Rvalue argRval;
	if( !argVal->rvalue(argRval) )
	{
		printe(ERR_arg_eval, E_ERROR);
		return 0;
	}
	Symbol		ptrUt;
	Fund_type	ft;
	if( arg->type.isPtrType(&ft, &ptrUt) &&
		 ptrUt.isArrayType() )
	{ 
		TYPE	rslt_type = ft_ulong;
		// push of an array argument should get its address
		Iaddr addr;
		addr = argVal->object().loc.addr; 
		argRval.reinit((void *)&addr, sizeof(long), rslt_type);
	}

	argRval.convert(zNode->type, lang);	// ZCONS node has formal...
						//      ... parameter's type
	if(!push_val(argRval.dataPtr(), argRval.type().size(), wrk_sp))
	{
		return 0;
	}

	// clean up string literal allocation
	if( arg != zNode->operand[0] )
		delete arg;

	return 1;
}

int
node_eval::setupFcnCall(CCtree *e, Rvalue &fcnAddr, Iaddr &retAddr)
{

	Itype spIval;
	if( !lwp->readreg(REG_ESP,Saddr, spIval) )
	{
		printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
		return 0;
	}
	Iaddr wrk_sp = ALIGN4( spIval.iaddr );

	Iaddr structSaveAddr = 0;
	if( e->type.isStruct() || e->type.isUnion() || e->type.isClass() )
	{
		// reserve space on stack for returned struct/union
		wrk_sp -= ROUND4(e->type.size());
		structSaveAddr = wrk_sp;
		spIval.iaddr = wrk_sp;
		if( !lwp->writereg(REG_ESP, Saddr, spIval) )
		{
			printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
			return 0;
		}
	}

	if( e->n_operands > 1 )
		pushArgs(e->operand[1], wrk_sp);

	if( structSaveAddr )
	{
        	// push address of return stucture save area
		if( !push_val((void *)&structSaveAddr, sizeof(Iaddr), wrk_sp) )
		{
			printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
			return 0;
		}
	}
		
	// push return address
	Itype ipIval;
	if( ((ipIval.iaddr = lwp->pc_value()) == 0) || 
	    !push_val((void *)&ipIval.iaddr, sizeof(Iaddr), wrk_sp) )
	{
		printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
		return 0;
	}
	retAddr = ipIval.iaddr;

	// update esp
	spIval.iaddr = wrk_sp;
	if( !lwp->writereg(REG_ESP, Saddr, spIval) )
	{
		printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
		return 0;
	}

        // pick up and set new ip
	Stype addrStype;
	addrStype = fcnAddr.get_Itype(ipIval);
	if( !lwp->set_pc(ipIval.iaddr))
	{
		printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
		return 0;
	}

	return 1;
}

int
node_eval::push_val(void *val, int size, Iaddr &stk_addr)
{
	stk_addr -= ROUND4(size); // word align stk_addr 
	
	if( !lwp->write(stk_addr, val, size) )
	{
		return 0;
	}
	return 1;
}

Value *
node_eval::getReturnVal(TYPE retTYPE)
{
	Rvalue rval;
	rval.null();

	Symbol retUt;
	Fund_type retFt;
	if( retTYPE.fund_type(retFt) || retTYPE.isPointer() )
	{
		Itype retIval;
		TYPE temptype; // To avoid C++ 2.0 errors
		switch(retFt)
		{
		case ft_sfloat:
		case ft_lfloat:
		case ft_xfloat:
			if( !lwp->readreg(REG_XR0, Sxfloat, retIval) )
			{
				return 0;
			}
			temptype = ft_xfloat;
			rval.reinit((void *)&retIval.rawbytes[0],
						     XFLOAT_SIZE , temptype);
			if( !rval.convert(retTYPE, lang) )
			{
				return 0;
			}
			break;
		case ft_void:
			// put something in rvalue to avoid error messages
			// and faults
			retIval.ichar = 0;
			temptype = ft_void;
			rval.reinit((void *)&retIval, 1, temptype);
			break;
		default:
			if( !lwp->readreg(REG_EAX, Suint4, retIval) )
			{
				return 0;
			}
			temptype = ft_ulong;
			rval.reinit((void *)&retIval.iuint4, retTYPE.size(), 
								 temptype);
			if( !rval.convert(retTYPE, lang) )
			{
				return 0;
			}
			break;
		}
		return new Value(rval);
	}
	else if( retTYPE.user_type(retUt) )
	{
		// pick-up (non-scalar) return value
		Itype spIval;
		if( !lwp->readreg(REG_ESP, Saddr, spIval) )
		{
			printe(ERR_call_stack, E_ERROR, lwp->lwp_name());
			return 0;
		}
		int sz = retTYPE.size();
		char *retArea = new char[sz];
		if( !lwp->read(spIval.iaddr, sz, retArea) )
		{
			return 0;
		}
                rval.reinit((void *)retArea, sz, retTYPE);
                delete retArea;

		// save (temp) stack addr in an Obj_info struct so it
		// can be used by the "." and "->" operators; nothing
		// had better happen to the stack between the call
		// (that just happened) and the "." or "->" operator
		Obj_info obj(lwp, retTYPE, spIval.iaddr);
	   	
		// reset stack pointer
		spIval.iaddr += ROUND4(sz);
		if( !lwp->writereg(REG_ESP, Saddr, spIval) )
		{
			printe(ERR_restore_stack, E_WARNING, lwp->lwp_name());
		}

		return new Value(obj, rval);
	}
	else
	{
		printe(ERR_internal, E_ERROR, "node_eval:eval", __LINE__);
		return 0;
	}
	// NOT REACHED
}

Fund_type 
node_eval::getOperationFT(TYPE &rsltTYPE)
{
	Stype resultStype;
	rsltTYPE.get_Stype(resultStype);

	switch(resultStype)
	{
        case Suchar:
        case Suint1:
        case Suint2:
        case Suint4:
	case Saddr:
	case Sdebugaddr:
		return ft_ulong;
        case Schar:
        case Sint1:
        case Sint2:
        case Sint4:
		return ft_long;
        case Ssfloat:
        case Sdfloat:
		return ft_lfloat;
        case Sxfloat:
		return ft_xfloat;
	default:
		printe(ERR_internal, E_ERROR, "node_eval::getOperationFT", __LINE__);
		return ft_none;
	}
}
