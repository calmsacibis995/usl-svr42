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
#ident	"@(#)debugger:libexp/common/CCtree.C	1.7"

#include "CC.h"
#include "CCtree.h"
#include "Language.h"
#include "Interface.h"
#include "str.h"
#include "Value.h"
#include "LWP.h"
#include "Frame.h"
#include "Itype.h"
#include "Machine.h"
#include "cvt_util.h"
#include <string.h>
#include <stdio.h>


CCtree::CCtree(Operator o, int cnt, CCtree *a, CCtree *b, CCtree *c)
{
    if (cnt > EMAX_ARITY) 
	printe(ERR_internal, E_ERROR, "CCtree::CCtree", __LINE__);

    operand[0] = a;
    operand[1] = b;
    operand[2] = c;

    // -- normalize cnt field; i.e. don't count trialing 0 branches.
    //    Note: N_ZCONS is a special case. It always cnt two.
    //
    if (cnt == 3 && c == 0)                 cnt = 2;
    if (cnt == 2 && b == 0 && o != N_ZCONS) cnt = 1;
    if (cnt == 1 && a == 0)                 cnt = 0;

    op         = o;
    n_operands = cnt;
    valkind    = VT_none;
    flags      = 0;
    lwp = 0;
    frame = 0;
    nodeVal = 0;

    entry.null();  // must clear in case node is a retread.
    type.null();
}

CCtree::CCtree(CCtree *e)
{
    op = e->op;
    n_operands = e->n_operands;

    register int i = 0;
    for ( ; i < n_operands ; i++ ) 
    {
	operand[i] = new CCtree(e->operand[i]);
    }
    for ( ; i < EMAX_ARITY ; i++ ) 
    {
	operand[i] = 0;
    }

    entry = e->entry;
    type = e->type;
    flags = e->flags;
    valkind = e->valkind;
    memcpy(rawbytes, e->rawbytes, XFLOAT_SIZE);
    lwp	= e->lwp;
    frame = e->frame;
    nodeVal = e->nodeVal;
}

CCtree::CCtree(Const& c)
{
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags = 0;
	lwp = 0;
	frame = 0;
	nodeVal = 0;
	entry.null(); 
	type.null();

	switch (c.const_kind)
	{
	case CK_CHAR:
		op       = N_CCON;
		valkind  = VT_char;
		charval  = c.c;
		break;
	case CK_INT:
		op       = N_ICON;
		valkind  = VT_long;
		longval  = c.l;
		break;
	case CK_UINT:
		op       = N_ICON;
		valkind  = VT_ulong;
		ulongval = c.ul;
		break;
	case CK_FLOAT:
		op       =  N_FCON;
		valkind  =  VT_float;
		floatval =  c.d;
		break;
	case CK_XFLOAT:
		op       =  N_FCON;
		valkind  =  VT_xfloat;
		cvt_to_internal(rawbytes, c.x);
		break;
	default:
		printe(ERR_internal, E_ERROR, "CCtree_Const", __LINE__);
	}
}

CCtree::CCtree(Operator node_type, int c, ValKind v)
{
	op = node_type;
	valkind = v;
	code    = c;
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags      = 0;
        lwp = 0;
	frame = 0;
	nodeVal = 0;

	entry.null();  // must clear in case node is a retread.
	type.null();
}

CCtree::CCtree(Operator node_type, char *string)
{
	op = node_type;
	valkind = VT_strval;
	strval  = makestr(string);
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags = 0;
	lwp = 0;
	frame = 0;
	nodeVal = 0;
	entry.null(); 
	type.null();
}

CCtree *
CCtree::CCtree_append(CCtree *node)  // length(list)=#(N_ZCONS nodes)
{
	register CCtree *n;

	n = (node->op == N_ZCONS ? node : new CCtree(N_ZCONS, 2, node, 0));

	if (node == 0) return this;

	if (this->op != N_ZCONS)
	{
		this = new CCtree(N_ZCONS, 2, this, 0);
	}

	register CCtree *last = this;
	register CCtree *next;

	while ((next = last->operand[1]) != 0)
	{
		last = next;
	}
	last->operand[1] = n;  // tie new node to end of list.

	return this;
}

