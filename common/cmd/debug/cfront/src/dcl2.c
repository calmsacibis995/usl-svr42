/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/dcl2.c	1.1"
/**************************************************************************

dcl2.c:
	Declaration of class, enum, and statement

*************************************************************************/

#include "cfront.h"
#include "size.h"

extern int tsize;
void classdef_dcl(this, cname, tbl)
Pclass this;
Pname cname;
Ptable tbl;
{
	Pname p, px, pnx, publist, it;
	Ptype cct;
	Pbase bt;
	Ptable btbl;
	Pclass bcl;
	Plist fl;
	int nmem, bvirt, i, fct_seen;
	int static_seen, local, scope, protect, byte_old;
	int bit_old, max_old, boff, in_union;
	int usz, make_ctor, make_dtor, waste;

	fct_seen = 0;
	static_seen = 0;
	local = tbl != gtbl;
	scope = PUBLIC;
	protect = 0;
	publist = 0;
	byte_old = byte_offset;
	bit_old = bit_offset;
	max_old = max_align;
	in_union = 0;
	make_ctor = 0;
	make_dtor = 0;

	/* this is the place for paranoia */
	if (this == 0){
		V1.u1.p = (char *)tbl;
		errorFIPC('i', "0->Cdef::dcl(%p)", &V1, ea0, ea0, ea0);
	}
	if (this->base != CLASS){
		V1.u1.i = (int)this->base;
		errorFIPC('i', "Cdef::dcl(%d)", &V1, ea0, ea0, ea0);
	}
	if (cname == 0) errorFIPC('i', "unNdC", ea0, ea0, ea0, ea0);
	if (cname->u1.tp != (Ptype)this)
		errorFIPC('i', "badCdef", ea0, ea0, ea0, ea0);
	if (tbl == 0){
		V1.u1.p = (char *)cname;
		errorFIPC('i', "Cdef::dcl(%n,0)", &V1, ea0, ea0, ea0);
	}
	if (tbl->base != TABLE){
		V1.u1.p = (char *)cname;
		V2.u1.i = (int)tbl->base;
		errorFIPC('i', "Cdef::dcl(%n,tbl=%d)", &V1, &V2, ea0, ea0);
	}
	nmem = no_of_names(this->mem_list) + no_of_names(this->pubdef);

	switch (this->csu){
	case UNION:
	case ANON:
		in_union = 1;
		if (this->virt_count) error("virtualF in union", ea0,ea0,ea0,ea0);
		break;
	case CLASS:
		scope = 0;
		if (this->virt_count == 0) this->csu = STRUCT; /* default simple */
	}

	if (this->clbase){
		if (this->clbase->base != TNAME){
			V1.u1.p = (char *)this->clbase;
			error("BC%nU", &V1, ea0, ea0, ea0);
		}
		this->clbase = ((Pbase)this->clbase->u1.tp)->b_name;
		bcl = (Pclass)this->clbase->u1.tp;
		if ((bcl->defined &(DEFINED|SIMPLIFIED)) == 0){
			V1.u1.p = (char *)this->clbase;
			error("BC%nU", &V1, ea0, ea0, ea0);
			goto nobase;
		}
		tbl = bcl->memtbl;
		if (tbl->base != TABLE){
			V1.u1.p = (char *)tbl;
			errorFIPC('i', "badBC table %p", &V1, ea0, ea0, ea0);
			goto nobase;
		}
		btbl = tbl;
		bvirt = bcl->virt_count;
		if (bcl->csu == UNION)
			error("C derived from union", ea0, ea0, ea0, ea0);
		if (in_union) error("derived union", ea0, ea0, ea0, ea0);
		if (this->pubbase == 0)this->csu = CLASS;
		boff = bcl->real_size;
		max_align = type_align((Ptype)bcl);
		this->bit_ass = bcl->bit_ass;
	}
	else {
	nobase:
		btbl = 0;
		bvirt = 0;
		boff = 0;
		tbl = gtbl;	/* class NOT in local scope */
		while (tbl!=gtbl && tbl->t_name)
			tbl = tbl->next;	/* nested classes */
		max_align = AL_STRUCT;
		if (this->virt_count == 0)
			this->bit_ass = 1;	/* can be bitwise copied */
	}

	Set_scope(this->memtbl, tbl);
	Set_name(this->memtbl, cname);
	if (nmem) grow(this->memtbl,(nmem<=2)? 3: nmem);

	stack();
	cc->not = cname;
	cc->cot = this;
	byte_offset = usz = boff;
	bit_offset = 0;

	bt = new_basetype(COBJ, cname);
	bt->b_table = this->memtbl;
	this->this_type = cc->tot = cct = (Ptype)new_ptr(PTR, bt, 0);
	PERM(cct);
	PERM(bt);

	for(p = this->mem_list ;p ;p = px) {
		Pname m;
		px = p->n_list;

		switch (p->base) {
		case PUBLIC:
			scope = PUBLIC;
			protect = 0;
			goto prpr;
		case PRIVATE:
			scope = 0;
			protect = 0;
			this->csu = CLASS;
			goto prpr;
		case PROTECTED:
			scope = 0;
			protect = PROTECTED;
			this->csu = CLASS;
		prpr:
			if (in_union){
				V1.u1.i = (int)p->base;
				error("%k label in unionD", &V1, ea0, ea0, ea0);
			}
			continue;
		}

		if (p->base == PR){	/* visibility control */
			p->base = NAME;
			if (scope != PUBLIC) {
				errorFIPC('s', "visibilityD not in public section",
					ea0, ea0, ea0, ea0);
				continue;
			}
			p->n_list = publist;
			publist = p;
			continue;
		}

		if (scope==0 || scope==PROTECTED) this->csu = CLASS;

		if (p->u1.tp->base == FCT){
			Pfct f;
			Pstmt b;

			f = (Pfct)p->u1.tp;
			b = f->body;
			f->body = 0;
			switch (p->n_sto){
			case AUTO:
			case STATIC:
			case REGISTER:
			case EXTERN:
				V1.u1.p = (char *)p;
				V2.u1.i = (int)p->n_sto;
				error("M%n cannot be%k", &V1, &V2, ea0, ea0);
				p->n_sto = 0;
			}
			m = name_dcl(p, this->memtbl, scope);
			if (m == 0 ) continue;
			if ( tsize ) { byte_offset += tsize; tsize = 0; }
			m->n_protect = protect;
			if (b) {
				if (m->u1.tp->defined &(DEFINED|SIMPLIFIED))
				{
					V1.u1.p = (char *)m;
					error("two definitions of%n", &V1,
						ea0, ea0, ea0);
				} else if (p->where.line != m->where.line)
				{
					V1.u1.p = (char *)p;
					errorFIPC('s',
					"previously declared%n cannot be defined inCD",
						&V1, ea0, ea0, ea0);
				} else 
					((Pfct)m->u1.tp)->body = b;
			}
			fct_seen = 1;
		}
		else {
			m = name_dcl(p, this->memtbl, scope);
			m->n_protect = protect;
			if (m){
				if (m->n_stclass == STATIC){
					static_seen = 1;
					m->n_sto = (tbl == gtbl)? 0: STATIC;
					if (m->u3.n_initializer){
						V1.u1.p = (char *)m;
						errorFIPC('s', "staticM%nWIr",
							&V1, ea0, ea0, ea0);
					}
				}
				if (in_union){
					if (usz < byte_offset)usz = byte_offset;
					byte_offset = 0;
				}
			}
		}
	}

	if (in_union) byte_offset = usz;

	if (this->virt_count || bvirt){	/* assign virtual indices */
		Pname vp[100];
		Pname n, nn;
		char s[128];

		nn = has_ctor(this);
		if (nn==0 || nn->u4.n_table!=this->memtbl) make_ctor = 1;
		sprintf(s,"%s__vtbl", this->string);
		n = new_name(s);
		n->u1.tp = Pfctvec_type;
		nn = insert(gtbl, n, 0);
		use(nn);

		if (this->virt_count = bvirt)
			for(i=0; i<bvirt; i++) vp[i] = bcl->virt_init[i];

for(nn=get_mem(this->memtbl,i=1) ;nn ;nn=get_mem(this->memtbl,++i)) {
	switch (nn->u1.tp->base){
	case FCT:
	{	Pfct f;
		Pname vpn;

		f = (Pfct)nn->u1.tp;
		if (bvirt) {
			Pname vn;
			Pfct vnf;
			Pgen g;

			vn = look(btbl, nn->u2.string, 0);
			if (!vn) goto vvv;

			/* match up with base class */
			if (vn->u4.n_table==gtbl) goto vvv;
			switch (vn->u1.tp->base){
			case FCT:
				vnf = (Pfct)vn->u1.tp;
				if (!vnf->f_virtual)
					goto vvv;
				if (type_check(vnf,f,0)){
					V1.u1.p = (char *)nn;
					V2.u1.p = (char *)f;
					V3.u1.p = (char *)vnf;
					error("virtual%nT mismatch:%t and%t",
					&V1, &V2, &V3, ea0);
				}
				f->f_virtual=vnf->f_virtual;
				vp[f->f_virtual-1]=nn;
				break;
			case OVERLOAD:
				g = (Pgen)vn->u1.tp;
				if (f->f_virtual
				||((Pfct)g->fct_list->f->u1.tp)->f_virtual) {
					V1.u1.p = (char *)nn;
					errorFIPC('s',
				"virtual%n overloaded inBC but not in derivedC",
						&V1, ea0, ea0, ea0);
				}
				break;
			default:
				goto vvv;
			}
		}
		else {
		vvv:
			if (!f->f_virtual) break;
			f->f_virtual = ++this->virt_count;
			switch (f->f_virtual){
			case 1:
				vpn = new_name("_vptr");
				vpn->u1.tp = Pfctvec_type;
				name_dcl(vpn,this->memtbl,PUBLIC);
				name__dtor(vpn);
			default:
				vp[f->f_virtual-1] = nn;
			}
		}
		break;
	}

	case OVERLOAD:
	{
		Plist gl;
		Pgen g;

		g = (Pgen)nn->u1.tp;

		if (bvirt){
			Pname vn;
			Pgen g2;
			Pfct f2;

			vn = look(btbl, nn->u2.string, 0);
			if (!vn) goto ovvv;
			if (vn->u4.n_table == gtbl)goto ovvv;
			switch (vn->u1.tp->base){
			default:
				goto ovvv;
			case FCT:
				f2 = (Pfct)vn->u1.tp;
				if (f2->f_virtual
				||((Pfct)g->fct_list->f->u1.tp)->f_virtual) {
					V1.u1.p = (char *)nn;
					errorFIPC('s', 
				"virtual%n overloaded in derivedC but not inBC",
						&V1, ea0, ea0, ea0);
				}
				break;
			case OVERLOAD:
				g2 = (Pgen)vn->u1.tp;

				for(gl = g->fct_list ;gl ;gl = gl->l) {
					Pname fn;
					Pfct f;
					Pname vn2;

					fn = gl->f;
					f = (Pfct)fn->u1.tp;
					vn2 = find(g2, f, 0);

					if (vn2 == 0){
						if (f->f_virtual){
							V1.u1.p = (char *)fn;
							errorFIPC('s',
					"virtual overloaded%n not found inBC",
								&V1, ea0, ea0, ea0);
						}
					}
					else {
						Pfct vn2f;

						vn2f = (Pfct)vn2->u1.tp;
						if (vn2f->f_virtual){
							f->f_virtual = vn2f->f_virtual;
							vp[f->f_virtual-1] = fn;
						}
					}
				}
				break;
			}
		}
		else {
		ovvv:
			for(gl=g->fct_list; gl; gl=gl->l) {
				Pname fn;
				Pfct f;

				fn = gl->f;
				f = (Pfct)fn->u1.tp;
				if (f->f_virtual) {
					f->f_virtual = ++this->virt_count;
					switch (f->f_virtual) {
					case 1:
					{
						Pname vpn;

						vpn = new_name("_vptr");
						vpn->u1.tp = Pfctvec_type;
						name_dcl(vpn, this->memtbl, 0);
						name__dtor(vpn);
					}
					default:
						vp[f->f_virtual-1] = fn;
					}
				}
			}
		}
		break;
	}
}
		}
		this->virt_init = (Pname *)new(sizeof(Pname) * this->virt_count);
		for(i=0; i<this->virt_count; i++) this->virt_init[i] = vp[i];
	}

	for(p = publist ;p ;p = pnx) {
		char *qs;
		char *ms;
		Pname cx;
		Ptable ctbl;
		Pname mx;

		qs = p->u5.n_qualifier->u2.string;
		ms = p->u2.string;
		pnx = p->n_list;
		if (strcmp(ms, qs)==0) ms = "_ctor";

		for(cx = this->clbase ;cx ;cx = ((Pclass)cx->u1.tp)->clbase) {
			if (strcmp(cx->u2.string, qs)==0) goto ok;
		}
		V1.u1.p = (char *)qs;
		error("publicQr %s not aBC", &V1, ea0, ea0, ea0);
		continue;
	ok:
		ctbl = (((Pclass)cx->u1.tp))->memtbl;
		mx = lookc(ctbl, ms);
		if (Ebase) {	/* cc->nof ?? */
			if (!has_friend(Ebase, cc->nof)){
				V1.u1.p = (char *)p;
				error("QdMN%n is in privateBC", &V1, ea0, ea0, ea0);
			}
		}
		else if (Epriv){
			if (!has_friend(Epriv, cc->nof)
			&& !(mx->n_protect && baseofname(Epriv, cc->nof))) {
				V1.u1.p = (char *)p;
				error("QdMN%n is private", &V1, ea0, ea0, ea0);
			}
		}

		if (mx == 0){
			V1.u1.p = (char *)cx;
			V2.u1.p = (char *)p->u2.string;
			error("C%n does not have aM %s", &V1, &V2, ea0, ea0);
			p->u1.tp = (Ptype)any_type;
		}
		else {
			if (mx->u1.tp->base == OVERLOAD){
				V1.u1.p = (char *)mx;
				errorFIPC('s',
					"public specification of overloaded%n",
					&V1, ea0, ea0, ea0);
			}
			p->base = PUBLIC;
		}

		p->u5.n_qualifier = mx;
		insert(this->memtbl, p, 0);
		if (Nold){
			V1.u1.p = (char *)p;
			error("twoDs of CM%n", &V1, ea0, ea0, ea0);
		}
	}

	if (bit_offset) byte_offset += (bit_offset/BI_IN_BYTE+1);
	this->real_size = byte_offset;
	if (byte_offset < SZ_STRUCT) byte_offset = SZ_STRUCT;
	waste = byte_offset % max_align;
	if (waste) byte_offset += max_align-waste;
	this->obj_size = byte_offset;
	this->obj_align = max_align;

	if ( has_dtor(this) && has_ctor(this)==0)
	{
		V1.u1.p = (char *)this->string;
		errorFIPC('w', "%s has destructor but noK", &V1, ea0, ea0, ea0);
	}
{	/* now look look at the members */
	Pclass oc;
	Pname m, ct, dt;
	int un;

	oc = this->in_class;
	ct = has_ctor(this);
	dt = has_dtor(this);
	un = (this->csu == UNION);

	if (ct && ct->u4.n_table != this->memtbl) ct = 0;
	if (dt && dt->u4.n_table != this->memtbl) dt = 0;

	if ( ct == 0 || dt == 0 || un )
	for(m=get_mem(this->memtbl, i=1); m; m=get_mem(this->memtbl,++i)) {
		Ptype t;
		Pname cn;
		Pclass cl;
		Pname ctor;
		Pname dtor;

		t = m->u1.tp;
		switch (t->base){
		default:
			if (ct == 0 && m->n_stclass != ENUM) {
				V1.u1.p = (char *)m;
				V2.u1.p = (char *)this->string;
				if (is_ref(t))
					error("R%n inC %sWoutK", &V1,&V2,ea0,ea0);
				if (tconst(t) && vec_const == 0)
					error("constant%n inC %sWoutK", &V1,
						&V2, ea0, ea0);
			}
			break;
		case FCT:
		case OVERLOAD:
		case CLASS:
		case ENUM:
			continue;
		case VEC:
			break;
		}
		cn = is_cl_obj(t);
		if (cn == 0)cn = cl_obj_vec;
		if (cn == 0)continue;

		cl = (Pclass)cn->u1.tp;
		if (cl->bit_ass == 0)this->bit_ass = 0; /* no bit copy */

		ctor = has_ctor(cl);
		dtor = has_dtor(cl);

		V1.u1.p = (char *)m;
		V2.u1.p = (char *)cn;
		if (ctor) {
			if (m->n_stclass==STATIC)
				errorFIPC('s', "staticM%n ofC%nWK",&V1,&V2,ea0,ea0);
			else if (un)
				error("M%n ofC%nWK in union", &V1, &V2, ea0, ea0);
			else if (ct == 0) make_ctor = 1;
			}

		if (dtor) {
			if (m->n_stclass==STATIC)
				errorFIPC('s', "staticM%n ofC%nW destructor",
					&V1, &V2, ea0, ea0);
			else if (un)
				error("M%n ofC%nW destructor in union", &V1,
					&V2, ea0, ea0);
			else if (dt == 0) make_dtor = 1;
			}
		}
	}

	if (make_ctor) {
		Pname ct;
		ct = has_ctor(this);
		if (ct==0 || ct->u4.n_table!=this->memtbl) {
			/* make a constructor for the class: x::x() {}
			 * a base class's constructor is not good enough
			 */
			Pname n, m;
			Pfct f;

			if (ct && has_ictor(this)==0) {
				V1.u1.i = (int)this->csu;
				V2.u1.p = (char *)this->string;
				error("%k %s needs aK", &V1, &V2, ea0, ea0);
			}
			n = new_name(this->string);
			f = fct_ctor((Ptype)defa_type, 0, 1);
			n->u1.tp = (Ptype)f;
			n->n_oper = TNAME;
			m = name_dcl(n, this->memtbl, PUBLIC);
			((Pfct)m->u1.tp)->body = new_block(curloc, 0, 0);
		}
	}

	if (make_dtor && has_dtor(this) == 0) {
		/* make a destructor for the class: x::x() {} */
		Pname n, m;
		Pfct f;

		n = new_name(this->string);
		f = fct_ctor((Ptype)defa_type, 0, 1);
		n->u1.tp = (Ptype)f;
		n->n_oper = DTOR;
		m = name_dcl(n, this->memtbl, PUBLIC);
		((Pfct)m->u1.tp)->body = new_block(curloc, 0, 0);
	}

	this->defined |= DEFINED;

	it = has_itor(this);
	for(p=get_mem(this->memtbl, i=1) ;p ;p = get_mem(this->memtbl,++i)) {
		/* define members defined inline */
		switch (p->u1.tp->base){
		case FCT:
		{	Pfct f;

			f = (Pfct)p->u1.tp;
			if (f->body) {
				f->f_inline = 1;
				p->n_sto = STATIC;
				fct_dcl(f, p);
			}
			break;
		}
		case OVERLOAD:
		{	Pgen g;
			Plist gl;

			g = (Pgen)p->u1.tp;
			for(gl = g->fct_list; gl ;gl = gl->l) {
				Pname n;
				Pfct f;

				n = gl->f;
				f = (Pfct)n->u1.tp;
				if (f->body) {
					f->f_inline = 1;
					n->n_sto = STATIC;
					fct_dcl(f, n);
				}
			}
		}
		}
	}

	/* define friends defined inline and modify return types if necessary */
	for(fl = this->friend_list; fl ;fl = fl->l) {
		Pname p;

		p = fl->f;
		switch (p->u1.tp->base){
		case FCT:
		{	Pfct f;

			f = (Pfct)p->u1.tp;

			if (f->body
			&& (f->defined&(DEFINED|SIMPLIFIED)) == 0) {
				f->f_inline = 1;
				p->n_sto = STATIC;
				fct_dcl(f, p);
			}
			break;
		}
		case OVERLOAD:
		{	Pgen g;
			Plist gl;

			g = (Pgen)p->u1.tp;
			for(gl = g->fct_list ;gl ;gl = gl->l) {
				Pname n;
				Pfct f;

				n = gl->f;
				f = (Pfct)n->u1.tp;

				if (f->body
				&& (f->defined&(DEFINED|SIMPLIFIED)) == 0) {
					f->f_inline = 1;
					n->n_sto = STATIC;
					fct_dcl(f, n);
				}
			}
		}
		}
	}

	byte_offset = byte_old;
	bit_offset = bit_old;
	max_align = max_old;

	unstack();
}

