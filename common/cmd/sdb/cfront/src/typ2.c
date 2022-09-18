/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/typ2.c	1.2"

#include "cfront.h"
#include "size.h"

extern int chars_in_largest;

void typ_init()
{
	chars_in_largest = strlen(LARGEST_INT);

	defa_type = int_type = new_basetype(INT, 0);
	PERM(int_type);

	moe_type = new_basetype(INT, 0);
	PERM(moe_type);
	moe_type->b_const = 1;
	basetype_check(moe_type, 0);

	uint_type = new_basetype(INT, 0);
	PERM(uint_type);
	type_adj(uint_type, UNSIGNED);
	basetype_check(uint_type, 0);

	long_type = new_basetype(LONG, 0);
	PERM(long_type);
	basetype_check(long_type, 0);

	ulong_type = new_basetype(LONG, 0);
	PERM(ulong_type);
	type_adj(ulong_type, UNSIGNED);
	basetype_check(ulong_type, 0);

	short_type = new_basetype(SHORT, 0);
	PERM(short_type);
	basetype_check(short_type, 0);

	ushort_type = new_basetype(SHORT, 0);
	PERM(ushort_type);
	type_adj(ushort_type, UNSIGNED);
	basetype_check(ushort_type, 0);

	float_type = new_basetype(FLOAT, 0);
	PERM(float_type);

	double_type = new_basetype(DOUBLE, 0);
	PERM(double_type);

	zero_type = new_basetype(ZTYPE, 0);
	PERM(zero_type);
	zero->u1.tp = (Ptype)zero_type;

	void_type = new_basetype(VOID, 0);
	PERM(void_type);

	char_type = new_basetype(CHAR, 0);
	PERM(char_type);

	uchar_type = new_basetype(CHAR, 0);
	PERM(uchar_type);
	type_adj(uchar_type, UNSIGNED);
	basetype_check(uchar_type, 0);

	Pchar_type = (Ptype)new_ptr(PTR, char_type, 0);
	PERM(Pchar_type);

	Pint_type = (Ptype)new_ptr(PTR, int_type, 0);
	PERM(Pint_type);

	Pvoid_type = (Ptype)new_ptr(PTR, void_type, 0);
	PERM(Pvoid_type);

	Pfctchar_type = (Ptype)fct_ctor(char_type, 0, 0);
	Pfctchar_type = (Ptype)new_ptr(PTR, Pfctchar_type, 0);
	PERM(Pfctchar_type);

	Pfctvec_type = (Ptype)fct_ctor(int_type, 0, 0);
	/* must be last, see basetype_normalize() */
	Pfctvec_type = (Ptype)new_ptr(PTR, Pfctvec_type, 0);
	Pfctvec_type = (Ptype)new_ptr(PTR, Pfctvec_type, 0);
	PERM(Pfctvec_type);

	any_tbl = new_table(TBLSIZE, 0, 0);
	gtbl = new_table(GTBLSIZE, 0, 0);
	gtbl->t_name = new_name("global");
}

/*
 *	perform the "usual arithmetic conversions" C ref Manual 6.6
 *	on "this" op "t"
 *	"this" and "t" are integral or floating
 *	"t" may be 0
 */
Pbase arit_conv(this, t)
Pbase this;
Pbase t;
{
	bit l, u, f, l1, u1, f1;

	while (this->base == TYPE) this = (Pbase)(this->b_name->u1.tp);
	while (t && t->base == TYPE) t = (Pbase)(t->b_name->u1.tp);

	l1 = (this->base == LONG);
	u1 = this->b_unsigned;
	f1 = (this->base == FLOAT || this->base == DOUBLE);
	if (t){
		bit l2, u2, f2;

		l2 = (t->base == LONG);
		u2 = t->b_unsigned;
		f2 = (t->base == FLOAT || t->base == DOUBLE);
		l = (l1 || l2);
		u = (u1 || u2);
		f = (f1 || f2);
	}
	else {
		l = l1;
		u = u1;
		f = f1;
	}

	if (f)		return double_type;
	if (l & u)	return ulong_type;
	if (l & !u)	return long_type;
	if (u)		return uint_type;
			return int_type;
}

bit vec_const = 0;
bit fct_const = 0;

bit tconst(this)
Ptype this;
{
	Ptype t;

	t = this;
	vec_const = 0;
	fct_const = 0;
xxx:
	switch (t->base){
	case TYPE:
		if (((Pbase)t)->b_const) return 1;
		t = ((Pbase)t)->b_name->u1.tp;
		goto xxx;
	case VEC:
		vec_const = 1;
		return 1;
	case PTR:
	case RPTR:
		return ((Pptr)t)->rdo;
	case FCT:
	case OVERLOAD:
		fct_const = 1;
		return 1;
	default:
		return ((Pbase)t)->b_const;
	}
}

/*
 *	make somthing a constant or variable, return 0 if no problem
 */
