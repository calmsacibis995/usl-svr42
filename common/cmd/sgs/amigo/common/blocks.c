/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/blocks.c	1.33"
#include "amigo.h"
#include "l_unroll.h" /* for now PSP */
#include <memory.h>

#define PREV_IS_PRED 1
#define PREV_NOT_PRED 0

struct Branch_descriptor {
	int target_label;	/* target of this branch */
	Block block;		/* block in which branch is found */
	ND1* tree;
};

static max_label, min_label;
		/*
		** label_table: ( dynamic ) table indexed by label-min_label;
		** branch: table of branches, to be sorted on 
		*/
void label_table_alloc();
void label_table_zero();
struct Label_descriptor *label_table;
#define LABEL_INDEX(label) ((label) - min_label)

int get_max_label()
{
	return max_label;
}

int get_min_label()
{
	return min_label;
}

struct Label_descriptor *
get_label_descriptor(label)
int label;
{
	return label_table + LABEL_INDEX(label);
}

DEBUG_UNCOND(static void print_depth_first();)

struct Block_list *
init_block_list(block)
struct Block *block;
{
	struct Block_list *list=Arena_alloc(GLOBAL,1,struct Block_list);
	list->block=block;
	list->next=0;
	return list;
}

static void
add_block(list,block)
struct Block_list **list;
struct Block *block;
{
	struct Block_list *new=Arena_alloc(GLOBAL,1,struct Block_list);
	new->block = block;
	new->next = *list;
	*list = new;

}

static int block_count;

Block
new_block_alloc()
{
	Block new;
	++block_count;
	new = Arena_alloc(GLOBAL,1,struct Block);
	new->block_count = block_count;
	new->first = new->last = CGQ_NULL_INDEX;
	new->next = 0;
	new->lex_block_num = 0;
	new->succ = 0;
	new->pred = 0;
	new->next = 0;
	new->kills = 0;
	new->loop = 0;
	new->block_flags = 0;
	return new;
}

static Block
new_block(prev,prev_is_pred,flowindex)
struct Block *prev;	/* lexically previous block */
int prev_is_pred;	/* set if prev is a predecessor of this block */
Cgq_index flowindex;	/* first flownode of this block */
{
	struct Block *curr;
	if (flowindex == CGQ_NULL_INDEX)
		return 0;
	else
		curr = new_block_alloc();

	if (prev) {
		prev->next = curr;
	}

	if (prev_is_pred) {
		curr->pred = init_block_list(prev);
		prev->succ = init_block_list(curr);
	}
	else
		curr->pred = 0;

	curr->first = curr->last = flowindex;
	return curr;
}


static void resolve(), remove_jump_to_jump();