#define FIRST_ENUM 0

void enumdef_dcl(this, tbl)
Penum this;
Ptable tbl;
{
	int nmem, enum_old;
	Pname p, ns, nl, px;

	nmem = no_of_names(this->mem);
	ns = 0;
	nl = 0;
	enum_old = enum_count;
	this->no_of_enumerators = nmem;

	enum_count = FIRST_ENUM;

	if (this == 0){
		V1.u1.p = (char *)tbl;
		errorFIPC('i', "0->enumdef::dcl(%p)", &V1, ea0, ea0, ea0);
	}
	for(p=this->mem, this->mem=0; p; p=px) {
		Pname nn;
		px = p->n_list;
		if (p->u3.n_initializer){
			Pexpr i;

			i = expr_typ(p->u3.n_initializer, tbl);
			Neval = 0;
			enum_count = eval(i);
			if (Neval){
				V1.u1.p = (char *)Neval;
				error("%s", &V1, ea0, ea0, ea0);
			}
			if (TO_DEL(i)) expr_del(i);
			p->u3.n_initializer = 0;
		}
		p->n_evaluated = 1;
		p->n_val = enum_count++;
		nn = insert(tbl, p, 0);
		if (Nold){
			V1.u1.p = (char *)nn;
			if (nn->n_stclass == ENUM)
				errorFIPC((p->n_val!=nn->n_val)?0:'w',
					"enumerator%n declared twice",
					&V1, ea0, ea0, ea0);
			else 
				error("incompatibleDs of%n", &V1, ea0, ea0, ea0);
		}
		else {
			nn->n_stclass = ENUM;	/* no store will be allocated */
			if (ns)
				nl->n_list = nn;
			else 
				ns = nn;
			nl = nn;
		}
		name__dtor(p);
	}

	this->mem = ns;

	enum_count = enum_old;
	this->defined |= DEFINED;
}

