/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/l_unroll.c	1.16.2.1"
#include "amigo.h"
#include "l_unroll.h"
#include <memory.h>
#include "costing.h"

#define MIN_ITERATIONS 0

#define IS_INVARIANT(node,loop)  (OPTABLE(node) && \
        !(bv_belongs(node->opt->setid, loop->expr_kills)))
  
#define IS_INDVAR(node,loop)  (OPTABLE(node)  && node->opt->object && \
	bv_belongs(node->opt->object->setid, loop->ind_vars))
  
#define IS_ICON(node) ((node)->op == ICON && (node)->rval == ND_NOSYMBOL)

#define COUNT(X,Y,Z) ( (Z)<=0 ? 0 : ((X) > (Y) ? 0 : ((Y)-(X))/(Z) + 1) )

#ifndef NODBG
#ifdef __STDC__
#define ASSERT(cond)  if( !(cond) ) {/* Something is wrong, so GIVE UP */\
        DEBUG(aflags[lu].level&1,("\tWarning: optimizer gave up unrolling loop(%d).\n", loop->id)); \
        return; \
}
#else
#define ASSERT(cond)  if( !(cond) ) {/* Something is wrong, so GIVE UP */ \
        DEBUG(aflags[lu].level&1,("\tWarning: optimizer gave up unrolling loop(%d)\n", loop->id));\
}
#endif
#else
#define ASSERT(cond) if( !(cond) ) return;
#endif

typedef enum { const_limits, invar_limits, not_countable, not_cost_effective } Loop_type;
typedef enum { NOT_LE, NOT_LT, NOT_GE, NOT_GT, NOT_NE, NOT_INDVAR, UNKNOWN } Branch_type;

#ifndef NODBG
static char *pattern_st[] = {
	"NOT_LE","NOT_LT", "NOT_GE", "NOT_GT", "NOT_NE", "NOT_INDVAR", "UNKNOWN"
};
#endif

static Branch_type branch_op_type;
static int tree_count;

static Loop_type PROTO(get_test_type,(Loop));
static Loop_type PROTO(rel_op_test_type,(int, ND1 *, Loop));
static Loop_type PROTO(get_initial_type,(Loop));
static Loop_type PROTO(code_expansion,(Loop));
static long  PROTO(iterations,(Loop));
static int PROTO(cost_trees,(Loop));
static Arena loop_unrolling_arena;

	/* Move this decl into amigo.h */
extern Block_list init_block_list();

static int PROTO(find_ind_init,(ND1 *, Loop, Loop_type *));
static Loop_type PROTO(loop_type,(Loop));
static void PROTO(loop_edit,(Loop));
static void PROTO(copy_blocks, (long, Loop, Boolean first_copy, Boolean last_copy));
static void PROTO(block_map_alloc, (Block, Block));
static void PROTO(block_map_init, (Block, Block));
static void PROTO(map_tables_init, (Boolean, Block, Block));
	/* Put in util.c */
ND1 * PROTO(t1_copy,(Boolean *, Loop, long, ND1 *));
static ND2 * PROTO(t2_copy,(ND2 *));
static int PROTO(map_label,(int));
static Boolean PROTO(remove_loop_structure,(Loop));
static void PROTO(insert_counter_update, (long, Loop));
static void PROTO(remove_edge, (Block, Block));
static void PROTO(map_predecessors, (Block, Block_list));
static void PROTO(map_successors, (Block, Block_list));
static void PROTO(remove_back_links, (Block));
static Boolean PROTO(is_single_exit, (Loop));

static Boolean PROTO(fix_counter_update,(Cgq_index, Loop_unroll_info));

static void PROTO(insert_symbol, (SX));
static SX PROTO(map_symbol, (SX));

#define NEWLAB() getlab()

static min_label, max_label;

typedef struct symbol_map_descriptor {
	SX symbol;
	SX map_symbol;
	struct symbol_map_descriptor *next;	
} *Symbol_map;

static Symbol_map symbol_map;

#ifndef NODBG
static void PROTO(pr_symbol_map, (Symbol_map));
#endif

static Block last_block_copied;

void
loop_unrolling(first_loop)
Loop first_loop;
{
	Loop loop;

	loop_unrolling_arena = arena_init();

	label_table_alloc(loop_unrolling_arena);

	for(loop = first_loop; loop; loop = loop->next) {

		if(loop->nest) continue;

		switch(loop_type(loop)) {
		case const_limits:
START_OPT
			DEBUG(aflags[lu].level&2, ("Unrolling fixed loop(%d) %d times\n",
				loop->id, ITERATIONS(loop)));

			if(! fix_counter_update(loop->loop_unroll->increment_index,loop->loop_unroll))
				break;

			loop_edit(loop);
STOP_OPT
			break;
		case invar_limits:;
			DEBUG(aflags[lu].level&2, ("Loop(%d) has invariant limits\n",
				loop->id));
			break;
		default: /* Not unrollable for whatever reason */
			DEBUG(aflags[lu].level&2, ("Loop(%d) not unrollable\n",
				loop->id));
		}
	}

	arena_term(loop_unrolling_arena);
}

static void
loop_edit(loop)
Loop loop;
{
	Block start_block , stop_block;
	int i;
	long counter_val;

	last_block_copied = loop->header;

		/*
		** We have to save the beginning index of the source.
		** See the comment in copy_blocks.
		*/

	start_block = loop->loop_unroll->body_begin = loop->header->next;
	stop_block = loop->loop_unroll->body_end = loop->end;

	block_map_alloc(start_block, stop_block);

	if(! is_single_exit(loop) ) {
		DEBUG(aflags[lu].level&1,(
			"loop_edit(%d) failed, loop not single_exit\n",loop->id));
		return;
	}

	if(! remove_loop_structure(loop)) {
		DEBUG(aflags[lu].level&1,(
			"loop_edit(%d) failed, could not remove loop_structure\n",loop->id));
		return;
	}

	DEBUG(aflags[lu].level, ("Unrolling fixed loop(%d) %d times\n",
				loop->id, ITERATIONS(loop)));
	for(counter_val = BEGIN_LIMIT(loop), i = 1;
		i <= ITERATIONS(loop);
		counter_val += INCREMENT(loop), i++) {
		DEBUG(aflags[lu].level&16,("copy_blocks(iteration: %d, \
 counter_val %d)\n",i,counter_val));
		copy_blocks(counter_val, loop, i == 1, i == ITERATIONS(loop));
	}

	insert_counter_update(counter_val, loop);

	cg_q_cut(start_block->first, loop->loop_unroll->loop_test_block->last);
}

