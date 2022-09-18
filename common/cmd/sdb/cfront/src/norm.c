/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/norm.c	1.2"
/************************************************************************

norm.c:

	"normalization" handles problems which could have been handled
	by the syntax analyser; but has not been done. The idea is
	to simplify the grammar and the actions accociated with it,
	and to get a more robust error handling

****************************************************************************/

#include "cfront.h"
#include "size.h"

extern void syn_init();

static Penum	new_enum();
static Pclass	new_classdef();
static Pname	name_tdef();

void syn_init()
{
	any_type = new_basetype(ANY, 0);
	PERM(any_type);
	dummy = new_expr(0, DUMMY, 0, 0);
	PERM(dummy);
	dummy->u1.tp = (Ptype)any_type;
	zero = new_expr(0, ZERO, 0, 0);
	PERM(zero);
}

int stcount = 0;

char *make_name(c)
TOK c;
{
	char *s;
	int count, i;

	/* as it happens: fits in two words */
	s = (char *) new(sizeof(char) * 8);
	if (3100 <= ++stcount)
		errorFIPC('i', "too many generatedNs", ea0, ea0, ea0, ea0);

	s[0] = '_';
	s[1] = c;
	count = stcount;
	i = 2;
	if (10000 <= count){
		s[i++] = '0' + count/10000;
		count %= 10000;
	}
	if (1000 <= count){
		s[i++] = '0' + count/1000;
		count %= 1000;
	}
	else if (2<i) s[i++] = '0';

	if (100 <= count){
		s[i++] = '0' + count/100;
		count %= 100;
	}
	else if (2<i) s[i++] = '0';

	if (10 <= count){
		s[i++] = '0' + count/10;
		count %= 10;
	}
	else if (2<i) s[i++] = '0';

	s[i++] = '0' + count;
	s[i] = 0;

	return s;
}

Pbase type_adj(this, t)
Pbase this;
TOK t;
{
	Pbase bt;

	switch (this->base){
	case COBJ:
	case EOBJ:
		bt = new_basetype(0, 0);
		*bt = *this;
		if ( TO_DEL(this) ) type_del((Ptype)this);
		this = bt;
	}

	if (this->b_xname){
		if (this->base) {
			V1.u1.p = (char *)this->b_xname;
			V2.u1.i = (int)t;
			error("badBT:%n%k", &V1, &V2, ea0, ea0);
		} else {
			this->base = TYPE;
			this->b_name = this->b_xname;
		}
		this->b_xname = 0;
	}

	switch (t){
	case TYPEDEF:
		this->b_typedef = 1;
		break;
	case INLINE:
		this->b_inline = 1;
		break;
	case VIRTUAL:
		this->b_virtual = 1;
		break;
	case CONST:
		this->b_const = 1;
		break;
	case UNSIGNED:
		this->b_unsigned = 1;
		break;
	case SHORT:
		this->b_short = 1;
		break;
	case LONG:
		this->b_long = 1;
		break;
	case FRIEND:
	case OVERLOAD:
	case EXTERN:
	case STATIC:
	case AUTO:
	case REGISTER:
		if (this->b_sto) {
			V1.u1.i = (int)this->b_sto;
			V2.u1.i = (int)t;
			error("badBT:%k%k", &V1, &V2, ea0, ea0);
		} else 
			this->b_sto = t;
		break;
	case VOID:
	case CHAR:
	case INT:
	case FLOAT:
	case DOUBLE:
		if (this->base) {
			V1.u1.i = (int)this->base;
			V2.u1.i = (int)t;
			error("badBT:%k%k", &V1, &V2, ea0, ea0);
		} else 
			this->base = t;
		break;
	case SIGNED:
	case VOLATILE:
		V1.u1.i = (int)t;
		errorFIPC('w', "\"%k\" not implemented(ignored)",
			&V1, ea0, ea0, ea0);
		break;
	default:
		V1.u1.i = (int)t;
		errorFIPC('i', "BT::type_adj(%k)", &V1, ea0, ea0, ea0);
	}
	return this;
}

Pbase name_adj(this, n)
Pbase this;
Pname n;
{
	if (this->b_xname){
		if (this->base) {
			V1.u1.p = (char *)this->b_xname;
			V2.u1.p = (char *)n;
			error("badBT:%n%n", &V1, &V2, ea0, ea0);
		} else {
			this->base = TYPE;
			this->b_name = this->b_xname;
		}
		this->b_xname = 0;
	}

	if (!this->base && n->u1.tp->base != COBJ) {
		this->base = TYPE;
		this->b_name = n;
	}
	else this->b_xname = n;

	return this;
}

Pbase base_adj(this, b)
Pbase this;
Pbase b;
{
	Pname bn;

	bn = b->b_name;
	switch (this->base){
	case COBJ:
	case EOBJ:
		V1.u1.i = (int)this->base;
		V2.u1.p = (char *)this->b_name;
		error("NX after%k%n", &V1, &V2, ea0, ea0);
		return this;
	}

	if (this->base){
		if (this->b_name) {
			V1.u1.i = (int)this->base;
			V2.u1.p = (char *)this->b_name;
			V3.u1.i = (int)b->base;
			V4.u1.p = (char *)bn;
			error("badBT:%k%n%k%n", &V1, &V2, &V3, &V4);
		} else {
			V1.u1.i = (int)this->base;
			V2.u1.i = (int)b->base;
			V3.u1.p = (char *)bn;
			error("badBT:%k%k%n", &V1, &V2, &V3, ea0);
		}
	}
	else {
		this->base = b->base;
		this->b_name = bn;
		this->b_table = b->b_table;
	}
	return this;
}

