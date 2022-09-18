/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/dcl.c	1.2"
/**************************************************************************

dcl.c:

	``declare'' all names, that is insert them in the appropriate symbol tables.

	Calculate the size for all objects(incl. stack frames),
	and find store the offsets for all members(incl. auto variables).
	"size.h" holds the constants needed for calculating sizes.

	Note that(due to errors) functions may nest

*****************************************************************************/


#include "cfront.h"
#include "size.h"

struct dcl_context ccvec [MAXCONT];
struct dcl_context *cc = ccvec;
int byte_offset = 0;
int bit_offset = 0;
int max_align = 0;
int stack_size = 0;
int enum_count = 0;
int friend_in_class = 0;

Pname dclass();
Pname denum();
void dargs();
void merge_init();

/*
 *	enter a copy of this name into symbol table "tbl";
 *		- create local symbol tables as needed
 *	
 *	"scope" gives the scope in which the declaration was found
 *		- EXTERN, FCT, ARG, PUBLIC, or 0
 *	Compare "scope" with the specified storage class "n_sto"
 *		- AUTO, STATIC, REGISTER, EXTERN, OVERLOAD, FRIEND, or 0
 *
 *	After name::dcl()
 *	n_stclass ==	0		class or enum member
 *			REGISTER	auto variables declared register
 *			AUTO		auto variables not registers
 *			STATIC		statically allocated object
 *	n_scope ==	0		private class member
 *			PUBLIC		public class member
 *			EXTERN		name valid in this and other files
 *			STATIC		name valid for this file only
 *			FCT		name local to a function
 *			ARG		name of a function argument
 *			ARGT		name of a type defined in an
 *					argument list
 *
 *	typecheck function bodies;
 *	typecheck initializers;
 *
 *	note that functions(error recovery) and classes(legal) nest
 *
 *	The return value is used to chain symbol table entries, but cannot
 *	be used for printout because it denotes the sum of all type information
 *	for the name
 *
 *	names of typenames are marked with n_oper==TNAME
 *
 *	WARNING: The handling of scope and storage class is cursed!
 */
