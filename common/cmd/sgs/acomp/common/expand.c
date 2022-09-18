/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/expand.c	1.18"
/* expand.c */

/* This module implements inline expansion. The parser calls inline_begf
   at the beginning of each function, and calls inline_endf at the end of
   each function that can be optimized. After each function has been parsed
   the parser calls inline_eof.

   Inline expansion proceeds in three major phases.
	1. The construction of the call graph.

	2. Construction of a call tree that indictes which calls to expand.

	3. The inline expansions
*/

#ifndef NO_AMIGO

/* RMA need an initialization routine to initialize inline_arena and
   call_graph_arena
*/

/* RMA declare in cgstuff.h */
extern void cg_putq_between();

#include "p1.h"
#include "mfile2.h"
#include "arena.h"
#include "bitvector.h"


/* should be exported by arena.h */
extern Arena global_arena;
int do_inline = 0; /* For now default is NO inlining */

/* space growth limited to a factor of 1/IL_EXPANSION */
#define IL_EXPANSION 5

/* Used to map two numbers to one number VALUE_BITS long.
   The first number is shifted left by LOW_ORDER then added to the second 
   number.  The maximum value of the first number if MAX_HIGH. The maximum 
   value of the second number is MAX_LOW.
*/
#define VALUE_BITS 16
#define HIGH_ORDER 3
#define LOW_ORDER (VALUE_BITS-HIGH_ORDER)
#define MAX_HIGH ((1<<HIGH_ORDER) - 1)
#define MAX_LOW ((1<<LOW_ORDER) - 1)

typedef int Call_site;
typedef struct Funct *Function;

/* following used for args  */
#define ARG_CONST 1
#define ARG_KNOWN_PTR 2
#define ARG_COPY_PROP_CAND 4

/* value for Call_list flags */
#define DELETE_ON_EXPAND 1
#define DELETED 2
#define NOT_PRECOMPUTE 4

/* maximum value of f_value field */
#define MAX_VALUE 1<<30

struct Call_list {
				/* RMA make following shorts */
	int flags;
	int order;		/* used to sequence calls */
	int value;		/* benefit of expansion, weighted by size */
	int f_value;		/* value weighted by frequency */
	int size;		/* inverse measure of size of callee */
	int frequency;		/* how often function is called */
	Function func;		/* called function */
	Call_site call_site;
	int arg_count;
	char *args;		/* argument descriptors */
	T1WORD *types;		/* argument types */
	struct Call_list *next;
};

/* following used in formals flag */
#define PARAM_READ_ONLY 1
#define PARAM_OP_CONST 2

/* following used in summary flags */
#define CONTAINS_LOOP	01
#define LEAF_FUNCTION	02
struct Summary {
	struct Call_list *call_list;
	int function_count;
	int node_count;
	int formal_count;
	int flags;
	char *formals;		/* an array of flags for formals */
	T1WORD *types;		/* an array of formal types */
};

struct Call_table_list {
	ND1 *node;
	Cgq_index index;
	struct Call_table_list *next;
};

/* flags for FUNC */
#define GENERATED 1	/* code already generated */
#define DEFINED 2	/* have definition */
#define CALLED 4	/* target edge in call graph */
#define CALLED_ONCE 8
#define NODELETE 32	/* cannot be deleted (address has been taken) 
			   or there exists a non-optimizable function
			*/

struct Funct {
	int flags;
	Cgq_index save_index;	/* last index for cgq */
	char *name;		/* function name */
	Arena save_arena;
	struct td save_cgq;
	SX sid;
	struct Summary summary;
	struct Funct *next;

	/* root of call_tree for this function */
	struct Call_tree_list *call_tree;

	struct Call_list *unique_call;	/* one call to this function */
	struct Funct *unique_caller;	/* caller of single call */

	/* following used to construct tables during open */

	/* min is lowest sid of auto or param, max is highest sid of auto
	   or param + 1, we assume that args and autos declared after
	   function, also ties us into a particular implementation of
	   symbol table
	*/
	int min_symbol, max_symbol;

	/* min is lowest label number, max is highest + 1 */
	/* used to map labels */
	int min_label, max_label;

	int expr_count;

	/* used to construct Call_table when function is opened */
	struct Call_table_list *call_table_list;
};

struct Call_table {
	Cgq_index index;
	ND1 *node;
};

/* flags for Funct_copy */ 
#define EDITING 1	/* changing function defintion */
#define EDIT_COPY 2	/* changing function definition after making copy */
#define INLINING 0	/* copying function into caller */

struct Funct_copy {
	Arena arena;
	int flags;
	struct Funct *func;
	struct Call_table *call_table;
	int *label_table;
	SX *symbol_table;
	Cgq_index next_index;	/* used for reading */
};


/* datastructure for forest of call trees */
struct Call_tree_list {
	int open_flags;		/* same flags as Func_copy */
	struct Call_tree *tree;
	struct Call_tree_list *next;
};

struct Call_tree  {
	struct Funct *func;
	struct Tree_edge *edges;
};

struct Tree_edge {
#ifndef NODBG
	int flags;		/* Call_list flags */		
#endif
	struct Call_tree *callee;
	Call_site site;
	int order;		/* expansion order */
#ifndef NODBG
	int count;
#endif
	struct  Tree_edge *next;
};

/* used by get_precompute */
#define LEFT 0
#define RIGHT 1

static Function functions;
static Arena inline_arena;
static Arena call_graph_arena;
static int func_cnt;
static Function curr_func;
static int nodes_used;		/* debugging only */
static int node_count = 0;
static int all_processed = 1;	/* every function in file processed */
static struct Funct *last_begf = 0;



static void make_summary();
static int get_precompute();
static int read_cgq_item();
static void make_call();
static void expand_calls();
static Cgq_index extend_scope();
static struct Tree_edge * add_edge();
static Function get_function();
static int process_node();
static int process_args();
static int make_value();
static int eval_value();
static struct Expansion_sites *make_site();
static void push_site();
static int get_pointer() ;
static void process_deletes();
static NODE *copy_tree();
static void restore();
static void enter_expansion();
static void copy();
static Function filter();
static void expand_call_site_tables();
static Call_site mark_call_site();
static int translate_label();
static SX translate_symbol();
static void sort_call_list();
static ND1 *next_argument();
static struct Call_table * get_call_site();
static struct Call_tree_list * top_down_order();
static ND1 * make_assign();
static struct Call_tree_list * make_call_trees();
static struct Call_tree_list * order_trees(); 
static void close_function();
static struct Funct_copy * open_function();
static void inline();
static struct Summary * get_summary();

	/* Defines for debugging follow. To set one or more debugging
	   options add the values for the options obtaining x, and then
	   pass -Ix to acomp. -I0 turns of debugging.

	   If a function contains inlining, and that function is not
	   processed by amigo, because of the -Gfuncs AMIGO's debugging option,
	   then the generated code will be bad.
	*/

/* indicates expansion sites and function deletions. Prints cgq_item containing
   call
*/
#define DEXPAND 2

/* prints the cgq after each function is parsed (not real useful) */
#define DCGQ 4 

/* same as DEXPAND, but prints expansion and cgq_item containing call after 
   expansion
*/ 
#define DEXPAND1 8

/* prints the call graph */
#define DGRAPH	16

/* forces the inliner to only save 4 functions at a time, used to debug
   code that flushes out saved functions
*/
#define DFLUSH 32

/* Indicates the expansion sites. Also used to turn on and off selected
   inlining. However, if h calls f calls g. You cannot turn off "h calls f"
   without turning off "f calls g". Sites are numbered in the output.

   To inline sites 1,2,3,4,7,10, for example. Set the environment variable
   DCOUNT="1 2 3 4 7 10", and run acomp: acomp -I64
*/
#define DCOUNT 64

/* prints out the costs as an expansion is selected. Should be combined with
   DCGQ
*/
#define DBUILD_TREE 128

/* limits functions saved for inlining */
#define NODE_MAX 5000
#define FUNC_MAX 100


/* cannot delete a function whose address has been taken in the
   initialization of a static
*/
void
inline_address_function(func)
SX func;
{
	get_function(func)->flags |= NODELETE;
}

void
inline_flags(arg)
char *arg;
{
	static int zero_seen;
	int flags;
	if (zero_seen)
		return;
	flags = atoi(arg);
	if (flags == 0) {
		zero_seen = 1;
		do_inline = 0;
	}
	else
		do_inline |= flags;
}

#ifndef NODBG
static int 
	il_leaf_only = 0,	/* only inline leaf functions */
	il_copy_only = 0,	/* only inline funcs with NAME/ICON params */
	il_read_only = 0,	/* as above, but read only params */
	il_no_loop_only = 0;	/* only inline functions with no loops */
#endif