/*
 *	"n" is the first name to be declared using "this"
 *	check the consistency of "this"
 *	and use "b_xname" for "n->string" if possible and needed
 */
Pbase basetype_check(this, n)
Pbase this;
Pname n;
{
	this->b_inline = 0;
	this->b_virtual = 0;
	if (this->b_xname &&(n->u1.tp || n->u2.string)){
		if (this->base) {
			V1.u1.i = (int)this->base;
			V2.u1.p = (char *)this->b_xname;
			error("badBT:%k%n", &V1, &V2, ea0, ea0);
		} else {
			this->base = TYPE;
			this->b_name = this->b_xname;
		}
		this->b_xname = 0;
	}

	if (this->b_xname){
		if (n->u2.string) {
			V1.u1.p = (char *)this->b_xname;
			V2.u1.p = (char *)n;
			error("twoNs inD:%n%n", &V1, &V2, ea0, ea0);
		} else {
			n->u2.string = this->b_xname->u2.string;
			hide(this->b_xname);
		}
		this->b_xname = 0;
	}

	if (ccl == 0 && n && n->n_oper == TNAME
	&& n->u5.n_qualifier == 0 && n->u2.string) { /* hide type name */
		Pname nx;

		nx = look(ktbl, n->u2.string, 0);
		if (nx) hide(nx);
	}

	switch (this->base){
	case 0:
		this->base = INT;
		break;
	case EOBJ:
	case COBJ:
		if (this->b_name->base == TNAME) {
			V1.u1.p = (char *)this->b_name;
			V2.u1.p = (char *)this;
			errorFIPC('i', "TN%n inCO %d", &V1, &V2, ea0, ea0);
		}
	}

	if (this->b_long || this->b_short){
		TOK sl;

		sl = (this->b_short ? SHORT: LONG);
		if (this->b_long && this->b_short){
			V1.u1.i = (int)this->base;
			V2.u1.p = (char *)n;
			error("badBT:long short%k%n", &V1, &V2, ea0, ea0);
		}
		if (this->base != INT) {
			V1.u1.i = (int)sl;
			V2.u1.i = (int)this->base;
			V3.u1.p = (char *)n;
			error("badBT:%k%k%n", &V1, &V2, &V3, ea0);
		} else 
			this->base = sl;
		this->b_short = this->b_long = 0;
	}

	if (this->b_typedef && this->b_sto){
		V1.u1.i = (int)this->b_sto;
		V2.u1.p = (char *)n;
		error("badBT:Tdef%k%n", &V1, &V2, ea0, ea0);
	}
	this->b_typedef = this->b_sto = 0;

	if (Pfctvec_type == 0) return this;

	if (this->b_const){
		if (this->b_unsigned){
			switch (this->base){
			default:
				V1.u1.i = (int)this->base;
				V2.u1.p = (char *)n;
				error("badBT: unsigned const %k%n",
					&V1, &V2, ea0, ea0);
				this->b_unsigned = 0;
			case LONG:
			case SHORT:
			case INT:
			case CHAR:
				return this;
			}
		}
		return this;
	}
	else if (this->b_unsigned){
		switch (this->base){
		case LONG:
			delete((char *)this);
			return ulong_type;
		case SHORT:
			delete((char *)this);
			return ushort_type;
		case INT:
			delete((char *)this);
			return uint_type;
		case CHAR:
			delete((char *)this);
			return uchar_type;
		default:
			V1.u1.i = (int)this->base;
			V2.u1.p = (char *)n;
			error("badBT: unsigned%k%n", &V1, &V2, ea0, ea0);
			this->b_unsigned = 0;
			return this;
		}
	}
	else {
		switch (this->base){
		case LONG:
			delete((char *)this);
			return long_type;
		case SHORT:
			delete((char *)this);
			return short_type;
		case INT:
			if (this != int_type) delete((char *)this);
			return int_type;
		case CHAR:
			delete((char *)this);
			return char_type;
		case VOID:
			delete((char *)this);
			return void_type;
		case TYPE:
			/* use a single base saved in the keyword */
			if (this->b_name->u5.n_qualifier){
				Pbase rv;

				rv = (Pbase)this->b_name->u5.n_qualifier;
				delete((char *)this);
				return rv;
			}
			else {
				PERM(this);
				this->b_name->u5.n_qualifier = (Pname)this;
				return this;
			}
		default:
			return this;
		}
	}
}

/*
 *	"type SM" seen e.g.	struct s {};
 *				class x;
 *				enum e;
 *				int tname;
 *				friend cname;
 *				friend class x;
 *				int;
 *
 *	convert
 *		union { ... };
 *	into
 *		union name { ... } name ;
 */
