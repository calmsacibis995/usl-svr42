/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/simpl.c	1.1"
/*******************************************************************

simpl.c:

	simplify the typechecked function
	remove:		classes:
				class fct-calls
				operators
				value constructors and destructors
			new and delete operators(replace with function calls)
			initializers		(turn them into statements)
			constant expressions		(evaluate them)
			inline functions		(expand the calls)
			enums				(make const ints)
			unreachable code		(delete it)
	make implicit coersions explicit

	in general you cannot simplify something twice

*******************************************************************/

#include "cfront.h"
#include "size.h"
#include <ctype.h>

Pname new_fct;
Pname del_fct;
Pname vec_new_fct;
Pname vec_del_fct;
Pstmt del_list;
Pstmt break_del_list;
Pstmt continue_del_list;
bit not_inl;		/* is the current function an inline? */
Pname curr_fct;		/* current function */
Pexpr init_list;
Pexpr one;

static Pstmt new_pair();
static Pexpr new_texte();

/*
 *	generate a call to construct or destroy the elements of a vector
 */
Pexpr cdvec(f, vec, cl, cd, tail)
Pname f;
Pexpr vec;
Pclass cl;
Pname cd;
int tail;
{
	Pexpr sz;		/* sizeof elem */
	Pexpr noe;		/* sizeof(vec)/sizeof(elem) */
	Pexpr esz, arg;

	sz = new_texpr(SIZEOF, cl, 0);
	sz->u1.tp = (Ptype)uint_type;

	esz = new_texpr(SIZEOF, cl, 0);
	esz->u1.tp = (Ptype)int_type;

	noe = new_texpr(SIZEOF, vec->u1.tp, 0);
	noe->u1.tp = (Ptype)int_type;
	noe = new_expr(0, DIV, noe, esz);
	noe->u1.tp = (Ptype)uint_type;

	if (0 <= tail)
		arg = new_expr(0, ELIST, zero, 0);
	else
		arg = 0;

	/* constructor or destructor */
	arg = (Pexpr)new_expr(0, ELIST,(Pexpr)cd, arg);

	lval((Pexpr)cd, ADDROF);
	arg = (Pexpr) new_expr(0, ELIST, sz, arg);
	arg = (Pexpr) new_expr(0, ELIST, noe, arg);
	arg = (Pexpr) new_expr(0, ELIST, vec, arg);

	arg = new_call(f, arg);
	arg->base = G_CALL;
	arg->u4.fct_name = f;

	return arg;
}


/*
 *	strip off statements after RETURN etc.
 *	NOT general: used for stripping off spurious destructor calls
 */
Pstmt trim_tail(tt)
Pstmt tt;
{
	Pstmt tx;
	while (tt->s_list){
		switch (tt->base){
		case PAIR:
			tx = trim_tail(tt->u2.s2);
			goto txl;
		case BLOCK:
			tx = trim_tail(tt->s);
		txl:
			switch (tx->base){
			case SM:
				break;
			case CONTINUE:
			case BREAK:
			case GOTO:
			case RETURN:
				tt->s_list = 0;
			default:
				return tx;
			}
		default:
			tt = tt->s_list;
			break;
		case RETURN:
			tt->s_list = 0;
			return tt;
		}
	}

	switch (tt->base){
	case PAIR:
		return trim_tail(tt->u2.s2);

	case BLOCK:
		if (tt->s) return trim_tail(tt->s);
	default:
		return tt;
	}
}

void simpl_init()
{
	Pname nw, dl, vn, vd, a, al;

	nw = new_name(oper_name(NEW));
	dl = new_name(oper_name(DELETE));
	vn = new_name("_vec_new");
	vd = new_name("_vec_delete");

	new_fct = insert(gtbl, nw, 0);	/* void* operator new(long); */
	name__dtor(nw);
	a = new_name(0);
	a->u1.tp = (Ptype)long_type;
	new_fct->u1.tp = (Ptype)fct_ctor(Pvoid_type, a, 1);
	new_fct->n_scope = EXTERN;
	PERM(new_fct);
	PERM(new_fct->u1.tp);
	use(new_fct);

	del_fct = insert(gtbl, dl, 0);	/* void operator delete(void *); */
	name__dtor(dl);
	a = new_name(0);
	a->u1.tp = Pvoid_type;
	del_fct->u1.tp = (Ptype)fct_ctor((Ptype)void_type, a, 1);
	del_fct->n_scope = EXTERN;
	PERM(del_fct);
	PERM(del_fct->u1.tp);
	use(del_fct);

	a = new_name(0);
	a->u1.tp = Pvoid_type;
	al = a;
	a = new_name(0);
	a->u1.tp = (Ptype)int_type;
	a->n_list = al;
	al = a;
	a = new_name(0);
	a->u1.tp = (Ptype)int_type;
	a->n_list = al;
	al = a;
	a = new_name(0);
	a->u1.tp = Pvoid_type;
	a->n_list = al;
	al = a;		/* (Pvoid,int,int,Pvoid) */

	vec_new_fct = insert(gtbl, vn, 0);
	name__dtor(vn);
	vec_new_fct->u1.tp = (Ptype)fct_ctor(Pvoid_type, al, 1);
	vec_new_fct->n_scope = EXTERN;
	PERM(vec_new_fct);
	PERM(vec_new_fct->u1.tp);
	use(vec_new_fct);

	a = new_name(0);
	a->u1.tp = (Ptype)int_type;
	al = a;
	a = new_name(0);
	a->u1.tp = Pvoid_type;
	a->n_list = al;
	al = a;
	a = new_name(0);
	a->u1.tp = (Ptype)int_type;
	a->n_list = al;
	al = a;
	a = new_name(0);
	a->u1.tp = (Ptype)int_type;
	a->n_list = al;
	al = a;
	a = new_name(0);
	a->u1.tp = Pvoid_type;
	a->n_list = al;
	al = a;		/* (Pvoid,int,int,Pvoid,int) */

	vec_del_fct = insert(gtbl, vd, 0);
	name__dtor(vd);
	vec_del_fct->u1.tp = (Ptype)fct_ctor((Ptype)void_type, al, 1);
	vec_del_fct->n_scope = EXTERN;
	PERM(vec_del_fct);
	PERM(vec_del_fct->u1.tp);
	use(vec_del_fct);

	one = new_ival(1);
	one->u1.tp = (Ptype)int_type;
	PERM(one);

	/* assume void generates char */
	putstring("char *_new(); char _delete(); char *_vec_new(); char _vec_delete();\n");
}


Ptable scope;		/* current scope for simpl() */
Pname expand_fn;	/* name of function being expanded or 0 */
Ptable expand_tbl;	/* scope for inline function variables */

Pname has_oper(this, op)
Pclass this;
TOK op;
{
	char *s;
	Pname n;

	s = oper_name(op);
	if (this == 0){
		V1.u1.p = (char *)s;
		errorFIPC('i', "0->has_oper(%s)", &V1, ea0, ea0, ea0);
	}
	n = lookc(this->memtbl, s);
	if (n == 0) return 0;
	switch (n->n_scope){
	case 0:
	case PUBLIC:
		return n;
	default:
		return 0;
	}
}

/*
 *	is this statement simple enough to be converted into an expression for
 *	inline expansion?
 */
int is_expr(s)
Pstmt s;
{
	int i;
	Pstmt ss;

	i = 0;
	for(ss = (s->base == BLOCK ? s->s : s); ss ;ss = ss->s_list) {
		switch (ss->base){
		case BLOCK:
			if (((Pstmt)ss)->memtbl || is_expr(ss->s) == 0)
				return 0;
		case SM:
			if (ss->u2.e->base == ICALL){
				Pname fn;
				Pfct f;

				fn = ss->u2.e->u4.il->fct_name;
				f = (Pfct)fn->u1.tp;
				if (f->f_expr == 0) return 0;
			}
			break;
		case IF:
			if (is_expr(ss->s) == 0) return 0;
			if (ss->u3.else_stmt && is_expr(ss->u3.else_stmt) == 0)
				return 0;
			break;
		default:
			return 0;
		}
		i++;
	}
	return i;
}

int no_of_returns = 0;

