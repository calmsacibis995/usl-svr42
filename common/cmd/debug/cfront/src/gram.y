/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ident	"@(#)debugger:cfront/src/gram.y	1.1"
/*************************************************************************

gram.y:
	
	This is the C++syntax analyser.

	Syntax extensions for error handling:
		nested functions
		any expression can be empty
		any expression can be a constant_expression

	A call to error() does not change the parser's state

***************************************************************************/

#include "cfront.h"
#include "size.h"

#define YYMAXDEPTH 300

static int cdi = 0;
static Pnlist cd = 0, cd_vec[BLMAX];
static char stmt_seen = 0, stmt_vec[BLMAX];
static Plist tn_vec[BLMAX];

void sig_name();	/* fcts put into norm2.c just to get them out of gram.y */
Ptype tok_to_type();
void memptrdcl();

#define lex_unget(x) back = x

#define Ndata(a,b)	name_normalize((b),(Pbase)(a),0,0)
#define Ncast(a,b)	name_normalize((b),(Pbase)(a),0,1)
#define Nfct(a,b,c)	name_normalize((b),(Pbase)(a),(Pstmt)(c),0)
#define Ncopy(n)	(((n)->base==TNAME)? new_name((n)->u2.string):(n))

#define Finit(p)	((Pfct)(p))->f_init
#define Fargdcl(p,q,r)	argdcl((Pfct)(p),(q),(r))
#define Freturns(p)	((Pfct)(p))->returns
#define Vtype(v)	((Pvec)(v))->typ
#define Ptyp(p)		((Pptr)(p))->typ

		/* avoid redefinitions */
#undef EOFTOK
#undef ASM
#undef BREAK
#undef CASE
#undef CONTINUE
#undef DEFAULT
#undef DELETE
#undef DO
#undef ELSE
#undef ENUM
#undef FOR
#undef FORTRAN
#undef GOTO
#undef IF
#undef NEW
#undef OPERATOR
#undef RETURN
#undef SIZEOF
#undef SWITCH
#undef THIS
#undef WHILE
#undef LP
#undef RP
#undef LB
#undef RB
#undef REF
#undef DOT	
#undef NOT	
#undef COMPL	
#undef MUL	
#undef AND	
#undef PLUS	
#undef MINUS	
#undef ER	
#undef OR	
#undef ANDAND
#undef OROR
#undef QUEST
#undef COLON
#undef ASSIGN
#undef CM
#undef SM	
#undef LC	
#undef RC
#undef ID
#undef STRING
#undef ICON
#undef FCON	
#undef CCON	
#undef ZERO
#undef ASOP
#undef RELOP
#undef EQUOP
#undef DIVOP
#undef SHIFTOP
#undef ICOP
#undef TYPE
#undef TNAME
#undef EMPTY
#undef NO_ID
#undef NO_EXPR
#undef ELLIPSIS
#undef AGGR
#undef MEM
#undef CAST
#undef ENDCAST
#undef MEMPTR
#undef PR
%}

%union {
	char*	s;
	TOK	t;
	int	i;
	Loc	l;
	Pname	pn;
	Ptype	pt;
	Pexpr	pe;
	Pstmt	ps;
	Pbase	pb;
	Pnlist	nl;
	Pslist	sl;
	Pelist	el;
	Pnode	p;	/* fudge: pointer to all class node objects */
}
%{
extern YYSTYPE yylval, yyval;
extern int yyparse();

static	Pslist	new_slist();
static	Pstmt	new_lstmt();
static	Pstmt	new_forstmt();
static	Pelist	new_elist();
static	Pexpr	new_qexpr();
static	void	look_for_hidden();
static	Pnlist	new_nlist();

Pname syn()
{
ll:
	switch (yyparse()) {
	case 0:		return 0;	/* EOF */
	case 1:		goto ll;	/* no action needed */
	default:	return yyval.pn;
	}
}

static void look_for_hidden(n, nn)
Pname n, nn;
{
	Pname nx;
	nx = look(ktbl, n->u2.string, HIDDEN);
	if (nx == 0) {
		V1.u1.p = (char *)n;
		V2.u1.p = (char *)nn;
		error("nonTN%n before ::%n", &V1, &V2, ea0, ea0);
	}
	nn->u5.n_qualifier = nx;
}
%}
/*
	the token definitions are copied from token.h,
	and all %token replaced by %token
*/
			/* keywords in alphabetical order */
%token EOFTOK		0
%token ASM		1
%token BREAK		3
%token CASE		4
%token CONTINUE		7
%token DEFAULT		8
%token DELETE		9
%token DO		10
%token ELSE		12
%token ENUM		13
%token FOR		16
%token FORTRAN		17
%token GOTO		19
%token IF		20
%token NEW		23
%token OPERATOR		24
%token RETURN		28
%token SIZEOF		30
%token SWITCH		33
%token THIS		34
%token WHILE		39

			/* operators in priority order(sort of) */
%token LP		40
%token RP		41
%token LB		42
%token RB		43
%token REF		44
%token DOT		45
%token NOT		46
%token COMPL		47
%token MUL		50
%token AND		52
%token PLUS		54
%token MINUS		55
%token ER		64
%token OR		65
%token ANDAND		66
%token OROR		67
%token QUEST		68
%token COLON		69
%token ASSIGN		70
%token CM		71
%token SM		72
%token LC		73
%token RC		74
%token CAST		113
%token ENDCAST		122
%token MEMPTR		173

			/* constants etc. */
%token ID		80
%token STRING		81
%token ICON		82
%token FCON		83
%token CCON		84

%token ZERO		86

			/* groups of tokens */