Pname aggr(this)
Pbase this;
{
	Pname n, fr;

	if (this->b_xname){
		if (this->base){
			n = new_name(this->b_xname->u2.string);
			hide(this->b_xname);
			this->b_xname = 0;
			return name_normalize(n, this, 0, 0);
		}
		else {
			this->base = TYPE;
			this->b_name = this->b_xname;
			this->b_xname = 0;
		}
	}

	switch (this->base){
	case COBJ:
	{
		Pclass cl;
		char *s;

		cl = (Pclass)this->b_name->u1.tp;
		s = cl->string;

		if (this->b_name->base == TNAME){
			V1.u1.p = (char *)this->b_name;
			errorFIPC('i', "TN%n inCO", &V1, ea0, ea0, ea0);
		}
		if (this->b_const){
			V1.u1.i = (int)cl->csu;
			V2.u1.p = (char *)this->b_name;
			error("const%k%n", &V1, &V2, ea0, ea0);
		}
		if (cl->c_body == 2){	/* body seen */
			if (s[0] == '_' && s[1] == 'C') {
				Pname obj;
				char *ss; 	/* max size of generated name
						 * is 7 chars, see make_name()
						 */

				ss = (char *) new(sizeof(char) * 8);
				obj = new_name(ss);
				strcpy(ss, s);
				if (cl->csu == UNION){
					ss[1] = 'O';
					cl->csu = ANON;
					return name_normalize(obj, this, 0, 0);
				}
				V1.u1.i = (int)cl->csu;
				errorFIPC('w', "un-usable%k ignored",
					&V1, ea0, ea0, ea0);
			}
			cl->c_body = 1;
			return this->b_name;
		}
		else {	/* really a typedef for cfront only: class x; */
			if (this->b_sto == FRIEND) goto frr;
			return 0;
		}
	}

	case EOBJ:
	{
		Penum en;

		en = (Penum)this->b_name->u1.tp;
		if (this->b_name->base == TNAME){
			V1.u1.p = (char *)this->b_name;
			errorFIPC('i', "TN%n in enumO", &V1, ea0, ea0, ea0);
		}
		if (this->b_const){
			V1.u1.p = (char *)this->b_name;
			error("const enum%n", &V1, ea0, ea0, ea0);
		}
		if (en->e_body == 2){
			en->e_body = 1;
			return this->b_name;
		}
		else {
			if (this->b_sto == FRIEND) goto frr;
			return 0;
		}
	}

	default:
		if (this->b_typedef)
			errorFIPC('w', "illegalTdef ignored",
				ea0, ea0, ea0, ea0);

		if (this->b_sto == FRIEND){
		frr:
			fr = look(ktbl, this->b_name->u2.string, 0);
			if (fr == 0){
				V1.u1.p = (char *)this->b_name;
				errorFIPC('i', "cannot find friend%n",
					&V1, ea0, ea0, ea0);
			}
			n = new_name(this->b_name->u2.string);
			n->n_sto = FRIEND;
			n->u1.tp = fr->u1.tp;
			return n;
		}
		else {
			n = new_name(make_name('D'));
			n->u1.tp = (Ptype)this;
			errorFIPC('w', "NX inDL", ea0, ea0, ea0, ea0);
			return n;
		}
	}
}

/*
 *	hide "this": that is, "this" should not be a keyword in this scope
 */
void hide(this)
Pname this;
{
	if (this->base != TNAME) return;
	if (this->n_key == 0){
		if (this->lex_level == bl_level){ /* FUDGE to allow struct stat */
			if (this->u1.tp->base != COBJ){
				V1.u1.p = (char *)this;
				errorFIPC('w', "%n redefined",
					&V1, ea0, ea0, ea0);
			}
		}
		modified_tn = new_nalist(this, modified_tn);
		this->n_key = HIDDEN;
	}
}

Pname Ntncheck;

/*
 *	enter the scope of class tn after seeing "tn::f"
 */
void set_scope(tn)
Pname tn;
{
	Plist l;
	Pname n;
	int i;
	Pbase b;
	Pclass cl;

	l = 0;
	n = 0;
	i = 1;

	if (tn->base != TNAME)
	{
		V1.u1.p = (char *)tn;
		V2.u1.i = (int)tn->base;
		errorFIPC('i', "set_scope: not aTN %d %d", &V1, &V2, ea0, ea0);
	}
	b = (Pbase)tn->u1.tp;
	if (b->b_name == 0 || b->b_name->u1.tp->base != CLASS)
	{
		V1.u1.p = (char *)tn;
		errorFIPC('i', "T of%n not aC", &V1, ea0, ea0, ea0);
	}
	cl = (Pclass)b->b_name->u1.tp;
	if ( !Ntncheck || strcmp(tn->u2.string, Ntncheck->u2.string) ){
		Pname nn;

		nn = get_mem(cl->memtbl, i);
		for(;nn ;nn = get_mem(cl->memtbl,++i))
			if (n = look(ktbl, nn->u2.string, 0))
				l = new_nalist(n, l);
		cl->tn_list = l;
		Ntncheck = tn;
	}
	for(l = cl->tn_list ;l ;l = l->l) {
		n = l->f;
		n->n_key = (n->lex_level) ? 0: HIDDEN;
		modified_tn = new_nalist(n, modified_tn);
	}
}

void restore()
{
	Plist l;
	Pname n;

	for(l = modified_tn ;l ;l = l->l) {
		n = l->f;
		n->n_key = ((int)n->lex_level <= bl_level)? 0: HIDDEN;
	}
}

