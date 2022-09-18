/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/typ.c	1.1"

#include "cfront.h"
#include "size.h"

Pbase short_type;
Pbase int_type;
Pbase char_type;
Pbase long_type;

Pbase uchar_type;
Pbase ushort_type;
Pbase uint_type;
Pbase ulong_type;

Pbase zero_type;
Pbase float_type;
Pbase double_type;
Pbase void_type;
Pbase any_type;

Ptype Pint_type;
Ptype Pchar_type;
Ptype Pvoid_type;
Ptype Pfctchar_type;
Ptype Pfctvec_type;

Ptable gtbl;
Ptable any_tbl;

Pname Cdcl = 0;
Pstmt Cstmt = 0;

extern int suppress_error;

bit new_type = 0;

extern Ptype np_promote();

/*
 *	an arithmetic operator "oper" is applied to "t1" and "t2",
 *	types t1 and t2 has been checked and belongs to catagories
 *	"r1" and "r2", respectively:
 *		A	ANY
 *		Z	ZERO
 *		I	CHAR, SHORT, INT, LONG, FIELD, or EOBJ
 *		F	FLOAT DOUBLE
 *		P	PTR(to something) or VEC(of something)
 *	test for compatability of the operands,
 *	if (p) return the promoted result type
 */
Ptype np_promote(oper, r1, r2, t1, t2, p)
TOK oper;
TOK r1;
TOK r2;
Ptype t1;
Ptype t2;
TOK p;
{
	if (r2 == A) return t1;

	switch (r1){
	case A:
		return t2;
	case Z:
		switch (r2){
		case Z:	return (Ptype)int_type;
		case I:
		case F:	return (Ptype)(p ? arit_conv((Pbase)t2, 0) : 0);
		case P:	return t2;
		default:
			V1.u1.i = (int)r2;
			errorFIPC('i', "zero(%d)", &V1, ea0, ea0, ea0);
		}
	case I:
		switch (r2){
		case Z:
			t2 = 0;
			/* FALLTHRU */
		case I:
		case F:
			return (Ptype)(p ? arit_conv((Pbase)t1,(Pbase)t2) : 0);
		case P:
			switch (oper){
			case PLUS:
			case ASPLUS:
				break;
			default:
				V1.u1.i = (int)oper;
				error("int%kP", &V1, ea0, ea0, ea0);
				return (Ptype)any_type;
			}
			return t2;
		case FCT:
			V1.u1.i = (int)oper;
			error("int%kF", &V1, ea0, ea0, ea0);
			return (Ptype)any_type;
		default:
			V1.u1.i = (int)r2;
			errorFIPC('i', "int(%d)", &V1, ea0, ea0, ea0);
		}
	case F:
		switch (r2){
		case Z:
			t2 = 0;
			/* FALLTHRU */
		case I:
		case F:
			return (Ptype)(p ? arit_conv((Pbase)t1,(Pbase)t2) : 0);
		case P:
			V1.u1.i = (int)oper;
			error("float%kP", &V1, ea0, ea0, ea0);
			return (Ptype)any_type;
		case FCT:
			V1.u1.i = (int)oper;
			error("float%kF", &V1, ea0, ea0, ea0);
			return (Ptype)any_type;
		default:
			V1.u1.i = (int)r2;
			errorFIPC('i', "float(%d)", &V1, ea0, ea0, ea0);
		}
	case P:
		switch (r2){
		case Z:
			return t1;
		case I:
			switch (oper){
			case PLUS:
			case MINUS:
			case ASPLUS:
			case ASMINUS:
				break;
			default:
				V1.u1.i = (int)oper;
				error("P%k int", &V1, ea0, ea0, ea0);
				return (Ptype)any_type;
			}
			return t1;
		case F:
			V1.u1.i = (int)oper;
			error("P%k float", &V1, ea0, ea0, ea0);
			return (Ptype)any_type;
		case P:
			if (type_check(t1, t2, ASSIGN)){
				switch (oper){
				case EQ:
				case NE:
				case LE:
				case GE:
				case GT:
				case LT:
				case QUEST:
					if (type_check(t2, t1, ASSIGN) == 0)
						goto zz;
				}
				V1.u1.p = (char *)t1;
				V2.u1.i = (int)oper;
				V3.u1.p = (char *)t2;
				error("T mismatch:%t %k%t", &V1, &V2, &V3, ea0);
				return (Ptype)any_type;
			}
			zz:
			switch (oper){
			case MINUS:
			case ASMINUS:
				return (Ptype)int_type;
			case PLUS:
			case ASPLUS:
				error("P +P", ea0, ea0, ea0, ea0);
				return (Ptype)any_type;
			default:
				return t1;
			}
		case FCT:
			return t1;
		default:
			V1.u1.i = (int)r2;
			errorFIPC('i', "P(%d)", &V1, ea0, ea0, ea0);
		}
	case FCT:
		V1.u1.i = (int)oper;
		V2.u1.p = (char *)t2;
		error("F%k%t", &V1, &V2, ea0, ea0);
		return (Ptype)any_type;
	default:
		V1.u1.i = (int)r1;
		V2.u1.i = (int)r2;
		errorFIPC('i', "np_promote(%d,%d)", &V1, &V2, ea0, ea0);
	}
}