void name_simpl(this)
Pname this;
{
	if (this->base == PUBLIC) return;

	if (this->u1.tp == 0){
		V1.u1.p = (char *)this;
		errorFIPC('i', "%n->N::simple(tp==0)", &V1, ea0, ea0, ea0);
	}
	switch (this->u1.tp->base){
	case 0:
		V1.u1.p = (char *)this;
		errorFIPC('i', "%n->N::simpl(tp->B==0)",&V1,ea0,ea0,ea0);

	case OVERLOAD:
	{
		Plist gl;

		gl = ((Pgen)this->u1.tp)->fct_list;
		for(;gl ;gl = gl->l) name_simpl(gl->f);
		break;
	}
	case FCT:
	{
		Pfct f;
		Pname n, th;

		f = (Pfct)this->u1.tp;
		th = f->f_this;

		if (th){
			/* Make "this" a register if it is used more than twice */
			if (2 < th->n_used)
				th->n_sto = REGISTER;
			else th->n_sto = 0;
			if (this->n_oper == CTOR) f->s_returns = th->u1.tp;
		}

		if (this->u1.tp->defined &(SIMPLIFIED | ~DEFINED)) return;

		for(n=th?th:(f->f_result?f->f_result:f->argtype); n; n=n->n_list)
			name_simpl(n);

		if (f->body){
			Ptable oscope;

			oscope = scope;
			scope = f->body->memtbl;
			if (scope == 0){
				V1.u1.p = (char *)this;
				errorFIPC('i', "%n memtbl missing",
					&V1, ea0, ea0, ea0);
			}
			curr_fct = this;
			fct_simpl(f);
			if (f->f_inline && debug == 0) {
				int i;
				Pexpr ee;

				if (MIA <= f->nargs){
					V1.u1.p = (char *)this;
					errorFIPC('w',
					"too manyAs for inline%n(inline ignored)",
						&V1, ea0, ea0, ea0);
					f->f_inline = 0;
					scope = oscope;
					break;
				}
				i = 0;
				if (th)			n = th;
				else if (f->f_result)	n = f->f_result;
				else			n = f->argtype;
				for( ;n ;n = n->n_list) {
					n->base = ANAME;
					n->n_val = i++;
					if (n->u4.n_table != scope){
						V1.u1.p = (char *)n->u2.string;
						V2.u1.p = (char *)n->u4.n_table;
						V3.u1.p = (char *)scope;
						errorFIPC('i',
							"aname scope: %s %d %d\n",
							 &V1, &V2, &V3, ea0);
					}
				}
				if (f->returns->base!=VOID || this->n_oper==CTOR)
					expand_tbl = scope;
				else
					expand_tbl = 0;
				expand_fn = this;
				if (expand_tbl){
				genlab:
					/* value returning: generate expr */
					ee = (Pexpr)stmt_expand((Pstmt)f->body);
					/* the body still holds the memtbl */
					if (ee->base == CM)
						f->f_expr = ee;
					else
						f->f_expr = new_expr(0,CM,zero,ee);
					/*
					 * print.c assumes expansion into
					 * comma expression
					 */
				}
				else {
					if (is_expr((Pstmt)f->body)) {
						/* can genertae expr: do */
						f->s_returns = (Ptype)int_type;
						expand_tbl = scope;
						goto genlab;
					}
					/* not value return; can generate block */

					f->f_expr = 0;
					f->body= (Pstmt)stmt_expand((Pstmt)f->body);
				}
				expand_fn = 0;
				expand_tbl = 0;
			}
			scope = oscope;
		}
		break;
	}

	case CLASS:
		classdef_simpl((Pclass)this->u1.tp);
		break;

	default:
		break;
	}

	if (this->u3.n_initializer) expr_simpl(this->u3.n_initializer);
	this->u1.tp->defined |= SIMPLIFIED;
}

/*
 *	call only for the function definition(body != 0)
 *
 *	simplify argument initializers, and base class initializer, if any
 *	then simplify the body, if any
 *
 *	for constructor:call allocator if this==0 and this not assigned to
 *			(auto and static objects call constructor with this!=0,
 *			the new operator generates calls with this==0)
 *			call base & member constructors
 *	for destructor:	call deallocator(no effect if this==0)
 *			case base & member destructors
 *
 *	for arguments and function return values look for class objects
 *	that must be passed by constructor X(X&).
 *
 *	Allocate temporaries for class object expressions, and see if
 *	class object return values can be passed as pointers.
 *
 *	call constructor and destructor for local class variables.
 */
void fct_simpl(this)
Pfct this;
{
	Pexpr th;
	Ptable tbl;
	Pstmt ss, tail, dtail;
	Pname cln;
	Pclass cl;
	int ass_count;

	th = (Pexpr)this->f_this;
	tbl = this->body->memtbl;
	ss = 0;
	dtail = 0;

	not_inl = debug || this->f_inline == 0;
	del_list = 0;
	continue_del_list = 0;
	break_del_list = 0;
	scope = tbl;
	if (scope == 0)errorFIPC('i', "F::simpl()", ea0, ea0, ea0, ea0);

	if (th){
		Pptr p;

		p = (Pptr)th->u1.tp;
		cln = ((Pbase)p->typ)->b_name;
		cl = (Pclass)cln->u1.tp;
	}

	if (curr_fct->n_oper == DTOR){		/* initialize del_list */
		Pexpr ee;
		Pstmt es;
		Pname bcln, d, fa, free_arg, m;
		Pclass bcl;
		Ptable tbl;
		int i;

		bcln = cl->clbase;
		fa = new_name("_free");		/* fake argument for dtor */
		fa->u1.tp = (Ptype)int_type;
		free_arg = name_dcl(fa, this->body->memtbl, ARG);
		name__dtor(fa);
		this->f_this->n_list = free_arg;
		tbl = cl->memtbl;

		/* generate calls to destructors for all members of class cl */
		for(m = get_mem(tbl, i=1) ;m ;m = get_mem(tbl,++i) ) {
			Ptype t;
			Pname cn, dtor;
			Pclass cl;

			t = m->u1.tp;
			if (m->n_stclass == STATIC)continue;

			if (cn = is_cl_obj(t)){
				cl = (Pclass)cn->u1.tp;
				if (dtor = has_dtor(cl)) {
					/*	dtor(this,0);	*/
					Pexpr aa;

					aa = new_expr(0, ELIST, zero, 0);
					ee = new_ref(REF, th, m);
					ee = new_ref(DOT, ee, dtor);
					ee = new_call(ee, aa);
					ee->u4.fct_name = dtor;
					ee->base = G_CALL;
					es = new_estmt(SM, curloc, ee, 0);
					if (dtail)
						dtail->s_list = es;
					else 
						del_list = es;
					dtail = es;
				}
			}
			else if (cl_obj_vec){
				cl = (Pclass)cl_obj_vec->u1.tp;
				if (dtor = has_dtor(cl)) {
					Pexpr mm, ee;

					mm = new_ref(REF, th, m);
					mm->u1.tp = m->u1.tp;
					ee = cdvec(vec_del_fct, mm, cl, dtor, 0);
					es = new_estmt(SM, curloc, ee, 0);
					if (dtail)
						dtail->s_list = es;
					else 
						del_list = es;
					dtail = es;
				}
			}
		}

		/* delete base */
		if (bcln && (bcl = (Pclass)bcln->u1.tp)
		&& (d = has_dtor(bcl)) ) {	/* base.dtor(this,_free); */
			Pexpr aa;

			aa = (Pexpr)new_expr(0, ELIST,(Pexpr)free_arg, 0);
			ee = new_ref(REF, th, d);
			ee = new_call(ee, aa);
			ee->base = G_CALL;
			es = new_estmt(SM, curloc, ee, 0);
		}
		else {				/* if (_free) _delete(this); */
			Pexpr aa;

			aa = (Pexpr)new_expr(0, ELIST, th, 0);
			ee = new_call(del_fct, aa);
			ee->u4.fct_name = del_fct;
			ee->base = G_CALL;
			es = new_estmt(SM, curloc, ee, 0);
			es = new_ifstmt(curloc, free_arg, es, 0);
		}
		use(free_arg);
		use((Pname)th );
		if (dtail)
			dtail->s_list = es;
		else 
			del_list = es;
		del_list = new_ifstmt(curloc, th, del_list, 0);
		if (del_list) stmt_simpl(del_list);
	}

	if (curr_fct->n_oper == CTOR){
		Pexpr ee;
		Ptable tbl;
		Pname m, nn;
		int i;

		tbl = cl->memtbl;

		/*
		 *	generate: this=base::base(args)
		 *	this->b_init == base::base(args) or 0
		 */
		if (this->b_init){
			switch (this->b_init->base){
			case ASSIGN:
			case CM:
				break;
			default:
				{
				Pexpr ccc;
				Pname bn;
				Pname tt;

				ccc = (Pexpr)this->b_init;
				bn = ccc->u4.fct_name;
				tt = ((Pfct)bn->u1.tp)->f_this;
				ass_count = tt->n_assigned_to;
				call_simpl(ccc);
				init_list=new_expr(0,ASSIGN,th,ccc);
				}
			}
		}
		else {
			ass_count = 0;
			init_list = 0;
		}

		if (cl->virt_count){	/* generate: this->_vptr=virt_init; */
			Pname vp;
			Pexpr vtbl;
			vp = look(cl->memtbl, "_vptr", 0);
			vtbl = new_texte(cl->string, "_vtbl");
			ee = new_ref(REF, th, vp);
			ee = new_expr(0, ASSIGN, ee, vtbl);
			if (init_list)
				init_list = new_expr(0, CM, init_list, ee);
			else
				init_list = ee;
		}
		for(nn = this->f_init ;nn ;nn = nn->n_list) {
			if (nn->u3.n_initializer == 0)continue;
			{
				Pname m;

				m = look(tbl, nn->u2.string, 0);
				if (m && m->u4.n_table == tbl)
					m->u3.n_initializer = nn->u3.n_initializer;
			}
		}
		/* generate cl::cl(args) for all members of cl */
		for(m = get_mem(tbl, i=1) ;m ;m = get_mem(tbl,++i)) {
			Ptype t;
			Pname cn, ctor;
			Pclass cl;

			t = m->u1.tp;
			switch (m->n_stclass){
			case STATIC:
			case ENUM:
				continue;
			}
			switch (t->base){
			case FCT:
			case OVERLOAD:
			case CLASS:
			case ENUM:
				continue;
			}
			if (m->base == PUBLIC)continue;

			if (cn = is_cl_obj(t)){
				Pexpr ee;

				ee = m->u3.n_initializer;
				m->u3.n_initializer = 0;
				/* from  fct must not persist until next fct */

				if (ee == 0){	/* try default */
					cl = (Pclass)cn->u1.tp;
					if (ctor = has_ictor(cl)){
						ee = new_ref(REF, th, m);
						ee = new_ref(DOT, ee, ctor);
						ee = new_call(ee, 0);
						ee->u4.fct_name = ctor;
						ee->base = G_CALL;
						ee = expr_typ(ee, tbl);
						/* look for default arguments */
					}
					else if (has_ctor(cl)) {
						V1.u1.p = (char *)m;
						V2.u1.p = (char *)cl->string;
						error("M%n needsIr(no defaultK forC %s)",
							&V1, &V2, ea0, ea0);
					}
				}

				if (ee){
					expr_simpl(ee);
					if (init_list)
						init_list=new_expr(0,CM,init_list,ee);
					else 
						init_list = ee;
				}
			}
			else if (cl_obj_vec){
				cl = (Pclass)cl_obj_vec->u1.tp;
				if (ctor = has_ictor(cl)){
					/* _new_vec(vec,noe,sz,ctor); */
					Pexpr mm, ee;

					mm = new_ref(REF, th, m);
					mm->u1.tp = m->u1.tp;
					ee = cdvec(vec_new_fct, mm, cl, ctor, -1);
					expr_simpl(ee);
					if (init_list)
						init_list=new_expr(0,CM,init_list,ee);
					else 
						init_list = ee;
				}
				else if (has_ctor(cl)) {
					V1.u1.p = (char *)m;
					V2.u1.p = (char *)cl->string;
					error("M%n[] needsIr(no defaultK forC %s)",
		    				&V1, &V2, ea0, ea0);
				}
			}
			else if (m->u3.n_initializer){
				/* list of non-class mem set in mem_init() */
				if (init_list)
					init_list=new_expr(0,CM,init_list,m->u3.n_initializer);
				else 
					init_list = m->u3.n_initializer;
				/* from fct must not persist until next fct */
				m->u3.n_initializer = 0;
			}
			else if (is_ref(t)){
				V1.u1.p = (char *)m;
				error("RM%n needsIr", &V1, ea0, ea0, ea0);
			}
			else if (tconst(t) && vec_const == 0) {
				V1.u1.p = (char *)m;
				error("constM%n needsIr", &V1, ea0, ea0, ea0);
			}
		} /* for(m = get_mem(tbl, i=1) ... */
	}

	no_of_returns = 0;
	tail = block_simpl(this->body);

	/* return must have been seen */
	if (this->returns->base != VOID || this->f_result) {
		/* could be OK */
		if (no_of_returns) {
			Pstmt tt;

			if (tail->base == RETURN || tail->base == LABEL)
				tt = tail;
			else
				tt = trim_tail(tail);

			switch (tt->base){
			case RETURN:
			case GOTO:
				del_list = 0;	/* no need for del_list */
				break;
			case SM:
				switch (tt->u2.e->base){
				case ICALL:
				case G_CALL:
					goto chicken;
				}
			default:
				if (strcmp(curr_fct->u2.string, "main")) {
					V1.u1.p = (char *)curr_fct;
					errorFIPC('w',
						"maybe no value returned from%n",
						&V1, ea0, ea0, ea0);
				}
			case IF:
			case SWITCH:
			case DO:
			case WHILE:
			case FOR:
			case LABEL:
			chicken:		/* don't dare write a warning */
				break;
			}
		}
		else {				/* must be an error */
			if (strcmp(curr_fct->u2.string, "main")) {
				V1.u1.p = (char *)curr_fct;
				errorFIPC('w', "no value returned from%n",
				&V1, ea0, ea0, ea0);
			}
		}
		if (del_list)goto zaq;
	}
	else if (del_list){	/* return may not have been seen */
	zaq:
		if (tail)
			tail->s_list = del_list;
		else 
			this->body->s = del_list;
		tail = dtail;
	}

	if (curr_fct->n_oper == DTOR){
		/* body => if (this == 0) body */
		this->body->s = new_ifstmt(this->body->where, th, this->body->s, 0);
	}

	if (curr_fct->n_oper == CTOR) {
		Pstmt st;

		/* generate:	if (this==0) this=_new( sizeof(class cl) );
		 *		init_list ;
		 */
		if (((Pname)th)->n_assigned_to == 0) {
			Pexpr sz, ee;
			Pstmt es, ifs;

			((Pname)th)->n_assigned_to = ass_count ? ass_count: FUDGE111;

			sz = new_texpr(SIZEOF, cl, 0);
			sz->u1.tp = (Ptype)uint_type;
			ee = new_expr(0, ELIST, sz, 0);
			ee = new_call(new_fct, ee);
			ee->u4.fct_name = new_fct;
			ee->base = G_CALL;
			expr_simpl(ee);
			ee = new_expr(0, ASSIGN, th, ee);
			es = new_estmt(SM, this->body->where, ee, 0);
			ee = new_expr(0, EQ, th, zero);
			ifs = new_ifstmt(this->body->where, ee, es, 0);
			/*ifs->simpl();
			 *	do not simplify
			 *	or "this = " will cause an extra call of base::base
			 */
			if (init_list){
				es = new_estmt(SM, this->body->where, init_list, 0);
				es->s_list = this->body->s;
				this->body->s = es;
				if (tail == 0)tail = es;
			}
			ifs->s_list = this->body->s;
			this->body->s = ifs;
			if (tail == 0) tail = ifs;
		}

		st = new_estmt(RETURN, curloc, th, 0);
		if (tail)
			tail->s_list = st;
		else 
			this->body->s = st;
		tail = st;
	}
}