Pbase start_cl(t, c, b)
TOK t;
Pname c;
Pname b;
{
	Pname n;
	Pbase bt;
	Pclass occl;

	if (c == 0) c = new_name(make_name('C'));

	n = tname(c, t);		/* t ignored */
	n->where = curloc;
	bt = (Pbase)n->u1.tp;		/* COBJ */
	if (bt->base != COBJ){
		V1.u1.p = (char *)n;
		V2.u1.p = (char *)bt;
		error("twoDs of%n:%t andC", &V1, &V2, ea0, ea0);
		errorFIPC('i', "can't recover from previous errors",
			ea0, ea0, ea0, ea0);
	}
	occl = ccl;
	ccl = (Pclass)bt->b_name->u1.tp;	/* CLASS */
	if (ccl->defined){
		ccl->defined |= IN_ERROR;
	}
	ccl->defined |= DEF_SEEN;
	if (ccl->in_class = occl) occl->tn_list = modified_tn; /* save mod-list */
	modified_tn = 0;
	Ntncheck = 0;
	ccl->string = n->u2.string;
	ccl->csu = t;
	if (b)ccl->clbase = tname(b, t);
	return bt;
}

void end_cl()
{
	Pclass occl;
	Plist ol;

	occl = ccl->in_class;
	ol = occl ? occl->tn_list: 0;	/* saved modified name list */
	ccl->c_body = 2;

	if (modified_tn){	/* export nested class names to outer scope: */
		Plist local, l, nl;
		Pname n;

		local = 0;
		l = modified_tn;
		nl = 0;
		for(;l ;l = nl) {
			nl = l->l;
			n = l->f;
			if (look(ktbl, n->u2.string, 0)){
				/* add it to enclosing class's modified name list */
				l->l = ol;
				ol = l;
			}
			else {	/* retain it in this class's modified name list */
				l->l = local;
				local = l;
			}
		}
		if (ccl->tn_list = modified_tn = local) restore();
	}
	modified_tn = ol;	/* restore mod-list(possibly modified) */
	ccl = occl;
}

Pbase end_enum(n, b)
Pname n, b;
{
	Pbase bt;
	Penum en;

	if (n == 0) n = new_name(make_name('E'));
	n = tname(n, ENUM);
	bt = (Pbase)n->u1.tp;
	if (bt->base != EOBJ){
		V1.u1.p = (char *)n;
		V2.u1.p = (char *)bt;
		error("twoDs of%n:%t and enum", &V1, &V2, ea0, ea0);
		errorFIPC('i', "can't recover from previous errors",
			ea0, ea0, ea0, ea0);
	}
	en = (Penum)bt->b_name->u1.tp;
	en->e_body = 2;
	en->mem = name_unlist((Pnlist)b);
	if (en->defined){
		V1.u1.p = (char *)n;
		error("enum%n defined twice", &V1, ea0, ea0, ea0);
		en->defined |= IN_ERROR;
	}
	en->defined |= DEF_SEEN;
	return bt;
}

/*
 *	typedef "this"
 */
static Pname name_tdef(this)
Pname this;
{
	Pname n;

	n = insert(ktbl, this, 0);
	if (this->u1.tp == 0){
		V1.u1.p = (char *)this;
		errorFIPC('i', "Tdef%n tp==0", &V1, ea0, ea0, ea0);
	}
	n->base = this->base = TNAME;
	PERM(n);
	PERM(this->u1.tp);
	modified_tn = new_nalist(n, modified_tn);
	return n;
}

/*
 *	"csu" "this" seen, return typedef'd name for "this"
 *	return	(TNAME,x)
 *	x:	(COBJ,y)
 *	y:	(NAME,z)
 *	z:	(CLASS,ae);
 */
Pname tname(this, csu)
Pname this;
TOK csu;
{
	switch (this->base){
	case TNAME:
		return this;
	case NAME:
	{
		Pname tn, on;

		tn = insert(ktbl, this, 0);
		on = new_name(0);
		tn->base = TNAME;
		tn->lex_level = this->lex_level;
		modified_tn = new_nalist(tn, modified_tn);
		tn->n_list = this->n_list = 0;
		this->u2.string = tn->u2.string;
		*on = *this;
		switch (csu){
		case ENUM:
			tn->u1.tp = (Ptype) new_basetype(EOBJ, on);
			on->u1.tp = (Ptype)new_enum(0);
			break;
		default:
			on->u1.tp = (Ptype) new_classdef(csu);
			((Pclass)on->u1.tp)->string = tn->u2.string;
			tn->u1.tp = (Ptype) new_basetype(COBJ, on);
			((Pbase)tn->u1.tp)->b_table = ((Pclass)on->u1.tp)->memtbl;
		}
		PERM(tn);
		PERM(tn->u1.tp);
		PERM(on);
		PERM(on->u1.tp);
		return tn;
	}
	default:
		V1.u1.p = (char *)this->u2.string;
		V2.u1.p = (char *)this;
		V3.u1.i = (int)this->base;
		errorFIPC('i', "tname(%s %d %k)", &V1, &V2, &V3, ea0);
	}
}