int inline_expansion = IL_EXPANSION;	/* limit growth to 1/inline_expansion */
	
void
inline_begf(sid) SX sid; {

   	if (!inline_arena) {
		inline_arena = arena_init();
		call_graph_arena = arena_init();
#ifndef NODBG
		{
			char *arg;
			if ((arg = getenv("IL_LEAF_ONLY")) != NULL)
				il_leaf_only = atoi(arg);
			if ((arg = getenv("IL_COPY_ONLY")) != NULL)
				il_copy_only = atoi(arg);
			if ((arg = getenv("IL_READ_ONLY")) != NULL)
				il_read_only = atoi(arg);
			if ((arg = getenv("IL_NO_LOOP_ONLY")) != NULL)
				il_no_loop_only = atoi(arg);
		}
#endif
	}

	/* don't see the endf if function not optimizable */
	else if ((!last_begf) || !(last_begf->flags&DEFINED))
		all_processed = 0;
	last_begf = get_function(sid);

	/* used for making label and symbol translation tables */
	last_begf->min_label = getlab() + 1;
	last_begf->min_symbol= sid - TY_NPARAM(SY_TYPE(sid));
}
void
inline_endf(sid)
SX sid;
{
    Function new_func;
    ++func_cnt;
    new_func = get_function(sid);
    new_func->flags |= DEFINED;
    new_func->save_index = cg_inline_endf();
    new_func->save_cgq = td_cgq;
    new_func->sid = sid;
    new_func->save_arena = global_arena;

    /* used for making label and symbol translation tables */
    new_func->max_label = getlab();
    new_func->max_symbol = SY_MAX+1;		/* SY_MAX is id of last symbol*/

    curr_func = new_func;
    make_summary(sid, &new_func->summary);
    node_count += new_func->summary.node_count;

#ifndef NODBG
    if (do_inline & DCGQ) {
	DPRINTF("\t\tputting CGQ for %s(%d)\n\n", SY_NAME(sid), sid);
	cg_putq(3); 
    }
#endif
    tinit();	/* get new arena */

#ifndef NODBG
	if (do_inline & DFLUSH) {
		static int flush_count;
		++flush_count;
		if (flush_count == 4) {
			flush_count = 0;
			func_cnt = FUNC_MAX + 1;
		}

	}
#endif

	/* if exceed maximum function saving, then inline expand what we
	   have and start saving functions again
	*/
	if (node_count > NODE_MAX || func_cnt > FUNC_MAX) {
		all_processed = 0;
		inline_eof();
		func_cnt = 0;
		functions = 0;
		last_begf = 0;
		functions = 0;
		arena_term(inline_arena);
		inline_arena = 0;
		node_count = 0;
#ifndef NODBG
		nodes_used = 0;
		if (do_inline > 1)
			DPRINTF("restarting inlining\n");
#endif
	}
}

/****************************************************************************/
/****************************************************************************/
/*********************** make_summary pass **********************************/
/****************************************************************************/
/****************************************************************************/

/* make_summary records the size of the function, information about its 
   parameters, and information about its calls
*/
static void
make_summary(sid,summary)
SX sid;
struct Summary *summary;
{
	T1WORD type = SY_TYPE(sid);
	int nparam = TY_NPARAM(type);
	int cannot_be_called = 0;
	T1WORD *types;
	
	int i;
	int depth=0;	/* loop nesting depth */

	/* temporary hack until ty_nparam is fixed */
	if (nparam == -1)
		cannot_be_called = 1;

	if (TY_ISVARARG(type) || TY_ISSU(TY_DECREF(type)))
		cannot_be_called = 1;

	if (cannot_be_called == 0) {
		summary->types=types=Arena_alloc(global_arena, nparam, T1WORD);
		for (i=0; i < nparam; ++i) {
			types[i] = TY_PROPRM(type,i);
			if (TY_ISSU(types[i]))
				cannot_be_called = 1;
		}
	}
	if (cannot_be_called)
		summary->formal_count = -1;
	else {
		summary->formal_count = nparam;
		summary->formals = Arena_alloc(global_arena, nparam, char);
		memset(summary->formals, PARAM_READ_ONLY, (unsigned)nparam);
	}

	summary->node_count = 0;
	summary->call_list = 0;
	summary->flags = LEAF_FUNCTION;

	i = 0;
	CGQ_FOR_ALL(cgq,index)
		switch(cgq->cgq_op) {
		case CGQ_CALL_SID:
			if (cgq->cgq_func == cg_copyprm) {
				/* map param symbols to an index into args
				   and types arrays for the function
				*/
				SY_AUX(cgq->cgq_arg.cgq_sid).arg = i;
				++i;
			}
			break;
		case CGQ_CALL_INT:
			if (cgq->cgq_func == os_loop) {
				summary->flags |= CONTAINS_LOOP;
				if (cgq->cgq_arg.cgq_int == OI_LBODY)
					++depth;
				else if (cgq->cgq_arg.cgq_int == OI_LEND)
					--depth;
			}
			break;
		case CGQ_EXPR_ND2:

			if (cgq->cgq_arg.cgq_nd2->op == DEFNAM &&
			   (cgq->cgq_arg.cgq_nd2->lval&FUNCT) == 0) {
				/* no expansion if encountered a function
				   static definition
				*/
				summary->formal_count = -1;
			}
			break;
		case CGQ_EXPR_ND1:
			(void)process_node(cgq->cgq_arg.cgq_nd1,index,summary,depth,0, 0);
			break;
		case CGQ_FIX_SWBEG:
			(void)process_node((ND1 *)cgq->cgq_arg.cgq_nd2->left,
			    index,summary,depth,0,0);
			break;
		}
	CGQ_END_FOR_ALL
}

#define IS_PARAM(node) \
	(((node)->op == NAME && \
	(node)->rval > 0 && \
	SY_CLASS((node)->rval) == SC_PARAM) \
		? SY_AUX((node)->rval).arg : -1)

/* This routine analyzes a function and the uses of its formals.
   It also records calls from this function.
*/

#define PROCESS(side) \
	order = process_node(side == LEFT ? node->left : node->right \
		,index,summary, depth,order, get_precompute(no_pre, side,node))
static int
process_node(node, index, summary,depth,order,no_pre)
ND1 *node;
struct Summary *summary;
Cgq_index index;
int depth;	/* loop nesting */
int order;	/* depth first ordering */
int no_pre;	/* node cannot be pre-computed */
{
	int param;

	if (optype(node->op) != LTYPE) {
		if (callop(node->op)) {
			summary->flags &= ~LEAF_FUNCTION;

			/* Must not call process_node for left child of
			** call, since this code will think the function
			** address has escaped. See ICON case below.
			*/
			if (node->left->op == ICON)
				++summary->node_count;
			else
				order = PROCESS(LEFT);
		}
		else
			order = PROCESS(LEFT);
	}
	if (optype(node->op) == BITYPE)
		order = PROCESS(RIGHT);

	switch(node->op) {
	case ICON:
		if (node->rval > 0 && TY_ISFTN(SY_TYPE(node->rval)))
			/* cannot delete a function if address taken, since 
			   the function may be called indirectly
			*/
			get_function(node->rval)->flags |= NODELETE;
		break;
	case STCALL: case UNARY STCALL:
	case CALL: case UNARY CALL:
		if (node->left->op==ICON && node->left->rval>0) {
			struct Call_list *new_call = 
			    Arena_alloc(inline_arena,1,struct Call_list);
			new_call->next = summary->call_list;
			summary->call_list = new_call;
			new_call->func = get_function(node->left->rval);
			if (no_pre) {
				new_call->flags = NOT_PRECOMPUTE;
				break;
			}
			else
				new_call->flags = 0;

			new_call->call_site = mark_call_site(node,index);
			new_call->order = order;
			new_call->frequency = depth;
			if (node->op == CALL) 
				new_call->arg_count =
			    	process_args(new_call,node->right,0);
			else
				new_call->arg_count = 0;
			++order;
		}
		break;
	case UNARY AND:
	ASSIGN_CASES:
		
		if (summary->formal_count != -1 &&(param= IS_PARAM(node->left)) >=0 )
			summary->formals[param] &= ~PARAM_READ_ONLY;
	
		break;
	case LS: case RS: case DIV: case MOD: case PLUS: case MINUS: case MUL:
	case EQ: case NE: case LE: case LT: case GE: case GT:
	case ULE: case ULT: case UGE: case UGT:
		if (summary->formal_count == -1)
			break;

		/* comparison of parameter to a constant is recorded */
		if (((node->left->op == ICON || node->left->op == FCON) 
		    && (param=IS_PARAM(node->right)) >=0) ||
		    ((node->right->op == ICON || node->right->op == FCON)
		    && (param=IS_PARAM(node->left)) >=0))

		    summary->formals[param] |= PARAM_OP_CONST;

		break;
	}
	++summary->node_count;
	return order;
}