/*	v ==	I	integral
 *		N	numeric
 *		P	numeric or pointer
 */
TOK type_kind(this, oper, v)
Ptype this;
TOK oper;
TOK v;
{
	Ptype t;

	t = this;
xx:
	switch (t->base){
	case ANY:
		return A;
	case ZTYPE:
		return Z;
	case FIELD:
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
	case EOBJ:
		return I;
	case FLOAT:
	case DOUBLE:
		if (v == I){
			V1.u1.i = (int)oper;
			error("float operand for %k", &V1, ea0, ea0, ea0);
		}
		return F;
	case PTR:
		if (v != P){
			V1.u1.i = (int)oper;
			error("P operand for %k", &V1, ea0, ea0, ea0);
		}
		switch (oper){
		case INCR:
		case DECR:
		case MINUS:
		case PLUS:
		case ASMINUS:
		case ASPLUS:
			if (((Pptr)t)->memof ||((Pptr)t)->typ->base == FCT)
			{
				V1.u1.p = (char *)this;
				V2.u1.i = (int)oper;
				error("%t operand of%k", &V1, &V2, ea0, ea0);
			}
			else 	/* get increment */
				tsizeof(((Pptr)t)->typ );
			break;
		default:
			if (((Pptr)t)->memof ||((Pptr)t)->typ->base == FCT)
			{
				V1.u1.p = (char *)this;
				V2.u1.i = (int)oper;
				error("%t operand of%k", &V1, &V2, ea0, ea0);
			}
		case ANDAND:
		case OROR:
		case ASSIGN:
		case NE:
		case EQ:
		case IF:
		case WHILE:
		case DO:
		case FOR:
		case QUEST:
		case NOT:
			break;
		}
		return P;
	case RPTR:
		V1.u1.i = (int)oper;
		error("R operand for %k", &V1, ea0, ea0, ea0);
		return A;
	case VEC:
		if (v != P){
			V1.u1.i = (int)oper;
			error("V operand for %k", &V1, ea0, ea0, ea0);
		}
		return P;
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case FCT:
		if (v != P){
			V1.u1.i = (int)oper;
			error("F operand for %k", &V1, ea0, ea0, ea0);
		}
		return FCT;
	case CLASS:
	case ENUM:
		V1.u1.i = (int)this->base;
		V2.u1.i = (int)oper;
		error("%k operand for %k", &V1, &V2, ea0, ea0);
		return A;
	default:
		V1.u1.p = (char *)this;
		V2.u1.i = (int)oper;
		error("%t operand for %k", &V1, &V2, ea0, ea0);
		return A;
	}
}

