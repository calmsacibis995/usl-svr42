/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/expand.c	1.1"
/*****************************************************************

expand.c:

	expand inline functions

******************************************************************/

#include "cfront.h"

/*
 *	make the name of the temporary: _Xvn_fn_cn
 */
char *temp(vn, fn, cn)
char *vn, *fn, *cn;
{
	if (vn[0] != '_' || vn[1] != 'X') {
		int vnl, fnl, cnl;
		char *s;

		vnl = strlen(vn);
		fnl = strlen(fn);
		cnl = (cn) ? strlen(cn): 0;
		s = (char *) new(vnl+fnl+cnl+5);

		s[0] = '_';
		s[1] = 'X';
		strcpy(s+2, vn);
		s[vnl+2] = '_';
		strcpy(s+vnl+3, fn);
		if (cnl) {
			s[vnl+fnl+3] = '_';
			strcpy(s+vnl+fnl+4, cn);
		}
		return s;
	}
	else 
		return vn;
}


Pname dcl_local(scope, an, fn)
Ptable scope;
Pname an, fn;
{
	Pname cn, nx, r;
	char *s;

	if (scope == 0){
		errorFIPC('s',
			"cannot expand inlineF needing temporaryV in nonF context",
			ea0, ea0, ea0, ea0);
		return an;
	}
	if (an->n_stclass == STATIC){
		V1.u1.p = (char *)an;
		errorFIPC('s', "static%n in inlineF", &V1, ea0, ea0, ea0);
		return an;
	}
	cn = fn->u4.n_table->t_name;
	s = temp(an->u2.string, fn->u2.string, cn ? cn->u2.string: 0);
	nx = new_name(s);
	nx->u1.tp = an->u1.tp;
	PERM(nx->u1.tp);
	nx->n_used = an->n_used;
	nx->n_assigned_to = an->n_assigned_to;
	nx->n_addr_taken = an->n_addr_taken;
	nx->n_xref = an->n_xref;
	nx->lex_level = an->lex_level;
	r = insert(scope, nx, 0);
	r->n_stclass = an->n_stclass;
	name__dtor(nx);
	r->where.line = 0;
	return r;
}

/*
 * 	inline expansion of if statement needs to check that both
 *	sides of generated ?: are of compatible types;   
 *	val = 1:: ?(...,0) 2:: :(...,0) 0,3:: no action 
 */
int needs_zero(e)
Pexpr e;
{
	int val, tmp;
	Pexpr ee;
	Pbase b;

	val = 0;
	tmp = 1;
	ee = e->u2.e1;

xxx:
	if (ee->u1.tp == 0)return val;
	switch (ee->u1.tp->base){
	case TYPE:
	case TNAME:
		b = (Pbase)ee->u1.tp;
		if (b->b_name->u1.tp->base == COBJ)val += tmp;
		break;
	case COBJ:
		val += tmp;
		break;
	}
	if (tmp == 1){
		tmp++;
		if (ee = e->u3.e2)goto xxx;
	}

	return val;
}

/*
 *	return a value of type t2 from a function returning a t1
 *	return 1 if cast is needed
 */
int ck_cast(t1, t2)
Ptype t1, t2;
{
	while (t1->base == TYPE) t1 = ((Pbase)t1)->b_name->u1.tp;
	while (t2->base == TYPE) t2 = ((Pbase)t2)->b_name->u1.tp;

	if (t1 == t2) return 0;

	if (t1->base != t2->base) return 1;

	switch (t1->base){
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
		if (((Pbase)t1)->b_unsigned != ((Pbase)t2)->b_unsigned)
			return 1;
	}

	if (t1->base == COBJ){
		Pname nn;

		nn = ((Pbase)t1)->b_name;
		if (((Pclass)nn->u1.tp)->csu == UNION) return 0;

		if (t2->base == COBJ && nn->u1.tp == ((Pbase)t2)->b_name->u1.tp)
			return 0;

		return 1;
	}

	return 0;
}

/*
 *	copy the statements with the formal arguments replaced by ANAMES 
 *
 *	called once only per inline function
 *	expand_tbl!=0 if the function should be transformed into an expression
 *	and expand_tbl is the table for local variables
 */
