/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/expr.c	1.2"
/***************************************************************************

expr.c:

	type check expressions

************************************************************************/

#include "cfront.h"
#include "size.h"

#define nppromote(b)	t=np_promote(b,r1,r2,t1,t2,1)
#define npcheck(b)	(void)np_promote(b,r1,r2,t1,t2,0)

int const_save = 0;

Pexpr address(this)
Pexpr this;
{
	register Pexpr ee;

	switch (this->base){
	case DEREF:
		if (this->u3.e2 == 0) return this->u2.e1;	/* &*e  => e */
		break;
	case CM:
	case G_CM:
		this->u3.e2 = address(this->u3.e2);	/* &(e1,e2) =>(e1, &e2) */
		return this;
	case CALL:
	case G_CALL:
	{
		Pname fn;
		Pfct f;

		fn = this->u4.fct_name;
		f = (fn) ?(Pfct)fn->u1.tp: 0;
		if (f && f->f_inline && fn->n_used > 1)
			return this;
		break;
	}
	case NAME:
		if (((Pname)this)->n_stclass == REGISTER) {
			V1.u1.p =(char *)this;
			error("& register%n", &V1, ea0, ea0, ea0);
		}
		take_addr((Pname)this);
	}

	ee = new_expr(0, G_ADDROF, 0, this);
	if (this->u1.tp){
		ee->u1.tp = (Ptype)addrof(this->u1.tp);
		switch (this->u1.tp->base){
		case PTR:
			((Pptr)ee->u1.tp)->memof = ((Pptr)this->u1.tp)->memof;
			break;
		case FCT:
			((Pptr)ee->u1.tp)->memof = ((Pfct)this->u1.tp)->memof;
			break;
		case OVERLOAD:
			((Pptr)ee->u1.tp)->memof =
			((Pfct)((Pgen)this->u1.tp)->fct_list->f->u1.tp)->memof;
		}
	}

	return ee;
}

Pexpr contents(this)
Pexpr this;
{
	register Pexpr ee;

	if (this->base == ADDROF || this->base == G_ADDROF)
		return this->u3.e2;		/* *& */
	ee = new_expr(0, DEREF, this, 0);
	if (this->u1.tp)
		ee->u1.tp = ((Pptr)this->u1.tp)->typ;	/* tp==0 ??? */
	return ee;
}

int bound = 0;
int chars_in_largest = 0;	/* no of characters in largest int */

/*
 *	find the type of "this" and place it in tp;
 *	return the typechecked version of the expression:
 *	"tbl" provides the scope for the names in "this"
 */
