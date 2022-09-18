/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/lalex.c	1.2"
/***********************************************************************

lalex.c:

	lexical lookahead
	unravel casts to handle cast/constructor ambiguity
	handle "struct x" => "x" transformation
	make NAME nodes for identifiers
	try to catch missing semi-colons
	(those sets of characters ought to be handled by table lookup)

**************************************************************************/

#include "cfront.h"
#include "yystype.h"
#include "tqueue.h"

/*
 *	first define a queue of tokens(as a linked list)
 */

static Ptoknode new_toknode();

Ptoknode front = 0;
Ptoknode rear = 0;

Ptoknode free_toks = 0;
#define NBITE 16

static Ptoknode new_toknode(t, r)
TOK t;
YYSTYPE r;
{
	int i;
	register Ptoknode q;
	Ptoknode this;

	if (free_toks == 0){
		i = sizeof(struct toknode);
		q = (Ptoknode)malloc(NBITE * i);
		free_toks = q;
		for(q+=NBITE-1, q->next=0; q!=free_toks; q--)(q-1)->next = q;
	}
	this = free_toks;
	free_toks = free_toks->next;
	this->tok = t;
	this->retval = r;
	this->next = this->last = 0;
	return this;
}

void del_tnode(this)
Ptoknode this;
{
	if ( !this ) return;

	this->next = free_toks;
	free_toks = this;
	this = 0;
	return;
}

void addtok(t, r)
TOK t;
YYSTYPE r;
{
	Ptoknode T;

	T = (Ptoknode)new_toknode(t, r);
	if (front == 0)
		front = rear = T;
	else {
		rear->next = T;
		T->last = rear;
		rear = T;
	}
}


TOK tk;		/* last token returned */

TOK deltok()
{
	Ptoknode T;

	T = front;
	tk = T->tok;
	yylval = T->retval;
	if (front = front->next) front->last = 0;
	del_tnode(T);
	return tk;
}

int scan_type();
int scan_mod();
int scan_tlist();
int scan_suf();
void get_tag();
void scan_e();

Ptoknode latok = 0;		/* "lookahead" token */

TOK  la_start()
{
	if (front == 0) tlex();
	latok=front;
	return latok->tok;
}

TOK lookahead()
{
	if (latok==rear) tlex();
	latok=latok->next;
	return latok->tok;
}

void backup()
{
	if (latok->last == 0)
		errorFIPC('i', "token q backup", ea0, ea0, ea0, ea0);
	latok = latok->last;
}

void insert_tok(t)
TOK t;
{
	Ptoknode nt;

	nt = (Ptoknode)new_toknode(t, yylval);
	nt->last = latok->last;
	nt->last->next = nt;
	nt->next = latok;
	latok->last = nt;
	latok = nt;
}

/* replace bad cast with cast to ANY */
void rep_cast()
{
	Ptoknode tt;
	Ptoknode junk;

	tt = front->next;	/* front->tok == LP */
	junk = tt->next;
	if (junk == latok) return;
	tt->tok = TYPE;
	tt->retval.pt = (Ptype)any_type;
	tt->next = latok;
	latok->last->next = 0;
	latok->last = tt;
	tt = junk;
	while (tt){
		register Ptoknode tx;
		tx = tt->next;
		del_tnode(tt);
		tt = tx;
	}
}

/*
 *	PRE-PARSER--SCAN AHEAD TO DETERMINE TOKEN TYPE
 */

#define DO_RET goto ret

int bad_cast = 0;

