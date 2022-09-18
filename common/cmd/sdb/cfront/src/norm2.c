/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/norm2.c	1.2"
/************************************************************************
norm2.c:

	"normalization" handles problems which could have been handled
	by the syntax analyser; but has not been done. The idea is
	to simplify the grammar and the actions associated with it,
	and to get a more robust error handling

****************************************************************************/

#include "cfront.h"
#include "size.h"

#define NBITE	(CHUNK-8)/sizeof(struct name)-1
#define EBITE	(CHUNK-8)/sizeof(struct expr)-1
#define SBITE	(CHUNK-8)/sizeof(struct stmt)-1

Pfct fct_ctor(t, arg, known)
Ptype t;
Pname arg;
TOK known;
{
	Pfct this;
	register Pname n;

	this = (Pfct) new(sizeof(struct fct));
	Nt++;
	this->base = FCT;
	this->nargs_known = known;
	this->returns = t;
	this->argtype = arg;

	if (arg == 0 || arg->base == ELIST) return this;

	for(n = arg ;n ;n = n->n_list) {
		switch (n->u1.tp->base){
		case VOID:
			this->argtype = 0;
			this->nargs = 0;
			this->nargs_known = 1;
			if (n->u2.string)
			{
				V1.u1.p = (char *)n;
				error("voidFA%n", &V1, ea0, ea0, ea0);
			} else if (this->nargs || n->n_list){
				error("voidFA", ea0, ea0, ea0, ea0);
				this->nargs_known = 0;
			}
			break;
		case CLASS:
		case ENUM:
			break;
		default:
			this->nargs++;
		}
	}
	return this;
}


Pexpr expr_free;

Pexpr new_expr(this, ba, a, b)
Pexpr this;
TOK ba;
Pexpr a;
Pexpr b;
{
	register Pexpr p;

	if (this)goto ret;

	if ((p = expr_free)== 0){
		register Pexpr q;

		q = (Pexpr) chunk(1);
		for(p = expr_free = &q[EBITE-1];q < p ;p--) p->u2.e1 = p - 1;
		(p+1)->u2.e1 = 0;
	}
	else 
		expr_free = p->u2.e1;

	this = p;

	this->permanent = 0;
	this->u1.tp = 0;
	this->u4.tp2 = 0;

	Ne++;	/* excluding names */
ret:
	this->base = ba;
	this->u2.e1 = a;
	this->u3.e2 = b;
	return this;
}


void expr__dtor(this)
Pexpr this;
{
	if (!this) return;

	NFe++;
	this->u2.e1 = expr_free;
	expr_free = this;
	this = 0;
}


Pstmt stmt_free;

Pstmt new_stmt(this, ba, ll, a)
Pstmt this;
TOK ba;
Loc ll;
Pstmt a;
{
	register Pstmt p;

	if ((p = stmt_free)== 0){
		register Pstmt q;

		q = (Pstmt) chunk(1);
		for(p = stmt_free = &q[SBITE-1];q < p ;p--) p->s_list = p-1;
		(p + 1)->s_list = 0;
	}
	else 
		stmt_free = p->s_list;

	this = p;

	this->permanent = 0;
	this->u2.e = this->u1.e2 = 0;
	this->memtbl = 0;
	this->u3.else_stmt = 0;
	this->s_list = 0;

	Ns++;
	this->base = ba;
	this->where = ll;
	this->s = a;
	return this;
}


void stmt__dtor(this)
Pstmt this;
{
	if (!this) return;

	NFs++;
	this->s_list = stmt_free;
	stmt_free = this;
	this = 0;
}


Pbase new_basetype(b, n)
TOK b;
Pname n;
{
	Pbase this;
	this = (Pbase) new(sizeof(struct basetype));
	Nbt++;
	switch (b){
	case 0:
		break;
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
	case FRIEND:
	case OVERLOAD:
	case EXTERN:
	case STATIC:
	case AUTO:
	case REGISTER:
		this->b_sto = b;
		break;
	case SHORT:
		this->b_short = 1;
		break;
	case LONG:
		this->b_long = 1;
		break;
	case ANY:
	case ZTYPE:
	case VOID:
	case CHAR:
	case INT:
	case FLOAT:
	case DOUBLE:
		this->base = b;
		break;
	case TYPE:
	case COBJ:
	case EOBJ:
	case FIELD:
	case ASM:
		this->base = b;
		this->b_name = n;
		break;
	case SIGNED:
	case VOLATILE:
		V1.u1.i = (int)b;
		errorFIPC('w', "\"%k\" not implemented(ignored)",
			&V1, ea0, ea0, ea0);
		break;
	default:
		V1.u1.i = (int)b;
		errorFIPC('i', "badBT:%k", &V1, ea0, ea0, ea0);
	}
	return this;
}


Pname name_free;

