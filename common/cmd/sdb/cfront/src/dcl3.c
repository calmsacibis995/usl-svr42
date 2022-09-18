/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/dcl3.c	1.2"
/**************************************************************************

dcl3.c:
	Etc routines used by ::dcl functions

*****************************************************************************/


#include "cfront.h"
#include "size.h"

/*
 *	returns X where X(X&) has been declared
 *	add "_result" argument of type X*
 */
void make_res(f)
Pfct f;
{
	Pname rv;

	rv = new_name("_result");
	rv->u1.tp = (Ptype)new_ptr(PTR, f->returns, 0);
	rv->n_scope = FCT;	/* not a ``real'' argument */
	rv->n_used = 1;
	rv->n_list = f->argtype;
	if (f->f_this) f->f_this->n_list = rv;
	f->f_result = rv;
}

/*
 *	check declarations of operators, ctors, dtors
 */
void check_oper(this, cn)
Pname this, cn;
{
	Pfct f;

	switch (this->n_oper){
	case CALL:
		if (cn == 0) error("operator() must be aM", ea0, ea0, ea0, ea0);
		break;
	case DEREF:
		if (cn == 0) error("operator[] must be aM", ea0, ea0, ea0, ea0);
		break;
	case 0:
	case TNAME:	/* may be a constructor */
		if (cn && strcmp(cn->u2.string, this->u2.string) == 0) {
			if (this->u1.tp->base == FCT){

				f = (Pfct)this->u1.tp;
				if (f->returns != (Ptype)defa_type && fct_void == 0)
				{
					V1.u1.p = (char *)this->u2.string;
					V2.u1.p = (char *)this->u2.string;
					error("%s::%s()W returnT", &V1,&V2,ea0,ea0);
				}
				f->returns = (Ptype)void_type;
				this->u2.string = "_ctor";
				this->n_oper = CTOR;
			}
			else 
			{
				V1.u1.p = (char *)cn;
				V2.u1.p = (char *)cn;
				errorFIPC('s', "struct%cnM%n", &V1,&V2,ea0,ea0);
			}
		}
		else 
			this->n_oper = 0;
		break;
	case DTOR:	/* must be a destructor */
		if (cn == 0){
			this->n_oper = 0;
			V1.u1.p = (char *)this->u2.string;
			error("destructor ~%s() not inC", &V1, ea0, ea0, ea0);
		}
		else if (strcmp(cn->u2.string, this->u2.string) == 0){
		dto:
			f = (Pfct)this->u1.tp;
			this->u2.string = "_dtor";
			V1.u1.p = (char *)cn->u2.string;
			V2.u1.p = (char *)cn->u2.string;
			if (this->u1.tp->base != FCT){
				error("%s::~%s notF", &V1, &V2, ea0, ea0);
				this->u1.tp = (Ptype)fct_ctor(void_type,0,1);
			}
			else if (f->returns != (Ptype)defa_type
				&& f->returns != (Ptype)void_type
				&& fct_void == 0)
				error("%s::~%s()W returnT", &V1, &V2, ea0, ea0);
			if (f->argtype){
				if (fct_void == 0)
					error("%s::~%s()WAs", &V1, &V2, ea0, ea0);
				f->nargs = 0;
				f->nargs_known = 1;
				f->argtype = 0;
			}
			f->returns = (Ptype)void_type;
		}
		else {
			if (strcmp(this->u2.string, "_dtor") == 0)goto dto;
			V1.u1.p = (char *)this->u2.string;
			V2.u1.p = (char *)cn->u2.string;
			error("~%s in %s", &V1, &V2, ea0, ea0);
			this->n_oper = 0;
		}
		break;
	case TYPE:
		if (cn == 0){
			V1.u1.p = (char *)this->u3.n_initializer;
			error("operator%t() not aM", &V1, ea0, ea0, ea0);
			this->n_oper = 0;
			this->u3.n_initializer = 0;
		}
		else {
			Ptype tx;
			Pname nx;
			char buf [256];
			char *bb, *p;
			int l2;

			f = (Pfct)this->u1.tp;
			tx = (Ptype)this->u3.n_initializer;
			this->u3.n_initializer = 0;
			if (f->base != FCT){
				V1.u1.p = (char *)cn;
				V2.u1.p = (char *)tx;
				error("badT for%n::operator%t()", &V1,&V2,ea0,ea0);
			}
			if (f->returns != (Ptype)defa_type){
				if (type_check(f->returns, tx, 0)) {
					V1.u1.p = (char *)cn;
					V2.u1.p = (char *)tx;
					error("bad resultT for%n::operator%t()",
						&V1, &V2, ea0, ea0);
				}
				if (TO_DEL(f->returns)) type_del(f->returns);
			}
			if (f->argtype){
				V1.u1.p = (char *)cn;
				V2.u1.p = (char *)tx;
				error("%n::operator%t()WAs", &V1, &V2, ea0, ea0);
				f->argtype = 0;
			}
			f->returns = tx;
			nx = is_cl_obj(tx);
			if (nx && can_coerce(tx, cn->u1.tp)){
				V1.u1.p = (char *)cn;
				V2.u1.p = (char *)cn;
				V3.u1.p = (char *)nx;
				V4.u1.p = (char *)tx;
				error("both %n::%n(%n) and %n::operator%t()",
					&V1, &V2, &V3, &V4);
			}
			bb = signature(tx, buf);
			l2 = bb - buf;
			if (*bb!=0) errorFIPC('d', "impossible", ea0,ea0,ea0,ea0);
			if (255<l2)
				errorFIPC('i', "N::check_oper():N buffer overflow",
					ea0, ea0, ea0, ea0);
			p = (char *) new(l2+3);
			p[0] = '_';
			p[1] = 'O';
			strcpy(p+2, buf);
			this->u2.string = p;
		}
		break;
	}
}


int inline_restr = 0;	/* report use of constructs that the inline expanded
			 * cannot handle here
			 */