TOK lalex()
{
	static int nocast = 0;	/* prevent ID CAST, FOR CAST, etc. */
	static int incast = 0;	/* don't scan in already recognized cast */
	static int in_enum;
	static int fr;
	char en;

	en = 0;
	switch (la_start()) {
	case ENUM:
		en = 1;
	case AGGR:
		switch (tk){
/*				strictly speaking these ought to be included,
				but they only cause grief
		case ELSE:
*/
		case 0:
		case LP:	/* argument list */
		case CM:
		case NEW:
		case CAST:
		case RP:	/* yok, old C: f(i) struct s * i; {} */
		case OPERATOR:
		case DO:
		case TYPE:	/* might be "const" */
		case COLON:	/* label */
		case SM:	/* end of statement */
		case RC:	/* end of statement */
		case LC:	/* first statement */
			break;
		default:
			V1.u1.i = (int)latok->tok;
			errorFloc(&curloc, "';' missing afterS orD before\"%k\"",
				&V1, ea0, ea0, ea0);
			return (tk = SM);
		}

	{	TOK t, x;

		t = lookahead();
		switch (t){
		case TNAME:
			x = lookahead();
			break;
		case ID:			/* hidden or undefined */
			x = lookahead();
			backup();
			switch (x){
			case LC:
				in_enum = en;
			case COLON:		/* defining: return AGGR ID */
				backup();
				fr = 0;
				DO_RET;
			default:
			{
				Pname n;

				n = look(ktbl, latok->retval.s, HIDDEN);
				if (n == 0){	/* new tag: define it */
					n = new_name(latok->retval.s);
					n->lex_level = 0;
					n = tname(n, latok->last->retval.t);
					modified_tn = modified_tn->l;
				}
				else {
					switch (n->u1.tp->base){
					case COBJ:
					case EOBJ:
						break;
					default:
						V1.u1.p = (char *)n;
						V2.u1.p = (char *)n->u1.tp;
						errorFIPC('i', "hidden%n:%t",
							&V1, &V2, ea0, ea0);
					}
				}
				latok->tok = TNAME;
				latok->retval.pn = n;
			}
			}
			(void) lookahead();
			break;
		case LC:
			in_enum = en;
		default:
			fr = 0;
			DO_RET;
		};

		switch (x){
		case LC:		/* class x { */
			in_enum = en;
		case COLON:		/* class x : */
			fr = 0;
			DO_RET;
		case SM:
			if (tk != NEW && fr == 0) {
				/* no further processing necessary */
				switch (tk){
				case 0:
				case RC:
				case LC:
				case SM:
					break;
				default:
					V1.u1.i = (int)tk;
					error("syntax error: invalid token: %k",
						&V1, ea0, ea0, ea0);
				}

				deltok();	/* class */
				deltok();	/* x */
				deltok();	/* ; */
				return lalex();
			}			/* new class x ; => new x ; */

		default:
			deltok();	/* AGGR(?) TNAME(x) => TNAME(x) */
			fr = 0;
			DO_RET;
		}
	}

	case LP:
		fr = 0;
		if (nocast){
			nocast = 0;
			DO_RET;
		} else if (incast)
			DO_RET;
		/* possible cast */
		bad_cast = 0;
		if (scan_type()){
			if (scan_mod()){
				if (lookahead() != RP) DO_RET;
				switch (lookahead()) {
				case CM: case RP: case SM: case LC: case ASSIGN:
					/* arg type list in declaration */
					if (tk != SIZEOF) DO_RET;
					break;

				case PLUS: case MINUS: case MUL: case AND:
				case NEW: case DELETE: case SIZEOF: case MEM:
				case NOT: case COMPL: case ICOP:
				case LP: case CAST:
				case ID: case TYPE: case TNAME:
				case THIS: case OPERATOR: case ZERO:
				case ICON: case FCON: case CCON: case STRING:
					/* cast of a term */
					break;
				default:	/* something wrong... */
					/* binary op, missing ;,  etc.
					 * "bad cast" could be legal expr
					 *    "( TNAME() )"(ctor call)
					 */
					if (bad_cast) DO_RET;
					else break;
				}
				backup();
				front->tok = CAST;
				latok->tok = ENDCAST;
				if (bad_cast){
					error("can't cast toF", ea0, ea0, ea0, ea0);
					rep_cast();
				}
				incast = 1;
			}
		}
		DO_RET;
	case CAST:
		incast++;
		DO_RET;
	case ENDCAST:
		if (--incast == 0) nocast = 0;
		DO_RET;
	case ID:
	{	char *s;

		s = front->retval.s;
		fr = 0;
		nocast = 1;
		switch (lookahead()) {
		case ID:
		{	/* handle ID ID
			 * assume ID is a missing, hidden, or misspelt TNAME
			 */
			char *s2;
			Pname n;

			s2 = latok->retval.s;
			backup();
			n = look(ktbl, s, HIDDEN);
			if (n == 0){	/* new tag: define it */
				V1.u1.p = (char *)s;
				V2.u1.p = (char *)s2;
				V3.u1.p = (char *)s;
				error("%s %s:TX(%s is not a TN)", &V1,
					&V2, &V3, ea0);
				n = new_name(s);
				n->lex_level = 0;
				n = tname(n, 0);
				modified_tn = modified_tn->l;
				n->u1.tp = (Ptype)any_type;
			}
			else {
				V1.u1.p = (char *)s;
				V2.u1.p = (char *)s2;
				V3.u1.p = (char *)s;
				error("%s %s: %s is hidden", &V1, &V2, &V3, ea0);
			}
			latok->tok = TNAME;
			latok->retval.pn = n;
			break;
		}
		case LC:
			backup();
			front->retval.pn = new_name(s);
			front->retval.pn->lex_level--;
			break;
		default:
			backup();
			front->retval.pn = new_name(s);
		}
		DO_RET;
	}
	case CASE:
	case DEFAULT:
	case PR:
	case ELSE:
		fr = 0;
		switch (tk){
		case COLON:	/* label */
		case SM:	/* end of statement */
		case RC:	/* end of statement */
		case LC:	/* first statement */
			DO_RET;
		default:
			V1.u1.i = (int)latok->tok;
			errorFloc(&curloc, "';' missing afterS orD before\"%k\"",
				&V1, ea0, ea0, ea0);
			return (tk = SM);
		}
	case DO:
	case GOTO:
	case CONTINUE:
	case BREAK:
	case RETURN:
		fr = 0;
		switch (tk){
		case ELSE:
		case DO:
		case COLON:	/* label */
		case RP:	/* end of condition */
		case SM:	/* end of statement */
		case RC:	/* end of statement */
		case LC:	/* first statement */
			DO_RET;
		default:
			V1.u1.i = (int)latok->tok;
			errorFloc(&curloc, "';' missing afterS orD before\"%k\"",
				&V1, ea0, ea0, ea0);
		return (tk = SM);
		}
	case IF:
	case WHILE:
	case FOR:
	case SWITCH:
		fr = 0;
		switch (tk){
		case ELSE:
		case DO:
		case COLON:	/* label */
		case RP:	/* end of condition */
		case SM:	/* end of statement */
		case RC:	/* end of statement */
		case LC:	/* first statement */
			nocast = 1;
			DO_RET;
		default:
			V1.u1.i = (int)latok->tok;
			errorFloc(&curloc, "';' missing afterS orD before\"%k\"",
				&V1, ea0, ea0, ea0);
		return (tk = SM);
		}
	case TYPE:	/* dangerous to diddle with: constructor notation */
		fr = 0;
		switch (tk){
		case ID:
		case RB:
			V1.u1.i = (int)latok->tok;
			errorFloc(&curloc, "';' missing afterS orD before\"%k\"",
				&V1, ea0, ea0, ea0);
			return (tk = SM);
		}
		if (latok->retval.t == FRIEND) fr = 1;
		nocast = 1;
		DO_RET;
	case TNAME:	/* dangerous to diddle with: name hiding */
	{	Pname n;
		TOK otk;

		n = latok->retval.pn;
		if (fr){	/* guard against: TYPE(friend) TNAME(x) SM */
			nocast = 1;
			fr = 0;
			DO_RET;
		}
		fr = 0;
		otk = tk;
		switch (lookahead()) {		/* TN ? */
		case MEM:			/* TN :: ? */
			switch (lookahead()) {
			case MUL:		/* TN :: * pointer to member */
				deltok();	/* TN */
				deltok();	/* :: */
				front->tok = MEMPTR;
				front->retval.pn = n;
				nocast = 1;
				DO_RET;
			default:
				backup();
			}
			break;
		default:
			break;
		}
		backup();
		tk = otk;
		switch (tk){
		case TYPE:	/* int TN ? or unsigned TN ? */
				/* beware of unsigned etc. */
			switch (lookahead()) {
			case SM:
			case RB:
			case COLON:
			case ASSIGN:
				goto hid;
			default:
				nocast = 1;
				DO_RET;
			}

		case TNAME:	/* TN TN ? */
			switch (lookahead()) {
			case MEM:	/* cl_name::mem_name */
			case DOT:	/* anachronism: cl_name.mem_name */
				nocast = 1;
				DO_RET;
			}
		hid:
			backup();	/* undo lookahead after TNAME */
			hide(n);
			n = new_name(n->u2.string);
			n->n_oper = TNAME;
			latok->tok = ID;
			latok->retval.pn = n;
		}
	}

	case NEW:
		fr = 0;
		nocast = 1;
		DO_RET;

	case RC:
		fr = 0;
		switch (tk){
		case RC:	/* nested }(either stmt or expr) */
		case LC:	/* empty block: {} */
		case SM:	/* end of statement */
			break;
		default:
		{	TOK t;
			Loc x;

			x = curloc;
			switch (t = lookahead()) {
			case ELSE:
			case RC:	/* } } probably end of initializer */
			case CM:	/* } , definitely end of initializer */
			case SM:	/* } ; probably end of initializer or class*/
			case RP:	/* int f( struct { ... } ); */
				break ;
			default:
				/* either	"= { ... E } SorD"
				 * or		" SorD }"
				 * or enum { a, b } c; - yuk
				 */
				if (in_enum == 0){
					errorFloc(&x,"';'X at end ofS orD before '}'"
						, ea0, ea0, ea0, ea0);
					return (tk = SM);
				}
				in_enum = 0;
			}
		}
		}
		in_enum = 0;
	default:
		fr = 0;
		nocast = 0;
		DO_RET;
	}
ret:
	{	Ptoknode T;

		T = front;
		tk = T->tok;
		yylval = T->retval;
		if (front = front->next)front->last = 0;
		del_tnode(T);
		return tk;
	}
}


