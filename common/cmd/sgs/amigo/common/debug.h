/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/debug.h	1.28"
extern void PROTO(		pr_object_list,(Object_list));
extern Object PROTO(	debug_object,(Setid));
extern void PROTO(	pr_object_set,(Object_set));
extern void PROTO(	pr_object,(Object));
extern void PROTO(	loop_debug,(void));
extern void PROTO(	scope_debug,(Scope));
extern void PROTO(	scope_usage_debug,(char *, Object));
extern int PROTO(	debug_scope_id,(Scope));
extern void PROTO(	print_expr,(Expr));
extern void PROTO(	print_exprs,(int));
extern void PROTO(	print_trees, (int level, Block top_down_basic));
extern void PROTO(	dprint1,(ND1 *));
extern void PROTO(	pr_blocks,(Block));
extern void PROTO(	pr_block_list,(Block_list, char *msg));
extern void PROTO(	push_time,(void));
extern int PROTO(	pop_time,(void));
extern int PROTO(	get_high_water,(void));
extern void PROTO(	init_high_water,(void));
extern void PROTO(	pr_label_table,(int from, int to));

extern unsigned opt_curr;
extern unsigned opt_min;
extern unsigned opt_max;
#define START_OPT {\
	++opt_curr; \
	if (opt_curr <= opt_max && opt_curr >= opt_min){ \
		DEBUG(aflags[opts].level,("\topt=%d\n", opt_curr)); {
#define STOP_OPT }}}
#define DEBUG_COND(cond,state) if(cond) state
#define DEBUG_UNCOND(state) state

#define OBJ_TO_NAME(obj) (\
((obj)->fe_numb)!=SY_NOSYM ? SY_NAME((obj)->fe_numb) : "")


enum flag_enum {
	all, 	/* (at least) level 1 debugging for all basic optimiztions */
	bb,	/* basic blocks */
	cf,	/* constant folding */
	cm,	/* code motion */
	cp,	/* copy propagation */
	cse,	/* local cse */
	cst,	/* debug costing */
	df,	/* data flow */
	ds,	/* dead store */
	env,	/* set tunables from environment */
	exp,	/* expr information */
	fs,	/* function stats */
	funcs,	/* limit functions optimized */
	h,	/* expr hashing */
	jj,	/* jump to jump */
	l,	/* loop information */
	li,	/* local information */
	lu,	/* loop unrolling */
	mem,    /* memory allocation */
	opts,	/* limit optimization occurrences */
	ra,     /* global register allocation */
	sc,	/* scopes */
	sr,	/* strength reduction */
	ta, tb,	tbcp, tblu, tbra,	/*
				** trees after,before optimization, 
				** before copy propagation,
				** before loop unrolling, before reg_alloc
				*/
	tm,	/* timing */
	un,	/* unreachable code elimination */
	flag_max
};

extern struct flag_data {
	unsigned level;
	int xflag;
	char *flag_name;
} aflags[]; /* Array is indexed by flag_enum */

#define PUSH_TIME push_time()
#define POP_TIME pop_time()
extern int cumulative_times[];
#define ACCUMULATE_TIME(x) cumulative_times[x] += POP_TIME

enum timings {
	AMIGO,
	Aux1,
	Aux2,
	Copy_prop,
	Dead_store,
	Hash_exprs,
	Init_depth_first,
	Jumps_to_jumps,
	Local_const_folding,
	Local_cse,
	Local_info,
	Loop_build,
	Loop_invariant_code_motion,
	Loop_unrolling,
	Reg_alloc,
	Remove_unreachable,
	Strength_reduction,
	Top_down_basic,
	Vect,
	Max_stats
}; /* list of things we want to time */

extern costing_call_count, nodes_costed_count;