Pstmt stmt_expand(this)
Pstmt this;
{
	if (this == 0){
		V1.u1.p = (char *)expand_fn;
		errorFIPC('i', "0->S::expand() for%n", &V1, ea0, ea0, ea0);
	}
	if (this->memtbl){	/* check for static variables */
		register Ptable t;
		register int i;
		register Pname n;

		t = this->memtbl;
		for(n = get_mem(t, i=1) ;n ;n = get_mem(t,++i)) {
			if (n->n_stclass == STATIC){
				V1.u1.p = (char *)n;
				errorFIPC('s', "static%n in inlineF", &V1,
					ea0, ea0, ea0);
				n->n_stclass = AUTO;
			}
			n->where.line = 0;
		}
	}

	if (expand_tbl){	/* make expression */
		Pexpr ee;
		static int ret_seen = 0;

		if (this->memtbl){
			int i;
			Pname n;
			Ptable tbl;

			tbl = this->memtbl;
			for(n = get_mem(tbl, i=1) ;n ;n = get_mem(tbl,++i)) {
				char *s;
				Pname nn;

				if (n->base != NAME || n->u1.tp == (Ptype)any_type)
					continue;

				if (this->base == BLOCK
				&& tbl->real_block == this
				&& n->lex_level < 2)
					continue;

				s = n->u2.string;
				if (s[0] == '_' && s[1] == 'X') continue;

				nn = dcl_local(scope, n, expand_fn);
				nn->base = NAME;
				n->u2.string = nn->u2.string;
			}
		}
		switch (this->base){
		default:
			V1.u1.i = (int)this->base;
			V2.u1.p = (char *)expand_fn;
			errorFIPC('s', "%kS in inline%n", &V1, &V2, ea0, ea0);
			return (Pstmt)dummy;

		case BLOCK:
			if (this->s_list){
				ee = (Pexpr) stmt_expand(this->s_list);
				if (this->s){
					ee=new_expr(0,CM,(Pexpr)stmt_expand(this->s),ee);
					PERM(ee);
				}
				return (Pstmt)ee;
			}

			if (this->s) return stmt_expand(this->s);

			return (Pstmt)zero;

		case PAIR:
			ee = (this->u2.s2) ?(Pexpr)stmt_expand(this->u2.s2): 0;
			ee=new_expr(0,CM,this->s?(Pexpr)stmt_expand(this->s):0,ee);
			if (this->s_list)
				ee=new_expr(0,CM,ee,(Pexpr)stmt_expand(this->s_list));
			PERM(ee);
			return (Pstmt)ee;

		case RETURN:
			ret_seen = 1;
			this->s_list = 0;

			if (this->u2.e == 0)
				ee = zero;
			else {
				Ptype tt;

				ee = expr_expand(this->u2.e);
				tt = ((Pfct)expand_fn->u1.tp)->returns;
				if (ck_cast(tt, ee->u1.tp))
					ee = new_texpr(CAST, tt, ee);
			}
			return (Pstmt)ee;

		case SM:
			if (this->u2.e == 0 || this->u2.e == dummy)
				ee = zero;
			else {
				if (this->u2.e->base == DEREF)
					this->u2.e = this->u2.e->u2.e1;
				ee = expr_expand(this->u2.e);
			}
			if (this->s_list){
				ee=new_expr(0,CM,ee,(Pexpr)stmt_expand(this->s_list));
				PERM(ee);
			}
			return (Pstmt)ee;

		case ASM:
			if (this->s_list){
				ee=new_expr(0,CM,ee,(Pexpr)stmt_expand(this->s_list));
				PERM(ee);
			}
			return (Pstmt)ee;

		case IF:
		{	Pexpr qq;

			ret_seen = 0;
			qq = new_expr(0, QUEST,(Pexpr)stmt_expand(this->s), 0);
			qq->u4.cond = expr_expand(this->u2.e);
			if (this->u3.else_stmt)
				qq->u3.e2 = (Pexpr)stmt_expand(this->u3.else_stmt);
			else
				qq->u3.e2 = zero;

			switch (needs_zero(qq)){
			case 1:
				qq->u2.e1 = new_expr(0, CM, qq->u2.e1, zero);
				break;
			case 2:
				qq->u3.e2 = new_expr(0, CM, qq->u3.e2, zero);
				break;
			}

			if (ret_seen && this->s_list)
				errorFIPC('s', "S after \"return\" in inlineF",
					ea0, ea0, ea0, ea0);
			ret_seen = 0;
			if (this->s_list)
				qq=new_expr(0,CM,qq,(Pexpr)stmt_expand(this->s_list));
			PERM(qq);
			return (Pstmt)qq;
		}
		}
	}
	this->where.line = 0;

	switch (this->base){
	default:
		if (this->u2.e)this->u2.e = expr_expand(this->u2.e);
		break;
	case PAIR:
		if (this->u2.s2)this->u2.s2 = stmt_expand(this->u2.s2);
		break;
	case ASM:
	case BLOCK:
		break;
	case FOR:
		if (this->u3.for_init)
			this->u3.for_init = stmt_expand(this->u3.for_init);
		if (this->u1.e2)this->u1.e2 = expr_expand(this->u1.e2);
		break;
	case LABEL:
	case GOTO:
	case RETURN:
	case BREAK:
	case CONTINUE:
		V1.u1.i = (int)this->base;
		V2.u1.p = (char *)expand_fn;
		errorFIPC('s', "%kS in inline%n", &V1, &V2, ea0, ea0);
	}

	if (this->s)this->s = stmt_expand(this->s);
	if (this->s_list)this->s_list = stmt_expand(this->s_list);
	PERM(this);
	return this;
}