/*
 * type	:  TYPE type
 *	|  TNAME type
 *	|  AGGR tag type
 *	|  ENUM tag type
 *
 *	Beware of TNAME::
 */
int scan_type()
{
	int is_type;

	is_type = 0;
	for(;;)
		switch (lookahead()) {
		case TNAME:
			if (lookahead() == MEM) {
				backup();
				goto def;
			}
			backup();
			goto ttt;
		case AGGR:
		case ENUM:
			get_tag();
		case TYPE:
		ttt:
			is_type = 1;
			continue;
		default:
		def:
			backup();
			return is_type;
		}
}


/*
 *mod	:  ptr mod
 *	|  TNAME :: * mod
 *	| ( mod )
 *	|  mod suf
 *	|  ...
 */
int scan_mod()
{
	for(;;)
		switch (lookahead()) {
		case AND:
		case MUL:		/* ptr mod */
			continue;
		case LP:		/* "(" mod ")" [suf] | suf */
			switch (lookahead()) {
			case AND:
			case MUL:
			case LP:
			case LB:
				backup();
				if (!scan_mod()) return 0;
				if (lookahead() != RP) return 0;
				if (!scan_suf()) return 0;
				return 1;
			case AGGR:
			case ENUM:
			case TYPE:
			case TNAME:
				backup();
				if (!scan_tlist()) return 0;
				if (lookahead() != RP) return 0;
				/* FALLTHRU */
			case RP:
				bad_cast = 1;	/* possible cast to ftn */
				if (!scan_suf()) return 0;
				return 1;
			default:
				return 0;
			}
		case LB:		/* mod suf */
			backup();
			if (!scan_suf()) return 0;
			return 1;
		case RP:
		case CM:
		case ELLIPSIS:
			backup();
			return 1;
		case TNAME:
			if (lookahead() == MEM){
				if (lookahead() == MUL)continue;
				backup();
			}
			backup();
		default:
			return 0;
		}
}