void fct_dcl(this, n)
Pfct this;
Pname n;
{
	int nmem;
	Pname a, ll;
	Ptable ftbl;
	Pptr cct;
	int const_old;
	int bit_old;
	int byte_old;
	int max_old;
	int stack_old;
	Pname ax;

	nmem = TBLSIZE;
	cct = 0;
	const_old = const_save;
	bit_old = bit_offset;
	byte_old = byte_offset;
	max_old = max_align;
	stack_old = stack_size;

	if (this->base != FCT){
		V1.u1.i = (int)this->base;
		errorFIPC('i', "F::dcl(%d)", &V1, ea0, ea0, ea0);
	}
	if (this->body == 0){
		V1.u1.p = (char *)this->body;
		errorFIPC('i', "F::dcl(body=%d)", &V1, ea0, ea0, ea0);
	}
	if (n == 0 || n->base != NAME) {
		V1.u1.p = (char *)n;
		V2.u1.i = (int)(n? n->base: 0);
		errorFIPC('i', "F::dcl(N=%d %d)", &V1, &V2, ea0, ea0);
	}
	if (this->body->memtbl == 0)
		this->body->memtbl = new_table(nmem+3, n->u4.n_table, 0);
	ftbl = this->body->memtbl;
	this->body->u2.own_tbl = 1;
	ftbl->real_block = (Pstmt)this->body;

	max_align = 0;	/* AL_FRAME; */
	stack_size = byte_offset = 0;	/* SZ_BOTTOM; */
	bit_offset = 0;

	stack();
	cc->nof = n;
	cc->ftbl = ftbl;

	switch (n->n_scope){
	case 0:
	case PUBLIC:
		cc->not = n->u4.n_table->t_name;
		cc->cot = (Pclass)cc->not->u1.tp;
		cc->tot = cc->cot->this_type;
		if (this->f_this == 0 || cc->tot == 0) {
			V1.u1.p = (char *)n;
			V2.u1.p = (char *)this->f_this;
			V3.u1.p = (char *)cc->tot;
			errorFIPC('i', "F::dcl(%n): f_this=%d cc->tot=%d",
				&V1, &V2, &V3, ea0);
		}
		this->f_this->u4.n_table = ftbl; /* fake for inline printout */
		cc->c_this = this->f_this;
	}

	if (this->f_result == 0){
		/* protect against: class x; x f(); class x { x(x&); .... */
		Pname rcln;

		rcln = is_cl_obj(this->returns);
		if (rcln && has_itor((Pclass)rcln->u1.tp)) make_res(this);
	}

	/* fake for inline printout */
	if (this->f_result)this->f_result->u4.n_table = ftbl;

	for(a = this->argtype, ll = 0 ;a ;a = ax) {
		Pname nn;

		ax = a->n_list;
		nn = name_dcl(a, ftbl, ARG);
		nn->n_assigned_to = nn->n_used = nn->n_addr_taken = 0;
		nn->n_list = 0;
		switch (a->u1.tp->base){
		case CLASS:
		case ENUM:	/* unlink types declared in arg list */
			a->n_list = dcl_list;
			dcl_list = a;
			break;
		default:
			if (ll)
				ll->n_list = nn;
			else 
				this->argtype = nn;
			ll = nn;
			name__dtor(a);
		}
	}

	/* link in f_result and f_this: */
	if (this->f_result)this->f_result->n_list = this->argtype;
	if (this->f_this)
		this->f_this->n_list = (this->f_result ? this->f_result: this->argtype);

	/*
	 *	handle initializers for base class and member classes
	 *	this->f_init == list of names of classes to be initialized
	 *	no name		=> base class
	 *			=> constructor call in f_init->n_initializer
	 *	name "m"	=> member object
	 *			=> constructor call in m->n_initializer
	 */
	if (n->n_oper != CTOR) {
		if (this->f_init) errorFIPC(0, "unexpectedAL: not aK", ea0,
			ea0, ea0, ea0);
	}
	else {
		if (this->f_init){		/* explicit initializers */
			Pname bn, nx, nn;
			Ptable tbl;
			Pexpr binit;

			bn = cc->cot->clbase;
			tbl = cc->cot->memtbl;
			binit = 0;
			const_save = 1;
			for(nn = this->f_init ;nn ;nn = nx) {
				Pexpr i;
				char *s;

				nx = nn->n_list;
				i = nn->u3.n_initializer;
				s = nn->u2.string;

				if (s){
					Pname m;
					m = look(tbl, s, 0);
					if (m){
						/* class member initializer */
						if (m->u4.n_table == tbl)
							nn->u3.n_initializer =
								mem_init(this,m,i,ftbl);
						else {
							V1.u1.p  = (char *)m;
							V2.u1.p  = (char *)n;
							error("%n not inC%n", &V1,
								&V2, ea0, ea0);
							nn->u3.n_initializer = 0;
						}
					}
					else if (m = look(ktbl, s, 0)){
						/* named base class initializer */
						binit = base_init(this,bn,i,ftbl);
						nn->u3.n_initializer = 0;
					}
					else {
						V1.u1.p = (char *)m;
						V2.u1.p = (char *)n;
						error("%n not inC%n", &V1, &V2,
							ea0, ea0);
						nn->u3.n_initializer = 0;
					}
				}
				else if (bn){
					/* unnamed base class initializer */
					binit = base_init(this, bn, i, ftbl);
					nn->u3.n_initializer = 0;
				}
				else 
					error("unexpectedAL: noBC", ea0, ea0,
						ea0, ea0);
			} /* for */
			const_save = const_old;
			this->b_init = binit;
		}

		if (this->b_init == 0){
			/* try default initialization of base class */
			Pname bn;

			bn = cc->cot->clbase;
			if (bn && has_ctor((Pclass)bn->u1.tp))
				this->b_init = base_init(this, bn, 0, ftbl);
		}
	}

	PERM(this->returns);
	const_save = (this->f_inline ? 1: 0);
	inline_restr = 0;
	block_dcl(this->body, ftbl);
	if (this->f_inline && inline_restr && this->returns->base != VOID) {
		char *s;

		this->f_inline = 0;
		s = (inline_restr & 8)? "loop"
				:(inline_restr & 4)? "switch"
					:(inline_restr & 2)? "goto"
						:(inline_restr & 1)? "label"
							: "" ;
		V1.u1.p = (char *)n;
		V2.u1.p = (char *)s;
		errorFIPC('w', "\"inline\" ignored, %n contains %s", &V1,
			&V2, ea0, ea0);
	}
	const_save = const_old;

	if (this->f_inline) isf_list = new_nalist(n, isf_list);

	this->defined |= DEFINED;

	bit_offset = bit_old;
	byte_offset = byte_old;
	max_align = max_old;
	stack_size = stack_old;

	unstack();
}


/*
 *	have base class bn and expr list i
 *	return "( *(base*)this ) . ctor( i )"
 *	ctor call generated in expr.typ()
 */
Pexpr base_init(this, bn, i, ftbl)
Pfct this;
Pname bn;
Pexpr i;
Ptable ftbl;
{
	Pclass bcl;
	Pname bnw;

	bcl = (Pclass)bn->u1.tp;
	bnw = has_ctor(bcl);

	if (bnw){
		Ptype t, ty;
		Pfct f;
		Pexpr th, v;

		t = bnw->u1.tp;
		f = (Pfct)((t->base == FCT)? t:((Pgen)t)->fct_list->f->u1.tp);
		ty = f->f_this->u1.tp;			/* this */
		th = new_texpr(CAST,ty,this->f_this);	/*(base*)this */
		v = new_texpr(VALUE, bcl, i);		/* ?.base(i) */
		v->u3.e2 = new_expr(0,DEREF,th,0);	/*(*(base*)this).base(i) */
		v = expr_typ(v, ftbl);			/* *base(&*(base*)this,i) */

		switch (v->base){
		case DEREF:
		{
			Pexpr vv;

			vv = v;
			v = v->u2.e1;			/* base(&*(base*)this,i) */
			break;
		}
		case ASSIGN:	/* degenerate base(base&): *(base*)this=i */
			th = new_texpr(CAST, ty, this->f_this);
			v = new_expr(0,CM,v,th);
			/*(*(base*)this=i,(base*)this); */
			v = expr_typ(v, ftbl);
			break;
		default:
			V1.u1.i = (int)v->base;
			errorFIPC('i', "F::base_init: unexpected %k", &V1,
				ea0, ea0, ea0);
		}
		return v;
	}
	else
		errorFIPC(0, "unexpectedAL: noBCK", ea0, ea0, ea0, ea0);
	return 0;
}


