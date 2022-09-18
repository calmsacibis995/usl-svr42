/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/interface.c	1.62"
#include "amigo.h"
#include <stdio.h>
#include <string.h>

#ifndef NODBG
#include "costing.h" /* to allow fiddling costing parameters */
int costing_call_count, nodes_costed_count;
#ifndef __STDC__
		long strtol();
		extern char * getenv();
#endif
#endif

#include <ctype.h>

extern Arena global_arena; /* defined in cg/common/comm2.c */
Arena file_arena = 0;
int do_amigo_flag = 1;

void init_depth_first();
void remove_unreachable();

#ifndef NODBG /* gather timing stats for optimizations */

extern reg_alloc_ran;
unsigned opt_min=0;
unsigned opt_max=(unsigned)4294967295;
unsigned opt_curr;
static unsigned func_min=0;
static unsigned func_max=(unsigned)4294967295;
unsigned func_curr;
struct flag_data aflags[] = {	/*
				** Array is indexed by flag_enum,
				** conditionally defined in debug.h
				*/
	{0,1,"all"},	/* level one debugging for all amigo opts */
	{0,1,"bb"},	/* basic block */
	{0,1,"cf"},	/* constant folding */
	{0,1,"cm"},	/* code motion */
	{0,1,"cp"},	/* copy propagation */
	{0,1,"cse"},	/* local cse */
	{0,1,"cst"},	/* costing */
	{0,1,"df"},	/* data flow */
	{0,1,"ds"},	/* dead store */
	{0,1,"env"},	/* set tunables with getenv */
	{0,1,"exp"},	/* expressions */
	{0,1,"fs"},	/* function stats */
	{0,1,"funcs"},	/* functions to optimize */
	{0,1,"h"},	/* expression hashing */
	{0,1,"jj"},	/* jumps to jumps */
	{0,1,"l"},	/* loop information */
	{0,1,"li"},	/* local information */
	{0,1,"lu"},	/* loop unrolling */
	{0,1,"mem"},	/* memory allocation */
	{0,1,"opts"},	/* optimization instance to do */
	{0,1,"ra"},     /* global register allocation */
	{0,1,"sc"},     /* scopes */
	{0,1,"sr"},	/* strength reduction */
	{0,1,"ta"},	/* trees after optimization */
	{0,1,"tb"},	/* trees before optimization */
	{0,1,"tbcp"},	/* trees before copy propagation */
	{0,1,"tblu"},	/* trees before loop unrolling */
	{0,1,"tbra"},	/* trees before reg_alloc */
	{0,1,"tm"},	/* timing info */
	{0,1,"un"}	/* unreachable code removal */
};

static void
usage() {
	DPRINTF("usage: -G [OPT:OPT .. :OPT:[funcs[=<n1>-<n2>]]:[opts[=<n1>-<n2>]]:[v<n1><n2>\n");
	DPRINTF("\tOPT :- [~] OPTION <level>\n");
	DPRINTF("\tOPTION :- all | bb | cf | cp  | cm | cse | cst | df | ds | \n\t\tenv | exp | fs | h | l | li | lu | mem | ra | sc | sr | ta | tb | tbcp | tblu | tbra | tm | un\n");
	exit(1);
}

	/*
	** Search the environment for tunables.
	*/
static void
get_env()
{
	char *str; 
	char **ptr = (char **)malloc(sizeof(char *));

	str = getenv("SR_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) sr_threshold = (int)val;
		/* overrides default */
	}
	
	str = getenv("REG_SLOPE");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) reg_slope = (int)val;
	}

	str = getenv("REG_HEIGHT");
	if(str) {
		long val = strtol(str,ptr,10);
			if(*ptr != str) reg_height = (int)val;
		}

	str = getenv("LU_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) lu_threshold = (int)val;
	}
	
	str = getenv("LU_L_SIZE_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) lu_l_size_threshold = (int)val;
	}
	
	str = getenv("EXP_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) exp_threshold = (int)val;
	}
	
	str = getenv("CM_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) cm_threshold = (int)val;
	}
	
	str = getenv("CF_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) cf_threshold = (int)val;
	}

	str = getenv("PASS_THRU_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) pass_thru_threshold = (int)val;
	}

	str = getenv("CSE_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) cse_threshold = (int)val;
	}

}

static void 
set_limit(flags, min, max)
char *flags;
unsigned *min, *max;
{
	if (*flags != '=')
		return;
	*min = *max = 0;
	*min = strtol(flags+1, &flags,10);
	*max = strtol(flags+1, &flags,10);
	if (! *min || ! *max)
		usage();
}