#define KNOWN_PTR 1
#define INT_TYPE 2
#define BAD_TYPE 3

/* analyzes an argument to a function call as to whether it is a pointer
   to a known variable, it is a constant, or it is a copy propagation 
   candidate 
*/
static void
process_arg(call_list, node, arg_cnt) 
struct Call_list *call_list;
ND1 *node;
int arg_cnt;
{
	call_list->types[arg_cnt] = node->type;
	if (TY_ISPTR(node->type)) {
		if (get_pointer(node) == KNOWN_PTR)
			call_list->args[arg_cnt] = ARG_KNOWN_PTR;
		else
			call_list->args[arg_cnt] = ARG_KNOWN_PTR;
	}
	else if (node->op == ICON)
		 call_list->args[arg_cnt] = ARG_CONST;
	else if (node->op == FCON)
		 call_list->args[arg_cnt] = ARG_CONST;
	else if (node->op == NAME)
		/* COPY_PROP on const's? */
		 call_list->args[arg_cnt] = ARG_COPY_PROP_CAND;
	else
		 call_list->args[arg_cnt] = 0;
		
}
/* If there are n arguments and n > 1, then process_args calls itself for
   the first n-1 arguments, and then calls process_arg for the last argument.
   Returns the number of arguments. 

   Input looks like following:
			       CM
      			     .      .

			CM		ARG (n'th arg)
	             .      .

		CM		ARG(n-1'th arg)
	    .

	CM

    Last CM looks like:
				CM
			     .       .
			ARG		ARG
	
*/

static int
process_args(call_list, args, depth)
struct Call_list *call_list;
ND1 *args ;
int depth;	/* depth in argument tree */
{
	/* returns numer of arguments below node */
	int arg_cnt;
	if (args->op == CM) {
		arg_cnt=process_args(call_list,args->left,depth+1);
		process_arg(call_list,args->right->left,arg_cnt);
		return arg_cnt+1;
	}
	else {	/* args->op == ARG */
		call_list->args = Arena_alloc(global_arena, depth+1,char);
		call_list->types = Arena_alloc(global_arena, depth+1,T1WORD);
		process_arg(call_list, args->left, 0);
		return 1;
	}
}

/* indicates whether the left or right child of a node can be evaluated into
    a temp before the other child is evaluted
*/
static int
get_precompute(no_pre, left_right,node)
int no_pre; 	/* if the node itself can be precomputed */
int left_right;
ND1 *node;
{
	switch(node->op) {
	case COLON:
		return 1;
	case ANDAND:
	case OROR:
	case COMOP:
		if (left_right == RIGHT)
			return 1;
	/* FALLTHRU */
	default:
		return no_pre;
	}
}

/* Determines whether the optimizer can statically determine a pointer targ 
   Will recognize: &name, ICON which is an address constant, and known pointer
   + integer
*/
static int
get_pointer(node) 
ND1 *node;
{
	int ltype, rtype;
	switch(node->op) {
	case UNARY AND:
		if (node->left->op == NAME)
			return KNOWN_PTR;
		else
			return BAD_TYPE;
	case ICON:
		if (node->rval !=0)
			return KNOWN_PTR;
		else
			return INT_TYPE;
	case PLUS: case MINUS:
		ltype = get_pointer(node->left);
		rtype = get_pointer(node->right);
		if (ltype == KNOWN_PTR && rtype == INT_TYPE)
			return KNOWN_PTR;
		else if (rtype == KNOWN_PTR && ltype == INT_TYPE)
			return KNOWN_PTR;
		else if (rtype == INT_TYPE && ltype == INT_TYPE)
			return INT_TYPE;
		else
			return BAD_TYPE;
	
	default:
		if (TY_ISINTTYPE(node->type))
			return INT_TYPE;
		else
			return BAD_TYPE;
	}
}

/* finds the function structure associated with a symbol index, by looking
   a sequential search through the function structures for the name. Cannot
   simple store a pointer to function structure in symbol table because of
   SY_SAMEAS
*/
static Function
get_function(sid)
SX sid;
{
	char *name = SY_NAME(sid);
	Function new_func;
	Function f;

	/* point symbol table at function? */
	for (f=functions; f; f = f->next) {
		if (f->name == name)
			break;
	}
	if (f)
		return f;

   	if (!inline_arena) {
		inline_arena = arena_init();
		call_graph_arena = arena_init();
	}

    	new_func = Arena_alloc(inline_arena,1, struct Funct);
	new_func->expr_count = 0;
	new_func->call_table_list = 0;
    	new_func->flags = 0;
    	new_func->next = functions;
	new_func->name = name;
	new_func->call_tree = 0;
	new_func->sid = sid;
    	new_func->summary.call_list = 0;
	new_func->summary.node_count = -1;
    	functions = new_func;
    	return new_func;
}

/* Records call site information for the current function. Returns an integer
   used to access this information. If the current function is inlined, the
   call site information changes since the function is copied. The integer is
   stored in the call site to facilitate this change.
*/
static Call_site
mark_call_site(node,index) ND1* node; Cgq_index index;
{
	struct Call_table_list *call_table_list= Arena_alloc(inline_arena,
	    1, struct Call_table_list);
	call_table_list->node = node;
	call_table_list->index = index;
	call_table_list->next = curr_func->call_table_list;
	curr_func->call_table_list = call_table_list;
	++curr_func->expr_count;

	node->opt = (struct Expr_info *) curr_func->expr_count;

	return curr_func->expr_count;
}



/****************************************************************************/
/****************************************************************************/
/******************** driver to make call trees and then expand *************/
/****************************************************************************/
/****************************************************************************/

/* sort_call_list sorts the edges of the call graph constructed by make_summary
   by the benefit of expanding the call. If every function in the file is 
   optimizable, and therefore has been processed by make_summary, then 
   process_deletes deletes any static functions that are not referenced in the 
   call_graph and have not had its address taken. static functions that are
   called a single time are marked for deletion when they are expanded.

   For each function in the file, make_call_trees, indicates the calls that
   are to be expanded. Expanded calls that in turn contain expanded calls
   are indicated in the call_tree. order_trees attempts to order the call_trees
   so that if the call tree for f contains a call to g, then f's call tree
   is processed before g's call tree, since processing g's call tree changes
   g's defintion. With recursive calls, a call to a g will be processed after
   starting to process g's call tree, in this case g must be copied before
   processing its call tree.

   expand_calls performs the expansions for the call_trees for each function.
   After the expansions, code is generated for the function. After the
   calls to expand_calls, code is generated for those functions that contain
   no expanded calls.

*/

void
inline_eof() {
	Function f = functions;
	struct Call_tree_list *call_tree;

	if (!last_begf)
		return;

	if (!(last_begf->flags&DEFINED))
		all_processed = 0;

	functions = filter(functions);

	for (f=functions; f; f = f->next) 
		sort_call_list(f);

	if (all_processed) {
		process_deletes(functions);
		functions = filter(functions);
	}

#ifndef NODBG
	if (do_inline&DGRAPH) {
		static void pr_graph();
		pr_graph();
	}
				
#endif
	call_tree = make_call_trees(functions);
	arena_term(call_graph_arena);
	call_tree = order_trees(call_tree);
#ifndef NODBG
	if (do_inline&(DEXPAND|DEXPAND1)) 
		DPRINTF("total nodes=%d expansion limit=%d\n",
			node_count, node_count/inline_expansion);
#endif
	for ( ; call_tree; call_tree = call_tree->next) {
		struct Call_tree *caller = call_tree->tree;
		struct Funct_copy *fc; 
		if (caller->edges == 0)
			continue;
		fc = open_function(caller->func, call_tree->open_flags);
		expand_calls(fc, caller->edges,0);
		close_function(fc);	/* calls amigo and generates code */
	}
	for (f=functions; f; f=f->next)
		if (!(f->flags&GENERATED)) {
			restore(f);

			/* calls amigo and generates code */
			cg_endf(al_endf(),f->sid);
		}
}