Pstmt curr_loop;
Pstmt curr_switch;
Pstmt curr_block;

void reached(this)
Pstmt this;
{
	register Pstmt ss;

	ss = this->s_list;

	if (ss == 0) return;

	switch (ss->base){
	case LABEL:
	case CASE:
	case DEFAULT:
		break;
	default:
		errorFIPC('w', "S not reached", ea0, ea0, ea0, ea0);
		for(; ss; ss=ss->s_list) {	/* delete unreacheable code */
			switch (ss->base) {
			case LABEL:
			case CASE:
			case DEFAULT:		/* reachable */
				this->s_list = ss;
				return;
			case DCL:		/* the dcl may be used later */
						/* keep to avoid cascading errors */
			case IF:
			case DO:
			case WHILE:
			case SWITCH:
			case FOR:
			case BLOCK:		/* may hide a label */
				this->s_list = ss;
				return;
			}
		}
		this->s_list = 0;
	}
}

bit arg_err_suppress;

Pexpr check_cond(e, b, tbl)
Pexpr e;
TOK b;
Ptable tbl;
{
	Pname cn;
	if (cn = is_cl_obj(e->u1.tp)){
		Pclass cl;
		int i;
		Pname found;
		Pname on;

		cl = (Pclass)cn->u1.tp;
		i = 0;
		found = 0;
		for(on = cl->conv ;on ;on = on->n_list) {
			Pfct f;
			Ptype t;

			f = (Pfct)on->u1.tp;
			t = f->returns;
		xx:
			switch (t->base) {
			case TYPE:
				t = ((Pbase)t)->b_name->u1.tp;
				goto xx;
			case FLOAT:
			case DOUBLE:
			case PTR:
				if (b == DEREF) break;
			case CHAR:
			case SHORT:
			case INT:
			case LONG:
			case EOBJ:
				i++;
				found = on;
			}
		}
		switch (i) {
		case 0:
			V1.u1.p = (char *)cn;
			V2.u1.i = (int)b;
			error("%nO in%kE", &V1, &V2, ea0, ea0);
			return e;
		case 1:
		{
			Pclass cl;
			Pexpr r, c;

			cl = (Pclass)cn->u1.tp;
			r = new_ref(DOT, e, found);
			r->u1.tp = found->u1.tp;
			c = new_expr(0, G_CALL, r, 0);
			c->u4.fct_name = found;
			return expr_typ(c, tbl);
		}
		default:
			V1.u1.i = i;
			V2.u1.p = (char *)cn;
			V3.u1.i = (int)b;
			error("%d possible conversions for%nO in%kE",
				&V1, &V2, &V3, ea0);
			return e;
		}
	}
	if (num_ptr(e->u1.tp, b) == FCT){
		V1.u1.i = (int)b;
		error("%k(F)", &V1, ea0, ea0, ea0);
	}
	return e;
}