Pstmt block_simpl(this)
Pstmt this;
{
	int i;
	Pname n, cln;
	Pstmt ss, sst, dd, ddt, stail, st;
	Ptable old_scope;
	Pexpr in;
	char *s;
	register char c3;

	ss = sst = dd = ddt = 0;
	old_scope = scope;

	if (this->u2.own_tbl == 0){
		ss = this->s ? stmt_simpl(this->s): 0;
		return ss;
	}

	scope = this->memtbl;
	if (scope->init_stat == 0)
		scope->init_stat = 1;	/* table is simplified */

	for(n = get_mem(scope, i=1) ;n ;n = get_mem(scope,++i)) {
		st = 0;
		in = n->u3.n_initializer;

		if (in) scope->init_stat = 2; /* initializer in this scope */

		switch (n->n_scope){
		case ARG:
		case 0:
		case PUBLIC:
			continue;
		}

		if (n->n_stclass == STATIC)continue;

		if (in && in->base == ILIST)
			errorFIPC('s', "initialization of automatic aggregates",
				ea0, ea0, ea0, ea0);
		if (n->u1.tp == 0)continue;	/* label */

		if (n->n_evaluated)continue;

		/* construction and destruction of temporaries is handled locally */
		s = n->u2.string;
		c3 = s[3];
		if (s[0] == '_' && s[1] == 'D' && isdigit(c3)) continue;
		if (cln = is_cl_obj(n->u1.tp)){
			Pclass cl;
			Pname d;

			cl = (Pclass)cln->u1.tp;
			d = has_dtor(cl);

			if (d){			/* n->cl.dtor(0); */
				Pexpr r;
				Pexpr ee;
				Pexpr dl;
				Pstmt dls;

				r = new_ref(DOT, n, d);
				ee = new_expr(0, ELIST, zero, 0);
				dl = new_call(r, ee);
				dls = new_estmt(SM, n->where, dl, 0);
				dl->base = G_CALL;
				dl->u4.fct_name = d;

				if (dd){
					dls->s_list = dd;
					dd = dls;
				}
				else 
					ddt = dd = dls;
			}

			if (in){
				switch (in->base){
				case DEREF:	/* constructor? */
					if (in->u2.e1->base == G_CALL){
						Pname fn;

						fn = in->u2.e1->u4.fct_name;
						if (fn == 0 || fn->n_oper != CTOR)
							goto ddd;
						st= new_estmt(SM,n->where,in->u2.e1,0);
						n->u3.n_initializer = 0;
						break;
					}
					goto ddd;
				case G_CM:
					st = new_estmt(SM, n->where, in->u2.e1, 0);
					n->u3.n_initializer = 0;
					break;
				case ASSIGN:	/* assignment to "n"? */
					if (in->u2.e1 == (Pexpr)n){
						st = new_estmt(SM,n->where,in,0);
						n->u3.n_initializer = 0;
						break;
					}
				default:
					goto ddd;
				}
			}
		}
		else if (cl_obj_vec){
			Pclass cl;
			Pname d, c;

			cl = (Pclass)cl_obj_vec->u1.tp;
			d = has_dtor(cl);
			c = has_ictor(cl);

			n->u3.n_initializer = 0;

			if (c){		/* _vec_new(vec,noe,sz,ctor); */
				Pexpr a;

				a = cdvec(vec_new_fct,(Pexpr)n, cl, c, -1);
				st = new_estmt(SM, n->where, a, 0);
			}

			if (d){		/* _vec_delete(vec,noe,sz,dtor,0); */
				Pexpr a;
				Pstmt dls;

				a = cdvec(vec_del_fct,(Pexpr)n, cl, d, 0);
				dls = new_estmt(SM, n->where, a, 0);
				if (dd){
					dls->s_list = dd;
					dd = dls;
				}
				else 
					ddt = dd = dls;
			}
		}
		else if (in){
			switch (in->base){
			case ILIST:
				switch (n->n_scope){
				case FCT:
				case ARG:
					V1.u1.p = (char *)n;
					errorFIPC('s', "Ir list for localV%n",
						&V1, ea0, ea0, ea0);
				}
				break;
			case STRING:
				if (n->u1.tp->base == VEC)
					break;	/* char vec only */
			default:
			ddd:
			{	Pexpr ee;
				ee = new_expr(0, ASSIGN,(Pexpr)n, in);
				st = new_estmt(SM, n->where, ee, 0);
				n->u3.n_initializer = 0;
			}
			}
		}

		if (st){
			if (ss)
				sst->s_list = st;
			else 
				ss = st;
			sst = st;
		}
	}

	if (dd){
		Pstmt od, obd, ocd;
		Pfct f;

		od = del_list;
		obd = break_del_list;
		ocd = continue_del_list;

		stmt_simpl(dd);
		del_list = (od) ? new_pair(curloc, dd, od) : dd;
		break_del_list = (break_del_list&&obd)? new_pair(curloc,dd,obd): dd;
		if (continue_del_list && ocd)
			continue_del_list = new_pair(curloc,dd,ocd);
		else
			continue_del_list = dd;
		stail = (this->s) ? stmt_simpl(this->s) : 0;

		f = (Pfct)curr_fct->u1.tp;
		if (this != f->body || f->returns->base == VOID
		||(f->returns->base != VOID && no_of_returns == 0)
		|| strcmp(curr_fct->u2.string, "main") == 0) {
			/* not dropping through the bottom of a value
			 * returning function
			 */
			if (stail){
				Pstmt tt;

				if (stail->base == RETURN || stail->base == LABEL)
					tt = stail;
				else
					tt = trim_tail(stail);
				if (tt->base != RETURN) stail->s_list = dd;
			}
			else 
				this->s = dd;
			stail = ddt;
		}

		del_list = od;
		continue_del_list = ocd;
		break_del_list = obd;
	}
	else
		stail = (this->s) ? stmt_simpl(this->s) : 0;

	if (ss){	/* place constructor calls */
		stmt_simpl(ss);
		sst->s_list = this->s;
		this->s = ss;
		if (stail == 0)stail = sst;
	}

	scope = old_scope;

	return stail;
}


