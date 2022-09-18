/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/expr2.c	1.1"
/***************************************************************************

expr2.c:

	type check expressions

************************************************************************/

#include "cfront.h"
#include "size.h"

static int refd;
int tsize = 0;
int t_const = 0;

Pname make_tmp(c, t, tbl)
char c;
Ptype t;
Ptable tbl;
{
	Pname tmpx, tmp, nn;

	if (Cstmt){	/* make Cstmt into a block */
		if (Cstmt->memtbl == 0)
			Cstmt->memtbl = new_table(4, tbl, 0);
		tbl = Cstmt->memtbl;
	}
	tmpx = new_name(make_name(c));
	tmpx->u1.tp = t;
	tmp = name_dcl(tmpx, tbl, ARG);	/* ARG => no init */
	nn = tbl->t_name;
	if (nn && nn->u1.tp->base == CLASS) tsize = tsizeof(tmp->u1.tp);
	name__dtor(tmpx);
	tmp->n_scope = FCT;
	return tmp;
}

Pexpr init_tmp(tmp, init, tbl)
Pname tmp;
Pexpr init;
Ptable tbl;
{
	Pname cn;
	Pname ct;
	Pexpr ass;

	cn = is_cl_obj(tmp->u1.tp);
	ct = cn ? has_itor((Pclass)cn->u1.tp) : 0;

	tmp->n_assigned_to = 1;

	if (ct){	/* must inilialize */
		Pexpr a, r;

		if (refd &&
		  (init->u2.e1->base == NAME || init->u2.e1->base == REF))
			init = new_expr(0, G_CM, init, address(init->u2.e1));

		if (refd) tbl = 0;
		r = new_ref(DOT, tmp, ct);
		a = new_expr(0, ELIST, init, 0);
		ass = new_expr(0, G_CALL, r, a);
		ass->u4.fct_name = ct;
		if (tbl) ass = expr_typ(ass, tbl);
	}
	else {		/* can assign */
		ass = new_expr(0, ASSIGN,(Pexpr)tmp, init);
		ass->u1.tp = tmp->u1.tp;
	}
	return ass;
}

void assign(this)
Pname this;
{
	if (this->n_assigned_to++ == 0){
		switch (this->n_scope){
		case FCT:
			if (this->n_used && this->n_addr_taken == 0) {
				Ptype t;

				t = this->u1.tp;
			ll:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto ll;
				case VEC:
					break;
				default:
					V1.u1.p = (char *)this;
					if (curr_loop)
						errorFIPC('w',
						"%n may have been used before set",
							&V1, ea0, ea0, ea0);
					else 
						errorFIPC('w',"%n used before set",
							&V1, ea0, ea0, ea0);
				}
			}
		}
	}
}

int lval(this, oper)
Pexpr this;
TOK oper;
{
	register Pexpr ee;
	register Pname n;
	int deref;
	char *es;

	ee = this;
	deref = 0;
	switch (oper){
	case ADDROF:
	case G_ADDROF:
		es = "address of";
		break;
	case INCR:
	case DECR:
		es = "increment of";
		goto def;
	case DEREF:
		es = "dereference of";
		break;
	default:
		es = "assignment to";
	def:
		if (tconst(this->u1.tp)){
			if (oper){
				V1.u1.p = (char *)es;
				V2.u1.p = (char *)this;
				if (this->base == NAME)
					error("%s constant%n", &V1,&V2,ea0,ea0);
				else 
					error("%s constant", &V1, ea0, ea0, ea0);
			}
			return 0;
		}
	}

	for(;;) {
		switch (ee->base){
		case G_CALL:
		case CALL:
			if (deref == 0){
				switch (oper){
				case ADDROF:
				case G_ADDROF:
				case 0:
					if (ee->u4.fct_name
					&&((Pfct)ee->u4.fct_name->u1.tp)->f_inline)
						return 1;
				}
			}
		default:
			if (deref == 0){
				if (oper){
					V1.u1.p = (char *)es;
					V2.u1.i = (int)ee->base;
					error("%s %k(not an lvalue)",
						&V1, &V2, ea0, ea0);
				}
				return 0;
			}
			return 1;
		case ZERO:
		case CCON:
		case ICON:
		case FCON:
			if (oper){
				V1.u1.p = (char *)es;
				error("%s numeric constant", &V1, ea0, ea0, ea0);
			}
			return 0;
		case STRING:
			if (oper){
				V1.u1.p = (char *)es;
				errorFIPC('w', "%s string constant",
					&V1, ea0, ea0, ea0);
			}
			return 1;

		case DEREF:
		{	Pexpr ee1;

			ee1 = ee->u2.e1;
			if (ee1->base == ADDROF) /* *& vanishes */
				ee = ee1->u3.e2;
			else {
				ee = ee1;
				deref = 1;
			}
			break;
		}

		case DOT:
			switch (ee->u2.e1->base){ /* update use counts, etc. */
			case NAME:
				switch (oper){
				case ADDROF:
				case G_ADDROF:
					take_addr((Pname)ee->u2.e1);
				case 0:
					break;
				case ASSIGN:
					((Pname)ee->u2.e1)->n_used--;
				default:
					assign((Pname)ee->u2.e1); /* asop */
				}
				break;
			case DOT:
			{	Pexpr e;

				e = ee->u2.e1;
				do 
					e = e->u2.e1;
				while (e->base == DOT);

				if (e->base == NAME){
					switch (oper){
					case ADDROF:
					case G_ADDROF:
						take_addr((Pname)e);
					case 0:
						break;
					case ASSIGN:
						((Pname)e)->n_used--;
					default:
						assign((Pname)e); /* asop */
					}
				}
			}
			}

			n = ee->u4.mem;
			if (deref == 0 && tconst(ee->u2.e1->u1.tp)){
				switch (oper){
				case 0:
				case ADDROF:
				case G_ADDROF:
				case DEREF:
					break;
				default:
					V1.u1.p = (char *)es;
					V2.u1.p = (char *)n;
					V3.u1.p = (char *)ee->u2.e1->u1.tp;
					error("%sM%n of%t", &V1, &V2, &V3, ea0);
				}
				return 0;
			}
			goto xx;

		case REF:
			n = ee->u4.mem;
			if (deref == 0){
				Ptype p;

				p = ee->u2.e1->u1.tp;
			zxc:
				switch (p->base){
				case TYPE:
					p = ((Pbase)p)->b_name->u1.tp;
					goto zxc;
				case PTR:
				case VEC:
					break;
				default:
					V1.u1.p = (char *)p;
					V2.u1.p = (char *)n;
					errorFIPC('i', "%t->%n", &V1,&V2,ea0,ea0);
				}
				if (tconst(((Pptr)p)->typ)){
					switch (oper){
					case 0:
					case ADDROF:
					case G_ADDROF:
					case DEREF:
						break;
					default:
						V1.u1.p = (char *)es;
						V2.u1.p = (char *)n;
						V3.u1.p = (char *)((Pptr)p)->typ;
						error("%sM%n of%t", &V1,&V2,&V3,ea0);
					}
					return 0;
				}
			}
			goto xx;
		case NAME:
			n = (Pname)ee;
		xx:
			if (deref || oper == 0) return 1;

			if (n->u1.tp->base==FIELD &&((Pbase)n->u1.tp)->b_bits==0) {
				V1.u1.p = (char *)es;
				V2.u1.p = (char *)n;
				error("%s 0-length field%n", &V1,&V2,ea0,ea0);
				return 0;
			}
			switch (oper){
			case ADDROF:
			case G_ADDROF:
			{	Pfct f;

				f = (Pfct)n->u1.tp;
				if (n->n_sto == REGISTER){
					V1.u1.p = (char *)n;
					error("& register%n", &V1, ea0, ea0, ea0);
					return 0;
				}
				if (f == 0){
					V1.u1.p = (char *)n;
					error("& label%n", &V1, ea0, ea0, ea0);
					return 0;
				}
				if (n->n_stclass == ENUM){
					V1.u1.p = (char *)n;
					error("& enumerator%n", &V1,ea0,ea0,ea0);
					return 0;
				}
				if (n->u1.tp->base == FIELD){
					V1.u1.p = (char *)es;
					V2.u1.p = (char *)n;
					error("& field%n", &V1, &V2, ea0, ea0);
					return 0;
				}
				n->n_used--;
				if (n->u5.n_qualifier)
	n = look(((Pclass)n->u4.n_table->t_name->u1.tp)->memtbl, n->u2.string, 0);
				take_addr(n);

				if ((n->n_evaluated && n->n_scope != ARG)
				|| (f->base == FCT && f->f_inline) ) {
					/*address of const or inline: allocate it*/
					Pname nn;

					nn = new_name(0);
					if (n->n_evaluated && n->n_scope != ARG) {
						/* use allocated version */
						n->n_evaluated = 0;
						n->u3.n_initializer =
							new_ival(n->n_val);
					}
					*nn = *n;
					nn->n_sto = STATIC;
					nn->n_list = dcl_list;
					dcl_list = nn;
				}
				break;
			}
			case ASSIGN:
				n->n_used--;
				assign(n);
				break;
			default:	/* incr ops, and asops */
				if (cc->tot && n == cc->c_this) {
					V1.u1.p = (char *)n;
					V2.u1.i = (int)oper;
					error("%n%k", &V1, &V2, ea0, ea0);
					return 0;
				}
				assign(n);
			}
			return 1;
		}
	}
}