/****************************************************************************/
/****************************************************************************/
/******************** manipulate call graph**********************************/
/****************************************************************************/
/****************************************************************************/
static void
sort_call_list(f)
Function f;
{
	struct Call_list min;
	struct Call_list max;
	/* RMA call with this */
	struct Call_list *call_list=get_summary(f)->call_list;
	struct Call_list *next_call;

	min.f_value = -1;
	max.f_value = MAX_VALUE;
	max.next = &min;
	min.next = 0;

	/* sort the call_list by benefit calculated by make_value */
	for ( ; call_list; call_list=next_call) {

		/* evaluates the benefit of the call based on size of called
		   function and parameter argument parings
		*/
		int value = call_list->value = eval_value(call_list);

		struct Call_list *insert;
		next_call = call_list->next;

		/* weights the value by whether call occurs in loop */
		value= call_list->f_value = 
		    make_value(call_list->frequency+1,value);

		/* Records the call. Uncalled functions are later deleted
		   in process_deletes. Functions called once can be deleted
		   if the single call is expanded
		*/
		make_call(f,call_list->func,call_list);

		if (value <= 0)
			continue;

		for (insert = &max; value < insert->next->f_value; 
		    insert=insert->next);

		/* insert->f_value > value >= insert->next->f_value */

		call_list->next = insert->next;
		insert->next = call_list;
	}

	/* find the predecessor to min */
	for (call_list = &max; call_list->next != &min; 
	    call_list = call_list->next);

	/* delete min from list */
	call_list->next = 0;

	/* set call_list to sorted list */
	get_summary(f)->call_list = max.next;
}


/* marks a call in the Function structure of the callee */
static void
make_call(caller,callee, call)
Function caller, callee;
struct Call_list *call;
{
	if (callee->flags & CALLED)
		callee->flags &= ~CALLED_ONCE;
	else {
		callee->flags |= CALLED_ONCE;
		callee->unique_caller = caller;
		callee->unique_call = call;
	}
	callee->flags |= CALLED;
}

#define FIRST_SIG 4

/* Evaluates the benefit of a call into an unsigned number VALUE_BITS long.
   The several high order bits, represent a number which is larger for 
   inlined calls to small functions. The low order bits represent the benefit
   of the argument-parameter pairings of the call
*/
static int
eval_value(call)
struct Call_list *call;
{
	struct Summary *callee_summary = get_summary(call->func);
	int size_count;
	int count;
	int size;
	int value = 1;

	/* Return if definition of called function not seen */
	if (!callee_summary)
		return -1;

	/* Return if arguments and formals don't have the same number or
	   if call cannot be precomputed (eg. contained in rhs of a boolean.
	   Formal_count is set to -1 if called function doesn't meet certain
	   criteria (eg. a structure argument)
	*/
	if (call->arg_count != callee_summary->formal_count || 
	    call->flags&NOT_PRECOMPUTE)
		return -1;

#ifndef NODBG
	if (il_leaf_only && !(callee_summary->flags & LEAF_FUNCTION))
		return -1;
	if (il_no_loop_only && (callee_summary->flags & CONTAINS_LOOP))
		return -1;
	if (il_read_only || il_copy_only) {
		for (count=0; count < call->arg_count; ++count) {
			if (!(callee_summary->formals[count] & PARAM_READ_ONLY)
			  	&& il_read_only)
				return -1;
			if (call->args[count]&ARG_CONST) 
				continue;
			if (call->args[count]&ARG_COPY_PROP_CAND) 
				continue;
			return -1;
		}
	}
#endif
	size=callee_summary->node_count;

	/* find most significant bit and map as follows:
	   bits 0-FIRST_SIG to MAX_HIGH
	   bits FIRST_SIG+1 - FIRST_SIG+MAX_HIGH to MAX_HIGH-1, 0
	   bits > FIRST_SIG+MAX_HIGH+1 to 0
	*/

	size >>= FIRST_SIG;
	for (size_count=MAX_HIGH; size_count>0; --size_count) {
		if (size == 0)
			break;
		size >>= 1;
	}

	/* evaluate argument parameter pairings */
	for (count=0; count < call->arg_count; ++count) {
		if (cg_machine_type(call->types[count]) != 
		    cg_machine_type(callee_summary->types[count]))
			return -1;
		if (call->args[count]&ARG_CONST) {
			++value;
			if (callee_summary->formals[count] & PARAM_OP_CONST)
				++value;
		}
		if (call->args[count]&ARG_KNOWN_PTR) 
			++value;
		if (call->args[count]&ARG_COPY_PROP_CAND)  {
			++value;
			if (callee_summary->formals[count] & PARAM_READ_ONLY)
				++value;
		}
	}
	if (value > MAX_LOW)
		value = MAX_LOW;
	value = value + (size_count<<LOW_ORDER);
	return value;
}

/* deletes functions that are not called and have not had address taken by
   marking functions is not defined. static functions that are called once
   are marked so that they will be deleted if the unique call is expanded
*/
static void
process_deletes(f) Function f;
{
	/* eliminate access to Function */
	for ( ; f; f= f->next) {
		if ((f->flags&(CALLED|NODELETE)) == 0 && SY_CLASS(f->sid) == SC_STATIC) { 
			f->flags &= ~DEFINED;
#ifndef NODBG
			if (do_inline&(DEXPAND|DEXPAND1)) 
				DPRINTF("deleting function=%s(%d)\n",
					SY_NAME(f->sid), f->sid);
#endif
		/* RMA cascade deletes */
		}

		/* check for a unique call to static function */
		else if ((f->flags&(CALLED_ONCE|NODELETE)) == CALLED_ONCE && 
		    SY_CLASS(f->sid) == SC_STATIC &&
		    (f->unique_caller->flags&CALLED) == 0) {

			/* mark call site deleted and insert new call site
			   at front of call_list
			*/
			struct Call_list *new_call = 
			    Arena_alloc(inline_arena,1,struct Call_list);

			*new_call = *f->unique_call;
			f->unique_call->flags = DELETED;
			new_call->flags = DELETE_ON_EXPAND;
			new_call->next =
			    f->unique_caller->summary.call_list;
			f->unique_caller->summary.call_list = new_call;
		}
	}
}
/* Given a list of function, deletes those functions that have not been
   defined, or have already been sent to code generator. Reverses the
   order of the list. There is really not reason to reverse this order
*/
static Function
filter(f) Function f;
{
	Function last=0;
	while (f) {
		Function next;
		if (!(f->flags&DEFINED)) {
			f = f->next;
			continue;
		}
		next = f->next;
		f->next = last;
		last = f;
		f = next;
	}
	return last;
}

/****************************************************************************/
/****************************************************************************/
/******************** make call trees ***************************************/
/****************************************************************************/
/****************************************************************************/
/* Initially, for each function, make_call_tree constructions a singleton tree
   with the function at the root. For every node in the call trees that 
   contains an unexpanded call there corresponds an Expansion_site structure.
   push_site maintains a sorted list of the Expansion_sites. make_call_tree
   calls push_site to process the singleton call trees. It then calls
   expand_list to add to the call trees until the expected space increase
   due to the expansions exceed a thresh-hold.
*/


/* creates a linked list of singleton call_trees. Each singleton tree is
   recorded as sites for inline expansion by make_site. expand_list, fills
   out the call_trees.
*/
static Arena call_tree_arena;
static struct Call_tree_list *
make_call_trees(funcs)
Function funcs;
{
	static void expand_list();
	struct Call_tree_list *tree_list = 0;
	static void init_expansions();
	init_expansions();

	call_tree_arena = arena_init();
	for ( ; funcs; funcs=funcs->next) {
		struct Expansion_sites *new_site;
		struct Call_tree *tree_node;
		struct Call_tree_list *new_list;
		if (!(funcs->flags&DEFINED))
			continue;
		new_list = Arena_alloc(inline_arena,1, struct Call_tree_list);
		new_list->next = tree_list;
		new_list->open_flags = EDITING;
		tree_list = new_list;
		funcs->call_tree = tree_list;

		tree_list->tree = tree_node = 
		    Arena_alloc(inline_arena,1, struct Call_tree);
		tree_node->func = funcs;
		tree_node->edges = 0;
		new_site = make_site(new_list,tree_node,1);
		if (new_site)
			push_site(new_site);
	}
	/* put expand_list inline */
	expand_list();
	arena_term(call_tree_arena);
	return tree_list;
}


/* Each node in the call_tree's containing an unexpanded call has an
   Expansion_site.
*/
struct Expansion_sites {
	struct Call_tree *tree;		  /* a node in the call tree */
	struct Call_tree_list *tree_list;/* list that contains the tree */

	/* Estimates the number of times this node in the call tree reached
	   given that the root is reached once. Estimation based on the
	   amount of nesting at the call sites
	*/
	int caller_frequency;

	int value;			/* benefit of the expansion */
	struct Call_list *next_call;	/* next call to expand */
	struct Expansion_sites *next;	/* next call to expand */
};

static struct Expansion_sites last_site = {0,0,0, -1, 0, 0};
static struct Expansion_sites best_site ={0,0,0,MAX_VALUE,0,0};

static void
init_expansions() {
	best_site.next = &last_site;
}