static void PROTO(delete_branch,(Loop));
static Boolean PROTO(delete_counter_update,(Cgq_index, Expr));
static void PROTO(delete_os_loop_calls,(Loop_unroll_info));

static Boolean
remove_loop_structure(loop)
Loop loop;
{
	Cgq_index label_index;

	label_index = CGQ_NULL_INDEX;
		/*
		** First check that the first interesting tree
		** in the loop body is what we think is the top
		** of loop label.
		*/
	CGQ_FOR_ALL_BETWEEN(elem, index,
		loop->header->next->first, loop->header->next->last)

		if(HAS_EXECUTABLE(elem)) {
			if(elem->cgq_op == CGQ_EXPR_ND2
				&& elem->cgq_arg.cgq_nd2->op == LABELOP
				&& elem->cgq_arg.cgq_nd2->label
					== loop->loop_unroll->label)
				label_index = index;
			CGQ_FOR_ALL_BETWEEN_BREAK;
		}

		CGQ_END_FOR_ALL

	if(label_index == CGQ_NULL_INDEX)
		return false;
	cg_q_delete(label_index);

	cg_q_delete(loop->loop_unroll->increment_index);

	delete_os_loop_calls(loop->loop_unroll);
	delete_branch(loop);
	return true;
}

static void
delete_os_loop_calls(l_u)
Loop_unroll_info l_u;
{
	DEBUG(aflags[lu].level&8,("Deleting flow(%d) from block(%d)\n",
			CGQ_INDEX_NUM(l_u->loop_start),
			l_u->loop_start_block->block_count));

	amigo_delete(l_u->loop_start_block, l_u->loop_start);

	DEBUG(aflags[lu].level&8,("Deleting flow(%d) from block(%d)\n",
			CGQ_INDEX_NUM(l_u->loop_body),
			l_u->loop_body_block->block_count));

	amigo_delete(l_u->loop_body_block, l_u->loop_body);

	DEBUG(aflags[lu].level&8,("Deleting flow(%d) from block(%d)\n",
			CGQ_INDEX_NUM(l_u->loop_cond),
			l_u->loop_cond_block->block_count));

	amigo_delete(l_u->loop_cond_block, l_u->loop_cond);

	DEBUG(aflags[lu].level&8,("Deleting flow(%d) from block(%d)\n",
			CGQ_INDEX_NUM(l_u->loop_end),
			l_u->loop_end_block->block_count));

	amigo_delete(l_u->loop_end_block, l_u->loop_end);
}

	/*
	** Define maps for  all labels, autos and parms declared in the body.
	** Because the number of items to be mapped is small, we could
	** probably just use linked lists of map pairs.  But the
	** code here uses an existing array layout ( from blocks.c )
	** for labels and a linked list of pairs to map symbols.
	*/
static void
map_tables_init(first_call, start_block, stop_block)
Boolean first_call;
Block start_block, stop_block;
{
	Block block;
	int i;

		/*
		** Initially set limits for label search.  If we
		** see any labels, we will get min_label <= max_label.
		*/

	if(first_call) {
		min_label = get_max_label() + 1;
		max_label = get_min_label();

		label_table_zero();
		symbol_map = 0;

		for(block = start_block; block != stop_block; block = block->next) {

		CGQ_FOR_ALL_BETWEEN(elem, index, block->first, block->last)
				/* Check for label, auto, parm or amigo temp */
			if(elem->cgq_op == CGQ_EXPR_ND2
				&& elem->cgq_arg.cgq_nd2->op == LABELOP) {

				int label = elem->cgq_arg.cgq_nd2->label;
				struct Label_descriptor *l_d;
				l_d = get_label_descriptor(label);
				l_d->map_label = label;
					/* Next needed ?? */
				l_d->block = block;
				if(label < min_label)
					min_label = label;
				if(label > max_label)
					max_label = label;
			}
			else if(elem->cgq_op == CGQ_CALL_SID &&
				elem->cgq_func == db_symbol || 
				elem->cgq_op == CGQ_START_SCOPE) {
				insert_symbol(elem->cgq_arg.cgq_sid);
			}
		CGQ_END_FOR_ALL_BETWEEN

		}	
	}

	DEBUG_COND(aflags[lu].level&64,
		pr_label_table(min_label, max_label));

	for(i = min_label; i <= max_label; i++) {
		struct Label_descriptor *l_d;
		l_d = get_label_descriptor(i);
		if(l_d->map_label)
			l_d->map_label = NEWLAB(); 
	}

	{
	Symbol_map sy_m;
	for(sy_m = symbol_map; sy_m; sy_m = sy_m->next) {
		sy_m->map_symbol = sy_temp(SY_TYPE(sy_m->symbol));
		SY_NAME(sy_m->map_symbol) = SY_NAME(sy_m->symbol);
		SY_FLAGS(sy_m->map_symbol) = SY_FLAGS(sy_m->symbol);
	}
	}

	DEBUG_COND(aflags[lu].level&4,
		pr_label_table(min_label, max_label));
/*
	DEBUG_COND(aflags[lu].level&4,
		pr_symbol_map(symbol_map));
*/
}

