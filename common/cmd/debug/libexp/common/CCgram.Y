/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
%{
#ident	"@(#)debugger:libexp/common/CCgram.Y	1.5"

#include <string.h>
#include <unistd.h>
#include "CC.h"
#include "CCtree.h"
#include "Language.h"
#include "Interface.h"
#include "Resolver.h"
#include "Symbol.h"

//-- This grammar came from the C++ compiler.
//   It has been modified to eliminate the distinction between
//   ID and TNAME - to make context free parsing possible.

static char* buildOperatorNm(int);
static Language language;
static Resolver *parse_context;

#ifdef DEBUG
#define YYDEBUG 1
#endif

/* -- define standard internal names to match extern definitions.
 *    this is necessary to support 2 or more yacc parsers in the
 *    same a.out
*/
#define yylex   CClalex

#define NEXTTOK() ( (yychar==-1) ? (yychar=yylex(),yychar) : yychar )

// inline char *copy(char *s) { return strcpy( new char[strlen(s)+1], s); }

static int yyerrors;
static CCtree *etree_root = 0;
static int    yyparse();
%}

/**** WARNING: Update CCtokdefs.h if %token definitions are changed. ****/

%token  <i>  	EOFTOK		0
%token  <i>  	DELETE		1
%token  <i>  	ENUM		2
%token  <i>  	NEW		3
%token  <i>  	OPERATOR	4
%token  <i>  	SIZEOF		5
%token  <i>  	THIS		6

			/* operators in priority order (sort of) */
%token  <i>     LP		7
%token  <i>     RP		8
%token  <i>  	LB		9
%token  <i>  	RB		10
%token  <i>     REF		11
%token  <i>     DOT		12
%token  <i>     NOT		13
%token  <i>     COMPL		14
%token  <i>     MUL		15
%token  <i>     REFMUL		16
%token  <i>     AND		17
%token  <i>     PLUS		18
%token  <i>     MINUS		19
%token  <i>     ER		20
%token  <i>     OR		21
%token  <i>     ANDAND		22
%token  <i>     OROR		23
%token  <i>  	QUEST		24
%token  <i>     COLON		25
%token  <i>     ASSIGN		26
%token  <i>     ASSIGNOP	27
%token  <i>  	CM		28
%token  <i>  	SM		29
%token  <i>  	LC		30
%token  <i>  	RC		31

			/* constants etc. */
%token  <s>     ID		32
%token	<s>	REG_ID		33
%token  <s>     STRING		34
%token  <c>     CONSTANT	35
%token	<s>	DEBUG_ID	52
%token	<s>	USER_ID		53
%token	<i>	AT_SIGN		54	/* "@"  */
%token	<s>	FN_SUFFIX	55

			/* groups of tokens */
%token  <i>     RELOP		36	/* <, <=, >=, > */
%token  <i>     EQUOP		37	/* ==, != */
%token  <i>     DIVOP		38	/* /, % */
%token  <i>     SHIFTOP		39	/* <<, >> */
%token  <i>     ICOP		40	/* ++, -- */
%token  <i>     AGGR		41	/* class, struct, union */

%token  <i>     FTYPE		42	/* int, short, char,
					   long, float, double
					   unsigned, signed, void */
%token  <i>     SCTYPE		43	/* typedef, extern, static,
					   auto, register, overload,
					   inline, virtual, friend
					   const, volatile */
%token  <i>  	ILLEGAL		44
%token  <i>     EMPTY		45
%token  <i>  	LOW		46
%token  <i>     FDEF		47
%token  <i>  	DECL_MARKER	48
%token  <i>  	ELLIPSIS	49	/* ... */
%token  <i>  	MEM		50	/* :: */
%token  <s>     TSCOPE		51

%left	LOW
%left	SCTYPE FTYPE ID
%left	CM
%right	ASSIGN ASSIGNOP
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
%left	MUL DIVOP REFMUL
%right	NOT COMPL NEW DELETE
%right	ICOP
%left   SIZEOF
%left	LB LP DOT REF MEM TSCOPE ELLIPSIS
%right	AT_SIGN OPERATOR

/* dummy precedences */
%left	DECL_MARKER AGGR ENUM THIS CONSTANT STRING

/* nonterminal type values */
%type	<n>   	expr e_opt e term prim elist_opt elist vec evec
%type	<n>  	id qualifier proto_or_params
%type	<n>   	arg_types arg_type_list opt_arg_types	
%type	<i>   	oper 
%type	<n>   	type type_part ellipsis_opt
%type	<n>   	op_type conv_op
%type	<n>   	arg_prototype new_type
%type	<n>   	declarator new_decl
%type	<n>   	ptr
%type	<n>	cast
%type	<i>   	select
%type	<i>   	term_lp

