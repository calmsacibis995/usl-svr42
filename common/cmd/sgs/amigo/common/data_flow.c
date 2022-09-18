/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/data_flow.c	1.12"
#include "amigo.h"
#define ALLOCATE 1
#ifndef NODBBG
static  void PROTO(	debug_antic,(void));
static  void PROTO(	debug_avail_in,(void));
static  void PROTO(	debug_live_on_entry,(Arena));
#endif

	/* Solve globally for ANTICIP */
void
anticipate(arena) 
Arena arena;
{
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Expr_set temp_set = Expr_set_alloc(arena);

	REVERSE_DEPTH_FIRST(blk_ptr)

		Block block;

		block = *blk_ptr;
		block->antic = Expr_set_alloc(arena);
		block_expr_kills(block,ALLOCATE);
		if(block->succ)	
			bv_init(true,block->antic);
		else
			bv_assign(block->antic, block->loc_antic);

	END_DEPTH_FIRST

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Anticipate data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[df].level&2, debug_antic());

		changed = false;

		REVERSE_DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list successor;
				/* init to PRESERVES maybe should store
				   in block, instead of exprs_killed.
				   Could also speed up the algorithm
				   by storing a change flag in each block
				   and checking for at least one changed
				   successor, rather than simply checking
				   for at least one successor -- PSP */
			successor = block->succ;
			if(successor) {
				bv_init(true,temp_set);
				bv_minus_eq(temp_set,block->exprs_killed);

				do {
					if(block->block_count !=
						successor->block->block_count)
						/* only follow out edges! */	
						bv_and_eq(temp_set,successor->block->antic);
					successor = successor->next;
				} while(successor);

				bv_or_eq(temp_set,block->loc_antic);

				if(! bv_equal(temp_set,block->antic)) {
					Expr_set temp;
					DEBUG(aflags[df].level&2,
						("Changed antic in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[df].level&2,
						bv_print(temp_set));
					DEBUG(aflags[df].level&2,("\told:"));
					DEBUG_COND(aflags[df].level&2,
						bv_print(block->antic));

					temp = block->antic;
					block->antic = temp_set;
					temp_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,("Final anticipate data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_antic());
}

	/* Solve globally for AVIN ( assignments available on entry ) */
void
avail_in(arena) 
Arena arena;
{
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Assign_set temp1_set = Assign_set_alloc(arena);
	Assign_set temp2_set = Assign_set_alloc(arena);

	DEPTH_FIRST(blk_ptr)

		Block block = *blk_ptr;
		block->avin = Assign_set_alloc(arena);
		if(block->pred)
			bv_init(true,block->avin);	
		else
			bv_init(false,block->avin);
	END_DEPTH_FIRST

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Avail_in data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[df].level&2, debug_avail_in());

		changed = false;

		DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list predecessor;

			predecessor = block->pred;
			if(predecessor) {
				bv_init(true,temp2_set);

				do {
					bv_assign(temp1_set,
						predecessor->block->avin);
					bv_minus_eq(temp1_set,
						predecessor->block->assigns_killed);
					bv_or_eq(temp1_set,
						predecessor->block->reaches_exit);
					bv_and_eq(temp2_set,temp1_set);
					
					predecessor = predecessor->next;
				} while(predecessor);

				if(! bv_equal(temp2_set,block->avin)) {
					Expr_set temp;
					DEBUG(aflags[df].level&2,
						("Changed avin in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[df].level&2,
						bv_print(temp1_set));
					DEBUG(aflags[df].level&2,("\told:"));
					DEBUG_COND(aflags[df].level&2,
						bv_print(block->avin));

						/*
						** Switch pointers, rather
						** than copying bitvectors
						*/
					temp = block->avin;
					block->avin = temp2_set;
					temp2_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,("Final avail_in data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_avail_in());
}

	/*
	** Solve live on entry data flow equations for the
   	** Register Allocator.  This should be converted to always use
   	** Object_sets, in which case the first parameter should be eliminated
 	** and all bv_alloc() calls changed to Object_set_alloc()
	** -- PSP
	*/

void
live_at_bottom(bv_leng, arena) 
int bv_leng;
Arena arena; /* Arena in which the live_on_entry field will persist. */
{
        REVERSE_DEPTH_FIRST(block_ptr)
		int init = 0;
		Block block = *block_ptr;
		Block_list s;
		block->live_at_bottom = bv_alloc(bv_leng, arena);
		if (!block->succ) {
			bv_init(false,block->live_at_bottom);
			continue;
		}
		for (s= block->succ; s; s= s->next) {
			Block succ = s->block;
			if (!init) {
				init = 1;
				bv_assign(block->live_at_bottom, 
				    succ->live_on_entry);
			}
			else
				bv_or_eq(block->live_at_bottom,
				    succ->live_on_entry);
		}
        END_DEPTH_FIRST
}

void
live_on_entry(bv_length, arena) 
int bv_length;
Arena arena; /* Arena in which the live_on_entry field will persist. */
{
        /* see Aho & Ullman dragon book */
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Bit_vector temp_set = bv_alloc(bv_length, arena);

	changed = true;

		/*
		** Allocate live_on_entry.  This code should
		** be moved out to init code if the live_on_entry
		** ends up being solved more than once in the arena.
		*/

	DEPTH_FIRST(blk_ptr)

		Block block = *blk_ptr;
		block->live_on_entry = bv_alloc(bv_length, arena);
		bv_assign(block->live_on_entry, block->use);

	END_DEPTH_FIRST

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Live_on_entry data_flow before pass %d:\n", pass));
			DEBUG_COND(aflags[df].level&2, debug_live_on_entry(arena));

		changed = false;

		REVERSE_DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list successor;
			successor = block->succ;
			if(successor) {
				bv_init(false,temp_set);

				do {
				   bv_or_eq(temp_set,successor->block->live_on_entry);
				   successor = successor->next;
				} while(successor);
				bv_minus_eq(temp_set,block->def);
				bv_or_eq(temp_set,block->use);

				if(! bv_equal(temp_set,block->live_on_entry)) {
					bv_assign(block->live_on_entry,temp_set);
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,
		("Final live_on_entry data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_live_on_entry(arena));
}

void
fixup_live_on_entry() {
/* sets the sizes of live_on_entry sets to OBJECT_SET_SIZE */
	DEPTH_FIRST(blk_ptr)
		Block block;
		block = *blk_ptr;
		bv_set_size(OBJECT_SET_SIZE,
			(Bit_vector)(block->live_on_entry));
	END_DEPTH_FIRST
}

#ifndef NODBG
static void
debug_antic()
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tantic: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->antic));
		DEBUG(aflags[df].level,("\tlocally_antic: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->loc_antic));
		DEBUG(aflags[df].level,("\texprs_killed: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->exprs_killed));
	END_DEPTH_FIRST
}

static void
debug_avail_in()
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tavin: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->avin));
		DEBUG(aflags[df].level,("\treaches_exit: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->reaches_exit));
		DEBUG(aflags[df].level,("\tassigns_killed: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->assigns_killed));
		pr_block_list((*block_ptr)->pred,"\tpreds");
		pr_block_list((*block_ptr)->succ," succs");
		fprintf(stderr,"\n");
		fflush(stderr);
	END_DEPTH_FIRST
}

static void
debug_live_on_entry(arena)
Arena arena;
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tlive_on_entry: "));
		DEBUG_COND(aflags[df].level,
			pr_object_set((*block_ptr)->live_on_entry));
		DEBUG(aflags[df].level,("\tuse: "));
		DEBUG_COND(aflags[df].level,pr_object_set((*block_ptr)->use));
		DEBUG(aflags[df].level,("\tdef: "));
		DEBUG_COND(aflags[df].level,pr_object_set((*block_ptr)->def));
	END_DEPTH_FIRST
}

#endif