static Symbol_map
NEW_S_T_ENTRY()
{
	return Arena_alloc(loop_unrolling_arena, 1, struct symbol_map_descriptor);
}

	/*
	** Insert this symbol into the list, keeping in INCREASING
	** order by symbol table index.
	*/
static void
insert_symbol(sid)
SX sid;
{
	Symbol_map s_t, last = symbol_map;
	DEBUG(aflags[lu].level&16,("Symbol table before insert sid(%d)\n",
			sid));
	DEBUG_COND(aflags[lu].level&16,pr_symbol_map(symbol_map));
	for(s_t = symbol_map; s_t; last = s_t, s_t = s_t->next) {
		if(sid < s_t->symbol)
			break;
		assert(sid != s_t->symbol);
	}	

	if(s_t == last) {
		assert(s_t == symbol_map);
		symbol_map = NEW_S_T_ENTRY();
		symbol_map->symbol = sid;
		symbol_map->next = last;
	}
	else {
		Symbol_map new = NEW_S_T_ENTRY();	
		new->symbol = sid;
		new->next = last->next;
		last->next = new;
	}
	DEBUG(aflags[lu].level&16,("Symbol table after insert sid(%d)\n",
			sid));
	DEBUG_COND(aflags[lu].level&16,pr_symbol_map(symbol_map));
}

static SX
map_symbol(sid)
SX sid;
{
	Symbol_map s_m;

	for(s_m = symbol_map; s_m && s_m->symbol < sid; s_m = s_m->next)
		;
	if(s_m && s_m->symbol == sid) 
		return s_m->map_symbol;
	else
		return sid;
}


typedef struct block_map_elem {
	Block old, new; /* may not need old */
} Block_map_elem;

static int block_map_size;
static Block_map_elem *block_map;
static lo_block, hi_block;	/*
				** array bounds
				*/

	/*
	** Build mapping of all blocks in the loop body.
	*/
static void
block_map_alloc(start_block, stop_block)
Block start_block, stop_block;
{
	int i;
	Block block;
	lo_block = start_block->block_count; 
	hi_block = stop_block->block_count - 1; 
	block_map_size = hi_block - lo_block + 1;
	assert(block_map_size > 0 );
	block_map = Arena_alloc(loop_unrolling_arena, block_map_size, Block_map_elem);
	(void)memset((char *)block_map,0,sizeof(Block_map_elem)*block_map_size);
	for(block = start_block; block != stop_block;
		block = block->next) {
		i = block->block_count - lo_block;
		assert( 0 <= i && i < hi_block );
		block_map[i].old = block;
		block_map[i].new = 0;
	}
}

static void
block_map_init(start_block, stop_block)
Block start_block, stop_block;
{
	int i;
	Block block;
	for(block = start_block; block != stop_block; 
		block = block->next) {
		i = block->block_count - lo_block;
		assert( 0 <= i && i < hi_block );
		block_map[i].new = new_block_alloc();
			/*
			** Next may not be quite right, e.g., it will
			** set BL_HAS_OS_LOOP incorectly.
			*/
		block_map[i].new->block_flags = block->block_flags;
		DEBUG(aflags[lu].level&8,("Mapping block %d-->%d, flags:%d\n",
			i + lo_block, block_map[i].new->block_count,
			block_map[i].new->block_flags));
	}
}

#ifndef NODBG
static int
debug_block_map()
{
	int i;

	fprintf(stderr,"\t+=+=+=+=+=+=+%s+=+=+=+=+=+=+\n",
		"block map (index old new new_pred new_succ)");

	for(i=0; i < block_map_size; i++) {
		fprintf(stderr,"\t%d\t%d\t%d\t", i,
			block_map[i].old ? block_map[i].old->block_count : -1,
			block_map[i].new ? block_map[i].new->block_count : -1);
		
		if(block_map[i].new) {
			pr_block_list((block_map[i].new)->pred, "");
			pr_block_list((block_map[i].new)->succ, "\t");
		}
		fprintf(stderr,"\n");
	}
}
#endif

static Block
map_block(block)
Block block;
{
	int block_num = block->block_count;
	if(block_num < lo_block || block_num > hi_block)
		return block;
	else
		return(block_map[block_num - lo_block].new);
}

	/* 
	** is_single_exit is called when the block map has first been
	** initialized.  At that time, all blocks in the loop body
	** are mapped to 0, all other blocks map to themselves ( as
	** usual ).
	** We check that every block in the body, except for the last
	** ( loop test ) block, has all its successors in the body.
	*/
static Boolean
is_single_exit(loop)
Loop loop;
{
	Block block;
	for(block = loop->loop_unroll->body_begin;
		block != loop->loop_unroll->loop_test_block; block = block->next) {
		Block_list bl;
		for(bl = block->succ; bl; bl = bl->next) {
			if(bl->block == map_block(bl->block)) {
				DEBUG(aflags[lu].level&8,
				("Block %d has successor %d not in loop\n",
					block->block_count,
					bl->block->block_count));
				return false;
			}
		}	
	}
	return true;
}
	/*
	** Remove this block from predecessor list of every
	** successsor which is not in the body ( not mapped ).
	*/
static void
remove_back_links(block)
Block block;
{
	Block_list succ_list;
	for(succ_list = block->succ; succ_list; succ_list = succ_list->next) {
		Block mapped_block;
		mapped_block = map_block(succ_list->block);
		if(mapped_block == succ_list->block) {
			Block_list bl, remember_bl = 0;
			for(bl = mapped_block->pred; bl && bl->block != block;
				bl = bl->next)
				remember_bl = bl;
			assert(bl);

			if(remember_bl)
				remember_bl->next = bl->next;
			else
				mapped_block->pred = bl->next;
		}
	}
}
	/*
	** Copy the blocks of the edited loop body.
	** Replace every instance of the loop counter by its current
	** value.
	** Update the block info for the new blocks.
	*/