/*
 *	go through the type(list) and
 *	(1) evaluate vector dimentions
 *	(2) evaluate field sizes
 *	(3) lookup struct tags, etc.
 *	(4) handle implicit tag declarations
 */
void type_dcl(this, tbl)
Ptype this;
Ptable tbl;
{
	Ptype t;
	Pptr p;


	t = this;
	if (this == 0)
		errorFIPC('i', "T::dcl(this==0)", ea0, ea0, ea0, ea0);
	if (tbl->base != TABLE){
		V1.u1.i = (int)tbl->base;
		errorFIPC('i', "T::dcl(%d)", &V1, ea0, ea0, ea0);
	}
xx:
	switch (t->base){
	case PTR:
	case RPTR:
		p = (Pptr)t;
		t = p->typ;
		if (t->base == TYPE){
			Ptype tt;

			tt = ((Pbase)t)->b_name->u1.tp;
			if (tt->base == FCT) p->typ = tt;
			return;
		}
		goto xx;

	case VEC:
	{
		Pvec v;
		Pexpr e;

		v = (Pvec)t;
		e = v->dim;
		if (e){
			Ptype et;
			v->dim = e = expr_typ(e, tbl);
			et = e->u1.tp;
			if (integral(et, 0) == A) {
				error("UN in array dimension", ea0, ea0, ea0, ea0);
			}
			else if (!new_type) {
				int i;
				Neval = 0;
				i = eval(e);
				if (Neval){
					V1.u1.p = (char *)Neval;
					error("%s", &V1, ea0, ea0, ea0);
				} else if (i == 0)
					errorFIPC('w', "array dimension == 0",
						ea0, ea0, ea0, ea0);
				else if (i < 0){
					error("negative array dimension",
						ea0, ea0, ea0, ea0);
					i = 1;
				}
				v->size = i;
				if (TO_DEL(v->dim))
					expr_del(v->dim);
				v->dim = 0;
			}
		}
		t = v->typ;
	llx:
		switch (t->base){
		case TYPE:
			t = ((Pbase)t)->b_name->u1.tp;
			goto llx;
		case FCT:
			v->typ = t;
			break;
		case VEC:
			if (((Pvec)t)->dim == 0 &&((Pvec)t)->size == 0)
				error("null dimension(something like [][] seen)",
					ea0, ea0, ea0, ea0);
		}
		goto xx;
	}

	case FCT:
	{
		Pfct f;
		Pname n, cn;

		f = (Pfct)t;
		for(n=f->argtype ;n ;n = n->n_list)
			type_dcl(n->u1.tp, tbl);

		cn = is_cl_obj(f->returns);
		if (cn && has_itor((Pclass)cn->u1.tp)) {
			Pname rv;

			rv = new_name("_result");
			rv->u1.tp = (Ptype)new_ptr(PTR, f->returns, 0);
			rv->n_scope = FCT;	/* not a "real" argument */
			rv->n_used = 1;
			rv->n_list = f->argtype;
			if (f->f_this)f->f_this->n_list = rv;
			f->f_result = rv;
		}
		t = f->returns;
		goto xx;
	}

	case FIELD:
	{
		Pbase f;
		Pexpr e;
		int i;
		Ptype et;

		f = (Pbase)t;
		e = (Pexpr)f->b_name;

		e = expr_typ(e, tbl);
		f->b_name = (Pname)e;
		et = e->u1.tp;
		if ( integral(et, 0) == A){
			error("UN in field size", ea0, ea0, ea0, ea0);
			i = 1;
		}
		else {
			Neval = 0;
			i = eval(e);
			if (Neval) {
				V1.u1.p = (char *)Neval;
				error("%s", &V1, ea0, ea0, ea0);
			} else if (i < 0){
				error("negative field size", ea0, ea0, ea0, ea0);
				i = 1;
			}
			else if (tsizeof(f->b_fieldtype)*BI_IN_BYTE < i)
			{
				V1.u1.p = (char *)f->b_fieldtype;
				error("field size > sizeof(%t)", &V1,ea0,ea0,ea0);
			}
			if ( TO_DEL(e) )
				expr_del(e);
		}
		f->b_bits = i;
		f->b_name = 0;
		break;
	}

	}
}