struct Block *
basic_block(branch_count, max_lbl, min_lbl, has_loop) 
int branch_count, max_lbl, min_lbl;
Boolean *has_loop;
{
	struct Branch_descriptor *branch, *curr_branch;

	struct Block *first_block;
	struct Block *curr_block;
	int last_label; /* Used to determine if a JUMP is a potential
			   target of another JUMP or branch. */
	Arena local_arena = arena_init();
	Cgq_index last_index = 0;

	max_label = max_lbl;
	min_label = min_lbl;
	block_count = 0;
	if (branch_count) 
		curr_branch = branch =
			Arena_alloc(local_arena,branch_count,struct Branch_descriptor);
	label_table_alloc(local_arena);
	label_table_zero();

	curr_block = first_block =
		new_block((struct Block *)0,PREV_NOT_PRED,CGQ_FIRST_INDEX);

	last_label = 0;

	CGQ_FOR_ALL(root,index)
		if (HAS_ND1(root)) {
			curr_block->block_flags |= BL_HAS_EXPR;
				/*
				** No ND1 is a JUMP so any branch to 
				** last_label is not a branch to a jump.
				*/
			last_label = 0;
		}

		if (root->cgq_op == CGQ_CALL_INT || root->cgq_op == CGQ_CALL ||
		    root->cgq_op == CGQ_CALL_SID) {
			if (root->cgq_func == os_loop) {
				*has_loop = true;
				switch(root->cgq_arg.cgq_int) {
				case OI_LSTART:
					curr_block->block_flags |= BL_PREHEADER;
					/* FALLTHRU */
				case OI_LBODY:
					/* set HAS_EXPR here, so we don't have
					   to update this flag if exprs are
					   added to the loop header -- no harm
					   if flag is falsely set
					*/
					curr_block->block_flags |= BL_HAS_EXPR;
					/* FALLTHRU */
				case OI_LEND:
				case OI_LCOND:
					curr_block->block_flags |= BL_HAS_OS_LOOP;
					break;
				default:
					break;
				}
			}	
			/* if cg_setlocctr not at beginning of current block
			   then start a new block
			*/
			else if (root->cgq_func == cg_setlocctr && 
			    curr_block->first != index ) {
				curr_block->last = last_index;
				curr_block = new_block(curr_block, 
				    PREV_IS_PRED, index);
			}
		}

		else if (root->cgq_op == CGQ_EXPR_ND1 || 
		    root->cgq_op == CGQ_EXPR_ND2) 
		    switch(root->cgq_arg.cgq_nd2->op) {
		    int fallthru;
		case SWEND:
			if(root->cgq_arg.cgq_nd2->lval != -1) {/* default case*/
			curr_branch->target_label = root->cgq_arg.cgq_nd2->lval;
			curr_branch->block = curr_block;
			curr_branch->tree = (ND1 *)root->cgq_arg.cgq_nd2;
			++curr_branch;
			fallthru = PREV_NOT_PRED;/* no fallthru from SWEND */
			}
			else /* no default case */
				/* fallthru if no case is taken */
				fallthru = PREV_IS_PRED;

			curr_block->last = index;
			curr_block = new_block(curr_block,fallthru,
				CGQ_NEXT_INDEX(root));
			break;
		case SWCASE: /* don't start a new block */
			curr_branch->target_label = root->cgq_arg.cgq_nd2->label;
			curr_branch->block = curr_block;
			curr_branch->tree = (ND1 *)root->cgq_arg.cgq_nd2;
			++curr_branch;
			break;
		case JUMP:
			curr_branch->target_label = root->cgq_arg.cgq_nd2->label;
			curr_branch->block = curr_block;
			curr_branch->tree = (ND1 *)root->cgq_arg.cgq_nd2;
			++curr_branch;

			curr_block->last = index;
			curr_block = new_block(curr_block,PREV_NOT_PRED,
				CGQ_NEXT_INDEX(root));
			if(last_label){
					/*
					** Any branch to here is a jump to
					** jump. 
					*/
				label_table[LABEL_INDEX(last_label)].map_label =
					root->cgq_arg.cgq_nd2->label;
				last_label = 0;	
			}
			break;
		case CBRANCH:
			curr_branch->target_label = root->cgq_arg.cgq_nd1->right->lval;
			curr_branch->tree = root->cgq_arg.cgq_nd1;
			curr_branch->block = curr_block;
			++curr_branch;

			curr_block->last = index;
			curr_block = new_block(curr_block,PREV_IS_PRED,
				CGQ_NEXT_INDEX(root));
			break;
		case LABELOP:
			/* if LABELOP is not beginning of current block
			   and not the second item in current block following
			   a cg_setlocctr, then begin a new block
			*/
			if (curr_block->first != index  &&
			    !(last_index == curr_block->first &&
			    CGQ_ELEM_OF_INDEX(last_index)->cgq_op == CGQ_CALL_INT &&
			    CGQ_ELEM_OF_INDEX(last_index)->cgq_func == cg_setlocctr)) {
				curr_block->last = last_index;
				curr_block = new_block(curr_block, 
				PREV_IS_PRED, index);
			}
			last_label = root->cgq_arg.cgq_nd2->label;
			label_table[LABEL_INDEX(last_label)].block = curr_block;
			break;
		case RETURN:
			curr_block->last=index;
			curr_block = new_block(curr_block,PREV_NOT_PRED,
				CGQ_NEXT_INDEX(root));
			break;
		}
		last_index = index;
	CGQ_END_FOR_ALL

	curr_block->last = last_index;
	if (branch_count) {
		remove_jump_to_jump(branch,curr_branch);
		resolve(branch,curr_branch);
	}
	arena_term(local_arena);
	(void)order_blocks(first_block);
	return first_block;
}

	/*
	** Go down the block list in lexical order to set
	** the lexical number.
	*/