void
CCtree::re_init(Operator node_type, char *string)
{
	for(int i = 0; i < n_operands; i++)
		delete operand[i];
	if (valkind == VT_strval)
		delete strval;
	delete nodeVal;

	op = node_type;
	valkind = VT_strval;
	strval  = makestr(string);
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags = 0;
	lwp = 0;
	frame = 0;
	nodeVal = 0;
	entry.null(); 
	type.null();
}

// reinitiallize current tree with new values
void
CCtree::re_init(Operator o, int cnt, CCtree *a, CCtree *b, CCtree *c)
{
    if (cnt > EMAX_ARITY) 
	printe(ERR_internal, E_ERROR, "CCtree::CCtree", __LINE__);

    for(int i = 0; i < n_operands; i++)
	delete operand[i];

    operand[0] = a;
    operand[1] = b;
    operand[2] = c;

    // -- normalize cnt field; i.e. don't count trialing 0 branches.
    //    Note: N_ZCONS is a special case. It always cnt two.
    //
    if (cnt == 3 && c == 0)                 cnt = 2;
    if (cnt == 2 && b == 0 && o != N_ZCONS) cnt = 1;
    if (cnt == 1 && a == 0)                 cnt = 0;

    op         = o;
    n_operands = cnt;
    valkind    = VT_none;
    flags      = 0;
    lwp = 0;
    frame = 0;
    nodeVal = 0;

    entry.null();  // must clear in case node is a retread.
    type.null();
}

// General evaluation interface
Value *
CCtree::eval(Language lang, LWP *lwp, Frame *frame, int isEvent)
{ 
	return CCeval(lang, this, lwp, frame, isEvent); 
}

CCtree&
CCtree::operator=(CCtree& e)
{
	op = e.op;
	n_operands = e.n_operands;

	register int i = 0;
	for ( ; i < n_operands ; i++ ) 
	{
		// make a new copies of operands
		delete operand[i];
		operand[i] = new CCtree(e.operand[i]);
	}

	for ( ; i < EMAX_ARITY ; i++ ) 
	{
		operand[i] = 0;
	}
	
	entry = e.entry;
	type = e.type;
	flags = e.flags;
	valkind = e.valkind;
    	memcpy(rawbytes, e.rawbytes, XFLOAT_SIZE);
	lwp	= e.lwp;
	frame = e.frame;
	nodeVal = e.nodeVal;

	return *this;
}

CCtree::CCtree(Symbol &sym)
{
	op = N_ID; 
	n_operands = 0;
	operand[0] = operand[1] = operand[2] = 0;
	entry = sym;
	valkind = VT_strval;
	strval = makestr(sym.name());
	flags = 0;
	lwp = 0;
	frame = 0;
	nodeVal = 0;

	if( entry.isSubrtn() || entry.isLabel() )
	{
		type = ft_pointer;	// not true, but it works
	}
}

#define DONT_MARK	0
#define MARK_VAR	1
#define MARK_CONST_VAR	2
#define MARK_PTRVAR	3

int
CCtree::triggerList(Language lang, LWP *lwp, Resolver *context, List &trigList)
{
	trigList.clear();

	if (!buildTriggerList(lang, lwp, context, trigList, DONT_MARK))
		return 0;

	if (trigList.isempty() && 
		(n_operands == 0) && (op == N_ICON))
	{
		// special case handling for a single constant
		// treat it as an address
		//
		// set nodeVal of etree
		TYPE		t(ft_ulong);
		Obj_info	obj(lwp, t, ulongval);
		TriggerItem	trigItem;

		//set the trigger item expression 
		trigItem.setNode(this);	

		if (nodeVal)
			delete nodeVal;
		nodeVal = new Value(obj);
		flags |= eHAS_PLACE | eHAS_ADDRESS;
		trigItem.setGlobalContext(context->proc());
		trigList.add((void *)(new TriggerItem(trigItem)));
	}
	return 1;
}

