/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/loops.c	1.36.2.2"
/* Gather loop info */

#include "amigo.h"
#include "l_unroll.h" /* Temporary PSP */
static Loop last_loop, first_loop, loop_stack, last_loop_entered;
static Block last_block_entered;
static loop_count;
static void loop_body();
static void loop_lcond();
static void loop_end();
static Boolean PROTO(structured_loop_body,(Block header, Block end));
static void PROTO(enter_block_loop_data,(Block, Loop)); 

Loop 
loop_build(blocklist)
Block blocklist;
{
	/* work through every flownode in every block */
	Block block;
      	Loop loop;
      	TEMP_SCOPE_ID outer_scope;
		/*
		** Keep track of the location of the last os_loop_start.
		** Loop unrolling needs this to remove the loop info
		*/
	static Cgq_index remember_lstart_index = CGQ_NULL_INDEX;
	static Block remember_lstart_block = 0;
	loop_count = 0;
	loop_stack = last_loop = first_loop = last_loop_entered = (Loop)0;
	last_block_entered = (Block)0;
	for (block = blocklist; block; block = block->next) {

			/* 
			   The orderings in the following switch reflect
			   correct (loop) ownership of the block in which the
			   loop delimiters appear.
			*/

		if(block->block_flags & BL_HAS_OS_LOOP) {

		CGQ_FOR_ALL_BETWEEN(flow, index, block->first, block->last)

		if(flow->cgq_op == CGQ_CALL_INT && flow->cgq_func == os_loop)
			switch(flow->cgq_arg.cgq_int) {
			case OI_LSTART:
				remember_lstart_index = index;
				remember_lstart_block = block;
				enter_block_loop_data(block,loop_stack);
				break;
			case OI_LEND:	/* This block belongs to
					** containing loop, if any.
					*/
				loop_end(index,loop_stack, block);		
				enter_block_loop_data(block,loop_stack);
				break;
			case OI_LBODY:
				enter_block_loop_data(block,loop_stack);

				loop_body(remember_lstart_block,
					remember_lstart_index, index,
					block);
				remember_lstart_index = CGQ_NULL_INDEX;
				remember_lstart_block = 0;
				break;
			case OI_LCOND:
				enter_block_loop_data(block,loop_stack);
				loop_lcond(index, loop_stack, block);
				break;
			}
		} CGQ_END_FOR_ALL_BETWEEN
		else
			enter_block_loop_data(block,loop_stack);
	}

	/* top down through loops */
	for (loop= last_loop; loop; loop = loop->prev) {
		if (!loop->enclose) {
			Cgq_index first_nd1 = loop->header->first;
			Block block;
			/* first_nd1 is first ND1 in block or first item of
			   any variety in block
			*/
			FOR_ALL_ND1_IN_BLOCK(loop->header, flow, node, index)
				first_nd1 = index;
				break;
			END_FOR_ALL_ND1_IN_BLOCK
			loop->scope = al_create_scope(first_nd1,
			    CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(loop->end->first)));
			outer_scope = loop->scope;
		}
		else
			loop->scope = outer_scope;
	}

	if(loop_stack)
		Amigo_fatal("loop_build(): unbalanced loop start/stop");
	DEBUG_COND(aflags[l].level,loop_debug());
	return first_loop;
}

	/* Compute expr_kills for each loop, using object_kills */
void
loop_expr_kills(first_loop)
Loop first_loop;
{
	Loop loop;
	for(loop = first_loop; loop; loop = loop->next) {
		Object_list objects;
		bv_init(false,loop->expr_kills);
		for(objects = loop->kills; objects; objects = objects->next) {
			DEBUG(aflags[l].level>=3,("adding to loop expr_kills: "));
			DEBUG_COND(aflags[l].level>=3,bv_print(objects->object->kills));
			bv_or_eq(loop->expr_kills,objects->object->kills);
		}
#ifndef NODBG
		if (aflags[l].level) {
			DPRINTF("expr_kills for loop %d:", loop->id);
			bv_print(loop->expr_kills);
		}
#endif
	}
}