void classdef_simpl(this)
Pclass this;
{
	int i;
	Pname m, p;
	Pclass oc;
	Plist fl;
	Pexpr ii;

	oc = this->in_class;
	this->in_class = this;
	for(m = get_mem(this->memtbl, i=1) ;m ;m = get_mem(this->memtbl,++i)) {
		ii = m->u3.n_initializer;
		m->u3.n_initializer = 0;
		name_simpl(m);
		m->u3.n_initializer = ii;
	}
	this->in_class = oc;

	for(fl = this->friend_list ;fl ;fl = fl->l) {	/* simplify friends */
		p = fl->f;
		switch (p->u1.tp->base){
		case FCT:
		case OVERLOAD:
			name_simpl(p);
		}
	}
}

Ptype nstd_type;
void expr_simpl(this)
Pexpr this;
{
	Pexpr r;
	Pname m;

	if (this == 0 || this->permanent == 2) return;

	switch (this->base){
	case ICALL:
		return;
	case G_ADDROF:
	case ADDROF:
		expr_simpl(this->u3.e2);
		switch (this->u3.e2->base){
		case DOT:
		case REF:
			r = (Pexpr)this->u3.e2;
			m = r->u4.mem;
			if (m->n_stclass == STATIC){	/* & static member */
				Pexpr x;
			delp:
				x = this->u3.e2;
				this->u3.e2 = (Pexpr)m;
				r->u4.mem = 0;
				if (TO_DEL(x)) expr_del(x);
			}
			else if(m->u1.tp->base == FCT){	/* & member fct */
				Pfct f;

				f = (Pfct)m->u1.tp;
				if (f->f_virtual){	/* &p->f ==> p->vtbl[fi] */
					int index;
					Pexpr ie;
					Pname vp;

					index = f->f_virtual;
					ie = (1<index) ? new_ival(index-1) : 0;
					if (ie) ie->u1.tp = (Ptype)int_type;
					vp = look(m->u4.n_table, "_vptr", 0);
					r->u4.mem = vp;
					this->base = DEREF;
					this->u2.e1 = this->u3.e2;
					this->u3.e2 = ie;
				}
				else {
					goto delp;
				}
			}
		}
		break;

	default:
		if (this->u2.e1) expr_simpl(this->u2.e1);
		if (this->u3.e2) expr_simpl(this->u3.e2);
		break;

	case NAME:
	case DUMMY:
	case ICON:
	case FCON:
	case CCON:
	case IVAL:
	case FVAL:
	case LVAL:
	case STRING:
	case ZERO:
	case ILIST:
	case SIZEOF:
		return;

	case G_CALL:
	case CALL:
		call_simpl((Pexpr)this);
		break;

	case NEW:
		simpl_new(this);
		return;

	case DELETE:
		simpl_delete(this);
		break;

	case QUEST:
		expr_simpl(this->u4.cond);
		expr_simpl(this->u3.e2);
		/* FALLTHRU */
	case CAST:
	case REF:
		expr_simpl(this->u2.e1);
		break;

	case DOT:
		expr_simpl(this->u2.e1);
		switch (this->u2.e1->base){
		case CM:
		case G_CM:
			{	/* &(,name). => (...,&name)-> */
			Pexpr ex;

			ex = this->u2.e1;
			cfr:
			switch (ex->u3.e2->base){
			case NAME:
				this->base = REF;
				ex->u3.e2 = address(ex->u3.e2);
				break;
			case CM:
			case G_CM:
				ex = ex->u3.e2;
				goto cfr;
			}
			}
		}
		break;

	case ASSIGN:
		{
		Pfct f;
		Pexpr th;

		f = (Pfct)curr_fct->u1.tp;
		th = (Pexpr)f->f_this;
		if (this->u2.e1) expr_simpl(this->u2.e1);
		if (this->u3.e2 && this->u3.e2->base == ASSIGN)
			nstd_type = this->u3.e2->u1.tp;
		if (this->u3.e2) expr_simpl(this->u3.e2);

		if (th && th==this->u2.e1 && curr_fct->n_oper==CTOR && init_list) {
			/* this=e2 => (this=e2,init_list) */
			this->base = CM;
			this->u2.e1 = new_expr(0,ASSIGN, this->u2.e1, this->u3.e2);
			this->u3.e2 = init_list;
			if (nstd_type && nstd_type == th->u1.tp) {
				Pexpr ee;

				ee = new_expr(0, CM, this->u2.e1, this->u3.e2);
				this->u2.e1 = ee;
				this->u3.e2 = th;
			}
		}
		if (nstd_type) nstd_type = 0;
		break;
		}
	}

	if (this->u1.tp == (Ptype)int_type){
		int i;

		Neval = 0;
		i = eval(this);
		if (Neval == 0){
			this->base = IVAL;
			this->u2.i1 = i;
		}
	}
}


/*
 *	fix member function calls:
 *		p->f(x) becomes f(p,x)
 *		o.f(x)  becomes f(&o,x)
 *	or if f is virtual:
 *		p->f(x) becomes( *p->_vptr[ type_of(p).index(f)-1 ] )(p,x)
 *	replace calls to inline functions by the expanded code
 */