static char *
get_name()
{
	static char buffer[100];
	CGQ_FOR_ALL(root, index)
		if (root->cgq_op ==  CGQ_EXPR_ND2 &&
			root->cgq_arg.cgq_nd2->op == DEFNAM) {
			sprintf(buffer,"%s(%d)",root->cgq_arg.cgq_nd2->name,
				func_curr);
			return buffer;
		}
	CGQ_END_FOR_ALL
	sprintf(buffer,"%s(%d)", "NO NAME", func_curr);
	return buffer;
}
#endif
	
void
amigo_flags(flags)
char *flags;
{
#ifndef NODBG /* Currently all amigo flags only for debugging -- PSP */
	char *token;
	for(token = strtok(flags,":"); token; token = strtok((char *)0,":")) {
		char *p = token, c;
		int i, x_flag = 0;
		char **ptr = (char **)malloc(sizeof(char *));
#ifdef MDOPT
		if(*p == 'v') {
		   /* vectorizer option */
		   mdflags(p);
		   continue;
		}
#endif
		if(*p == '~') { /* e.g., ~cse means turn off cse */
			x_flag = 1;
			++p;
			++token;
		}
		while(isalpha(*p)) ++p; /* scan to end of token */
		c = *p;
		*p= 0;
		for(i=0;i<flag_max;i++) {
			if(strcmp(token,aflags[i].flag_name) == 0)
				break;
		}
		if(i == flag_max) {
			usage();
		}
		if(x_flag) {
			aflags[i].xflag = 0;
			continue;
		}
		/* turn on -1a register allocation */
		else if (i == ra)
			++a1debug;
		*p = c;
		if(isdigit(*p)) 
			aflags[i].level = (unsigned)strtol(p,&p,10);
		else 
			aflags[i].level = 1;

		/* Process weird flags: funcs, opts, cst */

		if (i == funcs) 
			set_limit(p, &func_min, &func_max);

		else if (i == opts) 
			set_limit(p, &opt_min, &opt_max);

		else if (i == env) {
			get_env();
		}

		else if (i == all) {
			extern a1debug; /* acomp reg alloc flag (-1a), p1allo.c */
			aflags[cf].level |= aflags[i].level;
			aflags[cm].level |= aflags[i].level;
			aflags[cp].level |= aflags[i].level;
			aflags[cse].level |= aflags[i].level;
			aflags[ds].level |= aflags[i].level;
			aflags[funcs].level |= aflags[i].level;
			aflags[jj].level |= aflags[i].level;
			a1debug = 1;	
			aflags[lu].level |= aflags[i].level;
			aflags[ra].level |= aflags[i].level;
			aflags[sr].level |= aflags[i].level;
			aflags[un].level |= aflags[i].level;
		}
		else if (i == cst) {

			DEBUG(aflags[cst].level&1,	
				("cf_threshold: %d cm_threshold:\
 %d sr_threshold: %d cse_threshold: %d\n",
				cf_threshold, cm_threshold,
				sr_threshold, cse_threshold));
			
		}
	}
	do_amigo_flag = do_amigo = aflags[all].xflag;
#else
	if(*flags == '~' ) do_amigo_flag = do_amigo = 0;	
	else do_amigo_flag = do_amigo = 1;	
#endif
}

/* RMA maybe optimizations return a value for changed then cleanup
   cleanup includes freeing the previous phase arena and allocated the
   next phase arena, and then calling build_expr_info_array, maybe
   we have a macro RUN_OPT(optimization,args). We need a final cleanup.
   The expr_info_array in phase arena, the kills and expr_info in the global
   arena; however, we may run into problems when running an optimiztion and
   find_kills causes a bitvector to expand, then space is lost. By the way
   we need bitvector expansion. One solution is bv extents, one solution is
   to totally reorganize the kills bitvectors when new expressions are added.
*/