%token ASOP		90	/* op= */
%token RELOP		91	/* LE GE LT GT */
%token EQUOP		92	/* EQ NE */
%token DIVOP		93	/* DIV MOD */
%token SHIFTOP		94	/* LS RS */
%token ICOP		95	/* INCR DECR */

%token TYPE		97	/*	INT FLOAT CHAR DOUBLE
					REGISTER STATIC EXTERN AUTO
					CONST INLINE VIRTUAL FRIEND
					LONG SHORT UNSIGNED
					TYPEDEF */
%token TNAME		123
%token EMPTY		124
%token NO_ID		125
%token NO_EXPR		126
%token ELLIPSIS		155	/* ... */
%token AGGR		156	/* CLASS STRUCT UNION */
%token MEM		160	/* :: */
%token PR		175	/* PUBLIC PRIVATE PROTECTED */


%type <p>	external_def fct_dcl fct_def att_fct_def arg_dcl_list 
		base_init init_list binit
		data_dcl ext_def vec ptr
		type tp enum_dcl moe_list
		moe 
		tag class_head class_dcl cl_mem_list 
		cl_mem dl decl_list 
		fname decl initializer stmt_list
		block statement simple ex_list elist e  term prim
		cast_decl cast_type c_decl c_type c_tp
		arg_decl at arg_type arg_list arg_type_list
		new_decl new_type
		condition
		TNAME tn_list MEMPTR
%type <l>	LC RC SWITCH CASE DEFAULT FOR IF DO WHILE GOTO RETURN DELETE
		BREAK CONTINUE
%type <t>	oper
		EQUOP DIVOP SHIFTOP ICOP RELOP ASOP
		ANDAND OROR PLUS MINUS MUL ASSIGN OR ER AND 
		LP LB NOT COMPL AGGR
		TYPE PR
%type <s>	CCON ZERO ICON FCON STRING
%type <pn>	ID 

%left	EMPTY
%left	NO_ID
%left	RC LC ID BREAK CONTINUE RETURN GOTO DELETE DO IF WHILE FOR CASE DEFAULT
	AGGR ENUM TYPE
%left	NO_EXPR

%left	CM
%right	ASOP ASSIGN
%right	QUEST COLON
%left	OROR
%left	ANDAND
%left	OR
%left	ER
%left	AND
%left	EQUOP
%left	RELOP
%left	SHIFTOP
%left	PLUS MINUS
%left	MUL DIVOP MEMPTR
%right	NOT COMPL NEW
%right	CAST ICOP SIZEOF
%left	LB LP DOT REF MEM

%start ext_def

%%
/*
	this parser handles declarations one by one,
	NOT a complete .c file
*/






/************** DECLARATIONS in the outermost scope: returns Pname(in yylval) ***/

ext_def		:  external_def		{	return 2; }
		|  SM			{	return 1; }
		|  EOFTOK		{	return 0; }
		;

external_def	:  data_dcl
			{	modified_tn = 0; if ($<pn>1==0) $<i>$ = 1; }
		|  att_fct_def
			{	goto mod; }
		|  fct_def
			{	goto mod; }
		|  fct_dcl
			{ mod:	if (modified_tn) {
					restore();
					modified_tn = 0;
				}
			}
		|  ASM LP STRING RP SM
			{	Pname n;
				n = new_name(make_name('A'));
				n->u1.tp = (Ptype) new_basetype(ASM, 0);
				((Pbase)n->u1.tp)->b_name = (Pname)($<s>3);
				$$ = (Pnode)n;
			}
		;

fct_dcl		:  decl ASSIGN initializer SM
			{	errorFIPC('s',"T ofIdE too complicated(useTdef or leave out theIr)",
				ea0, ea0, ea0, ea0);
				goto fix;
			}
		|  decl SM
			{	Pname n;
				Ptype t;
			fix:
				if ((n=$<pn>1) == 0) {
					error("syntax error:TX", ea0,ea0,ea0,ea0);
					$$ = (Pnode)Ndata(defa_type,n);
				}
				else if ((t=n->u1.tp) == 0) {
					V1.u1.p = (char *)n;
					error("TX for%n", &V1, ea0, ea0, ea0);
					$$ = (Pnode)Ndata(defa_type,n);
				}
				else if (t->base==FCT) {
					if (((Pfct)t)->returns==0)
						$$ = (Pnode)Nfct(defa_type,n,0);
					else
						$$ = (Pnode)Ndata(0,n);
				}
				else {
					V1.u1.i = (int)t->base;
					V2.u1.p = (char *)n;
					error("syntax error:TX for%k%n", &V1,
						&V2, ea0, ea0);
					$$ = (Pnode)Ndata(defa_type,n);
				}
			}
		;


att_fct_def	:  type decl arg_dcl_list base_init block
			{	Pname n;
				n = Nfct($1,$<pn>2,$5);
				Fargdcl(n->u1.tp,name_unlist($<nl>3),n);
				Finit(n->u1.tp) = $<pn>4;
				$$ = (Pnode)n;
			} 
		;

fct_def		:  decl arg_dcl_list base_init block
			{	Pname n;
				n = Nfct(defa_type,$<pn>1,$4);
				Fargdcl(n->u1.tp,name_unlist($<nl>2),n);
				Finit(n->u1.tp) = $<pn>3;
				$$ = (Pnode)n;
			}
		;

base_init	:  COLON init_list
			{	$$ = $2; }
		|  %prec EMPTY
			{	$$ = 0; }
		;

init_list	:  binit
		|  init_list CM binit
			{	$<pn>$ = $<pn>3;  $<pn>$->n_list = $<pn>1; }
		;

binit		:  LP elist RP
			{	$<pn>$ = new_name(0);
				$<pn>$->u3.n_initializer = $<pe>2;
			}
		|  ID LP elist RP
			{	$<pn>$ = $1;
				$<pn>$->u3.n_initializer = $<pe>3;
			}
		;