int
CCtree::buildTriggerList(Language lang, LWP *lwp, Resolver *context,
	List &trigList, int markState)
{
	static int lvalVaries=0;// synthesised attribute used in conjuction
				// with inherited attribute "markState" to
				// determine if the reinit flag should be
				// set in the trigger item corresponding to
				// this node
	CCtree *e;
	TriggerItem trigItem;

	trigItem.setNode(this);	//set the trigger item expression 

	switch(op)
	{
	case N_INDEX:
	{
		int lhsVaries;
		if( !operand[0]->buildTriggerList(lang, lwp, context,
						trigList, MARK_PTRVAR) )
		{
			trigList.clear();
			return 0;
		}
		lhsVaries = lvalVaries;
		
		// A[x]: context is context of A 
		if( lvalVaries )
		{
			trigItem.copyContext(*(TriggerItem *)trigList.item());
		}
		// better be an array id
		else if (( operand[0]->op != N_ID ) ||
			!operand[0]->setIdContext(trigItem, lwp, frame) )
		{
			trigList.clear();
			return 0;
		}

		if( !operand[1]->buildTriggerList(lang, lwp, context,
							trigList, MARK_VAR) ||
		    !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}

		if( markState!=DONT_MARK && (lhsVaries||lvalVaries) )
		{
			trigItem.setReinit();
		}
		else
		{
			lvalVaries = 0;
		}

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_REF:
	case N_DEREF:
		if( !operand[0]->buildTriggerList(lang, lwp, context,
							trigList, MARK_VAR) ||
		    !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}
		
		// A->x: context is context of A -  or
		// *A: context is context of A
		trigItem.copyContext(*(TriggerItem *)trigList.item());

		if( markState!=DONT_MARK && lvalVaries )
		{
			trigItem.setReinit();
		}
		else
		{
			lvalVaries = 0;
		}

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	case N_DOT:
		if( !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}

		// A.b: context is context of A (A must be ID)
		if( !operand[0]->setIdContext(trigItem, lwp, frame) )
		{
			trigList.clear();
			return 0;
		}

		if( markState != DONT_MARK )
		{
			// with respect to reinit, N_DOT behaves
			// like leaf node
			trigItem.setReinit();
			lvalVaries = 1;
		}

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	case N_DOTREF:	
		if( !operand[1]->buildTriggerList(lang, lwp, context,
						       trigList, MARK_VAR) ||
		    !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}
	
		// A.*B: context is context of A
		if( !operand[0]->setIdContext(trigItem, lwp, frame) )
		{
			trigList.clear();
			return 0;
		}

		if( markState!=DONT_MARK )
		{
			trigItem.setReinit();
			lvalVaries = 1;
		}

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	case N_REFREF:
		if( !operand[0]->buildTriggerList(lang, lwp, context,
						       trigList, MARK_VAR) ||
		    !operand[1]->buildTriggerList(lang, lwp, context,
						       trigList, MARK_VAR) ||
		    !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}
	
		// A->*B: context is context of A
		if( !operand[0]->setIdContext(trigItem, lwp, frame) )
		{
			trigList.clear();
			return 0;
		}

		if( markState != DONT_MARK )
		{
			trigItem.setReinit();
			lvalVaries = 1;
		}

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	case N_AT:
	{
		if( !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}

		// find id node
		for( e = this->operand[0]; e->op != N_ID; e = e->operand[0] )
			;

		//  set context into id node and add it to the list
		if( !e->setIdContext(trigItem, (this->lwp? this->lwp: lwp),
								this->frame) )
		{
			trigList.clear();
			return 0;
		}

		trigItem.setNode(e);	//set the trigger item expression 
					// tree pointer to the id node

		if(( markState==MARK_VAR || markState==MARK_CONST_VAR ) ||
			( markState==MARK_PTRVAR && e->type.isPointer() ))
		{
			trigItem.setReinit();
			lvalVaries = 1;
			trigList.add((void *)(new TriggerItem(trigItem)));
		}
		else if( markState == DONT_MARK )
		{
			trigList.add((void *)(new TriggerItem(trigItem)));
		}
			
		break;
	}
	case N_ID:
	{
		if( !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}

		if( !setIdContext(trigItem, lwp, frame) )
		{
			trigList.clear();
			return 0;
		}

		if(( markState==MARK_VAR || markState==MARK_CONST_VAR ) ||
			( markState==MARK_PTRVAR  && this->type.isPointer() ))
		{
			trigItem.setReinit();
			lvalVaries = 1;
			trigList.add((void *)(new TriggerItem(trigItem)));
			break;
		}
		else if( markState == DONT_MARK )
		{
			trigList.add((void *)(new TriggerItem(trigItem)));
		}

		break;
	}
	case N_ICON:
		if( !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}
		if( markState == MARK_CONST_VAR )
		{
			// markState == MARK_CONST_VAR implies immediate
			// parent is a deref node, put out a trigger item
			trigItem.setGlobalContext(context->proc());
			trigItem.setReinit();
			lvalVaries = 1;

			trigList.add((void *)(new TriggerItem(trigItem)));
		}
		break;
	case N_REG_ID:
		if( !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}
		trigItem.lwp = context->proc();
		trigItem.frame = trigItem.lwp->curframe()->id();
		trigItem.scope = NULL_SCOPE;

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	case N_CAST:
		if( !CCresolve(lang, this, context, 1) )
		{
			trigList.clear();
			return 0;
		}

		// propogate markState to object being cast
		if( !operand[1]->buildTriggerList(lang,lwp,context,trigList,
								markState) )
		{
			return 0;
		}
		break;
	case N_DEBUG_ID: 
	case N_USER_ID:
	case N_ADDR:
		return 1;
	case N_AS:
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
	case N_CALL:
	case N_DELETE:
	case N_NEW:
	case N_POSTMIMI:
	case N_POSTPLPL:
	case N_PREMIMI:
	case N_PREPLPL:
		printe(ERR_stop_operator, E_ERROR, getCCOperatorStr(op));
		return 0;
	default:
		for( int i = 0; i < n_operands; i++ )
		{
			if( !operand[i]->buildTriggerList(lang,lwp,context,trigList,
			     markState==MARK_CONST_VAR? MARK_VAR: markState) )
			{
				return 0;
			}
		}
	}

	return 1;
}

int 
CCtree::getTriggerLvalue(Place &place)
{
	if (!nodeVal || nodeVal->object().loc.isnull())
		return 0;

	place = nodeVal->object().loc;
	return 1;
}

int 
CCtree::getTriggerRvalue(Rvalue &rval)
{
	if (!nodeVal || !nodeVal->rvalue(rval))
		return 0;

	return 1;
}

int 
CCtree::exprIsTrue(Language, LWP*, Frame*)
{
	Rvalue rval;
	int isTrue;

	if (nodeVal && nodeVal->rvalue(rval))
	{
		if( cvtTo_and_return_INT(&rval, isTrue) )
			return isTrue;
	}
	return 0;
}

ParsedRep*
CCtree::copyEventExpr(List& old_tl, List& new_tl, LWP *lwp)
{

	CCtree *newCCtree = new CCtree(this);
	(void) old_tl.first();
	newCCtree->buildNewTrigList(new_tl, old_tl, this, lwp);

	return (ParsedRep *)newCCtree;
}

//
// create a new trigger list and link the nodes to "this" expression tree
//
void
CCtree::buildNewTrigList(List& new_tl, List& old_tl, CCtree* oldNode, LWP* lwp)
{
	for(int i=0; i < n_operands; i++)
	{
		operand[i]->buildNewTrigList(new_tl, old_tl,
						oldNode->operand[i], lwp);
	}

	TriggerItem *curItem = (TriggerItem *)old_tl.item();
	if( curItem->getNode() == oldNode )
	{
		// build new trigger item, link it to new CCtree node,
		// and add item to new trigger list
		TriggerItem *ti = new TriggerItem(*curItem);
		ti->setNode(this);
		ti->lwp = lwp;
		new_tl.add((void *)ti);
		old_tl.next();		// update list to return next item
	}
}

  /*********************** CCtree Build Utilities ***********************/
ParsedRep *
CCtree::clone()		// deep copy, recursive
{
    if ( !this )	// abort()?
         return 0;
    return ( (ParsedRep *)new CCtree(this) );
}

CCtree::~CCtree()		// recursive delete
{
	for( int i = 0 ; i < n_operands ; i++ )
		delete operand[i];
	if (valkind == VT_strval)
		delete strval;
	delete nodeVal;
}

  /*********************** CCtree dump routines *************************/

#undef OP_ENTRY
#ifdef DEBUG
#define OP_ENTRY(e, s, t, v) { v, t, s },
#else
#define OP_ENTRY(e, s, t, v) { v, t },
#endif

static struct Ccop_table {
	int   enumval;
	char *opString;
#ifdef DEBUG
	char *printname;
#endif
} ccop_table[] = {
#include "CCops.h"
    { 0, 0 }
};

char *
getCCOperatorStr(Operator op)
{
	for (register struct Ccop_table *p = ccop_table; p->opString; ++p)
	{
		if (p->enumval == op) return p->opString;
   	}
	return 0;
}

char *
CCtree::opStr()
{
	return getCCOperatorStr(this->op);
}

#ifdef DEBUG
char *
CCtree::CCtree_op_string(Operator op)
{
    static char buff[10];

    for (register struct Ccop_table *p = ccop_table; p->printname; ++p) {
	if (p->enumval == op) return p->printname;
    }
    sprintf(buff, "%d", op);
    return buff;
}


inline void
indent(int n) { printf("%*s", 4*n, ""); }

void 
CCtree::dump_value(int level)
{
    double	d;
    indent(level);
    switch (valkind) {
    case VT_char:   printf("'%c'", charval);        break;
    case VT_long:   printf("long(%d)", longval);    break;
    case VT_ulong:  printf("ulong(%d)", ulongval);  break;
    case VT_float:  printf("float(%g)", floatval);  break;
    case VT_xfloat: 
		    extended2double(rawbytes, &d);
		    printf("xfloat(as double)(%g)", d);  break;
    case VT_code:   printf("code(%d)", code);       break;
    case VT_strval: printf("\"%s\"", strval);       break;
    case VT_none:   break;
    default:        printf("unknown valkind: %d", valkind);
    }
    printf("\n");
}

static struct CCtree_flags {
    int   val;
    char *str;
} etree_flags[] = {
    eHAS_PLACE,   "eHAS_PLACE",
    eHAS_ADDRESS, "eHAS_ADDRESS",
    eHAS_RVALUE,  "eHAS_RVALUE",
    0, 0
};

void 
CCtree::dump_etree_flags(int flags)
{
    printf("{ ");
    for (register CCtree_flags *p = etree_flags; p->val != 0; ++p) {
	if ((flags & p->val) == p->val) {	// exact match to allow cords.
	    printf("%s ", p->str);
	}
    }
    printf("}");
}

void 
CCtree::dump_CCtree(int level)
{
    indent(level);
    if (this != 0) {
	printf("<op:%s>[%d]@%x", CCtree_op_string(op), n_operands, this);
	if (flags != 0) this->dump_etree_flags(flags);
	printf("\n");
	if (valkind != VT_none) {
	    this->dump_value(level);
	}
	for (int i = 0; i < n_operands; ++i) {
	    operand[i]->dump_CCtree(level+1);
	}
    } else {
	printf("(null CCtree*)\n");
    }
}
#endif /*DEBUG*/

//
// TriggerItem Rtns
//
int
CCtree::setIdContext(TriggerItem &ti, LWP *localLwp, Frame *qualifingFrame)
{
	if( op == N_ICON )
	{
		ti.setGlobalContext(localLwp);
		return 1;
	}

	if( op != N_ID )
	{
		printe(ERR_internal, E_ERROR, "CCtree::setIdContext", __LINE__);
		return 0;
	}

	// MORE use type and Symbol rtns
	if (entry.isGlobalVar())
	{
		ti.setGlobalContext(localLwp);
		return 1;
	}

	if( entry.isSubrtn() )
	{
		printe(ERR_stop_func, E_ERROR);
		return 0;
	}

	ti.lwp = localLwp;

	// automatics, parameters, and static variables
	if( qualifingFrame )
	{
		// frame qualified id, use qualifingFrame and no scope
		ti.frame = qualifingFrame->id();
		ti.scope = NULL_SCOPE;
		return 1;
	}

	Symbol idParent = entry;
	Tag parentTag;
	do
	{
		idParent = idParent.parent();
		parentTag = idParent.tag();

	} while( parentTag!=t_subroutine &&
		 parentTag!=t_global_sub && parentTag!=t_functiontype &&
	         parentTag!=t_sourcefile );

	if( parentTag == t_sourcefile )
	{
		// static variable, no scope, no frame
		ti.frame.null();
		ti.scope = NULL_SCOPE;
		return 1;
	} 

	if( !strcmp(localLwp->symbol_name(localLwp->current_function()),
		    localLwp->symbol_name(idParent)) )
	{
		// id is local to current function, frame <- current frame
		ti.frame = ti.lwp->curframe()->id();
	}

	Attribute *attr;
	if( !(attr=idParent.attribute(an_lopc)) || attr->form!=af_addr)
	{
       		printe(ERR_internal, E_ERROR,
                               	"CCtree::setIdContext", __LINE__ );
		return 0;
       	}
	ti.scope = attr->value.addr;
	
	return 1;
}