static void
copy_blocks(counter_val, loop, first_copy, last_copy)
long counter_val;
Loop loop;
Boolean first_copy, last_copy;
{
	Cgq_index insert_index;
	Block block, remember_last_block, mapped_start_block;
	Block start_block, stop_block;

		/*
		** We can't use loop->header->next to find the first
		** block to be copied, because once the first copy
		** has been made, this link points to the COPY,
		** not the source.
		*/
	start_block = loop->loop_unroll->body_begin;
	stop_block = loop->loop_unroll->body_end;

	assert(start_block == loop->loop_unroll->body_begin);
	assert(stop_block == loop->loop_unroll->body_end);
	block_map_init(start_block, stop_block);
	map_tables_init(first_copy, start_block, stop_block);

	if(first_copy) {
		Block block;

		for(block = start_block; block != stop_block;
			block = block->next) {
			remove_back_links(block);
		}
	}

	DEBUG(aflags[lu].level&4,("\n\tlast_block_copied(%d)->succ ",
		last_block_copied->block_count));
	DEBUG_COND(aflags[lu].level&4,pr_block_list(last_block_copied->succ,
		"before: "));
	DEBUG(aflags[lu].level&4,("\n"));
		/*
		** Make the successor of the last block copied, i.e.,
		** the block preceeding the current copy of the body,
		** the first block of this copy.
		*/
	mapped_start_block = map_block(start_block);
	last_block_copied->succ =
			init_block_list(mapped_start_block);
	mapped_start_block->pred =
			init_block_list(last_block_copied);
	assert(last_block_copied->succ->block->next == 0);
	DEBUG_COND(aflags[lu].level&2, debug_block_map());
	DEBUG(aflags[lu].level&4,("\n\tlast_block_copied(%d)->succ ",
		last_block_copied->block_count));
	DEBUG_COND(aflags[lu].level&4,pr_block_list(last_block_copied->succ,
		"after: "));
	DEBUG(aflags[lu].level&4,("\n"));
	insert_index = last_block_copied->last;

	for(block = start_block; block != stop_block;
		block = block->next) {
		Block new_block;

		remember_last_block = block;

		new_block = map_block(block);
		new_block->next = map_block(block->next);

		DEBUG(aflags[lu].level&8,
			("Setting block(%d)->next(prev) to (new)block %d(%d)\n",
			new_block->block_count,
			map_block(block->next)->block_count
			));
		assert(loop->loop_unroll->loop_cbranch_index != block->first);

		
		CGQ_FOR_ALL_BETWEEN(elem, index, block->first, block->last)

		cgq_t *cgq_item;
			
		cgq_item = cg_q_insert(insert_index);

		if(index == block->first) {
			DEBUG(aflags[lu].level&8,
				("Setting block(%d)->next to (new)block(%d)\n",
				last_block_copied->block_count,
				new_block->block_count));
			new_block->first = CGQ_INDEX_OF_ELEM(cgq_item);
			last_block_copied->next = new_block;
		}

		if(index == block->last) {
			new_block->last= CGQ_INDEX_OF_ELEM(cgq_item);
			DEBUG(aflags[lu].level&8,
				("Setting block(%d)->last to index(%d)\n",
				new_block->block_count,
				CGQ_INDEX_NUM(CGQ_INDEX_OF_ELEM(cgq_item))));
		}

		DEBUG(aflags[lu].level&8,
			("Copy  old flow(%d) to new flow(%d) after flow(%d)\n",
			CGQ_INDEX_NUM(index),
			CGQ_INDEX_NUM(CGQ_INDEX_OF_ELEM(cgq_item)),
			CGQ_INDEX_NUM(insert_index)));

		cgq_item->cgq_op = elem->cgq_op;
		cgq_item->cgq_func = elem->cgq_func;
		cgq_item->cgq_ln = elem->cgq_ln; /* ?? PSP */
		cgq_item->cgq_dbln = elem->cgq_dbln; /* ?? PSP */
		cgq_item->cgq_scope_marker = elem->cgq_scope_marker; /* ?? PSP */
		cgq_item->cgq_arg = elem->cgq_arg; /* ?? PSP */

			/*
			** Copy the actual tree.  Attempt to minimize
			** the horrors of mixing pass1/pass2 structs.
			** For another approach, see routines in
			** expand.c!
			*/
		switch(cgq_item->cgq_op) {
 		case CGQ_EXPR_ND1:
			{
			ND1 *node;
			Boolean changed = 0;
			node = t1_copy(&changed, loop, counter_val, elem->cgq_arg.cgq_nd1);
			if(changed)
				node = op_optim(node);
			if(node->op == JUMP) {
				cgq_item->cgq_arg.cgq_nd2 = 
					(ND2 *)node;
				cgq_item->cgq_op = CGQ_EXPR_ND2;
					/* No call to new_expr */
				break;
			}
			cgq_item->cgq_arg.cgq_nd1 = node;
			if(cgq_item->cgq_arg.cgq_nd1->op != CBRANCH)
				new_expr(cgq_item->cgq_arg.cgq_nd1, new_block);
			else
				new_expr(cgq_item->cgq_arg.cgq_nd1->left,
					new_block);
			break;
			}
		case CGQ_EXPR_ND2:
			cgq_item->cgq_arg.cgq_nd2 =
				t2_copy(elem->cgq_arg.cgq_nd2);
			break;
		case CGQ_FIX_SWBEG:
			{
				/*
				** We need to copy the whole tree,
				** as cg will blow it away.
				*/
			ND2 *t2 = (ND2 *)talloc();
			*t2 = *(cgq_item->cgq_arg.cgq_nd2);
			cgq_item->cgq_arg.cgq_nd2 = t2;
			cgq_item->cgq_arg.cgq_nd2->left =
				(ND2 *)t1_copy(0, loop, counter_val, (ND1 *)(elem->cgq_arg.cgq_nd2->left));
			new_expr((ND1 *)(cgq_item->cgq_arg.cgq_nd2->left),
				new_block);
			}
			break;
		case CGQ_CALL_SID:
			if( cgq_item->cgq_func == db_symbol && !first_copy) {
				cgq_item->cgq_delete_flag = true;
			}
			else if ((cgq_item->cgq_func == db_sy_clear ||
				cgq_item->cgq_func == os_symbol) && ! last_copy ){
				cgq_item->cgq_delete_flag = true;
			}
			break;
		case CGQ_START_SCOPE:
			if(!first_copy)
				cgq_item->cgq_delete_flag = true;
			break;
		case CGQ_END_SCOPE:
			if(!last_copy) {
				cgq_item->cgq_delete_flag = true;
			}
			break;
		}	

		insert_index = CGQ_INDEX_OF_ELEM(cgq_item);

		CGQ_END_FOR_ALL

		last_block_copied = new_block;
		DEBUG(aflags[lu].level&8, (
			"last_block copied:(%d)\n",
			last_block_copied->block_count));
	}

	for(block = start_block; block != stop_block; block = block->next) {
	 	Block mapped_block;
		mapped_block = map_block(block);

		if(block != start_block) {
			map_predecessors(mapped_block, block->pred);
		}

		if(block != remember_last_block) {
			map_successors(mapped_block, block->succ);
		}
		else
			mapped_block->succ = block->succ;
	}

		/*
		** Set the predecessor list of the block following
		** the unrolled loops to the last block copied.
		** This assumes the copies alway exit by falling
		** through.
		*/

	stop_block->pred =
			init_block_list(last_block_copied);
			
	DEBUG_COND(aflags[lu].level&4, debug_block_map());
	
}

