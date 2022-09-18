/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/local_info.c	1.34"
#include "amigo.h"

static void PROTO(block_kills,(struct Block *));
static void PROTO(block_anticipate,(Block, Expr_set));
static Object_list PROTO(object_list_new,(Object));
static Object_list PROTO(object_list_add,(Object,Object_list,Object_set));


Object_list object_list_new (object)
Object object;
{
	Object_list o;

	/* RMA should not neccessarily be global arena */
	o = Arena_alloc(global_arena,1,struct Object_list);
	o ->object = object;
	o ->next = 0;
	return o;
}

void
local_info(blocks) 
struct Block *blocks;
{
	struct Block *block;

	for(block=blocks; block; block=block->next) {
		block_kills(block);
	}
}

void
locally_anticipate(arena)
Arena arena;
{
	REVERSE_DEPTH_FIRST(block_ptr)
		Expr_set kills = Expr_set_alloc(arena); /* global_arena ?? PSP */
		block_anticipate(*block_ptr,kills);
 		DEBUG(aflags[df].level&2, ("block %d locally antic: %s\n",
 			(*block_ptr)->block_count,
 			bv_sprint((*block_ptr)->loc_antic)));
	END_DEPTH_FIRST
}

#ifndef NODBG

void
pr_object(object)
Object object;
{
	fprintf(stderr,"%s[%d](%d) ", 
		( object->fe_numb ? SY_NAME(object->fe_numb):
			(is_generic_call(object)? "generic_call":
				(is_generic_deref?"generic_deref":"?")))
		, object->fe_numb, object->setid);
}

void
pr_object_list(list)
Object_list list;
{
	while(list) {
		pr_object(list->object);
		list = list->next;
	}
	fprintf(stderr,"\n");
}

void
pr_object_set(set)
Object_set set;
{
if(set) {
BV_FOR(set,bit)
      	struct Object_info *p = debug_object(bit);
      	assert(p && p->setid == bit);
      	fprintf(stderr,"%s(%d) ",p->fe_numb ? SY_NAME(p->fe_numb):"?",bit);
END_BV_FOR
}
	fprintf(stderr,"\n");
}
#endif
		/* look for all objects killed in this block
		   also gather local induction variables */
static void
block_kills(block)
struct Block *block;
{
	Cgq_index first = CGQ_NULL_INDEX;
	int flags = 0;
	Cgq_index last;
      	Object_list kill_list = NULL;
      	Object_set not_ind_vars = Object_set_alloc(GLOBAL);
      	block->ind_vars = Object_set_alloc(GLOBAL);
	bv_init(false,block->ind_vars);
	bv_init(false,not_ind_vars);
	FOR_ALL_ND1_IN_BLOCK(block,flow,node,index)
			Boolean is_ind;
			is_ind = false;
			if (first == CGQ_NULL_INDEX)
				first = index;
			last = index;
			flags |= node->opt->flags;
			switch(node->op) {
			case ASG PLUS: case ASG MINUS:
				if(node->right->op != ICON) break;
				/* FALLTHRU */
			case INCR: case DECR:
				if(node->left->op != NAME || ! EXCEPT_OPT(node->left)){
					break;
				}
					/*
					** Next statement guarantees
					** that members in structs will
					** not be used as ind variables.
					** The problem is that AMIGO
					** does not consider these to be
					** bona fide objects.  At some point
					** this restriction should be removed.
					*/
				if(! TY_ISSCALAR(SY_TYPE(node->left->rval)) )
					break;
				kill_list = node_kills(node,kill_list,(Object_set)0);
				bv_set_bit(node->left->opt->object->setid,
					block->ind_vars);
				is_ind = true;
				break;
			default:
				break;
			}
			if(! is_ind ) kill_list =
				node_kills(node,kill_list,not_ind_vars);
				
	END_FOR_ALL_ND1_IN_BLOCK

	if(flags & HAS_CALL)
		block->block_flags |= BL_CONTAINS_CALL;
	if (flags & HAS_DIVIDE)
		block->block_flags |= BL_CONTAINS_DIV;
	if (flags & HAS_FP_DIVIDE)
		block->block_flags |= BL_CONTAINS_FP_DIV;

	if (first == CGQ_NULL_INDEX) {
		first =  block->first;
		last =  block->last;
	}
	block->scope = al_create_scope(first,last);

	bv_minus_eq(block->ind_vars,not_ind_vars);
	block->kills = kill_list;