Pexpr expr_expand(this)
Pexpr this;
{
	if (this == 0) errorFIPC('i', "E::expand(0)", ea0, ea0, ea0, ea0);

	switch (this->base){
	case NAME:
		if (expand_tbl &&((Pname)this)->n_scope == FCT) {
			Pname n, cn;
			char *s;

			n = (Pname)this;
			s = n->u2.string;
			if (s[0] == '_' && s[1] == 'X') break;
			cn = expand_fn->u4.n_table->t_name;
			n->u2.string=temp(s,expand_fn->u2.string,cn?cn->u2.string:0);
		}
	case DUMMY:
	case ICON:
	case FCON:
	case CCON:
	case IVAL:
	case FVAL:
	case LVAL:
	case STRING:
	case ZERO:
	case TEXT:
	case ANAME:
		break;
	case ICALL:
		if (expand_tbl && this->u2.e1 == 0) {
			Pname fn;
			Pfct f;

			fn = this->u4.il->fct_name;
			f = (Pfct)fn->u1.tp;
			V1.u1.p = (char *)fn;
			V2.u1.p = (char *)expand_fn;
			if (f->returns == (Ptype)void_type
			&& f->s_returns != (Ptype)int_type
			&& fn->n_oper != CTOR)
				errorFIPC('s',
		"non-value-returning inline%n called in value-returning inline%n",
					&V1, &V2, ea0, ea0);
			else 
				error("inline%n called before defined", &V1,
					ea0, ea0, ea0);
		}
		break;
	case SIZEOF:
	case CAST:
		if (this->u4.tp2) PERM(this->u4.tp2);
		goto rrr;
	case QUEST:
		this->u4.cond = expr_expand(this->u4.cond);
	default:
		if (this->u3.e2)this->u3.e2 = expr_expand(this->u3.e2);
	case REF:
	case DOT:
	rrr:
		if (this->u2.e1)this->u2.e1 = expr_expand(this->u2.e1);
		break;
	}

	PERM(this);
	return this;
}

/*
 *	is a temporary variable needed to hold the value of this expression
 *	as an argument for an inline expansion?
 *	return 1; if side effect
 *	return 2; if modifies expression
 */