/*
 *	return "member_ctor( m, i )"
 */
Pexpr mem_init(this, member, i, ftbl)
Pfct this;
Pname member;
Pexpr i;
Ptable ftbl;
{
	Pname cn;
	Pexpr tn;

	if (member->n_stclass == STATIC){
		V1.u1.p = (char *)member;
		errorFIPC('s', "MIr for static%n", &V1, ea0, ea0, ea0);
	}
	if (i)i = expr_typ(i, ftbl);
	cn = is_cl_obj(member->u1.tp);		/* first find the class name */
	if (cn){
		Pclass mcl;			/* then find the classdef */
		Pname ctor, icn;

		mcl = (Pclass)cn->u1.tp;
		ctor = has_ctor(mcl);
		if (has_itor(mcl) == 0 && i &&(icn=is_cl_obj(i->u1.tp))
		&&(Pclass)icn->u1.tp == mcl) {	/* bitwise copy */
			Pexpr init;

			tn = new_ref(REF, this->f_this, member);
			init = new_expr(0, ASSIGN, tn, i);
			return expr_typ(init, ftbl);
		}
		else if (ctor){		/* generate: this->member.cn::cn(args) */
			Pexpr ct, c;

			tn = new_ref(REF, this->f_this, member);
			ct = new_ref(DOT, tn, ctor);
			c = new_expr(0, G_CALL, ct, i);
			return expr_typ(c, ftbl);
		}
		else {
			V1.u1.p = (char *)member;
			error("Ir forM%nW noK", &V1, ea0, ea0, ea0);
			return 0;
		}
	}

	if (cl_obj_vec){
		errorFIPC('s', "Ir forCM vectorWK", ea0, ea0, ea0, ea0);
		return 0;
	}

	if (i->u3.e2){				/* i is an ELIST */
		V1.u1.p = (char *)member;
		error("Ir for%m not a simpleE", &V1, ea0, ea0, ea0);
	}
	i = i->u2.e1;
	tn = new_ref(REF, this->f_this, member);

	if (tconst(member->u1.tp)){
		TOK t;

		t = set_const(member->u1.tp, 0);
		switch (t){
		case ANY:
		case VEC:
		case RPTR:
			V1.u1.p = (char *)member;
			error("MIr for%kM%n", &V1, ea0, ea0, ea0);
			return 0;
		}
		i = new_expr(0, ASSIGN, tn, i);
		i = expr_typ(i, ftbl);
		set_const(member->u1.tp, 1);
		return i;
	}

	if (is_ref(member->u1.tp)){
		i = ref_init((Pptr)member->u1.tp, i, ftbl);
		i = new_expr(0, ASSIGN, tn, i);
		assign(member);	/* cannot call typ: would cause dereference */
		return i;
	}

	i = new_expr(0, ASSIGN, tn, i);
	return expr_typ(i, ftbl);
}


/*
 *	e is on the form
 *				f(&temp,arg) , temp
 *			or
 *				&temp->ctor(arg) , temp
 *			or
 *				x->f(&temp,arg) , temp
 *	change it to
 *				f(n,arg)
 *			or
 *				n->ctor(arg)
 */
Pexpr replace_temp(e, n)
Pexpr e, n;
{
	Pexpr c;		/* f(&temp,arg) or &temp->ctor(args) */
	Pexpr ff;
	Pexpr a;		/* maybe ELIST(&temp,arg) */
	Pexpr tmp;

	c = e->u2.e1;
	ff = c->u2.e1;
	a = c->u3.e2;
	tmp = e->u3.e2;

	if (tmp->base == DEREF)tmp = tmp->u2.e1;
	if (tmp->base == CAST)tmp = tmp->u2.e1;
	if (tmp->base == ADDROF || tmp->base == G_ADDROF) tmp = tmp->u3.e2;
	if (tmp->base != NAME){
		V1.u1.i = (int)tmp->base;
		errorFIPC('i', "replace %k", &V1, ea0, ea0, ea0);
	}
	tmp->u1.tp = (Ptype)any_type;	/* temporary not used: suppress it */

	switch (ff->base){
	case REF:
		if (ff->u2.e1->base == G_ADDROF && ff->u2.e1->u3.e2 == tmp)
			a = ff;				/* &tmp -> f() */
		break;
	case DOT:
		if (ff->u2.e1->base == NAME && ff->u2.e1 == tmp) {
			a = ff;				/* tmp . f() */
			a->base = REF;
		}
		break;
	}
	a->u2.e1 = n;
	return c;
}

/*
 *	does this class have a constructor taking no arguments?
 */
Pname has_ictor(this)
Pclass this;
{
	Pname c;
	Pfct f;
	Plist l;

	c = has_ctor(this);

	if (c == 0) return 0;

	f = (Pfct)c->u1.tp;

	switch (f->base){
	default:
		V1.u1.p = (char *)this->string;
		V2.u1.i = (int)c->u1.tp->base;
		errorFIPC('i', "%s: badK(%k)", &V1, &V2, ea0, ea0);

	case FCT:
		switch (f->nargs){
		case 0:		return c;
		default:	if (f->argtype->u3.n_initializer)return c;
		}
		return 0;

	case OVERLOAD:
		for(l = ((Pgen)f)->fct_list ;l ;l = l->l) {
			Pname n;

			n = l->f;
			f = (Pfct)n->u1.tp;
			switch (f->nargs){
			case 0:		return n;
			default:	if (f->argtype->u3.n_initializer)return n;
			}
		}
		return 0;
	}
}

Pgen new_gen(s)
char *s;
{
	char *p;
	Pgen this;

	this = (Pgen) new(sizeof(struct gen));
	p = (char *) new(strlen(s)+1);
	strcpy(p, s);
	this->string = p;
	this->base = OVERLOAD;
	this->fct_list = 0;
	return this;
}


/*
 *	add "n" to the tail of "fct_list"
 *	(overloaded names are searched in declaration order)
 *
 *	detect:	 	multiple identical declarations
 *			declaration after use
 *			multiple definitions
 */
Pname add(this, n, sig)
Pgen this;
Pname n;
int sig;
{
	Pfct f;
	Pname nx;

	f = (Pfct)n->u1.tp;
	if (f->base != FCT){
		V1.u1.p = (char *)n;
		error("%n: overloaded nonF", &V1, ea0, ea0, ea0);
	}
	if (this->fct_list && (nx=find(this,f,1)) )
		Nold = 1;
	else {
		char *s;

		s = this->string;
		if (this->fct_list || sig || n->n_oper){
			char buf[256];
			char *bb, *p;
			int l1, l2;

			bb = signature(n->u1.tp, buf);
			l2 = bb - buf;
			if (*bb != 0)
				errorFIPC('d', "impossible sig", ea0,ea0,ea0,ea0);
			if (255 < l2)
				errorFIPC('i', "gen::add():N buffer overflow",
					ea0, ea0, ea0, ea0);
			l1 = strlen(s);
			p = (char *) new(l1+l2+1);
			strcpy(p, s);
			strcpy(p+l1, buf);
			n->u2.string = p;
		}
		else 
			n->u2.string = s;

		nx = new_name(0);
		*nx = *n;
		PERM(nx);
		Nold = 0;
		if (this->fct_list){
			Plist gl;

			for(gl = this->fct_list ;gl->l ;gl = gl->l)
				/* EMPTY */;
			gl->l = new_nalist(nx, 0);
		}
		else 
			this->fct_list = new_nalist(nx, 0);
		nx->n_list = 0;
	}
	return nx;
}