/*
 *	if (bl) : a function definition(check that it really is a type
 *
 *	if (cast) : no name string
 *
 *	for each name on the name list
 *	invert the declarator list(s) and attatch basetype
 *	watch out for class object initializers
 *
 *	convert
 *		struct s { int a; } a;
 *	into
 *		struct s { int a; }; struct s a;
 */
Pname name_normalize(this, b, bl, cast)
Pname this;
Pbase b;
Pstmt bl;
bit cast;
{
	Pname n;
	Pname nn;
	TOK stc;
	bit tpdf;
	bit inli;
	bit virt;
	Pfct f;
	Pname nx;

	if (b){
		stc = b->b_sto;
		tpdf = b->b_typedef;
		inli = b->b_inline;
		virt = b->b_virtual;
	}
	else {
		stc = 0;
		tpdf = 0;
		inli = 0;
		virt = 0;
	}

	if (inli && stc == EXTERN) {
		error("both extern and inline", ea0, ea0, ea0, ea0);
		inli = 0;
	}

	if (stc == FRIEND && this->u1.tp == 0) {
		/*	friend x;
		 *	must be handled during syntax analysis to cope with
		 *		class x { friend y; y* p; };
		 *	"y" is not local to "x":
		 *		class x { friend y; ... }; y* p;
		 *	is legal
		 */
		if (b->base)
			errorFIPC((int)0, "T specified for friend",
				ea0, ea0, ea0, ea0);
		if (this->n_list){
			error("L of friends", ea0, ea0, ea0, ea0);
			this->n_list = 0;
		}
		nx = tname(this, CLASS);
		modified_tn = modified_tn->l;	/* global */
		this->n_sto = FRIEND;
		this->u1.tp = nx->u1.tp;
		return this;
	
	}

	if (this->u1.tp		/* HORRIBLE FUDGE: fix the bad grammar */
	&& this->u1.tp->base == FCT
	&&(this->n_oper == TNAME ||((Pfct)this->u1.tp)->returns) ) {
		Pfct f, f2;

		f = (Pfct)this->u1.tp;
		f2 = (Pfct)f->returns;

		if (f2){
			Ptype pt, t;

			t = (Ptype)f2;
		lxlx:
			switch (t->base){
			case PTR:	/* x(* p)(args) ? */
			case VEC:	/* x(* p[10])(args) ? */
				if (pt = ((Pptr)t)->typ){
					if (pt->base == TYPE){
						((Pptr)t)->typ = 0;
						b = (Pbase)pt;
						stc = b->b_sto;
						tpdf = b->b_typedef;
						inli = b->b_inline;
						virt = b->b_virtual;
					}
					else {
						t = pt;
						goto lxlx;
					}
				}
				goto zse1;
			case FCT:
			{
				Pexpr e, ee;
				Ptype tx, t;

				e = (Pexpr)f2->argtype;
				if (e && e->base == ELIST) {
					/* get the real name; fix its type */
					if (e->u3.e2)goto zse1;
					if (e->u2.e1->base != DEREF)goto zse1;
					ee = e->u2.e1;
					t = 0;
				ldld:
					switch (ee->base){
					case DEREF:
					{
						Ptype tt;

						if (ee->u3.e2)
							tt=(Ptype)new_vec(0,ee->u3.e2);
						else
							tt=(Ptype)new_ptr(PTR,0,0);
						if (t)
							((Pptr)t)->typ = tt;
						else 
							tx = tt;
						t = tt;
						ee = ee->u2.e1;
						goto ldld;
					}
					case NAME:
					{
						Pname rn;

						rn = (Pname)ee;
						b=new_basetype(TYPE, look(ktbl,this->u2.string,0));
						f->returns = tx;
						this->n_oper = 0;
						this->u2.string = rn->u2.string;
						this->base = NAME;
					}
					}
				}
			}
			}
		}
	}
zse1:
	if (b == 0){
		V1.u1.p = (char *)this->u2.string;
		error("BTX for %s", &V1, ea0, ea0, ea0);
		b = (Pbase)defa_type;
	}
	if (cast)this->u2.string = "";
	b = basetype_check(b, this);

	switch (b->base){	/* separate class definitions */
				/* from object and function type declarations */
	case COBJ:
		nn = b->b_name;
		if (((Pclass)nn->u1.tp)->c_body == 2){ /* first occurrence */
			if (this->u1.tp && this->u1.tp->base == FCT) {
				V1.u1.i = (int)((Pclass)nn->u1.tp)->csu;
				V2.u1.p = (char *)nn;
				V3.u1.p = (char *)this;
				errorIloc('s', &this->where,
				"%k%n defined as returnT for%n(did you forget a ';' after '}' ?)",
					&V1, &V2, &V3, ea0);
				nn = this;
				break;
			}
			nn->n_list = this;
			((Pclass)nn->u1.tp)->c_body = 1; /* other occurences */
		}
		else 
			nn = this;
		break;
	case EOBJ:
		nn = b->b_name;
		if (((Penum)nn->u1.tp)->e_body == 2){
			if (this->u1.tp && this->u1.tp->base == FCT) {
				V1.u1.p = (char *)nn;
				V2.u1.p = (char *)this;
				errorFIPC('s',
				"enum%n defined as returnT for%n(did you forget a ';'?)",
					&V1, &V2, ea0, ea0);
				nn = this;
				break;
			}
			nn->n_list = this;
			((Penum)nn->u1.tp)->e_body = 1;
		}
		else 
			nn = this;
		break;
	default:
		nn = this;
	}

	for(n = this ;n ;n = nx) {
		Ptype t;
		Pbase tb;

		t = n->u1.tp;
		nx = n->n_list;
		n->n_sto = stc;

		if (n->base == TNAME){
			V1.u1.p = (char *)n;
			errorFIPC('i', "redefinition ofTN%n",
				&V1, ea0, ea0, ea0);
		}
		if (t == 0){
			if (bl == 0)
				n->u1.tp = t = (Ptype)b;
			else {
				V1.u1.p = (char *)n;
				error("body of nonF%n", &V1, ea0, ea0, ea0);
				t = (Ptype) fct_ctor((Ptype)defa_type, 0, 0);
			}
		}
		switch (t->base){
		case PTR:
		case RPTR:
			n->u1.tp = ptr_normalize((Pptr)t,(Ptype)b);
			break;
		case VEC:
			n->u1.tp = vec_normalize((Pvec)t,(Ptype)b);
			break;
		case FCT:
			n->u1.tp = fct_normalize((Pfct)t,(Ptype)b);
			break;
		case FIELD:
			if (n->u2.string == 0)
				n->u2.string = make_name('F');
			n->u1.tp = t;
			tb = b;
		flatten:
			switch (tb->base){
			case TYPE:	/* chase typedefs */
				tb = (Pbase)tb->b_name->u1.tp;
				goto flatten;
			case INT:
				if (b->b_unsigned)
					((Pbase)t)->b_fieldtype = (Ptype)uint_type;
				else
					((Pbase)t)->b_fieldtype = (Ptype)int_type;
				goto iii;
			case CHAR:
				if (b->b_unsigned)
					((Pbase)t)->b_fieldtype = (Ptype)uchar_type;
				else
					((Pbase)t)->b_fieldtype = (Ptype)char_type;
				goto iii;
			case SHORT:
				if (b->b_unsigned)
					((Pbase)t)->b_fieldtype = (Ptype)ushort_type;
				else
					((Pbase)t)->b_fieldtype = (Ptype)short_type;
				goto iii;
			iii:
				((Pbase)t)->b_unsigned = b->b_unsigned;
				((Pbase)t)->b_const = b->b_const;
				break;
			default:
				error("non-int field", ea0, ea0, ea0, ea0);
				n->u1.tp = (Ptype)defa_type;
			}
			break;
		}
		f = (Pfct)n->u1.tp;

		if (f->base != FCT){
			if (bl){
				V1.u1.p = (char *)n;
				error("body for nonF%n", &V1, ea0, ea0, ea0);
				f = fct_ctor((Ptype)defa_type, 0, 0);
				n->u1.tp = (Ptype)f;
				continue;
			}
			if (inli){
				V1.u1.p = (char *)n;
				error("inline nonF%n", &V1, ea0, ea0, ea0);
			}
			if (virt){
				V1.u1.p = (char *)n;
				error("virtual nonF%n", &V1, ea0, ea0, ea0);
			}

			if (tpdf){
				if (n->u3.n_initializer){
					V1.u1.p = (char *)n;
					error("Ir forTdefN%n", &V1, ea0, ea0, ea0);
					n->u3.n_initializer = 0;
				}
				name_tdef(n);
			}
			continue;
		}

		f->f_inline = inli;
		f->f_virtual = virt;

		if (tpdf){
			if (f->body = bl){
				V1.u1.p = (char *)n;
				error("Tdef%n { ... }", &V1, ea0, ea0, ea0);
			}
			if (n->u5.n_qualifier){
				/* typedef T x::f(args);
				 * a pointer to member fucntion:
				 * equivalent to typedef T x::(f)(args);
				 */
				f->memof = (Pclass)((Pbase)n->u5.n_qualifier->u1.tp)->b_name->u1.tp;
				n->u5.n_qualifier = 0;
			}
			name_tdef(n);
			continue;
		}

		if (f->body = bl)continue;

		/*
		 *	Check function declarations.
		 *	Look for class object instansiations
		 *	The real ambiguity:		; class x fo();
		 *		is interpreted as an extern function
		 *		declaration NOT a class object with an
		 *		empty initializer
		 */
		{	Pname cn;
			bit clob;

			cn = is_cl_obj(f->returns);
			clob = (cn || cl_obj_vec);
			if (f->argtype){ /* check argument/initializer list */
				Pname nn;

				for(nn = f->argtype ;nn ;nn = nn->n_list) {
					if (nn->base != NAME){
						if (!clob){
							V1.u1.p = (char *)n;
							error("ATX for%n",
								&V1, ea0, ea0, ea0);
							goto zzz;
						}
						goto is_obj;
					}

					if (nn->u1.tp)goto ok;
				}
				if (!clob){
					error("FALX", ea0, ea0, ea0, ea0);
					goto zzz;
				}
		is_obj:
				/* it was an initializer: expand to constructor */
				n->u1.tp = f->returns;
				if (f->argtype->base != ELIST)
					f->argtype = (Pname)new_expr(0,ELIST,(Pexpr)f->argtype,0);
				n->u3.n_initializer=
					new_texpr(VALUE,cn->u1.tp,(Pexpr)f->argtype);
				goto ok;
			zzz:
				if (f->argtype){
					if (TO_DEL(f->argtype))
						name_del(f->argtype);
					f->argtype = 0;
					f->nargs = 0;
					f->nargs_known = !fct_void;
				}
			}

		ok:
			;
		}
	}
	return nn;
}