/*************** declarations: returns Pname ********************/

arg_dcl_list	:  arg_dcl_list data_dcl
			{	if ($<pn>2 == 0)
					error("badAD", ea0, ea0, ea0, ea0);
				else if ($<pn>2->u1.tp->base == FCT) {
					V1.u1.p = (char *)($<pn>2);
					error("FD inAL(%n)", &V1, ea0, ea0, ea0);
				} else if ($1) {
					nlist_add($<nl>1, $<pn>2);
				} else
					$<nl>$ = new_nlist($<pn>2);
			}
		|  %prec EMPTY
			{	$$ = 0; }
		;

dl		:  decl
		|  ID COLON e		%prec CM
			{	$$ = (Pnode)($<pn>1);
				$<pn>$->u1.tp =(Ptype)new_basetype(FIELD,$<pn>3);
		 	}
		|  COLON e		%prec CM
			{	$$ = (Pnode)new_name(0);
				$<pn>$->u1.tp =(Ptype)new_basetype(FIELD,$<pn>2);
			}
		|  decl ASSIGN initializer
			{	Pexpr e;
				e = $<pe>3; 
				if (e == dummy) error("emptyIr", ea0,ea0,ea0,ea0);
				$<pn>1->u3.n_initializer = e; 
			}
		;

decl_list	:  dl
			{	if ($1) $<nl>$ = new_nlist($<pn>1); }
		|  decl_list CM dl
			{	if ($1)
					if ($3) {
						nlist_add($<nl>1, $<pn>3);
					} else
						error("DL syntax",ea0,ea0,ea0,ea0);
				else {
					if ($3) $<nl>$ = new_nlist($<pn>3);
					error("DL syntax", ea0, ea0, ea0, ea0);
				}
			}
		;

data_dcl	:  type decl_list SM	{ $$ = (Pnode)Ndata($1,name_unlist($<nl>2));}
		|  type SM		{ $$ = (Pnode)aggr($<pb>1); }
		
		;

tp		:  TYPE			{ $$ = (Pnode)new_basetype($<t>1,0); }
		|  TNAME		{ $$ = (Pnode)new_basetype(TYPE,$<pn>1);}
		|  class_dcl
		|  enum_dcl
		;

type		:  tp
		|  type TYPE		{ $$ = (Pnode)type_adj($<pb>1, $<t>2); }
		|  type TNAME		{ $$ = (Pnode)name_adj($<pb>1, $<pn>2); }
		|  type class_dcl	{ $$ = (Pnode)base_adj($<pb>1, $<pb>2); }
		|  type enum_dcl	{ $$ = (Pnode)base_adj($<pb>1, $<pb>2); }
		;




/***************** aggregate: returns Pname *****************/


enum_dcl	:  ENUM LC moe_list RC
			{	$$ = (Pnode)end_enum(0,$<pn>3); }
		|  ENUM tag LC moe_list RC
			{	$$ = (Pnode)end_enum($<pn>2,$<pn>4); }
		;

moe_list	:  moe
			{	if ($1) $<nl>$ = new_nlist($<pn>1); }
		|  moe_list CM moe
			{	if( $3)
					if ($1) {
						nlist_add($<nl>1, $<pn>3);
					 } else
						$<nl>$ = new_nlist($<pn>3);
			}
		;

moe		:  ID
			{	$$ = (Pnode)$<pn>1; $<pn>$->u1.tp = (Ptype)moe_type;}
		|  ID ASSIGN e
			{	$$ = (Pnode)$<pn>1;
				$<pn>$->u1.tp = (Ptype)moe_type;
				$<pn>$->u3.n_initializer = $<pe>3;
			}
		|  /* empty: handle trailing CM: enum e { a,b, }; */
			{	$$ = 0; }
		;


class_dcl	:  class_head cl_mem_list RC
			{	
				ccl->mem_list = name_unlist($<nl>2);
				end_cl();
			}
		|  class_head cl_mem_list RC TYPE
			{	
				ccl->mem_list = name_unlist($<nl>2);
				end_cl();
				error("`;' or declaratorX afterCD",ea0,ea0,ea0,ea0);
				lex_unget($4);
				/* lex_unget($4); but only one unget, sorry */
			}
		;

class_head	:  AGGR LC
			{	$$ = (Pnode)start_cl($<t>1,0,0); }
		|  AGGR tag LC
			{	$$ = (Pnode)start_cl($<t>1,$<pn>2,0); }
		|  AGGR tag COLON TNAME LC
			{	$$ = (Pnode)start_cl($<t>1,$<pn>2,$<pn>4);
				if ($<t>1 == STRUCT) ccl->pubbase = 1;
			}
		|  AGGR tag COLON PR TNAME LC
			{	
				$$ = (Pnode)start_cl($<t>1,$<pn>2,$<pn>5);
				ccl->pubbase = 1;
			}
/*
		|  AGGR tag COLON TNAME TNAME LC
			{	char* s = $<pn>4->u2.string;
				if (strcmp(s,"public")) {
					V1.u1.p = s;
					error("unX %s after : inCD",&V1,ea0,ea0,ea0);
				}
				$$ = (Pnode)start_cl($<t>1,$<pn>2,$<pn>5);
				ccl->pubbase = 1;
			}
		|  AGGR tag COLON TNAME ID LC
			{	char* s = $<pn>4->u2.string;
				if (strcmp(s,"public")) {
					V1.u1.p = s;
					error("unX %s after : inCD",&V1,ea0,ea0,ea0);
				}
				$$ = (Pnode)start_cl($<t>1,$<pn>2,$<pn>5);
				ccl->pubbase = 1;
			}
		|  AGGR tag COLON ID LC
			{	$$ = (Pnode)start_cl($<t>1,$<pn>2,$<pn>4);
				if ($<t>1 == STRUCT) ccl->pubbase = 1;
			}
*/
		;