/* Iteratively removes the best site from the sorted list of sites, adds
   a new node for callee to the call_tree, and adds an arc from the caller
   to the callee. If not all sites at caller have been expanded, repushes
   the caller back into sorted list. Pushes callee into sored list
*/
static void
expand_list() {
	struct Expansion_sites *caller;
	int limit = node_count/inline_expansion;
	while ( ( caller = best_site.next) != &last_site) {
		static int back_off();
		int caller_frequency;
		struct Tree_edge *new_edge;
		struct Call_list *func_call = caller->next_call;
		struct Summary *summary;
		summary = get_summary(func_call->func);

		if (func_call->flags != DELETE_ON_EXPAND)
			limit -= summary->node_count;	/* record expansion */
		else {	/* no expansion */
			struct Expansion_sites *dsite;
			struct Call_tree_list *dtree_list = 
			    func_call->func->call_tree;

			/* mark GENERATED to suppress code generation */
			func_call->func->flags |= GENERATED;
#ifndef NODBG
			if (do_inline & (DEXPAND|DEXPAND1))
				DPRINTF("deleting function=%s(%d)\n",
			    	    SY_NAME(func_call->func->sid), 
			    	    func_call->func->sid);

			/* DCOUNT may suppress this expansions, so don't
			   delete the function
			*/
			if (do_inline&DCOUNT)
				func_call->func->flags &= ~GENERATED;
#endif

			/* back of expansions rooted at func_call->func */
			limit += back_off(dtree_list->tree);

			/* remove sites rooted at func_call->func */
			for (dsite = &best_site; dsite != &last_site; 
				dsite = dsite->next) {
				if (dsite->next->tree_list == dtree_list)
					dsite->next = dsite->next->next;
			}
		}
		if (limit < 0)	/* end of expansions */
			break;

		caller_frequency = caller->caller_frequency;

		/* add an edge in the tree */
		new_edge = add_edge(caller->tree, func_call);

#ifndef NODBG
		if (do_inline & DBUILD_TREE) {
			DPRINTF("expansion count=%d use=%d * (%d,%d)\n",
			new_edge->count, caller->value / func_call->value,
			func_call->value>>LOW_ORDER, func_call->value&MAX_LOW);
			
		}
#endif

		/* delete site from list */
		best_site.next = caller->next;

		/* push site back onto the list, with new next_call */
		caller->next_call = func_call->next;
		push_site(caller);

		/* push the calls from the edge target */
		caller  = make_site(caller->tree_list,new_edge->callee, 
		    caller_frequency+ func_call->frequency);
		if (caller)
			push_site(caller);
	}
}


/* accumulates the amount of expansion for a call_tree */
static int
back_off(call_tree)
struct Call_tree *call_tree;
{
	int count=0;
	struct Tree_edge *edge;
	for (edge = call_tree->edges; edge; edge = edge->next) {
		count += edge->callee->func->summary.node_count;
		count += back_off(edge->callee);
	}
	call_tree->edges = 0;
	return count;
}

/* makes an Expansion_site */
static struct Expansion_sites *
make_site(tree_list,tree, caller_frequency) 
struct Call_tree *tree;
struct Call_tree_list *tree_list;
int caller_frequency;
{
	struct Call_list *first_call = tree->func->summary.call_list;
	struct Expansion_sites *new_site;

	/* skip deleted calls */
	while(first_call && 
	    (first_call->flags == DELETED || first_call->value <= 0))
		first_call = first_call->next;

	/* if no calls left then don't create site */
	if (!first_call)
		return 0;

	new_site = Arena_alloc(call_tree_arena, 1, struct Expansion_sites);
	new_site->tree= tree;
	new_site->tree_list= tree_list;
	new_site->next_call = first_call;
	new_site->caller_frequency = caller_frequency;
	return new_site;
}

/* splice a new site into the sorted list of sites */
static void
push_site(new_site)
struct Expansion_sites *new_site;
{
	struct Expansion_sites *insert;
	struct Call_list *first_call = new_site->next_call;

	
	while(first_call && 
	    (first_call->flags == DELETED || first_call->value <= 0))
		first_call = first_call->next;

	if (!first_call)
		return;

	/* first call is the next unexpanded call at new_site */
	new_site->next_call = first_call;

	/* value depends on value of the call, frequency of calling
	   function, and frequency of the call site
	*/
	new_site->value = make_value(new_site->caller_frequency+
	    first_call->frequency, first_call->value);

	/* insert the new_site with a bubble sort */
	for (insert = &best_site; new_site->value < insert->next->value; 
	    insert=insert->next);
	/* insert->value > value >= insert->next->value */
	new_site->next = insert->next;
	insert->next = new_site;
}

/* weights "value" by frequency, insures no overflow */
static int
make_value(frequency, value)
int frequency, value;
{
	/* the maximum value of value is 1<<VALUE_BITS, following check
	   insures that value * frequency < MAX_VALUE 
	*/
	if (frequency > (MAX_VALUE >> VALUE_BITS))
		return MAX_VALUE - 1;
	return frequency*value;
}



/* adds an edge to the calling tree */
static struct Tree_edge *
add_edge(tree, call)
struct Call_tree *tree;
struct Call_list *call;
{
	static int count=0;
	struct Tree_edge *new_tree_edge = 
	   Arena_alloc(inline_arena,1, struct Tree_edge);
	new_tree_edge->next = tree->edges;
	tree->edges = new_tree_edge;
	new_tree_edge->site  = call->call_site;
#ifndef NODBG
	new_tree_edge->count = ++count;
#endif
	new_tree_edge->callee = Arena_alloc(inline_arena,1, struct Call_tree);
	new_tree_edge->callee->func = call->func;
	new_tree_edge->order = call->order;
	new_tree_edge->callee->edges = 0;
#ifndef NODBG
	new_tree_edge->flags = call->flags;
#endif
	return new_tree_edge;
}

/****************************************************************************/
/****************************************************************************/
/************************** order the trees *********************************/
/****************************************************************************/
/****************************************************************************/
/* Orders the processing of functions for modification by inline expansion.
   First create an expansion graph with nodes corresponding to the functions.
   An edge from p to q indicates that q is inlined into p. It may be the
   case the p calls r1 calls r2 .. call rn  calls q and r1 .. rn are all
   expanded. Then orders the graph so that if q is inlined in p, then
   p is processed before q. If q were processed before p, then it would
   have to be copied before processing.
*/



/* flags for visited in Expansions */
#define NOT_VISITED 0
#define VISITED 1
#define VISITING 2

static struct Expansions {
	int visited;
	Bit_vector callees;
	struct Call_tree_list *tree_root;
} *expansions;

#define FUNCT_TO_SLOT(function) \
	((function)->flags & DEFINED ? (function)->summary.function_count : 0)

#define SLOT_TO_FUNC(slot) expansions[slot].tree_root->tree->func


/* creates the expansion graph */
static struct Call_tree_list *
order_trees(trees) 
struct Call_tree_list *trees;
{
	Arena order_arena = arena_init();
	int max_count = 1;
	int func_count;
	struct Call_tree_list new_head;
	struct Call_tree_list *curr_list = &new_head;
	new_head.next = trees;

	/* count functions containing expansions */
	while (curr_list->next != 0) {
	    if (curr_list->next->tree->func->flags&GENERATED ||
		curr_list->next->tree->edges == 0) {
		    curr_list->next->tree->func->summary.function_count = 0;
		    curr_list->next = curr_list->next->next;
		    continue;
	    }
	    else {
		++max_count;
		curr_list = curr_list->next;
	    }
	}

	trees = new_head.next;

	/* initialize expansions */
	expansions = Arena_alloc(order_arena, max_count, struct Expansions);

	func_count = 1;
	for (curr_list = trees; curr_list; curr_list = curr_list->next) {
		expansions[func_count].callees = 
		    bv_alloc(max_count, order_arena);
		bv_init(false,expansions[func_count].callees);
		expansions[func_count].tree_root = curr_list; 
		expansions[func_count].visited = NOT_VISITED;
		curr_list->tree->func->summary.function_count = func_count;
		++func_count;
	}

	/* populate expansions */
	func_count = 1;
	for (curr_list = trees; curr_list; curr_list = curr_list->next) {
		enter_expansion(expansions[func_count].callees, curr_list->tree);
		++func_count;
	}

	/* order functions */
	curr_list = 0;
	for (func_count=1; func_count < max_count; ++func_count) {
		if (!expansions[func_count].visited)
			curr_list = top_down_order(expansions+func_count,
		    	    curr_list);
	}

	arena_term(order_arena);
	return curr_list;
}