extern bit const_problem;

Pname find(this, f, warn)
Pgen this;
Pfct f;
bit warn;
{
	Plist gl;

	for(gl = this->fct_list ;gl ;gl = gl->l) {
		Pname nx, a, ax;
		Pfct fx;
		int vp, cp, op, xp, ma, acnt;

		nx = gl->f;
		fx = (Pfct)nx->u1.tp;
		vp = 0;
		cp = 0;
		op = 0;
		xp = 0;
		ma = 0;

		if (fx->nargs_known != f->nargs_known
		&& fx->nargs
		&& fx->nargs_known != ELLIPSIS)
			continue;

		acnt = (fx->nargs > f->nargs)? fx->nargs: f->nargs;
		for(ax=fx->argtype,a=f->argtype; a&&ax; ax=ax->n_list,a=a->n_list)
		{
			Ptype at, atp;
			int atpc, rr;

			at = ax->u1.tp;
			atp = a->u1.tp;
			atpc = 0;
			rr = 0;
			acnt--;

			/*
			 * warn against:
			 *	overload f(X&), f(X);
			 * and
			 *	overload f(int), f(enum e);
			 * and
			 *	overload f(int), f(const);
			 * and
			 *	overload f(char), f(int);
			 * and
			 *	overload f(x&), f(const x&);
			 */
		aaa:
			switch (atp->base){
			case TYPE:
				if (((Pbase)atp)->b_const)atpc = 1;
				atp = ((Pbase)atp)->b_name->u1.tp;
				goto aaa;
			case RPTR:
				if (warn && type_check(((Pptr)atp)->typ,at,0) == 0)
					op += 1;
				rr = 1;
			}

		atl:
			switch (at->base){
			case TYPE:
				if (((Pbase)at)->b_const != atpc)cp = 1;
				at = ((Pbase)at)->b_name->u1.tp;
				goto atl;
			case RPTR:
				if (warn && type_check(((Pptr)at)->typ,atp,0) == 0)
					op += 1;
				rr = 1;
				goto defa;
			case FLOAT:
			case DOUBLE:
				if (warn){
					if (atp->base != at->base){
						switch (atp->base){
						case FLOAT:
						case DOUBLE:	op += 1;
						}
					}
					else xp += 1;
				}
				goto defa;
			case EOBJ:
				if (warn){
					if (atp->base != at->base){
						switch (atp->base){
						case CHAR:
						case SHORT:
						case INT:	op += 1;
						}
					}
					else if (((Pbase)atp)->b_name->u1.tp != ((Pbase)at)->b_name->u1.tp)
						op += 1;
					else xp += 1;
				}
				goto defa;
			case CHAR:
			case SHORT:
			case INT:
				if (warn){
					if (atp->base != at->base){
						switch (atp->base){
						case SHORT:
						case INT:
						case CHAR:
						case EOBJ:	op += 1;
						}
					}
					else xp += 1;
				}

			case LONG:
				if (type_check(at, atp, 0)) {
					if (const_problem)cp = 1;
					if (acnt)
						{ ma = 1; break; }
					else goto xx;
				}
			default:
			defa:
				if (type_check(at, atp, 0)){
					if (const_problem && rr==0) cp = 1;
					if (acnt)
						{ ma = 1; break; }
					else goto xx;
				}
				if (const_problem)cp = 1;
				if (vrp_equiv)vp = 1;
			}
		}

		if (ma)goto xx;

		if (ax){
			if (warn && ax->u3.n_initializer) {
				V1.u1.p = (char *)nx;
				error("Ir makes overloaded%n ambiguous",
					&V1, ea0, ea0, ea0);
			}
			continue;
		}

		if (a){
			if (warn && a->u3.n_initializer) {
				V1.u1.p = (char *)nx;
				error("Ir makes overloaded%n ambiguous",
					&V1, ea0, ea0, ea0);
			}
			continue;
		}

		if (warn && type_check(fx->returns, f->returns, 0)) {
			V1.u1.p = (char *)nx;
			V2.u1.p = (char *)fx->returns;
			V3.u1.p = (char *)f->returns;
			error("two different return valueTs for overloaded%n: %t and %t",
				&V1, &V2, &V3, ea0);
		}
		if (warn){
			if (vp) errorFIPC('w', "ATs differ(only): [] vs *",
					ea0, ea0, ea0, ea0);
			if (op || cp){
				V1.u1.p = (char *)fx;
				V2.u1.p = (char *)f;
				errorFIPC('w',
				"the overloading mechanism cannot tell a%t from a%t",
					&V1, &V2, ea0, ea0);
			}
		}
		return nx;
	xx:
		if (warn && (int)fx->nargs <= vp+op+cp+xp && fx->nargs == f->nargs) {
			/* warn only in all argument are ``similar'', not for:
			 * overload f(int,double), f(const,int);
			 */
			if (vp) errorFIPC('w', "ATs differ(only): [] vs *",
					ea0, ea0, ea0, ea0);
			if (op || cp){
				V1.u1.p = (char *)fx;
				V2.u1.p = (char *)f;
				errorFIPC('w',
				"the overloading mechanism cannot tell a%t from a%t",
					&V1, &V2, ea0, ea0);
			}
		}
	}

	return 0;
}

int no_of_names(this)
Pname this;
{
	register int i;
	register Pname n;

	i = 0;
	for(n = this ;n ;n = n->n_list) i++;
	return i;
}

static Pexpr lvec[20], *lll;
static Pexpr list_back = 0;

void new_list(lx)
Pexpr lx;
{
	if (lx->base != ILIST) errorFIPC('i', "IrLX", ea0, ea0, ea0, ea0);

	lll = lvec;
	lll++;
	*lll = lx->u2.e1;
}

Pexpr next_elem()
{
	Pexpr e, lx;

	if (lll == lvec) return 0;

	lx = *lll;

	if (list_back){
		e = list_back;
		list_back = 0;
		return e;
	}

	if (lx == 0){				/* end of list */
		lll--;
		return 0;
	}

	switch (lx->base){
	case ELIST:
		e = lx->u2.e1;
		*lll = lx->u3.e2;
		switch (e->base){
		case ILIST:
			lll++;
			*lll = e->u2.e1;
			return (Pexpr)1;	/* start of new ILIST */
		case ELIST:
			error("nestedEL", ea0, ea0, ea0, ea0);
			return 0;
		default:
			return e;
		}
	default:
		errorFIPC('i', "IrL", ea0, ea0, ea0, ea0);
	}
}