%start expr

%%

expr		: e
		  { etree_root = $1; }
		;

e		:  e PLUS e
		   { 
	   binaryop:
		   $$ = new CCtree($2, 2, $1, $3);
		   }
		|  e MINUS e
		   { goto binaryop; }
		|  e MUL e
		   { goto binaryop; }
		|  e REFMUL e
		   { goto binaryop; }
		|  e DIVOP e
		   { goto binaryop; }
		|  e AND e
		   { goto binaryop; }
		|  e OR e
		   { goto binaryop; }
		|  e ER e
		   { goto binaryop; }
		|  e SHIFTOP e
		   { goto binaryop; }
		|  e EQUOP e
		   { goto binaryop; }
		|  e RELOP e
		   { goto binaryop; }
		|  e ANDAND  e
		   { goto binaryop; }
		|  e OROR  e
		   { goto binaryop; }
		|  e CM e
		   { goto binaryop; }
		|  e ASSIGN e
		   { goto binaryop; }
		|  e ASSIGNOP e
		   { goto binaryop; }
		|  e QUEST e COLON e
		   { $$ = new CCtree(N_QUEST, 3, $1, $3, $5); }
		|  term
		;

e_opt		:  %prec LOW
		   { $$ = 0; }
		|  e
		;

elist		:  e	%prec CM
		   { $$ = new CCtree(N_ZCONS, 2, $1); }  /* list of one member */
		|  elist CM e
		   { $$ = $1->CCtree_append($3); }
		;

elist_opt	:  /* epsilon */
		    { $$ = 0; }
		|  elist
		;

term		:  FTYPE LP elist RP
		   {
			$$ = new CCtree(N_CAST, 2,
					new CCtree(N_TYPE, $1, VT_code), $3);
		   }
		|  NEW new_type
		   { $$ = new CCtree(N_NEW, 1, $2); }
		|  NEW LP new_type RP
		   { $$ = new CCtree(N_NEW, 1, $3); }
		|  NEW new_type LP elist_opt RP
		   { $$ = new CCtree(N_NEW, 2, $2, $4); }
		|  DELETE term		%prec LOW
		   { $$ = new CCtree(N_DELETE, 1, $2); }
		|  DELETE evec term	%prec LOW
		   { $$ = new CCtree(N_DELETE, 2, $3, $2); }
		|  term ICOP %prec SIZEOF
		   {
		     int incop = ($2 == N_PREPLPL ? N_POSTPLPL : N_POSTMIMI);
		     $$ = new CCtree(incop, 1, $1);
		   }
		|  ICOP term
		   { $$ = new CCtree($1, 1, $2); }  /* lexer assumes pre. */
		|  term_lp e RP	%prec LOW
		   { $$ = $2; }
		|  cast term %prec LOW
		   { $$ = new CCtree(N_CAST, 2, $1, $2); }
		|  MUL term %prec SIZEOF
		   { $$ = new CCtree(N_DEREF, 1, $2); }
		|  AND term %prec SIZEOF
		   { $$ = new CCtree(N_ADDR, 1, $2); }
		|  PLUS  term %prec SIZEOF
		   { $$ = new CCtree(N_UPLUS, 1, $2); }
		|  MINUS term %prec SIZEOF
		   { $$ = new CCtree(N_UMINUS, 1, $2); }
		|  NOT term %prec SIZEOF
		   { $$ = new CCtree(N_NOT, 1, $2); }
		|  COMPL term %prec SIZEOF
		   { $$ = new CCtree(N_TILDE, 1, $2); }
		|  SIZEOF cast %prec MEM
		   { $$ = new CCtree(N_SIZEOF, 1, $2); }
		|  SIZEOF term
		   { $$ = new CCtree(N_SIZEOF, 1, $2); }
                |  term proto_or_params
                  { $$ = new CCtree(N_CALL, 2, $1, $2); }  /* or cast. */
		|  term evec
		   { $$ = new CCtree(N_INDEX, 2, $1, $2); }
		|  term select prim
		   { $$ = new CCtree($2, 2, $1, $3); }
		|  term select TSCOPE prim
		   {
		     $$ = new CCtree($2, 2, $1,
				    new CCtree(N_MEM, 2, $4,
					      new CCtree(N_ID, $3)));
		   }
		|  MEM prim
		   { $$ = new CCtree(N_MEM, 1, $2); }
		|  prim
		|  TSCOPE prim
		   { $$ = new CCtree(N_MEM, 2, $2, new CCtree(N_ID, $1)); }
		|  CONSTANT
		   { $$ = new CCtree($1); }
		|  STRING
		   { $$ = new CCtree(N_STRING, $1); }
		;