Boolean
amigo()
{
	Loop first_loop; /* inner to outer ordering */
	struct Block *top_down_basic;
	int branch_count, max_label, min_label;
	Boolean has_loop; /* Don't do loop opts, unless there are loops */
	int dead_stores;	/* number of objects for dead store */
	int regalloc;		/* number of objects for register alloc */
	Arena reg_arena;

#ifndef NODBG
	unsigned int save_opt = opt_curr;
	costing_call_count = 0, nodes_costed_count = 0;
	++func_curr;
	if (aflags[funcs].level&1 && (func_curr < func_min || func_curr > func_max)) 
		return 0;
#endif
	DEBUG_COND(aflags[mem].level&4,init_high_water());
	DEBUG(aflags[mem].level&2, ("Sbrk before amigo: %x\n", sbrk(0)));

	PUSH_TIME; /* Global AMIGO timer */
      	if(! file_arena) file_arena = arena_init();

	PUSH_TIME;
	hash_exprs(&branch_count, &max_label, &min_label);
	ACCUMULATE_TIME(Hash_exprs);

	DEBUG_UNCOND(if (aflags[cf].xflag))
	{
	PUSH_TIME;
	local_const_folding();
	ACCUMULATE_TIME(Local_const_folding);
	}

	DEBUG_COND(aflags[exp].level&1,print_exprs(aflags[exp].level));

	PUSH_TIME;
	has_loop = false;
	top_down_basic=basic_block(branch_count, max_label, min_label, &has_loop);
	ACCUMULATE_TIME(Top_down_basic);

	DEBUG_COND(aflags[tb].level&1, print_trees((int)aflags[tb].level,top_down_basic));

	DEBUG(aflags[fs].level&1,("before opt: blocks=%d exprs=%d occurs=%d objects=%d\n",
		get_block_count(), get_expr_count(), get_occur_count(),
		get_object_count()));

	PUSH_TIME;
	init_depth_first(top_down_basic);
	ACCUMULATE_TIME(Init_depth_first);

	DEBUG_UNCOND(if (aflags[un].xflag))
	{
	PUSH_TIME;
	remove_unreachable(top_down_basic);
	ACCUMULATE_TIME(Remove_unreachable);
	}

	DEBUG_COND(aflags[li].level&2,print_exprs(aflags[li].level));

	PUSH_TIME;
	local_info(top_down_basic);
	ACCUMULATE_TIME(Local_info);

	if(has_loop) {
		DEBUG_COND(aflags[l].level&128, print_trees((int)aflags[l].level,top_down_basic));
		PUSH_TIME;
		first_loop = loop_build(top_down_basic);
		ACCUMULATE_TIME(Loop_build);

		DEBUG_COND(aflags[bb].level&1,pr_blocks(top_down_basic));

#ifdef MDOPT
		PUSH_TIME;
		/* turn off with -Gv45 */
		(void)MDOPT((Cgq_index)0,
			first_loop,top_down_basic,get_block_count());
		ACCUMULATE_TIME(Vect);
#endif

		DEBUG_UNCOND(if (aflags[cm].xflag))
		{
		PUSH_TIME;
		loop_invariant_code_motion(first_loop);
		ACCUMULATE_TIME(Loop_invariant_code_motion);
		}

		DEBUG_UNCOND(if (aflags[sr].xflag))
		{
		PUSH_TIME;
		strength_reduction(first_loop);
		ACCUMULATE_TIME(Strength_reduction);
		}

		DEBUG_UNCOND(if (aflags[lu].xflag))
		{
		PUSH_TIME;

		DEBUG(aflags[tblu].level,("TREES BEFORE LOOP UNROLLING\n"));
		DEBUG_COND(aflags[tblu].level&3,pr_blocks(top_down_basic));
		DEBUG_COND(aflags[tblu].level&1,
			print_trees((int)aflags[tblu].level,top_down_basic));

		loop_unrolling(first_loop);
		DEBUG_COND(aflags[exp].level&&aflags[lu].level,
			print_exprs(aflags[exp].level));
		init_depth_first(top_down_basic);

		ACCUMULATE_TIME(Loop_unrolling);
		}

	}

#ifndef NODBG
	else
		DEBUG_COND(aflags[bb].level&1,pr_blocks(top_down_basic));
#endif

	DEBUG_UNCOND(if (aflags[cp].xflag))
	{
	DEBUG(aflags[tbcp].level&4,("BLOCKS BEFORE CP\n"));
	DEBUG_COND(aflags[tbcp].level&4,pr_blocks(top_down_basic));
	DEBUG_COND(aflags[tbcp].level&1,
			print_trees((int)aflags[tbcp].level,top_down_basic));

	PUSH_TIME;
	copy_prop(top_down_basic);
	ACCUMULATE_TIME(Copy_prop);

	DEBUG(aflags[tbcp].level&4,("BLOCKS AFTER CP\n"));
	DEBUG_COND(aflags[tbcp].level&4,pr_blocks(top_down_basic));
	DEBUG_COND(aflags[tbcp].level&4,
			print_trees((int)aflags[tbcp].level,top_down_basic));
	}

		/* Print exprs with level 1 detail */
	DEBUG_COND(aflags[exp].level&2,print_exprs(1));

	dead_stores = remap_object_ids(~0);
#ifndef NODBG
		/* Print exprs with level 1 detail */
	DEBUG_COND(aflags[ds].level&2,print_exprs(aflags[ds].level));
	if (!aflags[ds].xflag)
		dead_stores = 0;
#endif
	if (dead_stores) {
		reg_arena = arena_init();
		OBJECT_SET_SIZE = dead_stores;
		get_use_def(reg_arena);
		live_on_entry(OBJECT_SET_SIZE, reg_arena);

		PUSH_TIME;
		dead_store();
		ACCUMULATE_TIME(Dead_store);
		arena_term(reg_arena);
	}

#ifndef NO_CSE
	DEBUG_UNCOND(if (aflags[cse].xflag))
	{
	PUSH_TIME;
	global_cse(top_down_basic);
	ACCUMULATE_TIME(Local_cse);
	}
#endif

	regalloc = remap_object_ids(CAN_BE_IN_REGISTER);
	if (regalloc) {
		reg_arena = arena_init();
		OBJECT_SET_SIZE = regalloc;
		get_use_def(reg_arena);
		live_on_entry(OBJECT_SET_SIZE, reg_arena);
		fixup_live_on_entry();
		
		PUSH_TIME;
		reg_alloc(reg_arena,top_down_basic);
#ifndef NODBG
		ACCUMULATE_TIME(Reg_alloc);
		reg_alloc_ran = 1;
#endif
		arena_term(reg_arena);
	}

	DEBUG_COND(aflags[exp].level&8,print_exprs(8));

	ACCUMULATE_TIME(AMIGO);
	
	DEBUG_COND(aflags[ta].level&1, print_trees((int)aflags[ta].level,top_down_basic));

	DEBUG(aflags[fs].level&~1,
		("after opt: blocks=%d exprs=%d occurs=%d objects=%d\n",
		get_block_count(), get_expr_count(), get_occur_count(),
		get_object_count()));

	DEBUG(aflags[cst].level&2,("costing calls: %d\n", costing_call_count));
	DEBUG(aflags[cst].level&2,("nodes costed: %d\n", nodes_costed_count));

	DEBUG(aflags[funcs].level&1 || aflags[opts].level&1, 
		("func=%s first_opt=%d last_opt=%d\n",
		get_name(), save_opt+1, opt_curr) );
	DEBUG(aflags[mem].level&2, ("Sbrk after amigo: %x\n", sbrk(0)));
	DEBUG(aflags[mem].level&1, ("Extent high water: %d\n", get_high_water()));
	return 1;
}