Pexpr Ninit;	/* default argument used; */
int Nstd = 0;	/* standard coercion used(derived* =>base* or int=>long or ...) */

/*
 *	look for an exact match between "n" and the argument list "arg" 
 */
bit gen_match(n, arg)
Pname n;
Pexpr arg;
{
	Pfct f;
	register Pexpr e;
	register Pname nn;

	f = (Pfct)n->u1.tp;
	for(e=arg, nn=f->argtype; e; e=e->u3.e2, nn=nn->n_list) {
		Pexpr a;
		Ptype at, nt;

		a = e->u2.e1;
		at = a->u1.tp;
		if (at->base == ANY) return 0;
		if (nn == 0) return f->nargs_known == ELLIPSIS;

		nt = nn->u1.tp;

		switch (nt->base){
		case RPTR:
			if (at == (Ptype)zero_type) return 0;
			if (type_check(nt, at, COERCE)) {
				Pptr pt;

				pt = addrof(at);
				nt->base = PTR;		/* handle derived classes */
				if (type_check(nt,(Ptype)pt, COERCE)) {
					nt->base = RPTR;
					delete((char *)pt);
					return 0;
				}
				nt->base = RPTR;
				delete((char *)pt);
			}
			break;
		default:
			switch (at->base) {
			default:
				if (type_check(nt, at, COERCE)) return 0;
				break;
			case OVERLOAD:
			{
				register Plist gl;
				Pgen g;
				int no_match;

				g = (Pgen)at;
				no_match = 1;

				for(gl = g->fct_list ;gl ;gl = gl->l)
				{
					Pname nn;
					Ptype t;

					nn = gl->f;
					t = nn->u1.tp;
					if (type_check(nt, t, COERCE) == 0) {
						no_match = 0;
						break;
					}
				}
				if (no_match) return 0;
			}
			}
		}
	}

	if (nn){
		Ninit = nn->u3.n_initializer;
		return (Ninit != 0);
	}

	return 1;
}

Pname Ncoerce;

/*	return number of possible coercions of t2 into t1,
 *	Ncoerce holds a coercion function(not constructor), if found
 */
bit can_coerce(t1, t2)
Ptype t1, t2;
{
	Pname c1, c2;
	int val;

	Ncoerce = 0;
	if (t2->base == ANY) return 0;

	switch (t1->base){
	case RPTR:
	rloop:
		switch (t2->base){
		case TYPE:
			t2 = ((Pbase)t2)->b_name->u1.tp;
			goto rloop;
		default:
		{
			Ptype tt1, tt2;
			int i;

			tt2 = (Ptype)addrof(t2);
			if (type_check(t1, tt2, COERCE) == 0) return 1;
			tt1 = ((Pptr)t1)->typ;
			i = can_coerce(tt1, t2);
			return i;
		}
		}
	}

	c1 = is_cl_obj(t1);
	c2 = is_cl_obj(t2);
	val = 0;

	if (c1){
		Pclass cl;
		Pname ctor;
		register Pfct f;

		cl = (Pclass)c1->u1.tp;
		if (c2 && c2->u1.tp == (Ptype)cl) return 1;

		/*	look for constructor
		 *		with one argument
		 *		or with default for second argument
		 *	of acceptable type
		 */
		ctor = has_ctor(cl);
		if (ctor == 0) goto oper_coerce;
		f = (Pfct)ctor->u1.tp;

		switch (f->base){
		case FCT:
			switch (f->nargs){
			case 1:
			one:
			{	Ptype tt;
				tt = f->argtype->u1.tp;
				if (type_check(tt, t2, COERCE) == 0) val = 1;
				if (tt->base == RPTR){
					Pptr pt;

					pt = (Pptr)addrof(t2);
					/* handle derived classed */
					tt->base = PTR;
					if (type_check(tt,(Ptype)pt,COERCE) == 0)
						val = 1;
					tt->base = RPTR;
					delete((char *)pt);
				}
				goto oper_coerce;
			}
			default:
				if (f->argtype->n_list->u3.n_initializer) goto one;
			case 0:
				goto oper_coerce;
			}
		case OVERLOAD:
		{	register Plist gl;

			for(gl = ((Pgen)f)->fct_list ;gl ;gl = gl->l) {
				/* look for match */
				Pname nn;
				Pfct ff;

				nn = gl->f;
				ff = (Pfct)nn->u1.tp;
				switch (ff->nargs){
				case 0:
					break;
				case 1:
				over_one:
				{	Ptype tt;

					tt = ff->argtype->u1.tp;
					if (type_check(tt, t2, COERCE) == 0)
						val = 1;
					if (tt->base == RPTR){
						Pptr pt;

						/* handle derived class */
						pt = addrof(t2);
						tt->base = PTR;
						if( !type_check(tt,(Ptype)pt,COERCE))
						{
							tt->base = RPTR;
							delete((char *)pt);
							val = 1;
							goto oper_coerce;
						}
						tt->base = RPTR;
						delete((char *)pt);
					}
					break;
				}
				default:
					if (ff->argtype->n_list->u3.n_initializer)
						goto over_one;
				}
			}
			goto oper_coerce;
		}
		default:
			V1.u1.i = (int)f->base;
			errorFIPC('i', "cannot_coerce(%k)\n", &V1, ea0, ea0, ea0);
		}
	}
oper_coerce:
	if (c2){
		Pclass cl;
		int std;
		register Pname on;

		cl = (Pclass)c2->u1.tp;
		std = 0;
		for(on = cl->conv; on ;on = on->n_list) {
			Pfct f;

			f = (Pfct)on->u1.tp;
			Nstd = 0;
			if (type_check(t1, f->returns, COERCE) == 0){
				if (Nstd == 0){
					/* forget solutions involving
					 * standard conversions
					 */
					if (std){	/* forget */
						val = 1;
						std = 0;
					}
					else 
						val++;
					Ncoerce = on;
				}
				else {	/* take note only if no exact match seen */
					if (val == 0 || std){
						Ncoerce = on;
						val++;
						std = 1;
					}
				}
			}
		}
	}
	if (val) return val;
	if (c1 && has_itor((Pclass)c1->u1.tp)) return 0;
	if (type_check(t1, t2, COERCE)) return 0;
	return 1;
}


/*
 *	look to see if the argument list "arg" can be coerced into a call of "n"
 *	1: it can
 *	0: it cannot or it can be done in more than one way
 */
int gen_coerce(n, arg)
Pname n;
Pexpr arg;
{
	Pfct f;
	register Pexpr e;
	register Pname nn;
	Pexpr a;
	Ptype at;
	int i;

	f = (Pfct)n->u1.tp;
	for(e=arg, nn=f->argtype; e; e=e->u3.e2, nn=nn->n_list) {
		if (nn == 0) return (f->nargs_known == ELLIPSIS);
		a = e->u2.e1;
		at = a->u1.tp;
		i = can_coerce(nn->u1.tp, at);
		if (i != 1)return 0;
	}
	if (nn && nn->u3.n_initializer == 0) return 0;
	return 1;
}


Pname Nover;
int Nover_coerce = 0;

/*	
 *	return 2 if n(arg) can be performed without user defined coercion of arg
 *	return 1 if n(arg) can be performed only with user defined coercion of arg
 *	return 0 if n(arg) is an error
 *	Nover is the function found, if any
 */
int over_call(n, arg)
Pname n;
Pexpr arg;
{
	register Plist gl;
	Pgen g;
	Pname exact;
	int no_exact;

	g = (Pgen)n->u1.tp;
	if (arg && arg->base != ELIST) errorFIPC('i', "ALX", ea0, ea0, ea0, ea0);
	Nover_coerce = 0;
	switch (g->base){
	default:
		V1.u1.p = (char *)g;
		errorFIPC('i', "over_call(%t)\n", &V1, ea0, ea0, ea0);
	case OVERLOAD:
		break;
	case FCT:
		Nover = n;
		Ninit = 0;
		if (gen_match(n, arg) && Ninit == 0) return 2;
		return gen_coerce(n, arg);
	}

	exact = 0;
	no_exact = 0;
	for(gl = g->fct_list ;gl ;gl = gl->l) {		/* look for match */
		Nover = gl->f;
		Ninit = 0;
		Nstd = 0;
		if (gen_match(Nover, arg) && Ninit == 0){
			if (Nstd == 0)return 2;
			if (exact)
				no_exact++;
			else 
				exact = Nover;
		}
	}

	if (exact){
		if (no_exact){
			V1.u1.p = (char *)n;
			errorFIPC('w',
				"more than one standard conversion possible for%n",
				&V1, ea0, ea0, ea0);
		}
		Nover = exact;
		return 2;
	}

	Nover = 0;
	for(gl = g->fct_list ;gl ;gl = gl->l) {		/* look for coercion */
		Pname nn;

		nn = gl->f;
		if (gen_coerce(nn, arg)){
			if (Nover){
				Nover_coerce = 2;
				return 0;
			}
			Nover = nn;
		}
	}

	return (Nover ? 1: 0);
}