ND1 *
t1_copy(changed, loop, counter_val, tree)
Boolean *changed;
Loop loop;
long counter_val;
ND1 *tree;
{
	ND1 *new;

	new = t1alloc();
	*new = *tree;

	switch(optype(tree->op)) {
	case BITYPE:
		new->left = t1_copy(changed, loop, counter_val, tree->left);
		new->right = t1_copy(changed, loop, counter_val, tree->right);
		if(tree->op == CBRANCH) {
			assert(tree->right->op == ICON);
			new->right->lval = map_label(new->right->lval);
		}
		break;
	case UTYPE:
		new->left = t1_copy(changed, loop, counter_val, tree->left);
		new->right = tree->right;
		break;
	case LTYPE:
		if(new->op == NAME) {
			if(changed)
				*changed = 1;
			if(new->opt == loop->loop_unroll->loop_counter) {
				new->op = ICON;
				new->lval = counter_val;
				new->rval = 0;
			}
/*
			else
				new->rval = map_symbol(new->rval);
*/
		}
		break;
	default:
		Amigo_fatal("Bad op type in tree copy");	
	}

	return(new);
}

static ND2 *
t2_copy(tree)
ND2 *tree;
{
	ND2 *new;

	new = (ND2 *)tcopy(tree);

	switch(tree->op) {
	case LABELOP:
	case JUMP:
	case SWCASE:
	/* What about SWEND ?? */
		new->label = map_label(tree->label);	
		break;
	}
	return new;
}

static int
map_label(label)
int label;
{
	struct Label_descriptor *l_d;
	if(min_label > max_label) return label;
	l_d = get_label_descriptor(label);
	return l_d->map_label ? l_d->map_label : label;
}

	/*
	** Set the predecessor field of a mapped (new) block, using the
	** predecessor list of the (old) unmapped block.
	** Also add the new block to the successors of any
	** unmapped (old) block.
	*/
static void
map_predecessors(block, bl)
Block block;
Block_list bl;
{
	
	block->pred = 0;
	while(bl) {
		Block mapped_block = map_block(bl->block);
		Block_list new;
		new = init_block_list(mapped_block);
		new->next = block->pred;
		block->pred = new;
		if(mapped_block == bl->block) {
			new = init_block_list(block);
			new->next = mapped_block->succ;
			mapped_block->succ = new;
		}
		bl = bl->next;
	}
}

static void
map_successors(block, bl)
Block block;
Block_list bl;
{
	
	block->succ = 0;
	while(bl) {
		Block mapped_block = map_block(bl->block);
		Block_list new;
		new = init_block_list(mapped_block);
		new->next = block->succ;
		block->succ = new;
		if(mapped_block == bl->block) {
			new = init_block_list(block);
			new->next = mapped_block->pred;
			mapped_block->pred = new;
		}
		bl = bl->next;
	}
}

	/*
	** Delete edge in the flow graph and fix the predecessor
	** and successor lists accordingly.
	*/
static void
remove_edge(from_block, to_block)
Block from_block, to_block;
{
	Block_list bl, remember_bl;
	remember_bl = 0;
	DEBUG(aflags[lu].level&4,
		("\tremoving edges from block(%d) to block(%d)\n",
		from_block->block_count, to_block->block_count));
	DEBUG(aflags[lu].level&4,("\tfrom_block(%d)->succ before",
		from_block->block_count));
	DEBUG_COND(aflags[lu].level&4,pr_block_list(from_block->succ,
		": "));
	DEBUG(aflags[lu].level&4,("\n"));
	for(bl = from_block->succ; bl && bl->block != to_block; bl = bl->next) {
		remember_bl = bl;
	}
	assert(bl);
	if(bl) {
		assert(bl->block == to_block);
		if( remember_bl )
			remember_bl->next = bl->next;
		else
			from_block->succ = bl->next;
	}
	DEBUG(aflags[lu].level&4,("\tfrom_block(%d)->succ after",
		from_block->block_count));
	DEBUG_COND(aflags[lu].level&4,pr_block_list(from_block->succ,
		": "));
	DEBUG(aflags[lu].level&4,("\n"));

	remember_bl = 0;

	DEBUG(aflags[lu].level&4,("\tto_block(%d)->pred before",
		to_block->block_count));
	DEBUG_COND(aflags[lu].level&4,pr_block_list(to_block->pred,
		"\t: "));
	DEBUG(aflags[lu].level&4,("\n"));
	for(bl = to_block->pred; bl && bl->block != from_block; bl = bl->next) {
		remember_bl = bl;
	}
	assert(bl);
	if(bl) {
		assert(bl->block == from_block);
		if( remember_bl )
			remember_bl->next = bl->next;
		else
			to_block->pred = bl->next;
	}
	DEBUG(aflags[lu].level&4,("\tto_block(%d)->pred after",
		to_block->block_count));
	DEBUG_COND(aflags[lu].level&4,pr_block_list(to_block->pred,
		": "));
	DEBUG(aflags[lu].level&4,("\n"));
}

	/*
	** Delete the tree which increments the counter in the loop
	** body.  The only complication is that this update may
	** be under a comma op ( because of an AMIGO optimization ).
	** To solve this, split the cg_q item apart while looking
	** for the update.  We might avoid all this by getting
	** rid of all top level comma ops in the cg_q.
	*/