Ptype vec_normalize(this, vecof)
Pvec this;
Ptype vecof;
{
	Ptype t;

	t = this->typ;
	this->typ = vecof;
	if (t == 0) return (Ptype)this;

xx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case PTR:
	case RPTR:
		return ptr_normalize((Pptr)t,(Ptype)this);
	case VEC:
		return vec_normalize((Pvec)t,(Ptype)this);
	case FCT:
		return fct_normalize((Pfct)t,(Ptype)this);
	default:
		V1.u1.i = (int)t->base;
		errorFIPC('i', "bad vectorT(%d)", &V1, ea0, ea0, ea0);
	}
}

Ptype ptr_normalize(this, ptrto)
Pptr this;
Ptype ptrto;
{
	int ttconst;
	Ptype t;

	ttconst = 0;
	t = this->typ;
	this->typ = ptrto;

	while (ptrto->base == TYPE){
		if (!ttconst) ttconst = ((Pbase)ptrto)->b_const;
		ptrto = ((Pbase)ptrto)->b_name->u1.tp;
	}

	if (ptrto->base == FCT)
		if (this->memof)
			if (((Pfct)ptrto)->memof){
				if (this->memof != ((Pfct)ptrto)->memof){
					V1.u1.p = (char *)this->memof->string;
					V2.u1.p=(char*)((Pfct)ptrto)->memof->string;
					error("P toMF mismatch: %s and %s",
						&V1, &V2, ea0, ea0);
				}
			}
			else 
				((Pfct)ptrto)->memof = this->memof;
		else 
			this->memof = ((Pfct)ptrto)->memof;

	if (t == 0){
		Pbase b;

		b = (Pbase)ptrto;
		if (Pfctvec_type && this->rdo == 0
		&& b->b_unsigned == 0 && b->b_const == 0
		&& ttconst == 0 && this->memof == 0
		&& this->base == PTR) {
			switch (b->base){
			case INT:
				delete((char *)this);
				return Pint_type;
			case CHAR:
				delete((char *)this);
				return Pchar_type;
			case VOID:
				delete((char *)this);
				return Pvoid_type;
			}
		}
		if (this->base == RPTR && b->base == VOID)
			error("void& is not a validT", ea0, ea0, ea0, ea0);
		return (Ptype)this;
	}

xx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case PTR:
	case RPTR:
		return ptr_normalize((Pptr)t,(Ptype)this);
	case VEC:
		return vec_normalize((Pvec)t,(Ptype)this);
	case FCT:
		return fct_normalize((Pfct)t,(Ptype)this);
	default:
		V1.u1.i = (int)t->base;
		errorFIPC('i', "badPT(%k)", &V1, ea0, ea0, ea0);
	}
}