/*
 *	see if the list "lll" can be assigned to something of type "t"
 *	"nn" is the name of the variable for which the assignment is taking place.
 *	"il" is the last list element returned by next_elem()
 */
void list_check(nn, t, il)
Pname nn;
Ptype t;
Pexpr il;
{
	Pexpr e;
	bit lst;
	int i;
	int tdef;
	Pclass cl;

	lst = 0;
	tdef = 0;
	if (il == (Pexpr)1)
		lst = 1;
	else if (il)
		list_back = il;

zzz:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		tdef += 1;
		goto zzz;

	case VEC:
	{	Pvec v;
		Ptype vt;

		v = (Pvec)t;
		vt = v->typ;

		if (v->size){	/* get at most v->size initializers */
			if (v->typ->base == CHAR){
				e = next_elem();
				if (e->base == STRING){	/* v[size] = "..." */
					int isz;

					isz = ((Pvec)e->u1.tp)->size;
					if (v->size < isz){
						V1.u1.i = isz;
						V2.u1.p =(char *)nn;
						V3.u1.i = v->size;
						error("Ir too long(%d characters) for%n[%d]",
							&V1, &V2, &V3, ea0);
						}
					break;
				}
				else 
					list_back = e;
			}
			for(i=0; i<v->size; i++) {
				/* check next list element type */
			ee:
				e = next_elem();
				if (e == 0)goto xsw;
				/* too few initializers are ok */
			vtz:
				switch (vt->base){
				case TYPE:
					vt = ((Pbase)vt)->b_name->u1.tp;
					goto vtz;
				case VEC:
				case COBJ:
					list_check(nn, vt, e);
					break;
				default:
					if (e == (Pexpr)1) {
						error("unexpectedIrL", ea0, ea0,
							ea0, ea0);
						goto ee;
					}
					if (type_check(vt,e->u1.tp,ASSIGN)) {
						V1.u1.p = (char *)nn;
						V2.u1.p = (char *)e->u1.tp;
						V3.u1.p = (char *)vt;
						error("badIrT for%n:%t(%tX)",
							&V1, &V2, &V3, ea0);
					}
				}
			}
			if (lst &&(e=next_elem()) )
				error("end ofIrLX after vector", ea0,ea0,ea0,ea0);
		xsw:;
		}
		else {		/* determine v->size */
			i = 0;
		xx:
			while ( e=next_elem() ) {	/* get another initializer */
				i++;
			vtzz:
				switch (vt->base){
				case TYPE:
					vt = ((Pbase)vt)->b_name->u1.tp;
					goto vtzz;
				case VEC:
				case COBJ:
					list_check(nn, vt, e);
					break;
				default:
					if (e == (Pexpr)1) {
						error("unexpectedIrL", ea0, ea0,
							ea0, ea0);
						goto xx;
					}
					if (type_check(vt, e->u1.tp, ASSIGN)) {
						V1.u1.p = (char *)nn;
						V2.u1.p = (char *)e->u1.tp;
						V3.u1.p = (char *)vt;
						error("badIrT for%n:%t(%tX)",
							&V1, &V2, &V3, ea0);
					}
				}
			}
			if (!tdef) v->size = i;
		}
		break;
	}

	case CLASS:
		cl = (Pclass)t;
		goto ccc;

	case COBJ:	/* initialize members */
		cl = (Pclass)((Pbase)t)->b_name->u1.tp;
	ccc:
	{
		Ptable tbl;
		Pname m;

		tbl = cl->memtbl;
		if (cl->clbase) list_check(nn, cl->clbase->u1.tp, 0);

		for(m=get_mem(tbl, i=1); m; m=get_mem(tbl,++i)) {
			Ptype mt;

			mt = m->u1.tp;
			switch (mt->base){
			case FCT:
			case OVERLOAD:
			case CLASS:
			case ENUM:
				continue;
			}
			if (m->n_stclass == STATIC)continue;
			/* check assignment to next member */
		dd:
			e = next_elem();
			if (e == 0)return;
		mtz:
			switch (mt->base){
			case TYPE:
				mt = ((Pbase)mt)->b_name->u1.tp;
				goto mtz;
			case CLASS:
			case ENUM:
				break;
			case VEC:
			case COBJ:
				list_check(nn, m->u1.tp, e);
				break;
			default:
				if (e == (Pexpr)1) {
					error("unexpectedIrL", ea0, ea0, ea0, ea0);
					goto dd;
				}
				if (type_check(mt, e->u1.tp, ASSIGN)) {
					V1.u1.p = (char *)m;
					V2.u1.p = (char *)e->u1.tp;
					V3.u1.p = (char *)m->u1.tp;
					error("badIrT for%n:%t(%tX)", &V1,
						&V2, &V3, ea0);
				}
			}
		}
		if (lst &&(e=next_elem()) )
			error("end ofIrLX afterCO", ea0, ea0, ea0, ea0);
		break;
	}
	default:
		e = next_elem();

		if (e == 0){
			error("noIr forO", ea0, ea0, ea0, ea0);
			break;
		}

		if (e == (Pexpr)1) {
			error("unexpectedIrL", ea0, ea0, ea0, ea0);
			break;
		}
		if (type_check(t, e->u1.tp, ASSIGN)){
			V1.u1.p = (char *)nn;
			V2.u1.p = (char *)e->u1.tp;
			V3.u1.p = (char *)t;
			error("badIrT for%n:%t(%tX)", &V1, &V2, &V3, ea0);
		}
		if (lst &&(e=next_elem()) )
			error("end ofIrLX afterO", ea0, ea0, ea0, ea0);
		break;
	}
}

Pname dclass(n, tbl)
Pname n;
Ptable tbl;
{
	Pclass cl;
	Pbase bt;
	Pname bn, nx, ln, bnn;

	nx = look(ktbl, n->u2.string, 0);		/* TNAME */
	if (nx == 0){
	/*	search for hidden name for
	 *	(1) nested class declaration
	 *	(2) local class declaration
	 */
		int tn;

		tn = 0;
		for(nx = look(ktbl,n->u2.string,HIDDEN) ;nx ;nx = nx->n_tbl_list) {
			if (nx->n_key != HIDDEN)continue;
			if (nx->u1.tp->base != COBJ){
				tn = 1;
				continue;
			}
			bt = (Pbase)nx->u1.tp;
			bn = bt->b_name;
			cl = (Pclass)bn->u1.tp;
			if (cl == 0)continue;
			goto bbb;
		}

		V1.u1.p = (char *)n;
		if (tn)
			error("%n redefined using typedef", &V1, ea0, ea0, ea0);
		else 
			errorFIPC('i', "%n is not aCN", &V1, ea0, ea0, ea0);
	}
	else {
		bt = (Pbase)nx->u1.tp;			/* COBJ */
		bn = bt->b_name;
	}
bbb:
	ln = look(tbl, bn->u2.string, 0);
	if (ln && ln->u4.n_table == tbl) {
		V1.u1.p = (char *)ln;
		errorFIPC('w', "%n redefined", &V1, ea0, ea0, ea0);
	}
	bn->where = nx->where;
	bnn = insert(tbl, bn, CLASS);		/* copy for member lookup */
	cl = (Pclass)bn->u1.tp;
						/* CLASS */
	if (cl->defined &(DEFINED|SIMPLIFIED))
	{
		V1.u1.p = (char *)n;
		error("C%n defined twice", &V1, ea0, ea0, ea0);
	} else {
		if (bn->n_scope == ARG) bn->n_scope = ARGT;
		classdef_dcl(cl, bn, tbl);
	}
	n->u1.tp = (Ptype)cl;
	return bnn;
}

