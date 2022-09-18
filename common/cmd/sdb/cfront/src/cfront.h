/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/cfront.h	1.2"
/***********************************************************************

cfront.h:

	Here is all the class definitions for cfront, and most of the externs

***********************************************************************/

/*
 *	WARNING:
 *	This program relies on non-initialized class members being ZERO.
 *	This will be true as long as they are allocated using the "new" operator
 */

#include "token.h"
#include "typedef.h"
#include <string.h>

#ifndef __STDC__
#define const
#endif

extern bit old_fct_accepted;	/* if set:
					old style function definitions are legal,
					implicit declarations are legal
				*/
extern bit fct_void;		/* if set:
					int f(); ... f(1); gives a warning per file
					undeclared(); gives a warning per file
				   if not:
					int f(); ... f(1); is an error
					undeclared(); is an error								(currently only a warning)
					
				*/

#ifndef GRAM
extern char* prog_name;		/* compiler name and version */
extern int inline_restr;	/* inline expansion restrictions  */
extern bit emode;		/* print_mode error */
#endif

extern Pname name_free;		/* free lists */
extern Pexpr expr_free;
extern Pstmt stmt_free;

/* "spy" counters: */
extern int Nspy;
extern int Nfile, Nline, Ntoken, Nname, Nfree_store, Nalloc, Nfree;
extern int Nn, Nbt, Nt, Ne, Ns, Nstr, Nc, Nl;
extern int NFn, NFtn, NFpv, NFbt, NFf, NFs, NFc, NFe, NFl;

extern TOK	lex();
extern Pname	syn();
extern void	ext();

extern char* 	make_name();

struct loc {		/* a source file location */
	short	file;	/* index into file_name[], or zero */
	short	line;
};

void loc_put();
void loc_putline();

extern Loc curloc;
extern int curr_file;

struct ea {	/* fudge portable printf-like formatting for error() */
	union {
		char* p;
		int i;
	} u1;
};

extern struct ea* ea0;
extern struct ea V1, V2, V3, V4;

extern int error();
extern int errorFIPC();
extern int errorFloc();
extern int errorIloc();

#ifndef GRAM
extern int error_count;
extern bit debug;
extern int vtbl_opt;
extern FILE* out_file;
extern FILE* in_file;
extern char scan_started;
extern bit warn;
#endif

extern int br_level;
extern int bl_level;
extern Ptable ktbl;		/* keywords and typedef names */
extern Ptable gtbl;		/* global names */
extern char* oper_name();
extern Pclass ccl;
extern Pbase defa_type;
extern Pbase moe_type;

#ifndef GRAM
extern Pstmt Cstmt;		/* current statement, or 0 */
extern Pname Cdcl;		/* name currently being declared, or 0 */
extern void put_dcl_context();

extern Ptable any_tbl;		/* table of undefined struct members */
extern Pbase any_type;
#endif

extern Pbase int_type;
extern Pbase char_type;
extern Pbase short_type;
extern Pbase long_type;
extern Pbase uint_type;
extern Pbase float_type;
extern Pbase double_type;
extern Pbase void_type;

#ifndef GRAM
extern Pbase uchar_type;
extern Pbase ushort_type;
extern Pbase ulong_type;
extern Ptype Pchar_type;
extern Ptype Pint_type;
extern Ptype Pfctvec_type;
extern Ptype Pfctchar_type;
extern Ptype Pvoid_type;
extern Pbase zero_type;

extern int byte_offset;
extern int bit_offset;
extern int max_align;
extern int stack_size;
extern int enum_count;
extern int const_save;
#endif

extern Pexpr dummy;	/* the empty expression */
extern Pexpr zero;
extern Pexpr one;
extern Pname sta_name;	/* qualifier for unary :: */

#define TO_DEL(p)	((p) && (p)->permanent==0)
#define PERM(p)		(p)->permanent=1
#define UNPERM(p)	(p)->permanent=0

struct node {
	TOK	base;
	TOK	n_key;	/* for names in table: class */
	bit	permanent;
};

#ifndef GRAM
extern Pclass Ebase, Epriv;	/* lookc return values */
#endif