/* is ``found'' visible? */
void visible_check(found, string, e, no_virt)
Pname found;
char *string;
Pexpr e;
bit no_virt;
{
	Pexpr e1;
	Pbase b;
	Pfct fn;
	Ptype pt;
	Ptable tblx;

	e1 = e->u2.e1;
	if (e->base == NEW){
		if (found->u1.tp->base == OVERLOAD){
			int no_match;
			Plist gl;

			Ninit = 0;
			no_match = 1;
			for(gl = ((Pgen)found->u1.tp)->fct_list ;gl ;gl = gl->l) {
				if (e->u2.e1 == 0){
					if (((Pfct)gl->f->u1.tp)->nargs_known
					&& ((Pfct)gl->f->u1.tp)->nargs == 0) {
						found = gl->f;
						break;
					}
					else 
						continue;
				}

				if (gen_match(gl->f, e) && Ninit == 0) {
					found = gl->f;
					break;
				}
				if (no_match && gen_coerce(gl->f, e)) {
					no_match = 0;
					found = gl->f;
				}
			}
		}
		if (found->u1.tp->base == OVERLOAD)
			fn = (Pfct)((Pgen)found->u1.tp)->fct_list->f->u1.tp;
		else
			fn = (Pfct)found->u1.tp;
		pt = fn->s_returns;
		b = (Pbase)((Pptr)pt)->typ;
		goto xxxx;
	}
	if (e1)
		switch (e1->base){
		default:
			if (no_virt)e->u2.e1 = (Pexpr)found;
			/* instead of using fct_name */
			return;

		case REF:
		{	Ptype pt;

			if (no_virt)
				e1->u4.mem = found; /*instead of using fct_name*/
			if (e1->u2.e1 == 0)return;  /* constructor: this==0 */
			pt = e1->u2.e1->u1.tp;
			for(;pt->base == TYPE ;pt = ((Pbase)pt)->b_name->u1.tp)
				/* EMPTY */;
			b = (Pbase)((Pptr)pt)->typ;
			break;
		}
		case DOT:
			if (no_virt) e1->u4.mem = found;
			/* instead of using fct_name */
			b = (Pbase)e1->u2.e1->u1.tp;
		}

xxxx:
	switch (b->base){
	case TYPE:
		b = (Pbase)b->b_name->u1.tp;
		goto xxxx;
	case ANY:
		return;
	case COBJ:
		break;
	default:
		V1.u1.p = (char *)b;
		errorFIPC('i', "no tblx %p", &V1, ea0, ea0, ea0);
	}

	tblx = b->b_table;
	if (tblx->base != TABLE){
		V1.u1.p = (char *)tblx;
		V2.u1.i = (int)tblx->base;
		errorFIPC('i', "tblx %p %d", &V1, &V2, ea0, ea0);
	}
	if (lookc(tblx, string) == 0) return;

	switch (found->n_scope){
	case 0:
		if (Epriv && Epriv != cc->cot
		&& !has_friend(Epriv, cc->nof)
		&& !(found->n_protect && baseofname(Epriv, cc->nof))) {
			V1.u1.p = (char *)found;
			V2.u1.p = found->n_protect? "protected" : "private";
			error("%n is %s", &V1, &V2, ea0, ea0);
			break;
		}
		/* FALLTHRU */
	case PUBLIC:
		if (Ebase &&
		   (cc->cot == 0 ||
			(Ebase!=(Pclass)cc->cot->clbase->u1.tp
			 && !has_friend(Ebase,cc->nof)
			)
		    )
		   ) {
			V1.u1.p = (char *)found;
			error("%n is from a privateBC", &V1, ea0, ea0, ea0);
		}
	}
}


/*
 *	check "this" call:
 *		 e1(e2)
 *	e1->typ() and e2->typ() has been done
 */
Ptype fct_call(this, tbl)
Pexpr this;
Ptable tbl;
{
	Pfct f;
	Pname fn;
	int x;
	int k;
	Pname nn;
	Pexpr e;
	Ptype t;
	Pexpr arg;
	Ptype t1;
	int argno;
	Pexpr etail;
	Pname no_virt;	/* set if explicit qualifier was used: c::f() */

	arg = this->u3.e2;
	etail = 0;
	switch (this->base){
	case CALL:
	case G_CALL:
		break;
	default:
		V1.u1.i = (int)this->base;
		errorFIPC('i', "fct_call(%k)", &V1, ea0, ea0, ea0);
	}

	if (this->u2.e1 == 0 ||(t1=this->u2.e1->u1.tp) == 0) {
		V1.u1.p = (char *)this->u2.e1;
		V2.u1.p = (char *)t1;
		errorFIPC('i', "fct_call(e1=%d,e1->tp=%t)", &V1, &V2, ea0, ea0);
	}
	if (arg && arg->base != ELIST) {
		V1.u1.p = (char *)arg;
		V2.u1.i = (int)arg->base;
		errorFIPC('i', "badAL%d%k", &V1, &V2, ea0, ea0);
	}
	switch (this->u2.e1->base){
	case NAME:
		fn = (Pname)this->u2.e1;
		switch (fn->n_oper){
		case 0:
		case CTOR:
		case DTOR:
		case TYPE:
			break;
		default:	/* real operator: check for operator+(1,2); */
		{	Pexpr a;

			if (arg == 0)break;
			a = arg->u2.e1;		/* first operand */
			if (is_cl_obj(a->u1.tp) || is_ref(a->u1.tp)) break;
			a = arg->u3.e2;
			if (a == 0) {		/* unary */
				V1.u1.i = (int)fn->n_oper;
				error("%k of basicT", &V1, ea0, ea0, ea0);
			} else {		/* binary */
				a = a->u2.e1;	/* second operand */
				if (is_cl_obj(a->u1.tp) || is_ref(a->u1.tp))
					break;
				V1.u1.i = (int)fn->n_oper;
				error("%k of basicTs", &V1, ea0, ea0, ea0);
			}
			break;
		}
		}
		no_virt = fn->u5.n_qualifier;
		break;
	case REF:
	case DOT:
		fn = this->u2.e1->u4.mem;
		no_virt = fn->u5.n_qualifier;
		break;
	case MEMPTR:
	default:
		fn = 0;
		no_virt = 0;
	}

lll:
	switch (t1->base){
	case TYPE:
		t1 = ((Pbase)t1)->b_name->u1.tp;
		goto lll;

	case PTR:	/* pf() allowed as shorthand for(*pf)() */
		if (((Pptr)t1)->typ->base == FCT){
			if (((Pptr)t1)->memof)
				error("O missing in call throughP toMF",
					ea0, ea0, ea0, ea0);
			t1 = ((Pptr)t1)->typ;
			fn = 0;
			goto case_fct;
		}

	default:
		V1.u1.p = (char *)fn;
		V2.u1.p = (char *)fn;
		V3.u1.p = (char *)this->u2.e1->u1.tp;
		error("call of%n;%n is a%t", &V1, &V2, &V3, ea0);

	case ANY:
		return (Ptype)any_type;

	case OVERLOAD:
	{
		register Plist gl;
		Pgen g;
		Pname found;
		Pname exact;
		int no_exact;

		g = (Pgen)t1;
		found = 0;
		exact = 0;
		no_exact = 0;
		for(gl = g->fct_list ;gl ;gl = gl->l) {	/* look for match */
			register Pname nn;

			nn = gl->f;
			Ninit = 0;
			Nstd = 0;
			if (gen_match(nn, arg)){
				if (Nstd == 0){
					found = nn;
					goto fnd;
				}
				if (exact)
					no_exact++;
				else 
					exact = nn;
			}
		}

		if (exact){
			if (no_exact){
				V1.u1.i = no_exact+1;
				V2.u1.p = (char *)fn;
				errorFIPC('w',
					"%d standard conversion possible for%n",
					&V1, &V2, ea0, ea0);
			}
			found = exact;
			goto fnd;
		}

		for(gl = g->fct_list ;gl ;gl = gl->l) {	/* look for coercion */
			register Pname nn;

			nn = gl->f;
			if (gen_coerce(nn, arg)){
				if (found){
					V1.u1.p = (char *)fn;
					error("ambiguousA for overloaded%n",
						&V1, ea0, ea0, ea0);
					goto fnd;
				}
				found = nn;
			}
		}

	fnd:
		if (this->u4.fct_name = found){
			f = (Pfct)found->u1.tp;
			visible_check(found, g->string, this,(no_virt!=0));
		}
		else {
			V1.u1.p = (char *)fn;
			error("badAL for overloaded%n", &V1, ea0, ea0, ea0);
			return (Ptype)any_type;
		}
		break;
	}
	case FCT:
	case_fct:
		f = (Pfct)t1;
		if ((this->u4.fct_name = fn) && fn->n_oper == CTOR) {
			visible_check(fn, fn->u2.string, this, 0);
		}
	}

	if (no_virt)this->u4.fct_name = 0;

	t = f->returns;
	x = f->nargs;
	k = f->nargs_known;

	if (k == 0){
		if (fct_void && fn && x == 0 && arg)
			if (no_of_badcall++ == 0)badcall = fn;
		goto rlab;
	}

	e = arg;
	nn = f->argtype;
	argno = 1;
	for( ; e || nn; nn=nn->n_list, e=etail->u3.e2, argno++) {
		Pexpr a;

		if (e){
			a = e->u2.e1;
			etail = e;
			if (nn){	/* type check */
				Ptype t1;

				t1 = nn->u1.tp;
			lx:
				switch (t1->base){
				case TYPE:
					if (!t_const)
						t_const = ((Pbase)t1)->b_const;
					t1 = ((Pbase)t1)->b_name->u1.tp;
					goto lx;
				case RPTR:
					a = ref_init((Pptr)t1, a, tbl);
					goto cbcb;
				case COBJ:
					if (a->base != G_CM
					|| type_check(t1, a->u1.tp, ASSIGN))
						a = class_init(0, t1, a, tbl);
					if (nn->n_xref){
						a = address(a);
					}
					else {
						/* defend against:
						 *	int f(X); ... X(X&);
						 */
						Pname cln;

						cln = ((Pbase)t1)->b_name;
						if (cln
						&& has_itor((Pclass)cln->u1.tp)) {
							/* mark X(X&) arguments */
							nn->n_xref = 1;
							a = address(a);
						}
					}
				cbcb:
					if (a->base == G_CM){
						if (a->u2.e1->base == DEREF)
							/*(*e1,e2) =>(e1,e2) */
							a->u2.e1 = a->u2.e1->u3.e2;
						if (a->u2.e1->base == G_CALL
						&&(Pname)a->u2.e1->u4.fct_name
						&&((Pname)a->u2.e1->u4.fct_name)->n_oper==CTOR
						&&(a->u3.e2->base == G_ADDROF
						    || a->u3.e2->base == ADDROF
						   )
						)
							a = a->u2.e1;
							/*(ctor(&tmp),&tmp)
							 * => ctor(&tmp)
							 */
					}
					e->u2.e1 = a;
					break;
				case ANY:
					goto rlab;
				case PTR:
					e->u2.e1 = a = ptr_init((Pptr)t1, a, tbl);
					goto def;
				case CHAR:
				case SHORT:
				case INT:
					if (a->base == ICON
					&& a->u1.tp == (Ptype)long_type) {
						V1.u1.p = (char *)fn;
						V2.u1.i = (int)t1->base;
						errorFIPC('w',
							"long constantA for%n,%kX",
							&V1, &V2, ea0, ea0);
					}
				case LONG:
					if (((Pbase)t1)->b_unsigned
					&& a->base == UMINUS
					&& a->u3.e2->base == ICON) {
						V1.u1.p = (char *)fn;
						errorFIPC('w',
							"negativeA for%n, unsignedX",
							&V1, ea0, ea0, ea0);
					}
				default:
				def:
				{	Pexpr x;

					x = try_to_coerce(t1, a, "argument", tbl);
					if (x)
						e->u2.e1 = x;
					else if (type_check(t1, a->u1.tp, ARG)){
						if (arg_err_suppress == 0){
							V1.u1.i = argno;
							V2.u1.p = (char *)fn;
							V3.u1.p = (char *)a->u1.tp;
							V4.u1.p = (char *)nn->u1.tp;
							error("badA %dT for%n:%t(%tX)",
								&V1,&V2,&V3,&V4);
						}
						return (Ptype)any_type;
					}
				}
				}
			}
			else {
				if (k != ELLIPSIS){
					if (arg_err_suppress == 0){
						V1.u1.i = argno;
						V2.u1.p = (char *)fn;
						error("unexpected %dA for%n",
							&V1, &V2, ea0, ea0);
					}
					return (Ptype)any_type;
				}
				goto rlab;
			}
		}
		else {	/* default argument? */
			a = nn->u3.n_initializer;
			if (a == 0){
				if (arg_err_suppress == 0){
					V1.u1.i = argno;
					V2.u1.p = (char *)nn->u1.tp;
					V3.u1.p = (char *)fn;
					error("A %d ofT%tX for%n", &V1,
						&V2, &V3, ea0);
				}
				return (Ptype)any_type;
			}

			a->permanent = 2; /* ought not be necessary, but it is */
			e = new_expr(0, ELIST, a, 0);
			if (etail)
				etail->u3.e2 = e;
			else 
				this->u3.e2 = e;
			etail = e;
		}
	}
rlab:
	/* protect against class cn; cn f(); ... class cn { cn(cn&): ... }; */
	if (fn && f->f_result == 0) {
		Pname cn;

		cn = is_cl_obj(f->returns);
		if (cn && has_itor((Pclass)cn->u1.tp)) {
			int ll;

			ll = (this->u4.fct_name->u4.n_table == gtbl)? 2: 1;
			/* amazing fudge: use count doubled! */
			if (ll < this->u4.fct_name->n_used){
				V1.u1.p = (char *)fn;
				V2.u1.p = (char *)cn;
				V3.u1.p = (char *)cn->u2.string;
				V4.u1.p = (char *)cn->u2.string;
				errorFIPC('s',
					"%n returning %n called before %s(%s&)D seen",
					&V1, &V2, &V3, &V4);
			}
			make_res(f);
		}
	}
	if (f->f_result){		/* f(args) => f(&temp,args),temp */
		Pname tn;
		Pexpr ee;

		tn = make_tmp('R', f->returns, tbl);
		this->u3.e2 = new_expr(0, ELIST, address((Pexpr)tn), this->u3.e2);
		ee = new_expr(0, 0, 0, 0);
		*ee = *this;
		this->base = G_CM;	/* f(&temp,args),temp */
		this->u2.e1 = ee;
		this->u3.e2 = (Pexpr)tn;
	}
	return t;
}