TOK set_const(this, mode)
Ptype this;
bit mode;
{
	Ptype t;

	t = this;
xxx:
	switch (t->base){
	case TYPE:
		((Pbase)t)->b_const = mode;
		t = ((Pbase)t)->b_name->u1.tp;
		goto xxx;
	case ANY:
	case RPTR:
	case VEC:	/* constant by definition */
		return t->base;
	case PTR:
		((Pptr)t)->rdo = mode;
		return 0;
	default:
		((Pbase)t)->b_const = mode;
		return 0;
	}
}

int is_ref(this)
Ptype this;
{
	Ptype t;

	t = this;
xxx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xxx;
	case RPTR:
		return 1;
	default:
		return 0;
	}
}

int type_align(this)
Ptype this;
{
	Ptype t;

	t = this;
xx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case COBJ:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case VEC:
		t = ((Pvec)t)->typ;
		goto xx;
	case ANY:
		return 1;
	case CHAR:
		return AL_CHAR;
	case SHORT:
		return AL_SHORT;
	case INT:
		return AL_INT;
	case LONG:
		return AL_LONG;
	case FLOAT:
		return AL_FLOAT;
	case DOUBLE:
		return AL_DOUBLE;
	case PTR:
	case RPTR:
		return AL_WPTR;
	case CLASS:
		return ((Pclass)t)->obj_align;
	case ENUM:
	case EOBJ:
		return AL_INT;
	case VOID:
		error("illegal use of void", ea0, ea0, ea0, ea0);
		return AL_INT;
	default:
		V1.u1.p = (char *)t;
		V2.u1.i = (int)t->base;
		errorFIPC('i', "(%d,%k)->type::align", &V1, &V2, ea0, ea0);
	}
}

bit fake_sizeof;

/*
 *	the sizeof type operator
 *	return the size in bytes of the types representation
 */
int tsizeof(this)
Ptype this;
{
	Ptype t;

	t = this;
zx:
	if (t == 0)
		errorFIPC('i', "typ.tsizeof(t==0)", ea0, ea0, ea0, ea0);
	switch (t->base){
	case TYPE:
	case COBJ:
		t = ((Pbase)t)->b_name->u1.tp;
		goto zx;
	case ANY:
		return 1;
	case VOID:
		return 0;
	case ZTYPE:	/* assume pointer */
		return SZ_WPTR;
	case CHAR:
		return SZ_CHAR;
	case SHORT:
		return SZ_SHORT;
	case INT:
		return SZ_INT;
	case LONG:
		return SZ_LONG;
	case FLOAT:
		return SZ_FLOAT;
	case DOUBLE:
		return SZ_DOUBLE;
	case VEC:
	{
		Pvec v;

		v = (Pvec)t;
		if (v->size == 0){
			if (fake_sizeof == 0) {
				errorFIPC('w',
					"sizeof vector with undeclared dimension",
					ea0, ea0, ea0, ea0);
			}
			return SZ_WPTR;	/* vector argument has sizeof ptr */
		}
		return (v->size * tsizeof(v->typ));
	}
	case PTR:
	case RPTR:
		t = ((Pptr)t)->typ;
xxx:
		switch (t->base){
		default:
			return SZ_WPTR;
		case CHAR:
			return SZ_BPTR;
		case TYPE:
			t = ((Pbase)t)->b_name->u1.tp;
			goto xxx;
		}
	case FIELD:
	{
		Pbase b;

		b = (Pbase)t;
		return (b->b_bits / BI_IN_BYTE + 1);
	}
	case CLASS:
	{
		Pclass cl;
		int sz;

		cl = (Pclass)t;
		sz = cl->obj_size;
		if ((cl->defined &(DEFINED|SIMPLIFIED))== 0){
			V1.u1.p = (char *)cl->string;
			error("%sU, size not known", &V1, ea0, ea0, ea0);
			return SZ_INT;
		}
		return sz;
	}
	case EOBJ:
	case ENUM:
		return SZ_INT;
	default:
		V1.u1.i = (int)t->base;
		errorFIPC('i', "sizeof(%d)", &V1, ea0, ea0, ea0);
	}
}

bit vec_type(this)
Ptype this;
{
	Ptype t;

	t = this;
xx:
	switch (t->base){
	case ANY:
	case VEC:
	case PTR:
	case RPTR:
		return 1;
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	default:
		return 0;
	}
}

/*
 *	index==1: *p
 *	index==0: p[expr]
 */
Ptype type_deref(this)
Ptype this;
{
	Ptype t;

	t = this;
xx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case PTR:
	case RPTR:
	case VEC:
		if (t == Pvoid_type) error("void* deRd", ea0, ea0, ea0, ea0);
		return ((Pvec)t)->typ;
	case ANY:
		return t;
	default:
		error("nonP deRd", ea0, ea0, ea0, ea0);
		return (Ptype)any_type;
	}
}


Pptr new_ptr(b, t, r)
TOK b;
Ptype t;
bit r;
{
	Pptr Xthis_ptr;

	Xthis_ptr = (Pptr)new(sizeof(struct ptr));
	Nt++;
	Xthis_ptr->base = b;
	Xthis_ptr->typ = t;
	Xthis_ptr->rdo = r;

	return Xthis_ptr;
}