/* Orders the functions for modification by inline expansion as follows:
   If there is a path from p to q in the expansion graph and not a path from
   q to p then p will appear before q in the ordered list. If there is a cycle
   including p and q, then which ever function is placed earlier in the list
   is marked for copy, since this function will be referenced after it has
   been modified by inline expansion
*/
static struct Call_tree_list *
top_down_order(caller, curr_list)
struct Expansions *caller;
struct Call_tree_list *curr_list;
{
	caller->visited = VISITING;
	BV_FOR(caller->callees, callee)
		if (expansions[callee].visited) {

			/* if VISITING then callee will appear before caller */
			if (expansions[callee].visited == VISITING)
				expansions[callee].tree_root->open_flags =
				    EDIT_COPY;
		}
		else {
			curr_list = top_down_order(expansions+callee,curr_list);
		}
	END_BV_FOR

	caller->visited = VISITED;
	caller->tree_root->next = curr_list;
	return caller->tree_root;
}

/* enter every function below the root of call_tree into root_calls */
static void
enter_expansion(root_calls, call_tree) 
Bit_vector root_calls;
struct Call_tree *call_tree;
{
	struct Tree_edge *edge;
	for (edge = call_tree->edges; edge; edge = edge->next) {
		struct Call_tree *callee = edge->callee;
		int slot = FUNCT_TO_SLOT(callee->func);
		if (slot > 0) 
			bv_set_bit(FUNCT_TO_SLOT(callee->func), root_calls);
		enter_expansion(root_calls, callee);
		
	}
}

/*****************************************************************************/
/*****************************************************************************/
/*************************** perform inline expansion*************************/
/*****************************************************************************/
/*****************************************************************************/

/* Inline expansion is done by replacing the call to be expanded by a variable
   or a NOP (if call returns no value). The code for the inlined function
   is then inserted before the statement containing the call. The code is
   preceded by a label (to help register allocation) and followed by a
   label where control goes for a return.

   1. return is mapped to "goto endlabel;"
   2. return expr is mapped to "return_variable = expr; goto endlabel;"
   3. automatics are parameters in the inlined procedure are mapped to new
      automatics.
   4. An argument is assigned to the automatic mapped to the corresponding
      formal.
   5. labels defined in the inlined procedure are mapped to new labels.
*/



/* The calls in f scheduled for expansion are ordered so that if both g
   and h are scheduled for expansion, and g is called in h's arguments, then
   g is expanded before h. The ordered calls are then expanded. After c is
   expanded, any calls from c are expanded by a recursive call to expand_calls
*/
static void
expand_calls(f, calls,level)
struct Funct_copy *f;	/* caller */
struct Tree_edge *calls;/* calls from caller */
int level;	/* for debugging only */
{
#define MAX_ORDER 1<<30
	struct Tree_edge *next_call=0;	/* init'ed for lint */
	struct Tree_edge max;
	struct Tree_edge min;
	max.order = MAX_ORDER;
	max.next = 0;
	min.order = -1;
	min.next = &max;

	/* order the calls */
	for ( ; calls ; calls = next_call) {
		struct Tree_edge *e;
		next_call = calls->next;

		/* after iter, know that edge should be between e and e->next*/
		for (e = &min; calls->order>e->next->order; e=e->next);

		calls->next = e->next;
		e->next = calls;
	}


	/* expand the calls in order */
	for (calls=min.next ; calls->order != MAX_ORDER; calls = calls->next) {
		struct Funct_copy *callee;
		struct Call_table *e = get_call_site(calls->site, f);
#ifndef NODBG
		static int do_expand();
		if (do_inline&DCOUNT && !do_expand(calls->count))
			continue;
#endif

		callee = open_function(calls->callee->func,INLINING);
#ifndef NODBG
		if (do_inline&(DEXPAND|DEXPAND1|DCOUNT)) {
			if (calls->flags != DELETE_ON_EXPAND)
				nodes_used += 
				    calls->callee->func->summary.node_count;
			DPRINTF("Expanding call at level=%d count=%d site=%d  from %s%s(%d) at (node=%d index=%d) to function=%s(%d) -  nodes left=%d\n",
			level, calls->count, calls->site,
			level == 0 && f->flags == EDIT_COPY ? "copy " : "",
			SY_NAME(f->func->sid), f->func->sid,
			node_no(e->node), CGQ_INDEX_NUM(e->index), 
			SY_NAME(callee->func->sid),callee->func->sid,
			node_count/inline_expansion - nodes_used);
		}
				
#endif
		inline(e->node, e->index, callee);
		expand_calls(callee, calls->callee->edges,level+1);
		close_function(callee);
	}
}


static void 
inline(site, index,call )
ND1 *site;		/* points to a call */
Cgq_index index;	/* index of tree containing the call */
struct Funct_copy *call;
{
	Cgq_index prev = CGQ_ELEM_OF_INDEX(index)->cgq_prev;
	cgq_t *cgq,*lab;
	
	int return_label;
	SX return_value = 0;
	ND2 *end;
	Cgq_index call_index = index;

#ifndef NODBG
	if (do_inline&(DEXPAND|DEXPAND1)) {
		DPRINTF("\t\tBefore: \n\n");
		cg_putq_between(3, index, index);
	}
#endif
	/* create a label before expansion to help register allocation */
	index = CGQ_INDEX_OF_ELEM(lab=cg_q_insert(prev));
	lab->cgq_op = CGQ_EXPR_ND2;
	lab->cgq_arg.cgq_nd2=end=(ND2 *)talloc();
	end->op=LABELOP;
	end->label = getlab();

	/* first item in expanded function */
	index = CGQ_INDEX_OF_ELEM(cg_q_insert(index));

	/* label after function */
	lab = cg_q_insert(index);
	lab->cgq_op = CGQ_EXPR_ND2;
	lab->cgq_arg.cgq_nd2=end=(ND2 *)talloc();
	end->op=LABELOP;
	end->label =return_label =getlab();

	if (TY_TYPE(site->type) != TY_VOID) {

		/* create scope markers for return value */
		TEMP_SCOPE_ID scope; 
		scope = al_create_scope(index,extend_scope(call_index));
		return_value = sy_temp(site->type);
		SY_NAME(return_value) = "RNODE";
		al_add_to_scope(return_value,scope);

		/* replace call by reference to return_value */
		site->op = NAME;
		site->lval=0;
		site->rval = return_value;
	}
	else {
		/* replace call by a no-opt */
		site->op = ICON;
		site->type = TY_INT;
		site->lval = site->rval = 0;
	}

	/* read in cgq items from the inlined function to the calling
	   function until encounter a db_symbol. This symbol declares an
	   argument. The read_cgq_item has replaced the sid in the db_symbol
	   with a new sid for an automatic. An assignment of an argument to
	   the db_symbol is then inserted.
	*/
	cgq=CGQ_ELEM_OF_INDEX(index);
	for (;;) {
		read_cgq_item(call,index, return_value);
		if (cgq->cgq_op ==CGQ_CALL && cgq->cgq_func == db_s_block)
			break;
		if (cgq->cgq_op == CGQ_CALL_SID && cgq->cgq_func == db_symbol) {
			SX param=cgq->cgq_arg.cgq_sid;

			/* insert a cgq item for assignment to arg */
			index = CGQ_INDEX_OF_ELEM((cgq=cg_q_insert(index)));
			cgq->cgq_op = CGQ_EXPR_ND1;
			cgq->cgq_arg.cgq_nd1 = 
			    make_assign(param, next_argument(&site->right));

			/* insert an element for the next read */
			index = CGQ_INDEX_OF_ELEM(cgq=cg_q_insert(index));
		}
	} 
	/* any arguments and the db_s_block are now in caller's cgq */

	/* Read in the rest of the function body. read_cgq_item copies each
	   cgq item into the caller's cgq. read_cgq_item creates new symbols
	   for automatics defined in the function body and creates new labels
	   for labels defined in the body. References to automatics and labels
	   are mapped to the new symbols and labels.
	*/
	do {
		ND2 *jump;

		index = CGQ_INDEX_OF_ELEM(cgq=cg_q_insert(index));
		read_cgq_item(call,index,return_value);

		/* translate RETURNS */
		if (cgq->cgq_op == CGQ_EXPR_ND1 && 
		    cgq->cgq_arg.cgq_nd1->op == RETURN) {

			/* make a goto */
			cgq->cgq_op = CGQ_EXPR_ND2;
			cgq->cgq_arg.cgq_nd2=jump = (ND2*)talloc();
			jump->op = JUMP;
			jump->type = T2_INT;
			jump->label = return_label;
		}
	} while(!(cgq->cgq_op ==CGQ_CALL && cgq->cgq_func == db_e_fcode));
	cg_q_delete(index);	/* delete the db_e_fcode */

#ifndef NODBG
	if (do_inline&DEXPAND1) {
		DPRINTF("\t\tAfter: \n\n");
		cg_putq_between(3,prev,call_index);
	}
#endif
}

