/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/util.c	1.24"
#include "amigo.h"

static SX
alloc_temp(expr,scope)
struct Expr_info *expr;
TEMP_SCOPE_ID scope;
{
	if (SCOPE_NOT_EQ(scope,expr->temp_scope)) {
		expr->temp = sy_temp(expr->node.type);
		expr->temp_scope = scope;
		DEBUG_UNCOND(SY_NAME(expr->temp) = ".temp");
		al_add_to_scope(expr->temp,scope);
	}
	return expr->temp;
}

void
store_expr(expr, scope, src)
struct Expr_info *expr;
TEMP_SCOPE_ID scope;
ND1  *src;
{
	ND1 *save_src=t1alloc();

	*save_src = *src;
	src->left = make_temp(expr,scope);
	src->right = save_src;
	src->flags = FF_SEFF;
	/* src->type unchanged */
	src->op = ASSIGN;
}

		/* targ node is clobbered by temp associated with expr */
void
replace_expr_by_temp(expr,scope,targ)
struct Expr_info *expr;
TEMP_SCOPE_ID scope;
ND1 *targ;
{
	if(targ->left)
		tfree((NODE *)(targ->left));
	if(targ->right)
		tfree((NODE *)(targ->right));
	targ->op = NAME;
	targ->rval =alloc_temp(expr,scope);
	targ->lval = 0;
	/* leave the targ type */
	return;
}


ND1 *
make_temp(expr, scope)
struct Expr_info *expr;
TEMP_SCOPE_ID scope;
{
	/* do not al_auto in future (RMA) */
	ND1 *temp = t1alloc();
	temp->op = NAME;

	/* RMA maybe change */
	temp->rval = alloc_temp(expr,scope);
	temp->lval = 0;
	temp->type = expr->node.type;
	return temp;
}

	/* Insert node at the end of the block, update block */

void
insert_node(node,block)
ND1 * node; Block block;
{
	cgq_t *item;
	item = cg_q_insert(block->last);
	item->cgq_op = CGQ_EXPR_ND1;
	item->cgq_arg.cgq_nd1 = node;
	block->last = CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(block->last));
	block->scope.last = block->last;
}
	/* Convert an expr to a real tree */
ND1*
expr_to_nd1(expr)
Expr expr;
{
	ND1 *new_nd1 = t1alloc();
	*new_nd1 = expr->node;
	if(CHILD(expr,left))
		new_nd1->left = expr_to_nd1(CHILD(expr,left));
	if(CHILD(expr,right))
		new_nd1->right = expr_to_nd1(CHILD(expr,right));
	return new_nd1;
}

#define NL node->left
#define NR node->right

/*
		Example: transform
	
		+=			=
	       /  \        =>          / \
	      /    \                  /   \
	     a     XXX	             a     +
					  / \
					 /   \
					a    XXX

( Contents of type member indicated in parentheses )

		+=(t1)			=(t1)
	       /  \        =>          / \
	      /    \                  /   \
	 CONV(t2)  XXX(t2)	   a(t3)  CONV(t3)
            / 				    \
           /				     \ 
          a(t3)			              +(t2)
					     / \
					    /   \
				       CONV(t2) XXX(t2)
					  /
				         /
					a(t3)
*/
    
Boolean
#ifndef NODBG
rewrite_assigns(node, debug_flag)
ND1 *node;
int debug_flag;
#else
rewrite_assigns(node)
ND1 *node;
#endif
{
	ND1 *temp;
	int operator = node->op;

	switch(operator) {
	case ASG PLUS:
	case ASG MINUS:
	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
	case ASG OR:
	case ASG AND:
	case ASG ER:
	case ASG LS:
	case ASG RS:
	case INCR:
	case DECR:
			/* op=, ++, and -- only evaluates LHS once! */
		if(CONTAINS_SIDE_EFFECTS(NL))
			break;
		DEBUG(debug_flag,("Rewriting as ASSIGN expr:\n"));
		DEBUG_COND(debug_flag, print_expr(node->opt));
		temp = t1alloc();

		switch(operator) {
		case INCR:
			temp->op = PLUS;
			break;
		case DECR:
			temp->op = MINUS;
			break;
		default:
			temp->op = NOASG operator;
		}

		temp->type = NL->type;
		temp->right = NR;
		temp->left = NL;
		NL = tr_copy(NL);
		node->op = ASSIGN;
		if(NL->op == CONV) {
			NR = NL;	/* Right child will be CONV */
					/* skip CONV on left to get lval */
			NL = NL->left;
					/* Now finish hooking in CONV node */
			NR->left = temp;
			NR->type = NL->type;
		}
		else {
			NR = temp;
		}
		DEBUG(debug_flag,("New ASSIGN expr:\n"));
		DEBUG_COND(debug_flag, tr_e1print(node,"XX"));
		return true;
		break;
	default:
		break;
	}
	return false;
}

static void
block_delete(block, index)
Block block;
Cgq_index index;
{
	if (block->first == block->last) 
		CGQ_ELEM_OF_INDEX(index)->cgq_op = CGQ_DELETED;
	else if (index == block->last) 
		block->last = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(index));
	else if (index == block->first) 
		block->first = CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(index));
	block->scope.first = block->first;
	block->scope.last = block->last;
}

	/* When the block is known, this function is preferable
	** to cg_q_insert, which messes up the blocks.
	*/
cgq_t *
amigo_insert(block, index)
Block block;
Cgq_index index;
{
	cgq_t *item;
	item = cg_q_insert(index);
	if(index == block->last) {
		block->last = CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(index));
		block->scope.last = block->last;
	}	
	return item;
}

void
amigo_delete(block, index)
Block block;
Cgq_index index;
{
	block_delete(block, index);
	if(CGQ_ELEM_OF_INDEX(index)->cgq_op != CGQ_DELETED)
		cg_q_delete(index);
}

void
amigo_remove(block, index)
Block block;
Cgq_index index;
{
	block_delete(block, index);
	cg_q_remove(index);
}

#ifndef NODBG
int
eval(f, arg1, arg2, arg3)
int (*f)();
int arg1, arg2, arg3;
{
	return (*f)(arg1, arg2, arg3);
}

static Block block_bound;

int
block_boundary(index)
Cgq_index index;
{
	int ret;
	if(! block_bound ) ret = 0;
	else if(block_bound->first == index) {
		ret = block_bound->block_count;
		block_bound = block_bound->next;
	}
	else ret = 0;
	return(ret);
}

void
print_trees(level,top_down_basic)
int level;
Block top_down_basic;
{
	
	block_bound = top_down_basic;
	cg_putq(level);
}
#endif