static Boolean
delete_counter_update(index, counter)
Cgq_index index;
Expr counter;
{
	cgq_t *item;
	item = CGQ_ELEM_OF_INDEX(index);
	if(item->cgq_arg.cgq_nd1->op != COMOP) {
		if(item->cgq_arg.cgq_nd1->opt->object != 0
			&&
			item->cgq_arg.cgq_nd1->opt->object == counter->object) {
			cg_q_delete(index);
			return true;
		}
		else {
			return false;
		}
	}
	else {	/* Split the comma into two cgq_items and try the
		** deletion on both sides.
		*/
		cgq_t *right;
			
		right = cg_q_insert(index);
		item = CGQ_ELEM_OF_INDEX(index);
		right->cgq_op = item->cgq_op;
		right->cgq_func = item->cgq_func;
		right->cgq_arg.cgq_nd1 = item->cgq_arg.cgq_nd1->right;
		item->cgq_arg.cgq_nd1 = item->cgq_arg.cgq_nd1->left;
		right->cgq_ln = item->cgq_ln;
		right->cgq_dbln = item->cgq_dbln;
		return
			delete_counter_update(index,counter) ||
			delete_counter_update(CGQ_INDEX_OF_ELEM(right), counter);
	}

}

static void
delete_branch(loop)
Loop loop;
{
	Cgq_index test_index = loop->loop_unroll->loop_cbranch_index;
	Cgq_index prev_index;
	prev_index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(test_index));
	if(test_index == loop->loop_unroll->loop_test_block->last)
		loop->loop_unroll->loop_test_block->last = prev_index;
	cg_q_delete(test_index);
	remove_edge(loop->loop_unroll->loop_test_block, loop->header->next);
}

static void 
insert_counter_update(counter_val, loop)
long counter_val;
Loop loop;
{
	ND1 *node = t1alloc();
	cgq_t *cgq_item;

	node->left = expr_to_nd1(loop->loop_unroll->loop_counter);
	assert(node->left->op == NAME);

	node->op = ASSIGN;
	node->type = node->left->type;
	node->flags = FF_SEFF;

	node->right = t1alloc();
	node->right->op = ICON;
	node->right->rval = ND_NOSYMBOL;
	node->right->lval = counter_val;
	node->right->type = node->left->type;
	node->right->flags = 0;

	cgq_item = cg_q_insert(last_block_copied->last);
	cgq_item->cgq_op = CGQ_EXPR_ND1;
	cgq_item->cgq_arg.cgq_nd1 = node;

	last_block_copied->last = CGQ_INDEX_OF_ELEM(cgq_item);

	new_expr(node, last_block_copied);
}

#ifndef NODBG
static void
pr_symbol_map(symbol_map)
Symbol_map symbol_map;
{
	Symbol_map s_m;
	fprintf(stderr,"\n\t[][][] symbol_map [][][][]\n");
	for(s_m = symbol_map; s_m; s_m = s_m->next) {
		fprintf(stderr,"\tsymbol(%d) --> symbol(%d)\n",
			s_m->symbol, map_symbol(s_m->symbol));
	}
}
#endif

/*
** Main driver for determining what type of loop we have. The
** loop type will determine how we unroll a loop.
*/

static Loop_type
loop_type(loop)
Loop loop;
{

	Loop_type test_type = not_countable;
	Loop_type initial_type = not_countable;
	Loop_type code_growth = not_cost_effective;

	loop->loop_unroll->end_limit = 0;
	loop->loop_unroll->begin_limit = 0;

	DEBUG(aflags[lu].level&2,("\nLoop(%d) loop_cbranch_index(%d)\n",
		loop->id,
		CGQ_INDEX_NUM(loop->loop_unroll->loop_cbranch_index) ));

	DEBUG(aflags[lu].level&2,("increment_index(%d) increment(%d)\n",
		CGQ_INDEX_NUM(loop->loop_unroll->increment_index),
		INCREMENT(loop)));

	/*
	**  If the index of the loop test was not set, then there
	**  was a problem finding the CBRANCH containing the loop test,
	**  thus this loop should not be a candidate for unrolling.
	**
	**  If  the induction variables' update value was not found in the
	**  same block as the loop test OR it's value is 0,  then this
	**  loop should not be a candidate for unrolling.
	*/

	if ( loop->loop_unroll->loop_cbranch_index == CGQ_NULL_INDEX ||
	     loop->loop_unroll->increment_index == CGQ_NULL_INDEX || 
	     INCREMENT(loop) == 0 )
		return(not_countable);

	if ( (tree_count = cost_trees(loop)) > lu_l_size_threshold)
	{
		DEBUG(aflags[lu].level&2,("quitting: cost_trees (%d > %d):\n", tree_count, lu_l_size_threshold));
		return(not_cost_effective);
	}
	
	if ( (test_type = get_test_type(loop)) == not_countable )
		return(not_countable);

	DEBUG_COND(aflags[lu].level&2,
		pr_object(loop->loop_unroll->loop_counter->object));
	DEBUG(aflags[lu].level&2,("\n"));

	DEBUG(aflags[lu].level&2,("pattern(%s):\n", pattern_st[branch_op_type]));

	if ( (initial_type = get_initial_type(loop)) == not_countable )
		return(not_countable);


	if ( test_type == const_limits && initial_type == const_limits)
	{
		
		DEBUG(aflags[lu].level&2,("begin_limit(%d):\n", BEGIN_LIMIT(loop)));
		DEBUG(aflags[lu].level&2,("end_limit(%d):\n", END_LIMIT(loop)));

		ITERATIONS(loop) = iterations(loop);

		if ( ITERATIONS(loop) > MIN_ITERATIONS)
		{
			
			DEBUG(aflags[lu].level&2,("ITERATIONS: %d\n",
				ITERATIONS(loop)));

			/* 
			** Determine if the amount of code expansion is
			** acceptable.
			*/

			code_growth = code_expansion(loop);
#ifndef NODBG
			if (code_growth == not_cost_effective)
				DEBUG(aflags[lu].level&2,("quitting: code_growth is not acceptable (%d):\n", tree_count * ITERATIONS(loop)));
#endif
				
			return(code_growth);
		}
		else 
			return(not_countable);
	}
	else /* one or both are loop invariants */
		return(invar_limits);
}