/* a table is a node only to give it a "base" for debugging */
struct table {
	TOK	base;
	TOK	n_key;
	bit	permanent;
	char	init_stat;	/* ==0 if block(s) of table not simplified,
				   ==1 if simplified but had no initializers,
				   ==2 if simplified and had initializers.
				*/
	short	size;
	short	hashsize;
	short	free_slot;	/* next free slot in entries */
	Pname*	entries;
	short*	hashtbl;
	Pstmt	real_block;	/* the last block the user wrote,
				   not one of the ones cfront created
				*/
	Ptable	next;		/* table for enclosing scope */
	Pname	t_name;		/* name of the table */
};

#ifndef GRAM
extern bit Nold;
extern bit vec_const, fct_const;
#endif

extern Plist modified_tn;
extern void restore();
extern void set_scope();
extern Pbase start_cl();
extern void end_cl();
extern Pbase end_enum();

/************ types : basic types, aggregates, declarators ************/

#ifndef GRAM
extern bit new_type;
extern Pname cl_obj_vec;
extern Pname eobj;
#endif


#define DEFINED 01	/* definition fed through ?::dcl() */
#define SIMPLIFIED 02	/* in ?::simpl() */
#define DEF_SEEN 04	/* definition seen, but not processed */
			/*   used for class members in norm.c */
#define IN_ERROR 010

struct type {
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;	/* flags DEF_SEEN, DEFINED, SIMPLIFIED, IN_ERROR
					not used systematically yet
				*/
};

struct enumdef {	/* ENUM */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	bit	e_body;
	short	no_of_enumerators;
	Pname	mem;
};

struct classdef {	/* CLASS */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	bit	pubbase;
	bit	c_body;		/* print definition only once */
	TOK	csu;		/* CLASS, STRUCT, UNION, or ANON */
	char	obj_align;
	char	bit_ass;	/* 1 if no member has operator=() */
	char	virt_count;	/* number of virtual functions
				   incl. virtuals in base classes */
	Pname	clbase;		/* base class */
	char*	string;		/* name of class */
	Pname	mem_list;
	Ptable	memtbl;
	int	obj_size;
	int	real_size;	/* obj_size - alignment waste */
	Plist	friend_list;
	Pname	pubdef;
	Plist	tn_list;	/* list of member names hiding type names */
	Pclass	in_class;	/* enclosing class, or 0 */
	Ptype	this_type;
	Pname*	virt_init;	/* vector of jump table initializers */
	Pname	itor;		/* constructor X(X&) */
	Pname	conv;		/* operator T() chain */
};

/*	ZTYPE CHAR SHORT INT LONG FLOAT DOUBLE
 *	FIELD EOBJ COBJ TYPE ANY
 *
 *	used for gathering all the attributes
 *	for a list of declarators
 *
 *	ZTYPE is the(generic) type of ZERO
 *	ANY is the generic type of an undeclared name
 */
struct basetype {
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	bit	b_unsigned;
	bit	b_const;
	bit	b_typedef;
	bit	b_inline;
	bit	b_virtual;
	bit	b_short;
	bit	b_long;
	char	b_bits;		/* number of bits in field */
	char	b_offset;	/* bit offset of field */
	TOK	b_sto;		/* AUTO STATIC EXTERN REGISTER 0 */
	Pname	b_name;		/* name of non-basic type */
	Ptable	b_table;	/* memtbl for b_name, or 0 */
	Pexpr	b_field; 	/* field size expression for a field */
	Pname	b_xname;	/* extra name */
	Ptype	b_fieldtype;
};


struct fct {	/* FCT */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	TOK	nargs;
	TOK	nargs_known;	/* KNOWN, ELLIPSIS, or 0 */
	char	f_virtual;	/* 1+index in virtual table, or 0 */
	char	f_inline;	/* 1 if inline, 2 if being expanded, else 0  */
	Ptype	returns;
	Pname	argtype;
	Ptype	s_returns;
	Pname	f_this;
	Pclass	memof;		/* member of class memof */
	Pstmt	body;
	Pname	f_init;		/* base/member initializers */
				/* null name => base class init; */
				/* ids => member classes(with ctors) */
	Pexpr	b_init;		/* base class initializer */
				/* ctor call after fct.dcl() */
	Pexpr	f_expr;		/* body expanded into an expression */
	Pexpr	last_expanded;
	Pname	f_result;	/* extra second argument of type X& */
};