static void 
enter_block_loop_data(block, loop)
Block block;
Loop loop;
{
	
	if(loop && (last_block_entered != block || last_loop_entered != loop) ) {
		
		Block_list bl = Arena_alloc(GLOBAL,1,struct Block_list);
		loop->kills = object_list_union_eq(loop->kills,block->kills,0);
		block->loop = loop;
		bl->block = block;
		bl->next = loop->blocks;
		loop->blocks = bl;
		last_block_entered = block;
		last_loop_entered = loop;
	}
}

	/* Push a new (incomplete) loop record on the stack */
static void
loop_body(loop_start_block, loop_start_index, loop_body_index, block)
Block loop_start_block;
Cgq_index loop_start_index, loop_body_index;
Block block;
{
	Loop loop = Arena_alloc(GLOBAL,1,struct Loop_info);
	loop->expr_kills = Expr_set_alloc(GLOBAL);
	loop->kills = 0;
	loop->invariants = Expr_set_alloc(GLOBAL);

	bv_init(false,loop->invariants);
	loop->ind_vars = Expr_set_alloc(GLOBAL);
	loop->blocks = 0;
	loop->nest = 0;
	loop->enclose = 0;
	loop->sibling = 0;
	loop->header = block;
		/* push the temporary stack */
	loop->next = loop_stack;
	loop_stack = loop;
	loop->header_pred_count = 0;

		/* Init loop_unroll info */
	loop->loop_unroll = Arena_alloc(GLOBAL,1,struct Loop_unroll_info);
	(void)memset((char *)(loop->loop_unroll),0, sizeof(struct Loop_unroll_info));
	loop->loop_unroll->loop_cbranch_index = CGQ_NULL_INDEX;
	loop->loop_unroll->increment_index = CGQ_NULL_INDEX;

/*
	assert(loop_start_index != CGQ_NULL_INDEX);
*/

/*
	assert(loop_start_block);
*/
	loop->loop_unroll->loop_start = loop_start_index;
	loop->loop_unroll->loop_start_block = loop_start_block;
	loop->loop_unroll->loop_body_block = block;
	loop->loop_unroll->loop_body = loop_body_index;
}

static void
add_spill(first, first_block, last, last_block)
Cgq_index first, last;
Block first_block, last_block;
{
	static spill_num;
	cgq_t * item;
	item = amigo_insert(first_block, first);
	item->cgq_op = CGQ_START_SPILL;
	item->cgq_arg.cgq_int = ++spill_num;

	item = amigo_insert(last_block, last);
	item->cgq_op = CGQ_END_SPILL;
	item->cgq_arg.cgq_int = spill_num;
}

static Boolean
find_loop_test(loop)
Loop loop;
{
		/*
		** Look for the loop test between the lcond directive
		** and the lend directive.  Loop unrolling is only interested
		** in loops whose test is a CBRANCH.  If we find neither
		** a CBRANCH nor a JUMP, something is very fishy
		** and this thing is probably not a loop, so we
		** get ready to throw it out.
		*/
	Block block, lcond_block, lend_block;
	
	lend_block = loop->loop_unroll->loop_end_block;
	block = lcond_block = loop->loop_unroll->loop_cond_block;

	while(block) {
		Cgq_index start_index, end_index;

		start_index = block->first;
		end_index = block->last;

		if(block == lcond_block) {
			start_index = loop->loop_unroll->loop_cond;
		}

		if(block == lend_block)
			end_index = loop->loop_unroll->loop_end;
			
		CGQ_FOR_ALL_BETWEEN(flow, index, start_index, end_index)
			if( flow->cgq_op == CGQ_EXPR_ND1 && 
				flow->cgq_arg.cgq_nd1->op == CBRANCH ) 
			{
				assert(flow->cgq_arg.cgq_nd1->right->op == ICON);
				loop->loop_unroll->label =
					flow->cgq_arg.cgq_nd1->right->lval;
				loop->loop_unroll->loop_cbranch_index = index;
				loop->loop_unroll->loop_test_block = block;
				return true;
			}
			else if( flow->cgq_op == CGQ_EXPR_ND2 && 
				flow->cgq_arg.cgq_nd2->op == JUMP ) {
				loop->loop_unroll->loop_test_block = block;
				return true;
			}
			else if( HAS_ND1(flow) )
				return false;
		CGQ_END_FOR_ALL_BETWEEN	

		if(block == lend_block)
			block = 0;
		else
			block = block->next;
	}
	return false;
}

		/* Pop loop off temporary stack and equeue on
		** depth first list of loops.
		*/