void call_simpl(this)
Pexpr this;
{
	Pname fn;
	Pfct f;
	Pexpr ee, p, q;
	Pclass cl;

	fn = this->u4.fct_name;
	f = (fn) ?(Pfct)fn->u1.tp : 0;

	if (fn == 0) expr_simpl(this->u2.e1);

	if (f){
		Pgen g;

		switch (f->base){
		case ANY:
			return;
		case FCT:
			break;
		case OVERLOAD:
			g = (Pgen)f;
			this->u4.fct_name = fn = g->fct_list->f;
			f = (Pfct)fn->u1.tp;
		}
	}

	switch (this->u2.e1->base){
	case MEMPTR:		/* (p->*p)(args) */
		p = this->u2.e1->u2.e1;
		q = this->u2.e1->u3.e2;
		cl = (Pclass)this->u2.e1->u4.tp2;
		f = (Pfct) type_deref(q->u1.tp);

		if (cl->virt_count == 0){	/* no virtuals: simple */
			/* (p->*p)(args) => (*p)(p,args) */
			this->u3.e2 = new_expr(0, ELIST, p, this->u3.e2);
			this->u2.e1 = new_expr(0, DEREF, q, 0);
			this->u2.e1->u1.tp = type_deref(q->u1.tp);
		}
		else {				/* may be virtual: */
			Pexpr c, pp, ie, vc;
			Pname vp;

			if (f->f_this == 0){
				Pname tt;

				if (f->memof == 0)
					errorFIPC('i', "memof missing",
						ea0, ea0, ea0, ea0);
				tt = new_name("this");
				tt->n_scope = ARG;
				tt->n_sto = ARG;
				tt->u1.tp = f->memof->this_type;
				PERM(tt);
				f->f_this = tt;
				tt->n_list = (f->f_result)? f->f_result: f->argtype;
			}

			/* (p ->* q)(args) =>
			 * ((int)q&~127)? (*q)(p,args): (p->vtbl[(int)*q-1])(p,args)
			 * (((int)q&~127)? *q: p->vtbl[(int)*q-1]) (p,args)
			 */

			/* first beware of side effects: */
			if (not_simple_exp(q))
				errorFIPC('s',
					"second operand of .* too complicated",
			 		ea0, ea0, ea0, ea0);
			if (not_simple_exp(p))
				errorFIPC('s',
					"first operand of .* too complicated",
					ea0, ea0, ea0, ea0);

			/* (int)q&~127 : */
			c = new_ival(127);
			pp = new_texpr(CAST, SZ_INT==SZ_WPTR?int_type:long_type, q);
			c = new_expr(0, COMPL, 0, c);
			c = new_expr(0, AND, pp, c);

			ie = new_texpr(CAST, int_type, q);
			ie = new_expr(0, MINUS, ie, one);
			vp = look(cl->memtbl, "_vptr", 0);
			vc = new_expr(0, DEREF, new_ref(REF,p,vp), ie);

			if (f->returns->base == VOID)
				vc = new_texpr(CAST, Pfctchar_type, vc);

			this->base = G_CALL;
			this->u2.e1 = new_expr(0, QUEST, q, vc);
			this->u2.e1->u4.cond = c;
			this->u2.e1 = new_expr(0, DEREF, this->u2.e1, 0);

			/* encode argtype */
			this->u2.e1->u4.tp2 = (Ptype)f->f_this;
			this->u3.e2 = new_expr(0, ELIST, p, this->u3.e2);
		}
		break;
	case DOT:
	case REF:
	{
		Pexpr r;
		Pexpr a1;

		r = (Pexpr)this->u2.e1;
		a1 = r->u2.e1;
		if (f && f->f_virtual){
			Pexpr a11, ie, vptr, ee;
			Pname vp;
			Ptype pft;
			int index;

			a11 = 0;
			switch (a1->base){ /* see if temporary might be needed */
			case NAME:
				a11 = a1;
				break;

			case REF:
			case DOT:
				if (a1->u2.e1->base == NAME)a11 = a1;
				break;
			case ADDROF:
			case G_ADDROF:
				if (a1->u3.e2->base == NAME)a11 = a1;
				break;
			}

			if (this->u2.e1->base == DOT){
				if (a11)a11 = address(a11);
				a1 = address(a1);
			}

			if (a11 == 0){
				/*
				 * temporary may be needed
				 * e->f() =>(t=e,t->f(t))
				 */
				Pname nx, n, cln;
				Pexpr ccc;

				nx = new_name(make_name('K'));
				nx->u1.tp = a1->u1.tp;
				n = name_dcl(nx, scope, ARG);
				name__dtor(nx);
				cln = is_cl_obj(a1->u1.tp);
				if (cln && has_itor((Pclass)cln->u1.tp))
					n->n_xref = 1;
				n->n_scope = FCT;
				assign(n);
				a11 = (Pexpr)n;
				a1 = new_expr(0, ASSIGN, n, a1);
				a1->u1.tp = n->u1.tp;
				expr_simpl(a1);
				ccc = new_call(0, 0);
				*ccc = *this;
				this->base = CM;
				this->u2.e1 = a1;
				this->u3.e2 = ccc;
				this = ccc;
			}
			this->u3.e2 = new_expr(0, ELIST, a11, this->u3.e2);
			index = f->f_virtual;

			ie = (1 < index)? new_ival(index-1) : 0;
			vp = look(fn->u4.n_table, "_vptr", 0);
			vptr = new_ref(REF, a11, vp);		/* p->vptr */
			ee = new_expr(0, DEREF, vptr, ie);	/* p->vptr[i] */
			pft = (Ptype) new_ptr(PTR, f, 0);
			ee = new_texpr(CAST, pft, ee);		/*(T)p->vptr[i] */
			ee->u1.tp = (Ptype)f->f_this;		/* encode argtype */
			this->u2.e1 = new_expr(0,DEREF,ee,0);	/* *(T)p->vptr[i] */
			/* e1->tp must be 0, means "argtype encoded" */
			this->u4.fct_name = 0;
			fn = 0;
			expr_simpl(this->u3.e2);
			return;			/*(*(T)p->vptr[i])(e2) */
		}
		else {
			Ptype tt;

			tt = r->u4.mem->u1.tp;
		llp:
			switch (tt->base){
			case TYPE:
				tt = ((Pbase)tt)->b_name->u1.tp;
				goto llp;
			case FCT:
			case OVERLOAD:	/* n->fctmem(args) */
				if (a1->base != QUEST){
					if (this->u2.e1->base == DOT)
						a1 = address(a1);
					this->u3.e2 =
						new_expr(0,ELIST,a1,this->u3.e2);
					this->u2.e1 = (Pexpr)r->u4.mem;
				}
				else {
					Pexpr t0, t1, tt0, tt1;

					t0 = new_expr(0, this->base, 0, 0);
					t1 = new_expr(0, this->u2.e1->base, 0, 0);
					*t0 = *((Pexpr)this);
					*t1 = *this->u2.e1;
					t0->u2.e1 = t1;
					t1->u2.e1 = a1->u2.e1;
					a1->u2.e1 = t0;
					tt0 = new_expr(0, this->base, 0, 0);
					tt1 = new_expr(0, this->u2.e1->base,0,0);
					*tt0 = *((Pexpr)this);
					*tt1 = *this->u2.e1;
					tt0->u2.e1 = tt1;
					tt1->u2.e1 = a1->u3.e2;
					a1->u3.e2 = tt0;
					*((Pexpr)this) = *a1;
					expr_simpl((Pexpr)this);
					return;
				}
			}
		}
	}
	}
	expr_simpl(this->u3.e2);

	if (this->u2.e1->base == NAME && this->u2.e1->u1.tp->base == FCT) {
		/* reconstitute fn destroyed to suppress "virtual" */
		this->u4.fct_name = fn = (Pname)this->u2.e1;
		f = (Pfct)fn->u1.tp;
	}

	if (fn && f->f_inline && debug == 0) {
		ee = fct_expand(f, fn, scope, this->u3.e2);
		if (ee)
			*((Pexpr)this) = *ee;
		else if (this->u1.tp->base == TYPE
			&&((Pbase)this->u1.tp)->b_name
			&&((Pbase)this->u1.tp)->b_name->u1.tp->base == COBJ) {
			Pexpr ee1, ee2;
			Pname tmp;

			ee1 = new_expr(0, this->base, 0, 0);
			*ee1 = *((Pexpr)this);
			tmp = make_tmp('T', this->u1.tp, scope);
			ee = new_expr(0, ASSIGN, tmp, ee1);
			ee2 = new_expr(0, G_ADDROF, 0, tmp);
			ee = new_expr(0, CM, ee, ee2);
			*((Pexpr)this) = *ee;
		}
	}
}

/*
 *	 Is there a conditional in this expression?(not perfect)
 */
void ccheck(e)
Pexpr e;
{
	if (!e) return;

	switch (e->base){
	case QUEST:
	case ANDAND:
	case OROR:
		V1.u1.i = (int)e->base;
		errorFIPC('s',
		"E too complicated: uses%k and needs temorary ofCW destructor",
			&V1, ea0, ea0, ea0);
		break;
	case LT:
	case LE:
	case GT:
	case GE:
	case EQ:
	case NE:
	case ASSIGN:
	case ASPLUS:
	case ASMINUS:
	case G_CM:
	case CM:
	case PLUS:
	case MINUS:
	case MUL:
	case DIV:
	case OR:
	case ER:
	case AND:
	case G_CALL:
	case CALL:
	case ELIST:
		ccheck(e->u2.e1);
	case NOT:
	case COMPL:
	case CAST:
	case ADDROF:
	case G_ADDROF:
		ccheck(e->u3.e2);
	}
}

/*
 *	insert destructor calls 'ss' into condition 'ee'
 */
void temp_in_cond(ee, ss, tbl)
Pexpr ee;
Pstmt ss;
Ptable tbl;
{
	Ptype ct;
	Pname n, tmp;
	Pexpr v, c, ex;
	Pstmt sx;

	ccheck(ee);
	while (ee->base == CM || ee->base == G_CM) ee = ee->u3.e2;
	ct = ee->u1.tp;
	n = new_name(make_name('Q'));
	n->u1.tp = ct;
	tmp = name_dcl(n, tbl, ARG);
	name__dtor(n);
	tmp->n_scope = FCT;
	v = new_expr(0, 0, 0, 0);
	*v = *ee;
	v = new_texpr(CAST, ct, v);
	v->u1.tp = ct;
	c = new_expr(0, ASSIGN,(Pexpr)tmp, v);
	c->u1.tp = ct;
	ee->base = CM;
	ee->u2.e1 = c;
	ex = 0;
	for(sx = ss ;sx ;sx = sx->s_list) {
		ex = (ex) ? new_expr(0, CM, ex, sx->u2.e) : sx->u2.e;
		ex->u1.tp = sx->u2.e->u1.tp;
	}
	ee->u3.e2 = new_expr(0, CM, ss->u2.e, tmp);
	ee->u3.e2->u1.tp = ct;
}