bit not_simple_exp(this)
Pexpr this;
{
	int s;

	switch (this->base){
	default:
		return 2;
	case ZERO:
	case IVAL:
	case FVAL:
	case ICON:
	case CCON:
	case FCON:
	case STRING:
	case NAME:	/* unsafe(alias) */
		return 0;
	case SIZEOF:
		if (this->u2.e1 == 0 || this->u2.e1 == dummy)
			return 0;
		else
			return (int)not_simple_exp(this->u2.e1);
	case G_ADDROF:
	case ADDROF:
		return not_simple_exp(this->u3.e2);
	case CAST:
	case DOT:
	case REF:
		return not_simple_exp(this->u2.e1);
	case UMINUS:
	case NOT:
	case COMPL:
		return not_simple_exp(this->u3.e2);
	case DEREF:
		s = not_simple_exp(this->u2.e1);
		if (1 < s)return 2;
		if (this->u3.e2 == 0)return s;
		return s |= not_simple_exp(this->u3.e2);
	case MUL:
	case DIV:
	case MOD:
	case PLUS:
	case MINUS:
	case LS:
	case RS:
	case AND:
	case OR:
	case ER:
	case LT:
	case LE:
	case GT:
	case GE:
	case EQ:
	case NE:
	case ANDAND:
	case OROR:
	case CM:
		s = not_simple_exp(this->u2.e1);
		if (1 < s)return 2;
		return s |= not_simple_exp(this->u3.e2);
	case QUEST:
		s = not_simple_exp(this->u4.cond);
		if (1 < s)return 2;
		s |= not_simple_exp(this->u2.e1);
		if (1 < s)return 2;
		return s |= not_simple_exp(this->u3.e2);
	case ANAME:
		if (curr_icall){
			Pname n;
			int argno;
			Pin il;

			n = (Pname)this;
			argno = n->n_val;
			for(il = curr_icall ;il ;il = il->i_next)
				if (n->u4.n_table == il->i_table)goto aok;
			goto bok;
		aok:
			if (il->local[argno])
				return 0;
			else
				return (int)not_simple_exp(il->arg[argno]);
		}
	bok:
		V1.u1.p = (char *)this;
		errorFIPC('i', "expand aname%n", &V1, ea0, ea0, ea0);
	case G_CM:
	case VALUE:
	case NEW:
	case CALL:
	case G_CALL:
	case ICALL:
	case ASSIGN:
	case INCR:
	case DECR:
	case ASPLUS:
	case ASMINUS:
	case ASMUL:
	case ASDIV:
	case ASMOD:
	case ASAND:
	case ASOR:
	case ASER:
	case ASLS:
	case ASRS:
		return 2;
	}
}


/*
 *	expand call to(previously defined) inline function in "scope"
 *	with the argument list "ll"
 *	(1) declare variables in "scope"
 *	(2) initialize argument variables
 *	(3) link to body
 */