int
order_blocks(blocks)
Block blocks;
{
	Block blk;
	int count;
	count = 0;
	for(blk = blocks; blk; blk = blk->next) {
		blk->lex_block_num = ++count;
	}
	return count;
}

void
label_table_alloc(arena)
Arena arena;
{
	label_table = Arena_alloc(arena, max_label-min_label+1,
		struct Label_descriptor);
}

void
label_table_zero()
{
	if (max_label) {
				/*
				** Labels are stored in label_table  at offset
				** label-min_label.  E.g., if min_label == 6,
				** max_label == 9, use slots 0,1,2,3.
				*/	
		(void)memset((char *)label_table,0,
			(max_label-min_label+1)*sizeof(struct Label_descriptor));
	}
}


static int df_num = 0;
static void visit();
static int *visited; /* array of flags */
static Block *dfb;	/* array of blocks in depth first ordering */

#ifndef NODBG
static void
print_depth_first()
{
	int i;
	DPRINTF("depth first order: ( ");
	for(i = df_num; i<= block_count; i++)
		DPRINTF("%d ",dfb[i]->block_count);	
	DPRINTF(")\n");
}
#endif

int
get_block_count() { return block_count; }

Block *
get_dfn_ptr_first() { return &dfb[df_num]; } 

Block *
get_dfn_ptr_last() { return &dfb[get_block_count()]; } 


	/* Sort the blocks in depth-first order, i.e., shallower before
	** deeper.  See Aho,Sethi & Ullman, p 662.
	*/
void
init_depth_first(first_block)
Block first_block;
{
		/* allow 1-indexing for arrays */
	visited = Arena_alloc(GLOBAL, (block_count + 1), int);
	dfb = Arena_alloc(GLOBAL, (block_count + 1), Block);
	/* dfb = (Block *)malloc(sizeof(Block) * (block_count + 1)); */
	if(!(visited && dfb)) Amigo_fatal("Malloc failed");
		/* mark all blocks 'not visited' */
	(void)memset((char *)visited, 0, sizeof(int) * (block_count + 1));
	(void)memset((char *)dfb, 0, sizeof(Block) * (block_count + 1));
	df_num = block_count;
	visit(first_block);
	df_num++; /* Will be 1 if no dead blocks */
	DEBUG_COND(aflags[df].level&4, print_depth_first());
	DEBUG(aflags[df].level&4, ("dead block count: %d\n",df_num-1));
}

static void remove_from_flow_graph();