Pname new_name(s)
char *s;
{
	register Pname this, p;

	if ((p = name_free)== 0){
		register Pname q;

		q = (Pname) chunk(1);
		for(p = name_free = &q[NBITE-1];q < p ;p--) p->n_tbl_list = p-1;
		(p+1)->n_tbl_list = 0;
	}
	else 
		name_free = p->n_tbl_list;

	this = p;
	this = (Pname) new_expr((Pexpr)this, NAME, 0, 0);

	Nn++;
	this->u2.string = s;
	this->where = curloc;
	this->lex_level = bl_level;

	/* beware of alignment differences & pointer-zeros that are not int-zeros */
	this->u1.tp = 0;
	this->u3.n_initializer = 0;
	this->u4.n_table = 0;
	this->n_oper = 0;
	this->n_sto = 0;
	this->n_stclass = 0;
	this->n_scope = 0;
	this->n_union = 0;
	this->n_evaluated = 0;
	this->n_xref = 0;
	this->n_protect = 0;
	this->n_addr_taken = 0;
	this->n_used = 0;
	this->n_assigned_to = 0;
	this->n_val = 0;
	this->n_offset = 0;
	this->n_list = 0;
	this->n_tbl_list = 0;
	this->u5.n_qualifier = 0;
	return this;
}


void name__dtor(this)
Pname this;
{
	if (!this) return;

	NFn++;
	this->n_tbl_list = name_free;
	name_free = this;
	this = 0;
}

void add_list(this, n)
Pnlist this;
Pname n;
{
	Pname nn;

	if (n->u1.tp &&(n->u1.tp->defined & IN_ERROR)) return;
	this->tail->n_list = n;
	for(nn = n ;nn->n_list ;nn = nn->n_list);
	this->tail = nn;
}

int NFl = 0;

Pname name_unlist(l)
Pnlist l;
{
	Pname n;

	if (l == 0) return 0;
	n = l->head;
	NFl++;
	delete((char *)l);
	return n;
}

Pstmt stmt_unlist(l)
Pslist l;
{
	Pstmt s;

	if (l == 0) return 0;
	s = l->head;
	NFl++;
	delete((char *)l);
	return s;
}

Pexpr expr_unlist(l)
Pelist l;
{
	Pexpr e;

	if (l == 0) return 0;
	e = l->head;
	NFl++;
	delete((char *)l);
	return e;
}

void sig_name(n)
Pname n;
{
	static char buf [256];
	char *p;

	buf[0] = '_';
	buf[1] = 'O';
	p = signature(n->u1.tp, buf+2);
	if (255 < p-buf)
		errorFIPC('i', "sig_name():N buffer overflow",
			ea0, ea0, ea0, ea0);
	n->u2.string = buf;
	n->u1.tp = 0;
}

Ptype tok_to_type(b)
TOK b;
{
	Ptype t;
	switch (b){
	case CHAR:
		t = (Ptype)char_type;
		break;
	case SHORT:
		t = (Ptype)short_type;
		break;
	case LONG:
		t = (Ptype)long_type;
		break;
	case UNSIGNED:
		t = (Ptype)uint_type;
		break;
	case FLOAT:
		t = (Ptype)float_type;
		break;
	case DOUBLE:
		t = (Ptype)double_type;
		break;
	case VOID:
		t = (Ptype)void_type;
		break;
	default:
		V1.u1.i = (int)b;
		error("illegalK:%k", &V1, ea0, ea0, ea0);
	case INT:
		t = (Ptype)int_type;
	}
	return t;
}

Pbase defa_type;
Pbase moe_type;
Pexpr dummy;
Pexpr zero;

Pclass ccl;
Plist modified_tn = 0;

static struct name sta_name_dummy;
Pname sta_name = &sta_name_dummy;

TOK back;

void memptrdcl(bn, tn, ft, n)
Pname bn;
Pname tn;
Ptype ft;
Pname n;
{
	Pptr p;
	Pbase b;
	Pfct f;
	Ptype t;

	p = new_ptr(PTR, 0, 0);
	p->memof = (Pclass)((Pbase)bn->u1.tp)->b_name->u1.tp;
	b = new_basetype(TYPE, tn);
	PERM(p);
	f = (Pfct)ft;
	t = n->u1.tp;
	if (t){
		p->typ = t;
	ltlt:
		switch (t->base){
		case PTR:
		case RPTR:
		case VEC:
			if (((Pptr)t)->typ == 0){
				((Pptr)t)->typ = (Ptype)b;
				break;
			}
			t = ((Pptr)t)->typ;
			goto ltlt;
		default:
			errorFIPC('s', "P toMFT too complicated",ea0,ea0,ea0,ea0);
		}
	}
	else 
		p->typ = (Ptype)b;
	f->returns = (Ptype)p;
	n->u1.tp = (Ptype)f;
}

extern int _STI__src_norm2_c()
{
	new_name(&sta_name_dummy);
}

extern int _STD__src_norm2_c()
{
	name__dtor(&sta_name_dummy);
}