Pexpr fct_expand(this, fn, scope, ll)
Pfct this;
Pname fn;
Ptable scope;
Pexpr ll;
{
	Pin il;
	Pexpr ic, ee;
	Pname n, nn, at;
	int i, not_simple;
	int s;	/* could be avoided when expanding into a block */
	Ptable tbl;
	Pstmt ss;

	if ((this->body == 0 && this->f_expr == 0)	/* before defined */
	|| (this->defined & SIMPLIFIED)== 0		/* before simplified */
	|| ((Pfct)fn->u1.tp)->body->memtbl == scope	/* while defining */
	|| this->f_inline > 2				/* recursive call */
	|| (this->last_expanded && this->last_expanded==curr_expr)
							/* twice in an expression */
	) {
		if (this->f_inline == 2 || this->f_inline == 4)
		{
			V1.u1.p = (char *)fn;
			V2.u1.p = (char *)fn;
			errorFIPC('s',
	"cannot expand%n twice inE: %n declared as non-inline but defined as inline",
				&V1, &V2, ea0, ea0);
		}
		take_addr(fn);				/* so don't expand */
		if (fn->n_addr_taken == 1){
			nn = new_name(0);		/* but declare it */
			*nn = *fn;
			nn->n_list = dcl_list;
			nn->n_sto = STATIC;
			dcl_list = nn;
		}
		return 0;
	}

	this->f_inline += 2;

	il = (Pin) new(sizeof(struct iline));
	ic = new_texpr(ICALL, 0, 0);
	il->fct_name = fn;
	ic->u4.il = il;
	ic->u1.tp = this->returns;
	at = (this->f_this) ? this->f_this:(this->f_result) ? this->f_result: this->argtype;
	if (at)il->i_table = at->u4.n_table;
	i = 0;
	not_simple = 0;		/* is a temporary argument needed? */

	for(n = at ;n ;n = n->n_list, i++) {
		/*	check formal/actual argument pairs
		 *	and generate temporaries as necessary
		 */
		if (ll == 0){
			V1.u1.p = (char *)fn;
			errorFIPC('i', "F::expand(%n):AX", &V1, ea0, ea0, ea0);
		}

		if (ll->base == ELIST){
			ee = ll->u2.e1;
			ll = ll->u3.e2;
		}
		else {
			ee = ll;
			ll = 0;
		}

		nn = 0;
		if (n->n_assigned_to == FUDGE111){	/* constructor's this */
			if (ee != zero && not_simple_exp(ee) == 0) {
				/* automatic or static, then we can use
				 * the actual variable
				 */
				il->local[i] = 0;
				goto zxc;
			}
		}

		if (n->n_addr_taken || n->n_assigned_to){
			nn = dcl_local(scope, n, fn);
			nn->base = NAME;
			il->local[i] = nn;
			++not_simple;
		}
		else if (n->n_used
			&& (s = not_simple_exp(ee))
			&&(1 < s || 1 < n->n_used) ) {
			nn = dcl_local(scope, n, fn);
			nn->base = NAME;
			il->local[i] = nn;
			++not_simple;
		}
		else if (not_simple_exp(ee)){
			/*(this) not used: evaluate it anyway */
			nn = dcl_local(scope, n, fn);
			nn->base = NAME;
			il->local[i] = nn;
			++not_simple;
		}
		else 
			il->local[i] = 0;
	zxc:
		il->arg[i] = ee;
		il->tp[i] = n->u1.tp;
	}
	tbl = this->body->memtbl;
	if (this->f_expr){	/* generate comma expression */
		char loc_var;
		Pexpr ex;

		loc_var = 0;
		/* look for local variables needing declaration: */
		for(n = get_mem(tbl, i=1) ;n ;n = get_mem(tbl,++i)) {
			if (n->base == NAME	/* don't re-declare the args */
			&&(n->n_used || n->n_assigned_to || n->n_addr_taken)) {
				nn = dcl_local(scope, n, fn);
				nn->base = NAME;
				n->u2.string = nn->u2.string;
				loc_var++;
			}
		}

		if (not_simple || loc_var)this->last_expanded = curr_expr;

		if (not_simple){
			Pexpr etail, e;

			etail = ex = new_expr(0, CM, 0, 0);
			for(i = 0 ;i < MIA;i++) {
				n = il->local[i];
				if (n == 0)continue;
				e = il->arg[i];
				etail->u2.e1 = new_expr(0, ASSIGN,(Pexpr)n, e);
				if (--not_simple)
					etail = etail->u3.e2 = new_expr(0,CM,0,0);
				else 
					break;
			}
			etail->u3.e2 = this->f_expr;
		}
		else {
			ex = this->f_expr;
		}
		ic->u2.e1 = ex;
	}
	else {		/* generate block: */
		for(n = get_mem(tbl, i=1) ;n ;n = get_mem(tbl,++i)) {
			/* mangle local names */
			if (n->base == NAME
			&&(n->n_used || n->n_assigned_to || n->n_addr_taken)) {
				Pname cn;

				cn = fn->u4.n_table->t_name;
				n->u2.string = temp(n->u2.string,fn->u2.string,(cn)?cn->u2.string:0);
			}
		}

		if (not_simple){
			Pstmt st, stail;
			Pexpr e;

			this->last_expanded = curr_expr;
			st = new_estmt(SM, curloc, 0, 0);
			st->where.line = 0;
			stail = st;
			for(i = 0 ;i < MIA;i++) {
				n = il->local[i];
				if (n == 0)continue;
				e = il->arg[i];
				stail->u2.e = new_expr(0, ASSIGN,(Pexpr)n, e);
				if (--not_simple){
					stail->s_list = new_estmt(SM,curloc,0,0);
					stail = stail->s_list;
					stail->where.line = 0;
				}
				else 
					break;
			}
			stail->s_list = (Pstmt)this->body;
			ss = new_block(curloc, 0, st);
			ss->where.line = 0;
		}
		else {
			ss = (Pstmt)this->body;
		}
		ic->u3.e2 = (Pexpr)ss;
	}

	this->f_inline -= 2;
	return ic;
}