bit not_safe(e)
Pexpr e;
{
	Pname n;
	int i;


	switch (e->base){
	default:
		return 1;

	case NAME:
		/*
		 * if the name is automatic and has a destructor it is not safe
		 * to destroy it before returning an expression depending on it
		 */
		n = (Pname)e;
		if (n->u4.n_table != gtbl && n->u4.n_table->t_name == 0) {
			Pname cn;

			cn = is_cl_obj(n->u1.tp);
			if (cn && has_dtor((Pclass)cn->u1.tp))
				return 1;
		}
	case IVAL:
	case ICON:
	case CCON:
	case FCON:
	case STRING:
		return 0;
	case NOT:
	case COMPL:
	case ADDROF:
	case G_ADDROF:
		return not_safe(e->u3.e2);
	case DEREF:
		i = not_safe(e->u2.e1);
		if (i) return i;
		if (e->u3.e2) return not_safe(e->u3.e2);
		return 0;
	case CM:
	case PLUS:
	case MINUS:
	case MUL:
	case DIV:
	case MOD:
	case ASSIGN:
	case ASPLUS:
	case ASMINUS:
	case ASMUL:
	case ASDIV:
	case OR:
	case AND:
	case OROR:
	case ANDAND:
	case LT:
	case LE:
	case GT:
	case GE:
	case EQ:
	case NE:
		return (not_safe(e->u2.e1) || not_safe(e->u3.e2));
	case QUEST:
		return not_safe(e->u4.cond)||not_safe(e->u2.e1)||not_safe(e->u3.e2);
	}
}


Pexpr curr_expr;
/*
 *	to protect against an inline being expanded twice
 *	in a simple expression keep track of expressions
 *	being simplified
 */

/*
 *	return a pointer to the last statement in the list, or 0
 */