struct name_list {
	Pname	f;
	Plist	l;
};

#ifndef GRAM
struct gen {		/* OVERLOAD */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	Plist	fct_list;
	char*	string;
};
#endif

struct pvtyp {
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	Ptype	typ;
};

struct vec {
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	Ptype	typ;
	Pexpr	dim;
	int	size;	
};

struct ptr { 			/* PTR, RPTR i.e. reference */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	bit	defined;
	Ptype	typ;
	Pclass	memof;	/* pointer to member of memof: memof::* */
	bit	rdo;		/* "*const" */
};

#ifndef GRAM
extern bit vrp_equiv;
#endif


/****************************** constants ********************************/

		/* STRING ZERO ICON FCON CCON ID */
		/* IVAL FVAL LVAL */

/***************************** expressions ********************************/

#ifndef GRAM
extern Pexpr next_elem();
extern void new_list();
extern void list_check();
extern Pexpr ref_init();
extern Pexpr class_init();
extern Pexpr check_cond();
#endif

/* IMPORTANT:	all expressions are of sizeof(expr)
 *	DEREF		=>	*e1(e2==0) OR e1[e2]
 *	UMINUS		=>	-e2
 *	INCR(e1==0)	=>	++e2
 *	INCR(e2==0)	=>	e1++
 *	CM		=>	e1 , e2
 *	ILIST		=>	LC e1 RC  (an initializer list)
 *	a Pexpr may denote a name
 */
union type_u1 {
	Ptype	tp;
	int	syn_class;
};
union type_u2 {
	Pexpr	e1;
	char*	string;
	int	i1;
};
union type_u3 {
	Pexpr	e2;
	Pexpr	n_initializer;
	char*	string2;
};
union type_u4 {			/* used by the derived classes */
	Ptype	tp2;
	Pname	fct_name;
	Pexpr	cond;
	Pname	mem;
	Ptype	as_type;
	Ptable	n_table;
	Pin	il;
};

struct expr {	 	/* PLUS, MINUS, etc. */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	union type_u1	u1;
	union type_u2	u2;
	union type_u3	u3;
	union type_u4	u4;
};


/************************* names(are expressions) ****************************/

struct name {			/* NAME TNAME and the keywords in the ktbl */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	union type_u1	u1;
	union type_u2	u2;
	union type_u3	u3;
	union type_u4	u4;
	TOK	n_oper;		/* name of operator or 0 */
	TOK	n_sto;		/* EXTERN STATIC AUTO REGISTER ENUM 0 */
	TOK	n_stclass;	/* STATIC AUTO REGISTER 0 */
	TOK	n_scope;	/* EXTERN STATIC FCT ARG PUBLIC 0  */
	unsigned char	n_union;	/* 0 or union index */
	bit	n_evaluated;	/* 0 or n_val holds the value */
	bit	n_xref;		/* argument of type X(X&) */
	unsigned char	lex_level;
	TOK	n_protect;	/* PROTECTED(<=>n_scope==0) or 0 */
	short	n_addr_taken;
	short	n_used;
	short	n_assigned_to;
	Loc	where;
	int	n_val;		/* the value of n_initializer */
				/* also used as the argument number */
				/* for inline arguments */
	int	n_offset;	/* byte offset in frame or struct */
	Pname	n_list;
	Pname	n_tbl_list;
	union {
		Pname	n_qualifier;	/* name of containing class */
		Ptable	n_realscope;	/* for labels(always entered in
					   function table) the table for the
					   actual scope in which label occurred.
					*/
	} u5;
};


#ifndef GRAM
extern int friend_in_class;
#endif

/******************** statements *********************************/

