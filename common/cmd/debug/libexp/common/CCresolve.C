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
#ident	"@(#)debugger:libexp/common/CCresolve.C	1.13"

#include "Debug_var.h"
#include "Language.h"
#include "Interface.h"
#include "CCtree.h"
#include "CC.h"
#include "Resolver.h"
#include "TYPE.h"
#include "LWP.h"
#include "Proglist.h"
#include "Frame.h"
#include "Symbol.h"
#include "Fund_type.h"
#include "Tag.h"
#include "Source.h"
#include "CCtokdefs.h"
#include "utility.h"
#include "cvt_util.h"
#include "Machine.h"
#include "Buffer.h"
#include "str.h"
#include "Value.h"
#include "Rvalue.h"
#include "Parser.h"
#include <limits.h>
#include <ctype.h>
#include <string.h>

#define ELLIPSIS_STR	"..."

// The resolver traverses the expression tree and performs these 
// functions:
//	1. Reports any semantic errors
//	2. Sets entry and type information necessary for evaluation.
//	3. Does minor rewrites, like operator change, in some cases.


enum C_TypeClass {C_integral, C_arithmetic };
// The order of the Match enumeration literals is important

class nodeResolve
{
    public:
		nodeResolve(Language lang, int isEvent)
			{ this->lang = lang;
			  this->isEvent = isEvent;
			}

		~nodeResolve(void) {}

		resolve(CCtree* e, Resolver* context);

    private:
	Language lang;
	int isEvent;

	Fund_type standardConversionType(Fund_type *lhs_ft, Fund_type *rhs_ft);
	void buildNodeList(CCtree *e, List &nodeList);
	int resolve_BinaryOpArgs(CCtree &e, Resolver* context);
	int twoIntegralArgs(CCtree& e, Fund_type& lhs_ft, Fund_type& rhs_ft);
	int twoArithArgs(CCtree &e, Fund_type &lhs_ft, Fund_type &rhs_ft);
	int resolve_AssignOprnds(CCtree& e, Resolver* context);
	int isFuncProtoType(CCtree& e, Resolver* context);
	int buildProtoFuncId(CCtree& e);
	void replaceWithFuncIdNode(CCtree&, CCtree*);
	unsigned int matchArgTypes(CCtree*&, Symbol&);
	void markParamTypes(CCtree*, Symbol&);
	int typeName_cat(Buffer*, CCtree*);
	int do_Select(CCtree& e, Resolver* context);
	int do_MemberSelect(CCtree& e, Resolver* context);
	int do_Member(CCtree& e, Resolver* context);
	int do_Deref(CCtree& e, Resolver* context);
	int do_Addr(CCtree& e, Resolver* context);
	int do_Index(CCtree& e, Resolver* context);
	int do_Constant(CCtree& e);
	int do_Sizeof(CCtree& e, Resolver* context);
	int do_AddOp(CCtree &e, Resolver *context);
	int do_SubOp(CCtree &e, Resolver *context);
	int do_BinOp(CCtree &e, Resolver* context, C_TypeClass argClass);
	int do_CompareOp(CCtree &e, Resolver *context);
	int do_LogicOp(CCtree &e, Resolver *context);
	int do_UnaryOp(CCtree &e, Resolver* context, C_TypeClass argClass);
	int do_PrePostOp(CCtree &e, Resolver* context);
	int do_NotOp( CCtree &e, Resolver* context);
	int do_ID(CCtree &e, Resolver* context);
	int buildQualifier(CCtree *e, char *&qualifier);
	int collectQualifiers(CCtree &e, char *qualifier[], CCtree *&idNode);
	int validProcId(char *qualifier, LWP *&lwp);
	int getFuncQualifier(CCtree &e, char *fname, Resolver *&rslvr,
		int dodelete);
	int getFrameContext(CCtree &e, long frameNbr, 
		Resolver *&rslvr, int dodelete);
	int getBlockContext(long lineNbr, Resolver *&rslvr,int dodelete );
	int getQualifiedContext(CCtree &e, char *qualifier[],
				Resolver *context, Resolver *&newContext);
	int do_At(CCtree &e, Resolver* context);
	int do_Assign(CCtree& e, Resolver* context);
	int do_AssignAddOp(CCtree& e, Resolver* context);
	int do_AssignOp(CCtree& e, Resolver* context, C_TypeClass argClass);
	int do_COMop(CCtree &e, Resolver* context);
	int do_QuestionOp(CCtree &e, Resolver* context);
	int do_Type(CCtree& e, Resolver* context);
	int do_Call(CCtree& e, Resolver* context);
	int do_Cast(CCtree &e, Resolver *context);
};


int
CCresolve(Language lang, CCtree* e, Resolver *context, int isEvent)
{
	nodeResolve nr(lang, isEvent);

	return nr.resolve(e, context);
}

Fund_type
nodeResolve::standardConversionType(Fund_type *lhs_ft, Fund_type *rhs_ft)
{
	if( rhs_ft == 0 )
	{
		// for unary ops
		rhs_ft = lhs_ft;
	}

	// change type of enums to int

	if( *lhs_ft == ft_none )
	{
		*lhs_ft = ft_int;
	}

	if( *rhs_ft==ft_none )
	{
		*rhs_ft = ft_int;
	}

	
	// change signed forms to generic forms
	
	if( *lhs_ft == ft_slong )
		*lhs_ft = ft_long;
	else if( *lhs_ft == ft_sint )
		*lhs_ft = ft_int;
	
	if( *rhs_ft == ft_slong )
		*rhs_ft = ft_long;
	else if( *rhs_ft == ft_sint )
		*rhs_ft = ft_int;

	
	// return "Usual arithmetic conversion" type (ANSI X3J11 3.2.1.5)
	
	if( *rhs_ft==ft_xfloat || *lhs_ft==ft_xfloat )
		return ft_xfloat;
	else if( *rhs_ft==ft_lfloat || *lhs_ft==ft_lfloat )
		return ft_lfloat;
	else if( *rhs_ft==ft_sfloat || *lhs_ft==ft_sfloat )
		return ft_sfloat;
	else if( *rhs_ft==ft_ulong || *lhs_ft==ft_ulong )
		return ft_ulong;
	else if( (*lhs_ft==ft_long && *rhs_ft==ft_uint) ||
		 (*rhs_ft==ft_long && *lhs_ft==ft_uint) )
#if LONG_MAX >= UINT_MAX
		return ft_long;
#else
		return ft_ulong;
#endif
	else if( *lhs_ft==ft_long || *rhs_ft==ft_long )
		return ft_long;
	else
		return ft_int;
	
}

//
// Type building routines
//

// Linearize N_TYPE subtree, makes it easier to process
void
nodeResolve::buildNodeList(CCtree *e, List &nodeList)
{

	if( e->n_operands == 2 )
	{
		buildNodeList(e->operand[0], nodeList);
		buildNodeList(e->operand[1], nodeList);
	}
	else if( e->n_operands == 1 )
	{
		buildNodeList(e->operand[0], nodeList);
	}
	else if( e->n_operands != 0 )
	{
		printe(ERR_internal, E_ERROR, "buildNodeList", __LINE__);
		return;
	}

	if( e->op == N_AGGR || e->op == N_ENUM || e->op == N_DT_PTR || 
	    e->op == N_DT_REF || e->op == N_DT_ARY || e->op == N_DT_FCN ||
	    e->op == N_DT_MPTR || (e->op == N_TYPE &&  e->n_operands==0) )
	{
		nodeList.add((void *)e);
	}
	else if( e->op == N_DT_PTR )
	{
		printe(ERR_operator_support, E_WARNING, "::*");
	}
}