static void
loop_end(loop_end_index,loop, block)
Cgq_index loop_end_index;
Loop loop; /* loop to be popped */
Block block;
{
	extern int do_inline;

	Loop enclose_loop;

	if(! loop)
		Amigo_fatal("loop_end(): unexpected empty stack");

	enclose_loop = loop->next;

	if(enclose_loop) {
		enclose_loop->kills =
			object_list_union_eq(enclose_loop->kills, loop->kills,0);
	}	

	if(find_loop_test(loop) &&
		structured_loop_body(loop->header,block)) {

		loop->prev = last_loop;
		loop->end = block;
		if (last_loop) {
			last_loop->next = loop;
			last_loop = loop;
		}	
		else {
			first_loop = last_loop = loop;
		}

		loop->next = 0;
		loop->enclose = enclose_loop;
		loop->id = ++loop_count;

		if(enclose_loop) {
			loop->sibling = enclose_loop->nest;
			enclose_loop->nest = loop;
		}

		loop->loop_unroll->loop_end = loop_end_index;
		loop->loop_unroll->loop_end_block = block;

#ifndef NODBG
		if (aflags[ra].xflag)
#else
		if (do_inline)
#endif
			add_spill(loop->loop_unroll->loop_body,
				loop->loop_unroll->loop_body_block,
				loop_end_index, block);
	}

	else {
		Loop child;

			/*
			** Don't save this "loop", but give its blocks
			** to enclosing loop.
			** Also give nested loops (children) to enclosing loop.
			*/

		
		child = loop->nest;

		if(child) for(;;) {
			child->enclose = enclose_loop;
			if(child->sibling)
				child = child->sibling;
			else break; /* child is last child */
		}

		if(enclose_loop) {
			Block_list bl = loop->blocks;

				/* 
				** Find the last block on the chain.
				** Change each block on the chain to
				** show it belongs to enclose_loop,
				** not the loop we are throwing away.	
				** Link the chain into the enclosing loop's
				** chain.
				*/
			if(bl) { 
				for(;;) {
					bl->block->loop = enclose_loop;
					if(bl->next) bl=bl->next;
					else break;
				}
				bl->next = enclose_loop->blocks;
				enclose_loop->blocks = loop->blocks;
			}

			if(child) { /* give enclosed loops to enclosing loop */
				child->sibling = enclose_loop->nest;
				enclose_loop->nest = loop->nest;
			}
		}
	}

	loop_stack = enclose_loop;
}

	/* ensure there are no branches into loop from outside
	   loop */

static Boolean
structured_loop_body(header,end)
Block header, end;
{
	Block bl;
	int lo_block = header->block_count, hi_block = end->block_count;
		/*
		** We probably could return true if the next check
		** succeeds, but the loop is definitely fishy and
		** contains no interesting code
		** so safer to flag it as bad. -- PSP ??
		*/
	if(lo_block == hi_block)
		return false;
		/* check all blocks up to, but not including, end block */
	for(bl=header->next; bl != end; bl = bl->next) {
		Block_list pred_list;
		for(pred_list = bl->pred; pred_list;
			pred_list = pred_list->next) {
	
			int this_block = pred_list->block->block_count;
			if(this_block >= hi_block || this_block < lo_block ) {
				DEBUG(aflags[l].level,("Block %d has block %d as \
pred, expected range [%d,%d)\n",
				bl->block_count,this_block,lo_block,hi_block));
				return false;
			}
		}
	}
	return true;
}