/*
 *	initialize the "p" with the "init"
 */
Pexpr ref_init(p, init, tbl)
Pptr p;
Pexpr init;
Ptable tbl;
{
	register Ptype it;
	Ptype p1;
	Pname c1;

	it = init->u1.tp;
	p1 = p->typ;
	c1 = is_cl_obj(p1);

rloop:
	switch (it->base){
	case TYPE:
		it = ((Pbase)it)->b_name->u1.tp;
		goto rloop;
	default:
		{	Ptype tt;
			int x;

			tt = (Ptype)addrof(it);
			p->base = PTR;	/* allow &x for y& when y : public x */
					/* but not &char for int& */
			x = type_check((Ptype)p, tt, COERCE);
			if (x == 0){
				if (tconst(init->u1.tp) && vec_const == 0) {
					/* not ``it'' */
					((Pptr)tt)->rdo = 1;
					if (tconst(p->typ) == 0)
						error("R to constO", ea0, ea0, ea0,
							ea0);
				}
				if (type_check((Ptype)p, tt, COERCE))
					error("R to constO", ea0, ea0, ea0, ea0);
				p->base = RPTR;

				if (lval(init, 0)) return address(init);
				goto xxx;
			}
			p->base = RPTR;
		}
	}
	if (c1){
		Pexpr a;

		refd = 1;
		a = class_init(0, p1, init, tbl);
		refd = 0;
		if (a == init && init->u1.tp != (Ptype)any_type) goto xxx;
		switch (a->base){
		case G_CALL:
		case CM:
			init = a;
			goto xxx;
		}
		return address(a);
	}
	if (type_check(p1, it, 0)){
		V1.u1.p = (char *)it;
		V2.u1.p = (char *)p;
		error("badIrT:%t(%tX)", &V1, &V2, ea0, ea0);
		if (init->base != NAME) init->u1.tp = (Ptype)any_type;
		return init;
	}

xxx:	/*
	 *	here comes the test of a ``fundamental theorem'':
	 *	a structure valued expression is
	 *		(1) an lvalue of type T(possibly const)
	 *	or	(2) the result of a function(a _result if X(X&) is defined)
	 *	or	(3) a * or [] or ? or , expression
	 */
	switch (init->base){
	case NAME:
	case DEREF:
	case REF:
	case DOT:			/* init => &init */
		if (tconst(it) && vec_const == 0) goto def;
		lval(init, ADDROF);

	case G_CM:			/* &(f(&temp), temp) */
		return address(init);

	case QUEST:
		switch (init->u2.e1->base){	/* try for: &(c?a:b) => c?&a:&b */
		case NAME:
		case DEREF:
		case REF:
		case DOT:
			if (tconst(init->u2.e1->u1.tp) && vec_const == 0) break;
		case G_CM:
			switch (init->u3.e2->base){
			case NAME:
			case DEREF:
			case REF:
			case DOT:
				if (tconst(init->u3.e2->u1.tp) && vec_const == 0)
					break;
			case G_CM:
				init->u2.e1 = address(init->u2.e1);
				init->u3.e2 = address(init->u3.e2);
				return init;
			}
		}

		if (has_itor((Pclass)c1->u1.tp)) {
			V1.u1.p = (char *)c1;
			V2.u1.p = (char *)c1->u2.string;
			V3.u1.p = (char *)c1->u2.string;
			errorFIPC('s', "?:E ofT%n: %s(%s&)Dd", &V1,
				&V2, &V3, ea0);
			return init;
		}
		goto def;
	case CALL:
	case G_CALL:
		goto def;

	case CM:		/* try for &(... , b) =>(... , &b) */
	{	Pexpr ee;

		ee = init->u3.e2;
	cml:
		switch (ee->base){
		case CM:
			ee = ee->u3.e2;
			goto cml;
		case G_CM:
		case NAME:
		case DEREF:
			return address(init);
		}
		/* FALLTHRU */
	}

	default:
	def:
	{	Pname n;
		Pexpr a, as;

		if (tbl == gtbl)
			errorFIPC('s', "Ir for staticR not an lvalue",
				ea0, ea0, ea0, ea0);
		n = make_tmp('I', p1, tbl);
		if (c1 != is_cl_obj(init->u1.tp)){
			/* derived class => must cast:
			 * ``it Ix;(Ix=init,(p)&Ix);''
			 */
			n->u1.tp = init->u1.tp;
			a = address((Pexpr)n);
			PERM(p);
			a = new_texpr(CAST, p, a);
			a->u1.tp = (Ptype)p;
		}
		else 
			a = address((Pexpr)n);

		if (init->base == ASSIGN && init->u2.e1->base == DEREF)
			init = new_expr(0, G_CM, init, init->u2.e1->u2.e1);

		refd = 1;
		as = init_tmp(n, init, tbl);
		refd = 0;
		a = new_expr(0, CM, as, a);
		a->u1.tp = a->u3.e2->u1.tp;
		return a;
	}
	}
}