/* Obtain the cgq for the opened functions, intialize tables used to map
   symbols and labels, and tables that obtain Cgq_index and node pointers
   for Call_sites. Symbol tables and label tables used for INLINING. Call_site
   table used in all modes of open. Since saved cgq is not modified for
   open(EDIT_COPY) or open(EDITING), the Call_site table can be translated
   at once. For open(EDITING), the Call_site table is filled in as each
   cgq_item containing a Call_site is read by read_cgq_item
*/
static
struct Funct_copy *
open_function(f,flag) Function f; int flag; {
	Arena a = arena_init();
	struct Funct_copy *fc = Arena_alloc(a,1,struct Funct_copy);
	fc->func = f;
	fc->arena = a;
	fc->flags = flag;
	fc->call_table = Arena_alloc(a, f->expr_count+1, struct Call_table);

	if (!(f->flags & DEFINED))
		cerror("Function does not exist");

	if (flag == INLINING) {
		fc->next_index = CGQ_FIRST_INDEX;
		fc->label_table = Arena_alloc(a, f->max_label-f->min_label,int);
		fc->symbol_table = 
		    Arena_alloc(a,f->max_symbol-f->min_symbol, SX );
		memset(fc->label_table,0,sizeof(int)*
		    (f->max_label - f->min_label));
		memset(fc->symbol_table,0,sizeof(SX)*
		    (f->max_symbol - f->min_symbol));
	}
	else if (flag == EDIT_COPY) {

		/* expand linked list form of Call_site mappings in Function
		   to array format used in Funct_copy
		*/
		expand_call_site_tables(fc);

		/* makes f the "current" cgq. The cgq accessed and modified
		   by acomp's primitivies (cgq_insert, CGQ_FOR_ALL, etc)
		*/
		restore(f);

		/* creates a copy of the cgq, and stores pointer in f */
		copy(f);
	}
	else if (flag == EDITING) {
		/* editing destroys the definition */
		f->flags &= ~DEFINED;
		expand_call_site_tables(fc);

		/* makes f the "current" cgq. The cgq accessed and modified
		   by acomp's primitivies (cgq_insert, CGQ_FOR_ALL, etc)
		*/
		restore(f);
	}
	else {
		cerror("bad flag to open_function");
	}
	return fc;
}
/* Deletes tables created by open, generates code for functions opened for
   EDIT or EDIT_COPY
*/
static void
close_function(f) struct Funct_copy *f; {
	if (f->flags == EDIT_COPY || f->flags == EDITING) {
		cg_endf(al_endf(),f->func->sid);
		f->func->flags |= GENERATED;
	}
	arena_term(f->arena);
}



/* makes f the "current" cgq. The cgq accessed and modified
   by acomp's primitivies (cgq_insert, CGQ_FOR_ALL, etc)
*/
static void
restore(f) Function f; 
{
	global_arena = f->save_arena;
	td_cgq = f->save_cgq;
	al_begf();      /* marks f as no setjmp or asm since f is optimizable */
	cg_restore(f->save_index);
}

/* copies the CGQ for open(EDIT_COPY) */
static void
copy(f) Function f;
{
	Arena save_arena;
	f->save_arena = arena_init();
	f->save_index = CGQ_FIRST_INDEX;
	f->save_cgq.td_used = 0;
	f->save_cgq.td_start = (myVOID *)malloc(td_cgq.td_allo*sizeof(cgq_t));
	if (!f->save_cgq.td_start)
		cerror("No space for cgq");

	/* do not need call_table_list and expr_count */

	save_arena = global_arena;
	global_arena = f->save_arena;

	/* access a cgq item from the current cgq, and copy it into an empty
	   element in the new cgq (f->save_cgq). Point the forward pointer
	   of the previous element to the copied item
	*/
	CGQ_FOR_ALL(src_pt, src_index)
		Cgq_index targ_index = TD_USED(f->save_cgq)*sizeof(cgq_t);
		cgq_t *targ_pt = CGQ_ELEM_OF_INDEX_Q(targ_index,&f->save_cgq);

		/* copy cgq item, RMA should do field by field */
		*targ_pt = *src_pt;

		targ_pt->cgq_prev = f->save_index;

		/* set forward pointer of previous element to targ_index. On
		   first item, this action does nothing harmful or useful, but
		   avoids making a special case
		*/
		CGQ_ELEM_OF_INDEX_Q(f->save_index,&f->save_cgq)->cgq_next =
				targ_index;

		/* copy trees */
		switch(src_pt->cgq_op) {
		case CGQ_FIX_SWBEG:
		case CGQ_EXPR_ND1:
		case CGQ_EXPR_ND2:
			targ_pt->cgq_arg.cgq_nd1 = 
			    (ND1 *)tcopy(targ_pt->cgq_arg.cgq_nd1);
		}

		++TD_USED(f->save_cgq);
		TD_CHKMAX(f->save_cgq);
		f->save_index = targ_index;
	CGQ_END_FOR_ALL

	CGQ_ELEM_OF_INDEX_Q(f->save_index,&f->save_cgq)->cgq_next = CGQ_NULL_INDEX;

	global_arena = save_arena;
}

/* RMA does it need to return? */
/* Copies the next item from f's cgq into the slot in the current cgq indexed
   by buffer index. Following transformations are done (by routines called
   from read_cgq_item):
	1. References to RNODE replaced by the return variable.
	2. Labels are mapped to new labels.
	3. Automatics are parameters are mapped to new automatics.
	4. When copying a tree that was passed to mark_call_site, the new
	   location of the tree is noted in "f". 
*/
static int
read_cgq_item(f,buffer_index,return_variable)
struct Funct_copy *f;
Cgq_index buffer_index;
int return_variable;
{
	cgq_t *place=CGQ_ELEM_OF_INDEX(buffer_index);
	Cgq_index save_prev = place->cgq_prev;
	Cgq_index save_next = place->cgq_next;
	
	struct td *cgq;
	Cgq_index input_index = f->next_index;

	cgq = &f->func->save_cgq;

	if (input_index == CGQ_NULL_INDEX || input_index > CGQ_LAST_ITEM(cgq)) {
		cerror("reading eof");
		return 0;
	}

	/* copy the cgq item (RMA inefficient) */
	*place = *CGQ_ELEM_OF_INDEX_Q(input_index,cgq);
	f->next_index = place->cgq_next;

	place->cgq_prev = save_prev;
	place->cgq_next = save_next;

	switch (place->cgq_op) {
		case CGQ_EXPR_ND1:
		case CGQ_EXPR_ND2:
		case CGQ_FIX_SWBEG:
			/* copy the trees and perform transformations */
			place->cgq_arg.cgq_nd1=
			    (ND1 *)copy_tree((NODE *)place->cgq_arg.cgq_nd1,f,
			       buffer_index, return_variable);
			break;
		case CGQ_CALL_SID:
			place->cgq_arg.cgq_sid =
			    translate_symbol(place->cgq_arg.cgq_sid,f );
	}
	return 1;
}
/* copies trees and performs transformations described in read_cgq_item */
static NODE *
copy_tree(in,f,index, return_variable) NODE *in; 
struct Funct_copy *f;
Cgq_index index;
int return_variable;
{
	/* should call translate type */
	/* should call translate string */
	NODE *out=talloc();

	*out = *in;	/* we can be much more efficient */

	/* non-zero "opt" indicates expression tagged by mark_call_site,
	   more new location of this expression in the call_table
	*/
	if (in->fn.opt) {
		f->call_table[(int)in->fn.opt].node = (ND1 *)out;
		f->call_table[(int)in->fn.opt].index = index;
	}

	if (optype(in->tn.op) == UTYPE || optype(in->tn.op) == BITYPE)
		out->tn.left = copy_tree(in->tn.left,f,index,return_variable);
	if (optype(in->tn.op) == BITYPE)
		out->tn.right = copy_tree(in->tn.right,f,index,return_variable);

	switch(in->tn.op) {
	case CBRANCH:
		((ND1 *)out)->right->lval = 
		    translate_label((int)((ND1 *)in)->right->lval,f);
		break;
	case ICON:
	case NAME:
		if (in->fn.rval > 0)
			out->fn.rval = translate_symbol(in->fn.rval,f);
		else if (in->fn.rval < 0)
			out->fn.rval = -translate_label((int)-in->fn.rval,f);
		break;
	case RNODE:
		/* translate RNODE to return value */
		out->fn.op = NAME;
		out->fn.flags = 0;
		out->fn.lval = 0;
		out->fn.rval = return_variable;
		break;
	case LABELOP:
	case JUMP:
	case SWCASE:
		out->tn.label = translate_label(in->tn.label,f);
		break;
	case SWEND:
		if (out->tn.lval != -1)
			out->tn.lval = translate_label((int)in->tn.lval,f);
		break;
	}
	return out;
}