static void
loop_lcond(lcond_index,loop, block)
Cgq_index lcond_index;
Loop loop; 
Block block;
{
	if(! loop)
		Amigo_fatal("loop_lcond(): unexpected empty stack");

	DEBUG(aflags[lu].level&2,("Check index between %d and %d\n",
		CGQ_INDEX_NUM(lcond_index),
		CGQ_INDEX_NUM(block->last)));
	loop->loop_unroll->loop_cond = lcond_index;
	loop->loop_unroll->loop_cond_block = block;
}

static ind_var_defs_size;
Def_descr *ind_var_defs;

	/*
	** Compute loop induction variables from block induction variables:
	** i is an induction variable for loop l if it is an induction
	** variable in one of l's top level blocks, and is not killed
	** in any block belonging to l or any loop nested in l.
	*/
void
find_ind_vars(first_loop)
Loop first_loop;
{
	Loop loop;
	for(loop = first_loop; loop; loop = loop->next) {
		Boolean has_call, has_pointer_kill;
		struct Block_list *blist;

			/* 
			   Make two passes through the blocks, first
			   to find all potential induction variables,
			   second to remove from this list every variable
			   which is killed in some block but is not an
			   induction variable in that block.
			*/

		DEBUG_COND(aflags[sr].level&2,loop_debug());
		bv_init(false,loop->ind_vars);
		for(blist = loop->blocks; blist; blist = blist->next) {
			DEBUG(aflags[sr].level&4,
				("Adding block %d ind vars: ",
				blist->block->block_count));
			DEBUG_COND(aflags[sr].level&4,
				pr_object_set(blist->block->ind_vars));
			DEBUG(aflags[sr].level&4,("\n"));
			if(blist->block->ind_vars)
				bv_or_eq(loop->ind_vars,blist->block->ind_vars);
		}

		has_call = has_pointer_kill = 0;
		for(blist = loop->blocks; blist; blist = blist->next) {
			Object_list olist;
			Block thisblock = blist->block;
			for(olist = thisblock->kills; olist; olist = olist->next) {
				Object this;
				this = olist->object;
				
				if(! bv_belongs(this->setid,thisblock->ind_vars)) {
					bv_clear_bit(this->setid,loop->ind_vars); 
				}

				if(!has_call && is_generic_call(this))
					has_call = 1;

				if(!has_pointer_kill && is_generic_deref(this))
					has_pointer_kill = 1;
			}
		}

		if(has_call)
			bv_minus_eq(loop->ind_vars, generic_call_obj_kills);

		if(has_pointer_kill)
			bv_minus_eq(loop->ind_vars, generic_deref_obj_kills);

			/*
			   Finally, remove all variables killed in a
			   nested loop ( because updates will be too
			   expensive ).
			*/
		{
			Loop l;
			for(l = loop->nest; l; l = l->sibling) {
				Object_list olist;
				for(olist = l->kills; olist; olist = olist->next) {
					DEBUG(aflags[sr].level&4,("Clearing bit %d\n",olist->object->setid));
					bv_clear_bit(olist->object->setid,loop->ind_vars);
				}
			}
		}

		DEBUG(aflags[sr].level&2,("Loop %d ind_vars:\n",loop->id));
		DEBUG_COND(aflags[sr].level&2, pr_object_set(loop->ind_vars));
		ind_var_defs_size = 2*get_object_count() + 1;
		ind_var_defs = Arena_alloc(GLOBAL, ind_var_defs_size, Def_descr);
	}
}

	/* Identify all assignments to induction variables */