Pstmt stmt_simpl(this)
Pstmt this;
{
	if (this == 0)
		errorFIPC('i', "0->S::simpl()", ea0, ea0, ea0, ea0);

	curr_expr = this->u2.e;

	switch (this->base){
	default:
		V1.u1.i = (int)this->base;
		errorFIPC('i', "S::simpl(%k)", &V1, ea0, ea0, ea0);

	case ASM:
		break;

	case BREAK:
		if (break_del_list){	/* break => { _dtor()s; break; } */
			Pstmt bs, dl;

			bs = new_stmt(0, this->base, this->where, 0);
			dl = copy(break_del_list);
			this->base = BLOCK;
			this->s = new_pair(this->where, dl, bs);
		}
		break;

	case CONTINUE:
		if (continue_del_list){	/* continue => { _dtor()s; continue; } */
			Pstmt bs, dl;

			bs = new_stmt(0, this->base, this->where, 0);
			dl = copy(continue_del_list);
			this->base = BLOCK;
			this->s = new_pair(this->where, dl, bs);
		}
		break;

	case DEFAULT:
		stmt_simpl(this->s);
		break;

	case SM:
		if (this->u2.e){
			if (this->u2.e->base == DEREF)
				this->u2.e = this->u2.e->u2.e1;
			expr_simpl(this->u2.e);
			if (this->u2.e->base == DEREF)
				this->u2.e = this->u2.e->u2.e1;
		}
		break;

	case RETURN:
		{
		/*	return x;
		 *	=>
		 *		{ dtor()s; return x; }
		 *	OR(returning an X where X(X&) is defined) =>
		 *		{ ctor(_result,x); _dtor()s; return; }
		 *	OR(where x needs temporaries)
		 *	OR(where x might involve an object to be destroyed) =>
		 *		{ _result = x; _dtor()s;  return _result; }
		 *	return;		=>
		 *		{ _dtor()s; return; }
		 *	OR(in constructors) =>
		 *		{ _dtor()s; return _this; }
		 */
		Pstmt dl;
		Pfct f;

		no_of_returns++;
		dl = (del_list) ? copy(del_list) : 0;
		f = (Pfct)curr_fct->u1.tp;

		if (this->u2.e == 0) this->u2.e = dummy;
		if (this->u2.e == dummy && curr_fct->n_oper == CTOR)
			this->u2.e = (Pexpr)f->f_this;

		if (f->f_result){	/* ctor(_result,x); dtors; return; */
			Pstmt cs;

			if (this->u2.e->base == G_CM)
				this->u2.e = replace_temp(this->u2.e,(Pexpr)f->f_result);
			expr_simpl(this->u2.e);
			cs = new_estmt(SM, this->where, this->u2.e, 0);
			if (dl) cs = new_pair(this->where, cs, dl);
			this->base = PAIR;
			this->s = cs;
			this->u2.s2 = new_estmt(RETURN, this->where, 0, 0);
#ifdef RETBUG
			this->u2.s2->u3.empty = 1;
			/* fudge to bypass C bug(see print.c) */
			this->u2.s2->u1.ret_tp = this->u1.ret_tp;
#endif
		}
		else {			/* dtors; return e; */
			expr_simpl(this->u2.e);
			if (dl){
				if (this->u2.e != dummy && not_safe(this->u2.e)){
					/* {_result=x;_dtor()s;return _result;} */
					Ptable ftbl;
					Pname r;
					Pexpr as;
					Pstmt cs;

					ftbl = ((Pfct)curr_fct->u1.tp)->body->memtbl;
					r = look(ftbl, "_result", 0);
					if (r == 0){
						Pname rn;

						r = new_name("_result");
						r->u1.tp = this->u1.ret_tp;
						rn = name_dcl(r, ftbl, ARG);
						rn->n_scope = FCT;
						assign(rn);
						name__dtor(r);
						r = rn;
					}
					as= new_expr(0,ASSIGN,(Pexpr)r,this->u2.e);
					as->u1.tp = this->u1.ret_tp;
					/*
					 * wrong if = overloaded, but then
					 * X(X&) ought to have been used
					 */
					cs = new_estmt(SM, this->where, as, 0);
					cs = new_pair(this->where, cs, dl);
					this->base = PAIR;
					this->s = cs;
					this->u2.s2 = new_estmt(RETURN,this->where,r,0);
					this->u2.s2->u1.ret_tp = this->u1.ret_tp;
				}
				else {	/* {_dtor()s; return x; */
					this->base = PAIR;
					this->s = dl;
					this->u2.s2 = new_estmt(RETURN,this->where,this->u2.e,0);
				}
			}
		}
		break;
	}

	case WHILE:
	case DO:
		expr_simpl(this->u2.e);
		{
			Pstmt obl;
			Pstmt ocl;

			obl = break_del_list;
			ocl = continue_del_list;
			break_del_list = 0;
			continue_del_list = 0;
			stmt_simpl(this->s);
			break_del_list = obl;
			continue_del_list = ocl;
		}
		break;

	case SWITCH:
		expr_simpl(this->u2.e);
		{
			Pstmt obl;

			obl = break_del_list;
			break_del_list = 0;
			stmt_simpl(this->s);
			break_del_list = obl;
		}
		switch (this->s->base){
		case DEFAULT:
		case LABEL:
		case CASE:
			break;
		case BLOCK:
			if (!this->s->s) break;

			switch (this->s->s->base){
			case BREAK:	/*  to cope with #define Case break; case */
			case CASE:
			case LABEL:
			case DEFAULT:
				break;
			default:
				goto df;
			}
			break;
		default:
		df:
			errorIloc('w', &this->s->where,
				"S not reached: case label missing",
				ea0, ea0, ea0, ea0);
		}
		break;

	case CASE:
		expr_simpl(this->u2.e);
		stmt_simpl(this->s);
		break;

	case LABEL:
		if (del_list)
			errorFIPC('s', "label in blockW destructors",
				ea0, ea0, ea0, ea0);
		stmt_simpl(this->s);
		break;

	case GOTO:
		/*	If the goto is going to a different(effective) scope,
		 *	then it is necessary to activate all relevant destructors
		 *	on the way out of nested scopes, and issue errors if there
		 *	are any constructors on the way into the target.
		 *
		 *	Only bother if the goto and label have different effective
		 *	scopes.(If mem table of goto == mem table of label, then
		 *	they're in the same scope for all practical purposes.
		 */
		{
		Pname n;

		n = look(scope, this->u1.d->u2.string, LABEL);
		if (n == 0){
			V1.u1.p = (char *)this->u1.d;
			errorIloc('i', &this->where, "label%n missing",
				&V1, ea0, ea0 , ea0);
		}
		if (n->u5.n_realscope != scope && n->n_assigned_to){
			Ptable r, q, p;

			/*
			 *	Find the root of the smallest subtree
			 *	containing the path of the goto.
			 *	This algorithm is quadratic only if the
			 *	goto is to an inner or unrelated scope.
			 */
			r = 0;
			for(q = n->u5.n_realscope; q != gtbl ;q = q->next) {
				for( p = scope ;p != gtbl ;p = p->next) {
					if (p == q){
						r = p; /* found root of subtree */
						goto xyzzy;
					}
				}
			}
xyzzy:
			if (r == 0)
				errorIloc('i', &this->where,
					"finding root of subtree", ea0,
					ea0, ea0, ea0);

			/* At this point, r = root of subtree, n->n_realscope
			 * = mem table of label, and scope = mem table of goto.
			 */

			/* Climb the tree from the label mem table to the table
			 * preceding the root of the subtree, looking for
			 * initializers and ctors.  If the mem table "belongs"
			 * to an unsimplified block(s), the n_initializer field
			 * indicates presence of initializer, otherwise initializer
			 * information is recorded in the init_stat field of
			 * mem table.
			 */
			for(p = n->u5.n_realscope; p != r ;p = p->next)
				if (p->init_stat == 2){
					V1.u1.p = (char *)this->u1.d;
					errorFloc(&this->where, "goto%n pastDWIr",
						&V1, ea0, ea0, ea0);
					goto plugh; /* avoid multiple error msgs */
				}
				else if (p->init_stat == 0){
					int i;
					Pname nn;

					nn = get_mem(p, i = 1);
					for(;nn ;nn = get_mem(p,++i))
						if (nn->u3.n_initializer
						|| nn->n_evaluated) {
							V1.u1.p = (char *)this->u1.d;
							V2.u1.p = (char *)nn;
							errorFloc(&nn->where,
								"goto%n pastId%n",
								&V1, &V2, ea0, ea0);
							goto plugh;
						}
				}

plugh:
			/*
			 * Proceed in a similar manner from the point
			 * of the goto, generating the code to activate
			 * dtors before the goto.
			 * There is a bug in this code.
			 * If there are class objects of the same name
			 * and type in(of course) different mem tables
			 * on the path to the root of the subtree from
			 * the goto, then the innermost object's dtor
			 * will be activated more than once.
			 */
			{
			Pstmt dd, ddt;
			Pname n, cln;
			int i;

			dd = ddt = 0;
			for(p = scope; p != r ;p = p->next) {
				n = get_mem(p, i = 1);

				for(;n ;n = get_mem(p,++i)) {
					if (n->u1.tp == 0) continue; /* label */

					if (cln = is_cl_obj(n->u1.tp)){
						Pclass cl;
						Pname d;
								
						cl = (Pclass)cln->u1.tp;
						d = has_dtor(cl);

						if (d){	/* n->cl::~cl(0); */
							Pexpr r;
							Pexpr ee;
							Pexpr dl;
							Pstmt dls;

							r = new_ref(DOT,n,d);
							ee=new_expr(0,ELIST,zero,0);
							dl = new_call(r, ee);
							dls = new_estmt(SM,n->where,dl,0);
							dl->base = G_CALL;
							dl->u4.fct_name = d;
							if (dd)
								ddt->s_list = dls;
							else 
								dd = dls;
							ddt = dls;
						}
					}
					else if (cl_obj_vec){
						Pclass cl;
						Pname d;

						cl = (Pclass)cl_obj_vec->u1.tp;
						d = has_dtor(cl);
						/* _vec_delete(vec,noe,sz,dtor,0); */
						if (d){
							Pexpr a;
							Pstmt dls;

							a = cdvec(vec_del_fct,(Pexpr)n,cl,d,0);
							dls = new_estmt(SM,n->where,a,0);
							if (dd)
								ddt->s_list = dls;
							else 
								dd = dls;
							ddt = dls;
						}
					}
				} /* end mem table scan */
			} /* end dtor loop */

			/* "activate" the list of dtors obtained. */
			if (dd){
				Pstmt bs;

				stmt_simpl(dd);
				bs = new_stmt(0, this->base, this->where, 0);
				*bs = *this;
				this->base = PAIR;
				this->s = dd;
				this->u2.s2 = bs;
			}
			}
		} /* end special case for non-local goto */
		}
		break;

	case IF:
		expr_simpl(this->u2.e);
		stmt_simpl(this->s);
		if (this->u3.else_stmt) stmt_simpl(this->u3.else_stmt);
		break;

	case FOR:	/* "for(s;e;e2) s2; => "s; for(;e,e2) s2" */
		if (this->u3.for_init){
			stmt_simpl(this->u3.for_init);
		}

		if (this->u2.e) expr_simpl(this->u2.e);
		if (this->u1.e2){
			curr_expr = this->u1.e2;
			expr_simpl(this->u1.e2);
			if (this->u1.e2->base == ICALL
			&& this->u1.e2->u1.tp == (Ptype)void_type)
				errorFIPC('s',
					"call of inline voidF in forE",
		   			ea0, ea0, ea0, ea0);
		}
		{
			Pstmt obl;
			Pstmt ocl;

			obl = break_del_list;
			ocl = continue_del_list;
			break_del_list = 0;
			continue_del_list = 0;
			stmt_simpl(this->s);
			break_del_list = obl;
			continue_del_list = ocl;
		}
		break;

	case BLOCK:
		block_simpl((Pstmt)this);
		break;

	case PAIR:
		break;
	}

	if (this->base != BLOCK && this->memtbl){
		int i;
		Pstmt t1, tx, ss;
		Pname cln, tn;

		t1 = (this->s_list) ? stmt_simpl(this->s_list) : 0;
		tx = t1 ? t1: this;
		ss = 0;
		tn = get_mem(this->memtbl, i=1);
		for(;tn ;tn = get_mem(this->memtbl,++i)) {
			if (cln = is_cl_obj(tn->u1.tp)){
				Pclass cl;
				Pname d;

				cl = (Pclass)cln->u1.tp;
				d = has_dtor(cl);
				if (d){	/* n->cl::~cl(0); */
					Pexpr r;
					Pexpr ee;
					Pexpr dl;
					Pstmt dls;

					r = new_ref(DOT,tn,d);
					ee = new_expr(0, ELIST, zero, 0);
					dl = new_call(r, ee);
					dls = new_estmt(SM, tn->where, dl, 0);
					dl->base = G_CALL;
					dl->u4.fct_name = d;
					dls->s_list = ss;
					ss = dls;
				}
			}
		}
		if (ss){
			Pstmt t2;

			t2 = stmt_simpl(ss);
			switch (this->base){
			case IF:
			case WHILE:
			case DO:
			case SWITCH:
				temp_in_cond(this->u2.e, ss, this->memtbl);
				break;

			case PAIR:	/* call hide a return */
			{
				Pstmt ts;

				ts = this->u2.s2;
				while (ts->base == PAIR)ts = ts->u2.s2;
				if (ts->base == RETURN){
					this = ts;
					goto retu;
				}
				goto def;
			}
			case RETURN:
			retu:
			{
				Pname cln;

				if (this->u2.e == 0){
					/* return; dtors; => dtors; return; */
					Pstmt rs;

					rs = new_estmt(RETURN, this->where, 0, 0);
					rs->u3.empty = this->u3.empty;
					rs->u1.ret_tp = this->u1.ret_tp;
					this->base = PAIR;
					this->s = ss;
					this->u2.s2 = rs;
					return (t1 ? t1: rs);
				}
				ccheck(this->u2.e);
				cln = is_cl_obj(this->u2.e->u1.tp);

				if (cln == 0
				|| has_oper((Pclass)cln->u1.tp, ASSIGN) == 0) {
					/* ...return e; dtors; =>
					 * ...Xr; ...r = e; dtors; return r;
					 */
					Pname rv, n;
					Pstmt rs, ps;

					rv = new_name("_rresult"); /*NOT "_result"*/
					rv->u1.tp = this->u1.ret_tp; /* e->tp */
					if (this->memtbl == 0)
						this->memtbl =
							new_table(4,0,0);
					n = name_dcl(rv, this->memtbl, FCT);
					n->n_assigned_to = 1;
					name__dtor(rv);
					rs = new_estmt(RETURN, this->where, n, 0);
					rs->u1.ret_tp = this->u1.ret_tp;
					this->base = SM;
					this->u2.e = new_expr(0, ASSIGN,(Pexpr)n, this->u2.e);
					this->u2.e->u1.tp = n->u1.tp;
					ps = new_pair(this->where, ss, rs);
					ps->s_list = this->s_list;
					this->s_list = ps;
					return (t1 ? t1: rs);
				}
			}

			case FOR:
				/* don't know which expression the temp comes from */
				V1.u1.i = (int)this->base;
				V2.u1.p = (char *)cln;
				errorIloc('s', &this->where,
					"E in %kS needs temporary ofC%nW destructor",
					&V1, &V2, ea0, ea0);
				break;

			case SM:	/* place dtors after all "converted" DCLs */
				if (t1){
					Pstmt tt, ttt;

					tt = this;
					for(;(ttt=tt->s_list)&&ttt->base==SM; tt=ttt)
						/* EMPTY */ ;
					t2->s_list = ttt;
					tt->s_list = ss;
					return (t1 != tt)? t1: t2;
				}

			default:
			def:
				if (this->u2.e) ccheck(this->u2.e);
				if (t1){	/* t1 == tail of statement list */
					t2->s_list = this->s_list;
					this->s_list = ss;
					return t1;
				}
				this->s_list = ss;
				return t2;
			}
		}
		return (t1 ? t1: this);
	}
	return (this->s_list ? stmt_simpl(this->s_list): this);
}

/*
 *	now handles dtors in the expression of an IF stmt not general!
 */
Pstmt copy(this)
Pstmt this;
{
	Pstmt ns;

	ns = new_stmt(0, 0, curloc, 0);

	*ns = *this;
	if (this->s) ns->s = copy(this->s);
	if (this->s_list) ns->s_list = copy(this->s_list);

	switch (this->base){
	case PAIR:
		ns->u2.s2 = copy(this->u2.s2);
		break;
	}

	return ns;
}

/*
 *	change NEW node to CALL node
 */