/*
 *	typecheck statement "this" in scope "curr_block->tbl"
 */
void stmt_dcl(this)
Pstmt this;
{
	Pstmt ss, ostmt;
	Pname n, nn;

	ostmt = Cstmt;
	for(ss=this; ss; ss=ss->s_list) {
		Pstmt old_loop, old_switch;
		Ptable tbl;

		Cstmt = ss;
		tbl = curr_block->memtbl;

		switch (ss->base){
		case BREAK:
			if (curr_loop==0 && curr_switch==0)
			{
				V1.u1.i = BREAK;
				error("%k not in loop or switch", &V1,ea0,ea0,ea0);
			}
			reached(ss);
			break;

		case CONTINUE:
			if (curr_loop == 0) {
				V1.u1.i = CONTINUE;
				error("%k not in loop", &V1, ea0, ea0, ea0);
			}
			reached(ss);
			break;

		case DEFAULT:
			if (curr_switch == 0) {
				error("default not in switch", ea0, ea0, ea0, ea0);
				break;
			}
			if (curr_switch->u1.has_default)
				error("two defaults in switch", ea0,ea0,ea0,ea0);

			curr_switch->u1.has_default = ss;
			ss->s->s_list = ss->s_list;
			ss->s_list = 0;
			stmt_dcl(ss->s);
			break;

		case SM:
		{
			TOK b;

			b = ss->u2.e->base;
			switch (b) {
			case DUMMY:
				ss->u2.e = 0;
				break;
					/* check for unused results
					 * don't check operators that are likely
					 * to be overloaded to represent "actions":
					 * ! ~ < <= > >= << >>
					 */
			case EQ:
			case NE:
			case PLUS:
			case MINUS:
			case REF:
			case DOT:
			case MUL:
			case DIV:
			case ADDROF:
			case AND:
			case OR:
			case ER:
			case DEREF:
			case ANDAND:
			case OROR:
			case NAME:
				if (ss->u2.e->u1.tp)
					break; /* avoid looking at generated code */
				ss->u2.e = expr_typ(ss->u2.e, tbl);
				if (ss->u2.e->base == CALL)break;
				if (ss->u2.e->u1.tp->base != VOID){
					V1.u1.i = (int)b;
					errorFIPC('w', "result of%kE not used",
						&V1, ea0, ea0, ea0);
				}
				break;
			default:
				ss->u2.e = expr_typ(ss->u2.e, tbl);
			}
			break;
		}
		case RETURN:
		{
			Pname fn;
			Pfct f;
			Ptype rt;
			Pexpr v;

			fn = cc->nof;
			f = (Pfct)fn->u1.tp;
			rt = f->returns;
			v = ss->u2.e;
			if (v != dummy) {
				if (rt->base == VOID) {
					errorFIPC('w', "unexpected return value",
						ea0, ea0, ea0, ea0);
					/*refuse to return the value:*/
					ss->u2.e = dummy;
				}
				else {
					v = expr_typ(v, tbl);
				lx:
					switch (rt->base) {
					case TYPE:
						rt = ((Pbase)rt)->b_name->u1.tp;
						goto lx;
					case RPTR:
						if (lval(v,0)==0
						&& tconst(v->u1.tp)==0)
							errorFIPC('w', 
							"R to non-lvalue returned",
								ea0, ea0, ea0, ea0);
						else if (v->base==NAME
						&&((Pname)v)->n_scope==FCT)
							errorFIPC('w',
							"R to localV returned",
								ea0, ea0, ea0, ea0);
						v = ref_init(((Pptr)rt), v, tbl);

					case ANY:
						break;
					case COBJ:
if (v->base == DEREF) {
	Pexpr v1;

	v1 = v->u2.e1;
	if (v1->base==CAST) {
		Pexpr v2;

		v2 = v1->u2.e1;
		if (v2->base == G_CM){ /* *(T)(e1,e2) =>(e1,*(T)e2) */
			Pexpr v3;

			v3 = v2->u3.e2;
			v2->u3.e2 = v;
			v2->u1.tp = v->u1.tp;
			v = v2;
			v1->u2.e1 = v3;
		}
	}
}
	if (f->f_result) {
		if (type_check(rt,v->u1.tp,ASSIGN)==0 && v->base==G_CM)
			v = replace_temp(v,(Pexpr)f->f_result);
		else {
			Pname rcn;

			v = class_init(contents((Pexpr)f->f_result), rt, v, tbl);
			rcn = is_cl_obj(rt);
			if (has_itor((Pclass)rcn->u1.tp)==0) {
				/* can happen for virtuals and for
				 * user defined conversions
				 */
				v->u1.tp = rt;
				v=new_expr(0,ASSIGN,contents((Pexpr)f->f_result),v);
				v->u1.tp = rt;
			}
		}
	}
	else
		v = class_init(0, rt, v, tbl);
						break;
					case PTR:
						v = ptr_init((Pptr)rt, v, tbl);
						goto def;
					case INT:
					case CHAR:
					case LONG:
					case SHORT:
						if (((Pbase)rt)->b_unsigned
						&& v->base==UMINUS
						&& v->u3.e2->base==ICON)
							errorFIPC('w',
					"negative retured fromF returning unsigned",
								ea0, ea0, ea0, ea0);
					default:
					def:
						if (type_check(rt,v->u1.tp,ASSIGN)){
							Pexpr x;

							x = try_to_coerce(rt,v,"return value",tbl);
							if (x)
								v = x;
							else 
							{
								V1.u1.p = (char *)fn;
								V2.u1.p = (char *)v->u1.tp;
								V3.u1.p = (char *)rt;
								error("bad return valueT for%n:%t(%tX)",
								&V1,&V2,&V3,ea0);
							}
						}
					}
					ss->u1.ret_tp = rt;
					ss->u2.e = v;
				}
			}
			else {
				if (rt->base != VOID)
					errorFIPC('w', "return valueX",
						ea0, ea0, ea0, ea0);
			}
			reached(ss);
			break;
		}

		case DO:	/* in DO the stmt is before the test */
			inline_restr |= 8;
			old_loop = curr_loop;
			curr_loop = ss;
			if (ss->s->base == DCL)
				errorFIPC('s', "D as onlyS in do-loop", ea0,
					ea0, ea0, ea0);
			stmt_dcl(ss->s);
			ss->u2.e = expr_typ(ss->u2.e, tbl);
			ss->u2.e = check_cond(ss->u2.e, DO, tbl);
			curr_loop = old_loop;
			break;

		case WHILE:
			inline_restr |= 8;
			old_loop = curr_loop;
			curr_loop = ss;
			ss->u2.e = expr_typ(ss->u2.e, tbl);
			ss->u2.e = check_cond(ss->u2.e, WHILE, tbl);
			if (ss->s->base == DCL)
				errorFIPC('s', "D as onlyS in while-loop",
					ea0, ea0, ea0, ea0);
			stmt_dcl(ss->s);
			curr_loop = old_loop;
			break;

		case SWITCH:
		{
			int ne;
			Ptype tt;

			ne = 0;
			inline_restr |= 4;
			old_switch = curr_switch;
			curr_switch = ss;
			ss->u2.e = expr_typ(ss->u2.e, tbl);
			ss->u2.e = check_cond(ss->u2.e, SWITCH, tbl);
			tt = ss->u2.e->u1.tp;
		sii:
			switch (tt->base) {
			case TYPE:
				tt = ((Pbase)tt)->b_name->u1.tp;
				goto sii;
			case EOBJ:
				ne = ((Penum)((Pbase)tt)->b_name->u1.tp)->no_of_enumerators;
			case ZTYPE:
			case ANY:
			case CHAR:
			case SHORT:
			case INT:
			case LONG:
			case FIELD:
				break;
			default:
				V1.u1.p = (char *)ss->u2.e->u1.tp;
				error("%t switchE must be converted to int", 
					&V1, ea0, ea0, ea0);
				}
			stmt_dcl(ss->s);
			if (ne) {	/* see if the number of cases is "close to"
					 * but not equal to the number of enumerators
					 */
				int i;
				Pstmt cs;

				i = 0;
				for(cs=ss->u3.case_list; cs; cs=cs->u3.case_list)
					i++;
				if (i && i!=ne) {
					if (ne < i) {
				ee:
						V1.u1.p = (char *)ss->u2.e->u1.tp;
						V2.u1.i = i;
						V3.u1.i = ne;
						errorFIPC('w', 
					"switch (%t)W %d cases(%d enumerators)", 
							&V1, &V2, &V3, ea0);
					}
					else {
						switch (ne-i) {
						case 1: if (3<ne) goto ee;
						case 2: if (7<ne) goto ee;
						case 3: if (23<ne) goto ee;
						case 4: if (60<ne) goto ee;
						case 5: if (99<ne) goto ee;
						}
					}
				}
			}
			curr_switch = old_switch;
			break;
		}
		case CASE:
			if (curr_switch == 0) {
				error("case not in switch", ea0, ea0, ea0, ea0);
				break;
			}
			ss->u2.e = expr_typ(ss->u2.e, tbl);
			num_ptr(ss->u2.e->u1.tp, CASE);
			{	Ptype tt;
				tt = ss->u2.e->u1.tp;
			iii:
				switch (tt->base) {
				case TYPE:
					tt = ((Pbase)tt)->b_name->u1.tp;
					goto iii;
				case ZTYPE:
				case ANY:
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
					break;
				default:
					V1.u1.p = (char *)ss->u2.e->u1.tp;
					errorFIPC('s', "%t caseE", &V1,ea0,ea0,ea0);
				}
			}
			{
				int i;

				Neval = 0;
				i = eval(ss->u2.e);
				if (Neval == 0) {
					Pstmt cs;
					cs = curr_switch->u3.case_list;
					for(;cs ;cs=cs->u3.case_list) {
						if (cs->u1.case_value == i){
							V1.u1.i = i;
							error("case %d used twice in switch",
								&V1, ea0, ea0, ea0);
						}
					}
					ss->u1.case_value = i;
					ss->u3.case_list = curr_switch->u3.case_list;
					curr_switch->u3.case_list = ss;
				}
				else 
				{
					V1.u1.p = (char *)Neval;
					error("bad case label: %s",
						&V1, ea0, ea0, ea0);
				}
			}

			if (ss->s->s_list){
				V1.u1.i = (int)ss->s->s_list->base;
				errorFIPC('i', "case%k", &V1, ea0, ea0, ea0);
			}
			ss->s->s_list = ss->s_list;
			ss->s_list = 0;
			stmt_dcl(ss->s);
			break;

		case GOTO:
			inline_restr |= 2;
			reached(ss);
		case LABEL:
			/* Insert label in function mem table;
			 *   labels have function scope.
			 */
			n = ss->u1.d;
			nn = insert(cc->ftbl, n, LABEL);

			/* Set a ptr to the mem table corresponding to the scope
			 * in which the label actually occurred.  This allows the
			 * processing of goto's in the presence of ctors and dtors
			 */
			if (ss->base == LABEL){
				nn->u5.n_realscope = curr_block->memtbl;
				inline_restr |= 1;
			}

			if (Nold) {
				if (ss->base == LABEL) {
					if (nn->u3.n_initializer){
						V1.u1.p = (char *)n;
						error("twoDs of label%n",
							&V1, ea0, ea0, ea0);
					}
					nn->u3.n_initializer = (Pexpr)1;
				}
				if (n != nn) ss->u1.d = nn;
			}
			else {
				if (ss->base == LABEL)
					nn->u3.n_initializer = (Pexpr)1;
				nn->where = ss->where;
			}
			if (ss->base == GOTO)
				use(nn);
			else {
				if (ss->s->s_list){
					V1.u1.i = (int)ss->s->s_list->base;
					errorFIPC('i', "label%k", &V1,ea0,ea0,ea0);
				}
				ss->s->s_list = ss->s_list;
				ss->s_list = 0;
				assign(nn);
			}
			if (ss->s) stmt_dcl(ss->s);
			break;

		case IF:
		{
			Pexpr ee;

			ee = expr_typ(ss->u2.e, tbl);
			if (ee->base == ASSIGN) {
				Neval = 0;
				eval(ee->u3.e2);
				if (Neval == 0)
					errorFIPC('w',
						"constant assignment in condition",
						ea0, ea0, ea0, ea0);
			}
			ss->u2.e = ee = check_cond(ee, IF, tbl);
			switch (ee->u1.tp->base){
			case INT:
			case ZTYPE:
			{
				int i;
				Neval = 0;
				i = eval(ee);

				if (Neval == 0) {
					Pstmt sl;

					sl = ss->s_list;
					if (i) {
						if (TO_DEL(ss->u3.else_stmt))
							stmt_del(ss->u3.else_stmt);
						stmt_dcl(ss->s);
						*ss = *ss->s;
					}
					else {
						if (TO_DEL(ss->s)) stmt_del(ss->s);
						if (ss->u3.else_stmt){
							stmt_dcl(ss->u3.else_stmt);
							*ss = *ss->u3.else_stmt;
						}
						else {
							ss->base = SM;
							ss->u2.e = dummy;
							ss->s = 0;
						}
					}
					ss->s_list = sl;
					continue;
				}
			}
			}
			stmt_dcl(ss->s);
			if (ss->u3.else_stmt) stmt_dcl(ss->u3.else_stmt);
			break;
		}
		case FOR:
			inline_restr |= 8;
			old_loop = curr_loop;
			curr_loop = ss;
			if (ss->u3.for_init){
				Pstmt fi;

				fi = ss->u3.for_init;
				switch (fi->base) {
				case SM:
					if (fi->u2.e == dummy){
						ss->u3.for_init = 0;
						break;
					}
				default:
					stmt_dcl(fi);
					break;
				case DCL:
					stmt_dcl(fi);

					switch (fi->base){
					case BLOCK:
					{
						/* { ... for( { a } b ; c) d ; e }
						 *	=>
						 * { ... { a for( ; b ; c) d ; e }}
						 */
						Pstmt tmp;

						tmp = new_stmt(0,SM,curloc,0);
						*tmp = *ss;	/* tmp = for */
						tmp->u3.for_init = 0;
						*ss = *fi;	/* ss = { } */
						if (ss->s)
							ss->s->s_list = tmp;
						else 
							ss->s = tmp;
						curr_block = (Pstmt)ss;
						tbl = curr_block->memtbl;
						Cstmt = ss = tmp;
						/* rest of for and s_list */
						break;
					}
					}
				}
			}
			if (ss->u2.e == dummy)
				ss->u2.e = 0;
			else {
				ss->u2.e = expr_typ(ss->u2.e, tbl);
				ss->u2.e = check_cond(ss->u2.e, FOR, tbl);
			}
			if (ss->s->base == DCL)
				errorFIPC('s', "D as onlyS in for-loop",
					ea0, ea0, ea0, ea0);
			stmt_dcl(ss->s);
			ss->u1.e2 = (ss->u1.e2==dummy)? 0 : expr_typ(ss->u1.e2,tbl);
			curr_loop = old_loop;
			break;

		case DCL:	/* declaration after statement */
		{
			/*	collect all the contiguous DCL nodes from the
			 * 	head of the s_list. find the next statement
			 */
			int non_trivial;
			int count;
			Pname tail, nn;
			Pstmt next_st;

			non_trivial = 0;
			count = 0;
			tail = ss->u1.d;
			for(nn = tail; nn ;nn = nn->n_list) {
				/* find tail;
				 * detect non-trivial declarations
				 */
				Pname n, cln;
				Pexpr in;

				count++;
				if (nn->n_list) tail = nn->n_list;
				n = look(tbl, nn->u2.string, 0);
				if (n && n->u4.n_table == tbl) non_trivial = 2;
				if (non_trivial == 2) continue;
				if ((nn->n_sto == STATIC && nn->u1.tp->base!=FCT)
				|| is_ref(nn->u1.tp)
				|| (tconst(nn->u1.tp) && fct_const==0)) {
					non_trivial = 2;
					continue;
				}
				in = nn->u3.n_initializer;
				if (in)
					switch (in->base) {
					case ILIST:
					case STRING:
						non_trivial = 2;
						continue;
					default:
						non_trivial = 1;
					}
				cln = is_cl_obj(nn->u1.tp);
				if (cln == 0) cln = cl_obj_vec;
				if (cln == 0) continue;
				if (has_ctor((Pclass)cln->u1.tp)) {
					non_trivial = 2;
					continue;
				}
				if (has_dtor((Pclass)cln->u1.tp)) non_trivial = 2;
			}

			while (ss->s_list && ss->s_list->base==DCL) {
				Pstmt sx;
				Pexpr in;
				Pname cln, n;

				sx = ss->s_list;
				tail = tail->n_list = sx->u1.d;	/* add to tail */
				for(nn=sx->u1.d ;nn ;nn = nn->n_list) {
					/* find tail;
					 * detect non-trivial declarations
					 */
					count++;
					if (nn->n_list)tail = nn->n_list;
					n = look(tbl, nn->u2.string, 0);
					if (n && n->u4.n_table == tbl)
						non_trivial = 2;
					if (non_trivial == 2) continue;
					if ((nn->n_sto==STATIC && nn->u1.tp->base!=FCT)
					|| is_ref(nn->u1.tp)
					|| (tconst(nn->u1.tp) && fct_const == 0)) {
						non_trivial = 2;
						continue;
					}
					in = nn->u3.n_initializer;
					if (in)
						switch (in->base) {
						case ILIST:
						case STRING:
							non_trivial = 2;
							continue;
						}
					non_trivial = 1;
					cln = is_cl_obj(nn->u1.tp);
					if (cln == 0) cln = cl_obj_vec;
					if (cln == 0) continue;
					if (has_ctor((Pclass)cln->u1.tp)) {
						non_trivial = 2;
						continue;
					}
					if (has_dtor((Pclass)cln->u1.tp)) continue;
				}
				ss->s_list = sx->s_list;
			}
			next_st = ss->s_list;
			if (non_trivial==2	/* must */
			||(non_trivial==1	/* might */
				&&( curr_block->u2.own_tbl==0	/* why not? */
				     || inline_restr&3		/* label seen */
				   )
			   )
			) {
				if (curr_switch && non_trivial==2) {
					Pstmt cs, ds, bl;

					cs = curr_switch->u3.case_list;
					ds = curr_switch->u1.has_default;
					if (cs == 0)
						bl = ds;
					else if (ds == 0)
						bl = cs;
					else if (cs->where.line<ds->where.line)
						bl = ds;
					else 
						bl = cs;
					if ((bl==0 || bl->s->base!=BLOCK)
					&& curr_switch->s->memtbl==tbl)
						errorFIPC('s', 
			"non trivialD in switchS(try enclosing it in a block)",
							ea0, ea0, ea0, ea0);
				}
				/*	Create a new block,
				 *	put all the declarations at the head,
				 *	and the remainder of the slist as the
				 *	statement list of the block.
				 */
				ss->base = BLOCK;

				/*	check that there are no redefinitions
				 *	since the last "real"(user-written,
				 *	non-generated) block
				 */
				for(nn = ss->u1.d ;nn ;nn = nn->n_list) {
					Pname n;
					if (curr_block->u2.own_tbl
					&&(n=look(curr_block->memtbl,nn->u2.string,0))
					&& n->u4.n_table->real_block==curr_block->memtbl->real_block)
					{
						V1.u1.p = (char *)n;
						error("twoDs of%n",&V1,ea0,ea0,ea0);
					}
				}

				/*	attach the remainder of the s_list
				 *	as the statement part of the block.
				 */
				ss->s = next_st;
				ss->s_list = 0;

				/*	create the table in advance, in order to
				 *	set the real_block ptr to that of the
				 *	enclosing table
				 */
				ss->memtbl = new_table(count+4, tbl, 0);
				ss->memtbl->real_block = curr_block->memtbl->real_block;

				block_dcl((Pstmt)ss, ss->memtbl);
			}
			else {	/*	to reduce the number of symbol tables,
				 *	do not make a new block,
				 *	instead insert names in enclosing block,
				 *	and make the initializers into expression
				 *	statements.
				 */
				Pstmt sss;

				sss = ss;
				for(nn = ss->u1.d ;nn ;nn = nn->n_list) {
					Pname n;
					Pexpr in;

					n = name_dcl(nn, tbl, FCT);
					if (n == 0) continue;
					in = n->u3.n_initializer;
					n->u3.n_initializer = 0;
					if (ss) {
						sss->base = SM;
						ss = 0;
					}
					else 
						sss = sss->s_list = new_estmt(SM,sss->where,0,0);
					if (in){
						switch (in->base){
						case G_CALL:	/* constructor? */
						{
							Pname fn;

							fn = in->u4.fct_name;
							if (fn && fn->n_oper==CTOR) break;
						}
						default:
							in = new_expr(0,ASSIGN,(Pexpr)n,in);
						}
						sss->u2.e = expr_typ(in, tbl);
					}
					else 
						sss->u2.e = dummy;
				}
				ss = sss;
				ss->s_list = next_st;
			}
			break;
		}

		case BLOCK:
			block_dcl((Pstmt)ss, tbl);
			break;

		case ASM:
			/* save string */
		{
			char *s;
			int ll;
			char *s2;

			s = (char *)ss->u2.e;
			ll = strlen(s);
			s2 = (char *) new(ll+1);
			strcpy(s2, s);
			ss->u2.e = (Pexpr)s2;
			break;
		}

		default:
			V1.u1.p = (char *)ss;
			V2.u1.i = (int)ss->base;
				errorFIPC('i', "badS(%p %d)", &V1, &V2, ea0, ea0);
		}
	}

	Cstmt = ostmt;
}