/*
   Returns the mapped value of a label. If no mapped value exists (mapped
   value is 0), then set the mapped value to a new label
*/
static int
translate_label(before,f) int before; struct Funct_copy *f; {
	int *label;
	label = &f->label_table[before-f->func->min_label];
	if (*label == 0) {
		*label = getlab();	/* gets a new label */
	}
	return *label;
}

/*
   Returns the mapped value of a symbol table index
*/
static int
translate_symbol(before,f) SX before; struct Funct_copy *f; {

	SX after;

	/* check if mapping appropriate */
	if (!(SY_CLASS(before) == SC_LABEL || SY_CLASS(before) == SC_AUTO || 

		/* Obscure point: The check for SY_LEVEL == 1 insures the 
	   	  "before" is a parameter to f, and not a parameter to 
	   	  a function prototype declared in f)
		*/
		(SY_CLASS(before) == SC_PARAM && SY_LEVEL(before) ==1)))

		return before;

	/* map the symbol if not mapped already */
	if (!(after=f->symbol_table[before-f->func->min_symbol])) {
		after = sy_temp(SY_TYPE(before));	/* new sumbol */
		SY_NAME(after) = SY_NAME(before);
		SY_FLAGS(after) = SY_FLAGS(before);
		f->symbol_table[before-f->func->min_symbol] = after;

		/* SY_OFFSET of a label symbol is the label number */
		if (SY_CLASS(before) == SC_LABEL) {
			SY_CLASS(after) = SC_LABEL;
			SY_OFFSET(after) = translate_label(SY_OFFSET(before),f);
		}
	}

	return after;
}

static struct Call_table *
get_call_site(handle,fc ) Call_site handle; struct Funct_copy *fc; {
	return &fc->call_table[handle];
}
/* if not SWBEG return index, else return index of SWEND, prevents register
   allocation from placing code between SWBEG and SWEND
*/
static Cgq_index
extend_scope(index)
Cgq_index index;
{
	cgq_t *elem = CGQ_ELEM_OF_INDEX(index);
	if (elem->cgq_op != CGQ_FIX_SWBEG) 
		return index;
	while (elem->cgq_arg.cgq_nd2->op != SWEND) {
		index = elem->cgq_next;
#ifndef NODBG
		if (index == CGQ_NULL_INDEX)
			cerror("SWEND not found");
#endif
		elem=CGQ_ELEM_OF_INDEX(index);
	}
	return index;
}

static struct Summary *
get_summary(f)
Function f;
{
	if (f->summary.node_count == -1)	/* check if has summary */
		return 0;
	else
		return &f->summary;
}

/* if one argument returns it, otherwise delete the first argument,
   and return it
*/
static ND1 *
next_argument(arg_adr) ND1 **arg_adr;
{

	ND1 *arg = *arg_adr;

	if (arg->op == CM) {
		for ( ; arg->left->op == CM; arg=arg->left) {
			arg_adr = &arg->left;
		}
		/* arg is lowest CM */
		nfree(arg);

		arg=arg->left;

		/* replace CM by right child */
		*arg_adr = (*arg_adr)->right;	
	}

	/* arg points to left-most  ARG node */
	nfree(arg);
	return arg->left;
}


/* converts index list and node list to index and node tables */
static void
expand_call_site_tables(f) struct Funct_copy *f;
{
	struct Call_table_list *e;
	struct Call_table *call_table=f->call_table;
	int count;

	for (e=f->func->call_table_list,count=f->func->expr_count; e; 
	    e = e->next,--count) {
		call_table[count].node = e->node;
		call_table[count].index = e->index;
	}
}


/* build an assignment of node to NAME(sid) */
static ND1 * 
make_assign(sid, node) SX sid; ND1 *node; {
	ND1 *assign = t1alloc();
	ND1 *targ;
	assign->op = ASSIGN;
	assign->flags = FF_SEFF;
	assign->type = SY_TYPE(sid);
	if (TY_UNQUAL(assign->type) != TY_UNQUAL(node->type)) {
		ND1 *convert = t1alloc();
		convert->left = node;
		convert->op = CONV;
		convert->type = assign->type;
		convert->flags = FF_ISCAST;
		node = convert;
	}
	assign->right = node;
	assign->left = targ = t1alloc();
	targ->op = NAME;
	targ->rval = sid;
	targ->lval = 0;
	targ->type = assign->type;
	return assign;
}
#ifndef NODBG

/* prints the call graph */
static void
pr_graph() {
	Function f;
	for (f=functions; f; f = f->next) {
		int i;
		struct Call_list *call;
		if (!(f->flags&DEFINED))
			continue;

		/* print the function and param info */
		DPRINTF("function=%s(%d) nodes=%d parmcnt=%d params=(",
		    SY_NAME(f->sid),f->sid, f->summary.node_count, 
		    f->summary.formal_count);

		/* params */
		for (i=0; i<f->summary.formal_count; ++i)
			DPRINTF("%d ", f->summary.formals[i]);
		DPRINTF(")\n");

		/* print the calls */
		for (call=f->summary.call_list; call; call=call->next) {
			if (call->flags == DELETED)
				continue;
			DPRINTF("\tcall %s order=%d site=%d f=%d val=(%d,%d) func=%s(%d) argcnt=%d args=( ",
		    call->flags == DELETE_ON_EXPAND ? "(delete)" : "",
		    call->order, call->call_site, call->frequency, call->value>>LOW_ORDER, 
		    call->value&MAX_LOW,
		    SY_NAME(call->func->sid),
		    call->func->sid, call->arg_count);

			/* call args */
			for(i=0; i < call->arg_count; ++i)
				DPRINTF("%d ", call->args[i]);
			DPRINTF(")\n");
		}


	}

}
/* puts the cgq associated with a function */
void 
putq(f,level)
Function f;
int level;
{
	struct td save_q;
	Cgq_index save_index;

	DPRINTF("Function = %s(%d)\n", SY_NAME(f->sid), f->sid);
	if (level == 0)
		return;

	/* save cgq */
	save_index = cg_inline_endf();
	save_q = td_cgq;

	/* clobber cgq with functions cgq */
	td_cgq = f->save_cgq;
	cg_restore(f->save_index);

	cg_putq(3);

	/* restore cgq */
	td_cgq = save_q;
	cg_restore(save_index);
}

/* prints the call trees showing the scheduled expansions */
void put_call_tree(tree,level) 
struct Call_tree *tree;
int level;
{
	struct Tree_edge *edge;
	DPRINTF("%d %s(%d) \n ",level,tree->func->name, tree->func->sid);
	for (edge = tree->edges; edge; edge = edge->next) {
		DPRINTF("call at site %d from %d to ", edge->site, level);
		put_call_tree(edge->callee,++level);
	}
}
void put_expansion(e)
struct Expansions *e;
{
	DPRINTF("expansion for func %s(%d) expand=%d visit=%d calls",
		e->tree_root->tree->func->name,
		e->tree_root->tree->func->sid,
		e->tree_root->tree->func->summary.function_count,
		e->visited);
	BV_FOR(e->callees,f)
		DPRINTF("expand=%d %s(%d) ", SLOT_TO_FUNC(f)->summary.function_count,
		SLOT_TO_FUNC(f)->name, SLOT_TO_FUNC(f)->sid);
	END_BV_FOR
	DPRINTF("\n");
}

void 
put_call_tree_list(list)
struct Call_tree_list *list;
{
	for (; list; list=list->next)
		DPRINTF("list_flags=0x%x func=%s(%d) fun_flags=0x%x edges=0x%x slot=%d\n",
		    list->open_flags, list->tree->func->name,
		    list->tree->func->sid,
		    list->tree->func->flags,
		    list->tree->edges,
		    list->tree->func->summary.function_count);
}
/* used when do_inline&DCOUNT, determines whether count is in the list
   of integers listed in the environment variable DCOUNT
*/
static int
do_expand(count)
int count;
{
	char *getenv();
	char *dcount = getenv("DCOUNT");
	char *value_pt;
	int value;

	/* expand if DCOUNT not set */
	if (dcount == 0)
		return 1;

	/* check for count in list of integers at dcount */
	for (value_pt = dcount; (value=strtol(value_pt, &value_pt, 10)) != 0;)
		if (value == count)
			return 1;
	return 0;
}

void put_call_list(list)
struct Call_list *list;
{
	for (;list;list=list->next) {
		
		if (list->flags == DELETED)
			continue;
		DPRINTF("site=%d %s func=%s(%d)\n", list->call_site,
			list->flags == DELETE_ON_EXPAND ? "delete" : "",
			list->func->name, list->func->sid);
		list = list->next;
	}
	DPRINTF("\n");
}
#endif
#endif /* NO_AMIGO */