// merge two fundemental; ft2 gives a base type while ft1 modifies (or 
// overrides) the base type.  For example, if ft2 is char (ft_char) ft1 
// could be signed (ft_sint) or unsigned (ft_uint) modifying the char to
//  ft_schar or ft_uchar respectively.
static int
mergeFundTypes(Fund_type &rsltFT, Fund_type ft1, Fund_type ft2)
{
	switch(ft2)
	{
	case ft_char:
		switch(ft1)
		{ 
		case ft_sint: rsltFT = ft_schar; break;
		case ft_uint: rsltFT = ft_uchar; break;
		default: return 0;
		}
		break;
	case ft_short:
		switch(ft1)
		{
		case ft_sint: rsltFT = ft_sshort; break;
		case ft_uint: rsltFT = ft_ushort; break;
		default: return 0;
		}
		break;
	case ft_int:
		switch(ft1)
		{
		case ft_char: 
		case ft_schar:
		case ft_uchar:
		case ft_short:
		case ft_sshort:
		case ft_ushort:
		case ft_long:
		case ft_slong:
		case ft_ulong:
			rsltFT = ft1; break;
		default: return 0;
		}
		break;
	case ft_long:
		switch(ft1)
		{
		case ft_sint: rsltFT = ft_slong; break;
		case ft_uint: rsltFT = ft_ulong; break;
		default: return 0;
		}
		break;
	case ft_lfloat:
		if( ft1 == ft_long ) 
		{
			rsltFT = ft_xfloat;
		}
		else
		{
			return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

//
// Operator argument handling routines
//

int
nodeResolve::resolve_BinaryOpArgs(CCtree &e, Resolver* context)
{
	if( !resolve(e[0], context) || !resolve(e[1], context) )
		return 0;

	if( !(e[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_lhs_novalue, E_ERROR, e.opStr());
		return 0;
	}

	if( !(e[1]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_rhs_novalue, E_ERROR, e.opStr());
		return 0;
	}
	return 1;
}

int
nodeResolve::twoIntegralArgs(CCtree& e, Fund_type& lhs_ft, Fund_type& rhs_ft)
{
	if( !e[0]->type.isIntegral(&lhs_ft) )
	{
		printe(ERR_lhs_not_integral, E_ERROR, e.opStr());
		return 0;
	}

	if( !e[1]->type.isIntegral(&rhs_ft) )
	{
		printe(ERR_rhs_not_integral, E_ERROR, e.opStr());
		return 0;
	}
	return 1;
}

int
nodeResolve::twoArithArgs(CCtree &e, Fund_type &lhs_ft, Fund_type &rhs_ft)
{
	if( !e[0]->type.isArithmetic(&lhs_ft) )
	{
		printe(ERR_lhs_arithmetic, E_ERROR, e.opStr());
		return 0;
	}

	if( !e[1]->type.isArithmetic(&rhs_ft) )
	{
		printe(ERR_rhs_arithmetic, E_ERROR, e.opStr());
		return 0;
	}
	return 1;
}

int
nodeResolve::resolve_AssignOprnds(CCtree& e, Resolver* context)
{
	if( !resolve(e[0], context) || !resolve(e[1], context) )
		return 0;

	if( !(e[0]->flags&eHAS_PLACE) )
	{
		printe(ERR_opd_not_lvalue, E_ERROR, e.opStr());
		return 0;
	}

	if( !(e[1]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_rhs_novalue, E_ERROR, e.opStr());
		return 0;
	}

	return 1;
}

int 
nodeResolve::isFuncProtoType(CCtree& e, Resolver* context)
{

	switch( e.op )
	{
	case N_AGGR:
	case N_ENUM:
	case N_DT_PTR:
	case N_DT_REF:
	case N_DT_ARY:
	case N_DT_FCN:
	case N_TYPE:
	case N_ZCONS:
		for( int i = 0; e.operand[i]; i++ )
		{
			if( !isFuncProtoType(*e[i], context) )
			{
				return 0;
			}
		}
		break;
	case ID:
		if( !context->lookup(e.strval, e.entry, st_usertypes ) )
		{
			return 0;
		}
		break;
	default:
		return 0;
	}

	return 1;
}

// uses 2nd level global buffer
int 
nodeResolve::buildProtoFuncId(CCtree& e) 
{
	gbuf2.clear();
	register CCtree *func = e[0];
	gbuf2.add(func->strval);

	// build the parameter signature
	gbuf2.add('(');

	register CCtree *zconsNode = e[1];
	do
	{
		if( zconsNode->op != N_ZCONS )
		{
			printe(ERR_prototype, E_ERROR);
			return 0;
		}

		if( !typeName_cat(&gbuf2,zconsNode->operand[0]) )
		{
			printe(ERR_internal, E_ERROR,
				"nodeResolve::buildProtoFuncId", __LINE__);
			return 0;
		}

		zconsNode = zconsNode->operand[1];
		if( zconsNode )
		{
			gbuf2.add(',');
		}
	} while( zconsNode );
	gbuf2.add(')');

	e.re_init(N_ID, (char *)gbuf2);

	return 1;
}

// Resolver returns symbol table names which have prototype
// information - this function strips the prototype
void
nodeResolve::replaceWithFuncIdNode(CCtree& oldnode, CCtree* e)
{
	char *symNm = e->entry.name();
	Symbol	oldentry = e->entry;
	TYPE	oldtype = e->type;
	int	oldflags = e->flags;

	// if name is prototyped, strip parmeter signature
	char* parenPosition;
	int len;
	len = ( (parenPosition=strchr( symNm, '(')) ?
			parenPosition - symNm: strlen(symNm) );
	char *funcNm = new char[len];
	strncpy(funcNm, symNm, len);
	funcNm[len] = 0;

	oldnode.re_init(N_ID, funcNm);
	oldnode.entry = oldentry;
	oldnode.type = oldtype;
	oldnode.flags = oldflags;
}

#define EXACT		0x1000000
#define WITH_PROMOTION	0x10000
#define WITH_CONVERSION	0x100
#define NO_FORMALS	2
#define	ELLIP_PARAM	1
#define NO_MATCH	0

unsigned int
nodeResolve::matchArgTypes(CCtree*& zNode, Symbol& formalParam)
{
	int match = NO_MATCH;
	CCtree *actualArg = zNode==0? 0: zNode->operand[0];
	if( formalParam.isnull() )
	{
		if( !zNode )
			return EXACT;

		return NO_FORMALS;
	}

	// check "this"; the types of the "this" parameter (if present)
	// must match but the match does not contribute to the match score
	TYPE formalTYPE;
	formalParam.type(formalTYPE);
	if( lang == CPLUS && 
		(strcmp(formalParam.name(), THIS_NM)==0)
		&& formalTYPE.isPtrType() )
	{
		if( formalTYPE == actualArg->type )
		{
			zNode->type = formalTYPE;
			zNode = zNode->operand[1];
			actualArg = zNode==0? 0: zNode->operand[0];
			formalParam = formalParam.sibling();
			if (!zNode)
			{
				while(!formalParam.isnull())
				{
					if (formalParam.isArgument())
					{
						// more formals than actuals
						return NO_MATCH;
					}
					formalParam = formalParam.sibling();
				}
				return EXACT;  // only "this" pointer
			}
		}
		else
			return NO_MATCH;
	}
	
	while(!formalParam.isnull())
	{
		if (!formalParam.isArgument() && 
			!formalParam.isUnspecArgs())
		{
			formalParam = formalParam.sibling();
			continue;
		}
		Attribute *paramAttr;
		if( formalParam.isUnspecArgs() ||
			((paramAttr=formalParam.attribute(an_name)) &&
		       paramAttr->form==af_stringndx &&
			!strcmp(ELLIPSIS_STR, paramAttr->value.name)) )
		{
			match |= ELLIP_PARAM;
			break;
		}
		else if (!actualArg)
			return NO_MATCH;

		formalParam.type(formalTYPE);
		if( formalTYPE == actualArg->type )
		{
			match += EXACT;
		}
		else if( actualArg->op==N_STRING && formalTYPE.isPtrType() )
		{
			TYPE drefType;
			Fund_type drefFt;
			if( formalTYPE.deref_type(drefType) &&
			    drefType.fund_type(drefFt) && drefFt==ft_char )
			{
				match += EXACT;
			}
			else
			{
				match = NO_MATCH;
				break;
			}
		}
		else if( CCtype_match(formalTYPE,actualArg->type) )
		{
			if( (formalTYPE.isIntegral() &&
				     actualArg->type.isIntegral()) ||
			    (formalTYPE.isFloatType() &&
				     actualArg->type.isFloatType()) )
			{
				match += WITH_PROMOTION;
			}
			{
				match += WITH_CONVERSION;
			}
		}
		else
		{
			match = NO_MATCH;
			break;
		}

		zNode = zNode->operand[1];
		actualArg = zNode==0? 0: zNode->operand[0];
		formalParam = formalParam.sibling();
	}
	if (actualArg && !(match & ELLIP_PARAM))
		return NO_MATCH;
	return match;
}

void
nodeResolve::markParamTypes(CCtree* zNode, Symbol& formalParam)
{
	int ellipsisSeen = 0;
	CCtree *actualArg = zNode==0? 0: zNode->operand[0];
	while(actualArg!=0 && !formalParam.isnull() )
	{
		if (!formalParam.isArgument() && 
			!formalParam.isUnspecArgs())
		{
			formalParam = formalParam.sibling();
			continue;
		}
		Attribute *paramAttr;
		if( formalParam.isUnspecArgs() ||
			((paramAttr=formalParam.attribute(an_name)) &&
		       paramAttr->form==af_stringndx &&
			!strcmp(ELLIPSIS_STR, paramAttr->value.name)) )
		{
			ellipsisSeen++;
		}
		if( !ellipsisSeen )
		{
			TYPE formalTYPE;
			formalParam.type(formalTYPE);
			zNode->type = formalTYPE;
		}
		else
		{
			zNode->type = actualArg->type;
		}
		zNode = zNode->operand[1];
		actualArg = zNode==0? 0: zNode->operand[0];
		formalParam = formalParam.sibling();
	}
}

int
nodeResolve::typeName_cat(Buffer* buf, CCtree* typeNode)
{
	List nodeList;
	buildNodeList(typeNode, nodeList);	// linearize N_TYPE subtree

	// build a TYPE object
	CCtree *currNode = (CCtree *)nodeList.first();
	while( currNode!=0 )
	{
		switch( currNode->op )
		{
		case N_TYPE:
		{
			if( currNode->valkind == VT_strval )
			{
				// user type 
				buf->add(currNode->strval);
				break;
			}
			else if( currNode->valkind != VT_code )
			{
				printe(ERR_internal, E_ERROR,
                                         "nodeResovle::typeNm_cat", __LINE__);

				return 0;
			}

			Fund_type curr_ft,
				  prev_ft = ft_none;
			do
			{
				switch(currNode->code)
				{
				case S_CHAR:	curr_ft = ft_char; break;
				case S_DOUBLE:	curr_ft = ft_lfloat; break;
				case S_FLOAT:	curr_ft = ft_sfloat; break;
				case S_INT:	curr_ft = ft_int; break;
				case S_LONG:	curr_ft = ft_long; break;
				case S_SHORT:	curr_ft = ft_short; break;
				case S_VOID:	curr_ft = ft_void; break;
				case S_SIGNED:	curr_ft = ft_sint; break;
				case S_UNSIGNED: curr_ft = ft_uint; break;
				default:
					printe(ERR_internal, E_ERROR,
						"nodeResovle::typeNm_cat", __LINE__);
					return 0;
				}

				if( prev_ft != ft_none )
				{
					if( !mergeFundTypes(curr_ft,
							     prev_ft,curr_ft) )
					{
						printe(ERR_invalid_type,
								    E_ERROR);
						return 0;
					}
				}
				prev_ft = curr_ft;
				currNode = (CCtree *)nodeList.next();
			} while( currNode && currNode->op==N_TYPE );

			char *ftStr;
			switch(curr_ft)
			{
			case ft_char:
				ftStr = "char";
			case ft_schar:
				ftStr = "signed char";
				break;
			case ft_uchar:
				ftStr = "unsigned char";
				break;
			case ft_short:
			case ft_sshort:
				ftStr = "short";
				break;
			case ft_ushort:
				ftStr = "unsigned short";
				break;
			case ft_int:
			case ft_sint:
				ftStr = "int";
				break;
			case ft_uint:
				ftStr = "unsigned int";
				break;
			case ft_long:
			case ft_slong:
				ftStr = "long";
				break;
			case ft_ulong:
				ftStr = "unsigned long";
				break;
			case ft_sfloat:
				ftStr = "float";
				break;
			case ft_lfloat:
				ftStr = "double";
				break;
			case ft_xfloat:
				ftStr = "long double";
				break;
			default:
				printe(ERR_internal, E_ERROR,
					"nodeResolve::typeNm_cat", __LINE__);
				return 0;
			}

			buf->add(ftStr);
			continue;
		}
		case N_AGGR:
		case N_ENUM:
			// e[0] is a TYPE with an ID value, will be handled 
			// in next iteration of loop by N_TYPE code 
			break;
		case N_DT_PTR:	
			currNode = (CCtree *)nodeList.next();
			if( currNode->op == N_DT_FCN )
			{
				buf->add("(*)");
			}
			else
			{
				buf->add('*');
			}
			continue;
		case N_DT_ARY:
			buf->add("[]");
			break;
		case N_DT_FCN:
			buf->add("()");
			break;
		case N_DT_REF:	
			buf->add('&');
			break;
		case N_DT_MPTR:
			buf->add(currNode->strval);
			buf->add("::*");
			break;
		default:
			printe(ERR_internal, E_ERROR,
					"nodeResolve::typeNm_cat", __LINE__);
			return 0;
		}

		currNode = (CCtree *)nodeList.next();
	}
	return 1;
}

//
// Operator handling routines
//

int
nodeResolve::do_Select(CCtree& e, Resolver* context)
{
	CCtree& lhs	= *e[0];
	register CCtree& member = *e[1];

	// resolve lhs and ...
	if (!resolve(&lhs, context))
		return 0;

	//                  ... validate it
	Symbol rhs_context_tsym;
	if( e.op == N_REF ) 
	{
		TYPE deref_type;
		Tag  ptr_tag;

		if( !lhs.flags&eHAS_RVALUE )
		{
			printe(ERR_lhs_novalue, E_ERROR, e.opStr());
			return 0;
		}
		
		// insure lhs is pointer and get the basetype (in deref_type)
		if( !lhs.type.deref_type(deref_type, &ptr_tag) )
		{
			printe(ERR_non_pointer_lhs, E_ERROR, e.opStr());
			return 0;
		}

		// insure basetype is a user type 
		if( !deref_type.user_type(rhs_context_tsym) )
		{
			printe(ERR_lhs_not_struct, E_ERROR, e.opStr());
			return 0;
		}
	}
	else  // N_DOT
	{
		if (! (lhs.flags & eHAS_PLACE))
		{
			printe(ERR_not_lvalue, E_ERROR, e.opStr());
			return 0;
		}
		// insure lhs is aggregate
		if (! lhs.type.user_type(rhs_context_tsym)) 
		{
			printe(ERR_lhs_not_struct, E_ERROR, e.opStr());
			return 0;
		}
	}

	if(!rhs_context_tsym.isStructType() && !rhs_context_tsym.isUnionType())
	{
		printe(ERR_lhs_not_struct, E_ERROR, e.opStr());
		return 0;
	}

 	// restrict context of field name search to lhs object
	Resolver	*r;
	if (lang == CPLUS)
		r = (Resolver *)new CCresolver(rhs_context_tsym,
			context->proc());
	else
		r = new Resolver(rhs_context_tsym,
			context->proc());
	if (!resolve(e[1], r)) // lookup field name.
		return 0;
	delete r;

	if( e[1]->entry.isSubrtn() )
	{
		// resolver may return full prototype - 
		// strip off arguments
		replaceWithFuncIdNode(e, e[1]);
	}
	else
	{
		e.type = e[1]->type;
		e.flags = e[1]->flags;
	}

	return 1;
}

int
nodeResolve::do_MemberSelect(CCtree& e, Resolver* context)
{
	CCtree& lhs	= *e[0];
	register CCtree& memberPtr = *e[1];

	if (!resolve(&lhs, context) || !resolve(&memberPtr, context) )
		return 0;

	// insure lhs is a class or class pointer
	Symbol rhs_context_tsym;
	if( e.op == N_REFREF ) 
	{
		// ->* insure lhs is ptr to struct
		if( !lhs.flags&eHAS_RVALUE )
		{
			printe(ERR_lhs_novalue, E_ERROR, e.opStr());
			return 0;
		}

		Fund_type dummyFt;
		Symbol lhsUt;
		TYPE lhsBase;
		if( !lhs.type.isPointer(&dummyFt, &lhsUt) || lhsUt.isnull() ||
		    !lhsUt.type(lhsBase, an_basetype) || !lhsBase.isClass() )
		{
			printe(ERR_class_ref, E_ERROR, e.opStr());
			return 0;
		}
	}
	else if( e.op == N_DOTREF )
	{
		if (! (lhs.flags & eHAS_PLACE))
		{
			printe(ERR_not_lvalue, E_ERROR, e.opStr());
			return 0;
		}

		// if .* insure lhs is a struct
		if( !lhs.type.isClass() )
		{
			printe(ERR_class_ref, E_ERROR, e.opStr());
			return 0;
		}	

	}

	// insure memberPtr is a pointer
	if( !memberPtr.type.isPointer() )
	{
		printe(ERR_rhs_not_ptr, E_ERROR, e.opStr());
		return 0;
	}

	e.type = memberPtr.type;
	e.flags |= eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;

	return 1;
}

int
nodeResolve::do_Member(CCtree& e, Resolver* context)
{
	// x::y
	register CCtree* classNode = e[1];
	register CCtree* member = e[0];

	if( !classNode )
	{
		CCresolver globalContext(context->proc(), 0);
		if( !resolve(member, (Resolver *)&globalContext) )
		{
			return 0;
		}
		e.type = member->type;
		e.flags |= eHAS_PLACE | eHAS_RVALUE |
				(e.type.isBitField()? 0: eHAS_ADDRESS);
		return 1;
	}
	
	// find classNode and insure it is a structure
	if( !context->lookup(classNode->strval, classNode->entry, st_tagnames) )
	{
		return 0;
	}

	if( !classNode->entry.isClassType() )
	{
		printe(ERR_class_ref, E_ERROR, e.opStr());
		return 0;
	}

 	// search for member 
	CCresolver classContext(classNode->entry, context->proc());
	if( !resolve(member, &classContext) )
	{
		return 0;
	}

	if( member->entry.isSubrtn() )
	{
		// resolver returns names with prototype info
		// strip off arguments
		replaceWithFuncIdNode(e, member);
		delete classNode;
		delete member;
	}
	else
	{
		e.type = member->type;
		e.flags = member->flags;
	}

	return 1;
}

int
nodeResolve::do_Deref(CCtree& e, Resolver* context)
{
	register CCtree *ptr = e[0];

	if( !resolve(ptr, context) )
		return 0;

	if( !(ptr->flags & eHAS_RVALUE) )
	{
		printe(ERR_un_novalue, E_ERROR, e.opStr());
		return 0;
	}

	if( ptr->type.isPtrType() )
	{
		ptr->type.deref_type(e.type);
	}
	else
	{
		Fund_type ptr_ftype;
		// not array or pointer, allow for dereference of
		// ADDR ops and integral constants
		if( ptr->type.fund_type(ptr_ftype) ) 
		{
			if( ptr->op==N_ICON && ptr->type.isIntegral() )
			{
				e.type = ft_pointer;
			}
			else if( ptr->op != N_ADDR )
			{
				printe(ERR_operand_not_ptr, E_ERROR, 
					e.opStr());
				return 0;
			}
		}
		else  // non-pointer user type, error
		{
			printe(ERR_operand_not_ptr, E_ERROR, e.opStr());
			return 0;
		}
	}

	e.flags |= eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;
	return 1;
}

int
nodeResolve::do_Addr(CCtree& e, Resolver* context)
{
	if (!resolve(e[0], context))
		return 0;

	if( !(e[0]->flags&eHAS_ADDRESS) )
	{
		printe(ERR_no_addr, E_ERROR, e.opStr());
		return 0;
	}

	if( !e.type.build_ptrToBase(e[0]->type, context->proc()) )
	{
		return 0;
	}

	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_Index(CCtree& e, Resolver* context)
{
	register CCtree *lhs   = e[0];
	register CCtree *rhs = e[1];

	// allows a[1] or 1[a]

	if( !resolve(lhs, context) || !resolve(rhs, context) )
	{
		return 0;
	}

	if( !(lhs->flags&eHAS_RVALUE) )
	{
		printe(ERR_lhs_novalue, E_ERROR, e.opStr());
		return 0;
	}

	if( !(rhs->flags&eHAS_RVALUE) )
	{
		printe(ERR_rhs_novalue, E_ERROR, e.opStr());
		return 0;
	}
	if( lhs->type.deref_type(e.type))
	{
		if( !rhs->type.isIntegral() )
		{
			printe(ERR_rhs_not_integral, E_ERROR, e.opStr());
			return 0;
		}
	}
	else if( rhs->type.deref_type(e.type))
	{
		if( !lhs->type.isIntegral() )
		{
			printe(ERR_lhs_not_integral, E_ERROR, e.opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_not_ptr, E_ERROR, e.opStr());
		return 0;
	}

	e.flags |= eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;
	return 1;
}

int
nodeResolve::do_Constant(CCtree& e)
{
	switch (e.valkind) {
	case VT_char:  e.type = ft_char;   break;
	case VT_long:  
		if( e.ulongval <= INT_MAX )
			e.type = ft_int;
		else if( e.ulongval <= LONG_MAX )
			e.type = ft_long;
		else
			e.type = ft_ulong;
		break;
	case VT_ulong: e.type = ft_ulong;  break;
	case VT_float: e.type = ft_lfloat; break; 
	case VT_xfloat: e.type = ft_xfloat; break; 

	case VT_strval:
	case VT_code:
	case VT_none:
	default:
		printe(ERR_internal, E_ERROR, "do_Constant", __LINE__);
		return 0;
	}
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_Sizeof(CCtree& e, Resolver* context)
{
	// verify operand not a bitfield or function

	register CCtree	*rhs = e[0];

	if (!resolve(e[0], context))
		return 0;

	if (rhs->type.isSubrtn() || rhs->type.isBitField())
	{
		printe(ERR_invalid_op_type, E_ERROR, e.opStr());
		return 0;
	}

	e.type  = SIZEOF_TYPE;
	e.flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_AddOp(CCtree &e, Resolver *context)
{
	// Verify:
	//   both arithmetic
	//   one ptr, other integral
	// Set Result:
	//   type after standard conversions 
	//   ptr to type

	if( !resolve_BinaryOpArgs(e, context) )
		return 0;

	TYPE elemTYPE;
	Fund_type lhs_ft,
		  rhs_ft;
	if( e[0]->type.isPtrType() )
	{
		if( !e[1]->type.isIntegral() )
		{
			printe(ERR_rhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
		e.type.build_ptrTYPE(e[0]->type, context->proc());
	}
	else if( e[1]->type.isPtrType() )
	{
		if( !e[0]->type.isIntegral() )
		{
			printe(ERR_lhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
		e.type.build_ptrTYPE(e[1]->type, context->proc());
	}
	else if( e[0]->type.isArithmetic(&lhs_ft) )
	{
		if( e[1]->type.isArithmetic(&rhs_ft) )
		{
			e.type=standardConversionType(&lhs_ft, &rhs_ft);
		}
		else
		{
			printe(ERR_rhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e.opStr());
		return 0;
	}

	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_SubOp(CCtree &e, Resolver *context)
{
	// Verify:
	//   both arithmetic
	//   both ptr to type
	//   left ptr, right integral
	// Set Result:
	//   type after standard conversions 
	//   ptrdiff_t 
	//   ptr to type

	if( !resolve_BinaryOpArgs(e, context) )
		return 0;

	Fund_type lhs_ft,
		  rhs_ft;
	if( e[0]->type.isPtrType() )
	{
		if( e[1]->type.isIntegral() )
		{
			e.type.build_ptrTYPE(e[0]->type, context->proc());
		}
		else if( e[1]->type.isPtrType() )
		{
			if( !CCtype_match(e[0]->type, e[1]->type ) )
			{
				printe(ERR_incomp_type_ptr, E_ERROR,
					e.opStr());
				return 0;
			}
			e.type = PTRDIFF_TYPE;
		}
		else
		{
			printe(ERR_illegal_ptr_sub, E_ERROR);
			return 0;
		}
	}
	else if( e[0]->type.isArithmetic(&lhs_ft) )
	{
		if( e[1]->type.isArithmetic(&rhs_ft) )
		{
			e.type=standardConversionType(&lhs_ft, &rhs_ft);
		}
		else
		{
			printe(ERR_rhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e.opStr());
		return 0;
	}

	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_BinOp(CCtree &e, Resolver* context, C_TypeClass argClass)
{
	// Verify:
	//   both arithmetic
	// Set Result:
	//   type after standard conversions 

	if( !resolve_BinaryOpArgs(e, context) )
		return 0;

	Fund_type lhs_ft,
		  rhs_ft;

	switch(argClass)
	{
	case C_integral:
		if( !twoIntegralArgs(e, lhs_ft, rhs_ft,) )
			return 0;
		break;
	case C_arithmetic:
		if( !twoArithArgs(e, lhs_ft, rhs_ft) )
			return 0;
		break;
	default:
		printe(ERR_internal, E_ERROR, "do_BinOp", __LINE__);
	}

	e.type = standardConversionType(&lhs_ft, &rhs_ft);
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_CompareOp(CCtree &e, Resolver *context)
{
	// Verify:
	//   both arithmetic
	//   both ptrs to compatible objects
	//   one operand is a pointer, the other 0
	//       and the operation is == or !=
	// Set Result
	//   int

	if( !resolve_BinaryOpArgs(e, context) )
		return 0;

	Fund_type lhs_ft,
		  rhs_ft;
	if( e[0]->type.isArithmetic(&lhs_ft) )
	{
		if( e[1]->type.isArithmetic(&rhs_ft) )
		{
			e.type = standardConversionType(&lhs_ft, &rhs_ft);
		}
		else
		{
			printe(ERR_rhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
	}
	else if( e[0]->type.isPtrType() )
	{
		if( e[1]->type.isPtrType() )
		{
			if( !CCtype_match(e[0]->type, e[1]->type ) )
			{
				printe(ERR_incomp_type_ptr, E_ERROR, 
					e.opStr());
				return 0;
			}
		}
		// special case for comparison of ptr with 0
		else if ((e[1]->op != N_ICON) || (e[1]->ulongval != 0)
			|| (e.op != N_EQ && e.op != N_NE))
		{
			printe(ERR_incomp_type, E_ERROR, e.opStr());
			return 0;
		}
		e.type = ft_int;
	}
	else if (e[1]->type.isPtrType() && 
		(e.op == N_EQ || e.op == N_NE))
	{
		if ((e[0]->op != N_ICON) || (e[0]->ulongval != 0))
		{
			printe(ERR_incomp_type, E_ERROR, e.opStr());
			return 0;
		}
		e.type = ft_int;
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e.opStr());
		return 0;
	}

	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_LogicOp(CCtree &e, Resolver *context)
{
	// Verify:
	//   both scalar
	// Set Result
	//   int

	if( !resolve_BinaryOpArgs(e, context) )
		return 0;

	if( !e[0]->type.isScalar() )
	{
		printe(ERR_lhs_scalar, E_ERROR, e.opStr());
		return 0;
	}

	if( !e[1]->type.isScalar() )
	{
		printe(ERR_rhs_scalar, E_ERROR, e.opStr());
		return 0;
	}
	
	e.type = ft_int;
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_UnaryOp(CCtree &e, Resolver* context, C_TypeClass argClass)
{
	if( !resolve(e[0], context) )
		return 0;

	if( !(e[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_un_novalue, E_ERROR, e.opStr());
		return 0;
	}

	Fund_type ft;
	switch(argClass)
	{
	case C_integral:
		if( !e[0]->type.isIntegral(&ft) )
		{
			printe(ERR_un_not_integral, E_ERROR, e.opStr());
			return 0;
		}
		break;
	case C_arithmetic:
		if( !e[0]->type.isArithmetic(&ft) )
		{
			printe(ERR_un_arithmetic,E_ERROR,e.opStr());
			return 0;
		}
		break;
	default:
		printe(ERR_internal, E_ERROR, "do_UnaryOp", __LINE__);
	}

	e.type = standardConversionType(&ft, 0);
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_PrePostOp(CCtree &e, Resolver* context)
{
	// Verify
	//   modifiable lvalue
	//   scalar
	// Set Result
	//   type of operand

	if( !resolve(e[0], context) )
		return 0;

	if( !(e[0]->flags&eHAS_PLACE) )
	{
		printe(ERR_opd_not_lvalue, E_ERROR, e.opStr());
		return 0;
	}

	if( !(e[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_un_novalue, E_ERROR, e.opStr());
		return 0;
	}

	if( e[0]->type.isPtrType() )
	{
		e.type.build_ptrTYPE(e[0]->type, context->proc());
	}
	else if( e[0]->type.isArithmetic() )
	{
		e.type = e[0]->type;
	}
	else // not pointer or arithmetic (i.e., not scalar)
	{
		printe(ERR_un_scalar, E_ERROR, e.opStr());
		return 0;
	}

	e.flags |= eHAS_RVALUE;
	return 1;
}

nodeResolve::do_NotOp( CCtree &e, Resolver* context)
{
	// Verify:
	//   scalar
	// Set Result:
	//   result type int

	if( !resolve(e[0], context) )
		return 0;

	if( !(e[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_un_novalue, E_ERROR, e.opStr());
		return 0;
	}

	if( !e[0]->type.isScalar() )
	{
		printe(ERR_un_scalar, E_ERROR, e.opStr());
		return 0;
	}
	
	e.type = ft_int;
	e.flags |= eHAS_PLACE | eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_ID(CCtree &e, Resolver* context)
{
	if( !context->lookup(e.strval, e.entry) )
	{
		if( !isEvent )
		{
			printe(ERR_no_symbol_info, E_ERROR, e.strval);
		}
		return 0;
	} 

	Symbol classSym;
	Symbol thisSym;
	if (lang == CPLUS && ((CCresolver *)context)->inMemberFunction(thisSym, 
		classSym) && 
		e.entry.parent().isClassType() )
	{
		// The context is a member function and the ID is a 
		// structure member. Therefore, the ID refers to a data
		// member in "this" class.  Replace the ID node by
		// sub-tree "this->ID" 
		CCtree *thisNode = new CCtree(thisSym);
		CCtree *memberNode = new CCtree(&e);
		e.re_init(N_REF, 2, thisNode, memberNode);
		return resolve(&e, context);
	}

	if( e.entry.isSubrtn() || e.entry.isLabel() )
	{
		e.type = ft_pointer;	// not true, but it works
	}
	else if( !e.entry.type(e.type) )
	{
		printe(ERR_no_type, E_WARNING, e.strval);
	}
	else if (e.entry.type_assumed(0) == 1)
	{
		printe(ERR_type_assumed, E_WARNING, e.strval);
	}

	Tag t;
	t = e.entry.tag("do_ID/entry");
	switch ( t )
	{
	case t_argument:
	case t_global_variable:
	case t_local_variable:
	case t_extlabel:
	case t_entry:
	case t_structuremem:
	case t_bitfield:
	case t_unionmem:
		e.flags = eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;
		break;
	case t_subroutine:
	case t_global_sub:
	case t_functiontype:
	case t_label:
		e.flags = eHAS_ADDRESS | eHAS_RVALUE; 
					// ...addr is rvalue; no lval
		break;
	case t_enumlittype:
		e.flags = eHAS_RVALUE;
		break;
	default:
		; // set no flags.
	}
	if (e.type.form() == TF_user)
	{
		// if symbol is an array type,
		// do not set eHAS_PLACE, so
		// can't assign to an array
		Symbol	utype;
		if (!e.type.user_type(utype))
			return 0;
		if (utype.isArrayType())
			e.flags &= ~eHAS_PLACE;
	}

	return 1;
}

int
nodeResolve::buildQualifier(CCtree *e, char *&qualifier)
{
	// concatenate operands of DOT
	if(e->operand[0]->op!=N_ID)
	{
		return 0;
	}

	char *op2str;
	
	if( e->operand[1]->op == N_FCON )
	{
		// MORE - will we have problems with conversion
		// from ASCII to float and vice-versa?

		char floatStr[9];	// gives 1,000,000 thread ids
		// thread id of lwp id comes through as a float const with
		// leading 0, e.g., .1 of p1.1 is float constant 0.1

		sprintf(floatStr, "%lg", e->operand[0]->floatval);
		op2str = floatStr+2;	// get past leading 0.
	}
	else
	{
		if(e->operand[1]->op!=N_ID)
		{
			return 0;
		}
		op2str = e->operand[1]->strval;
	}

	qualifier = new char[strlen(e->operand[0]->strval)+strlen(op2str)+2];
	strcpy(qualifier, e->operand[0]->strval);
	strcat(qualifier, ".");
	strcat(qualifier, op2str);

	return 1;
}

//
// Constants for "@" (id qualifier) routines
//
#define MAX_QUALIFIERS	4
#define NULL_QUALIFIER	((char *)-1)

int
nodeResolve::collectQualifiers(CCtree &e, char *qualifier[], CCtree *&idNode)
{
	int q_indx = 0;
	CCtree *at;

	for( at = &e; at->op == N_AT; at = at->operand[0] )
	{
		if( q_indx >= MAX_QUALIFIERS )
		{
			printe(ERR_id_qualifiers, E_ERROR);
			return 0;
		}

		if( at->n_operands == 2 )
		{
			if( at->operand[1]->op == N_ID )
			{
				
				qualifier[q_indx] = 
					makestr(at->operand[1]->strval);
			}
			else if( at->operand[1]->op == N_ICON )
			{
				qualifier[q_indx] = new char[11];
				sprintf(qualifier[q_indx], "%d", 
						at->operand[1]->ulongval);
			}
			else if( !buildQualifier(at->operand[1],
							qualifier[q_indx]) )
			{
				printe(ERR_id_qualifiers, E_ERROR);
				return 0;
			}

		}
		else // @ with no qualifier (null qualifier)
		{
			qualifier[q_indx] = NULL_QUALIFIER;
		}
		q_indx++;
	}
	
	idNode = at;	// return ptr to identifier sub-tree
	return 1;
}

int
nodeResolve::validProcId(char *qualifier, LWP *&lwp)
{
	char *cp = qualifier;

	// validate process/thread name
	if( *cp++ != 'p'  )
		return 0;
	while( isdigit((int)*cp) )
		++cp;
#if 0
	// MORE - thread ids
	// This allows only thread ids - is that okay?
	if( *cp && *cp++!='.' )
		return 0;
	while( *cp && isdigit((int)*cp) )
		++cp;
#endif
	if( *cp!=0 )
		return 0;

	// find the lwp 
	LWP *tmpLwp;
	if( !(tmpLwp=proglist.find_lwp(qualifier)) )
		return 0;

	lwp = tmpLwp;
	return 1;
}

int
nodeResolve::getFuncQualifier(CCtree &e, char *fname, 
	Resolver *&rslvr, int dodelete)
{
	LWP	*localLwp = rslvr->proc();
	Frame	*frame = localLwp->topframe();
	Symbol	cursym;

	for( ; frame;  frame = frame->caller())
	{
		cursym = localLwp->find_entry(frame->pc_value());
		if (!cursym.isnull() && 
			!strcmp(localLwp->symbol_name(cursym), fname))
			break;
	}
	if (!frame)
	{
		if( !isEvent )
		{
			printe(ERR_func_qualifier, E_ERROR, fname);
		}
		return 0;
	}
	e.frame = frame;
	if (dodelete)
		delete rslvr;
	if (lang == CPLUS)
		rslvr = (Resolver *)new CCresolver(localLwp, 
			frame->pc_value());
	else
		rslvr = new Resolver(localLwp, frame->pc_value());

	return 1;
}

int
nodeResolve::getFrameContext(CCtree &e, long frameNbr, Resolver *&rslvr,
	int dodelete)
{
        LWP *localLwp = rslvr->proc();

	int totalFrames = count_frames(localLwp);
	int walkBackCount = (int)(totalFrames - frameNbr);
	if( walkBackCount < 0 )
	{
		printe(ERR_frame_qualifier, E_ERROR, frameNbr);
		return 0;
	}
	
	Frame *frame = localLwp->topframe();
	while( walkBackCount-- > 0 )
	{
		frame = frame->caller();
	}
	
	e.frame = frame;
	if (dodelete)
		delete rslvr;
	if (lang == CPLUS)
		rslvr = (Resolver *)new CCresolver(localLwp, 
			frame->pc_value());
	else
		rslvr = new Resolver(localLwp, frame->pc_value());
	return 1;
}

int
nodeResolve::getBlockContext(long lineNbr, Resolver *&rslvr, int dodelete )
{
	// get address of line
	Symbol sym;
	if( !rslvr->getEnclosingSrc(sym) )
	{
		printe(ERR_block_source, E_ERROR, lineNbr);
		return 0;
	}

	Source src;
	sym.source(src);

	Iaddr stmtAddr;
	src.stmt_to_pc(lineNbr, stmtAddr);

	// find enclosing block
	LWP *lwp = rslvr->proc();
	Symbol scope = lwp->find_scope(stmtAddr);
	if( !scope.isBlock() )
	{
		printe(ERR_block_source, E_ERROR, lineNbr);
		return 0;
	}

	// find and validate enclosing function
	Symbol func = scope;
	do
	{
		func = func.parent();
	} while( !func.isSubrtn() );

	Symbol curfunc = lwp->current_function();
	if( strcmp(lwp->symbol_name(curfunc), func.name()) )
	{
		if( !isEvent )
		{
			printe(ERR_block_not_current, E_ERROR, lineNbr);
		}
		return 0;
	}

	if (dodelete)
		delete rslvr;
	if (lang == CPLUS)
		rslvr = (Resolver *)new CCresolver(scope, lwp);
	else
		rslvr = new Resolver(scope, lwp);
	return 1;
}

int
nodeResolve::getQualifiedContext(CCtree &e, char *qualifier[],
				Resolver *context, Resolver *&newContext)
{
#define LWP_QUAL	1
#define FILE_QUAL	2
#define FUNC_QUAL	3
#define BLOCK_FRAME_QUAL 4
#define Q_DONE		5

	int q_indx = 0;
	int nbrIsBlock = 0;
	Resolver *tmpContext = context;
	LWP *q_lwp = context->proc();
	int q_state = LWP_QUAL;
	char *endPtr;
	long nbr;

	while( q_state!=Q_DONE && qualifier[q_indx] )
	{
		if( qualifier[q_indx] == NULL_QUALIFIER )
		{
			q_state++;
			q_indx++;
			continue;
		}

		Symbol tmpSymbol;
		switch(q_state)
		{
		case LWP_QUAL:
			if( validProcId(qualifier[q_indx], q_lwp) )
			{
				if (lang == CPLUS)
					tmpContext = (Resolver *)new 
						CCresolver(q_lwp,
						q_lwp->curframe()->pc_value());
				else
					tmpContext = new Resolver(q_lwp,
						q_lwp->curframe()->pc_value());
				e.lwp =q_lwp;
				q_indx++;
			}
			break;
		case FILE_QUAL:
			if( q_lwp->find_source(qualifier[q_indx], tmpSymbol) )
			{
				if( tmpContext != context )
				{
					delete tmpContext;
				}
				if (lang == CPLUS)
					tmpContext = (Resolver *)new 
						CCresolver(tmpSymbol,
							q_lwp);
				else
					tmpContext = new
						Resolver(tmpSymbol,q_lwp);
				nbrIsBlock++;
				q_indx++;
			}
			break;
		case FUNC_QUAL:
        		nbr = strtol(qualifier[q_indx], &endPtr, 10);
        		if( *endPtr == 0 )
        		{
				// frame number
				break;
			}
			if( getFuncQualifier(e, qualifier[q_indx], 
				tmpContext, (tmpContext != context)) )
			{
				nbrIsBlock++;
				q_indx++;
			}
			break;
		case BLOCK_FRAME_QUAL:
        		nbr = strtol(qualifier[q_indx], &endPtr, 10);
        		if( *endPtr != 0 )
        		{
				break;
			}

			if( nbrIsBlock )
			{
				if( getBlockContext(nbr, tmpContext,
					(tmpContext != context)) )
				{
					q_indx++;
				}
			}
			else
			{
				if( getFrameContext(e, nbr, tmpContext,
					(tmpContext != context)) )
				{
					q_indx++;
				}
			}
			break;
		default:
			printe(ERR_internal, E_ERROR,"getQualifiedContext", 
								__LINE__);
			return 0;
		}
		q_state++;
	}

	if( q_indx<MAX_QUALIFIERS && qualifier[q_indx]!=0 )
	{
		// left over (unused) qualifiers
		if( !isEvent )
		{
			printe(ERR_id_qualifiers, E_ERROR);
		}
		return 0;
	}

	newContext = tmpContext;

	return 1;
}

int
nodeResolve::do_At(CCtree &e, Resolver* context)
{
	if( !context->proc() )
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}

	char *qualifier[MAX_QUALIFIERS];
	int q_indx = 0;
	
	for( q_indx = 0; q_indx < MAX_QUALIFIERS; q_indx++ )
	{
		qualifier[q_indx] = 0;
	}

	CCtree *idNode;
	if( !collectQualifiers(e, qualifier, idNode) )
	{
		return 0;
	}


	Resolver *newContext;
	if( !getQualifiedContext(e, qualifier, context, newContext) )
	{
		return 0;
	}

	if( !resolve(idNode, newContext) )
	{
		return 0;
	}
	if (newContext != context)
		delete newContext;
	e.flags = idNode->flags;
	e.type = idNode->type;

	return 1;
}

int
nodeResolve::do_Assign(CCtree& e, Resolver* context)
{
	// Verify:
	//   left is modifiable l_value
	// Verify one of:
	//   left/right arithmetic, 
	//   left/right compatible struct/union, 
	//   left/right ptrs to compatible struct/union
	//   one pointer to struct, one pointer to void (with same 
	//     qualifiers as other operands)
	//   left pointer, right null pointer constant
	// Set Result:
	//   type of left operand

	if( !resolve_AssignOprnds(e, context) )
		return 0;

	if( !(e[0]->type.isArithmetic() && e[1]->type.isArithmetic()) &&
            !CCtype_match(e[0]->type, e[1]->type ) )
	{
		printe(ERR_incomp_type, E_ERROR, e.opStr());
		return 0;
	}

	e.type   = e[0]->type;
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_AssignAddOp(CCtree& e, Resolver* context)
{
	// Verify:
	//   left is modifiable l_value
	// Verify one of:
	//   both arithmetic
	//   left pointer to object, right integral
	// Set Result:
	//   type of left operand

	if( !resolve_AssignOprnds(e, context) )
		return 0;

	if( e[0]->type.isPtrType() )
	{
		if( !e[1]->type.isIntegral() )
		{
			printe(ERR_rhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
	}
	else if( e[0]->type.isArithmetic() )
	{
		if( !e[1]->type.isArithmetic() )
		{
			printe(ERR_rhs_binop_type, E_ERROR, e.opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e.opStr());
		return 0;
	}

	e.type   = e[0]->type;
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_AssignOp(CCtree& e, Resolver* context, C_TypeClass argClass)
{
	// Verify:
	//   left is modifiable l_value
	//   both arithmetic
	// Set Result:
	//   type of left operand

	if( !resolve_AssignOprnds(e, context) )
		return 0;

	Fund_type lhs_ft,
		  rhs_ft;

	switch(argClass)
	{
	case C_integral:
		if( !twoIntegralArgs(e, lhs_ft, rhs_ft) )
			return 0;
		break;
	case C_arithmetic:
		if( !twoArithArgs(e, lhs_ft, rhs_ft) )
			return 0;
		break;
	default:
		printe(ERR_internal, E_ERROR, "do_AssignOp", __LINE__);
	}

	e.type   = e[0]->type;
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_COMop(CCtree &e, Resolver* context)
{
	// Verify:
	//   right operand has value
	// Set Result
	//   type of right operand

	if( !resolve(e[0], context) || !resolve(e[1], context) )
                return 0;

	if( !(e[1]->flags&eHAS_RVALUE) )
	{
		printe(ERR_rhs_novalue, E_ERROR, e.opStr());
		return 0;
	}

        Fund_type ft;

	if( !e[1]->type.fund_type(ft) )
	{
		printe(ERR_invalid_op_type, E_ERROR, e.opStr());
		return 0;
	}

	e.type = e[1]->type;
	e.flags |= eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_QuestionOp(CCtree &e, Resolver* context)
{
	// Verify:
	//   first scalar
	//   others arithemetic
	//	    compatible structure/union
	//	    void
	//	    ptrs to compatible objects
	//	    ptr to object and ptr to void
	// Set Result
	//	result after normal conversion
	//	struct/union type
	//	void
	//	ptr to type
	//	ptr to type

	if( !resolve(e[0], context) ||
	    !resolve(e[1], context) ||
	    !resolve(e[2], context) )
                return 0;

	Fund_type op2_ft,
		  op3_ft;
	if( !e[0]->type.isScalar() )
	{	
		printe(ERR_cond_not_scalar, E_ERROR);
		return 0;
	}
	
	if( e[1]->type.isArithmetic(&op2_ft) )
	{

		if( e[2]->type.isArithmetic(&op3_ft) )
		{
			e.type = standardConversionType(&op2_ft, &op3_ft);
			e.flags |= eHAS_RVALUE;
		}
		else if (e[2]->type.isPtrType())
		{
			if( !CCtype_match(e[1]->type, e[2]->type ) )
			{
				printe(ERR_cond_incomp_types, E_ERROR);
				return 0;
			}
			e.type = e[2]->type;
			e.flags |= eHAS_RVALUE;
		}
		else
		{
			printe(ERR_cond_incomp_types, E_ERROR);
			return 0;
		}
	}
	else if( op2_ft==ft_void && op3_ft==ft_void )
	{
		e.type = ft_void;
	}
	else if( e[1]->type.isPtrType() )
	{
		if( !CCtype_match(e[1]->type, e[2]->type ) )
		{
			printe(ERR_cond_incomp_types, E_ERROR);
			return 0;
		}
		e.type = e[1]->type;
		e.flags |= eHAS_RVALUE;
	}
	else
	{
		printe(ERR_cond_incomp_types, E_ERROR);
		return 0;
	}

	return 1;
}

int
nodeResolve::do_Cast(CCtree &e, Resolver *context)
{
	// Verify
	//   first operand integral, second scalar
	//   first operand float, second arithmatic
	//   first pointer, second pointer or integer
	//   first void, second scalar or void
	// Set Result
	//   type of first operand

	Fund_type lhsFt;
	if( e[0]->op!=N_TYPE || !resolve(e[0], context) )
	{
		printe(ERR_invalid_lhs_cast, E_ERROR);
		return 0;
	}

	if( !resolve(e[1], context) )
		return 0;

	e[0]->type.fund_type(lhsFt);

	if( !(e[1]->flags&eHAS_RVALUE) && (lhsFt != ft_void))
	{
		printe(ERR_rhs_cast_novalue, E_ERROR);
		return 0;
	}

	// the following allows casts of ptrs to functions to ptr
	// to non-function types, and vice-versa

	// NOTE cfront 1.2 had trouble with the following if statments
	// when they were coded as a single (very) compound if statment
	if( e[0]->type.isIntegral(&lhsFt) )
	{
		if( !e[1]->type.isScalar() )
		{
			printe(ERR_invalid_cast, E_ERROR);
			return 0;
		}
	}
	else if( lhsFt>=ft_sfloat && lhsFt<=ft_xfloat &&
				e.operand[1]->type.isArithmetic() )
	{
		;
	}
	else if( e.operand[0]->type.isPointer() &&
		 ( e.operand[1]->type.isPtrType() ||
		   e.operand[1]->type.isIntegral()) )
	{
		;
	}
	else
	{
		printe(ERR_invalid_cast, E_ERROR);
		return 0;
	}

	e.type = e[0]->type;

	if (lhsFt != ft_void)
		e.flags |= eHAS_RVALUE;
	return 1;
}

int 
nodeResolve::do_Type(CCtree& e, Resolver* context)
{
	List nodeList;
	buildNodeList(&e, nodeList);	// linearize N_TYPE subtree

	// build a TYPE object
	TYPE currTYPE;
	TYPE prevTYPE;
	CCtree *currNode;
	for(currNode = (CCtree *)nodeList.first(); currNode!=0; 
				      currNode = (CCtree *)nodeList.next())
	{

		switch( currNode->op )
		{
		case N_TYPE:
		{
			if( currNode->valkind == VT_strval )
			{
				// strval is typedef id
				if( !context->lookup(currNode->strval,
							currNode->entry) )
				{
					printe(ERR_no_symbol_info, E_ERROR,
							currNode->strval);
					return 0;
				}
	
				if( !currNode->entry.isTypedef() )
				{
					printe(ERR_not_typedef, E_ERROR,
						currNode->strval);
					return 0;
				}
				if( !currNode->entry.type(currTYPE) )
				{
					printe(ERR_internal, E_ERROR,
					       "nodeResolve::do_Type",__LINE__);
					return 0;
				}
				break;
			}

			switch(currNode->code)
			{
			case S_CHAR:	currTYPE = ft_char; break;
			case S_DOUBLE:	currTYPE = ft_lfloat; break;
			case S_FLOAT:	currTYPE = ft_sfloat; break;
			case S_INT:	currTYPE = ft_int; break;
			case S_LONG:	currTYPE = ft_long; break;
			case S_SHORT:	currTYPE = ft_short; break;
			case S_VOID:	currTYPE = ft_void; break;
			case S_SIGNED:	currTYPE = ft_sint; break;
			case S_UNSIGNED: currTYPE = ft_uint; break;
			default:
				printe(ERR_internal, E_ERROR,
					"nodeResolve::do_Type" ,__LINE__);
				return 0;
			}

			Fund_type rsltFT,
				  ft1,
				  ft2;
			if( prevTYPE.fund_type(ft1) &&  ft1!=ft_none )
			{
				// collapse adjacent fundemental types
				// to a single (fundemental) type
				currTYPE.fund_type(ft2);
				if( mergeFundTypes(rsltFT, ft1, ft2) )
				{
					currTYPE = rsltFT;
				}
				else
				{
					printe(ERR_invalid_type, E_ERROR);
					return 0;
				}
			}
			break;
		}
		case N_AGGR:
		case N_ENUM:
			// need to lookup symbol and return type
			if( !resolve(e.operand[0], context) )
				return 0;
			currTYPE = e[0]->entry;
			break;
		case N_DT_PTR:	
		case N_DT_REF:	
			currTYPE.build_ptrToBase(prevTYPE, context->proc());
			break;
		case N_DT_ARY:
		{
			IntList	*list = new IntList;
			LWP	*l;
			Frame	*f;
			CCtree	*node = currNode;
			CCtree	*save;
			l = context->proc();
			if (l)
				f = l->curframe();
			else
				f = 0;

			// For each array dimension, find no. of
			// elements
			do {
				int	hibound = 1;
				CCtree	*expr; 

				save = node;
				if (node->n_operands == 2)
				{
					expr = node->operand[1];
				}
				else
				{
					expr = node->operand[0];
				}
				node = (CCtree *)nodeList.next();
				if (expr)
				{
					Value	*value;
					Rvalue	rval;
					Itype	itype;

					if (!resolve(expr, context))
						return 0;
					if( !(expr->flags&eHAS_RVALUE) )
					{
						printe(ERR_un_novalue, E_ERROR, "[]");
						return 0;
					}
					if( !expr->type.isIntegral() )
					{
						printe(ERR_un_not_integral, E_ERROR, "[]");
						return 0;
					}

					if ((value = expr->eval(lang, l, f,
						isEvent)) == 0)
						return 0;
					value->rvalue(rval);
					delete value;
					if (rval.isnull())
						return 0;
					if (rval.get_Itype(itype) == SINVALID)
						return 0;
					hibound = (int)itype.iint4;
				}
				list->add(hibound);
			} while(node->op == N_DT_ARY);
			currNode = save;
			currTYPE.build_arrayTYPE(prevTYPE, l, list);
			delete list;
			break;
		}
		case N_DT_FCN:
			currTYPE.build_subrtnTYPE(prevTYPE, context->proc());
			break;
		case N_DT_MPTR:
			// x::* used to declare pointers to members
			// find classNode and insure it is a class then
			// ignore it (until more C++ debug info support)
			if( !context->lookup(currNode->strval,currNode->entry, st_tagnames) )
			{
				return 0;
			}
		
			if( !currNode->entry.isClassType() )
			{
				printe(ERR_class_ref, E_ERROR,
				       currNode->opStr());
				return 0;
			}
			continue;	// don't overwrite prevTYPE
		default:
			printe(ERR_internal, E_ERROR, "do_Type", __LINE__);
			return 0;
		}
		prevTYPE = currTYPE;
	}

	e.type = currTYPE;
	return 1;
}
 
int 
nodeResolve::do_Call(CCtree& e, Resolver* context)
{
	CCtree *implied_thisParam = 0;
	if( e[0]->op == N_REF || e[0]->op == N_DOT )
	{
		// build what may be the first parameter ("this")
		CCtree* instanceNode = new CCtree(e[0]->operand[0]);
		if(  e[0]->op == N_DOT )
		{
			instanceNode = new CCtree(N_ADDR,1,instanceNode);
		}
		if( !resolve(instanceNode, context) )
			return 0;
		implied_thisParam  = new CCtree(N_ZCONS, 2, instanceNode, e[1]);
	}

	if( !resolve(e[0], context) )
	{
		return 0;
	}

	if( e[1] && isFuncProtoType(*e.operand[1], context) )
	{
		// not really a call, rewrite as (prototyped func)
		// id and resolve it
		buildProtoFuncId(e);
		return resolve(&e, context);
	} 
	if( !e[0]->entry.isSubrtn() ) 
	{ 
		printe(ERR_not_function, E_ERROR, e[0]->strval); 
		return 0;
	}

	if( e[1] && !resolve(e[1], context) )
		return 0;


	int bestFit = NO_MATCH;
	CCtree *zNode, *saveNode;
	Symbol formalParam;
	Symbol saveSym;
	Symbol curSym;
	for( context->lookup(e[0]->strval, curSym);
			!curSym.isnull();
				context->getNext(e[0]->strval, curSym) )
	{
		// if global member function, use class instance as 1st param
		if (lang == CPLUS)
		{
			Symbol globSym = 
				context->proc()->find_global(curSym.name());
			if( implied_thisParam && !globSym.isnull() &&
			    strcmp(globSym.name(), curSym.name())==0 )
			{
				zNode = implied_thisParam;
			}
			else
			{
				zNode = e[1];
			}
		}
		else
			zNode = e[1];
			
		formalParam = curSym.child();
		int match = matchArgTypes(zNode, formalParam);
		if( (match>>1) > (bestFit>>1) )  // shift hides ELLIP_PARAM
		{
			e[0]->entry = curSym;
			bestFit = match;
			saveNode = zNode;
			saveSym = formalParam;
		}
		else if( match!=NO_MATCH && match==bestFit )
		{
			printe(ERR_ambiguous_call, E_ERROR,
				e[0]->strval);
			return 0;
		}
	} 

	if( bestFit==NO_MATCH )
	{
		printe(ERR_type_mismatch,E_ERROR,e.operand[0]->strval);
		return 0;
	}
	else if( bestFit==NO_FORMALS && e[1]!=0 )
	{
		// probably no debug information
		printe(ERR_parameter_type, E_WARNING, e[0]->strval);
	}
	else if( !(bestFit&ELLIP_PARAM) )
	{
		if( saveNode != 0 )
		{
			printe(ERR_toomany_args, E_ERROR, e.operand[0]->strval);
			return 0;
		}

		if( !saveSym.isnull() && saveSym.isArgument() )
		{
			printe(ERR_not_enough_args, E_ERROR, e.operand[0]->strval);
			return 0;
		}
	}

	if( implied_thisParam )
	{
	    	if( !context->proc()->find_global(e[0]->entry.name()).isnull() )
		{ 
			// global member function, link implied "this" 
			// param into parameter sub-tree
			e.operand[1] = implied_thisParam;
			if (e.n_operands == 1)
				e.n_operands = 2;
		}
		else
		{
			implied_thisParam->operand[1] =0; // don't delete params
			delete implied_thisParam;
		}
	}

	markParamTypes(e[1], e[0]->entry.child());

	if( !e[0]->entry.type(e.type, an_resulttype) ) 
	{
		printe(ERR_type_assumed, E_WARNING, e[0]->strval);
		e.type = ft_int;	// no result type, assume int return
	}

	Fund_type retFt;
	e.type.fund_type(retFt);
	e.flags = retFt==ft_void? 0: eHAS_PLACE|eHAS_RVALUE;

	return 1;
}

int
nodeResolve::resolve(CCtree *e, Resolver* context)
{
#ifdef DEBUG
	e->dump_CCtree();
#endif

	switch (e->op) {
	case N_AS:
		return do_Assign(*e, context);
	case N_ASPLUS:
	case N_ASMINUS:
		return do_AssignAddOp(*e, context);
	case N_ASDIV:
	case N_ASMOD:
	case N_ASMUL:
		return do_AssignOp(*e, context, C_arithmetic);
	case N_ASLS:
	case N_ASRS:
	case N_ASOR:
	case N_ASAND:
	case N_ASXOR:
		return do_AssignOp(*e, context, C_integral);
	case N_ADDR:

		return do_Addr(*e, context);
	case N_DOT:
	case N_REF:
		return do_Select(*e, context);

	case N_DEREF:
		return do_Deref(*e, context);

	case N_INDEX:
		return do_Index(*e, context);

	case N_ID:
		return do_ID(*e, context);

	case N_AT:      // qualified id
	        return do_At(*e, context);

	case N_REG_ID:
	case N_DEBUG_ID:
        case N_USER_ID:
		Debug_var * var = debug_var_table.Lookup(e->strval);
		if (!var)
		{
			printe(ERR_unknown_debug_var, E_ERROR, e->strval);
			return 0;
		}
		e->type   = TYPE(var->fund_type());
		e->flags |= eHAS_RVALUE | eHAS_PLACE;
		//
		// Note: funny case
		//   allow:   %r5 = 13
		//   but not: &(%r5)
		//   Don't see eHAS_ADDRESS for registers.
		return 1;

	case N_ICON:
	case N_FCON:
	case N_CCON:
		return do_Constant(*e);

	case N_SIZEOF:
		return do_Sizeof(*e, context);
	case N_PLUS:
		return do_AddOp(*e, context);
	case N_MINUS:
		return do_SubOp(*e, context);
	case N_MUL:
	case N_MOD:
	case N_DIV:
		return do_BinOp(*e, context, C_arithmetic);
	case N_AND:
	case N_OR:
	case N_XOR:
	case N_LS:
	case N_RS:
		return do_BinOp(*e, context, C_integral);
	case N_LT:
	case N_GT:
	case N_LE:
	case N_GE:
	case N_EQ:
	case N_NE:
		return do_CompareOp(*e, context);
	case N_ANDAND:
	case N_OROR:
		return do_LogicOp(*e, context);
	case N_UMINUS:
	case N_UPLUS:
		return do_UnaryOp(*e, context, C_arithmetic);
	case N_TILDE:
		return do_UnaryOp(*e, context, C_integral);
	case N_POSTMIMI:
	case N_POSTPLPL:
	case N_PREMIMI:
	case N_PREPLPL:
		return do_PrePostOp(*e, context);
	case N_NOT:
		return do_NotOp(*e, context);
	case N_STRING:
		e->type   = ft_string;
		e->flags |= eHAS_RVALUE;
		return 1;

	case N_COM:
		return do_COMop(*e, context);
		
	case N_QUEST:
		return do_QuestionOp(*e, context);
	case N_AGGR:	// generated for struct/union/class in casts and sizeof
	{
		if( !context->lookup(e->strval ,e->entry, st_tagnames) )
		{
			printe(ERR_no_symbol_info, E_ERROR, e->strval);
			return 0;
		}
		if( e->entry.isStructType() || e->entry.isUnionType() )
		{
			e->type = e->entry;  // struct/union def, entry is type
			return 1;
		}
		
		printe(ERR_invalid_name, E_ERROR, e->strval);
		return 0;
	}
	case N_ENUM:	// generated for keyword enum in casts and sizeof
	{
		if( !context->lookup(e->strval ,e->entry, st_tagnames) )
		{
			printe(ERR_no_symbol_info, E_ERROR, e->strval);
			return 0;
		}
		if( e->entry.isEnumType() )
		{
			e->type = e->entry;  // enum def, entry is type
			return 1;
		}

		printe(ERR_invalid_enum_name, E_ERROR, e->strval);
		return 0;
	}
	case N_ZCONS:	// zero (null ptr) terminated list generated by 
			// parenthised list of tokens (call, cast())
		if( !resolve(e->operand[0], context) )
		{
			return 0;
		}
		e->type = e->operand[0]->type;
		e->flags = e->operand[0]->flags;
		if( e->operand[1] != 0 )
			if( !resolve(e->operand[1], context) )
				return 0;
		return 1;
	case N_CAST:
		return do_Cast(*e, context);
	case N_TYPE:
		return do_Type(*e, context);
	case N_CALL:
		return do_Call(*e, context);
	case N_DOTREF:	// ".*" pointer to member
	case N_REFREF:	// ->* pointer to member
		return do_MemberSelect(*e, context);
	case N_MEM:	// x::yy 
		return do_Member(*e, context);
	case N_OPERATOR: // keyword "operator"
	case N_DT_PTR:  // Handle these ...
	case N_DT_ARY:  //       ... five ...
	case N_DT_REF:  // 		... types ...
	case N_DT_FCN:  // 	   	      ... in ...
	case N_DT_MPTR: //			  ... do_Type
	case N_NTYPE:	// ????
	case N_ELLIPSIS:
		printe(ERR_internal, E_ERROR, "nodeResolve::resolve", __LINE__);
		return 0;
	case N_DELETE:
	case N_NEW:
	default:
		printe(ERR_opr_not_supported, E_ERROR, e->opStr());
		return 0;
	}
}