extern int in_class_dcl;

/*
 *	Note: for a block without declarations memtbl denotes the table
 *	for the enclosing scope.
 *	A function body has its memtbl created by fct::dcl().
 */
void block_dcl(this, tbl)
Pstmt this;
Ptable tbl;
{
	int bit_old;
	int byte_old;
	int max_old;
	Pstmt block_old;

	bit_old = bit_offset;
	byte_old = byte_offset;
	max_old = max_align;
	block_old = curr_block;

	if (this->base != BLOCK){
		V1.u1.i = (int)this->base;
		errorFIPC('i', "block::dcl(%d)", &V1, ea0, ea0, ea0);
	}
	curr_block = this;

	if (this->u1.d){
		Pname nx, n;

		this->u2.own_tbl = 1;
		if (this->memtbl == 0){
			int nmem;

			nmem = no_of_names(this->u1.d)+4;
			this->memtbl = new_table(nmem, tbl, 0);
			this->memtbl->real_block = (Pstmt)this;
			/*	this is a "real" block from the
			 *	source text, and not one created by DCL's
			 *	inside a block.
			 */
		}
		else 
			if (this->memtbl != tbl)
				errorFIPC('i', "block::dcl(?)", ea0,ea0,ea0,ea0);

		for(n = this->u1.d; n; n = nx) {
			nx = n->n_list;
			name_dcl(n, this->memtbl, FCT);
			switch (n->u1.tp->base){
			case CLASS:
			case ANON:
			case ENUM:
				break;
			default:
				name__dtor(n);
			}
		}
	}
	else
		this->memtbl = tbl;

	if (this->s){
		Pname odcl, m;
		int i;

		odcl = Cdcl;
		stmt_dcl(this->s);
		if (this->u2.own_tbl) {
			m = get_mem(this->memtbl,i=1);
			for(;m ;m=get_mem(this->memtbl,++i)) {
				Ptype t;

				t = m->u1.tp;
				if (in_class_dcl) m->lex_level -= 1;
				if (t == 0){
					V1.u1.p = (char *)m->u2.string;
					if (m->n_assigned_to == 0)
						error("label %sU", &V1,ea0,ea0,ea0);
					if (m->n_used == 0)
						errorFIPC('w', "label %s not used",
						 &V1, ea0, ea0, ea0);
					continue;
				}
			ll:
				switch (t->base){
				case TYPE:
					t = ((Pbase)t)->b_name->u1.tp;
					goto ll;
				case CLASS:
				case ENUM:
				case FCT:
				case VEC:
					continue;
				}

				if (m->n_addr_taken == 0) {
					if (m->n_used) {
						if (!m->n_assigned_to){
							switch (m->n_scope){
							case FCT:
								Cdcl = m;
								V1.u1.p = (char *)m;
								errorFIPC('w',
								"%n used but not set"
								,&V1,ea0,ea0,ea0);
							}
						}
					}
					else {
						if (!m->n_assigned_to){
							switch (m->n_scope){
							case ARG:
								if (m->u2.string[0]=='_'
								&& m->u2.string[1]=='A')
									break;
							case FCT:
								Cdcl = m;
								V1.u1.p = (char *)m;
								errorFIPC('w',
								"%n not used",
								&V1,ea0,ea0,ea0);
							}
						}
					}
				}
			}
		}
		Cdcl = odcl;
	}

	this->u1.d = 0;

	if (bit_offset) byte_offset += SZ_WORD;
	if (stack_size < byte_offset) stack_size = byte_offset;
	bit_offset = bit_old;
	byte_offset = byte_old;
	curr_block = block_old;
}