/*
 *	initialize "nn" of type "tt" with "init"
 *	if nn==0 make a temporary,
 *	nn may not be a name
 */
Pexpr class_init(nn, tt, init, tbl)
Pexpr nn;
Ptype tt;
Pexpr init;
Ptable tbl;
{
	Pname c1;
	Pname c2;

	c1 = is_cl_obj(tt);
	c2 = is_cl_obj(init->u1.tp);
	if (c1){
		if (c1 != c2 || (refd == 0 && has_itor((Pclass)c1->u1.tp))) {
			int i;

			/*	really ought to make a temp if refd,
			 *	but ref_init can do that
			 */
			i = can_coerce(tt, init->u1.tp);
			switch (i){
			default:
				V1.u1.i = i;
				V2.u1.p = (char *)c1;
				V3.u1.p = (char *)init->u1.tp;
				error("%d ways of making a%n from a%t",
					&V1, &V2, &V3, ea0);
				init->u1.tp = (Ptype)any_type;
				return init;
			case 0:
				V1.u1.p = (char *)c1;
				V2.u1.p = (char *)init->u1.tp;
				error("cannot make a%n from a%t", &V1,
					&V2, ea0, ea0);
				init->u1.tp = (Ptype)any_type;
				return init;
			case 1:
				if (Ncoerce == 0){
					Pexpr a;

					a = new_expr(0, ELIST, init, 0);
					a = new_texpr(VALUE, tt, a);
					a->u3.e2 = nn;
					return expr_typ(a, tbl);
				}

				switch (init->base){
#ifdef BSD
				case CALL:
				case G_CALL:
#endif
				case CM:
				case NAME:	/* init.coerce() */
				{
					Pexpr r, rr;

					r = new_ref(DOT, init, Ncoerce);
					rr = expr_typ(r, tbl);
					init = new_expr(0, G_CALL, rr, 0);
					init->u4.fct_name = Ncoerce;
					break;
				}
				default:	/*(temp=init,temp.coerce()) */
				{	Pname tmp;
					Pexpr ass, r, rr, c;

					tmp = make_tmp('U', init->u1.tp, tbl);
					ass = init_tmp(tmp, init, tbl);
					r = new_ref(DOT, tmp, Ncoerce);
					rr = expr_typ(r, tbl);
					c = new_expr(0, G_CALL, rr, 0);
					c->u4.fct_name = Ncoerce;
					init = new_expr(0, CM, ass, c);
				}
				}
				if (nn){
					Pexpr a;

					a = new_expr(0, ELIST, init, 0);
					a = new_texpr(VALUE, tt, a);
					a->u3.e2 = nn;
					return expr_typ(a, tbl);
				}
			}
			return expr_typ(init, tbl);
		}
		else if (refd == 0){
			/* bitwise copy, check for dtor & operator= */
			Pclass cl;

			cl = (Pclass)c1->u1.tp;
			if (cl->itor == 0){
				if (cl->bit_ass == 0)
				{
					V1.u1.p = (char *)cl->string;
					errorFIPC('w',
					"bitwise copy: %s has a memberW operator=()",
						&V1, ea0, ea0, ea0);
				} else if (has_dtor(cl) && has_oper(cl, ASSIGN)) {
					V1.u1.p = (char *)cl->string;
					V2.u1.p = (char *)cl->string;
					V3.u1.p = (char *)cl->string;
					errorFIPC('w',
			"bitwise copy: %s has assignment and destructor but not %s(%s&)",
						&V1, &V2, &V3, ea0);
				}
			}
		}
		return init;
	}

	if (type_check(tt, init->u1.tp, ASSIGN) && refd == 0) {
		V1.u1.p = (char *)init->u1.tp;
		V2.u1.p = (char *)tt;
		error("badIrT:%t(%tX)", &V1, &V2, ea0, ea0);
		init->u1.tp = (Ptype)any_type;
	}
	return init;
}

/*	assume s points to a string:
 *		'c'
 *	or	'\c'
 *	or	'\0'
 *	or	'\ddd'
 *	or multi-character versions of the above
 *	(hex constants have been converted to octal by the parser)
 */
int char_to_int(s)
char *s;
{
	register int i;
	register char c, d, e;

	i = 0;
	switch (*s){
	default:
		errorFIPC('i', "char constant store corrupted",ea0,ea0,ea0,ea0);
	case '`':
		errorFIPC('s', "bcd constant", ea0, ea0, ea0, ea0);
		return 0;
	case '\'':
		break;
	}

	for(;;)	{		/* also handle multi-character constants */
		switch (c = *++s) {
		case '\'':
			return i;
		case '\\':		/* special character */
			switch (c = *++s) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': /* octal representation */
				c -= '0';
				switch (d = *++s) {	/* try for 2 */
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7':
					d -= '0';
					switch (e = *++s) { /* try for 3 */
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						c = c*64+d*8+e-'0';
						break;
					default:
						c = c*8+d;
						s--;
					}
					break;
				default:
					s--;
				}
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case '\\':
				c = '\\';
				break;
			case '\'':
				c = '\'';
				break;
			}
			/* FALLTHRU */
		default:
			if (i)i <<= BI_IN_BYTE;
			i += c;
		} /* switch */
	} /* for(;;) */
}

int A10 = 'A'-10;
int a10 = 'a'-10;

/*
 *	read decimal, octal, or hexadecimal integer
 */
int str_to_int(p)
register const char *p;
{
	register int c;
	register int i;

	i = 0;
	if ((c=(int)*p++) == '0'){
		switch (c = *p++) {
		case 0:
			return 0;

		case 'l':
		case 'L':	/* long zero */
			return 0;

		case 'x':
		case 'X':	/* hexadecimal */
			while (c = *p++)
				switch (c){
				case 'l':
				case 'L':
					return i;
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					i = i*16 + c-A10;
					break;
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					i = i*16 + c-a10;
					break;
				default:
					i = i*16 + c-'0';
				}
			return i;

		default:	/* local */
			do 
				switch (c){
				case 'l':
				case 'L':
					return i;
				default:
					i = i*8 + c-'0';
				}
			while (c = *p++);
			return i;
		}
	}

	i = c - '0';		/* decimal */
	while (c = *p++)
		switch (c){
		case 'l':
		case 'L':
			return i;
		default:
			i = i*10 + c-'0';
		}
	return i;
}


char *Neval = 0;
bit binary_val;

int eval(this)
Pexpr this;
{
	int i1, i2;

	if (Neval)return 1;

	switch (this->base){
	case ZERO:	return 0;
	case IVAL:	return this->u2.i1;
	case ICON:	return str_to_int(this->u2.string);
	case CCON:	return char_to_int(this->u2.string);
	case FCON:	Neval = "float in constant expression"; return 1;
	case STRING:	Neval = "string in constant expression"; return 1;
	case EOBJ:	return ((Pname)this)->n_val;
	case SIZEOF:	return tsizeof(this->u4.tp2);
	case NAME:
	{
		Pname n;

		n = (Pname)this;
		if (n->n_evaluated && n->n_scope != ARG) return n->n_val;
		if (n->u3.n_initializer && n->n_scope != ARG
		&& n->u3.n_initializer->base == IVAL
		&& n->u3.n_initializer->u2.i1 == n->n_val)
			return n->n_val;
		if (binary_val && strcmp(this->u2.string, "_result") == 0)
			return 8888;
		Neval = "cannot evaluate constant";
		return 1;
	}
	case ICALL:
		if (this->u2.e1){
			int i;

			this->u4.il->i_next = curr_icall;
			curr_icall = this->u4.il;
			i = eval(this->u2.e1);
			curr_icall = this->u4.il->i_next;
			return i;
		}
		Neval = "void inlineF";
		return 1;
	case ANAME:
	{
		Pname n;
		int argno;
		Pin il;
		Pexpr aa;

		n = (Pname)this;
		argno = n->n_val;
		for(il = curr_icall ;il ;il = il->i_next)
			if (il->i_table == n->u4.n_table)goto aok;
		goto bok;
	aok:
		if (il->local[argno]){
	bok:
			Neval="inlineF call too complicated for constant expression";
			return 1;
		}
		aa = il->arg[argno];
		return eval(aa);
		
	}
	case CAST:
	{
		int i;
		Ptype tt;

		i = eval(this->u2.e1);
		tt = this->u4.tp2;
	strip:
		switch (tt->base){
		case TYPE:
			tt = ((Pbase)tt)->b_name->u1.tp;
			goto strip;
		case LONG:
			errorFIPC('w', "cast to long in constantE(ignored)",
				ea0, ea0, ea0, ea0);
			break;
		case INT:
		case CHAR:
		case SHORT:
			if (((Pbase)this->u4.tp2)->b_unsigned && i < 0)
				Neval = "cast to unsigned in constant expression";
			else 
			{
				int diff;

				diff = tsizeof(int_type)-tsizeof(this->u4.tp2);
				if (diff){ /* narrowing may affect the value */
					int bits;
					int div;

					bits = diff*BI_IN_BYTE;
					div = 256;	/* 2**8 */
					if (BI_IN_BYTE != 8){
						errorFIPC('i',
			"expr::eval() assumes 8 bit bytes please re-write it",
							ea0, ea0, ea0, ea0);
					}
					while (--diff)div *= 256;
					i = (i << bits)/ div;
				}
			}
		}
		return i;
	}
	case UMINUS:
	case UPLUS:
	case NOT:
	case COMPL:
	case PLUS:
	case MINUS:
	case MUL:
	case LS:
	case RS:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case AND:
	case OR:
	case ER:
	case DIV:
	case MOD:
	case QUEST:
	case EQ:
	case ANDAND:
	case OROR:
		break;
	case G_ADDROF:
	case ADDROF:
		if (binary_val){	/* beware of &*(T*)0 */
			switch (this->u3.e2->base){
			case NAME:
			case DOT:
			case REF:
				return 9999;
			}
		}
	default:
		Neval = "bad operator in constant expression";
		return 1;
	}

	i1 = (this->u2.e1) ? eval(this->u2.e1): 0;
	i2 = (this->u3.e2) ? eval(this->u3.e2): 0;

	switch (this->base){
	case UMINUS:	return -i2;
	case UPLUS:	return i2;
	case NOT:	return !i2;
	case COMPL:	return ~i2;
	case CAST:	return i1;
	case PLUS:	return i1+i2;
	case MINUS:	return i1-i2;
	case MUL:	return i1*i2;
	case LS:	return i1<<i2;
	case RS:	return i1>>i2;
	case NE:	return i1!=i2;
	case EQ:	return i1==i2;
	case LT:	return i1<i2;
	case LE:	return i1<=i2;
	case GT:	return i1>i2;
	case GE:	return i1>=i2;
	case AND:	return i1&i2;
	case ANDAND:	return i1&&i2;
	case OR:	return i1|i2;
	case OROR:	return i1||i2;
	case ER:	return i1^i2;
	case MOD:	return (i2 == 0)? 1: i1%i2;
	case QUEST:	return eval(this->u4.cond) ? i1: i2;
	case DIV:
		if (i2 == 0){
			Neval = "divide by zero";
			errorFIPC('w', "divide by zero", ea0, ea0, ea0, ea0);
			return 1;
		}
		return i1/i2;
	}
}