/* IMPORTANT: all statement nodes have sizeof(stmt) */
struct stmt {			/* BREAK CONTINUE DEFAULT */
	TOK	base;
	TOK	n_key;
	bit	permanent;
	Pstmt	s;
	Pstmt	s_list;
	Loc	where;
	union {
		Pname	d;
		Pexpr	e2;
		Pstmt	has_default;
		int	case_value;
		Ptype	ret_tp;
	} u1;
	union {
		Pexpr	e;
		bit	own_tbl;
		Pstmt	s2;
	} u2;
	Ptable	memtbl;
	union {
		Pstmt	for_init;
		Pstmt	else_stmt;
		Pstmt	case_list;
		bit	empty;
	} u3;
};


#ifndef GRAM
extern char* Neval;
extern Pname dcl_temp();
extern char* temp();
extern Ptable scope;
extern Ptable expand_tbl;
extern Pname expand_fn;
#endif

struct nlist {
	Pname	head;
	Pname	tail;
};

extern Pname name_unlist();

struct slist {
	Pstmt	head;
	Pstmt	tail;
};

extern Pstmt stmt_unlist();

struct elist {
	Pexpr	head;
	Pexpr	tail;
};

extern Pexpr expr_unlist();

#ifndef GRAM
struct dcl_context {
	Pname	c_this;	/* current fct's "this" */
	Ptype	tot;	/* type of "this" or 0 */
	Pname	not;	/* name of "this"'s class or 0 */
	Pclass	cot;	/* the definition of "this"'s class */
	Ptable	ftbl;	/* current fct's symbol table */
	Pname	nof;	/* current fct's name */
};

#define MAXCONT	20
extern struct dcl_context * cc;
extern struct dcl_context ccvec[MAXCONT];
#endif

extern void yyerror();
extern TOK back;


#ifndef GRAM
extern char* line_format;

extern Plist isf_list;
extern Pstmt st_ilist;
extern Pstmt st_dlist;
extern Ptable sti_tbl;
extern Ptable std_tbl;
extern Pexpr try_to_coerce();
extern bit can_coerce();
extern Ptype np_promote();
extern void new_key();

extern Pname dcl_list;
extern int over_call();
extern Pname Nover;
extern Pname Ntncheck;
extern Pname Ncoerce;
extern Nover_coerce;

#define MIA	8
struct iline {
	Pname	fct_name;	/* fct called */
	Pin	i_next;
	Ptable	i_table;
	Pname	local[MIA];	/* local variable for arguments */
	Pexpr	arg[MIA];	/* actual arguments for call */
	Ptype	tp[MIA];	/* type of formal arguments */
};

extern Pexpr curr_expr;
extern Pin curr_icall;
#define FUDGE111 111

extern Pstmt curr_loop;
extern Pstmt curr_block;
extern Pstmt curr_switch;
extern bit arg_err_suppress;
extern struct loc last_line;

extern no_of_undcl;
extern no_of_badcall;
extern Pname undcl, badcall;

extern int str_to_int();
extern int c_strlen();
#endif

#ifndef GRAM
extern Pname vec_new_fct;
extern Pname vec_del_fct;

extern int Nstd; /* standard coercion used(derived* =>base* or int=>long or ...) */

extern int stcount;	/* number of names generated using make_name() */

extern Pname find_hidden();
Pexpr replace_temp();
void make_res();
Pexpr ptr_init();	

#endif

extern bit fake_sizeof;	/* suppress error message for ``int v[];'' */

extern TOK lalex();

#ifdef DEBUG
#define DB(a) fprintf a
#else
#define DB(a) /**/
#endif

/* end */

#define	is_simple(p)	(((p)->csu==CLASS)?0:(p)->csu)
#define	elist_add(p,e)	{(p)->tail->u3.e2 = e;(p)->tail = e; }
#define	nlist_add(p,n)	{(p)->tail->n_list = n;(p)->tail = n; }
#define	slist_add(p,s)	{(p)->tail->s_list = s;(p)->tail = s; }
#define	stack()		{ cc++; *cc = *(cc-1); }
#define	unstack()	{ cc--; }