Pname denum(n, tbl)
Pname n;
Ptable tbl;
{
	Pname nx, bn, bnn;
	Pbase bt;
	Penum en;

	nx = look(ktbl, n->u2.string, 0);		/* TNAME */
	if (nx == 0) nx = look(ktbl, n->u2.string, HIDDEN); /* hidden TNAME */
	bt = (Pbase)nx->u1.tp;				/* EOBJ */
	bn = bt->b_name;
	bnn = insert(tbl, bn, CLASS);
	en = (Penum)bn->u1.tp;				/* ENUM */
	if (en->defined &(DEFINED|SIMPLIFIED))
	{
		V1.u1.p = (char *)n;
		error("enum%n defined twice", &V1, ea0, ea0, ea0);
	} else {
		if (bn->n_scope == ARG) bn->n_scope = ARGT;
		enumdef_dcl(en, tbl);
	}
	n->u1.tp = (Ptype)en;
	return bnn;
}

void dargs(f, tbl)
Pfct f;
Ptable tbl;
{
	int oo;
	Pname a;

	oo = const_save;
	const_save = 1;
	for(a = f->argtype ;a ;a = a->n_list) {
		Pexpr init;
		Pname cln;

		cln = is_cl_obj(a->u1.tp);
		if (cln && has_itor((Pclass)cln->u1.tp)) {
			/* mark X(X&) arguments */
			a->n_xref = 1;
		}
		if (init = a->u3.n_initializer){ /* default argument */
			if (cln) {
				Pexpr i;

				if (init->base==VALUE) {
					Pname n2;

					switch (init->u4.tp2->base){
					case CLASS:
						if ((Pclass)init->u4.tp2 != (Pclass)cln->u1.tp)
							goto inin2;
						break;
					default:
						n2 = is_cl_obj(init->u4.tp2);
						if (n2==0 ||(Pclass)n2->u1.tp!=(Pclass)cln->u1.tp)
							goto inin2;
					}
					init->u3.e2 = (Pexpr)a;
					init = expr_typ(init, tbl);
					expr_simpl(init);
					init->permanent = 2;
					a->u3.n_initializer = init;
					errorFIPC('s', "K as defaultA",
						ea0, ea0, ea0, ea0);
				}
				else {
				inin2:
					if (init->base == ILIST)
						errorFIPC('s', "list as AIr",
							ea0, ea0, ea0, ea0);
					i = expr_typ(init, tbl);
					init = class_init((Pexpr)a,a->u1.tp,i,tbl);
					if (i!=init && init->base == DEREF)
						errorFIPC('s', "K needed forAIr",
							ea0, ea0, ea0, ea0);
					expr_simpl(init);
					init->permanent = 2;
					a->u3.n_initializer = init;
				}
			}
			else if (is_ref(a->u1.tp)){
				int tcount;

				init = expr_typ(init, tbl);
				tcount = stcount;
				init = ref_init((Pptr)a->u1.tp, init, tbl);
				if (tcount != stcount)
					errorFIPC('s',
						"needs temporaryV to evaluateAIr",
						ea0, ea0, ea0, ea0);
				expr_simpl(init);
				init->permanent = 2;
				a->u3.n_initializer = init;
			}
			else {
				init = expr_typ(init, tbl);
				if (type_check(a->u1.tp, init->u1.tp, ARG)) {
					int i;

					i = can_coerce(a->u1.tp, init->u1.tp);
					switch (i){
					case 1:
						if (Ncoerce){
							Pname cn;
							Pclass cl;
							Pexpr r;

							cn = is_cl_obj(init->u1.tp);
							cl = (Pclass)cn->u1.tp;
							r=new_ref(DOT,init,Ncoerce);
							init=new_expr(0,G_CALL,r,0);
							init->u4.fct_name = Ncoerce;
							init->u1.tp = a->u1.tp;
						}
						break;
					default:
						V1.u1.i = i;
						error("%d possible conversions for defaultA",
							&V1, ea0, ea0, ea0);
					case 0:
						V1.u1.p = (char *)init->u1.tp;
						V2.u1.p = (char *)a;
						error("badIrT%t forA%n",
							&V1, &V2, ea0, ea0);
						if (TO_DEL(init)) expr_del(init);
						a->u3.n_initializer = init = 0;
					}
				}

				if (init){
					int i;

					expr_simpl(init);
					init->permanent = 2;
					a->u3.n_initializer = init;
					Neval = 0;
					i = eval(init);
					if (Neval == 0){
						a->n_evaluated = 1;
						a->n_val = i;
					}
				}
			}
		}
	}
	const_save = oo;
}


void merge_init(nn, f, nf)
Pname nn;
Pfct f;
Pfct nf;
{
	Pname a1;
	Pname a2;

	a1 = f->argtype;
	a2 = nf->argtype;
	for(; a1; a1=a1->n_list, a2=a2->n_list) {
		int i1, i2;

		i1 = a1->u3.n_initializer || a1->n_evaluated;
		i2 = a2->u3.n_initializer || a2->n_evaluated;
		if (i1) {
			if (i2
			&&(	a1->n_evaluated == 0
				|| a2->n_evaluated == 0
				|| a1->n_val != a2->n_val)
			) {
				V1.u1.p = (char *)nn;
				V2.u1.p = (char *)a1;
				error("twoIrs for%nA%n", &V1, &V2, ea0, ea0);
			}
		}
		else if (i2){
			a1->u3.n_initializer = a2->u3.n_initializer;
			a1->n_evaluated = a2->n_evaluated;
			a1->n_val = a2->n_val;
		}
	}
}

/*
 *	``e'' is of class ``cn'' coerce it to type ``rt''
 */
Pexpr try_to_coerce(rt, e, s, tbl)
Ptype rt;
Pexpr e;
char *s;
Ptable tbl;
{
	int i;
	Pname cn;

	if ((cn=is_cl_obj(e->u1.tp))
	&&(i=can_coerce(rt,e->u1.tp))
	&& Ncoerce) {
		Pclass cl;
		Pexpr r, rr, c;

		if (1 < i){
			V1.u1.i = i;
			V2.u1.p = (char *)s;
			error("%d possible conversions for %s", &V1, &V2, ea0, ea0);
		}
		cl = (Pclass)cn->u1.tp;
		r = new_ref(DOT, e, Ncoerce);
		rr = expr_typ(r, tbl);
		c = new_expr(0, G_CALL, rr, 0);
		c->u4.fct_name = Ncoerce;

		return expr_typ(c, tbl);
	}

	return 0;
}