/*
 *	is ``this'' class a public base class of "f"'s class
 *	or its immediate base class
 */
bit baseofname(this, f)
Pclass this;
Pname f;
{
	Ptable ctbl;
	Pname b;

	ctbl = f->u4.n_table;
	b = ctbl->t_name;

	/* f is a member of class b or a class derived from ``b'' */
	for(;;) {
		Pclass cl;

		if (b == 0)return 0;
		cl = (Pclass)b->u1.tp;
		if (cl == 0)return 0;
		if (cl == this)return 1;
		if (cl->pubbase == 0)	/* immediate base class ? */
			return cl->clbase && cl->clbase->u1.tp==(Ptype)this;
		b = cl->clbase;
	}
}

/*
 *	is ``this'' class a public base class of "cl"
 */
bit baseofclass(this, cl)
Pclass this, cl;
{
	for(;;) {
		Pname b;

		if (cl == 0) return 0;
		if (cl == this) return 1;
		if (cl->pubbase==0 && this->clbase
		&& cl != (Pclass)this->clbase->u1.tp)
			return 0;
		b = cl->clbase;
		if (b == 0) return 0;
		cl = (Pclass)b->u1.tp;
	}
}

/*
 *	does this class have function "f" as its friend?
 */
bit has_friend(this, f)
Pclass this;
Pname f;
{
	Ptable ctbl;
	Plist l;

	if (f == 0) return 0;
	ctbl = f->u4.n_table;
	for(l = this->friend_list ;l ;l = l->l) {
		Pname fr;

		fr = l->f;
		switch (fr->u1.tp->base){
		case CLASS:
			if (((Pclass)fr->u1.tp)->memtbl == ctbl) return 1;
			break;
		case COBJ:
			if (((Pbase)fr->u1.tp)->b_table == ctbl) return 1;
			break;
		case FCT:
			if (fr == f) return 1;
			break;
		case OVERLOAD:
			l->f = fr = ((Pgen)fr->u1.tp)->fct_list->f; /* first fct */
			if (fr == f) return 1;
			break;
		default:
			V1.u1.i = (int)fr->u1.tp->base;
			errorFIPC('i', "bad friend %k", &V1, ea0, ea0, ea0);
		}
	}
	return 0;
}

/*
 *	a kludge initialize/assign-to pointer to function
 */
Pexpr pt(ef, e, tbl)
Pfct ef;
Pexpr e;
Ptable tbl;
{
	Pfct f;
	Pname n;

	n = 0;
	switch (e->base){
	case NAME:
		f = (Pfct)e->u1.tp;
		n = (Pname)e;
		switch (f->base){
		case FCT:
		case OVERLOAD:
			e = new_expr(0, G_ADDROF, 0, e);
			e->u1.tp = (Ptype)f;
		}
		goto ad;

	case DOT:
	case REF:
		f = (Pfct)e->u4.mem->u1.tp;
		switch (f->base){
		case FCT:
		case OVERLOAD:
			n = (Pname)e->u4.mem;
			e = new_expr(0, G_ADDROF, 0, e);
			e = expr_typ(e, tbl);
		}
		goto ad;

	case ADDROF:
	case G_ADDROF:
		f = (Pfct)e->u3.e2->u1.tp;
	ad:
		if (f->base == OVERLOAD){
			Pgen g;

			g = (Pgen)f;
			n = find(g, ef, 0);
			if (n == 0){
				V1.u1.p = (char *)g->string;
				error("cannot deduceT for &overloaded %s()",
					&V1, ea0, ea0, ea0);
			}
			e->u3.e2 = (Pexpr)n;
			e->u1.tp = n->u1.tp;
		}
		if (n) lval((Pexpr)n, ADDROF);
	}
	return e;
}

/*
 *	check for silly initializers
 *
 *	char* p = 0L;	 ??	fudge to allow redundant and incorrect `L'
 *	char* p = 2 - 2; ??	worse
 */
Pexpr ptr_init(p, init, tbl)
Pptr p;
Pexpr init;
Ptable tbl;
{
	Ptype it;

	it = init->u1.tp;
itl:
	switch (it->base){
	case TYPE:
		it = ((Pbase)it)->b_name->u1.tp;
		goto itl;
	case ZTYPE:
		if (init == zero)break;
	case INT:
	case CHAR:
	case SHORT:
	{
		int i;

		Neval = 0;
		i = eval(init);
		if (Neval) {
			V1.u1.p = (char *)Neval;
			error("badPIr: %s", &V1, ea0, ea0, ea0);
		} else if (i) {
			V1.u1.i = i;
			error("badPIr value %d", &V1, ea0, ea0, ea0);
		} else {
			if (TO_DEL(init)) expr_del(init);
			init = zero;
		}
		break;
	}
	case LONG:
		if (init->base == ICON && init->u2.string[0] == '0'
		&& (init->u2.string[1] == 'L' || init->u2.string[1] == 'l')) {
			if (TO_DEL(init)) expr_del(init);
			init = zero;
		}
	}

	return (p->typ->base == FCT)? pt((Pfct)p->typ, init, tbl): init;
}