tag		:  ID
			{ $$ = (Pnode)($1); }
		|  TNAME
		;

cl_mem_list	:  cl_mem_list cl_mem
			{	if ($2) {
					if ($1)
						add_list($<nl>1, $<pn>2);
					else
						$<nl>$ = new_nlist($<pn>2);
				}
			}
		|  %prec EMPTY
			{	$$ = 0; }
		;

cl_mem		:  data_dcl
		|  att_fct_def SM
		|  att_fct_def
		|  fct_def SM
		|  fct_def
		|  fct_dcl
		|  PR COLON
			{	$$ = (Pnode)new_name(0); $<pn>$->base = $<t>1; }
		|  tn_list tag SM 
			{	Pname n = Ncopy($<pn>2);
				n->u5.n_qualifier = $<pn>1;
				n->base = PUBLIC;
				$$ = (Pnode)n;
			}
		;



/************* declarators:	returns Pname **********************/

/*	a ``decl'' is used for function and data declarations,
		and for member declarations
		(it has a name)
	an ``arg_decl'' is used for argument declarations
		(it may or may not have a name)
	an ``cast_decl'' is used for casts
		(it does not have a name)
	a ``new_decl'' is used for type specifiers for the NEW operator
		(it does not have a name, and PtoF and PtoV cannot be expressed)
*/

fname		:  ID
			{	$$ = (Pnode)($<pn>1); }
		|  COMPL TNAME
			{	$$ = (Pnode)Ncopy($<pn>2);
				$<pn>$->n_oper = DTOR;
			}
		|  OPERATOR oper
			{	$$ = (Pnode)new_name(oper_name($2));
				$<pn>$->n_oper = $<t>2;
			}
		|  OPERATOR c_type
			{	Pname n;
				n = $<pn>2;
				n->u2.string = "_type";
				n->n_oper = TYPE;
				n->u3.n_initializer = (Pexpr)n->u1.tp;
				n->u1.tp = 0;
				$$ = (Pnode)n;
			}
		;

oper		:  PLUS
		|  MINUS
		|  MUL
		|  AND
		|  OR
		|  ER
		|  SHIFTOP
		|  EQUOP
		|  DIVOP
		|  RELOP
		|  ANDAND
		|  OROR
		|  LP RP	{	$$ = CALL; }
		|  LB RB	{	$$ = DEREF; }
		|  NOT
		|  COMPL
		|  ICOP
		|  ASOP
		|  ASSIGN
		|  NEW		{	$$ = NEW; }
		|  DELETE	{	$$ = DELETE; }
		|  REF		{	$$ = REF; }
		|  DOT		{	$$ = DOT; }
		;

tn_list		:  TNAME DOT
		   { 
	             if ( $<pn>1->u1.tp->base != COBJ ) {
			V1.u1.p = (char *)($<pn>1);
			error( "T of%n not aC", &V1, ea0, ea0, ea0);	
		     }
		   }
		|  TNAME MEM
		   { 
	             if ( $<pn>1->u1.tp->base != COBJ ) {
			V1.u1.p = (char *)($<pn>1);
			error( "T of%n not aC", &V1, ea0, ea0, ea0 );	
		     }
		   }
		|  tn_list TNAME DOT	{ error("CNs do not nest,ea0,ea0,ea0,ea0"); }
		|  tn_list ID DOT	{ error("CNs do not nest,ea0,ea0,ea0,ea0"); }
		;


decl		:  decl arg_list
			{	Freturns($2) = $<pn>1->u1.tp;
				$<pn>1->u1.tp = $<pt>2;
			}
		|  TNAME arg_list
			{	Pname n;
				n = $<pn>1;
				$$ = (Pnode)Ncopy(n);
				if (ccl && strcmp(n->u2.string,ccl->string)) hide(n);
				$<pn>$->n_oper = TNAME;
				Freturns($2) = $<pn>$->u1.tp;
				$<pn>$->u1.tp = $<pt>2;
			}
		|  decl LP elist RP
			/*	may be class object initializer,
				class object vector initializer,
				if not elist will be a CM or an ID
			*/
			{	TOK k;
				Pname l;
				k = 1;
				l = $<pn>3;
				if (fct_void && l==0) k = 0;
				$<pn>1->u1.tp = (Ptype)fct_ctor($<pn>1->u1.tp,l,k);
			}
		|  TNAME LP elist RP
			{	TOK k;
				Pname l;
				k = 1;
				l = $<pn>3;
				if (fct_void && l==0) k = 0;
				$$ = (Pnode)Ncopy($<pn>1);
				$<pn>$->n_oper = TNAME;
				$<pn>$->u1.tp = (Ptype)fct_ctor(0,l,k);
			} 
		|  TNAME LP MEMPTR decl RP arg_list
			{	memptrdcl($<pn>3,$<pn>1,$<pt>6,$<pn>4);
				$$ = $4;
			}
		|  fname
		|  ID DOT fname
			{	$$ = (Pnode)Ncopy($<pn>3);
				$<pn>$->u5.n_qualifier = $1;
			}
		|  tn_list fname
			{	$$ = $2;
				set_scope($<pn>1);
				$<pn>$->u5.n_qualifier = $<pn>1;
			}
		|  tn_list TNAME
			{	$$ = (Pnode)Ncopy($<pn>2);
				set_scope($<pn>1);
				$<pn>$->n_oper = TNAME;
				$<pn>$->u5.n_qualifier = $<pn>1;
			}
		|  ptr decl	%prec MUL
			{	Ptyp($1) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = $<pt>1;
				$$ = $2;
			}
		|  ptr TNAME	%prec MUL
			{	$$ = (Pnode)Ncopy($<pn>2);
				$<pn>$->n_oper = TNAME;
				hide($<pn>2);
				$<pn>$->u1.tp = $<pt>1;
			}
		|  TNAME vec	%prec LB
			{	$$ = (Pnode)Ncopy($<pn>1);
				$<pn>$->n_oper = TNAME;
				hide($<pn>1);
				$<pn>$->u1.tp = $<pt>2;
			}
		|  decl vec	%prec LB	
			{	Vtype($2) = $<pn>1->u1.tp;
				$<pn>1->u1.tp = $<pt>2;
			}
		|  LP decl RP arg_list
			{	Freturns($4) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = $<pt>4;
				$$ = $2;
			}
		|  LP decl RP vec
			{	Vtype($4) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = $<pt>4;
				$$ = $2;
			}
		;