/*
 *suf	:  suf vec
 *	|  suf arg_type_list
 *	|  ...
 *
 *vec	:  [ ICON ]
 *
 *arg_type_list:	( tlist )
 */
int scan_suf()
{
	int found;

	found = 0;
	for(;;)
		switch (lookahead()) {
		case LB:
			scan_e();
			found = 1;
			continue;
		case LP:
			if (!scan_tlist()) return 0;
			if (lookahead() != RP) return 0;
			if (found)
				bad_cast = 1;	/* possible cast to ftn */
			else 
				found = 1;
			continue;
		default:
			backup();
			return 1;
		}
}


/*
 *	tlist type | type
 *	type-->(TYPE | [AGGR] TNAME) mod
 */
int scan_tlist()
{
	for(;;) {
		switch (lookahead()) {
		case AGGR:
		case ENUM:
			get_tag();
			/* FALLTHRU */
		case TYPE:
		case TNAME:
			scan_type();
			break;
		case ELLIPSIS:
			if (lookahead() != RP){
				error("missing ')' after '...'", ea0,ea0,ea0,ea0);
				insert_tok(RP);
			}
		case RP:
			backup();
			return 1;
		default:
			return 0;
		}

		/* saw type */
		if (!scan_mod()) return 0;

		switch (lookahead()) {
		case CM:
			continue;
		case ELLIPSIS:
			if (lookahead() != RP){
				error("missing ')' after '...'", ea0,ea0,ea0,ea0);
				insert_tok(RP);
			}
		case RP:
			backup();
			return 1;
		default:
			return 0;
		}
	}
}