Pexpr try_to_overload(this, tbl)
Pexpr this;
Ptable tbl;
{
	TOK bb;

	Pname n1, n2, gname;
	Ptype t1, t2;
	Pexpr oe2, ee1, ee2;
	char *obb;
	int go, nc, ns;

	bb = (this->base==DEREF && this->u3.e2 == 0) ? MUL : this->base;

	n1 = 0;
	t1 = 0;
	if (this->u2.e1){
		Ptype tx;

		t1 = this->u2.e1->u1.tp;
		tx = t1;
		while (tx->base == TYPE) tx = ((Pbase)tx)->b_name->u1.tp;
		n1 = is_cl_obj(tx);
	}
	n2 = 0;
	t2 = 0;
	if (this->u3.e2){
		Ptype tx;

		t2 = this->u3.e2->u1.tp;
		tx = t2;
		while (tx->base == TYPE) tx = ((Pbase)tx)->b_name->u1.tp;
		n2 = is_cl_obj(tx);
	}
	if (n1 == 0 && n2 == 0) return 0;

	/* first try for non-member function:   op(e1,e2) or op(e2) or op(e1) */
	oe2 = this->u3.e2;
	if (this->u3.e2 && this->u3.e2->base!=ELIST)
		ee2 = this->u3.e2 = new_expr(0, ELIST, this->u3.e2, 0);
	else
		ee2 = 0;
	ee1 = (this->u2.e1) ? new_expr(0,ELIST,this->u2.e1,this->u3.e2) : ee2;
	obb = oper_name(bb);
	gname = look(gtbl, obb, 0);
	go = (gname ? over_call(gname, ee1): 0);

	/* first look at member functions, then if necessary check for ambiguities */
	nc = Nover_coerce;
	if (go)gname = Nover;
	ns = Nstd;

	if (n1){		/* look for member of n1 */
		Ptable ctbl;
		Pname mname;
		int mo;

		ctbl = ((Pclass)n1->u1.tp)->memtbl;
		mname = look(ctbl, obb, 0);
		if (mname == 0)goto glob;
		switch (mname->n_scope){
		default:
			goto glob;
		case 0:
		case PUBLIC:	break;		/* try e1.op(?) */
		}

		mo = over_call(mname, this->u3.e2);

		switch (mo){
		case 0:
			if (go == 2)goto glob;
			if (1 < Nover_coerce)goto am1;
			goto glob;
		case 1:
			if (go == 2)goto glob;
			if (go == 1){
			am1:
				V1.u1.p = (char *)n1;
				V2.u1.p = (char *)t2;
				V3.u1.i = (int)bb;
				error("ambiguous operandTs%n and%t for%k",
					&V1, &V2, &V3, ea0);
				this->u1.tp = (Ptype)any_type;
				return this;
			}
			else if (((Pclass)n1->u1.tp)->conv){
				switch (bb){
				case ASSIGN:
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
					/*don't coerce left hand side of assignment*/
					break;
				default:
					if (is_cl_obj(((Pfct)((Pclass)n1->u1.tp)->conv->u1.tp)->returns))
						break;
					V1.u1.i = (int)bb;
					V2.u1.p = (char *)Nover->u1.tp;
					errorFIPC('w',
					"overloaded%k may be ambiguous.FWT%tused",
						&V1, &V2, ea0, ea0);
				}
			}
			break;
		case 2:
			if (go == 2 && ns <= Nstd) {
				V1.u1.i = (int)bb;
				V2.u1.p = (char *)gname;
				V3.u1.p = (char *)Nover;
				error("%k defined both as%n and%n", &V1,
					&V2, &V3, ea0);
			}
		}

		if (bb==ASSIGN && mname->u4.n_table!=ctbl) { /* inherited = */
			V1.u1.p = (char *)n1;
			error("assignment not defined for class%n", 
				&V1, ea0, ea0, ea0);
			this->u1.tp = (Ptype)any_type;
			return this;
		}

		this->base = G_CALL;		/* e1.op(e2) or e1.op() */
		this->u2.e1 = new_ref(DOT, this->u2.e1, Nover);
		if (ee1) expr__dtor(ee1);
		return expr_typ(this, tbl);
	}

	if (n2 && this->u2.e1 == 0) {		/* look for unary operator */
		Ptable ctbl;
		Pname mname;
		int mo;

		ctbl = ((Pclass)n2->u1.tp)->memtbl;
		mname = look(ctbl, obb, 0);
		if (mname == 0)goto glob;
		switch (mname->n_scope){
		default:	goto glob;
		case 0:
		case PUBLIC:	break;		/* try e2.op() */
		}

		mo = over_call(mname, 0);

		switch (mo){
		case 0:
			if (1 < Nover_coerce) goto am2;
			goto glob;
		case 1:
			if (go == 2)goto glob;
			if (go == 1){
			am2:
				V1.u1.p = (char *)n2;
				V2.u1.i = (int)bb;
				error("ambiguous operandT%n for%k",
					&V1, &V2, ea0, ea0);
				this->u1.tp = (Ptype)any_type;
				return this;
			}
			break;
		case 2:
			if (go == 2 && ns <= Nstd) {
				V1.u1.i = (int)bb;
				V2.u1.p = (char *)gname;
				V3.u1.p = (char *)Nover;
				error("%k defined both as%n and%n", &V1, &V2,
					&V3, ea0);
			}
		}

		this->base = G_CALL;		/* e2.op() */
		this->u2.e1 = new_ref(DOT, oe2, Nover);
		this->u3.e2 = 0;
		if (ee2) expr__dtor(ee2);
		if (ee1 && ee1 != ee2) expr__dtor(ee1);
		return expr_typ(this, tbl);
	}
glob:
	if (1 < nc){
		V1.u1.p = (char *)t1;
		V2.u1.p = (char *)t2;
		V3.u1.i = (int)bb;
		error("ambiguous operandTs%t and%t for%k", &V1, &V2, &V3, ea0);
		this->u1.tp = (Ptype)any_type;
		return this;
	}
	if (go){
		if (go == 1){	/* conversion necessary => binary */
			/* very sloppy test: */
			if ((n1 &&((Pclass)n1->u1.tp)->conv
				 && is_cl_obj(((Pfct)((Pclass)n1->u1.tp)->conv->u1.tp)->returns)==0)
			||  (n2 &&((Pclass)n2->u1.tp)->conv
				 && is_cl_obj(((Pfct)((Pclass)n2->u1.tp)->conv->u1.tp)->returns)==0)
			) {
				V1.u1.i = (int)bb;
				V2.u1.p = (char *)gname->u1.tp;
				errorFIPC('w',
					"overloaded%k may be ambiguous.FWT%tused",
					&V1, &V2, ea0, ea0);
			}
		}
		this->base = G_CALL;	/* op(e1,e2) or op(e1) or op(e2) */
		this->u2.e1 = (Pexpr)gname;
		this->u3.e2 = ee1;
		return expr_typ(this, tbl);
	}

	if (ee2) expr__dtor(ee2);
	if (ee1 && ee1 != ee2) expr__dtor(ee1);
	this->u3.e2 = oe2;

	switch (bb){
	case ASSIGN:
	case ADDROF:
		break;
	case DEREF:
		if (this->u3.e2){
			Pexpr x;

			/* fudge to cope with ! as subscripting op */
			this->base = NOT;
			x = try_to_overload(this, tbl);
			if (x)return x;
			this->base = DEREF;
		}
	case CALL:
		if (n1 == 0)break;
	default:	/* look for conversions to basic types */
	{	int found;

		found = 0;
		if (n1){
			int val;
			Pclass cl;
			Pname on;

			val = 0;
			cl = (Pclass)n1->u1.tp;
			for(on = cl->conv; on ;on = on->n_list) {
				Pfct f;

				f = (Pfct)on->u1.tp;
				if (bb == ANDAND || bb == OROR) {
					this->u2.e1 = check_cond(this->u2.e1,bb,tbl);
					return 0;
				}
				if (n2
				||(t2 && type_check(f->returns,t2,ASSIGN)==0)
				||(t2 && type_check(t2,f->returns,ASSIGN)==0)) {
					Ncoerce = on;
					val++;
				}
			}
			switch (val){
			case 0:
				if (this->base == NOT) return 0;
				break;
			case 1:
			{
				Pexpr r, rr;

				r = new_ref(DOT, this->u2.e1, Ncoerce);
				rr = expr_typ(r, tbl);
				this->u2.e1 = new_expr(0, G_CALL, rr, 0);
				found = 1;
				break;
			}
			default:
				V1.u1.p = (char *)n1;
				errorFIPC('s', "ambiguous coercion of%n to basicT",
					&V1, ea0, ea0, ea0);
			}
		}
		if (n2){
			int val;
			Pclass cl;
			Pname on;

			val = 0;
			cl = (Pclass)n2->u1.tp;
			for(on = cl->conv; on ;on = on->n_list) {
				Pfct f;

				f = (Pfct)on->u1.tp;
				if (bb == ANDAND || bb == OROR || bb == NOT) {
					this->u3.e2 = check_cond(this->u3.e2,bb,tbl);
					return 0;
				}
				if (n1
				||(t1 && type_check(f->returns,t1,ASSIGN)==0)
				||(t1 && type_check(t1,f->returns,ASSIGN)==0)) {
					Ncoerce = on;
					val++;
				}
			}
			switch (val){
			case 0:
				break;
			case 1:
			{	Pexpr r, rr;

				r = new_ref(DOT, this->u3.e2, Ncoerce);
				rr = expr_typ(r, tbl);
				this->u3.e2 = new_expr(0, G_CALL, rr, 0);
				found++;
				break;
			}
			default:
				V1.u1.p = (char *)n2;
				errorFIPC('s', "ambiguous coercion of%n to basicT",
					&V1, ea0, ea0, ea0);
			}
		}

		if (found) return expr_typ(this, tbl);
		V1.u1.p = (char *)t1;
		V2.u1.p = (char *)t2;
		V3.u1.i = (int)bb;
		if (t1 && t2)
			error("bad operandTs%t%t for%k", &V1, &V2, &V3, ea0);
		else 
			error("bad operandT%t for%k",(t1? &V1:&V2), &V3, ea0, ea0);
		this->u1.tp = (Ptype)any_type;
		return this;
	}
	}
	return 0;
}

extern int bound;	/* fudge for bound pointers to functions */