arg_decl	:  ID
			{	$$ = (Pnode)($<pn>1); }
		|  ptr TNAME	%prec MUL
			{	$$ = (Pnode)Ncopy($<pn>2);
				$<pn>$->n_oper = TNAME;
				hide($<pn>2);
				$<pn>$->u1.tp = $<pt>1;
			}
		|  %prec NO_ID
			{	$$ = (Pnode)new_name(0); }
		|  ptr arg_decl		%prec MUL
			{	Ptyp($1) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = (Ptype)$1;
				$$ = $2;
			}
		|  arg_decl vec		%prec LB
			{	Vtype($2) = $<pn>1->u1.tp;
				$<pn>1->u1.tp = (Ptype)$2;
			}
		|  LP arg_decl RP arg_list
			{	Freturns($4) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = (Ptype)$4;
				$$ = $2;
			}
		|  LP arg_decl RP vec
			{	Vtype($4) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = (Ptype)$4;
				$$ = $2;
			}
		;

new_decl	:  %prec NO_ID
			{	$$ = (Pnode)new_name(0); }
		|  ptr new_decl		%prec MUL
			{	Ptyp($1) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = (Ptype)$1;
				$$ = $2;
			}
		|  new_decl vec		%prec LB
			{	Vtype($2) = $<pn>1->u1.tp;
				$<pn>1->u1.tp = (Ptype)$2;
			}
		;

cast_decl	:  %prec NO_ID
			{	$$ = (Pnode)new_name(0); }
		|  ptr cast_decl			%prec MUL
			{	Ptyp($1) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = (Ptype)$1;
				$$ = $2;
			}
		|  cast_decl vec			%prec LB
			{	Vtype($2) = $<pn>1->u1.tp;
				$<pn>1->u1.tp = (Ptype)$2;
			}
		|  LP cast_decl RP arg_list
			{	Freturns($4) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = $<pt>4;
				$$ = $2;
			}
		|  LP cast_decl RP vec
			{	Vtype($4) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = $<pt>4;
				$$ = $2;
			}
		;

c_decl		:  %prec NO_ID
			{	$$ = (Pnode)new_name(0); }
		|  ptr c_decl				%prec MUL
			{	Ptyp($1) = $<pn>2->u1.tp;
				$<pn>2->u1.tp = (Ptype)$1;
				$$ = $2;
			}
		;



/***************** statements: returns Pstmt *****************/

stmt_list	:  stmt_list statement
			{	if ($2)
					if ($1) {
						slist_add($<sl>1, $<ps>2);
					} else {
						$<sl>$ = new_slist($<ps>2);
						stmt_seen = 1;
					}
			}
		|  statement
			{	if ($1) {
					$<sl>$ = new_slist($<ps>1);
					stmt_seen = 1;
				}
			}
		;

condition	:  LP e RP
			{	$$ = $2;
				if ($<pe>$ == dummy)
					error("empty condition", ea0,ea0,ea0,ea0);
				stmt_seen = 1;
			}
		;

block		:  LC
			{	cd_vec[cdi] = cd;
				stmt_vec[cdi] = stmt_seen;
				tn_vec[cdi++] = modified_tn;
				cd = 0;
				stmt_seen = 0;
				modified_tn = 0;
			}
			stmt_list RC
			{	Pname n;
				Pstmt ss;
				n = name_unlist(cd);
				ss = stmt_unlist($<sl>3);
				$$ = (Pnode)new_block($<l>1,n,ss);
				if (modified_tn) restore();
				cd = cd_vec[--cdi];
				stmt_seen = stmt_vec[cdi];
				modified_tn = tn_vec[cdi];
				if (cdi < 0) {
					V1.u1.i = cdi;
					error('i',"block level(%d)",&V1,ea0,ea0,ea0);
				}
			}
		|  LC RC
			{	$$ = (Pnode)new_block($<l>1,0,0); }
		|  LC error RC
			{	$$ = (Pnode)new_block($<l>1,0,0); }
		;

simple		:  e
			{	$$ = (Pnode)new_estmt(SM,curloc,$<pe>1,0);	}
		|  BREAK
			{	$$ = (Pnode)new_stmt(0,BREAK,$<l>1,0); }
		|  CONTINUE
			{	$$ = (Pnode)new_stmt(0,CONTINUE,$<l>1,0); }
		|  RETURN e
			{	$$ = (Pnode)new_estmt(RETURN,$<l>1,$<pe>2,0); }
		|  GOTO ID
			{	$$ = (Pnode)new_lstmt(GOTO,$<l>1,$<pn>2,0); }
		|  DO { stmt_seen=1; } statement WHILE condition
			{	$$ = (Pnode)new_estmt(DO,$<l>1,$<pe>5,$<ps>3); }
		;