/*
** Based upon the begin_limit (the induction variable's initial value,
** which must be a constant), the end_limit (the upper bound of the loop,
** which also must be a constant), and the type of loop test, determine 
** the number of iterations this loop will make.
*/

static long
iterations(loop)
Loop loop;
{
	int sign;

        /* i.e., sign = 1 if c > 0, -1 if c < 0, 0, otherwise */

        /* Expain the patterns we identified: */

	switch(branch_op_type) {

	case NOT_LE:
	case NOT_GE:
		sign = ( INCREMENT(loop) > 0 ? 1: ( INCREMENT(loop) < 0 ? -1 : 0 ) );
		/* i.e., sign = 1 if c > 0, -1 if c < 0, 0, otherwise */

		return( COUNT(sign*BEGIN_LIMIT(loop),sign*END_LIMIT(loop), sign*INCREMENT(loop)));
	case NOT_INDVAR:
	case NOT_NE:
        	if( (BEGIN_LIMIT(loop)-END_LIMIT(loop)) % INCREMENT(loop) != 0 )
			return(0);
	        /* FALLTHROUGH */
	case NOT_LT:
	case NOT_GT:
		sign = ( INCREMENT(loop) > 0 ? 1: ( INCREMENT(loop) < 0 ? -1 : 0 ) );
		/* i.e., sign = 1 if c > 0, -1 if c < 0, 0, otherwise */
       	 	return( COUNT(sign*BEGIN_LIMIT(loop), (sign*END_LIMIT(loop)) - 1, 
			sign*INCREMENT(loop)));
	default:
        	return(0);
	} /* for switch */

}

/* 
** Determine if the loop test is of the appropriate form for
** unrolling.  Determine if the induction variable is being
** compared against a constant or a loop invariant. Categorize
** the loop test pattern - will be used for determining how
** many times the loop will be unrolled.
*/

static Loop_type
get_test_type(loop)
Loop loop;
{

	ND1 *loop_test;
	Loop_type  test_type;

	branch_op_type = UNKNOWN;

	/*
	** These ops work when they are below negation.
	*/
#define IS_REL_OP1(node)  ( node->op == NE ||  \
			   node->op == LT || node->op == GT || \
			   node->op == LE || node->op == GE )  

	/*
	** These ops work when they are directly below CBRANCH.
	*/
#define IS_REL_OP2(node)  ( node->op == EQ ||  \
			   node->op == LT || node->op == GT || \
			   node->op == LE || node->op == GE )  

	loop_test = CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_cbranch_index)->
			cgq_arg.cgq_nd1;

	if (loop_test->op != CBRANCH ) {
		branch_op_type = UNKNOWN;
		test_type = not_countable;
	}

	if (loop_test->left->op == NOT) 
	{
		if (IS_REL_OP1(loop_test->left->left) )
		{	
			test_type = rel_op_test_type(1,
					loop_test->left->left, loop);
		}
		else if (IS_INDVAR(loop_test->left->left,loop) )
		{
			loop->loop_unroll->loop_counter = 
						loop_test->left->left->opt;
			branch_op_type = NOT_INDVAR;
			test_type = const_limits;
		}
		else
		{
			branch_op_type = UNKNOWN;
			test_type = not_countable;
		} 
		
	}
	else if ( IS_REL_OP2(loop_test->left) )
	{
		test_type = rel_op_test_type(0,
					loop_test->left, loop);
	}
	else
	{
		branch_op_type = UNKNOWN;
		test_type = not_countable;
        }

	return(test_type);

}

/*
** 
*/

static Loop_type
rel_op_test_type(below_not,node,loop)
int below_not;
ND1 *node;
Loop loop;
{
	if (IS_INDVAR(node->left,loop) 

			/* Next is hack to avoid problem with CONV nodes,
			** This hack should be removed and
			** insert_counter_update generalized
			*/
		&& node->left->op == NAME

		&& ( IS_ICON(node->right) || IS_INVARIANT(node->right, loop)) )
	{
		loop->loop_unroll->loop_counter = node->left->opt;

		switch (node->op)
		{
			case NE: if (below_not)
					(branch_op_type = NOT_NE);
				 else
					return(not_countable);
				 break; 
					/*
					** < is the same as NOT >= , etc...
					*/
			case LT:
				branch_op_type = below_not ? NOT_LT : NOT_GE;
				break;
			case LE:
				branch_op_type = below_not ? NOT_LE : NOT_GT;
				break;
			case GT:
				branch_op_type = below_not ? NOT_GT : NOT_LE;
				break;
			case GE:
				branch_op_type = below_not ? NOT_GE : NOT_LT;
				break;
			case EQ: if (below_not)
					return(not_countable);
				 else 
					branch_op_type = NOT_NE;
				 break;
			default: return(not_countable);
		}

		loop->loop_unroll->end_limit = node->right;

		if ( IS_ICON(node->right) )
		{
			return(const_limits);
		}
		else
			return(invar_limits);

	}
	else
		return(not_countable);
}