void
remove_unreachable(first_block)
Block first_block;
{
	Block curr;
	if (df_num == 1) /* no unreachable blocks */
		return;

	/* delete unreachable code */
	for (curr = first_block; curr;  curr = curr->next) {
		Cgq_index before, after, index, new_index;

		if (visited[curr->block_count]) 
			continue;
START_OPT
		DEBUG(aflags[un].level&1,("block=%d(%d-%d) unreachable\n",
			curr->block_count,
			CGQ_INDEX_NUM(curr->first),
			CGQ_INDEX_NUM(curr->last)));

		/* since we never delete the first block (it is reachable)
		   the before entry will not be null, since it is only
		   null for the first block
		*/
		before=CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(curr->first));
		after= CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(curr->last));

		/* delete entries in the basic block */
		for (index=curr->first; index != after; index = new_index) {
			cgq_t *elem = CGQ_ELEM_OF_INDEX(index);
			new_index = CGQ_NEXT_INDEX(elem);

			if (elem->cgq_op == CGQ_EXPR_ND1 ||

				( elem->cgq_op == CGQ_EXPR_ND2 &&
					( elem->cgq_arg.cgq_nd2->op == JUMP
#ifdef DBLINE
					|| elem->cgq_arg.cgq_nd2->op == DBLINE 
#endif
					|| elem->cgq_arg.cgq_nd2->op == SWCASE
					)
				) 

				|| (elem->cgq_op == CGQ_CALL && 
				elem->cgq_func == db_lineno)
			) {
				DEBUG(aflags[un].level&2,
					("Deleting cgq item %d\n",
					CGQ_INDEX_NUM(index)));
				amigo_delete(curr, index);
			}
		}

		/* if after == CGQ_NULL_INDEX then curr is the last block
		   in the program, since we will not delete the last entry
		   in the last block (it is a call to al_e_fcode), we need
		   not change "last"
		*/
		if (after != CGQ_NULL_INDEX)
			curr->last=CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(after));

		curr->first=CGQ_NEXT_INDEX(CGQ_ELEM_OF_INDEX(before));
		/* if entire block has been deleted, "first" will be the
		   successor of "last" which marks block as empty to
		   CGQ_FOR_ALL_BETWEEN
		*/
		curr->block_flags &= ~BL_HAS_EXPR;
		remove_from_flow_graph(curr);
STOP_OPT
	}
}

static void
remove_from_flow_graph(block)
Block block;
{
	/*
	** Remove this block from each successor's predecessor list.
	** Then empty the successor list.
	** 
	** Remove this block from each predecessor's successor list.
	** Then empty the predecessor list.
	*/
	Block_list bl;
	for(bl = block->succ; bl; bl = bl->next) {
		Block_list b, last = 0;
		for(b = bl->block->pred; b; last = b, b = b->next) {
			if(b->block == block) { /* found it */
				if(last) 
					last->next = b->next;
				else
					bl->block->pred = b->next;
				DEBUG(aflags[un].level&2,
					("Removing block %d from pred list of block %d\n",
					block->block_count,
					bl->block->block_count));
				break;	
			}
		}
	}

	block->succ = 0;

	for(bl = block->pred; bl; bl = bl->next) {
		Block_list b, last = 0;
		for(b = bl->block->succ; b; last = b, b = b->next) {
			if(b->block == block) { /* found it */
				if(last) 
					last->next = b->next;
				else
					bl->block->succ = b->next;
				DEBUG(aflags[un].level&2,
					("Removing block %d from succ list of block %d\n",
					block->block_count,
					bl->block->block_count));
				break;	
			}
		}
	}

	block->pred = 0;
}

static void
visit(bl)
Block bl;
{
	Block_list child;
	visited[bl->block_count] = true;
	for(child = bl->succ; child; child = child->next) {
		if(!visited[child->block->block_count]) visit(child->block);
	}
	dfb[df_num--] = bl;
}

static
bcomp(first,second) 
const myVOID *first, *second;
{
	return ((struct Branch_descriptor *)first)->target_label - 
		((struct Branch_descriptor *)second)->target_label;
}