Pname name_dcl(this, tbl, scope)
Pname this;
Ptable tbl;
TOK scope;
{
	Pname nn, cn, odcl;
	Ptype nnt;
	Pclass cl;
	Ptable etbl;

	nnt = 0;
	odcl = Cdcl;
	Cdcl = this;
	switch (this->base){
	case TNAME:
		type_dcl(this->u1.tp, tbl);
		PERM(this->u1.tp);
		nn = new_name(this->u2.string);
		nn->base = TNAME;
		nn->u1.tp = this->u1.tp;
		insert(tbl, nn, 0);
		name__dtor(nn);
		Cdcl = odcl;
		return this;
	case NAME:
		switch (this->n_oper){
		case TNAME:
			if (this->u1.tp->base != FCT)
				this->n_oper = 0;
			break;
		case COMPL:
			if (this->u1.tp->base != FCT){
				V1.u1.p = (char *)this->u2.string;
				error("~%s notF", &V1, ea0, ea0, ea0);
				this->n_oper = 0;
			}
			break;
		}
		break;
	default:
		errorFIPC('i', "NX in N::dcl()", ea0, ea0, ea0, ea0);
	}

	if (this->u5.n_qualifier){	/* class function: c::f(); */
		Pname x;
		if (this->u1.tp->base != FCT){
			V1.u1.p = (char *)this;
			error("QdN%n inD of nonF", &V1, ea0, ea0, ea0);
			Cdcl = odcl;
			return 0;
		}

		cn = this->u5.n_qualifier;
		switch (cn->base){
		case TNAME:
			break;
		case NAME:
			cn = look(gtbl, cn->u2.string, 0);
			if (cn && cn->base==TNAME) break;
		default:
			V1.u1.p = (char *)this->u5.n_qualifier;
			V2.u1.p = (char *)this;
			error("badQr%n for%n", &V1, &V2, ea0, ea0);
			Cdcl = odcl;
			return 0;
		}

		cn = ((Pbase)cn->u1.tp)->b_name;
		if (this->n_oper) check_oper(this, cn);

		cl = (Pclass)cn->u1.tp;
		if (cl == cc->cot) {
			this->u5.n_qualifier = 0;
			goto xdr;
		}
		else if ((cl->defined&(DEFINED|SIMPLIFIED)) == 0) {
			V1.u1.p = (char *)cn;
			error("C%nU", &V1, ea0, ea0, ea0);
			Cdcl = odcl;
			return 0;
		}

		etbl = cl->memtbl;
		x = look(etbl, this->u2.string, 0);
		if (x==0 || x->u4.n_table!=etbl) {
			V1.u1.p = (char *)this;
			V2.u1.p = (char *)cn;
			error("%n is not aM of%n", &V1, &V2, ea0, ea0);
			Cdcl = odcl;
			return 0;
		}
	}

xdr:
	if (this->n_oper && this->u1.tp->base != FCT && this->n_sto != OVERLOAD) {
		V1.u1.i = (int)this->n_oper;
		error("operator%k not aF", &V1, ea0, ea0, ea0);
	}

	/*	if a storage class was specified
	 *		check that it is legal in the scope 
	 *	else
	 *		provide default storage class
	 *	some details must be left until the type of the object is known
	 */

	this->n_stclass = this->n_sto;
	this->n_scope = scope;	/* default scope & storage class */

	switch (this->n_sto){
	default:
		V1.u1.i = (int)this->n_sto;
		errorFIPC('i', "unexpected %k", &V1, ea0, ea0, ea0);
	case FRIEND:
	{
		cl = cc->cot;
		switch (scope){
		case 0:
		case PUBLIC:
			break;
		default:
			V1.u1.p = (char *)this;
			V2.u1.i = (int)scope;
			error("friend%n not inCD(%k)", &V1, &V2, ea0, ea0);
			this->base = 0;
			Cdcl = odcl;
			return 0;
		}

		switch (this->n_oper){
		case 0:
		case NEW:
		case DELETE:
		case CTOR:
		case DTOR:
		case TYPE:
			this->n_sto = 0;
			break;
		default:
			this->n_sto = OVERLOAD;
		}

		switch (this->u1.tp->base){
		case COBJ:
			nn = ((Pbase)this->u1.tp)->b_name;
			break;
		case CLASS:
			nn = this;
			break;
		case FCT:
			stack();
			cc->not = 0;
			cc->tot = 0;
			cc->cot = 0;
			friend_in_class++;
			this->n_sto = EXTERN;
			nn = name_dcl(this, gtbl, EXTERN);
			friend_in_class--;
			unstack();
			if (nn->u1.tp->base == OVERLOAD){
				Pgen g;

				g = (Pgen)nn->u1.tp;
				nn = find(g,(Pfct)this->u1.tp, 1);
			}
			break;
		default:
			V1.u1.p = (char *)this->u1.tp;
			V2.u1.p = (char *)this;
			error("badT%t of friend%n", &V1, &V2, ea0, ea0);
		}
		PERM(nn);
		cl->friend_list = new_nalist(nn, cl->friend_list);
		Cdcl = odcl;
		return nn;
	}
	case OVERLOAD:
		this->n_sto = 0;
		switch (scope){
		case 0:
		case PUBLIC:
			errorFIPC('w', "overload inCD(ignored)", ea0,ea0,ea0,ea0);
			switch (this->u1.tp->base){
			case INT:
				this->base = 0;
				Cdcl = odcl;
				return this;
			case FCT:
				return name_dcl(this, tbl, scope);
			}
		}
		if (this->n_oper && this->u1.tp->base == FCT)
			break;
		nn = insert(tbl, this, 0);

		if (Nold){
			if (nn->u1.tp->base != OVERLOAD){
				V1.u1.p = (char *)this;
				error("%n redefined as overloaded", &V1,
					ea0, ea0, ea0);
				nn->u1.tp = (Ptype)new_gen(this->u2.string);
			}
		}
		else {
			nn->u1.tp = (Ptype)new_gen(this->u2.string);
		}

		switch (this->u1.tp->base){
		case INT:
			this->base = 0;
			Cdcl = odcl;
			return nn;
		case FCT:
			break;
		default:
			V1.u1.p = (char *)this;
			V2.u1.i = (int)this->u1.tp->base;
			error("N%n ofT%k cannot be overloaded", &V1,&V2,ea0,ea0);
			Cdcl = odcl;
			return nn;
		}
		break;
	case REGISTER:
		if (this->u1.tp->base == FCT){
			V1.u1.p = (char *)this;
			errorFIPC('w', "%n: register(ignored)", &V1,ea0,ea0,ea0);
			goto ddd;
		}
	case AUTO:
		switch (scope){
		case 0:
		case PUBLIC:
		case EXTERN:
			V1.u1.i = (int)this->n_sto;
			error("%k not inF", &V1, ea0, ea0, ea0);
			goto ddd;
		}
		if (this->n_sto != REGISTER)this->n_sto = 0;
		break;
	case EXTERN:
		switch (scope){
		case ARG:
			error("externA", ea0, ea0, ea0, ea0);
			goto ddd;
		case 0:
		case PUBLIC:
			/* extern is provided as a default for
			 * functions without body
			 */
			if (this->u1.tp->base != FCT){
				V1.u1.p = (char *)this;
				error("externM%n", &V1, ea0, ea0, ea0);
			}
			goto ddd;
		}
		this->n_stclass = STATIC;
		this->n_scope = EXTERN;
		/* avoid FCT scoped externs to allow better checking */
		break;
	case STATIC:
		switch (scope){
		case ARG:
			V1.u1.p = (char *)this;
			error("static used forA%n", &V1, ea0, ea0, ea0);
			goto ddd;
		case 0:
		case PUBLIC:
			this->n_stclass = STATIC;
			this->n_scope = scope;
			break;
		default:
			this->n_scope = STATIC;
		}
		break;
	case 0:
	ddd:
		switch (scope){		/* default storage classes */
		case EXTERN:
			this->n_scope = EXTERN;
			this->n_stclass = STATIC;
			break;
		case FCT:
			if (this->u1.tp->base == FCT){
				this->n_stclass = STATIC;
				this->n_scope = EXTERN;
			}
			else 
				this->n_stclass = AUTO;
			break;
		case ARG:
			this->n_stclass = AUTO;
			break;
		case 0:
		case PUBLIC:
			this->n_stclass = 0;
			break;
		}
	}

	/*
	 *	now insert the name into the appropriate symbol table,
	 *	and compare types with previous declarations of that name
	 *
	 *	do type dependent adjustments of the scope
	 */
	switch (this->u1.tp->base){
	case ASM:
	{	Pbase b;
		Pname n;
		char *s, *s2;
		int ll;

		b = (Pbase)this->u1.tp;
		n = insert(tbl, this, 0);
		assign(n);
		use(n);
		s = (char *)b->b_name;		/* save asm string. Shoddy */
		ll = strlen(s);
		s2 = (char *) new(ll+1);
		strcpy(s2, s);
		b->b_name = (Pname)s2;
		return this;
		}
	case CLASS:
		nn = dclass(this,tbl);
		Cdcl = odcl;
		return nn;

	case ENUM:
		nn = denum(this,tbl);
		Cdcl = odcl;
		return nn;

	case FCT:
		nn = dofct(this, tbl, scope);
		if (nn == 0){
			Cdcl = odcl;
			return 0;
		}
		break;

	case FIELD:
		switch (this->n_stclass){
		case 0:
		case PUBLIC:
			break;
		default:
			V1.u1.i = (int)this->n_stclass;
			error("%k field", &V1, ea0, ea0, ea0);
			this->n_stclass = 0;
		}

		if (cc->not==0 || cc->cot->csu==UNION) {
			error((cc->not ? "field in union": "field not inC"),
				ea0, ea0, ea0, ea0);
			PERM(this->u1.tp);
			Cdcl = odcl;
			return this;
		}

		if (this->u2.string){
			nn = insert(tbl, this, 0);
			this->u4.n_table = nn->u4.n_table;
			if (Nold){
				V1.u1.p = (char *)this;
				error("twoDs of field%n", &V1, ea0, ea0, ea0);
			}
		}

		type_dcl(this->u1.tp, tbl);
		field_align(this);
		break;

	case COBJ:
	{
		cl = (Pclass)((Pbase)this->u1.tp)->b_name->u1.tp;
		if (cl->csu == ANON){	/* export member names to enclosing scope */
			char *p;
			int uindex, i;
			Ptable mtbl, tb;
			Pname n;

			p = cl->string;
			while (*p++!= 'C')
				/* EMPTY */;
			uindex = str_to_int(p);

			/* cannot cope with use counts for ANONs: */
			((Pbase)this->u1.tp)->b_name->n_used = 1;
			((Pbase)this->u1.tp)->b_name->n_assigned_to = 1;

			mtbl = cl->memtbl;
			nn = get_mem(mtbl, i = 1);
			for( ;nn ;nn = get_mem(mtbl,++i)) {
				if (nn->u1.tp->base == FCT){
					V1.u1.p = (char *)nn;
					errorFIPC('s', "M%n for anonymous union",
						&V1, ea0, ea0, ea0);
					break;
				}
				tb = nn->u4.n_table;
				nn->u4.n_table = 0;
				n = insert(tbl, nn, 0);
				if (Nold){
					V1.u1.p = (char *)nn;
					error("twoDs of%n(one in anonymous union)",
	   					&V1, ea0, ea0, ea0);
					break;
				}
				n->n_union = uindex;
				nn->u4.n_table = tb;
			}
		}
		goto cde;
	}

	case VEC:
	case PTR:
	case RPTR:
		type_dcl(this->u1.tp, tbl);

	default:
	cde:
		nn = insert(tbl, this, 0);

		this->u4.n_table = nn->u4.n_table;
		if (Nold){
			if (nn->u1.tp->base == ANY)goto zzz;

			if (type_check(this->u1.tp, nn->u1.tp, 0)){
				V1.u1.p = (char *)this;
				V2.u1.p = (char *)nn->u1.tp;
				V3.u1.p = (char *)this->u1.tp;
				error("twoDs of%n;Ts:%t and%t", &V1,&V2,&V3,ea0);
				Cdcl = odcl;
				return 0;
			}

			if (this->n_sto && this->n_sto != nn->n_scope)
			{
				V1.u1.p = (char *)this;
				V2.u1.i = (int)this->n_sto;
				if (nn->n_sto)
					V3.u1.i = nn->n_sto;
				else
					V3.u1.i = (scope == FCT)? AUTO: EXTERN;
				error("%n declared as both%k and%k", &V1, &V2,
					&V3, ea0);
			} else if (nn->n_scope==STATIC && this->n_scope==EXTERN) {
				V1.u1.p = (char *)this;
				error("%n both static and extern", &V1,ea0,ea0,ea0);
			} else if (nn->n_sto==STATIC && this->n_sto==STATIC) {
				V1.u1.p = (char *)this;
				error("static%n declared twice", &V1,ea0,ea0,ea0);
			} else {
				if (this->n_sto == 0
				&& nn->n_sto == EXTERN
				&& this->u3.n_initializer
				&& tconst(this->u1.tp))
					this->n_sto = EXTERN;
				this->n_scope = nn->n_scope;

				switch (scope){
				case FCT:
					V1.u1.p = (char *)this;
					error("twoDs of%n", &V1, ea0, ea0, ea0);
					Cdcl = odcl;
					return 0;
				case ARG:
					V1.u1.p = (char *)this;
					error("twoAs%n", &V1, ea0, ea0, ea0);
					Cdcl = odcl;
					return 0;
				case 0:
				case PUBLIC:
					V1.u1.p = (char *)this;
					error("twoDs ofM%n", &V1, ea0, ea0, ea0);
					Cdcl = odcl;
					return 0;
				case EXTERN:
					if (fct_void == 0
					&& this->n_sto == 0
					&& nn->n_sto == 0) {
						V1.u1.p = (char *)this;
						error("two definitions of%n",
							&V1, ea0, ea0, ea0);
						Cdcl = odcl;
						return 0;
					}
				}
			}
			this->n_scope = nn->n_scope;

			if (this->u3.n_initializer){
				if (nn->u3.n_initializer || nn->n_val){
					V1.u1.p = (char *)this;
					error("twoIrs for%n", &V1, ea0, ea0, ea0);
				}
				nn->u3.n_initializer = this->u3.n_initializer;
			}
			if (this->u1.tp->base == VEC){
				/* handle:  extern v[]; v[200]; */
				Ptype ntp;

				ntp = nn->u1.tp;
				while (ntp->base == TYPE)
					ntp = ((Pbase)ntp)->b_name->u1.tp;

				if (((Pvec)ntp)->dim == 0)
					((Pvec)ntp)->dim = ((Pvec)this->u1.tp)->dim;
				if (((Pvec)ntp)->size == 0)
					((Pvec)ntp)->size=((Pvec)this->u1.tp)->size;
			}
		}
		else {	/* check for the ambiguous plain "int a;" */
			if (scope != ARG
			&& this->n_sto != EXTERN
			&& this->u3.n_initializer == 0
			&& this->u1.tp->base == VEC
			&&((Pvec)this->u1.tp)->size == 0) {
				V1.u1.p = (char *)this;
				errorFloc(&this->where,
					"dimension missing for vector%n",
					&V1, ea0, ea0, ea0);
			}
		}

	zzz:
		if (this->base != TNAME){
			Ptype t;

			t = nn->u1.tp;
			if (t->base == TYPE){
				Ptype tt;

				tt = ((Pbase)t)->b_name->u1.tp;
				if (tt->base == FCT)
					nn->u1.tp = t = tt;
			}

			switch (t->base){
			case FCT:
			case OVERLOAD:
				break;
			default:
				fake_sizeof = 1;
				switch (nn->n_stclass){
				default:
					if (nn->n_scope != ARG){
						int x, y;

						x = type_align(t);
						y = tsizeof(t);

						if (max_align < x)max_align = x;

						while (0 < bit_offset){
							byte_offset++;
							bit_offset -= BI_IN_BYTE;
						}
						bit_offset = 0;

						if (byte_offset && 1<x)
							byte_offset =
							((byte_offset-1)/x)*x+x;
						nn->n_offset = byte_offset;
						byte_offset += y;
					}
					break;
				case STATIC:
					/* check that size is known */
					tsizeof(t);
				}
				fake_sizeof = 0;
			}
		}

	{	Ptype t;
		int const_old;
		bit vec_seen;
		Pexpr init;

		t = nn->u1.tp;
		const_old = const_save;
		vec_seen = 0;
		init = this->u3.n_initializer;

		if (init){
			switch (this->n_scope){
			case 0:
			case PUBLIC:
				if (this->n_stclass != STATIC){
					V1.u1.p = (char *)this;
					error("Ir forM%n", &V1, ea0, ea0, ea0);
				}
				break;
			}
		}

	lll:
		switch (t->base){
		case RPTR:
			if (init){
				if (nn->n_scope == ARG)break;
				init = expr_typ(init, tbl);
				if (this->n_sto == STATIC && lval(init, 0) == 0) {
					V1.u1.p = (char *)this;
					error("Ir for staticR%n not an lvalue",
				   		&V1, ea0, ea0, ea0);
				} else 
					nn->u3.n_initializer =
					this->u3.n_initializer =
					ref_init((Pptr)t, init, tbl);
				assign(nn);
			}
			else {
				switch (nn->n_scope){
				default:
					if (this->n_sto == EXTERN)break;
					V1.u1.p = (char *)this;
					error("unIdR%n", &V1, ea0, ea0, ea0);
				case ARG:
					break;
				case PUBLIC:
				case 0:
					if (this->n_sto == EXTERN){
						V1.u1.p = (char *)this;
						error("a staticM%n cannot be aR",
							&V1, ea0, ea0, ea0);
					}
					break;
				}
			}
			break;
		case COBJ:
		{
			Pname ctor, dtor;

			cn = ((Pbase)t)->b_name;
			cl = (Pclass)cn->u1.tp;
			ctor = has_ctor(cl);
			dtor = has_dtor(cl);

			if (dtor){
				Pstmt dls;
				switch (nn->n_scope){
				case EXTERN:
					if (this->n_sto == EXTERN)break;
				case STATIC:
				{	Ptable otbl;
					/* to collect temporaries generated */
					/* in static destructors where we */
					/* can find them again(in std_tbl) */
					otbl = tbl;
					if (std_tbl == 0)
						std_tbl = new_table(8,gtbl,0);
					tbl = std_tbl;
					if (vec_seen){
						/* _vec_delete(vec,noe,sz,dtor,0); */
						int esz;
						Pexpr noe, sz, arg;

						esz = tsizeof((Ptype)cl);
						noe=new_ival(tsizeof(nn->u1.tp)/esz);
						sz = new_ival(esz);
						arg = new_expr(0,ELIST,(Pexpr)dtor,zero);
						lval((Pexpr)dtor, ADDROF);
						arg = new_expr(0,ELIST,sz,arg);
						arg = new_expr(0,ELIST,noe,arg);
						arg = new_expr(0,ELIST,(Pexpr)nn,arg);
						arg = new_call(vec_del_fct,arg);
						arg->base = G_CALL;
						arg->u4.fct_name = vec_del_fct;
						arg->u1.tp = (Ptype)any_type;
						/* avoid another type check */

						dls = new_estmt(SM,nn->where,arg,0);
					}
					else {	/* nn->cl::~cl(0); */
						Pexpr r, ee, dl;

						r = new_ref(DOT,nn,dtor);
						ee = new_expr(0,ELIST,zero,0);
						dl = new_call(r, ee);
						dls = new_estmt(SM,nn->where,dl,0);
						dl->base = G_CALL;
						dl->u4.fct_name = dtor;
						dl->u1.tp = (Ptype)any_type;
						/* avoid another check */
					}
					/* destructors for statics are executed
					 * in reverse order
					 */

					if (st_dlist) dls->s_list = st_dlist;
					st_dlist = dls;
					tbl = otbl;
				}
				}
			}
			if (ctor)	{
				Pexpr oo;
				int vi, sti;
				Ptable otbl;
				Pname c;

				oo = (Pexpr)nn;
				vi = vec_seen;
				for(;vi ;vi--) oo = contents(oo);
				sti = 0;

				switch (nn->n_scope) {
				case EXTERN:
					if (init == 0 && this->n_sto == EXTERN)
						goto ggg;
				case STATIC:
					sti = 1;
					if (tbl != gtbl){
						/* prohibited only to avoid
						 * having to handle local
						 * variables in the constructors
						 * argument list
						 */
						V1.u1.p = (char *)this;
						errorFIPC('s',
							"local static%n ofCWK",
						    	&V1, ea0, ea0, ea0);
					}
				default:
					if (vec_seen && init){
						V1.u1.p = (char *)this;
						error("Ir forCO%n[]",
							&V1, ea0, ea0, ea0);
						this->u3.n_initializer= init= 0;
					}
					break;
				case ARG:
				case PUBLIC:
				case 0:
					goto ggg;
				}
				const_save = 1;
				assign(nn);
				otbl = tbl;
				if (sti){
					/* to collect temporaries generated
					 * in static initializers where we
					 * can find them again(in sti_tbl)
					 */
					if (sti_tbl == 0)
						sti_tbl = new_table(8,gtbl,0);
					tbl = sti_tbl;
					if (this->n_sto == EXTERN)
						nn->n_sto= this->n_sto= 0;
				}

				if (init) {
					if (init->base==VALUE) {
						Pname n2;

						switch (init->u4.tp2->base){
						case CLASS:
							if ((Pclass)init->u4.tp2!=cl)
								goto inin;
							break;
						default:
							n2 = is_cl_obj(init->u4.tp2);
							if (n2 == 0
							|| (Pclass)n2->u1.tp!= cl)
								goto inin;
						}

						init->u3.e2 = oo;
						init = expr_typ(init, tbl);
						if (init->base == G_CM)
						/* beware of type conversion
						 * operators
						 */
						switch (init->u4.tp2->base){
						case CLASS:
							if ((Pclass)init->u4.tp2!=cl)
								goto inin;
							break;
						default:
							n2 = is_cl_obj(init->u4.tp2);
							if (n2 == 0
							|| (Pclass)n2->u1.tp!= cl)
								goto inin;
						}
					}
					else {
					inin:
						init = expr_typ(init, tbl);
						if (init->base == G_CM &&
						type_check(nn->u1.tp,init->u1.tp,ASSIGN)==0)
							replace_temp(init, address((Pexpr)nn));
						else 
							init = class_init((Pexpr)nn, nn->u1.tp, init, tbl);
					}
				}
				else {
					init = new_texpr(VALUE, cl, 0);
					init->u3.e2 = oo;
					init = expr_typ(init, tbl);
				}

				if (vec_seen){
					c = has_ictor(cl);
					V1.u1.p = (char *)cn;
					if (c == 0)
						error("vector ofC%n that does not have aK taking noAs",
							&V1, ea0, ea0, ea0);
					else if (((Pfct)c->u1.tp)->nargs)
						errorFIPC('s',
						"defaultAs forK for vector ofC%n",
							&V1, ea0, ea0, ea0);
				}

	if (sti) {
		Pstmt ist;
		static Pstmt itail = 0;

		if (vec_seen) {	/* _vec_new(vec,noe,sz,ctor); */
			int esz;
			Pexpr noe, sz, arg;

			esz = tsizeof((Ptype)cl);
			noe = new_ival(tsizeof(nn->u1.tp)/esz);
			sz = new_ival(esz);
			arg = new_expr(0, ELIST,(Pexpr)c, 0);
			lval((Pexpr)c, ADDROF);
			arg = new_expr(0, ELIST, sz, arg);
			arg = new_expr(0, ELIST, noe, arg);
			arg = new_expr(0, ELIST,(Pexpr)nn, arg);
			init = new_call(vec_new_fct, arg);
			init->base = G_CALL;
			init->u4.fct_name = vec_new_fct;
			init->u1.tp = (Ptype)any_type;
		}
		else {
			switch (init->base) {
			case DEREF:		/* *constructor? */
				if (init->u2.e1->base == G_CALL) {
					Pname fn;

					fn = init->u2.e1->u4.fct_name;
					if (fn==0 || fn->n_oper!=CTOR) goto as;
					init = init->u2.e1;
					break;
				}
				goto as;
			case ASSIGN:
				if (init->u2.e1 == (Pexpr)nn)break;
				/* simple assignment */
			as:
			default:
				init = new_expr(0, ASSIGN,(Pexpr)nn, init);
			}
		}
		ist = new_estmt(SM, nn->where, init, 0);
		/* constructors for statics are executed in order */
		if (st_ilist == 0)
			st_ilist = ist;
		else 
			itail->s_list = ist;
		itail = ist;
		init = 0;
	} /* if (sti) */

				nn->u3.n_initializer = this->u3.n_initializer = init;
				const_save = const_old;
				tbl = otbl;
			}
			else if (init == 0)		/* no initializer */
				goto str;
			else if (is_simple(cl)
	    			&& cl->csu!=UNION
				&& cl->csu!=ANON) {	/* struct */
				if ( init->base == ILIST &&
					(cl->defined&(DEFINED|SIMPLIFIED))==0)
					{
						V1.u1.p = (char *)cn;
						V2.u1.p = (char *)nn;
						error("struct%nU: cannotI%n",
							&V1, &V2, ea0, ea0);
						Cdcl = odcl;
						return 0;
					}
				init = expr_typ(init, tbl);
				if (type_check(nn->u1.tp, init->u1.tp,ASSIGN)==0
				&& init->base == G_CM)
					replace_temp(init,address((Pexpr)nn));
				else 
					goto str;
			}
			else if (init->base == ILIST){	/* class or union */
				V1.u1.p = (char *)nn;
				error("cannotI%nWIrL", &V1, ea0, ea0, ea0);
			}
			else {			/* bitwise copy ok? */
						/* possible to get here? */
				init = expr_typ(init, tbl);
				if (type_check(nn->u1.tp,init->u1.tp,ASSIGN)==0){
					if (init->base==G_CM)
						replace_temp(init,address((Pexpr)nn));
					else 
						goto str;
				}
				else 
				{
					V1.u1.p = (char *)nn;
					V2.u1.i = (int)cl->csu;
					V3.u1.p = (char *)cl->string;
					error("cannotI%n:%k %s has noK", &V1,
						&V2, &V3, ea0);
				}
			}
			break;
		}
		case VEC:
			t = ((Pvec)t)->typ;
			vec_seen++;
			goto lll;
		case TYPE:
			if (init==0 &&((Pbase)t)->b_const) {
				switch (this->n_scope) {
				case ARG:
				case 0:
				case PUBLIC:
					break;
				default:
					if (this->n_sto!=EXTERN && is_cl_obj(t)==0){
						V1.u1.p = (char *)this;
						error("unId const%n", &V1, ea0,
							ea0, ea0);
					}
				}
			}
			t = ((Pbase)t)->b_name->u1.tp;
			goto lll;
		default:
		str:
			if (init == 0) {
				switch (this->n_scope){
				case ARG:
				case 0:
				case PUBLIC:
					break;
				default:
					if (this->n_sto!=EXTERN && tconst(t)) {
						V1.u1.p = (char *)this;
						error("unId const%n", &V1, ea0,
							ea0, ea0);
					}
				}

				break;
			}

			const_save =	   const_save
					|| this->n_scope==ARG
					||(tconst(t) && vec_const==0);
			nn->u3.n_initializer = this->u3.n_initializer =
				init = expr_typ(init, tbl);
			if (const_save) PERM(init);
			assign(nn);
			const_save = const_old;

			switch (init->base) {
			case ILIST:
				new_list(init);
				list_check(nn, nn->u1.tp, 0);
				if (next_elem())
					error("IrL too long", ea0, ea0, ea0, ea0);
				break;
			case STRING:
				if (nn->u1.tp->base == VEC){
					Pvec v;

					v = (Pvec)nn->u1.tp;
					if (v->typ->base == CHAR){
						int sz, isz;

						sz = v->size;
						isz = ((Pvec)init->u1.tp)->size;
						if (sz == 0)
							v->size = isz;
						else if (sz < isz)
						{
							V1.u1.i = isz;
							V2.u1.p = (char *)nn;
							V3.u1.i = sz;
							error("Ir too long(%d characters) for%n[%d]",
								&V1, &V2, &V3, ea0);
						}
						break;
					}
				}
			default:
			{	Ptype nt;
				int ntc;

				nt = nn->u1.tp;
				ntc = ((Pbase)nt)->b_const;

				if (vec_seen){
					V1.u1.p = (char *)nn;
					error("badIr for vector%n", &V1,ea0,ea0,ea0);
					break;
				}
			tlx:
				switch (nt->base){
				case TYPE:
					nt = ((Pbase)nt)->b_name->u1.tp;
					ntc |= ((Pbase)nt)->b_const;
					goto tlx;
				case INT:
				case CHAR:
				case SHORT:
					if (init->base==ICON
					&& init->u1.tp==(Ptype)long_type) {
						V1.u1.i = (int)nn->u1.tp->base;
						V2.u1.p = (char *)nn;
						errorFIPC('w',
							"longIr constant for%k%n",
							&V1,&V2,ea0,ea0);
					}
				case LONG:
					if (((Pbase)nt)->b_unsigned
					&& init->base==UMINUS
					&& init->u3.e2->base==ICON) {
						V1.u1.p = (char *)nn;
						errorFIPC('w',
							"negativeIr for unsigned%n",
							&V1,ea0,ea0,ea0);
					}
					if (ntc && scope != ARG) {
						int i;
						Neval = 0;
						i = eval(init);
						if (Neval == 0){
							if (TO_DEL(init))
								expr_del(init);
							nn->n_evaluated = this->n_evaluated = 1;
							nn->n_val = this->n_val = i;
							nn->u3.n_initializer = this->u3.n_initializer = 0;
						}
					}
					break;
				case PTR:
					this->u3.n_initializer = init =
						ptr_init((Pptr)nt,init,tbl);
				}

			{	Pexpr x;

				x = try_to_coerce(nt, init, "initializer", tbl);
				if (x){
					this->u3.n_initializer = x;
					goto stgg;
				}
			}

			if (type_check(nt, init->u1.tp, ASSIGN))
			{
				V1.u1.p = (char *)init->u1.tp;
				V2.u1.p = (char *)this;
				V3.u1.p = (char *)nn->u1.tp;
				error("badIrT%t for%n(%tX)", &V1, &V2, &V3, ea0);
			} else {
			stgg:
				if (init && this->n_stclass == STATIC) {
					/* check if non-static variables are used */
					/* INCOMPLETE */
					switch (init->base){
					case NAME:
						if (tconst(init->u1.tp) == 0){
							V1.u1.p = (char *)init;
							V2.u1.p = (char *)nn;
							error("V%n used inIr for%n",
								&V1, &V2, ea0, ea0);
						}
						break;
					case DEREF:
					case DOT:
					case REF:
					case CALL:
					case G_CALL:
					case NEW:
						V1.u1.i = (int)init->base;
						V2.u1.p = (char *)nn;
						error("%k inIr of static%n",
							&V1, &V2, ea0, ea0);
					}
			}
			}
		}
		} /* switch */
	} /* block */
	} /* default */

	} /* switch */
ggg:
	PERM(nn);
	switch (this->n_scope){
	case FCT:
		nn->u3.n_initializer = this->u3.n_initializer;
		break;
	default:
	{
		Ptype t;

		t = nn->u1.tp;
	px:
		PERM(t);
		switch (t->base){
		case PTR:
		case RPTR:
		case VEC:	t = ((Pptr)t)->typ; goto px;
		case TYPE:	t = ((Pbase)t)->b_name->u1.tp; goto px;
		case FCT:	t = ((Pfct)t)->returns; goto px; /* args? */
		}
	}
	}

	Cdcl = odcl;
	return nn;
}