void
find_ind_var_defs(loop)
Loop loop;
{
	long sign = 1;

	(void)memset((char *)ind_var_defs,0,sizeof(Def_descr)*ind_var_defs_size);

	FOR_ALL_TREES_IN_LOOP(loop,node,bl,flow,index)

	Boolean not_ind_var = false;
#define ISICON(node) ((node)->op == ICON && (node)->rval == ND_NOSYMBOL)
		/*
		** We assume that acomp has converted assignments
		** of the form x = x + const to their += equivalent.
		*/
	switch(node->op) {
	case ASG MINUS:
	case DECR: 
			sign = -1;
			/* FALLTHROUGH */
	case INCR:
	case ASG PLUS:
		if(!ISICON(node->right)) /* check needed ?? -- PSP */
				not_ind_var = true; /* looks like, but isn't */
		{
		struct Object_info * obj = node->left->opt->object;
		if(!obj) break;
		if(bv_belongs(obj->setid,loop->ind_vars)) {
			if(ind_var_defs[obj->setid].def||not_ind_var) {
				/* If we already have an assignment
				   for this variable, throw it away
				   as the updates will be too expensive.
				*/ 
				bv_clear_bit(obj->setid,loop->ind_vars);
				ind_var_defs[obj->setid].def = 0;
				loop->loop_unroll->increment_index = CGQ_NULL_INDEX;
			}
			else {
				ind_var_defs[obj->setid].def = node;
				ind_var_defs[obj->setid].block = bl->block;

				/*  For loop unrolling, we are only interested
				**  in induction variables that are updated
				**  in the same as the CBRANCH because the 
				**  ind_var must be unconditionally updated.
				*/

				if ( bl->block == loop->loop_unroll->loop_test_block){
					loop->loop_unroll->increment_index = index;
					loop->loop_unroll->increment = sign * node->right->lval;
				}
			}
		}
		}
		break;
	default: break;
	}
			
	END_FOR_ALL_TREES_IN_LOOP
}

#ifndef NODBG
	/* Print the loop info in inner/outer order */
void
loop_debug()
{
	Loop p ;
	for(p=first_loop; p; p = p->next) {
		Loop loop=p;
		DPRINTF(
			"loop: id %d, header %d, end %d, \
scope.first %d, scope.last %d, enclosed by %d,",
			p->id,p->header->block_count,p->end->block_count,
			p->scope.first, p->scope.last,
			(p->enclose?p->enclose->id:(int)(p->enclose)));
		DPRINTF(" enclosing");
		loop = p->nest;
		if(loop) {
			do {
				DPRINTF(" %d",loop->id);
				loop = loop->sibling;
			} while(loop);
			DPRINTF("\n");
		}
		else
			DPRINTF(" 0\n");
		DPRINTF("loop object kills:");	
		pr_object_list(p->kills);
		if(aflags[bb].level) {
			Block_list bl = p->blocks;
			DPRINTF("loop blocks:");
			for(; bl; bl = bl->next) 
				DPRINTF(" %d",bl->block->block_count);
			DPRINTF( "\n");	
		}
		if(aflags[lu].level) {
			assert(p->loop_unroll);
			DPRINTF("loop_start: %d loop_body: %d loop_cond: %d loop_end %d\n",
				CGQ_INDEX_NUM(p->loop_unroll->loop_start),
				CGQ_INDEX_NUM(p->loop_unroll->loop_body),
				CGQ_INDEX_NUM(p->loop_unroll->loop_cond),
				CGQ_INDEX_NUM(p->loop_unroll->loop_end));
			DPRINTF("loop_start_block: %d loop_body_block %d loop_cond_block: %d loop_end_block %d\n",
				p->loop_unroll->loop_start_block->block_count,
				p->loop_unroll->loop_body_block->block_count,
				p->loop_unroll->loop_cond_block->block_count,
				p->loop_unroll->loop_end_block->block_count);
		}
	}
}

void
debug_ind_var_defs(loop,ind_var_defs)
Loop loop;
Def_descr ind_var_defs[];
{
	int i;
	fprintf(stderr,"loop %d ind vars: ",loop->id);
	pr_object_set(loop->ind_vars);
	for(i = 1; i < ind_var_defs_size; i++) {
		if(ind_var_defs[i].def) {
			fprintf(stderr,"\tind_var_defs[%d]:",i);
			if(aflags[sr].level&2){
				fprintf(stderr,"\n");
				tr_e1print(ind_var_defs[i].def,"T");
			}
			else
				fprintf(stderr," node(%d)\n", node_no(ind_var_defs[i].def));
		}
	}
}
#endif