Pexpr docast(this, tbl)
Pexpr this;
Ptable tbl;
{
	Ptype t, tt, etp;
	int pmf, b;
	Pexpr ee;

	tt = t = this->u4.tp2;
	type_dcl(tt, tbl);
zaq:				/* is the cast legal? */
	switch (tt->base){
	case TYPE:
		tt = ((Pbase)tt)->b_name->u1.tp;
		goto zaq;
	case RPTR:
	case PTR:
		if (((Pptr)tt)->rdo) error("*const in cast", ea0, ea0, ea0, ea0);
	case VEC:
		tt = ((Pptr)tt)->typ;
		goto zaq;
	case FCT:
		break;	/* const is legal in function return types */
	default:
		if (((Pbase)tt)->b_const) error("const in cast", ea0,ea0,ea0,ea0);
	}

	/* now check cast against value, INCOMPLETE */

	tt = t;

	if (this->u2.e1 == dummy){
		error("E missing for cast", ea0, ea0, ea0, ea0);
		this->u1.tp = (Ptype)any_type;
		return this;
	}

	pmf = 0;
	ee = this->u2.e1;
	switch (ee->base){
	case ADDROF:
		ee = ee->u3.e2;
		switch (ee->base){
		case NAME:	goto nm;
		case REF:	goto rf;
		}
		break;

	case NAME:
	nm:
		if (((Pname)ee)->u5.n_qualifier) pmf = 1;
		break;

	case REF:
	rf:
		if (ee->u2.e1->base == THIS)bound = 1;
		break;
	}
	this->u2.e1 = expr_typ(this->u2.e1, tbl);
	b = bound;	/* distinguish between explicit and implicit THIS */
	bound = 0;
	pmf = pmf && this->u2.e1->base==CAST;
	etp = this->u2.e1->u1.tp;
	while (etp->base == TYPE)
		etp = ((Pbase)etp)->b_name->u1.tp;

	switch (etp->base){
	case COBJ:
	{	Pexpr x;

		x = try_to_coerce(tt, this->u2.e1, "cast", tbl);
		if (x) return x;
		break;
	}
	case VOID:
		if (tt->base == VOID){
			this->u1.tp = t;
			return this;
		}
		error("cast of void value", ea0, ea0, ea0, ea0);
	case ANY:
		this->u1.tp = (Ptype)any_type;
		return this;
	}

legloop:
	switch (tt->base){
	case TYPE:
		tt = ((Pbase)tt)->b_name->u1.tp;
		goto legloop;
	case PTR:
		switch (etp->base){
		case COBJ:
			error("cannot castCO toP", ea0, ea0, ea0, ea0);
			break;
		case FCT:
			this->u2.e1 = new_expr(0, G_ADDROF, 0, this->u2.e1);
			bound = b;
			this->u2.e1 = expr_typ(this->u2.e1, tbl);
			bound = 0;
			if (this->u2.e1->base == CAST)
				pmf = 1;
			else 
				break;
		case PTR:
			if (pmf){
		zaqq:
				switch (tt->base){
				case TYPE:
					tt = ((Pbase)tt)->b_name->u1.tp;
					goto zaqq;
				case PTR:
					if (((Pptr)tt)->memof)break;
				default:
					V1.u1.p = (char *)this->u2.e1->u1.tp;
					V2.u1.p = (char *)this->u4.tp2;
					V3.u1.p = (char *)this->u4.tp2;
					error("%t cast to%t(%t is not aP toM)",
						&V1, &V2, &V3, ea0);
				}
			}
		}
		break;

	case RPTR:		/*(x&)e: pretend e is an x */
		if ((this->u2.e1->base == G_CM
			|| this->u2.e1->base == CALL
			|| this->u2.e1->base == G_CALL
			|| lval(this->u2.e1, 0))
		&& tsizeof(((Pptr)tt)->typ) <= tsizeof(etp) ) {
			this->u2.e1 = address(this->u2.e1);	/* *(x*)&e */
			this->u1.tp = t;
			return contents(this);
		}
		else {
			V1.u1.p = (char *)etp;
			V2.u1.p = (char *)t;
			error("cannot cast%t to%t", &V1, &V2, ea0, ea0);
		}
		break;

	case COBJ:
		this->base = VALUE;	/*(x)e => x(e): construct an x from e */
		this->u2.e1 = new_expr(0, ELIST, this->u2.e1, 0);
		return expr_typ(this, tbl);

	case CHAR:
	case INT:
	case SHORT:
	case LONG:
	case FLOAT:
	case DOUBLE:
		switch (etp->base){
		case COBJ:
			V1.u1.i = (int)tt->base;
			error("cannot castCO to%k", &V1, ea0, ea0, ea0);
			break;
		}
		break;
	}

	this->u1.tp = t;
	return this;
}


Pexpr dovalue(this, tbl)
Pexpr this;
Ptable tbl;
{
	Ptype tt;
	Pclass cl;
	Pname cn, ctor;
	Pexpr ee, a;
	int tv;

	tt = this->u4.tp2;
	type_dcl(tt, tbl);
vv:
	switch (tt->base){
	case TYPE:
		tt = ((Pbase)tt)->b_name->u1.tp;
		goto vv;

	case EOBJ:
	default:
		if (this->u2.e1 == 0){
			V1.u1.p = (char *)tt;
			error("value missing in conversion to%t", &V1,ea0,ea0,ea0);
			this->u1.tp = (Ptype)any_type;
			return this;
		}
		this->base = CAST;
		this->u2.e1 = this->u2.e1->u2.e1;	/* strip ELIST */
		return expr_typ(this, tbl);

	case CLASS:
		cl = (Pclass)tt;
		break;

	case COBJ:
		cn = ((Pbase)tt)->b_name;
		cl = (Pclass)cn->u1.tp;
	}

	if (this->u2.e1 && this->u2.e1->u3.e2 == 0) {	/* single argument */
		Pname acn;

		this->u2.e1->u2.e1 = expr_typ(this->u2.e1->u2.e1, tbl);
		if (tt->base == COBJ){
			Pexpr x;

			x=try_to_coerce(tt,this->u2.e1->u2.e1,"type conversion",tbl);
			if (x)return x;
		}
		acn = is_cl_obj(this->u2.e1->u2.e1->u1.tp);
		if (acn && acn->u1.tp == (Ptype)cl && has_itor(cl) == 0) {
			if (this->u3.e2){	/* x(x_obj) => e2=x_obj */
				Pexpr ee;

				this->base = ASSIGN;
				ee = this->u2.e1->u2.e1;
				this->u2.e1 = this->u3.e2;
				this->u3.e2 = ee;
				this->u1.tp = this->u4.tp2;
				return this;
			}
			return this->u2.e1->u2.e1;	/* x(x_obj) => x_obj */
		}
	}

	/* x(a) => obj.ctor(a); where e1==obj */
	ctor = has_ctor(cl);
	if (ctor == 0){
		V1.u1.p = (char *)cn;
		error("cannot make a%n", &V1, ea0, ea0, ea0);
		this->base = SM;
		this->u2.e1 = dummy;
		this->u3.e2 = 0;
		return this;
	}

	tv = 0;
	if (this->u3.e2 == 0){	/* x(a) => x temp;(temp.x(a),temp) */
		Pname n;

		n = make_tmp('V', this->u4.tp2, tbl);
		assign(n);
		if (tbl == gtbl) name_dcl_print(n, 0);
		this->u3.e2 = (Pexpr)n;
		ee = new_expr(0, G_CM, this,(Pexpr)n);
		tv = 1;
	}
	else 
		ee = this;

	a = this->u2.e1;
	this->base = G_CALL;
	this->u2.e1 = new_ref(DOT, this->u3.e2, ctor);
	this->u3.e2 = a;
	ee = expr_typ(ee, tbl);
	if (tv == 0){	/* deref value returned by constructor */
		ee = new_expr(0, DEREF, ee, 0);
		ee->u1.tp = ee->u2.e1->u1.tp;
	}
	return ee;
}


Pexpr donew(this, tbl)
Pexpr this;
Ptable tbl;
{
	Ptype tt, tx;
	bit v, old;

	tt = this->u4.tp2;
	tx = tt;
	v = 0;
	old = new_type;
	new_type = 1;
	type_dcl(tt, tbl);
	new_type = old;
	if (this->u2.e1) this->u2.e1 = expr_typ(this->u2.e1, tbl);
ll:
	switch (tt->base){
	default:
		if (this->u2.e1){
			error("Ir for nonCO created using \"new\"",
				ea0, ea0, ea0, ea0);
			this->u2.e1 = 0;
		}
		break;
	case VEC:
		v = 1;
		tt = ((Pvec)tt)->typ;
		goto ll;
	case TYPE:
		tt = ((Pbase)tt)->b_name->u1.tp;
		goto ll;
	case COBJ:
	{
		Pname cn;
		Pclass cl;

		cn = ((Pbase)tt)->b_name;
		cl = (Pclass)cn->u1.tp;

		if ((cl->defined & (DEFINED|SIMPLIFIED))== 0){
			V1.u1.p = (char *)cn;
			V2.u1.p = (char *)cn;
			error("new%n;%n isU", &V1, &V2, ea0, ea0);
		}
		else {
			Pname ctor;
			TOK su;

			ctor = has_ctor(cl);
			if (ctor){
				if (v){
					Pname ic;
					if (this->u2.e1)
						errorFIPC('s',
					"Ir for vector ofCO created using \"new\"",
				    		ea0, ea0, ea0, ea0);
					else if ((ic = has_ictor(cl))== 0) {
						V1.u1.p = (char *)cn;
						error("vector ofC%n that does not have aK taking noAs",
							&V1, ea0, ea0, ea0);
					} else if (((Pfct)ic->u1.tp)->nargs) {
						V1.u1.p = (char *)cn;
						errorFIPC('s',
						"defaultAs forK for vector ofC%n",
							&V1, ea0, ea0, ea0);
					}
				}

				if (cc->cot != cl)
					visible_check(ctor,ctor->u2.string,this,0);
				this->u2.e1 = new_call(ctor, this->u2.e1);
				this->u2.e1 = expr_typ(this->u2.e1, tbl);
			}
			else if (su = is_simple(cl)) {
				if (this->u2.e1){
					V1.u1.p = (char *)cn;
					error("new%nWIr", &V1, ea0, ea0, ea0);
				}
			}
		}
	}
	}

	this->u1.tp = (v) ?(Ptype)tx:(Ptype)new_ptr(PTR, tx, 0);
	return this;
}