prim		:  id
		|  DEBUG_ID
		   { $$ = new CCtree(N_DEBUG_ID, $1); }
		|  USER_ID
		   { $$ = new CCtree(N_USER_ID, $1); }
		|  OPERATOR oper
		   { $$ = new CCtree(N_ID, buildOperatorNm($2)); }
		;

id		: ID
		   { $$ = new CCtree(N_ID, $1); }
		|  REG_ID
		   { $$ = new CCtree(N_REG_ID, $1); }
		| qualifier AT_SIGN id
		   {	if( $1 == 0 )
				$$ = new CCtree(N_AT, 1, $3);
			else
				$$ = new CCtree(N_AT, 2, $3, $1);
		    }
		;

qualifier	: 
		   { $$ =  0; };
		| ID 
		   { $$ = new CCtree(N_ID, $1); }
		| FN_SUFFIX DOT ID 
		   { $$ =  new CCtree(N_DOT, 2, new CCtree(N_ID, $1),
				     new CCtree(N_ID, $3));
		   }
		| ID CONSTANT 
		   { $$ =  new CCtree(N_DOT, 2, new CCtree(N_ID, $1),
				     new CCtree(N_ID, $1));
		   }
		| CONSTANT 
		   { $$ = new CCtree($1); }
		;

select		:  REF
		|  DOT
		;

ptr		:  MUL
		   { $$ = new CCtree(N_DT_PTR, 0, VT_code); }
		|  AND
		   { $$ = new CCtree(N_DT_REF, 0, VT_code); }
		|  TSCOPE MUL
		   { $$ = new CCtree(N_DT_MPTR, $1); }
		|  ptr SCTYPE
		   {
		     printe(ERR_store_class, E_WARNING);
		     $$ = $1;
		   }
		;

new_decl	:  %prec LOW
		   { $$ = 0; }
		|  ptr new_decl		%prec MUL
		   { $$ = new CCtree(N_NTYPE, 2, $1, $2); }
		|  new_decl evec	%prec LB
		   { $$ = new CCtree(N_DT_ARY, 2, $1, $2); }
		;

/* NEW expr won't reduce to term on LB since vec will go with new_decl */
new_type	:  type new_decl %prec LOW
		   { $$ = new CCtree(N_NTYPE, 2, $1, $2); }
		;

type		:  DECL_MARKER
		   { $$ = 0; }
		|  type_part
		|  type type_part
		   { $$ = ($1 == 0 ? $2 : new CCtree(N_TYPE, 2, $1, $2)); }
		;

type_part	:  SCTYPE
		   { printe(ERR_store_class, E_WARNING); $$ = 0; }
		|  FTYPE
		   { $$ = new CCtree(N_TYPE, $1, VT_code); }/*$1==char,int,...*/
		|  ID
		   { $$ = new CCtree(N_TYPE, $1); } /* user typename. */
		|  ENUM ID
		   { $$ = new CCtree(N_ENUM, $2); }
		|  AGGR ID
		   { $$ = new CCtree(N_AGGR, $2); } /* union,struct,class. */
		;

vec		:  LB e_opt RB
		   { $$ = $2; }
		;

evec		:  LB e RB
		   { $$ = $2; }
		;

term_lp		:  LP
		    { check_cast(); }
		;

cast		:  term_lp type declarator RP
		   { $$ = new CCtree(N_TYPE, 2, $2, $3); }
		;

declarator	:  %prec LOW
		    { $$ = 0; }
		|  declarator vec %prec SIZEOF
		   {
	              $$ = ( $1==0? new CCtree(N_DT_ARY, 1, $2):
				    new CCtree(N_DT_ARY, 2, $1, $2) ); }
		|  ptr declarator %prec MUL
		   { $$ = new CCtree(N_TYPE, 2, $1, $2); }
		|  LP ptr declarator RP vec
		   {
		     $$ = new CCtree(N_TYPE,2, 
				   ($3==0? new CCtree(N_DT_ARY, 1, $5):
					   new CCtree(N_DT_ARY, 2, $3, $5)),$2);
		   }
		|  LP ptr declarator RP opt_arg_types
		   {
		     $$ = new CCtree(N_TYPE,2,new CCtree(N_DT_FCN, 1,$5),
					     new CCtree(N_TYPE,2,$2,$3));
		   }
		;