void
amigo_fatal(msg,file,line)
char *msg, *file;
int line;
{
	fprintf(stderr,"Fatal error: %s at file: %s line: %d\n", msg,file,line);
	fflush(stderr);
	abort();
}

#ifndef NODBG

#include <sys/types.h>
#include <sys/times.h>

static int
get_clock()
{
	struct tms tbuf;
	times(&tbuf);
	return tbuf.tms_utime;
}

#define TIME_MAX 10
static int time_stack[TIME_MAX];
static int *stack_top = time_stack;

void
push_time()
{
	*stack_top = get_clock();
	++stack_top;
	if (stack_top >= time_stack+TIME_MAX)
		cerror("time stack overflow");
}


int
pop_time() {
	--stack_top;
	if (stack_top < time_stack)
		cerror("time stack underflow");
	return get_clock() - *stack_top;
}
	
int cumulative_times[Max_stats];

static char * timings_strngs[] = { /* indexed by enum timings in debug.h */
	"AMIGO",
	"Aux1",
	"Aux2",
	"Copy_prop",
	"Dead_store",
	"Hash_exprs",
	"Init_depth_first",
	"Jumps_to_jumps",
	"Local_const_folding",
	"Local_cse",
	"Local_info",
	"Loop_build",
	"Loop_invariant_code_motion",
	"Loop_unrolling",
	"Reg_alloc",
	"Remove_unreachable",
	"Strength_reduction",
	"Top_down_basic",
	"Vect",
	"Max_stats"
};

void
amigo_time_print()
{ 
	int i;
	if(aflags[tm].level & 1) {
		if (stack_top != time_stack)
			cerror("time stack not empty");
		for(i = 0; i < Max_stats; i++) fprintf(stderr, "Time %s: %d\n",
			timings_strngs[i], cumulative_times[i]);
	}
}
#endif