/*
 *	normalize return type
 */
Ptype fct_normalize(this, ret)
Pfct this;
Ptype ret;
{
	register Ptype t;

	t = this->returns;
	this->returns = ret;
	if (t == 0) return (Ptype)this;

	if (this->argtype && this->argtype->base != NAME){
		errorFIPC('i', "syntax: ANX", ea0, ea0, ea0, ea0);
		this->argtype = 0;
		this->nargs = 0;
		this->nargs_known = 0;
	}

xx:
	switch (t->base){
	case PTR:
	case RPTR:
		return ptr_normalize((Pptr)t,(Ptype)this);
	case VEC:
		return vec_normalize((Pvec)t,(Ptype)this);
	case FCT:
		return fct_normalize((Pfct)t,(Ptype)this);
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	default:
		V1.u1.i = (int)t->base;
		errorFIPC('i', "badFT:%k", &V1, ea0, ea0, ea0);
	}
}


/*
 *	sort out the argument types for old syntax:
 *			f(a,b) int a; char b; { ... }
 *	beware of
 *			f(a) struct s { int a; }; struct s a;
 */
void argdcl(this, dcl, fn)
Pfct this;
Pname dcl;
Pname fn;
{
	Pname n;

	switch (this->base){
	case FCT:
		break;
	case ANY:
		return;
	default:
		V1.u1.i = (int)this->base;
		errorFIPC('i', "fct::argdcl(%d)", &V1, ea0, ea0, ea0);
	}

	if (this->argtype){
		switch (this->argtype->base){
		case NAME:
			if (dcl) error("badF definition syntax",ea0,ea0,ea0,ea0);
			for(n = this->argtype ;n ;n = n->n_list) {
				if (n->u2.string == 0)
					n->u2.string = make_name('A');
			}
			return;
		case ELIST:	/* expression list:     f(a,b,c) int a; ... { ... }
				 * scan the elist and build a NAME list
				 */
		{	Pexpr e;
			Pname nn;
			Pname tail;

			tail = 0;
			n = 0;
/*
			if (old_fct_accepted == 0) {
				V1.u1.p = (char *)fn;
				errorIloc('w', &fn->where,
					"old style definition of%n",
					&V1, ea0, ea0, ea0);
			}
*/
			for(e = ((Pexpr)this->argtype);e ;e = e->u3.e2) {
				Pexpr id;

				id = e->u2.e1;
				if (id->base != NAME){
					error("NX inAL", ea0, ea0, ea0, ea0);
					this->argtype = 0;
					dcl = 0;
					break;
				}
				nn = new_name(id->u2.string);
				if (n)
					tail = tail->n_list = nn;
				else 
					tail = n = nn;
			}
			this->argtype = n;
			break;
		}
		default:
			V1.u1.i = (int)this->argtype->base;
			error("ALX(%d)", &V1, ea0, ea0, ea0);
			this->argtype = 0;
			dcl = 0;
		}
	}
	else {
		this->nargs_known = !fct_void;
		this->nargs = 0;
		if (dcl) error("ADL forFWoutAs", ea0, ea0, ea0, ea0);
		return;
	}

	this->nargs_known = 0;

	if (dcl){
		Pname d;
		Pname dx;
		/*	for each  argument name see if its type is specified
		 *	in the declaration list otherwise give it the default type
		 */

		for(n = this->argtype ;n ;n = n->n_list) {
			char *s;

			s = n->u2.string;
			if (s == 0){
				error("AN missing inF definition", ea0,
					ea0, ea0, ea0);
				n->u2.string = s = make_name('A');
			}
			else if (n->u1.tp){
				V1.u1.p = (char *)n->u2.string;
				error("twoTs forA %s", &V1, ea0, ea0, ea0);
			}
			for(d = dcl ;d ;d = d->n_list) {
				if (strcmp(s, d->u2.string) == 0){
					if (d->u1.tp->base == VOID){
						V1.u1.p = (char *)d;
						error("voidA%n",&V1,ea0,ea0,ea0);
						d->u1.tp = (Ptype)any_type;
					}
					n->u1.tp = d->u1.tp;
					n->n_sto = d->n_sto;
					d->u1.tp = 0; /* now merged into argtype */
					goto xx;
				}
			}
			n->u1.tp = (Ptype)defa_type;
		xx: ;
			if (n->u1.tp == 0){
				V1.u1.p = (char *)n->u2.string;
				errorFIPC('i', "noT for %s",
					&V1, ea0, ea0, ea0);
			}
		}

		/*	now scan the declaration list for "unused declarations"
		 *	and delete it
		 */
		for(d = dcl ;d ;d = dx) {
			dx = d->n_list;
			if (d->u1.tp){	/* not merged with argtype list */
				switch (d->u1.tp->base){
				case CLASS:
				case ENUM:
					/* WARNING: this will reverse the order of
					 *   class and enum declarations
					 */
					d->n_list = this->argtype;
					this->argtype = d;
					break;
				default:
					V1.u1.p = (char *)d;
					error("%n inADL not inAL",&V1,ea0,ea0,ea0);
				}
			}
		}
	}

	/* add default argument types if necessary */
	for(n = this->argtype ;n ;n = n->n_list) {
		if (n->u1.tp == 0) n->u1.tp = (Ptype)defa_type;
		this->nargs++;
	}
}