bit vrp_equiv;	/* vector == pointer wquivalence used in check() */
bit const_problem;
extern int t_const;

/*
 *	check if "this" can be combined with "t" by the operator "oper"
 *
 *	used for check of
 *			assignment types		(oper==ASSIGN)
 *			declaration compatability	(oper==0)
 *			argument types			(oper==ARG)
 *			return types			(oper==RETURN)
 *			overloaded function  name match	(oper==OVERLOAD)
 *			overloaded function coercion	(oper==COERCE)
 *
 *	NOT for arithmetic operators
 *
 *	return 1 if the check failed
 */
bit type_check(this, t, oper)
Ptype this;
Ptype t;
TOK oper;
{
	Ptype t1, t2;
	TOK b1, b2, r;
	bit t1_const, t2_const, first;

	t1 = this;
	t2 = t;
	t2_const = 0;
	t1_const = t_const;
	t_const = 0;
	first = 1;

	if (t1 == 0 || t2 == 0) {
		V1.u1.p = (char *)t1;
		V2.u1.p = (char *)t2;
		V3.u1.i = (int)oper;
		errorFIPC('i', "check(%p,%p,%d)", &V1, &V2, &V3, ea0);
	}
	vrp_equiv = 0;
	const_problem = 0;

	while (t1 && t2){
top:
		if (t1 == t2) return 0;
		if (t1->base == ANY || t2->base == ANY) return 0;

		b1 = t1->base;
		if (b1 == TYPE){
			if (!oper) t1_const = ((Pbase)t1)->b_const;
			t1 = ((Pbase)t1)->b_name->u1.tp;
			goto top;
		}

		b2 = t2->base;
		if (b2 == TYPE){
			if (!oper) t2_const = ((Pbase)t2)->b_const;
			t2 = ((Pbase)t2)->b_name->u1.tp;
			goto top;
		}

		if (b1 != b2){
			switch (b1){
			case PTR:
				vrp_equiv = 1;
				switch (b2){
				case VEC:
					t1 = ((Pptr)t1)->typ;
					t2 = ((Pvec)t2)->typ;
					first = 0;
					goto top;
				case FCT:
					t1 = ((Pptr)t1)->typ;
					if (first == 0 || t1->base != b2)
						return 1;
					first = 0;
					goto top;
				}
				first = 0;
				break;
			case VEC:
				vrp_equiv = 1;
				first = 0;
				switch (b2){
				case PTR:
					switch (oper){
					case 0:
					case ARG:
					case ASSIGN:
					case COERCE:
						break;
					case OVERLOAD:
					default:
						return 1;
					}
					t1 = ((Pvec)t1)->typ;
					t2 = ((Pptr)t2)->typ;
					goto top;
				}
				break;
			}
			goto base_check;
		}

		switch (b1){
		case VEC:
			{
			Pvec v1, v2;

			first = 0;
			v1 = (Pvec)t1;
			v2 = (Pvec)t2;
			if (v1->size != v2->size)
				switch (oper){
				case OVERLOAD:
				case COERCE:
					return 1;
				}
			t1 = v1->typ;
			t2 = v2->typ;
			break;
			}
		case PTR:
		case RPTR:
			{
			Pptr p1, p2;

			first = 0;
			p1 = (Pptr)t1;
			p2 = (Pptr)t2;
			if (p1->memof != p2->memof){
				if (baseofclass(p2->memof, p1->memof))
					return 1;
				Nstd++;
			}
			t1 = p1->typ;
			t2 = p2->typ;
			if (oper == ARG && t1->base == TYPE)
				t1_const = ((Pbase)p1->typ)->b_const;
			if (oper == ARG && t2->base == TYPE)
				t2_const = ((Pbase)p2->typ)->b_const;

			if (oper == 0){
				if (p1->rdo != p2->rdo){
					const_problem = 1;
					return 1;
				}
				if (b1 == RPTR && tconst(t1) != tconst(t2))
					const_problem = 1;
			}
			break;
			}

		case FCT:
			{
			Pfct f1, f2;
			Pname a1, a2;
			TOK k1, k2;
			int n1, n2;

			first = 0;
			f1 = (Pfct)t1;
			f2 = (Pfct)t2;
			a1 = f1->argtype;
			a2 = f2->argtype;
			k1 = f1->nargs_known;
			k2 = f2->nargs_known;
			n1 = f1->nargs;
			n2 = f2->nargs;

			if (f1->memof != f2->memof){
				if (baseofclass(f2->memof, f1->memof))
					return 1;
				Nstd++;
			}
			if ((k1 && k2 == 0) || (k2 && k1 == 0)) {
				if (f2->body == 0) return 1;
			}

			if (n1 != n2 && k1 && k2){
				goto aaa;
			}
			else if (a1 && a2){
				int i;

				i = 0;
				while (a1 && a2){
					i++;
					if (type_check(a1->u1.tp,a2->u1.tp,oper?OVERLOAD:0))
						return 1;
					a1 = a1->n_list;
					a2 = a2->n_list;
				}
				if (a1 || a2) goto aaa;
			}
			else if (a1 || a2){
			aaa:
				if (k1 == ELLIPSIS){
					switch (oper){
					case 0:
						if (a2 && k2 == 0) break;
							return 1;
					case ASSIGN:
						if (a2 && k2 == 0) break;
						return 1;
					case ARG:
						if (a1) return 1;
						break;
					case OVERLOAD:
					case COERCE:
						return 1;
					}
				}
				else if (k2 == ELLIPSIS){
					return 1;
				}
				else if (k1 || k2){
					return 1;
				}
			}
			t1 = f1->returns;
			t2 = f2->returns;
			}
			break;

		case FIELD:
			goto field_check;
		case CHAR:
		case SHORT:
		case INT:
		case LONG:
			goto int_check;
		case FLOAT:
		case DOUBLE:
			goto float_check;
		case EOBJ:
			goto enum_check;
		case COBJ:
			goto cla_check;
		case ZTYPE:
		case VOID:
			return 0;
		default:
			V1.u1.i = (int)oper;
			V2.u1.i = (int)b1;
			V3.u1.i = (int)b2;
			errorFIPC('i', "T::check(o=%d %d %d)", &V1, &V2,
				&V3, ea0);
		}
	}

	if (t1 || t2) return 1;
	return 0;

field_check:
	switch (oper){
	case 0:
	case ARG:
		errorFIPC('i', "check field?", ea0, ea0, ea0, ea0);
	}
	return 0;

float_check:
	if (first == 0 && b1 != b2 && b2 != ZTYPE) return 1;

int_check:
	if (((Pbase)t1)->b_unsigned != ((Pbase)t2)->b_unsigned){
		if (oper)
			Nstd++;
		else 
			return 1;
	}
enum_check:
const_check:
	if (!t1_const) t1_const = tconst(t1);
	if (!t2_const) t2_const = tconst(t2);

	if ((oper == 0 && t1_const != t2_const)
	|| (first == 0 && t2_const && t1_const == 0)) {
		const_problem = 1;
		return 1;
	}
	return 0;

cla_check:
	{
		Pbase c1, c2;
		Pname n1, n2;

		c1 = (Pbase)t1;
		c2 = (Pbase)t2;
		n1 = c1->b_name;
		n2 = c2->b_name;

		if (n1 == n2) goto const_check;
		if (first) return 1;

		switch (oper){
		case 0:
		case OVERLOAD:
			return 1;
		case ARG:
		case ASSIGN:
		case RETURN:
		case COERCE:
		{
			Pname b;
			Pclass cl;

			b = n2;
			while (b){
				cl = (Pclass)b->u1.tp;
				b = cl->clbase;

				if (b && cl->pubbase == 0) return 1;
				if (b == n1){
					Nstd++;
					goto const_check;
				}
			}
			return 1;
		}
		}
	}
	goto const_check;

base_check:
	if (oper) {
		if (first){
			if (b1 == VOID || b2 == VOID)
				return 1;
		}
		else {
			if (b1 == VOID){	/* check for void* = T* */
				register Ptype tx;

				tx = this;
			txloop:
				switch (tx->base){	/* t1 == void* */
				default:
					return 1;
				case VOID:
					break;
				case PTR:
				case VEC:
					tx = ((Pvec)tx)->typ;
					goto txloop;
				case TYPE:
					tx = ((Pbase)tx)->b_name->u1.tp;
					goto txloop;
				}

				tx = t;
			bloop:
				switch (tx->base){	/* t2 == T* */
				default:
					return 1;
				case VEC:
				case PTR:
				case FCT:
					Nstd++;
					goto const_check;
				case TYPE:
					tx = ((Pbase)tx)->b_name->u1.tp;
					goto bloop;
				}
			}
			if (b2 != ZTYPE) return 1;
		}

	}

	switch (oper){
	case 0:
	case OVERLOAD:
		return 1;
	case COERCE:
		switch (b1){
		case EOBJ:
		case ZTYPE:
		case CHAR:
		case SHORT:
		case INT:
			switch (b2){
			case EOBJ:
			case ZTYPE:
			case CHAR:
			case SHORT:
			case INT:
			case FIELD:
				goto const_check;
			}
			return 1;
		case LONG:	/* char, short, and int promotes to long */
			switch (b2){
			case ZTYPE:
			case EOBJ:
			case CHAR:
			case SHORT:
			case INT:
			case FIELD:
				Nstd++;
				goto const_check;
			}
			return 1;
		case FLOAT:
			switch (b2){
			case ZTYPE:
				Nstd++;
			case FLOAT:
			case DOUBLE:
				goto const_check;
			}
			return 1;
		case DOUBLE:	/* char, short, int and float promotes to double */
			switch (b2){
			case ZTYPE:
			case EOBJ:
			case CHAR:
			case SHORT:
			case INT:
				Nstd++;
			case FLOAT:
			case DOUBLE:
				goto const_check;
			}
			return 1;
		case PTR:
			switch (b2){
			case ZTYPE:
				Nstd++;
				goto const_check;
			}
		case RPTR:
		case VEC:
		case COBJ:
		case FCT:
			return 1;
		}
	case ARG:
	case ASSIGN:
	case RETURN:
		switch (b1){
		case COBJ:
			return 1;
		case EOBJ:
		case ZTYPE:
		case CHAR:
		case SHORT:
		case INT:
		case LONG:
			suppress_error++;
			r = num_ptr(t2, ASSIGN);
			suppress_error--;
			switch (r){
			case F:
				V1.u1.p = (char *)t2;
				V2.u1.p = (char *)t1;
				errorFIPC('w', "%t assigned to%t",
					&V1, &V2, ea0, ea0);

				break;
			case A:
			case P:
			case FCT:
				return 1;
			}
			break;
		case FLOAT:
		case DOUBLE:
			suppress_error++;
			r = numeric(t2, ASSIGN);
			suppress_error--;
			break;
		case VEC:
			return 1;
		case PTR:
			suppress_error++;
			r = num_ptr(t2, ASSIGN);
			suppress_error--;
			switch (r){
			case A:
			case I:
			case F:
				return 1;
			case FCT:
				if (((Pptr)t1)->typ->base != FCT)
					return 1;
			}
			break;
		case RPTR:
			return 1;
		case FCT:
			switch (oper){
			case ARG:
			case ASSIGN:
				return 1;
			}
		}
		break;
	}
	goto const_check;
}