	DEBUG(aflags[li].level&4,("Block %d induction vars: ",
		block->block_count));
	DEBUG_COND(aflags[li].level&4,pr_object_set(block->ind_vars));
	DEBUG(aflags[li].level&4,("\nKills but not ind_vars: "));
	DEBUG_COND(aflags[li].level&4,pr_object_set(not_ind_vars));
	DEBUG(aflags[li].level&4,("\n"));
	DEBUG(aflags[li].level&1,("block %d kills: ",block->block_count));
	DEBUG_COND(aflags[li].level&1,pr_object_list(block->kills));
}


	/* 
	** Gather exprs killed in this block by traversing the (object)
	** kills list.  The expr_kills member is up to date only if
	** this function has been called and no new exprs have
	** been added to the block.
	*/
void
block_expr_kills(block,init)
Block block;
int init; /* true if block->exprs_killed needs to be alloc'd */
{
	Object_list objects;
	if(init)
		block->exprs_killed = Expr_set_alloc(global_arena);
	bv_init(false,block->exprs_killed);
	for(objects = block->kills; objects; objects = objects->next)
		bv_or_eq(block->exprs_killed, objects->object->kills);
	DEBUG(aflags[li].level&8,
		("block->exprs_killed, block %d: ", block->block_count));
	DEBUG_COND(aflags[li].level&8, bv_print(block->exprs_killed));
 
}

Object_list
node_kills(node,kill_list,not_ind_vars)
ND1 *node;
Object_list kill_list;
Object_set not_ind_vars;
{
	if(! CONTAINS_SIDE_EFFECTS(node)) return kill_list;

	if(node->left)
		kill_list = node_kills(node->left,kill_list,not_ind_vars);

	if(node->right)
		kill_list = node_kills(node->right,kill_list,not_ind_vars);

	if (node->opt->flags & SIDE_EFFECT) 
		kill_list = object_list_add(node->opt->object,kill_list,not_ind_vars);

	return kill_list;
}

Object_list
object_list_add(obj,kill_list,not_ind_vars)
Object obj;
Object_list kill_list;
Object_set not_ind_vars;
{
	/* Linear search, for now -- PSP */
	int id=obj->setid;
	Object_list x = kill_list, prior = NULL;
	while(x && x->object->setid < id) {
		prior=x;
		x=x->next;
	}

	/* x is null or points to the first item
	   >= the sought item, and if prior != NULL,
	   prior points to the last item < the sought item.
	   If prior == NULL, the item must be inserted
	   in the first position. */

	if(x && x->object->setid == id) {
		if(not_ind_vars) bv_set_bit(id,not_ind_vars);
		DEBUG(aflags[sr].level&4,("Not ind var #%d\n",obj->setid));
		return kill_list;
	}

	x = object_list_new(obj);
	if(prior) {
		x->next=prior->next;
		prior->next=x;
	}
	else { /* new item first */
		x->next = kill_list;
		kill_list = x;
	}
	if(not_ind_vars) {
		bv_set_bit(obj->setid,not_ind_vars);
		DEBUG(aflags[sr].level&4,("Not ind var #%d\n",obj->setid));
	}
	DEBUG(aflags[li].level&4,("Add object #%d\n",obj->setid));
	return kill_list;
}


	/* Set target = target union source, then return target.
	   This code must not modify source */
	/* This stuff all belongs in a set manipulation package. PSP */
Object_list
object_list_union_eq(target, source, destroy)
Object_list target, source;
int destroy;
{
	Object_list prior = NULL, t = target, s = source;
	while (s) {
		Object_list x;
		int sid = s->object->setid;
		while(t && t->object->setid < sid) {
			prior = t;
			t = t->next;
		}
		if(t && t->object->setid == sid) { /* duplicate */
			s = s->next;
			continue;
		}
		if (destroy)
			x = s;
		else
			x = object_list_new(s->object);

		s = s->next;

		if(prior) {
			prior->next = x;
			x->next = t;
			prior = x;
		}
		else { /* No prior so s is first item in union */
			x->next = t;
			target = x;
			prior = x;
		}
	}
	return target;
}

static void walk();

	/* Fill in loc_antic member for this block.
	** An expression is locally anticipated if there
	** is a reference to it in the block with the property
	** that the expression has not been killed earlier in
	** the block.
	*/