statement	:  simple SM
		|  ASM LP STRING RP SM
			{	
				if (stmt_seen)
					$$ = (Pnode)new_estmt(ASM,curloc,(Pexpr)$<s>3,0);
				else {
					Pname n = new_name(make_name('A'));
					n->u1.tp = (Ptype)new_basetype(ASM,(Pname)$<s>3);
					if (cd)
						add_list(cd,n);
					else
						cd = new_nlist(n);
					$$ = 0;
				}
			}
		|  data_dcl
			{	Pname n;
				n = $<pn>1;
				if (n)
					if (stmt_seen) {
						$$ = (Pnode)new_block(n->where,n,0);
						$<ps>$->base = DCL;
					}
					else {
						if (cd)
							add_list(cd, n);
						else
							cd = new_nlist(n);
						$$ = 0;
					}
			}
		|  att_fct_def
			{	Pname n;
				n = $<pn>1;
				lex_unget(RC);
				V1.u1.p = (char *)n;
				error(&n->where,
				"%n's definition is nested(did you forget a ``}''?)",
				&V1, ea0, ea0, ea0);
				if (cd)
					add_list(cd, n);
				else
					cd = new_nlist(n);
				$$ = 0;
			}
		|  block
		|  IF condition statement
			{	$$ = (Pnode)new_ifstmt($<l>1,$<pe>2,$<ps>3,0); }
		|  IF condition statement ELSE statement
			{	$$ = (Pnode)new_ifstmt($<l>1,$<pe>2,$<ps>3,$<ps>5); }
		|  WHILE condition statement
			{	$$ = (Pnode)new_estmt(WHILE,$<l>1,$<pe>2,$<ps>3); }
		|  FOR LP { stmt_seen=1; } statement e SM e RP statement
			{	$$ = (Pnode)new_forstmt($<l>1,$<ps>4,$<pe>5,$<pe>7,$<ps>9); }
		|  SWITCH condition statement
			{	$$ = (Pnode)new_estmt(SWITCH,$<l>1,$<pe>2,$<ps>3); }
		|  ID COLON { $$ = (Pnode)$1; stmt_seen=1; } statement
			{	Pname n;
				n = $<pn>3;
				$$ = (Pnode)new_lstmt(LABEL,n->where,n,$<ps>4);
			}
		|  CASE { stmt_seen=1; } e COLON statement
			{	if ($<pe>3 == dummy)
					error("empty case label",ea0,ea0,ea0,ea0);
				$$ = (Pnode)new_estmt(CASE,$<l>1,$<pe>3,$<ps>5);
			}
		|  DEFAULT COLON { stmt_seen=1; } statement
			{	$$ = (Pnode)new_stmt(0,DEFAULT,$<l>1,$<ps>4); }
		;



/********************* expressions: returns Pexpr **************/


elist		: ex_list
			{	Pexpr e;
				e = expr_unlist($<el>1);
				while (e && e->u2.e1==dummy) {
					register Pexpr ee2 = e->u3.e2;
					if (ee2) error("EX inEL",ea0,ea0,ea0,ea0);
					expr__dtor(e);
					e = ee2;
				}
				$$ = (Pnode)e;
			}

ex_list		:  initializer		%prec CM
			{	$<el>$ = new_elist(new_expr(0,ELIST,$<pe>1,0)); }
		|  ex_list CM initializer
			{	Pexpr e;
				e = new_expr(0,ELIST,$<pe>3,0);
				elist_add($<el>1, e);
			}
		;

initializer	:  e				%prec CM
		|  LC elist RC
			{	Pexpr e;
				if ($2)
					e = $<pe>2;
				else
					e = new_expr(0,ELIST,dummy,0);
				$$ = (Pnode)new_expr(0,ILIST,e,0);
			}
		;



e		:  e ASSIGN e
			{	binop:	$$ = (Pnode)new_expr(0,$<t>2,$<pe>1,$<pe>3); }
		|  e PLUS e	{	goto binop; }
		|  e MINUS e	{	goto binop; }
		|  e MUL e	{	goto binop; }
		|  e AND e	{	goto binop; }
		|  e OR e	{	goto binop; }
		|  e ER e	{	goto binop; }
		|  e SHIFTOP e	{ 	goto binop; }
		|  e EQUOP e	{	goto binop; }
		|  e DIVOP e	{	goto binop; }
		|  e RELOP e	{	goto binop; }
		|  e ANDAND e	{	goto binop; }
		|  e OROR e	{	goto binop; }
		|  e ASOP e	{	goto binop; }
		|  e CM e	{	goto binop; }
		|  e QUEST e COLON e
			{	$$ = (Pnode)new_qexpr($<pe>1,$<pe>3,$<pe>5); }
		|  DELETE term 
			{	$$ = (Pnode)new_expr(0,DELETE,$<pe>2,0); }
		|  DELETE LB e RB term
			{	$$ = (Pnode)new_expr(0,DELETE,$<pe>5,$<pe>3); }
		|  term
		|  %prec NO_EXPR
			{	$$ = (Pnode)dummy; }
		;