#ifndef GRAM
#define	addrof(p)	new_ptr(PTR,(p),0)
#define	has_ctor(p)	look((p)->memtbl, "_ctor", 0)
#define	has_dtor(p)	look((p)->memtbl, "_dtor", 0)
#define	has_itor(p)	(p)->itor
#define	num_ptr(p,oo)	type_kind((p),(oo),P)
#define	numeric(p,oo)	type_kind((p),(oo),N)
#define	integral(p,oo)	type_kind((p),(oo),I)
#define	max(p)		(p)->free_slot-1
#define	Set_scope(p,t)	(p)->next = (t)
#define	Set_name(p,n)	(p)->t_name = (n)
#define	take_addr(p)	(p)->n_addr_taken++
#define	use(p)		(p)->n_used++
#endif

extern	Pname		name_normalize();
extern	Pname		insert();
extern	Pname		look();
extern	char		*signature();
extern	Pbase		type_adj();
extern	Pbase		base_adj();
extern	Pbase		name_adj();
extern	Pname		aggr();
extern	void		argdcl();
extern	Pname		tname();
extern	void		hide();

#ifndef GRAM
extern	void		table_del();
extern	void		type_print();
extern	void		type_dcl_print();
extern	void		type_del();
extern	void		type_dcl();
extern	void		enumdef_print();
extern	void		enumdef_dcl();
extern	void		enumdef_simpl();
extern	void		classdef_print();
extern	void		classdef_simpl();
extern	void		classdef_dcl();
extern	Pbase		basetype_check();
extern	Ptype		fct_normalize();
extern	void		fct_dcl();
extern	void		fct_simpl();
extern	Pexpr		fct_expand();
extern	Ptype		vec_normalize();
extern	Ptype		ptr_normalize();
extern	void		expr_del();
extern	void		expr_print();
extern	Pexpr		expr_typ();
extern	void		expr_simpl();
extern	Pexpr		expr_expand();
extern	void		call_simpl();
extern	Pexpr		call_expand();
extern	Pname		name_dcl();
extern	void		name_simpl();
extern	void		name_del();
extern	void		name_print();
extern	void		name_dcl_print();
extern	void		stmt_del();
extern	void		stmt_print();
extern	void		stmt_dcl();
extern	Pstmt		stmt_simpl();
extern	Pstmt		stmt_expand();
extern	void		block_dcl();
extern	Pstmt		block_simpl();

extern	void		put();
extern	void		putline();
extern	void		grow();
extern	Pname		get_mem();
extern	Pname		lookc();
extern	Pexpr		find_name();
extern	Pname		is_cl_obj();	/* sets cl_obj_vec */
extern	int		is_ref();
extern	int		tsizeof();
extern	bit		tconst();
extern	TOK		set_const();
extern	int		type_align();
extern	TOK		type_kind();
extern	bit		vec_type();
extern	bit		type_check();
extern	Ptype		type_deref();
extern	void		print_members();
extern	bit		has_friend();
extern	bit		baseofname();
extern	bit		baseofclass();
extern	Pname		has_oper();
extern	Pname		has_ictor();
extern	Pbase		arit_conv();
extern	Pexpr		base_init();
extern	Pexpr		mem_init();
extern	Pname		add();
extern	Pname  		find();
extern	int		eval();
extern	int		lval();
extern	Ptype		fct_call();
extern	Pexpr		address();
extern	Pexpr		contents();
extern	bit		not_simple_exp();
extern	Pexpr		try_to_overload();
extern	Pexpr		docast();
extern	Pexpr		dovalue();
extern	Pexpr		donew();
extern	void		simpl_new();
extern	void		simpl_delete();
extern	int		no_of_names();
extern	void		assign();
extern	void		check_oper();
extern	void		field_align();
extern	Pname		dofct();
extern	void		reached();
extern	Pstmt		copy();
extern	void		add_list();
#endif

extern	Pfct		fct_ctor();
extern	Pexpr		init_tmp();
extern	Pname		make_tmp();
extern	Pbase		new_basetype();
extern	Pstmt		new_block();
extern	Pexpr		new_call();
extern	Pstmt		new_estmt();
extern	Pexpr		new_expr();
extern	Pstmt		new_ifstmt();
extern	Pexpr		new_ival();
extern	Plist		new_nalist();
extern	Pname		new_name();
extern	Pptr		new_ptr();
extern	Pexpr		new_ref();
extern	Pstmt		new_stmt();
extern	Ptable		new_table();
extern	Pexpr		new_texpr();
extern	Pvec		new_vec();
