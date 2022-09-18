/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:i386/costing.c	1.2.1.19"
#include "amigo.h"
#include "costing.h"

int cse_threshold = CSE_THRESHOLD;
int exp_threshold = EXP_THRESHOLD;
int sr_threshold = SR_THRESHOLD;
int cf_threshold = CF_THRESHOLD;
int cm_threshold = CM_THRESHOLD;
int pass_thru_threshold = PASS_THRU_THRESHOLD;
int lu_threshold = LU_THRESHOLD; /* Loop unrolling */
int lu_l_size_threshold = LU_L_SIZE_THRESHOLD; /* Max size of loop for l_u */

#define C_DEFAULT 0
#define ADDRESS_MODE 1
#define ADDRESS 2
#define C 3
#define C_POW 4
#define INDEX 5
#define POINTER 10

#define TEST_COST 3
#define JUMP_COST 4
#define COND_COST (TEST_COST + JUMP_COST/2)	/* if then else code */

#define ARITH_COST 2
#define MULT_COST 9
#define DIV_COST 14
#define FP_COST 30

#define UNDER_PLUS 1
#define UNDER_STAR 2
#define UNDER_BOOL 3
#define UNDER_FP_OP 4

static int
recursive_cost(node,context)
ND1 *node;
int context;
{
	int cost;
	ND1 *l;

	/* CSE's can attempt to cost expressions with embedded assignments.
	** For these cases, the cost is the cost of the right.
	*/
	if (node->op == ASSIGN)
		return recursive_cost(node->right,0);

	if (TY_ISFPTYPE(node->type) && node->op != QUEST && node->op != COLON)  {
		switch(node->op) {
		case UPLUS:
			cost = recursive_cost(node->left,0);
			break;
		case PLUS: 
		case MINUS:
		case MUL:
		case DIV:
			cost = (recursive_cost(node->left,UNDER_FP_OP) +
				recursive_cost(node->right,UNDER_FP_OP) + 
				FP_COST);
			if (cost == FP_COST)
				/* Both left and right operands are free -
				** but at least one has to be loaded.	
				*/
				cost += LOAD_COST(node->type);
			break;
		case CONV:
			l = node->left;
			switch (node->type) {
			case TY_LDOUBLE:
				switch (l->type) {
				case TY_LDOUBLE:
				case TY_DOUBLE:
				case TY_FLOAT:
				case TY_INT:
					cost = recursive_cost(l,context);
					break;
				default:
					cost =  FP_COST + 
						recursive_cost(l,0);
				}
			case TY_DOUBLE:
				switch (node->left->type) {
				case TY_DOUBLE:
				case TY_FLOAT:
				case TY_INT:
					cost = recursive_cost(l,context);
					break;
				default:
					cost =  FP_COST + 
						recursive_cost(l,0);
				}
			case TY_FLOAT:
				switch (node->left->type) {
				case TY_FLOAT:
				case TY_INT:
					cost = recursive_cost(l,context);
					break;
				default:
					cost =  FP_COST + 
						recursive_cost(l,0);
				}
			}
			break;
		case STAR: 
			cost = (recursive_cost(node->left,UNDER_STAR));
			break;
		case NAME:
		case FCON:
			/* No need to load if under binary fp operation */
			cost = context==UNDER_FP_OP ? 0 : LOAD_COST(node->type);
			break;
		case UNARY MINUS:
			/* Operand must be loaded */
			cost = recursive_cost(node->left,0) + FP_COST;
			break;
		case LET:
			cost = recursive_cost(node->left,0) +
			       recursive_cost(node->right,0);
			break;
		case CSE:
			cost = 0;	/* Already loaded */
			break;
		default:
#ifndef NODBG
			dprint1(node);
#endif
			Amigo_fatal("Bad fp op");
		}
		return node->opt->cost = cost;
	}

	switch(node->op) {

	case QUEST:
		cost = COND_COST + 
			(recursive_cost(node->left, UNDER_BOOL) +
			recursive_cost(node->right, 0));
		break;
	case UPLUS:
		cost = recursive_cost(node->left,0);
		break;
	case COLON:
		cost = (recursive_cost(node->left,0)+recursive_cost(node->right,0))/2;
		break;
	case OROR:
	case ANDAND:
		cost = recursive_cost(node->left,UNDER_BOOL) +
			COND_COST + recursive_cost(node->right,UNDER_BOOL)/2 +	
			COND_COST/2;
		if (context != UNDER_BOOL) 
			cost += ARITH_COST;
		break;
	case ICON:
		if (context == UNDER_FP_OP)
			cost = LOAD_COST(TY_DOUBLE);
		else if (node->rval != 0 && !TY_ISPTR(node->type))
			cost =  LOAD_COST(node->type);	/* memory access */
		else
			cost = 0;
		break;
	case MINUS:
	case PLUS:
		cost = recursive_cost(node->left, UNDER_PLUS) +
			recursive_cost(node->right, UNDER_PLUS);
		if (context != UNDER_STAR)
			cost += ARITH_COST;
		break;
	case LS:

#define SCALE_FACTOR(node) node->op == ICON && node->rval == 0 && \
	(node->lval == 1 || node->lval == 2 || node->lval == 3)

		if (context == UNDER_PLUS) {
			
			if (SCALE_FACTOR(node->right))
				cost = recursive_cost(node->left,0)+1;
			else if (SCALE_FACTOR(node->left))
				cost = recursive_cost(node->right,0)+1;
			else
				cost = recursive_cost(node->left,0) +
		    	    	    recursive_cost(node->right,0) + ARITH_COST;
		}
		else
			cost = recursive_cost(node->left,0) +
		    	    recursive_cost(node->right,0) + ARITH_COST;
		break;
	case MUL:
		cost =  recursive_cost(node->left, 0) +
			recursive_cost(node->right,0) + MULT_COST;
		break;
	case UNARY AND:
		if (context == UNDER_PLUS )
			cost = 0;
		else
			cost = LOAD_COST(node->type);
		break;
	case NAME:
		{
		SY_CLASS_t class;
		if (node->rval == 0) {	/* STAR ICON for example */
			cost = LOAD_COST(node->type);
			break;
		}
		class = SY_CLASS(node->rval);
		if (context == UNDER_FP_OP)
			cost = LOAD_COST(TY_DOUBLE);
		else if (class == SC_AUTO || class == SC_PARAM)
			cost = 0;
		else
			cost = LOAD_COST(node->type);
		}
		break;
	case STAR:
		cost = LOAD_COST(node->type) + recursive_cost(node->left, UNDER_STAR);
		break;
	case DIV: case MOD:
		cost = recursive_cost(node->left,0) +
			recursive_cost(node->right,0) +
			DIV_COST;
		break;
	case CONV:
		if (TY_ISFPTYPE(node->left->type)) {
			cost = recursive_cost(node->left, 0)+  FP_COST;
		}
		else
			cost = recursive_cost(node->left, 0) + ARITH_COST;
		break;
	case FLD:
		cost = 3*ARITH_COST + recursive_cost(node->left,0);
		break;
	case COMOP: 
		cost = recursive_cost(node->left,0) +
			recursive_cost(node->right,0) ;
		break;
	case STRING:
		cost = 0;
		break;
	case RS: case AND: case OR: case ER: 
		cost = recursive_cost(node->left,0) + ARITH_COST +
			recursive_cost(node->right,0) ;
		break;
	case UNARY MINUS: case COMPL:
		cost = ARITH_COST + recursive_cost(node->left,0);
		break;

	/* The problem with costing NOT and all relational operators
	** is that if we are computing them for condition codes, they
	** are cheap.  If we are computing them for value, they are
	** expensive.  Cheap is conservative -- Take that approach for now.
	*/
	case NOT: 
		cost = recursive_cost(node->left,UNDER_BOOL) + TEST_COST;
		if (TY_ISFPTYPE(node->left->type) )
			cost += FP_COST;
		break;
	case LT: case LE: case GE: case GT: case EQ: case NE: case ULE: case ULT: case UGE: case UGT:
		cost =  recursive_cost(node->left,0) +
			recursive_cost(node->right,0);
		if (context != UNDER_BOOL)
			cost += TEST_COST;
		if (TY_ISFPTYPE(node->left->type) )
			cost += FP_COST;
		break;

	case CSE:
		cost = 0;
		break;	/* No additional cost */
	case LET:
		cost =  recursive_cost(node->left,0) +
			recursive_cost(node->right,0);
		break;
	default: 
#ifndef NODBG
		dprint1(node);
#endif
		Amigo_fatal("Bad op");
	}
	return cost;
}

/* cost of evaluating "node" into a register */
int
cost(node)
ND1 *node;
{
	int type;
	int c;
	c = recursive_cost(node, 0);
	node->opt->cost = c;
	return c;
}