term		:  TYPE LP elist RP
			{ 	$$ = (Pnode)new_texpr(VALUE,tok_to_type($<t>1),$<pe>3); }
		|  TNAME LP elist RP
			{	$$ = (Pnode)new_texpr(VALUE,$<pn>1->u1.tp,$<pe>3); }
		|  NEW new_type
			{	Ptype t = $<pn>2->u1.tp;
				$$ = (Pnode)new_texpr(NEW,t,0);
			}
		|  NEW LP new_type RP
			{	Ptype t = $<pn>3->u1.tp;
				$$ = (Pnode)new_texpr(NEW,t,0);
			}
		|  term ICOP
			{	$$ = (Pnode)new_expr(0,$<t>2,$<pe>1,0); }
		|  CAST cast_type ENDCAST term %prec ICOP
			{	$$ = (Pnode)new_texpr(CAST,$<pn>2->u1.tp,$<pe>4); }
		|  MUL term
			{	$$ = (Pnode)new_expr(0,DEREF,$<pe>2,0); }
		|  AND term
			{	$$ = (Pnode)new_expr(0,ADDROF,0,$<pe>2); }
		|  MINUS term
			{	$$ = (Pnode)new_expr(0,UMINUS,0,$<pe>2); }
		|  PLUS term
			{	$$ = (Pnode)new_expr(0,UPLUS,0,$<pe>2); }
		|  NOT term
			{	$$ = (Pnode)new_expr(0,NOT,0,$<pe>2); }
		|  COMPL term
			{	$$ = (Pnode)new_expr(0,COMPL,0,$<pe>2); }
		|  ICOP term
			{	$$ = (Pnode)new_expr(0,$<t>1,0,$<pe>2); }
		|  SIZEOF term
			{	$$ = (Pnode)new_texpr(SIZEOF,0,$<pe>2); }
		|  SIZEOF CAST cast_type ENDCAST %prec SIZEOF
			{	$$ = (Pnode)new_texpr(SIZEOF,$<pn>3->u1.tp,0); }
		|  term LB e RB
			{	$$ = (Pnode)new_expr(0,DEREF,$<pe>1,$<pe>3); }
/*		|  term NOT term %prec LB
			{	$$ = (Pnode)new_expr(0,DEREF,$<pe>1,$<pe>3); }
*/
		|  term LP elist RP
			{	Pexpr ee;
				Pexpr e;
				ee = $<pe>3;
				e = $<pe>1;
				if (e->base == NEW)
					e->u2.e1 = ee;
				else
					$$ = (Pnode)new_call(e,ee);
			}
		|  term REF prim
			{	$$ = (Pnode)new_ref(REF,$<pe>1,$<pn>3); }
		|  term REF MUL term
			{	$$ = (Pnode)new_expr(0,REF,$<pe>1,$<pe>4); }
		|  term REF TNAME
			{	$$ = (Pnode)new_ref(REF,$<pe>1,Ncopy($<pn>3)); }
		|  term DOT prim
			{	$$ = (Pnode)new_ref(DOT,$<pe>1,$<pn>3); }
		|  term DOT MUL term
			{	$$ = (Pnode)new_expr(0,DOT,$<pe>1,$<pe>4); }
		|  term DOT TNAME
			{	$$ = (Pnode)new_ref(DOT,$<pe>1,Ncopy($<pn>3)); }
		|  MEM tag
			{	$$ = (Pnode)Ncopy($<pn>2);
				$<pn>$->u5.n_qualifier = sta_name;
			}
		|  prim
		|  LP e RP
			{	$$ = (Pnode)$2; }
		|  ZERO
			{	$$ = (Pnode)zero; }
		|  ICON
			{	$$ = (Pnode)new_expr(0,ICON,0,0);
				$<pe>$->u2.string = $<s>1;
			}
		|  FCON
			{	$$ = (Pnode)new_expr(0,FCON,0,0);
				$<pe>$->u2.string = $<s>1;
			}
		|  STRING
			{	$$ = (Pnode)new_expr(0,STRING,0,0);
				$<pe>$->u2.string = $<s>1;
			}
		|  CCON
			{	$$ = (Pnode)new_expr(0,CCON,0,0);
				$<pe>$->u2.string = $<s>1;
			}
		|  THIS
			{	$$ = (Pnode)new_expr(0,THIS,0,0); }
		;

prim		:  ID
			{	$$ = (Pnode)$<pn>1; }
		|  TNAME MEM tag
			{	$$ = (Pnode)Ncopy($<pn>3);
				$<pn>$->u5.n_qualifier = $<pn>1;
			}
		|  ID MEM tag
			{	$$ = (Pnode)Ncopy($<pn>3);
				look_for_hidden($<pn>1,$<pn>$);
			}
		|  OPERATOR oper
			{	$$ = (Pnode)new_name(oper_name($2));
				$<pn>$->n_oper = $<t>2;
			}
		|  TNAME MEM OPERATOR oper
			{	$$ = (Pnode)new_name(oper_name($4));
				$<pn>$->n_oper = $<t>4;
				$<pn>$->u5.n_qualifier = $<pn>1;
			}
		|  ID MEM OPERATOR oper
			{	$$ = (Pnode)new_name(oper_name($4));
				$<pn>$->n_oper = $<t>4;
				look_for_hidden($<pn>1,$<pn>$);
			}
		|  OPERATOR c_type
			{	$$ = $2;
				sig_name($<pn>$);
			}
		|  TNAME MEM OPERATOR c_type
			{	$$ = $4;
				sig_name($<pn>$);
				$<pn>$->u5.n_qualifier = $<pn>1;
			}
		|  ID MEM OPERATOR c_type
			{	$$ = $4;
				sig_name($<pn>$);
				look_for_hidden($<pn>1,$<pn>$);
			}
		;



/****************** abstract types(return type Pname) *************/

cast_type	:  type cast_decl
			{	$$ = (Pnode)Ncast($1,$<pn>2); }
		;

c_tp		:  TYPE	
			{	$$ = (Pnode)new_basetype($<t>1,0); }
		|  TNAME
			{	$$ = (Pnode)new_basetype(TYPE,$<pn>1); }
		;

c_type		:  c_tp c_decl
			{	$$ = (Pnode)Ncast($1,$<pn>2); }
		;