Pname cl_obj_vec;	/* set if is_cl_obj() found a vector of class objects */
Pname eobj;		/* set if is_cl_obj() found an enum */

/*
 *	returns this->b_name if this is a class object
 *	returns 0 and sets cl_obj_vec to this->b_name
 *		if this is a vector of class objects
 *	returns 0 and sets eobj to this->b_name
 *		if this is an enum object
 *	else returns 0
 */
Pname is_cl_obj(this)
Ptype this;
{
	bit v;
	register Ptype t;

	v = 0;
	t = this;
	eobj = 0;
	cl_obj_vec = 0;
xx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;

	case COBJ:
		if (v){
			cl_obj_vec = ((Pbase)t)->b_name;
			return 0;
		}
		else 
			return ((Pbase)t)->b_name;

	case VEC:
		t = ((Pvec)t)->typ;
		v = 1;
		goto xx;

	case EOBJ:
		eobj = ((Pbase)t)->b_name;
	default:
		return 0;
	}
}

static Penum new_enum(n)
Pname n;
{
	Penum Xthis_enum;

	Xthis_enum = (Penum) new(sizeof(struct enumdef));
	Xthis_enum->base = ENUM;
	Xthis_enum->mem = n;

	return Xthis_enum;
}

Plist new_nalist(ff, ll)
Pname ff;
Plist ll;
{
	Plist Xthis_nalist;

	Xthis_nalist = (Plist) new(sizeof(struct name_list));
	Xthis_nalist->f = ff;
	Xthis_nalist->l = ll;

	return Xthis_nalist;
}

Pvec new_vec(t, e)
Ptype t;
Pexpr e;
{
	Pvec Xthis_vec;

	Xthis_vec = (Pvec) new(sizeof(struct vec));
	Nt++;
	Xthis_vec->base = VEC;
	Xthis_vec->typ = t;
	Xthis_vec->dim = e;

	return Xthis_vec;
}

static Pclass new_classdef(b)
TOK b;
{
	Pclass this;

	this = (Pclass) new(sizeof(struct classdef));
	this->base = CLASS;
	this->csu = b;
	this->memtbl = new_table(CTBLSIZE, 0, 0);
	return this;
}