/* 
** Determine if the induction variable's initial value is a constant 
** or a loop invariant.  Set BEGIN_LIMIT(loop) if it's a constant.
*/

static Loop_type
get_initial_type(loop)
Loop loop;
{

	Block block;
	Loop_type type;

		/*
		** Look for the block containing os_loop(start) 
		** and hope it contains the initialization of
		** the induction variable.  Otherwise, GIVE UP.
		*/
	if(loop->header->block_flags & BL_PREHEADER) {
		DEBUG(aflags[lu].level&2,("Searching HEADER for ind_var init.\n"));
		block = loop->header;
	}
	else if(loop->header->pred->block->block_flags & BL_PREHEADER) {
		DEBUG(aflags[lu].level&2,("Searching PREHEADER for ind_var init.\n"));
		block = loop->header->pred->block;
	}
	else
		return(not_countable);

	FOR_ALL_ND1_IN_BLOCK_REVERSE(block,flow,node,index)

		if ( find_ind_init(node,loop, &type) )
			return(type);

	END_FOR_ALL_ND1_IN_BLOCK
	
	return(not_countable);  
}

/*
** Traverse the ND1 looking for the assignment of the 
** induction variable to it's initial value. If the assignment 
** is within a COMOP, must be able to find it.  
** If the assignment is to an ICON, then we know the start value 
** of the loop iterator. If it's to a loop invariant, then the
** start value is unknown at this time. 
** Return 0 if the search may be continued, 1 if the type of
** the initialization has been determined, i.e., the assignment to
** the induction variable has been seen.
*/

static int
find_ind_init(node,loop,type)
ND1 *node;
Loop loop;
Loop_type *type;
{
	
	if ( node->op != COMOP) 
	{
		if (node->op == ASSIGN ) 
		{
			if (bv_belongs(node->left->opt->object->setid, loop->ind_vars))
			{
				if ( IS_ICON(node->right) ) /* && is a scalar */
				{
					loop->loop_unroll->begin_limit = node->right;
					*type = const_limits;
					return(1);
				}
				else if (IS_INVARIANT(node->right,loop) )
				{
					*type = invar_limits;
					loop->loop_unroll->begin_limit = node->right;
					return(1);
				}
				else
				{
					*type = not_countable;
					return(1);
				}
			}
		}
	}
	else
	{
		return 
			find_ind_init(node->left,loop, type) ||
			find_ind_init(node->right, loop, type);
			
	}

	return(0);
}


static int
cost_trees(loop)
Loop loop;
{
	Block block, start_block, stop_block;
	int trees = 0;
	
	start_block = loop->header->next;
	stop_block  = loop->end;

	for(block = start_block; block != stop_block; block = block->next) {

                CGQ_FOR_ALL_BETWEEN(elem, index, block->first, block->last)
                                /* 
				** If there is a static definition in the 
				** loop don't unroll it
				*/

			if(elem->cgq_op == CGQ_EXPR_ND2 && 
				elem->cgq_arg.cgq_nd2->op == DEFNAM)
				return(lu_l_size_threshold + 1);
			if(elem->cgq_op == CGQ_EXPR_ND1)
				trees++;
		CGQ_END_FOR_ALL_BETWEEN
	}


	DEBUG(aflags[lu].level&2,("cost_of_trees(%d):\n", trees));

	return(trees);
}

/* 
** At this point, we know the number of ND1 trees in a loop and
** we know the number of loop iterations. Based on those two factors,
** Determine if there is a potential for explosive code expansion. If
** there is not, then completely unroll the loop; otherwise unroll the
** loop in CHUNKS.
*/

static Loop_type
code_expansion(loop)
Loop loop;
{
	if ( tree_count * ITERATIONS(loop) < lu_threshold )
		return(const_limits);
	else
		return(not_cost_effective);
}

	/*
	** Find the cg_q element which increments the counter in the loop
	** body.  The only complication is that this update may
	** be under a comma op ( because of an AMIGO optimization ).
	** To solve this, split the cg_q item apart while looking
	** for the update.  Update the loop info.
	** We might avoid all this by getting rid of all top level
	** comma ops in the cg_q.
	*/

static Boolean
fix_counter_update(index, l_u_i)
Cgq_index index;
Loop_unroll_info l_u_i;
{
	cgq_t *item;
	item = CGQ_ELEM_OF_INDEX(index);
	if(item->cgq_arg.cgq_nd1->op != COMOP) {
		if(item->cgq_arg.cgq_nd1->opt->object != 0
			&&
			item->cgq_arg.cgq_nd1->opt->object == l_u_i->loop_counter->object) {
			l_u_i->increment_index = index;
			return true;
		}
		else {
			return false;
		}
	}
	else {	/* Split the comma into two cgq_items and look on
		** both sides.
		*/
		cgq_t *right;
			
		right = cg_q_insert(index);
		item = CGQ_ELEM_OF_INDEX(index);
		right->cgq_op = item->cgq_op;
		right->cgq_func = item->cgq_func;
		right->cgq_arg.cgq_nd1 = item->cgq_arg.cgq_nd1->right;
		item->cgq_arg.cgq_nd1 = item->cgq_arg.cgq_nd1->left;
		right->cgq_ln = item->cgq_ln;
		right->cgq_dbln = item->cgq_dbln;
		return
			fix_counter_update(index,l_u_i) ||
			fix_counter_update(CGQ_INDEX_OF_ELEM(right), l_u_i);
	}

}