static void 
remove_jump_to_jump(branch, branch_stop)
struct Branch_descriptor branch[], *branch_stop;
{
	DEBUG(aflags[jj].level&2, ("max_label%d, min_label%d",max_label,
		min_label));
#ifndef NODBG
	if(aflags[jj].level&2){
		int lbl; 
		struct Branch_descriptor *br;
		for(lbl=min_label;lbl <= max_label; lbl++) {
			dprintf("lbl %d: map_label %d; block %x\n",
				lbl,
				label_table[LABEL_INDEX(lbl)].map_label,
				label_table[LABEL_INDEX(lbl)].block);
		}
		for(br=branch; br < branch_stop; br++) {
			dprintf("branch %x, branch target %d, block %x\n",
				br, br->target_label, br->block);
		}
	}
#endif
	DEBUG_UNCOND( { int jump_removed = 0;)
	for (; branch < branch_stop; ++branch) {
		int label_index = LABEL_INDEX(branch->target_label);

		assert(label_index >= 0 && label_index <= LABEL_INDEX(max_label) 
			&& label_table[label_index].block != 0);
		if(label_table[label_index].map_label) {
			ND1 * tr = branch->tree;
				/*
				** If this "branch" is really a branch,
				** and not a SWEND or SWCASE, then see
				** if it's a branch to a JUMP
				*/
			if(tr->op == CBRANCH) { 
				tr->right->lval = branch->target_label =
					label_table[label_index].map_label;
				new_expr(tr,0); /* It changed, rehash */
				DEBUG_UNCOND(jump_removed++;)
			}
			else if(tr->op == JUMP) {
					((ND2 *)tr)->label =
						branch->target_label =
						label_table[label_index].map_label;
					DEBUG_UNCOND(jump_removed++;)
			}
		}
	}
	DEBUG(aflags[jj].level, ("Jump to jumps removed: %d\n",jump_removed));
	DEBUG_UNCOND(})
}

static void 
resolve (branch, branch_stop)
struct Branch_descriptor branch[], *branch_stop;
{
		/* sort branch table on target labels */
	qsort((char *)branch,(unsigned)(branch_stop-branch),sizeof(*branch), bcomp);

		/* Complete flow graph info to account for branches */
	for (; branch < branch_stop; ++branch) {
		int label_index = LABEL_INDEX(branch->target_label);

		if (label_index < 0 || label_index > LABEL_INDEX(max_label) ||
			label_table[label_index].block == 0)
			Amigo_fatal("unresolved_branch");

		add_block(&branch->block->succ,label_table[label_index].block);
		add_block(&label_table[label_index].block->pred,branch->block);
	}
}

#ifndef NODBG
static void 
pr_block(block) struct Block *block;
{
	fprintf(stderr,
		"block=%d lex_block_num=%d first=%d last=%d next=%d loop=%d scope=%d block_flags=%d",
		block->block_count,
		block->lex_block_num,
		CGQ_INDEX_NUM(block->first),
		CGQ_INDEX_NUM(block->last),
		block->next ? block->next->block_count : 0,
		block->loop ? block->loop->id : 0,
		block->scope,
		block->block_flags
		);
	pr_block_list(block->pred,"\n\tpreds");
	pr_block_list(block->succ," succs");
	fprintf(stderr, "\n\tkills: ");
	pr_object_list(block->kills);
      	fprintf(stderr, "block ind vars: ");
      	pr_object_set(block->ind_vars);
	fflush(stderr);
}

void 
pr_blocks(block)
struct Block *block;
{
	for (; block; block = block->next)
		pr_block(block);
	
}

void 
pr_block_list (list,msg)
struct Block_list *list;
char *msg;
{
	fprintf(stderr," %s (", msg);
	for (; list ; list = list->next)
		fprintf(stderr,"%d ", list->block->block_count);
	fputs(")",stderr);

}

void
pr_label_table(from, to)
int from, to;
{
	int i;
	fprintf(stderr,"%s%s%s",
		"\n\t()()()()()()^^^^",
		"label_table (label map_label block)"
		,"^^^^()()()()()()\n");
	for(i = from; i <= to; i++) {
		int index =  LABEL_INDEX(i);
		fprintf(stderr, "\t%d\t%d\t%d\n",
			i,
			label_table[index].map_label,
			label_table[index].block ?
				label_table[index].block->block_count : -1);
	}
}
#endif