static void
block_anticipate(block,kills)
Block block;
Expr_set kills; /* Assume caller allocates this */
{
	Expr_set loc_antic;
	loc_antic = Expr_set_alloc(global_arena);	/* global_arena ?? PSP */
	bv_init(false, kills);
	bv_init(false, loc_antic);
	FOR_ALL_ND1_IN_BLOCK(block,flow,node,index)	
		if(node->left)
			get_recursive_kills(kills, node->left,ACCUMULATE);
		if(node->right)
			get_recursive_kills(kills, node->right,ACCUMULATE);	
		walk(node,kills,loc_antic,0);
	END_FOR_ALL_ND1_IN_BLOCK
	block->loc_antic = loc_antic;
}

#define IS_UNCOND(cntxt) ( !(cntxt & IS_COND) )

static void
walk(node, kills, loc_antic, cntxt)
ND1 *node;
Expr_set kills, loc_antic;
int cntxt; /* two bits of information: IS_LVAL and IS_COND */
{
	if(node->left)
		walk(node->left,kills,loc_antic,get_context(node,LEFT,cntxt));
	if(node->right)
		walk(node->right,kills,loc_antic, get_context(node,RIGHT,cntxt));
	get_kills(kills, node, ACCUMULATE);	
	if(EXCEPT_OPT(node) && IS_RVAL(cntxt) && IS_UNCOND(cntxt) && !bv_belongs(node->opt->setid,kills))
		bv_set_bit(node->opt->setid,loc_antic);
}

void 
set_defs_refs(block, kills, node, context)
Block block;
Expr_set kills;
ND1 *node;
int context; 
{
	switch (node->op){
	ASSIGN_CASES:
		/* lhs to block_def? */
                if(node->left->opt->flags & OBJECT
			&& node->left->opt->object->flags & CAN_BE_IN_REGISTER
			&& !(context&IS_COND))
			bv_set_bit(node->left->opt->object->setid, block->def);

		if (node->op == ASSIGN) /* lhs is lval only */
			set_defs_refs(block,kills,node->left,context|IS_LVAL);
		else	/* lhs is lval and rval */
			set_defs_refs(block, kills, node->left, context);

		set_defs_refs(block, kills, node->right, context);
		break;
	case NAME:
		/* check if NAME is a candidate */
		if ( context != IS_LVAL && 
		     node->opt->object->flags & CAN_BE_IN_REGISTER ) {

			/* If not killed in the basic block (excluding
			   side-effects within statement containing node) then
			   set candidate as use
			*/
			if (!bv_belongs(node->opt->object->setid,kills))
				bv_set_bit(node->opt->object->setid, block->use);

		}
		break;
	default:
		if (optype(node->op) == LTYPE) 
			break;

		set_defs_refs(block, kills, node->left,
			get_context(node,LEFT,context));

		if (optype(node->op) == BITYPE)
			set_defs_refs(block, kills, node->right,
				 get_context(node,RIGHT,context));
	}
}
       
void
get_use_def(arena)
Arena arena;
{
	Object_set kills ;

	kills = Object_set_alloc(arena);

	DEPTH_FIRST(blk_ptr)
	
	Block block = *blk_ptr;

      		block->use = Object_set_alloc(arena);
      		block->def = Object_set_alloc(arena);
      		bv_init(false,block->def);
      		bv_init(false,block->use);

		FOR_ALL_ND1_IN_BLOCK(block, flow, node, index)

				/*
				** Next could prolly be or_eq
				** since we are accumulating the
				** local must_kills
				*/

			bv_assign(kills, block->def);
			set_defs_refs(block, kills, node, 0);

		END_FOR_ALL_ND1_IN_BLOCK

	END_DEPTH_FIRST
}

/*
** Reassign object ids for all objects satisfying cond.
*/
int
remap_object_ids(cond)
int cond;	
{
	struct Object_info *o;
	int object_count = 0;
	int ret;

	if (cond == (~0))  {	/* Everyhing matches */
		for (o= get_object_last(); o; o= o->next) {
			++object_count;
			o->setid = object_count;
		}
		return object_count;
	}

	/* count and set the setids for objects that meet cond1 */
	for (o= get_object_last(); o; o= o->next) {
		if (o->flags & cond) {
			++object_count;
			o->setid = object_count;
		}
	}
	ret = object_count;


	/* Hand out setids for all remaining objects */
	for(o= get_object_last(); o; o= o->next) {
		if((o->flags & cond))
			continue;
		++object_count;
		o->setid = object_count;
	}
	return ret;
}