/* saw AGGR or ENUM */
void get_tag()
{
	switch (lookahead()) {
	default:
		errorFIPC('e', "missing tag", ea0, ea0, ea0, ea0);
		insert_tok(ID);
		latok->retval.s = "__MISSING__";
	case ID:
	{	Pname n;

		n = look(ktbl, latok->retval.s, HIDDEN);
		if (n == 0){
			n = new_name(latok->retval.s);
			n->lex_level = 0;
			n = tname(n, latok->last->retval.t);
			modified_tn = modified_tn->l;
		}
		else {
			switch (n->u1.tp->base){
			case COBJ:
			case EOBJ:
				break;
			default:
				V1.u1.p = (char *)n;
				V2.u1.p = (char *)n->u1.tp;
				errorFIPC('i', "hidden%n:%t", &V1, &V2, ea0, ea0);
			}
		}
		latok->tok = TNAME;
		latok->retval.pn = n;
		break;
	}
	case TNAME:
		break;
	}

	switch (lookahead()) {
	default:
		backup();
		return;
	case COLON:
		switch (lookahead()) {
		case ID:
		case TNAME:
		case LC:
			break;
		default:
			backup();
			return;
		}
	case LC:
	{	int level;

		level = 1;
		for(;;)
			switch (lookahead()) {
			case LC:
				level++;
				break;
			case RC:
				if (--level == 0) return;
				break;
			case EOFTOK:
				errorFIPC('i', "unexpected eof", ea0,ea0,ea0,ea0);
			}
	}
	} /* switch */
}

/* scan expr in vec */
void scan_e()
{
	long brcount;
	int localcast;

	brcount = 1L;
	localcast = 0;
	for(;;)
		switch (lookahead()) {
		case RB:
			if (--brcount == 0L) return;
			continue;
		case LB:
			brcount++;
			continue;
		case LP:
		{
			Ptoknode mark;

			if (localcast)
				continue;
			mark = latok;
			if (scan_type())
				if (scan_mod())
					if (lookahead() == RP)
						switch (lookahead()) {
						case CM:
						case RP:
						case SM:
						case LC:
						case ASSIGN:
							break;
						default:
							backup();
							mark->tok = CAST;
							latok->tok = ENDCAST;
						}
			continue;
		}
		case CAST:	/* scenario: local cast recognized,
				 *	main cast in lalex fails,
				 *	lalex later recognizes a smaller cast
				 *	containing this one
				 */
			localcast++;
			continue;
		case ENDCAST:
			localcast--;
			continue;
		default:
			continue;
		}
}