proto_or_params	:  arg_types
		|  LP elist_opt RP
		   { $$ = $2; }
		;

arg_types	:  LP arg_type_list ellipsis_opt RP
		   { $$ = $2->CCtree_append($3); }
		|  LP ELLIPSIS RP
		   { $$ = new CCtree(N_ZCONS, 2, new CCtree(N_ELLIPSIS, 0)); }
		;

opt_arg_types	: arg_types
		
		|  LP RP
		   { $$ = 0; }
		;

arg_type_list	:  arg_prototype
		   { $$ = new CCtree(N_ZCONS, 2, $1); }
		|  arg_type_list CM arg_prototype
		   { $$ = $1->CCtree_append($3); }
		;

arg_prototype	: type declarator
		  { $$ = ($2==0? $1: new CCtree(N_TYPE, 2, $1, $2)); }
		;

ellipsis_opt	:  /* epsilon */
		   { $$ = 0; }
		|  ELLIPSIS
		   { $$ = new CCtree(N_ELLIPSIS, 0); }
		|  CM ELLIPSIS
		   { $$ = new CCtree(N_ELLIPSIS, 0); }
		;

oper		:  PLUS
		|  MINUS
		|  MUL
		|  REF	/* operator-> */
		|  AND
		|  OR
		|  ER
		|  SHIFTOP
		|  EQUOP
		|  DIVOP
		|  RELOP
		|  ANDAND
		|  OROR
		|  LP RP
		|  LB RB
		|  NOT
		|  COMPL
		|  ICOP
		|  ASSIGN
		|  ASSIGNOP
		|  NEW
		|  DELETE
		;

conv_op		:  op_type	%prec LOW
		|  op_type ptr	%prec LOW
		   { $$ = new CCtree(N_DT_PTR, 2, $1, $2); }
		;

op_type		:  type_part
		|  SCTYPE type_part
		   {
		      printe(ERR_store_class, E_WARNING);
		      $$ = $2;
		   }
		;
%%

YYSTYPE& CC_yylval = yylval;  // extern name available to CC lexer.

void
yyerror(const char* s)
{
    CClex_position();
    printe(ERR_syntax, E_ERROR, s);
    ++yyerrors;
}

static void
check_cast()
//
// Lookahead to direct parsing of cast
// CCla_cast() returns 1 if lookahead sees an ambiguous old-style C cast.
//
// NOTE: This does more now.  It must distinguish between
//       TNAME and ID since these two were combined.
{
    switch (NEXTTOK()) {
    case FTYPE:
	if (CCla_look() == LP && CCla_cast()) {
	    CCla_backup(yychar, yylval, CCpos);
	    yychar = DECL_MARKER;
	}
	break;
    case ID:
	switch (CCla_look()) {
	default:
	    // restricted some cases of RP and LB so that
	    // these will be considered in the default manner.
	    break;
	case LB:
	    if (CCla_bracket()) {
		CCla_backup(yychar, yylval, CCpos);
		yychar = DECL_MARKER;
	    }
	    break;
	case LP:
	case MUL:
	case AND:
	    if (CCla_cast()) {
		CCla_backup(yychar, yylval, CCpos);
		yychar = DECL_MARKER;
	    }
	    break;
	case RP:
	{
		// decide between regular id and type name
		Symbol	entry;
		if (!parse_context->lookup(yylval.s, entry))
			break;
		if (entry.isUserTypeSym())
		{
			
			CCla_backup(yychar, yylval, CCpos);
			yychar = DECL_MARKER;
		}
		break;
	}
	break;
	}
    }
}

static char *
buildOperatorNm(int op)
{
	char *operatorStr = getCCOperatorStr((Operator)op);
	char *operatorNm = new char[strlen(operatorStr)+9];
	sprintf(operatorNm, "operator%s", operatorStr);
	return operatorNm;
}

CCtree *
CCparse(char *estring, Language lang, Resolver *context)
{

	language = lang;	// allow actions to know C/C++

	CCtlex_init(estring, lang); // provide entire `input stream'.
	yyerrors = 0;
	parse_context = context;
	yyparse();
	CCreset_lalex();	// essential for C++; doesn't hurt for C.


	register CCtree *newtree = etree_root;
	etree_root = 0;		// clear to prevent unwanted use.

	if (yyerrors)
		return 0;
	else
		return newtree;
}

// to get around type problems with use of malloc in yaccpar
#define malloc(sz)	new char[sz]