/* adjust alignment */
void field_align(this)
Pname this;
{
	Pbase fld;
	int a, x;

	fld = (Pbase)this->u1.tp;
	a = (F_SENSITIVE) ? type_align(fld->b_fieldtype) : SZ_WORD;
	if (max_align < a) max_align = a;

	if (fld->b_bits == 0){		/* force word alignment */
		int b;
		if (bit_offset)
			fld->b_bits = BI_IN_WORD - bit_offset;
		else if (b = byte_offset%SZ_WORD)
			fld->b_bits = b * BI_IN_BYTE;
		else 
			fld->b_bits = BI_IN_WORD;
		if (max_align < SZ_WORD) max_align = SZ_WORD;
	}
	else if (bit_offset == 0) {	/* take care of part of word */
		int b;

		b = byte_offset%SZ_WORD;
		if (b) {
			byte_offset -= b;
			bit_offset = b*BI_IN_BYTE;
		}
	}

	x = (bit_offset += fld->b_bits);
	if (BI_IN_WORD < x) {
		fld->b_offset = 0;
		byte_offset += SZ_WORD;
		bit_offset = fld->b_bits;
	}
	else {
		fld->b_offset = bit_offset;
		if (BI_IN_WORD == x){
			bit_offset = 0;
			byte_offset += SZ_WORD;
		}
		else 
			bit_offset = x;
	}
	this->n_offset = byte_offset;
}