Pexpr expr_typ(this, tbl)
Pexpr this;
Ptable tbl;
{
	Pname n;
	Ptype t, t1, t2;
	TOK b, r1, r2;

	t = 0;
	b = this->base;
	if (tbl->base != TABLE){
		V1.u1.i = (int)tbl->base;
		errorFIPC('i', "expr::typ(%d)", &V1, ea0, ea0, ea0);
	}
	if (this->u1.tp){
		if (b == NAME) use((Pname)this);
		return this;
	}

	switch (b){		/* is it a basic type */
	case DUMMY:
		error("emptyE", ea0, ea0, ea0, ea0);
		this->u1.tp = (Ptype)any_type;
		return this;

	case ZERO:
		this->u1.tp = (Ptype)zero_type;
		return this;

	case IVAL:
		this->u1.tp = (Ptype)int_type;
		return this;

	case FVAL:
		this->u1.tp = (Ptype)float_type;
		return this;

	case ICON:
	{	int ll;

		/*	is it long?
		 *	explicit long?
		 *	decimal larger than largest signed int
		 *	octal or hexadecimal larger than largest unsigned int
		 */
		ll = strlen(this->u2.string);
		switch (this->u2.string[ll-1]){
		case 'l':
		case 'L':
			switch (this->u2.string[ll-2]){
			case 'u':
			case 'U':
				this->u2.string[ll-2] = 0;
				this = new_texpr(CAST, ulong_type, this);
				return expr_typ(this, tbl);
			}
		lng:
			this->u1.tp = (Ptype)long_type;
			goto save;
		case 'u':
		case 'U':			/* 1u => unsigned(1) */
			switch (this->u2.string[ll-2]) {
			case 'l':
			case 'L':
				this->u2.string[ll-2] = 0;
				t = (Ptype)ulong_type;
				break;
			default:
				this->u2.string[ll-1] = 0;
				t = (Ptype)uint_type;
			}
			this = new_texpr(CAST, t, this);
			return expr_typ(this, tbl);
		}

		if (this->u2.string[0] == '0'){	/* assume 8 bits in byte */
			switch (this->u2.string[1]) {
			case 'x':
			case 'X':
				if (SZ_INT+SZ_INT < ll-2) goto lng;
				goto nrm;
			default:
				if (BI_IN_BYTE*SZ_INT <(ll-1)*3) goto lng;
				goto nrm;
			}
		}
		else {
			char *p, *q;

			if (ll < chars_in_largest){
		nrm:
				this->u1.tp = (Ptype)int_type;
				goto save;
			}
			if (ll > chars_in_largest)goto lng;
			p = this->u2.string;
			q = LARGEST_INT;
			do 
				if (*p++>*q++) goto lng;
			while (*p);
		}

		goto nrm;
	}
	case CCON:
		this->u1.tp = (Ptype)char_type;
		goto save;

	case FCON:
		this->u1.tp = (Ptype)double_type;
		goto save;

	case STRING:	/* type of "as\tdf" is char[6] */
			/* c_strlen counts the terminating '\0' */
	{
		Pvec v;

		v = new_vec(char_type, 0);
		v->size = c_strlen(this->u2.string);
		this->u1.tp = (Ptype)v;
	}
	save:
		if (const_save){
			char *p;

			p = (char *) new(strlen(this->u2.string)+1);
			strcpy(p, this->u2.string);
			this->u2.string = p;
		}

		return this;

	case THIS:
		expr__dtor(this);
		if (cc->tot){
			use(cc->c_this);
			return (Pexpr)cc->c_this;
		}
		error("this used in nonC context", ea0, ea0, ea0, ea0);
		n = new_name("this");
		n->u1.tp = (Ptype)any_type;
		return (Pexpr)insert(tbl, n, 0);

	case NAME:
	{	Pexpr ee;

		ee = find_name(tbl,(Pname)this, 0);
		if (ee->u1.tp->base == RPTR) return contents(ee);
		if (ee->base == NAME &&((Pname)ee)->n_xref) {
			/* fudge to handle X(X&) args */
			ee = new_expr(0, DEREF, ee, 0);
			ee->u1.tp = ee->u2.e1->u1.tp;
		}

		return ee;
	}

	case ADDROF:
	case G_ADDROF:	/* handle lookup for &s::i */
		if (this->u3.e2->base == NAME)
			this->u3.e2 = find_name(tbl,(Pname)this->u3.e2, 3);
		if (this->u3.e2->base == NAME &&((Pname)this->u3.e2)->n_xref){
			/* fudge to handle X(X&) args */
			this->u3.e2 = new_expr(0, DEREF, this->u3.e2, 0);
			this->u3.e2->u1.tp = this->u3.e2->u2.e1->u1.tp;
		}
		break;

	case SIZEOF:
		if (this->u4.tp2){
			type_dcl(this->u4.tp2, tbl);
			if (this->u2.e1 && this->u2.e1 != dummy) {
				this->u2.e1 = expr_typ(this->u2.e1, tbl);
				if (TO_DEL(this->u2.e1)) expr_del(this->u2.e1);
				this->u2.e1 = dummy;
			}
		}
		else if (this->u2.e1 == dummy){
			error("sizeof emptyE", ea0, ea0, ea0, ea0);
			this->u1.tp = (Ptype)any_type;
			return this;
		}
		else {
			this->u2.e1 = expr_typ(this->u2.e1, tbl);
			this->u4.tp2 = this->u2.e1->u1.tp;
		}
		this->u1.tp = (Ptype)int_type;
		return this;

	case CAST:
		return docast(this, tbl);

	case VALUE:
		return dovalue(this, tbl);

	case NEW:
		return donew(this, tbl);

	case DELETE:	/* delete e1 OR delete[e2] e1 */
	{	int i;
		if (this->u2.e1->base == ADDROF)
			errorFIPC('w', "delete &E", ea0, ea0, ea0, ea0);
		this->u2.e1 = expr_typ(this->u2.e1, tbl);
		i = num_ptr(this->u2.e1->u1.tp, DELETE);
		if (i != P) error("nonP deleted", ea0, ea0, ea0, ea0);
		if (this->u3.e2){
			this->u3.e2 = expr_typ(this->u3.e2, tbl);
			integral(this->u3.e2->u1.tp, DELETE);
		}
		this->u1.tp = (Ptype)void_type;
		return this;
	}

	case ILIST:	/* an ILIST is pointer to an ELIST */
		this->u2.e1 = expr_typ(this->u2.e1, tbl);
		this->u1.tp = (Ptype)any_type;
		return this;

	case ELIST:
	{	Pexpr e, ex;

		if (this->u2.e1 == dummy && this->u3.e2 == 0) {
			error("emptyIrL", ea0, ea0, ea0, ea0);
			this->u1.tp = (Ptype)any_type;
			return this;
		}

		for(e = this ;e ;e = ex) {
			Pexpr ee;

			ee = e->u2.e1;
			if (e->base != ELIST){
				V1.u1.i = (int)e->base;
				errorFIPC('i', "elist%k", &V1, ea0, ea0, ea0);
			}
			if (ex = e->u3.e2){	/* look ahead for end of list */
				if (ee == dummy)
					error("EX in EL", ea0, ea0, ea0, ea0);
				if (ex->u2.e1 == dummy && ex->u3.e2 == 0) {
					/* { ... , } */
					if (TO_DEL(ex)) expr_del(ex);
					e->u3.e2 = ex = 0;
				}
			}
			e->u2.e1 = expr_typ(ee, tbl);
			t = e->u2.e1->u1.tp;
		}
		this->u1.tp = t;
		return this;
	}

	case DOT:
	case REF:
		if (this->u3.e2){
			/* a .* p => &a MEMPTR p => appropriate indirection */
			Pexpr a, p;
			Ptype pt, tx;
			Pclass pm, mm;
			Pname cn;

			a = expr_typ(this->u2.e1, tbl);
			if (this->base == DOT) a = address(a);
			p = expr_typ(this->u3.e2, tbl);
			pt = p->u1.tp;

			while (pt->base == TYPE)
				pt = ((Pbase)pt)->b_name->u1.tp;
			if (pt->base != PTR ||((Pptr)pt)->memof == 0) {
				V1.u1.p = (char *)pt;
				error("P toMFX in .*E: %t", &V1, ea0, ea0, ea0);
				this->u1.tp = (Ptype)any_type;
				return this;
			}
			pm = ((Pptr)pt)->memof;
			cn = is_cl_obj(((Pptr)a->u1.tp)->typ);
			mm = (cn) ?((Pclass)cn->u1.tp) : 0;
			if (mm != pm && baseofclass(pm, mm) == 0) {
				V1.u1.p = (char *)a->u1.tp;
				V2.u1.p = (char *)pm->string;
				error("badOT in .*E: %t(%s*X)", &V1,&V2,ea0,ea0);
					this->u1.tp = (Ptype)any_type;
					return this;
			
			}
			tx = ((Pptr)pt)->typ;
			while (tx->base == TYPE)
				tx = ((Pbase)tx)->b_name->u1.tp;
			if (tx->base == FCT){	/* a.*p =>(&a MEMPTR p) */
				this->base = MEMPTR;
				this->u4.tp2 = (Ptype)mm;
				/* keep the class for simpl.c */
				this->u2.e1 = a;
				this->u3.e2 = p;
			}
			else {	/* a .* p => *(typeof(p))((char*)&a +(int)p-1) */
				Pexpr pl;

				a = new_texpr(CAST, Pchar_type, a);
				a->u1.tp = Pchar_type;
				p = new_texpr(CAST, int_type, p);
				p->u1.tp = (Ptype)int_type;
				p = new_expr(0, MINUS, p, one);
				p->u1.tp = (Ptype)int_type;
				pl = new_expr(0, PLUS, a, p);
				pl->u1.tp = Pint_type;
				this->base = DEREF;
				this->u2.e1 = new_texpr(CAST, pt, pl);
				this->u2.e1->u1.tp = pt;
				this->u3.e2 = 0;
			}
			this->u1.tp = tx;
			if (this->u1.tp->base == RPTR) return contents(this);
			return this;
		}
		else {
			Pbase b;
			Ptable atbl;
			Pname nn;
			char *s;
			Pclass cl;

			this->u2.e1 = expr_typ(this->u2.e1, tbl);
			t = this->u2.e1->u1.tp;
			if (this->base == REF){
		xxx:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto xxx;
				default:
					V1.u1.p = (char *)this->u4.mem;
					error("nonP->%n", &V1, ea0, ea0, ea0);
					t = (Ptype)any_type;
				case ANY:
					goto qqq;
				case PTR:
				case VEC:
					b = (Pbase)((Pptr)t)->typ;
					break;
				}
			}
			else {
		qqq:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto qqq;
				default:
					V1.u1.p = (char *)this->u4.mem;
					error("nonO .%n", &V1, ea0, ea0, ea0);
					t = (Ptype)any_type;
				case ANY:
				case COBJ:
					break;
				}

				/* FUDGE, but cannot use lval(consts) */
				switch (this->u2.e1->base){
				case CM:	/*( ... , x). =>( ... , &x)-> */
				{
					Pexpr ex;

					ex = this->u2.e1;
			cfr:
					switch (ex->u3.e2->base){
					case NAME:
						this->base = REF;
						ex->u3.e2 = address(ex->u3.e2);
						this->u2.e1->u1.tp = 
							t = ex->u3.e2->u1.tp;
						goto xxx;
					case CM:
						ex = ex->u3.e2;
						goto cfr;
					}
					break;
				}
				case CALL:
				case G_CALL:	/* f(). =>(tmp=f(),&tmp)-> */
				{	Pname tmp;
					Pexpr aa;

					tmp = make_tmp('T',this->u2.e1->u1.tp,tbl);
					this->u2.e1 = init_tmp(tmp,this->u2.e1,tbl);
					aa = address((Pexpr)tmp);
					this->u2.e1=new_expr(0,CM,this->u2.e1,aa);
					this->u2.e1->u1.tp = aa->u1.tp;
					this->base = REF;
					break;
				}
			}
			b = (Pbase)t;
		}

	xxxx:
		switch (b->base){
		case TYPE:
			b = (Pbase)b->b_name->u1.tp;
			goto xxxx;
		default:
			V1.u1.p = (char *)this->u2.e1->u1.tp;
			V2.u1.i = (int)this->base;
			V3.u1.p = (char *)this->u4.mem;
			V4.u1.p = (char *)this->u4.mem;
			error("(%t) before %k%n(%n not aM)",&V1,&V2,&V3,&V4);
		case ANY:
			atbl = any_tbl;
			break;
		case COBJ:
			if (atbl = b->b_table)break;
			s = b->b_name->u2.string; /* lookup the class name */
			if (s == 0){
				V1.u1.i = CLASS;
				errorFIPC('i', "%kN missing", &V1, ea0, ea0, ea0);
			}
			nn = look(tbl, s, CLASS);
			if (nn == 0){
				V1.u1.i = CLASS;
				V2.u1.p = (char *)s;
				errorFIPC('i', "%k %sU", &V1, &V2, ea0, ea0);
			}
			if (nn != b->b_name) b->b_name = nn;
			cl = (Pclass)nn->u1.tp;
			PERM(cl);
			if (cl == 0){
				V1.u1.i = CLASS;
				V2.u1.p = (char *)s;
				errorFIPC('i', "%k %s'sT missing", &V1,&V2,ea0,ea0);
			}
			b->b_table = atbl = cl->memtbl;
		}

		nn = (Pname)find_name(atbl, this->u4.mem, 2);

		if (nn->n_stclass == 0){
			this->u4.mem = nn;
			this->u1.tp = nn->u1.tp;
			if (this->u1.tp->base == RPTR) return contents(this);
			return this;
		}
		if (nn->u1.tp->base == RPTR) return contents(this);
		return (Pexpr)nn;
	}

	case CALL:	/* handle undefined function names */
		if (this->u2.e1->base == NAME && this->u2.e1->u1.tp == 0) {
			this->u2.e1=find_name(tbl,(Pname)this->u2.e1,1);
		}
		if (this->u2.e1->base == NAME &&((Pname)this->u2.e1)->n_xref) {
			/* fudge to handle X(X&) args */
			this->u2.e1 = new_expr(0, DEREF, this->u2.e1, 0);
			this->u2.e1->u1.tp = this->u2.e1->u2.e1->u1.tp;
		}
		break;

	case QUEST:
		this->u4.cond = expr_typ(this->u4.cond, tbl);
	}

	if (this->u2.e1){
		this->u2.e1 = expr_typ(this->u2.e1, tbl);
		if (this->u2.e1->u1.tp->base == RPTR)
			this->u2.e1 = contents(this->u2.e1);
		t1 = this->u2.e1->u1.tp;
	}
	else 
		t1 = 0;

	if (this->u3.e2){
		this->u3.e2 = expr_typ(this->u3.e2, tbl);
		if (this->u3.e2->u1.tp->base == RPTR)
			this->u3.e2 = contents(this->u3.e2);
		t2 = this->u3.e2->u1.tp;
	}
	else 
		t2 = 0;

	switch (b){	/* filter out non-overloadable operators */
	default:
	{
		Pexpr x;

		x = try_to_overload(this, tbl);
		if (x)return x;
	}
	case CM:
	case G_CM:
	case QUEST:
	case G_ADDROF:
	case G_CALL:
		break;
	}

	t = (t1==0)? t2:(t2==0)? t1: 0;

	switch (b){		/* are the operands of legal types */
	case G_CALL:
	case CALL:
		/* two calls of use() for e1's names */
		this->u1.tp = fct_call(this, tbl);
		if (this->u1.tp->base == RPTR)return contents(this);
		return this;

	case DEREF:
		if (this->u2.e1 == dummy)
			error("O missing before []\n", ea0, ea0, ea0, ea0);
		if (this->u3.e2 == dummy)
			error("subscriptE missing", ea0, ea0, ea0, ea0);
		if (t){		/* *t */
			while (t->base == TYPE) t = ((Pbase)t)->b_name->u1.tp;
			vec_type(t);
			if (((Pptr)t)->memof)
				error("P toM deRd", ea0, ea0, ea0, ea0);
			this->u1.tp = type_deref(t);
		}
		else {				/* e1[e2] that is *(e1+e2) */
			if (vec_type(t1)){	/* e1[e2] */
				switch (t2->base){
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
				case EOBJ:
					break;
				default:
				{
					Pname cn;

					cn = is_cl_obj(t2);
					if (cn)	/* conversion to integral? */
						this->u3.e2 =
						check_cond(this->u3.e2,DEREF,tbl);
					else 
						integral(t2, DEREF);
				}
				}
				while (t1->base == TYPE)
					t1 = ((Pbase)t1)->b_name->u1.tp;
				if (((Pptr)t1)->memof)
					error("P toM deRd", ea0, ea0, ea0, ea0);
				this->u1.tp = type_deref(t1);
			}
			else if (vec_type(t2)){	/* really e2[e1] */
				integral(t1, DEREF);
				while (t2->base == TYPE)
					t2 = ((Pbase)t2)->b_name->u1.tp;
				if (((Pptr)t2)->memof)
					error("P toM deRd", ea0, ea0, ea0, ea0);
				this->u1.tp = type_deref(t2);
			}
			else {
				V1.u1.p = (char *)t1;
				V2.u1.p = (char *)t2;
				error("[] applied to nonPT:%t[%t]", &V1, &V2,
					ea0, ea0);
				this->u1.tp = (Ptype)any_type;
			}
		}
		if (this->u1.tp->base == RPTR)return contents(this);
		return this;

	case G_ADDROF:
	case ADDROF:
		if (lval(this->u3.e2, b) == 0){
			this->u1.tp = (Ptype)any_type;
			return this;
		}
		this->u1.tp = (Ptype)addrof(t);
		if (tconst(t) && vec_const == 0 && fct_const == 0)
			((Pptr)this->u1.tp)->rdo = 1;
		switch (this->u3.e2->base){
		case NAME:
		mname:					/* check for &s::i */
		{	Pname n2, cn;

			n2 = (Pname)this->u3.e2;
			cn = (n2->u4.n_table != gtbl)? n2->u4.n_table->t_name: 0;
			if (cn == 0)break;
			((Pptr)this->u1.tp)->memof = (Pclass)cn->u1.tp;
			if (t->base == FCT){
				Pfct f;

				lval((Pexpr)n2, ADDROF); /* ``outline'' inlines */
				f = (Pfct)t;
				if (f->f_virtual)	/* vtbl index + 1 */
					this->u2.e1 = new_ival(f->f_virtual);
				else 			/* use the pointer */
					break;
			}
			else {
				if (n2->n_stclass != STATIC) /* offset + 1 */
					this->u2.e1 = new_ival(n2->n_offset+1);
				else {				/* can't do it */
					this->u1.tp =
						(Ptype)new_ptr(PTR,this->u3.e2->u1.tp,0);
					return this;
				}
			}
			this->u2.e1->u1.tp = (Ptype)int_type;
			this->u3.e2 = 0;
			this->u4.tp2 = this->u1.tp;
			this->base = CAST;
			return this;
		}
		case DOT:
		case REF:
		{
			Pname m;
			Pfct f;

			m = this->u3.e2->u4.mem;
			f = (Pfct)m->u1.tp;
			if (f->base == FCT){
				if (bound == 0
				&& this->u3.e2->u2.e1 == (Pexpr)cc->c_this
				&& m->u5.n_qualifier) {
					/* FUDGE: &this->x::f => &x::f */
					if (TO_DEL(this->u3.e2))
						expr_del(this->u3.e2);
					this->u3.e2 = (Pexpr)m;
					goto mname;
				}
				V1.u1.p = (char *)m->u4.n_table->t_name->u2.string;
				V2.u1.p = (char *)m->u4.n_table->t_name->u2.string;
				V3.u1.p = (char *)m->u2.string;
				errorFIPC('w',
				"address of boundF(try %s::*PT and &%s::%s address)"
					, &V1, &V2, &V3, ea0);
				if (f->f_virtual == 0 || m->u5.n_qualifier){
					/* & x.f  = & f */
					if (TO_DEL(this->u3.e2))
						expr_del(this->u3.e2);
					this->u3.e2 = (Pexpr)m;
				}
			}
		}
		}
		return this;

	case UMINUS:
		numeric(t, UMINUS);
		this->u1.tp = t;
		return this;

	case UPLUS:
		num_ptr(t, UPLUS);
		errorFIPC('s', "unary +(ignored)", ea0, ea0, ea0, ea0);
		this->u1.tp = t;
		this->base = PLUS;
		this->u2.e1 = zero;
		return this;

	case NOT:
		this->u3.e2 = check_cond(this->u3.e2, NOT, tbl);
		this->u1.tp = (Ptype)int_type;
		return this;

	case COMPL:
		integral(t, COMPL);
		this->u1.tp = t;
		return this;

	case INCR:
	case DECR:
		if (this->u2.e1) lval(this->u2.e1, b);
		if (this->u3.e2) lval(this->u3.e2, b);
		r1 = num_ptr(t, b);
		this->u1.tp = t;
		return this;
	}

	if (this->u2.e1==dummy || this->u3.e2==dummy
	|| this->u2.e1==0 || this->u3.e2==0) {
		V1.u1.i = (int)b;
		error("operand missing for%k", &V1, ea0, ea0, ea0);
	}
	switch (b){
	case MUL:
	case DIV:
		r1 = numeric(t1, b);
		r2 = numeric(t2, b);
		nppromote(b);
		break;

	case PLUS:
		r2 = num_ptr(t2, PLUS);
		r1 = num_ptr(t1, PLUS);
		if (r1 == P && r2 == P) error("P +P", ea0, ea0, ea0, ea0);
		nppromote(PLUS);
		this->u1.tp = t;
		break;

	case MINUS:
		r2 = num_ptr(t2, MINUS);
		r1 = num_ptr(t1, MINUS);
		if (r2 == P && r1 != P && r1 != A)
			error("P - nonP", ea0, ea0, ea0, ea0);
		nppromote(MINUS);
		this->u1.tp = t;
		break;

	case LS:
	case RS:
	case AND:
	case OR:
	case ER:
		switch (this->u2.e1->base){
		case LT:
		case LE:
		case GT:
		case GE:
		case EQ:
		case NE:
			V1.u1.i = (int)this->u2.e1->base;
			V2.u1.i = (int)b;
			errorFIPC('w', "%kE as operand for%k", &V1, &V2, ea0, ea0);
		}
		switch (this->u3.e2->base){
		case LT:
		case LE:
		case GT:
		case GE:
		case EQ:
		case NE:
			V1.u1.i = (int)this->u3.e2->base;
			V2.u1.i = (int)b;
			errorFIPC('w', "%kE as operand for%k", &V1, &V2, ea0, ea0);
		}
	case MOD:
		r1 = integral(t1, b);
		r2 = integral(t2, b);
		nppromote(b);
		break;

	case LT:
	case LE:
	case GT:
	case GE:
	case EQ:
	case NE:
		r1 = num_ptr(t1, b);
		r2 = num_ptr(t2, b);
		npcheck(b);
		t = (Ptype)int_type;
		break;

	case ANDAND:
	case OROR:
		this->u2.e1 = check_cond(this->u2.e1, b, tbl);
		this->u3.e2 = check_cond(this->u3.e2, b, tbl);
		t = (Ptype)int_type;
		break;

	case QUEST:
		{
		Pname c1, c2;
		this->u4.cond = check_cond(this->u4.cond, b, tbl);
		/* still doesn't do complete checking for possible conversions... */
		if (t1 == t2
		||(	(c1=is_cl_obj(t1))
			&&(c2=is_cl_obj(t2))
			&&(c1->u1.tp==c2->u1.tp)
		))
			t = t1;
		else {
			r1 = num_ptr(t1, b);
			r2 = num_ptr(t2, b);

			if (r1==FCT && r2==FCT) {	/* fudge */
				if (type_check(t1, t2, ASSIGN)) {
					V1.u1.p = (char *)t1;
					V2.u1.p = (char *)t2;
					error("badTs in ?:E: %t and %t",
						&V1, &V2, ea0, ea0);
				}
				t = t1;
			}
			else 
				nppromote(b);

			if (t != t1 && type_check(t, t1, 0)) {
				this->u2.e1 = new_texpr(CAST, t, this->u2.e1);
				this->u2.e1->u1.tp = t;
			}
			if (t != t2 && type_check(t, t2, 0)) {
				this->u3.e2 = new_texpr(CAST, t, this->u3.e2);
				this->u3.e2->u1.tp = t;
			}
		}
		}
		break;

	case ASPLUS:
		r1 = num_ptr(t1, ASPLUS);
		r2 = num_ptr(t2, ASPLUS);
		if (r1 == P && r2 == P) error("P +=P", ea0, ea0, ea0, ea0);
		nppromote(ASPLUS);
		goto ass;

	case ASMINUS:
		r1 = num_ptr(t1, ASMINUS);
		r2 = num_ptr(t2, ASMINUS);
		if (r2==P && r1!=P && r1!=A) error("P -= nonP", ea0,ea0,ea0,ea0);
		nppromote(ASMINUS);
		goto ass;

	case ASMUL:
	case ASDIV:
		r1 = numeric(t1, b);
		r2 = numeric(t2, b);
		nppromote(b);
		goto ass;

	case ASMOD:
		r1 = integral(t1, ASMOD);
		r2 = integral(t2, ASMOD);
		nppromote(ASMOD);
		goto ass;

	case ASAND:
	case ASOR:
	case ASER:
	case ASLS:
	case ASRS:
		r1 = integral(t1, b);
		r2 = integral(t2, b);
		npcheck(b);
		t = (Ptype)int_type;
		goto ass;
	ass:
		this->u4.as_type = t;	/* the type of the rhs */
		t2 = t;

	case ASSIGN:
		if (lval(this->u2.e1, b) == 0){
			this->u1.tp = (Ptype)any_type;
			return this;
		}
	lkj:
		switch (t1->base){
		case TYPE:
			t1 = ((Pbase)t1)->b_name->u1.tp;
			goto lkj;
		case INT:
		case CHAR:
		case SHORT:
			if (this->u3.e2->base==ICON
			&& this->u3.e2->u1.tp==(Ptype)long_type)
			{
				V1.u1.i = (int)t1->base;
				errorFIPC('w', "long constant assigned to%k",
					&V1, ea0, ea0, ea0);
			}
		case LONG:
			if (b == ASSIGN
			&&((Pbase)t1)->b_unsigned
			&& this->u3.e2->base == UMINUS
			&& this->u3.e2->u3.e2->base == ICON)
				errorFIPC('w', "negative assigned to unsigned",
					ea0, ea0, ea0, ea0);
			break;
		case PTR:
			if (b == ASSIGN){
				this->u3.e2 = ptr_init((Pptr)t1,this->u3.e2,tbl);
				t2 = this->u3.e2->u1.tp;
			}
			break;
		case COBJ:
		{	Pname c1;

			c1 = is_cl_obj(t1);
			if (c1){
				Pname c2;

				c2 = is_cl_obj(t2);
				if (c1 != c2){
					this->u3.e2 =
						new_expr(0,ELIST,this->u3.e2,0);
					this->u3.e2 =
						new_texpr(VALUE, t1, this->u3.e2);
					this->u3.e2->u3.e2 = this->u2.e1;
					this->u3.e2 = expr_typ(this->u3.e2, tbl);
					this->u1.tp = t1;
					return this;
				}
				else {	/* check for bitwise copy */
					Pclass cl;

					cl = (Pclass)c1->u1.tp;
					if (cl->bit_ass == 0) {
						V1.u1.p = (char *)cl->string;
						errorFIPC('s',
					"bitwise copy: %s has aMW operator=()",
							&V1, ea0, ea0, ea0);
					} else if (cl->itor && has_dtor(cl)) {
						V1.u1.p = cl->string;
						V2.u1.p = cl->string;
						V3.u1.p = cl->string;
						errorFIPC('w',
		"bitwise copy: %s has destructor and %s(%s&) but not assignment",
							&V1,&V2,&V3,ea0);
					}
				}
			}
			break;
		}
		}

		{	Pexpr x;
			x = try_to_coerce(t1, this->u3.e2, "assignment", tbl);
			if (x)
				this->u3.e2 = x;
			else if (type_check(this->u2.e1->u1.tp, t2, ASSIGN)) {
				V1.u1.p = (char *)this->u2.e1->u1.tp;
				V2.u1.p = (char *)t2;
				error("bad assignmentT:%t =%t", &V1,&V2,ea0,ea0);
			}
		}
		t = this->u2.e1->u1.tp;		/* the type of the lhs */
		break;
	case CM:
	case G_CM:
		t = t2;
		break;
	default:
		V1.u1.i = (int)b;
		errorFIPC('i', "unknown operator%k", &V1, ea0, ea0, ea0);
	}

	this->u1.tp = t;
	return this;
}