new_type	:  type new_decl
			{	$$ = (Pnode)Ncast($1,$<pn>2); };

arg_type	:  type arg_decl
			{	$$ = (Pnode)Ndata($1,$<pn>2); }
		|  type arg_decl ASSIGN initializer
			{	$$ = (Pnode)Ndata($1,$<pn>2);
				$<pn>$->u3.n_initializer = $<pe>4;
			}
		;

arg_list	:  LP arg_type_list RP
			{	$$ = (Pnode)fct_ctor(0,name_unlist($<nl>2),1); }
		|  LP arg_type_list ELLIPSIS RP
			{	$$ = (Pnode)fct_ctor(0,name_unlist($<nl>2),ELLIPSIS); }
		|  LP arg_type_list CM ELLIPSIS RP
			{	$$ = (Pnode)fct_ctor(0,name_unlist($<nl>2),ELLIPSIS); }
		;

arg_type_list	:  arg_type_list CM at
			{	if ($3)
					if ($1) {
						nlist_add($<nl>1, $<pn>3);
					} else {
						error("AD syntax",ea0,ea0,ea0,ea0);
						$<nl>$ = new_nlist($<pn>3); 
					}
				else
					error("AD syntax",ea0,ea0,ea0,ea0);
			}
		|  at	%prec CM
			{	if ($1) $<nl>$ = new_nlist($<pn>1); }
		;


at		:  arg_type
		|  %prec EMPTY
			{	$$ = 0; }
		;


ptr		:  MUL %prec NO_ID
			{	$$ = (Pnode)new_ptr(PTR,0,0); }
		|  AND %prec NO_ID
			{	$$ = (Pnode)new_ptr(RPTR,0,0); }
		|  MUL TYPE
			{	TOK t;
				t = $<t>2;
				switch (t) {
				case VOLATILE:
					V1.u1.i = (int)t;
					error('w',"\"%k\" not implemented(ignored)",
					&V1, ea0, ea0, ea0);
					$$ = (Pnode)new_ptr(PTR,0,0);
					break;
				default:
					V1.u1.i = (int)t;
					error("syntax error: *%k",&V1,ea0,ea0,ea0);
				case CONST:
					$$ = (Pnode)new_ptr(PTR,0,1);
				}
			}
		|  AND TYPE	
			{	TOK t;
				t = $<t>2;
				switch (t) {
				case VOLATILE:
					V1.u1.i = (int)t;
					error('w',"\"%k\" not implemented(ignored)",
					&V1, ea0, ea0, ea0);
					$$ = (Pnode)new_ptr(RPTR,0,0);
					break;
				default:
					V1.u1.i = (int)t;
					error("syntax error: &%k",&V1,ea0,ea0,ea0);
				case CONST:
					$$ = (Pnode)new_ptr(RPTR,0,1);
				}
			}
		| MEMPTR %prec NO_ID
			{	Pptr p;
				p = new_ptr(PTR,0,0);
				p->memof = (Pclass)((Pbase)$<pn>1->u1.tp)->b_name->u1.tp;
				$$ = (Pnode)p;
			}
	
		;

vec		:  LB e RB
			{	$$ = (Pnode)new_vec(0,($<pe>2!=dummy)?$<pe>2:0 ); }
/*		|  NOT term %prec LB
			{	$$ = (Pnode)new_vec(0,$<pe>2); }
*/
		|  NOT %prec LB
			{	$$ = (Pnode)new_vec(0,0); }
		;

%%

static Pslist new_slist(s)
Pstmt s;
{
	Pslist Xthis_slist;

	Xthis_slist = (Pslist) new(sizeof(struct slist));
	Nl++;
	Xthis_slist->head = Xthis_slist->tail = s;

	return Xthis_slist;
}

static Pstmt new_lstmt(bb, ll, nn, ss)
TOK bb;
Loc ll;
Pname nn;
Pstmt ss;
{
	Pstmt Xthis_lstmt;

	Xthis_lstmt = 0;
	Xthis_lstmt = new_stmt(Xthis_lstmt, bb, ll, ss);
	Xthis_lstmt->u1.d = nn;

	return Xthis_lstmt;
}

static Pstmt new_forstmt(ll, fss, ee1, ee2, ss)
Loc ll;
Pstmt fss;
Pexpr ee1, ee2;
Pstmt ss;
{
	Pstmt Xthis_forstmt;

	Xthis_forstmt = 0;
	Xthis_forstmt = new_stmt(Xthis_forstmt, FOR, ll, ss);
	Xthis_forstmt->u3.for_init = fss;
	Xthis_forstmt->u2.e = ee1;
	Xthis_forstmt->u1.e2 = ee2;

	return Xthis_forstmt;
}

static Pelist new_elist(e)
Pexpr e;
{
	Pelist Xthis_elist;

	Nl++;
	Xthis_elist = (Pelist) new(sizeof(struct elist));
	Xthis_elist->head = Xthis_elist->tail = e;

	return Xthis_elist;
}

static Pexpr new_qexpr(ee, ee1, ee2)
Pexpr ee, ee1, ee2;
{
	Pexpr Xthis_qexpr;

	Xthis_qexpr = 0;
	Xthis_qexpr = new_expr(Xthis_qexpr, QUEST, ee1, ee2);
	Xthis_qexpr->u4.cond = ee;

	return Xthis_qexpr;
}

static Pnlist new_nlist(n)
Pname n;
{
	Pnlist this;
	Pname nn;

	this = (Pnlist) new(sizeof(struct nlist));
	this->head = n;
	for(nn = n ;nn->n_list ;nn = nn->n_list);
	this->tail = nn;
	Nl++;
	return this;
}