void simpl_new(this)
Pexpr this;
{
	Pname cln;
	Pname ctor;
	int sz;
	Pexpr var_expr;
	Pexpr const_expr;
	Ptype tt;
	Pexpr arg;
	Pexpr szof;

	sz = 1;
	var_expr = 0;
	tt = this->u4.tp2;

	if (cln = is_cl_obj(tt)){
		Pclass cl;

		cl = (Pclass)cln->u1.tp;
		if (ctor = has_ctor(cl)) {	/* 0->cl_ctor(args) */
			Pexpr p;
			Pexpr c;

			p = zero;
			if (ctor->u4.n_table != cl->memtbl){
				/* no derived constructor: pre-allocate */
				Pexpr ce;

				ce = new_texpr(SIZEOF, tt, 0);
				ce->u1.tp = (Ptype)uint_type;
				ce = new_expr(0, ELIST, ce, 0);
				p = new_expr(0, G_CALL,(Pexpr)new_fct, ce);
				p->u4.fct_name = new_fct;
			}
			c = (Pexpr)this->u2.e1;
			c->u2.e1 = new_ref(REF, p,(Pname)c->u2.e1);
			call_simpl(c);
			*this = *((Pexpr)c);
			return;
		}
	}
	else if (cl_obj_vec){
		Pclass cl;

		cl = (Pclass)cl_obj_vec->u1.tp;
		ctor = has_ictor(cl);
		if (ctor == 0){
			if (has_ctor(cl)) {
				V1.u1.p = (char *)cl->string;
				error("new %s[], no defaultK", &V1, ea0, ea0, ea0);
			}
			cl_obj_vec = 0;
		}
	}

xxx:
	switch (tt->base){
	case TYPE:
		tt = ((Pbase)tt)->b_name->u1.tp;
		goto xxx;

	default:
		szof = new_texpr(SIZEOF, tt, 0);
		szof->u1.tp = (Ptype)uint_type;
		break;

	case VEC:
	{
		Pvec v;

		v = (Pvec)tt;
		if (v->size)
			sz *= v->size;
		else if (v->dim)
			if (var_expr)
				var_expr = new_expr(0, MUL, var_expr, v->dim);
			else
				var_expr = v->dim;
		else {
			sz = SZ_WPTR;		/* <<< sizeof(int*) */
			break;
		}
		tt = v->typ;
		goto xxx;
		}
	}

	if (cl_obj_vec){
		Pexpr noe;

		const_expr = new_ival(sz);
		if (var_expr) {
			if (sz != 1)
				noe = new_expr(0, MUL, const_expr, var_expr);
			else
				noe = var_expr;
		 } else
			noe = const_expr;

		const_expr = szof;
		const_expr->u1.tp = (Ptype)uint_type;
		this->base = CALL;
		arg = new_expr(0, ELIST,(Pexpr)ctor, 0);

		lval((Pexpr)ctor, ADDROF);
		arg = new_expr(0, ELIST, const_expr, arg);
		arg = new_expr(0, ELIST, noe, arg);
		arg = new_expr(0, ELIST, zero, arg);
		this->base = CAST;
		this->u4.tp2 = this->u1.tp;
		this->u2.e1 = new_expr(0, G_CALL,(Pexpr)vec_new_fct, arg);
		this->u2.e1->u4.fct_name = vec_new_fct;
		expr_simpl(this);
		return;
	}

	/* call _new(element_size*no_of_elements) */
	if (sz == 1)
		arg = (var_expr) ? new_expr(0, MUL, szof, var_expr): szof;
	else {
		const_expr = new_ival(sz);
		const_expr->u1.tp = (Ptype)uint_type;
		const_expr = new_expr(0, MUL, const_expr, szof);
		const_expr->u1.tp = (Ptype)uint_type;
		arg = (var_expr)? new_expr(0,MUL,const_expr,var_expr): const_expr;
	}

	arg->u1.tp = (Ptype)uint_type;
	this->base = CAST;
	this->u4.tp2 = this->u1.tp;
	this->u2.e1=new_expr(0, G_CALL,(Pexpr)new_fct, new_expr(0,ELIST,arg,0));
	this->u2.e1->u4.fct_name = new_fct;
	expr_simpl(this);
}

/*
 *	delete p => _delete(p);
 *		    or  cl::~cl(p,1);
 *	delete[s]p => _delete(p);
 *			or vec_del_fct(p,vec_sz,elem_sz,~cl,1);
 */		 
void simpl_delete(this)
Pexpr this;
{
	Ptype tt;
	Pname cln, n;

	tt = this->u2.e1->u1.tp;
ttloop:
	switch (tt->base){
	case TYPE:
		tt = ((Pbase)tt)->b_name->u1.tp;
		goto ttloop;
	case VEC:
	case PTR:
		tt = ((Pptr)tt)->typ;
		break;
	}

	cln = is_cl_obj(tt);

	if (cln && (n=has_dtor((Pclass)cln->u1.tp))) {
		/* ~cl() might be virtual */
		Pexpr r;

		r = this->u2.e1;
		if (this->u3.e2 == 0){	/* e1->cl::~cl(1) */
			Pexpr rrr;
			Pexpr aaa;
			Pexpr ee;

			rrr = new_ref(REF, r, n);
			aaa = new_expr(0, ELIST, one, 0);
			ee = new_call(rrr, aaa);
			ee->u4.fct_name = (Pname)n;
			ee->base = G_CALL;
			if (((Pfct)n->u1.tp)->f_virtual){
				ee = new_expr(0, QUEST, ee, zero);
				ee->u1.tp = ee->u2.e1->u1.tp;
				ee->u4.cond = r;
			}
			*this = *ee;
			expr__dtor(ee);
			expr_simpl(this);
			return;
		}
		else {	/* del_cl_vec(e1, e2, elem_size, ~cl, 1); */
			Pexpr sz, arg, a;

			sz = new_texpr(SIZEOF, tt, 0);
			sz->u1.tp = (Ptype)uint_type;
			arg = one;
			if (((Pfct)n->u1.tp)->f_virtual){
				/* beware of side effects in expression e1 */
				if (this->u2.e1->base != NAME)
					errorFIPC('s',
						"PE too complicated for delete[]",
						ea0, ea0, ea0, ea0);
				a = new_ref(REF, this->u2.e1, n);
				a = address(a);
				arg = new_expr(0, ELIST, a, arg);
			}
			else {
				arg = new_expr(0, ELIST,(Pexpr)n, arg);
				lval((Pexpr)n, ADDROF);
			}
			arg = new_expr(0, ELIST, sz, arg);
			arg = new_expr(0, ELIST, this->u3.e2, arg);
			arg = new_expr(0, ELIST, this->u2.e1, arg);
			this->base = G_CALL;
			this->u2.e1 = (Pexpr)vec_del_fct;
			this->u3.e2 = arg;
			this->u4.fct_name = vec_del_fct;
		}
	}
	else if (cl_obj_vec){
		errorFIPC('s', "delete vector of vectors",ea0,ea0,ea0,ea0);
	}
	else {
		this->base = G_CALL;
		this->u3.e2 = new_expr(0, ELIST, this->u2.e1, 0);
		this->u2.e1 = (Pexpr)(this->u4.fct_name = del_fct);
	}
	call_simpl((Pexpr)this);
}

static Pexpr new_texte(a, b)
char *a, *b;
{
	Pexpr Xthis_texte;

	Xthis_texte = (Pexpr) new( sizeof(struct expr) );
	Xthis_texte = new_expr(Xthis_texte, TEXT, 0, 0);
	Xthis_texte->u2.string = a;
	Xthis_texte->u3.string2 = b;

	return Xthis_texte;
}

static Pstmt new_pair(ll, a, b)
Loc ll;
Pstmt a, b;
{
	Pstmt Xthis_pair;

	Xthis_pair = 0;
	Xthis_pair = new_stmt(Xthis_pair, PAIR, ll, a);
	Xthis_pair->u2.s2 = b;

	return Xthis_pair;
}

Pexpr new_texpr(bb, tt, ee)
TOK bb;
Ptype tt;
Pexpr ee;
{
	Pexpr Xthis_texpr;

	Xthis_texpr = 0;
	Xthis_texpr = new_expr(Xthis_texpr, bb, ee, 0);
	Xthis_texpr->u4.tp2 = tt;

	return Xthis_texpr;
}

Pexpr new_call(aa, bb)
Pexpr aa;
Pexpr bb;
{
	Pexpr Xthis_call;

	Xthis_call = 0;
	Xthis_call = new_expr(Xthis_call, CALL, aa, bb);

	return Xthis_call;
}

Pexpr new_ival(ii)
int ii;
{
	Pexpr Xthis_ival;

	Xthis_ival = 0;
	Xthis_ival = new_expr(Xthis_ival, IVAL, 0, 0);
	Xthis_ival->u2.i1 = ii;

	return Xthis_ival;
}

Pstmt new_estmt(t, ll, ee, ss)
TOK t;
Loc ll;
Pexpr ee;
Pstmt ss;
{
	Pstmt Xthis_estmt;

	Xthis_estmt = 0;
	Xthis_estmt = new_stmt(Xthis_estmt, t, ll, ss);
	Xthis_estmt->u2.e = ee;

	return Xthis_estmt;
}

Pstmt new_ifstmt(ll, ee, ss1, ss2)
Loc ll;
Pexpr ee;
Pstmt ss1, ss2;
{
	Pstmt Xthis_ifstmt;

	Xthis_ifstmt = 0;
	Xthis_ifstmt = new_stmt(Xthis_ifstmt, IF, ll, ss1);
	Xthis_ifstmt->u2.e = ee;
	Xthis_ifstmt->u3.else_stmt = ss2;

	return Xthis_ifstmt;
}