int in_class_dcl = 0;
Pname dofct(this, tbl, scope)
Pname this;
Ptable tbl;
TOK scope;
{
	Pfct f;
	Pname class_name;
	Ptable etbl;
	int can_overload;
	int just_made, i;
	Pname nn, a;

	f = (Pfct)this->u1.tp;
	just_made = 0;
	in_class_dcl = cc->not != 0;
	if (f->f_inline)this->n_sto = STATIC;

	if (f->argtype) dargs(f,tbl);

	type_dcl(this->u1.tp, tbl);	/* must be done before the type check */
	if (this->u5.n_qualifier){	/* qualified name: c::f() checked above */
		if (in_class_dcl){
			V1.u1.p = (char *)this;
			error("unexpectedQdN%n", &V1, ea0, ea0, ea0);
			return 0;
		}
		class_name = ((Pbase)this->u5.n_qualifier->u1.tp)->b_name;
		etbl = ((Pclass)class_name->u1.tp)->memtbl;
		if (f->f_virtual){
			V1.u1.p = (char *)class_name->u2.string;
			V2.u1.p = (char *)this;
			error("virtual outsideCD %s::%n", &V1, &V2, ea0, ea0);
			f->f_virtual = 0;
		}
	}
	else {
		class_name = cc->not;
		/* beware of local function declarations in member functions */
		if (class_name && tbl != cc->cot->memtbl) {
			class_name = 0;
			in_class_dcl = 0;
		}
		if (this->n_oper)check_oper(this, class_name);
		etbl = tbl;
	}

	((Pfct)this->u1.tp)->memof = (class_name) ?(Pclass)class_name->u1.tp: 0;

	if (etbl == 0 || etbl->base != TABLE) {
		V1.u1.p = (char *)etbl;
		errorFIPC('i', "N::dcl: etbl=%d", &V1, ea0, ea0, ea0);
	}
	switch (this->n_oper){
	case NEW:
	case DELETE:
		switch (scope){
		case 0:
		case PUBLIC:
			V1.u1.p = (char *)this;
			error("%nMF", &V1, ea0, ea0, ea0);
		}
	case 0:
		can_overload = in_class_dcl;
		break;
	case CTOR:
		if (f->f_virtual){
			error("virtualK", ea0, ea0, ea0, ea0);
			f->f_virtual = 0;
		}
	case DTOR:
		if (fct_void)this->n_scope = PUBLIC;
		can_overload = in_class_dcl;
		break;
	case TYPE:
		can_overload = 0;
		break;
	case ASSIGN:
		if (class_name && f->nargs==1) {
			Ptype t;
			Pname an;

			t = f->argtype->u1.tp;
			an = is_cl_obj(t);		/* X::operator=(X) ? */
			if (an == 0 && is_ref(t)) {	/* X::operator=(X&) ? */
				t = ((Pptr)t)->typ;
			rx1:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto rx1;
				case COBJ:
					an = ((Pbase)t)->b_name;
				}
			}
			if (an && an == class_name)
				((Pclass)an->u1.tp)->bit_ass = 0;
		}
		else if (f->nargs == 2){
			Ptype t;
			Pname an1, an2;

			t = f->argtype->u1.tp;
			if (is_ref(t)){	/* perator=(X&,?) ? */
				t = ((Pptr)t)->typ;
			rx2:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto rx2;
				case COBJ:
					an1 = ((Pbase)t)->b_name;
				}
			}
			t = f->argtype->n_list->u1.tp;
			an2 = is_cl_obj(t);		/* operator=(X&,X) ? */
			if (an2 == 0 && is_ref(t)){	/* operator=(X&,X&) ? */
				t = ((Pptr)t)->typ;
			rx3:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto rx3;
				case COBJ:
					an2 = ((Pbase)t)->b_name;
				}
			}
			if (an1 && an1==an2)((Pclass)an1->u1.tp)->bit_ass = 0;
		}
	default:
		can_overload = 1;	/* all operators are overloaded */
	}

	switch (scope){
	case FCT:
	case ARG:
	{	Pname nx;

		nx = insert(gtbl, this, 0);
		this->u4.n_table = 0;
		this->n_tbl_list = 0;
		if (Nold && type_check(this->u1.tp, nx->u1.tp, 0)) {
			V1.u1.p = (char *)this;
			V2.u1.p = (char *)nx->u1.tp;
			V3.u1.p = (char *)this->u1.tp;
			errorFIPC('w', "%n has been declared both as%t and as%t",
				&V1, &V2, &V3, ea0);
		}
	}
	}

	nn = insert(etbl, this, 0);
	assign(nn);
	this->u4.n_table = etbl;

	if (Nold){
		Pfct nf;

		nf = (Pfct)nn->u1.tp;
		if (nf->base == ANY || f->base == ANY)
			;
		else if (nf->base == OVERLOAD){
			Pgen g;

			g = (Pgen)nf;
			nn = add(g, this, 0);
			this->u2.string = nn->u2.string;
			if (Nold == 0){
				if (f->body){
					if (this->u5.n_qualifier){
						V1.u1.p=(char *)this->u5.n_qualifier;
						V2.u1.p = (char *)g->string;
						error("badAL for overloaded %n::%s()"
							, &V1, &V2, ea0, ea0);
						return 0;
					}
				}
				goto thth;
			}
			else {
				if (f->body == 0 && friend_in_class == 0) {
					V1.u1.p = (char *)nn;
					errorFIPC('w', "overloaded%n redeclared",
						&V1, ea0, ea0, ea0);
				}
			}

			nf = (Pfct)nn->u1.tp;

			if (f->body && nf->body){
				V1.u1.p = (char *)nn;
				error("two definitions of overloaded%n",
					&V1, ea0, ea0, ea0);
				return 0;
			}

			if (f->body) goto bdbd;

			goto stst;
		}
		else if (nf->base != FCT) {
			error("%n declared both as%t and asF", &V1, &V2, ea0, ea0);
			f->body = 0;
		}
		else if (can_overload) {
			if (type_check((Ptype)nf,(Ptype)f, OVERLOAD) || vrp_equiv){
				Pgen g;
				Pname n1, n2;

				if (f->body && this->u5.n_qualifier){
					V1.u1.p = (char *)nn;
					error("badT for%n", &V1, ea0, ea0, ea0);
					return 0;
				}
				g = new_gen(this->u2.string);
				n1 = add(g, nn, in_class_dcl);
				n2 = add(g, this, 0);
				nn->u1.tp = (Ptype)g;
				nn->u2.string = g->string;
				nn = n2;
				goto thth;
			}

			if (in_class_dcl) {
				V1.u1.p = (char *)this;
				error("twoDs of%n", &V1, ea0, ea0, ea0);
				f->body = 0;
				return 0;
			}

			if (nf->body && f->body) {
				V1.u1.p = (char *)this;
				error("two definitions of%n", &V1, ea0, ea0, ea0);
				f->body = 0;
				return 0;
			}

			if (f->body) goto bdbd;

			goto stst;
		}
		else if (type_check((Ptype)nf,(Ptype)f, 0)){
			switch (this->n_oper){
			case CTOR:
			case DTOR:
				f->s_returns = nf->s_returns;
			}
			V1.u1.p = (char *)this;
			V2.u1.p = (char *)nf;
			V3.u1.p = (char *)f;
			error("%nT mismatch:%t and%t", &V1, &V2, &V3, ea0);
			f->body = 0;
		}
		else if (nf->body && f->body) {
			V1.u1.p = (char *)this;
			error("two definitions of%n", &V1, ea0, ea0, ea0);
			f->body = 0;
		}
		else if (f->body) {
		bdbd:
			if (f->nargs_known && nf->nargs_known) merge_init(nn,f,nf);
			f->f_virtual = nf->f_virtual;
			f->f_this = nf->f_this;
			f->f_result = nf->f_result;
			f->s_returns = nf->s_returns;
			nn->u1.tp = (Ptype)f;
			if (f->f_inline) {
				if (nf->f_inline == 0) {
					if (nn->n_used) {
						V1.u1.p = (char *)nn;
						error("%n called before defined as inline",
							&V1, ea0, ea0, ea0);
					}
					f->f_inline = nf->f_inline = 2;
					/* can never be "outlined" 
					 * because it has already
					 * been declared extern
					 */
				}
				else 
					nf->f_inline = 1;
				nn->n_sto = STATIC;
			}
			else if (nf->f_inline) {
				f->f_inline = 1;
			}
			goto stst2;
		}
		else {	/* two declarations */
			f->f_this = nf->f_this;
			f->f_result = nf->f_result;
			f->s_returns = nf->s_returns;
		stst:
			if (f->nargs_known && nf->nargs_known) merge_init(nn,f,nf);
		stst2:
			if (f->f_inline) this->n_sto = STATIC;
			if (this->n_sto
			&& nn->n_scope != this->n_sto
			&& friend_in_class == 0
			&& f->f_inline==0){ /* allow re-def to "static" */
				switch (this->n_sto){
				case STATIC:
					nn->n_sto = STATIC;
					break;
				case EXTERN:
					if ( nn->n_scope == PUBLIC ||
						nn->n_scope == 0)
						break;
				default:
					V1.u1.p = (char *)this;
					V2.u1.i = (int)this->n_sto;
					V3.u1.i = (int)nn->n_scope;
					error("%n both%k and%k", &V1, &V2, &V3, ea0);
				}
			}
			this->n_scope = nn->n_scope;	/* first specifier wins */
			this->n_sto = nn->n_sto;

		}
	/*	((Pfct)nn->tp)->nargs_known = nf->nargs_known;	*/
	}
	else {	/* new function: make f_this for member functions */
		if (tbl == gtbl && this->n_oper){	/* overload operator */
			Pgen g;
			Pname n1;

			g = new_gen(this->u2.string);
			n1 = add(g, nn, 1);
			nn->u1.tp = (Ptype)g;
			nn->u2.string = g->string;
			this->u2.string = n1->u2.string;
			nn = n1;
		}
	thth:
		just_made = 1;
		if (f->f_inline)
			nn->n_sto = STATIC;
		else if (class_name == 0 && this->n_sto == 0 && f->body == 0)
			nn->n_sto = EXTERN;

		if (class_name && etbl!=gtbl) {	/* beware of implicit declaration */
			Pname cn, tt;

			cn = nn->u4.n_table->t_name;
			tt = new_name("this");
			tt->n_scope = ARG;
			tt->n_sto = ARG;
			tt->u1.tp = ((Pclass)class_name->u1.tp)->this_type;
			PERM(tt);
			((Pfct)nn->u1.tp)->f_this = f->f_this = tt;
			tt->n_list = f->argtype;
		}

		if (f->f_result == 0) {
			Pname rcln;

			rcln = is_cl_obj(f->returns);
			if (rcln && has_itor((Pclass)rcln->u1.tp)) make_res(f);
		}
		else if (f->f_this)
			f->f_this->n_list = f->f_result;

		if (f->f_virtual){
			switch (nn->n_scope){
			default:
				V1.u1.p = (char *)this;
				error("nonC virtual%n", &V1, ea0, ea0, ea0);
				break;
			case 0:
			case PUBLIC:
				cc->cot->virt_count = 1;
				((Pfct)nn->u1.tp)->f_virtual = 1;
				break;
			}
		}
	}

		/*	an operator must take at least one class object or
		 *	reference to class object argument
		 */
	switch (this->n_oper){
	case CTOR:
		if (f->nargs == 1) {	/* check for X(X) and X(X&) */
			Ptype t;

			t = f->argtype->u1.tp;
		clll:
			switch (t->base) {
			case TYPE:
				t = ((Pbase)t)->b_name->u1.tp;
				goto clll;
			case RPTR:			/* X(X&) ? */
				t = ((Pptr)t)->typ;
			cxll:
				switch (t->base) {
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto cxll;
				case COBJ:
					if (class_name == ((Pbase)t)->b_name)
						((Pclass)class_name->u1.tp)->itor=nn;
				}
				break;
			case COBJ:			/* X(X) ? */
				if (class_name == ((Pbase)t)->b_name) {
					V1.u1.p = (char *)class_name->u2.string;
					V2.u1.p = (char *)class_name->u2.string;
					error("impossibleK: %s(%s)", &V1,
						&V2, ea0, ea0);
				}
			}
		}
		break;
	case TYPE:
		if (just_made) {
			nn->n_list = ((Pclass)class_name->u1.tp)->conv;
			((Pclass)class_name->u1.tp)->conv = nn;
		}
		break;
	case DTOR:
	case NEW:
	case DELETE:
	case CALL:
	case 0:
		break;
	default:
		if (f->nargs_known != 1) {
			V1.u1.p = (char *)nn;
			error("ATs must be fully specified for%n", &V1,ea0,ea0,ea0);
		}
		else if (class_name == 0) {
			switch (f->nargs) {
			case 1:
			case 2:
				for(a=f->argtype; a; a=a->n_list) {
					Ptype tx;
					tx = a->u1.tp;
					if (tx->base == RPTR) tx = ((Pptr)tx)->typ;
					if (is_cl_obj(tx)) goto cok;
				}
				V1.u1.p = (char *)nn;
				error("%n must take at least oneCTA",
					&V1, ea0, ea0, ea0);
				break;
			default:
				V1.u1.p = (char *)nn;
				error("%n must take 1 or 2As", &V1, ea0, ea0, ea0);
			}
		}
		else {
			switch (f->nargs) {
			case 0:
			case 1:
				break;
			default:
				V1.u1.p = (char *)nn;
				error("%n must take 0 or 1As", &V1, ea0, ea0, ea0);
			}
		}
	cok:;
	}

	i = 0;		/* check that every argument after an argument with
			 * initializer have an initializer
			 */
	for(a = f->argtype; a; a = a->n_list) {
		if (a->u3.n_initializer)
			i = 1;
		else if (i)
		{
			V1.u1.p =(char *)a;
			error("trailingA%n withoutIr", &V1, ea0, ea0, ea0);
		}
	}

	/*
	 *	the body cannot be checked until the name
	 *	has been checked and entered into its table
	 */
	if (f->body) fct_dcl(f, nn);
	return nn;
}
