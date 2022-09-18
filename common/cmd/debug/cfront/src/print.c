/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/print.c	1.1"
/**************************************************************************

print.c:

	print the output of simpl, typ, or syn in a form suitable for cc input

****************************************************************************/

#include "cfront.h"
#include "size.h"

extern FILE *out_file;
char emode = 0;
extern int ntok;
int ntok = 0;
bit Cast;
Pin curr_icall;
static int last_ll = 1;

int MAIN = 0;	/* fudge to get _main() called by main() */

#define MX	20
#define NTBUF	10
/*
 *	buffer for assembling declaration(or cast)
 *	left contains CONST_PTR	=> *CONST
 *		     CONST_RPTR => &CONST
 *			PTR	=> *
 *			RPTR	=> &
 *			LP	=>(
 *	right contains	RP	=> )
 *			VEC	=>[ rnode]
 *			FCT	=>( rnode )
 *			FIELD	=> : rnode
 */
struct dcl_buf {	/* sizeof dcl_buf == 216 */
	Pbase b;
	Pname n;
	TOK left[MX];
	TOK right[MX];
	Pnode rnode[MX];
	Pclass lnode[MX];
	int li, ri;
};

#define	INIT(p,nn)	{(p)->b=0;(p)->n=(nn);(p)->li=(p)->ri=0; }
#define	BASE(p,bb)	{(p)->b = (bb); }
#define	SFRONT(p,t)	{(p)->left[++((p)->li)] = (t); }
#define	DFRONT(p,c)	{(p)->left[++((p)->li)]=MEMPTR;(p)->lnode[(p)->li]=(c); }
#define	BACK(p,t,nod)	{(p)->right[++((p)->ri)]=(t);(p)->rnode[(p)->ri]=(nod); }
#define	PARAN(p) 	{ SFRONT((p),LP); BACK((p),RP,0); }
#define EPRINT(e)	if (e) Eprint(e)

typedef struct dcl_buf *Sdclp;

Sdclp tbufvec[NTBUF]= { 0 };
Sdclp tbuf = 0;
int freetbuf = 0;

static Pname Look_use;

static void dcl_put();
static void base_dcl_prt();
static void class_dcl_prt();
static void enum_dcl_prt();
static void fct_dcl_prt();
static void tbl_dcl_prt();

/*
 *	print the output representation of "t"
 */
void puttok(t)
TOK t;
{
	if (keys[t]) putstring(keys[t]);
	if (12 < ntok++) {
		ntok = 0;
		putline(&last_line);
	}
	else if (t == SM){
		ntok = 0;
		putch('\n');
		if (last_ll) last_line.line++;
	}
	else 
		putch(' ');
}


static void dcl_put(this)
Sdclp this;
{
	int i;

	if (MX <= this->li || MX <= this->ri)
		errorFIPC('i', "T buffer overflow", ea0, ea0, ea0, ea0);
	if (this->b == 0){
		V1.u1.p = (Cast) ? " in case" : "";
		errorFIPC('i', "noBT%s", &V1, ea0, ea0, ea0);
	}
	if (this->n && this->n->n_sto && this->n->n_sto != REGISTER)
		puttok(this->n->n_sto);

	base_dcl_prt(this->b);

	for(;this->li ;this->li--) {
		switch (this->left[this->li]){
		case LP:
			putch('(');
			break;
		case PTR:
			putch('*');
			break;
		case RPTR:
			if (emode)
				putch('&');
			else 
				putch('*');
			break;
		case CONST_PTR:
			if (emode)
				putstring("*const ");
			else 
				putch('*');
			break;
		case CONST_RPTR:
			if (emode)
				putstring("&const ");
			else 
				putch('*');
			break;
		case MEMPTR:
			if (this->lnode[this->li])
				fprintf(out_file, "%s::",
					(this->lnode[this->li])->string);
		}
	}

	if (this->n) name_print(this->n);

	for(i = 1 ;i <= this->ri ;i++)
		switch (this->right[i]){
		case RP:
			putch(')');
			break;
		case VEC:
			putch('[');
			{
				Pvec v;
				Pexpr d;
				int s;

				v = (Pvec)(this->rnode[i]);
				d = v->dim;
				s = v->size;
				if (d) expr_print(d);
				if (s) fprintf(out_file, "%d", s);
			}
			putch(']');
			break;
		case FCT:
			fct_dcl_prt((Pfct)(this->rnode[i]));
			break;
		case FIELD:
			{
				Pbase f;
				Pexpr d;
				int s;

				f = (Pbase)(this->rnode[i]);
				d = (Pexpr)f->b_name;
				s = f->b_bits;
				putch(':');
				if (d) expr_print(d);
				if (s) fprintf(out_file, "%d", s);
			}
			break;
		}
}

void Eprint(e)
Pexpr e;
{
	switch (e->base){
	case REF:
		if (((Pexpr)e)->u4.mem->u1.tp->base == FCT){
			name_print(((Pexpr)e)->u4.mem);
			break;
		}
	case NAME:
	case ID:
	case ZERO:
	case ICON:
	case CCON:
	case FCON:
	case STRING:
	case IVAL:
	case TEXT:
	case CM:
	case G_CM:
	case ELIST:
	case COLON:
	case ILIST:
	case DOT:
	case THIS:
	case CALL:
	case G_CALL:
	case ICALL:
	case ANAME:
		expr_print(e);
	case DUMMY:
		break;
	default:
		putch('(');
		expr_print(e);
		putch(')');
	}
}

/*
 *	Print the declaration for a name(list==0) or a name list(list!=0):
 *		For each name
 *		(1) print storage class
 *		(2) print base type
 *		(3) print the name with its declarators
 *	Avoid(illegal) repetition of basetypes which are class or enum declarations
 *	(A name list may contain names with different base types)
 *	list == SM :	terminator SM
 *	list == 0:	single declaration with terminator SM
 *	list == CM :	separator CM
 */
void name_dcl_print(this, list)
Pname this;
TOK list;
{
	Pname n;

	if (this == 0) error("0->N::dcl_print()", ea0, ea0, ea0, ea0);

	for(n = this ;n ;n = n->n_list) {
		Ptype t;
		int sm, tc;
		Ptype tt;

		t = n->u1.tp;
		sm = 0;
		if (t == 0){
			V1.u1.p = (char *)n;
			errorFIPC('i', "N::dcl_print(%n)T missing",
				&V1, ea0, ea0, ea0);
		}
		if (n->n_stclass == ENUM) continue;

		if (n->where.line != last_line.line)
			if (last_ll = n->where.line)
				putline(&n->where);
			else 
				putline(&last_line);

		tc = ((Pbase)t)->b_const;
		for(tt = t ;tt->base == TYPE ;tt = ((Pbase)tt)->b_name->u1.tp)
			tc |= ((Pbase)tt)->b_const;

		switch (t->base){
		case CLASS:
			if (n->base != TNAME){
				class_dcl_prt((Pclass)t);
				sm = 1;
			}
			break;

		case ENUM:
			enum_dcl_prt(((Penum)t), n);
			sm = 1;
			break;

		case FCT:
		{
			Pfct f;

			f = (Pfct)t;
			if (n->base == TNAME) puttok(TYPEDEF);

			if (f->f_inline){
				if (f->f_virtual || n->n_addr_taken){
					TOK st;
					Pstmt b;

					st = n->n_sto;
					b = f->body;
					f->body = 0;
					type_dcl_print(t, n);
					n->n_sto = st;
					f->body = b;
				}
				else 
					sm = 1;	/* no SM */
			}
			else if (n->u4.n_table == gtbl
				&& strcmp(n->u2.string, "main") == 0) {
				MAIN = 1;
				Look_use = look(gtbl, "main", 0);
				use(Look_use);
				type_dcl_print(t, n);
				MAIN = 0;
			}
			else 
				type_dcl_print(t, n);
			break;
		}

		case OVERLOAD:
		{
			Pgen g;
			Plist gl;

			g = (Pgen)t;
			fprintf(out_file, "\t/* overload %s: */\n", g->string);
			for(gl = g->fct_list ;gl ;gl = gl->l) {
				Pname nn;

				nn = gl->f;
				name_dcl_print(nn, 0);
				sm = 1;
			}
			break;
		}

		case ASM:
			fprintf(out_file, "asm(\"%s\")\n",((Pbase)t)->b_name);
			break;

		case INT:
		case CHAR:
		case LONG:
		case SHORT:
		tcx:
			/* do not allocate space for constants unless necessary */
			if (tc
			&& n->n_sto != EXTERN	/* extern const one */
						/* const one = 1 */
						/* allocates storage */
			&&(n->n_scope==EXTERN	/* FUDGE const one = 1 */
						/* is treated as static */
						/* need loader support */
				|| n->n_scope==STATIC || n->n_scope==FCT)
			) {
				if (n->n_evaluated){
					sm = 1;	/* no */
					break;
				}
			}
			tc = 0;
			/* FALLTHRU */

		default:
		{
			Pexpr i;

			i = n->u3.n_initializer;
			if (tc){
				switch (tt->base){
				case CHAR:
				case SHORT:
				case INT:
				case LONG:
					goto tcx;
				}
			}
			if (n->base == TNAME) puttok(TYPEDEF);
			if (n->n_stclass == REGISTER){
				Pname cln;

				/*(imperfect) check against member functions */
				cln = is_cl_obj(n->u1.tp);
				if (cln){
					Pclass cl;

					cl = (Pclass)cln->u1.tp;
					if (cl->csu != CLASS
					&& cl->clbase == 0
					&& cl->itor == 0
					&& cl->virt_count == 0) puttok(REGISTER);
				}
				else 
					puttok(REGISTER);
			}

			if (i){
				if (n->n_sto == EXTERN && n->n_stclass == STATIC) {
					n->u3.n_initializer = 0;
					type_dcl_print(t, n);
					puttok(SM);
					n->u3.n_initializer = i;
					n->n_sto = 0;
					type_dcl_print(t, n);
					n->n_sto = EXTERN;
				}
				else 
					type_dcl_print(t, n);
			}
			else if (n->n_evaluated &&((Pbase)t)->b_const){
				if (n->n_sto == EXTERN && n->n_stclass == STATIC){
					int v;

					v = n->n_evaluated;
					n->n_evaluated = 0;
					type_dcl_print(t, n);
					puttok(SM);
					n->n_evaluated = v;
					n->n_sto = 0;
					type_dcl_print(t, n);
					n->n_sto = EXTERN;
				}
				else 
					type_dcl_print(t, n);
			}
			else {
				if (fct_void == 0
				&& n->n_sto == 0
				&& this->n_stclass == STATIC
				&& n->u4.n_table == gtbl) {
					switch (t->base){
					case CHAR:
					case SHORT:
					case INT:
					case LONG:
					case FLOAT:
					case DOUBLE:
					case EOBJ:
					case PTR:
						/* "int a:" == "int a = 0;" */
						n->u3.n_initializer = i = zero;
					}
				}
				type_dcl_print(t, n);
			}

			if (n->n_scope != ARG){
				if (i){
					puttok(ASSIGN);
					if (t != i->u1.tp
					&& i->base != ZERO
					&& i->base != ILIST) {
						Ptype t1;

						t1 = n->u1.tp;
					cmp:
						switch (t1->base){
						default:
							expr_print(i);
							break;
						case TYPE:
							t1 = ((Pbase)t1)->b_name->u1.tp;
							goto cmp;
						case VEC:
							if (((Pvec)t1)->typ->base == CHAR){
								expr_print(i);
								break;
							}
						case PTR:
						case RPTR:
							if (i->u1.tp == 0 ||
							type_check(n->u1.tp,i->u1.tp,0)){
								bit oc;

								putch('(');
								oc = Cast;
								Cast = 1;
								type_print(t);
								Cast = oc;
								putch(')');
							}
							EPRINT(i);
						}
					}
					else 
						expr_print(i);
				}
				else if (n->n_evaluated){
					puttok(ASSIGN);
					if (n->u1.tp->base != INT){
						bit oc;

						putstring("((");
						oc = Cast;
						Cast = 1;
						type_print(n->u1.tp);
						Cast = oc;
						fprintf(out_file, ")%d)", n->n_val);
					}
					else
						fprintf(out_file, "%d", n->n_val);
				}
			}
		}
		}

		switch (list){
		case SM:
			if (sm == 0) puttok(SM);
			break;
		case 0:
			if (sm == 0) puttok(SM);
			return;
		case CM:
			if (n->n_list) puttok(CM);
			break;
		}
	}
}

/*
 *	print just the name itself
 */
void name_print(this)
Pname this;
{
	if (this == 0)
		errorFIPC('i', "0->N::print()", ea0, ea0, ea0, ea0);

	if (this->u2.string == 0){
		if (emode) putch('?');
		return;
	}

	switch (this->base){
	default:
		V1.u1.p = (char *)this;
		V2.u1.i = (int)this->base;
		errorFIPC('i', "%p->N::print() base=%d",
			&V1, &V2, ea0, ea0);
	case TNAME:
		putst(this->u2.string);
		return;
	case NAME:
	case ANAME:
		break;
	}

	if (emode){
		Ptable tbl;
		char *cs;
		bit f;

		cs = 0;
		f = 0;
		if (this->u1.tp){
			switch (this->u1.tp->base){
			case OVERLOAD:
			case FCT:
				f = 1;
			default:
				if (tbl = this->u4.n_table){
					if (tbl == gtbl){
						if (f == 0) putstring("::");
					}
					else {
						if (tbl->t_name){
							cs = tbl->t_name->u2.string;
							fprintf(out_file,"%s::", cs);
						}
					}
				}

				if (this->n_scope == ARG
				&& strcmp(this->u2.string, "this") == 0) {
					Ptype tt;
					Pname cn;

					/* tell which "this" it is */
					tt = ((Pptr)this->u1.tp)->typ;
					cn = ((Pbase)tt)->b_name;
					fprintf(out_file, "%s::", cn->u2.string);
				}

			case CLASS:
			case ENUM:
				break;
			}
			switch (this->n_oper){
			case TYPE:
				putstring("operator ");
				type_dcl_print(((Pfct)this->u1.tp)->returns, 0);
				break;
			case 0:
				putstring(this->u2.string);
				break;
			case DTOR:
				putch('~');
			case CTOR:
				if (cs)
					putstring(cs);
				else {
					putstring("constructor");
					f = 0;
				}
				break;
			case TNAME:
				putstring(this->u2.string);
				break;
			default:
				putstring("operator ");
				putstring(keys[this->n_oper]);
				break;
			}
			if (f) putstring("()");
		}
		else 
			if (this->u2.string) putstring(this->u2.string);
		return;
	}

	if (this->u1.tp){
		Ptable tbl;
		int i;

		i = this->n_union;
		switch (this->u1.tp->base){
		default:
			if (tbl = this->u4.n_table){
				Pname tn;
				if (tbl == gtbl){
					if (i) fprintf(out_file, "_O%d.__C%d_",i,i);
					break;
				}
				if (tn = tbl->t_name){
					if (i)
						fprintf(out_file, "_%s__O%d.__C%d_",
							tn->u2.string, i, i);
					else 
						fprintf(out_file, "_%s_",
							tn->u2.string);
					break;
				}
			}

			switch (this->n_stclass){
			case STATIC:
			case EXTERN:
				if (i)
					fprintf(out_file, "_O%d.__C%d_", i, i);
				else if (this->n_sto == STATIC
					&& this->u1.tp->base != FCT)
					fprintf(out_file, "_st%d_",
						this->lex_level);
				break;
			default:
				if (i)
					/*
					 * lex_level of element is
					 * 1 greater than type
					 */
					fprintf(out_file, "_au%d_U%d.__C%d_",
						this->lex_level-1, i, i);
				else 
					fprintf(out_file, "_au%d_",
						this->lex_level);
			}
			break;
		case CLASS:
		case ENUM:
			break;
		}
	}

	if (this->u2.string) putst(this->u2.string);
}


void type_print(this)
Ptype this;
{
	switch (this->base){
	case PTR:
	case RPTR:
		type_dcl_print((Pptr)this, 0);
		break;
	case FCT:
		fct_dcl_prt((Pfct)this);
		break;
	case VEC:
		type_dcl_print((Pvec)this, 0);
		break;
	case CLASS:
	case ENUM:
		if (emode)
			fprintf(out_file, "%s",
				(this->base == CLASS)? "class": "enum");
		else  {
			V1.u1.p = (char *)this;
			V2.u1.i = (int)this->base;
			errorFIPC('i', "%p->T::print(%k)", &V1,
				&V2, ea0, ea0);
		}
		break;
	case TYPE:
		if (Cast){
			if (((Pbase)this)->b_name->u1.tp->base == PTR)
			{
				name_print(((Pbase)this)->b_name);
				break;
			}
			type_print(((Pbase)this)->b_name->u1.tp);
			break;
		}
	default:
		base_dcl_prt((Pbase)this);
	}
}

#define SDEL	'_'

/*
 *	take a signature suitable for argument types for overloaded
 *	function names
 */
char *signature(this, p)
Ptype this;
register char *p;
{
	Ptype t;
	int pp;

	t = this;
	pp = 0;
xx:
	switch (t->base){
	case TYPE:
		t = ((Pbase)t)->b_name->u1.tp;
		goto xx;
	case PTR:
		*p++ = 'P';
		t = ((Pptr)t)->typ;
		pp = 1;
		goto xx;
	case RPTR:
		*p++ = 'R';
		t = ((Pptr)t)->typ;
		pp = 1;
		goto xx;
	case VEC:
		*p++ = 'V';
		t = ((Pvec)t)->typ;
		pp = 1;
		goto xx;
	case FCT:
	{
		Pfct f;
		Pname n;

		f = (Pfct)t;
		*p++ = 'F';

		if (pp){	/* "T" result type'_' */
			*p++ = 'T';
			p = signature(f->returns, p);
			*p++ = SDEL;
		}

		n = f->argtype;
		for(;n ;n = n->n_list) {	/* argtype'_' */
			if (n->n_xref) *p++ = 'X';
			p = signature(n->u1.tp, p);
			*p++ = SDEL;
		}
		*p++ = SDEL;
		if (f->nargs_known == ELLIPSIS) *p++ = 'E';
		*p = 0;
		return p;
	}
	}

	if (((Pbase)t)->b_unsigned) *p++ = 'U';

	switch (t->base){
	case ANY:
		*p++ = 'A';
		break;
	case ZTYPE:
		*p++ = 'Z';
		break;
	case VOID:
		*p++ = 'V';
		break;
	case CHAR:
		*p++ = (pp ? 'C': 'I');
		break;
	case SHORT:
		*p++ = (pp ? 'S': 'I');
		break;
	case EOBJ:
	case INT:
		*p++ = 'I';
		break;
	case LONG:
		*p++ = 'L';
		break;
	case FLOAT:
		*p++ = 'F';
		break;
	case DOUBLE:
		*p++ = 'D';
		break;
	case COBJ:
		*p++ = 'C';
		strcpy(p,((Pbase)t)->b_name->u2.string);
		while (*p++);
		*(p-1) = SDEL;
		break;
	case FIELD:
	default:
		V1.u1.i = (int)t->base;
		errorFIPC('i', "signature of %k", &V1, ea0, ea0, ea0);
	}

	*p = 0;
	return p;
}

static void base_dcl_prt(this)
Pbase this;
{
	Pname nn;
	Pclass cl;

	if (emode){
		if (this->b_virtual)puttok(VIRTUAL);
		if (this->b_inline)puttok(INLINE);
		if (this->b_const)puttok(CONST);
	}
	if (this->b_unsigned)puttok(UNSIGNED);

	switch (this->base){
	case ANY:
		putstring("any ");
		break;

	case ZTYPE:
		putstring("zero ");
		break;

	case VOID:
		if (emode == 0){
			puttok(CHAR);
			break;
		}
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
	case FLOAT:
	case DOUBLE:
		puttok(this->base);
		break;

	case EOBJ:
		nn = this->b_name;
eob:
		if (emode == 0)
			puttok(INT);
		else {
			puttok(ENUM);
			name_print(nn);
		}
		break;

	case COBJ:
		nn = this->b_name;
cob:
		cl = (Pclass)nn->u1.tp;
		switch (cl->csu){
		case UNION:
		case ANON:
			puttok(UNION);
			break;
		default:
			puttok(STRUCT);
		}
		putst(cl->string);
		break;

	case TYPE:
		if (emode == 0){
			switch (this->b_name->u1.tp->base){
			case COBJ:
				nn = ((Pbase)this->b_name->u1.tp)->b_name;
				goto cob;
			case EOBJ:
				nn = ((Pbase)this->b_name->u1.tp)->b_name;
				goto eob;
			}
		}
		name_print(this->b_name);
		break;

	default:
		if (emode){
			unsigned int sy;
			sy = this->base;
			if (0<sy && sy<=MAXTOK && keys[sy])
				fprintf(out_file, " %s", keys[sy]);
			else 
				putch('?');
		}
		else 
		{
			V1.u1.p = (char *)this;
			V2.u1.i = (int)this->base;
			errorFIPC('i', "%p->BT::dcl_print(%d)", &V1,
				&V2, ea0, ea0);
		}
	}
}

/*
 *	"this" type is the type of "n". Print the declaration
 */
void type_dcl_print(this, n)
Ptype this;
Pname n;
{
	Ptype t;
	Pfct f;
	Pvec v;
	Pptr p;
	TOK pre;

	t = this;
	pre = 0;
	if (t == 0) errorFIPC('i', "0->dcl_print()", ea0, ea0, ea0, ea0);
	if (n && n->u1.tp != t) {
		V1.u1.p = (char *)n;
		V2.u1.p = (char *)t;
		errorFIPC('i', "not %n'sT(%p)", &V1, &V2, ea0, ea0);
	}
	if (this->base == OVERLOAD){
		Pgen g;
		Plist gl;
		Pname nn;

		if (emode){
			puttok(OVERLOAD);
			return;
		}
		g = (Pgen)this;
		fprintf(out_file, "\t/* overload %s: */\n", g->string);
		for(gl = g->fct_list ;gl ;gl = gl->l) {
			nn = gl->f;
			type_dcl_print(nn->u1.tp, nn);
			if (gl->l)puttok(SM);
		}
		return;
	}
	tbuf = tbufvec[freetbuf];
	if (tbuf == 0){
		if (freetbuf == NTBUF-1)
			errorFIPC('i', "AT nesting overflow",
				ea0, ea0, ea0, ea0);
		tbufvec[freetbuf] = tbuf = (Sdclp) new(sizeof(struct dcl_buf));
	}
	freetbuf++;
	INIT(tbuf, n);
	if (n && n->n_xref) SFRONT(tbuf, PTR);

	while (t){
		TOK k;

		switch (t->base){
		case PTR:
			p = (Pptr)t;
			k = (p->rdo ? CONST_PTR: PTR);
			goto ppp;
		case RPTR:
			p = (Pptr)t;
			k = (p->rdo ? CONST_RPTR: RPTR);
	ppp:
			SFRONT(tbuf, k);
			if (emode && p->memof) DFRONT(tbuf, p->memof);
			pre = PTR;
			t = p->typ;
			break;
		case VEC:
			v = (Pvec)t;
			if (Cast){
				SFRONT(tbuf, PTR);
				pre = PTR;
			}
			else {
				if (pre == PTR) PARAN(tbuf);
				BACK(tbuf, VEC, (Pnode)v);
				pre = VEC;
			}
			t = v->typ;
			break;
		case FCT:
			f = (Pfct)t;
			if (pre == PTR)
				PARAN(tbuf)
			else if (emode && f->memof)
				DFRONT(tbuf, f->memof);
			BACK(tbuf, FCT, (Pnode)f);
			pre = FCT;

			if (f->f_inline && f->returns
			&& f->returns->base == VOID
			&& f->s_returns && f->s_returns->base != PTR)
				t = f->returns;
			else 
				t = (f->s_returns ? f->s_returns: f->returns);
			break;
		case FIELD:
			BACK(tbuf, FIELD, (Pnode)t);
			BASE(tbuf,(Pbase)((Pbase)t)->b_fieldtype);
			t = 0;
			break;
		case CLASS:
		case ENUM:
			V1.u1.i = (int)t->base;
			errorFIPC('i', "unexpected%k asBT",
				&V1, ea0, ea0, ea0);
		case 0:
			errorFIPC('i', "noBT(B=0)", ea0, ea0, ea0, ea0);
		case TYPE:
			if (Cast){ /* unravel type in case it contains vectors */
				t = ((Pbase)t)->b_name->u1.tp;
				break;
			}
		default:
			BASE(tbuf,(Pbase)t);
			t = 0;
			break;
		} /* switch */
	} /* while */

	dcl_put(tbuf);
	freetbuf--;
}

static void fct_dcl_prt(this)
Pfct this;
{
	Pname nn, at;

	if (emode){
		putch('(');
		for(nn = this->argtype ;nn ;) {
			type_dcl_print(nn->u1.tp, 0);
			if (nn = nn->n_list) puttok(CM);
			else	break;
		}
		switch (this->nargs_known){
		case 0:
		case ELLIPSIS:
			puttok(ELLIPSIS);
			break;
		}
		putch(')');
		return;
	}

	at = (this->f_this)? this->f_this:
		(this->f_result)? this->f_result: this->argtype;
	putch('(');
	if (this->body && Cast == 0) {
		int vargPresent = 0;
		if(this->nargs_known == 0 || this->nargs_known == ELLIPSIS)
			vargPresent = 1;
		if(vargPresent){
			int someargs = 0;
			putstring("\n#ifdef __STDC__\n");
			for (nn=at; nn;) {
				someargs = 1;
				type_dcl_print(nn->u1.tp,0);
				name_print(nn);
				if (nn=nn->n_list) puttok(CM); else break;
			}
			if(someargs == 1) puttok(CM);
			puttok(ELLIPSIS);
			putch(')');
			putstring("\n#else\n");
		}
		for(nn = at ;nn ;) {
			name_print(nn);
			if (nn = nn->n_list) puttok(CM);
			else 
				break;
		}
		putch(')');

		if (at) name_dcl_print(at, SM);
		if(vargPresent) putstring("\n#endif\n");

		if (MAIN){
			putstring("{ _main(); ");
			stmt_print((Pstmt)this->body);
#ifdef apollo
			/* Force linker to use C++ produces exit() */
			putst(" exit(0);");
#endif
			putch('}');
		}
		else 
			stmt_print((Pstmt)this->body);
	}
	else 
		putch(')');
}

void print_members(this)
Pclass this;
{
	int i;
	Pname nn;

	if (this->clbase){
		Pclass bcl;

		bcl = (Pclass)this->clbase->u1.tp;
		print_members(bcl);
	}

	for(nn = get_mem(this->memtbl, i=1) ;nn ;nn = get_mem(this->memtbl,++i)) {
		if (nn->base == NAME && nn->n_union == 0
		&& nn->u1.tp->base != FCT
		&& nn->u1.tp->base != OVERLOAD
		&& nn->u1.tp->base != CLASS
		&& nn->u1.tp->base != ENUM
		&& nn->n_stclass != STATIC) {
			Pexpr i;

			i = nn->u3.n_initializer;
			nn->u3.n_initializer = 0;
			name_dcl_print(nn, 0);
			nn->u3.n_initializer = i;
		}
	}
}

static void class_dcl_prt(this)
Pclass this;
{
	Plist l;
	TOK c;
	int i, sm, sz;
	Pname nn;

	c = (this->csu == CLASS)? STRUCT: this->csu;
	for(nn = get_mem(this->memtbl, i=1) ;nn ;nn = get_mem(this->memtbl,++i)) {
		if (nn->base == NAME && nn->n_union == 0) {
			if (nn->u1.tp->base == CLASS)
				class_dcl_prt((Pclass)nn->u1.tp);
		}
		else if (nn->base == TNAME &&((Pbase)nn->u1.tp)->base != COBJ)
			name_dcl_print(nn, 0);
	}

	puttok(c);
	putst(this->string);

	if (this->c_body == 0) return;
	this->c_body = 0;
	sm = 0;
	sz = tsizeof((Ptype)this);
	fprintf(out_file, "{\t/* sizeof %s == %d */\n",
		this->string, this->obj_size);
	if (this->real_size)
		print_members(this);
	else 
		putstring("char _dummy; ");
	putstring("};\n");

	if (this->virt_count){	/* print initialized jump-table */
		nn = get_mem(this->memtbl, i=1);
		for( ;nn ;nn = get_mem(this->memtbl,++i)) {
			/* declare function names */
			if (nn->base == NAME && nn->n_union == 0) {
				Ptype t;

				t = nn->u1.tp;
				switch (t->base){
				case FCT:
				{
					Pfct f;

					f = (Pfct)t;
					if (f->f_virtual == 0)break;
					if (f->f_inline && vtbl_opt == -1)
						puttok(STATIC);
					type_print(f->returns);
					name_print(nn);
					putstring("()");
					puttok(SM);
					break;
				}
				case OVERLOAD:
				{
					Pgen g;
					Plist gl;

					g = (Pgen)t;
					for(gl = g->fct_list ;gl ;gl = gl->l) {
						Pfct f;

						f = (Pfct)gl->f->u1.tp;
						if (f->f_virtual == 0)continue;
						if (f->f_inline)puttok(STATIC);
						type_print(f->returns);
						name_print(gl->f);
						putstring("()");
						puttok(SM);
					}
				}
				}
			}
		}

		switch (vtbl_opt){
		case -1:
			putstring("static ");
		case 1:
			fprintf(out_file, "int (*%s__vtbl[])() = {", this->string);
			for(i = 0 ;i < this->virt_count ;i++) {
				putstring("\n(int(*)()) ");
				name_print(this->virt_init[i]);
				puttok(CM);
			}
			putstring("0}");
			puttok(SM);
			break;
		case 0:
			fprintf(out_file, "extern int (*%s__vtbl[])();",
				this->string);
			break;
		}
	}

	for(nn = get_mem(this->memtbl, i=1) ;nn ;nn = get_mem(this->memtbl,++i)) {
		if (nn->base == NAME && nn->n_union == 0) {
			Ptype t;

			t = nn->u1.tp;
			switch (t->base){
			case FCT:
			case OVERLOAD:
				break;
			default:
				if (nn->n_stclass == STATIC){
					TOK b;

					b = nn->n_sto;		/* silly */
					nn->n_sto = 0;
					if (tconst(nn->u1.tp)){
						if (nn->n_assigned_to)
							nn->n_sto = STATIC;
						else {
							V1.u1.p = (char *)nn;
							error("uninitialized const%n",
								&V1, ea0, ea0, ea0);
						}
					}

					name_dcl_print(nn, 0);
					nn->n_sto = b;
				}
			}
		}
	}

	for(nn = get_mem(this->memtbl, i=1) ;nn ;nn = get_mem(this->memtbl,++i)) {
		if (nn->base == NAME && nn->n_union == 0) {
			Pfct f;

			f = (Pfct)nn->u1.tp;
			switch (f->base){
			case FCT:
				/* suppress duplicate or spurious declaration */
				if (f->f_virtual || f->f_inline)break;
			case OVERLOAD:
				name_dcl_print(nn, 0);
			}
		}
	}

	for(l = this->friend_list ;l ;l = l->l) {
		Pname nn;

		nn = l->f;
		switch (nn->u1.tp->base){
		case FCT:
			Cast = 1;
			name_dcl_print(nn, 0);
			Cast = 0;
			break;
		case OVERLOAD:
			l->f = nn = ((Pgen)nn->u1.tp)->fct_list->f;
			name_dcl_print(nn, 0);
			break;
		}
	}
}


static void enum_dcl_prt(this, n)
Penum this;
Pname n;
{
	if (this->mem){
		fprintf(out_file, "/* enum %s */\n", n->u2.string);
		name_dcl_print(this->mem, SM);
	}
}

int addrof_cm = 0;

void expr_print(this)
Pexpr this;
{
	if (this == 0) errorFIPC('i', "0->E::print()", ea0, ea0, ea0, ea0);
	if (this == this->u2.e1 || this == this->u3.e2) {
		V1.u1.p = (char *)this;
		V2.u1.i = (int)this->base;
		V3.u1.p = (char *)this->u2.e1;
		V4.u1.p = (char *)this->u3.e2;
		errorFIPC('i', "(%p%k)->E::print(%p %p)", &V1, &V2, &V3, &V4);
	}
	switch (this->base){
	case NAME:
	{
		Pname n;
		bit oc;

		n = (Pname)this;
		if (n->n_evaluated && n->n_scope != ARG) {
			if (this->u1.tp->base != INT){
				putstring("((");
				oc = Cast;
				Cast = 1;
				type_print(this->u1.tp);
				Cast = oc;
				fprintf(out_file, ")%d)", n->n_val);
			}
			else	fprintf(out_file, "%d", n->n_val);
		}
		else 
			name_print(n);
		break;
	}
	case ANAME:
		if (curr_icall){	/* in expansion: look it up */
			Pname n;
			int argno;
			Pin il;

			n = (Pname)this;
			argno = n->n_val;

			for(il = curr_icall ;il ;il = il->i_next)
				if (n->u4.n_table == il->i_table)goto aok;
			goto bok;
		aok:
			if (n = il->local[argno]) {
				name_print(n);
			}
			else {
				Pexpr ee;
				Ptype t;

				ee = il->arg[argno];
				t = il->tp[argno];
				if (ee == 0 || ee == this) {
					V1.u1.p = (char *)this;
					V2.u1.p = (char *)ee;
					errorFIPC('i', "%p->E::print(A %p)",
						&V1, &V2, ea0, ea0);
				}
				if (ee->u1.tp == 0 ||
				   (t != ee->u1.tp && type_check(t, ee->u1.tp, 0)
				    && is_cl_obj(t) == 0 && eobj == 0)
				) {
					bit oc;

					putstring("((");
					oc = Cast;
					Cast = 1;
					type_print(t);
					Cast = oc;
					putch(')');
					EPRINT(ee);
					putch(')');
				}
				else	EPRINT(ee);
			}
		}
		else {
		bok:	/* in body: print it: */
			name_print((Pname)this);
		}
		break;

	case ICALL:
	{
		Pexpr a0;
		int val;

		this->u4.il->i_next = curr_icall;
		curr_icall = this->u4.il;
		if (this->u4.il == 0)
			errorFIPC('i', "E::print: iline missing", ea0,
				ea0, ea0, ea0);
		a0 = this->u4.il->arg[0];
		val = QUEST;
		if (this->u4.il->fct_name->n_oper != CTOR)goto dumb;

		/*
		 *	find the value of "this"
	   	 *	if the argument is a "this" NOT assigned to
		 *	by the programmer, it was initialized
		 */
		switch (a0->base){
		case ZERO:
			val = 0;
			break;
		case ADDROF:
		case G_ADDROF:
			val = 1;
			break;
		case CAST:
			if (a0->u2.e1->base == ANAME || a0->u2.e1->base == NAME) {
				Pname a;

				a = (Pname)a0->u2.e1;
				if (a->n_assigned_to == FUDGE111) val = FUDGE111;
			}
		}
		if (val == QUEST) goto dumb;
		/*
		 *	now find the test:  "(this==0) ? _new(sizeof(X)) : 0"
		 *
		 *	e1 is a comma expression,
		 *	the test is either the first sub-expression
		 *		or the first sub-expression after the assignments
		 *			initializing temporary variables
		 */

	{	Pexpr e, q;

		e = this->u2.e1;
	lx:
		switch (e->base){
		case CM:
			if (e->u3.e2->base == QUEST || e->u2.e1->base == ASSIGN)
				e = e->u3.e2;
			else	e = e->u2.e1;
			goto lx;

		case QUEST:
			q = e->u4.cond;
			if (q->base==EQ && q->u2.e1->base==ANAME && q->u3.e2==zero){
				Pexpr saved;
				Pexpr from;

				saved = new_expr(0, 0, 0, 0);
				from = (val == 0)? e->u2.e1: e->u3.e2;
				*saved = *e;
				*e = *from;
				EPRINT(this->u2.e1);
				*e = *saved;
				expr__dtor(saved);
				curr_icall = this->u4.il->i_next;
				return;
			}
		}
	}
	dumb:
		EPRINT(this->u2.e1);
		if (this->u3.e2) stmt_print((Pstmt)this->u3.e2);
		curr_icall = this->u4.il->i_next;
		break;
	}

	case REF:
	case DOT:
		EPRINT(this->u2.e1);
		puttok(this->base);
		name_print(this->u4.mem);
		break;

	case VALUE:
		type_print(this->u4.tp2);
		puttok(LP);
		if (this->u2.e1) expr_print(this->u2.e1);
		puttok(RP);
		break;

	case SIZEOF:
		puttok(SIZEOF);
		if (this->u2.e1 && this->u2.e1 != dummy) {
			EPRINT(this->u2.e1);
		}
		else if (this->u4.tp2){
			putch('(');
			if (this->u4.tp2->base == CLASS){
				if (((Pclass)this->u4.tp2)->csu == UNION)
					putstring("union ");
				else putstring("struct ");
				putstring(((Pclass)this->u4.tp2)->string);
			}
			else 
				type_print(this->u4.tp2);
			putch(')');
		}
		break;

	case NEW:
		puttok(NEW);
		type_print(this->u4.tp2);
		if (this->u2.e1){
			putch('(');
			expr_print(this->u2.e1);
			putch(')');
		}
		break;

	case DELETE:
		puttok(DELETE);
		expr_print(this->u2.e1);
		break;

	case CAST:
		putch('(');
		if (this->u4.tp2->base != VOID){
			bit oc;

			putch('(');
			oc = Cast;
			Cast = 1;
			type_print(this->u4.tp2);
			Cast = oc;
			putch(')');
		}

		EPRINT(this->u2.e1);
		putch(')');
		break;

	case ICON:
	case FCON:
	case CCON:
	case ID:
		if (this->u2.string) putst(this->u2.string);
		break;

	case STRING:
		fprintf(out_file, "\"%s\"", this->u2.string);
		break;

	case THIS:
	case ZERO:
		putstring("0 ");
		break;

	case IVAL:
		fprintf(out_file, "%d", this->u2.i1);
		break;

	case TEXT:
		if (this->u3.string2)
			fprintf(out_file, " %s_%s", this->u2.string,
				this->u3.string2);
		else 
			fprintf(out_file, " %s", this->u2.string);
		break;

	case DUMMY:
		break;

	case G_CALL:
	case CALL:
	{
		Pname fn, at;
		Pfct f;


		fn = this->u4.fct_name;
		if (fn){
			f = (Pfct)fn->u1.tp;
			if (f->base == OVERLOAD){	/* overloaded after call */
				Pgen g;

				g = (Pgen)f;
				this->u4.fct_name = fn = g->fct_list->f;
				f = (Pfct)fn->u1.tp;
			}
			name_print(fn);
			at = (f->f_this) ? f->f_this:
					 (f->f_result) ? f->f_result: f->argtype;
		}
		else {
			f = (Pfct)this->u2.e1->u1.tp;
			if (f){	/* pointer to fct */
				while (f->base == TYPE)
					f = (Pfct)((Pbase)f)->b_name->u1.tp;
				if (f->base == PTR){
					putstring("(*");
					expr_print(this->u2.e1);
					putch(')');
					f = (Pfct)((Pptr)f)->typ;
					while (f->base == TYPE)
						f = (Pfct)((Pbase)f)->b_name->u1.tp;
				}
				else 
					EPRINT(this->u2.e1);

				/* must be FCT */
				at = (f->f_result ? f->f_result: f->argtype);
			}
			else {
				/* virtual: argtype encoded
				 * f_this already linked to f_result and/or argtype
				 */
				if (this->u2.e1->base == QUEST)
					at = (Pname)this->u2.e1->u2.e1->u4.tp2;
				else
					at = (Pname)this->u2.e1->u4.tp2;
				EPRINT(this->u2.e1);
			}
		}
		puttok(LP);
		if (this->u3.e2){
			if (at){
				Pexpr e;

				e = this->u3.e2;
				while (at){
					Pexpr ex;
					Ptype t;

					t = at->u1.tp;
					if (e == 0){
						if(fn) V1.u1.p = fn->u2.string;
						else   V1.u1.p = "??";
						errorFIPC('i',
							"A missing for %s()", &V1,
							ea0, ea0, ea0);
					}
					if (e->base == ELIST){
						ex = e->u2.e1;
						e = e->u3.e2;
					}
					else 
						ex = e;

					if (ex == 0){
						V1.u1.p = (char *)t;
						errorFIPC('i',
							"A ofT%t missing", &V1,
							ea0, ea0, ea0);
					}
					if (t != ex->u1.tp && ex->u1.tp
					&& type_check(t, ex->u1.tp, 0)
					&& is_cl_obj(t) == 0 && eobj == 0) {
						bit oc;

						putch('(');
						oc = Cast;
						Cast = 1;
						type_print(t);
						Cast = oc;
						putch(')');
#ifdef sun
						if (ex->base == DIV) {
							/* defend against perverse
							 * SUN cc bug
							 */
							putstring("(0+");
							EPRINT(ex);
							putch(')');
						} else
#endif
						EPRINT(ex);
					}
					else expr_print(ex);
					at = at->n_list;
					if (at)puttok(CM);
				}
				if (e){
					puttok(CM);
					expr_print(e);
				}
			}
			else 
				expr_print(this->u3.e2);
		}
		puttok(RP);
		break;
	}

	case ASSIGN:
		/* suppress assignment to "this" that has been optimized away */
		if (this->u2.e1->base == ANAME
		&&((Pname)this->u2.e1)->n_assigned_to == FUDGE111) {
			Pname n;
			int argno;
			Pin il;

			n = (Pname)this->u2.e1;
			argno = n->n_val;
			for(il = curr_icall ;il ;il = il->i_next)
				if (il->i_table == n->u4.n_table) goto akk;
			goto bkk;
		akk:
			if (il->local[argno] == 0){
				expr_print(this->u3.e2);
				break;
			}
		}
	case EQ:
	case NE:
	case GT:
	case GE:
	case LE:
	case LT:
	bkk:
		EPRINT(this->u2.e1);
		puttok(this->base);

		if (this->u2.e1->u1.tp != this->u3.e2->u1.tp
		&& this->u3.e2->base != ZERO) {
			/* cast, but beware of int != long etc. */
			Ptype t1;

			t1 = this->u2.e1->u1.tp;
		cmp:
			switch (t1->base){
			default:
				break;
			case TYPE:
				t1 = ((Pbase)t1)->b_name->u1.tp;
				goto cmp;
			case PTR:
			case RPTR:
			case VEC:
				if (this->u3.e2->u1.tp == 0 ||
				(((Pptr)t1)->typ != ((Pptr)this->u3.e2->u1.tp)->typ
				 && type_check(t1, this->u3.e2->u1.tp, 0))
				) {
					bit oc;

					putch('(');
					oc = Cast;
					Cast = 1;
					type_print(this->u2.e1->u1.tp);
					Cast = oc;
					putch(')');
				}
			}
		}
		EPRINT(this->u3.e2);
		break;

	case DEREF:
		if (this->u3.e2){
			EPRINT(this->u2.e1);
			putch('[');
			expr_print(this->u3.e2);
			putch(']');
		}
		else {
			putch('*');
			EPRINT(this->u2.e1);
		}
		break;

	case ILIST:
		puttok(LC);
		if (this->u2.e1) expr_print(this->u2.e1);
		puttok(RC);
		break;

	case ELIST:
	{
		Pexpr e;

		e = this;
		for(;;) {
			if (e->base == ELIST){
				expr_print(e->u2.e1);
				if (e = e->u3.e2)
					puttok(CM);
				else 
					return;
			}
			else {
				expr_print(e);
				return;
			}
		}
	}

	case QUEST:
	{
		/* look for(&a == 0) etc. */
		extern bit binary_val;
		int i;

		Neval = 0;
		binary_val = 1;
		i = eval(this->u4.cond);
		binary_val = 0;
		if (Neval == 0)
			expr_print(i ? this->u2.e1: this->u3.e2);
		else {
			EPRINT(this->u4.cond);
			puttok(QUEST);
			EPRINT(this->u2.e1);
			puttok(COLON);
			EPRINT(this->u3.e2);
		}
		break;
	}
	case CM:	/* do &(a,b) =>(a,&b) for previously checked inlines */
	case G_CM:
		puttok(LP);

		switch (this->u2.e1->base){
		case ZERO:
		case IVAL:
		case ICON:
		case NAME:
		case DOT:
		case REF:
		case FCON:
		case FVAL:
		case STRING:
			goto le2;	/* suppress constant a: &(a,b) =>(&b) */
		default:
			{
				int oo;	/* &(a,b) does not affect a */

				oo = addrof_cm;
				addrof_cm = 0;
				EPRINT(this->u2.e1);
				addrof_cm = oo;
			}
			puttok(CM);
		le2:
			if (addrof_cm){
				switch (this->u3.e2->base){
				case CAST:
					switch (this->u3.e2->u3.e2->base){
					case CM:
					case G_CM:
					case ICALL:
						goto ec;
					}
				case NAME:
				case DOT:
				case DEREF:
				case REF:
				case ANAME:
					puttok(ADDROF);
					addrof_cm--;
					EPRINT(this->u3.e2);
					addrof_cm++;
					break;
				case ICALL:
				case CALL:
				case CM:
				case G_CM:
				ec:
					EPRINT(this->u3.e2);
					break;
				case G_CALL:
					/* &( e, ctor() ) with temporary
					 * optimized away
					 */
					if (this->u3.e2->u4.fct_name &&
					this->u3.e2->u4.fct_name->n_oper == CTOR){
						addrof_cm--;
						EPRINT(this->u3.e2);
						addrof_cm++;
						break;
					}
				default:
					V1.u1.i = (int)this->u3.e2->base;
					errorFIPC('i', "& inlineF call(%k)",
						&V1, ea0, ea0, ea0);
				}
			}
			else 
				EPRINT(this->u3.e2);
			puttok(RP);
		}
		break;

	case UMINUS:
	case NOT:
	case COMPL:
		puttok(this->base);
		EPRINT(this->u3.e2);
		break;
	case ADDROF:
	case G_ADDROF:
		switch (this->u3.e2->base){	/* & *e1 or &e1[e2] */
		case DEREF:
			if (this->u3.e2->u3.e2 == 0){	/* &*e == e */
				expr_print(this->u3.e2->u2.e1);
				return;
			}
			break;
		case ICALL:
			/* assumes inline expanded into ,-expression */
			addrof_cm++;
			EPRINT(this->u3.e2);
			addrof_cm--;
			return;
		}

		/* suppress cc warning on &fct */
		if (this->u3.e2->u1.tp == 0 || this->u3.e2->u1.tp->base != FCT)
			puttok(ADDROF);

		EPRINT(this->u3.e2);
		break;

	case PLUS:
	case MINUS:
	case MUL:
	case DIV:
	case MOD:
	case LS:
	case RS:
#ifdef DK
	case AND:	putc('&'); break;
	case OR:	putc('|'); break;
	case ER:	putc('^'); break;
	case ANDAND:	putstring("&&"); break;
	case OROR:	putstring("||"); break;
#else
	case AND:
	case OR:
	case ER:
	case ANDAND:
	case OROR:
#endif
	case ASOR:
	case ASER:
	case ASAND:
	case ASPLUS:
	case ASMINUS:
	case ASMUL:
	case ASMOD:
	case ASDIV:
	case ASLS:
	case ASRS:
	case DECR:
	case INCR:
		EPRINT(this->u2.e1);
		puttok(this->base);
		EPRINT(this->u3.e2);
		break;

	default:
		V1.u1.p = (char *)this;
		V2.u1.i = (int)this->base;
		errorFIPC('i', "%p->E::print%k", &V1, &V2, ea0, ea0);
	}
}

Pexpr aval(a)
Pname a;
{
	int argno;
	Pin il;
	Pexpr aa;

	argno = a->n_val;
	for(il = curr_icall ;il ;il = il->i_next)
		if (il->i_table == a->u4.n_table) goto aok;
	return 0;
aok:
	aa = il->arg[argno];
ll:
	switch (aa->base){
	case CAST:
		aa = aa->u2.e1;
		goto ll;
	case ANAME:
		return aval((Pname)aa);
	default:
		return aa;
	}
}

#define putcond()       putch('('); expr_print(this->u2.e); putch(')')

void stmt_print(this)
Pstmt this;
{
	if (this->where.line != last_line.line)
		if (last_ll = this->where.line)
			putline(&this->where);
		else 
			putline(&last_line);

	if (this->memtbl && this->base != BLOCK) {
		/* also print declarations of temporaries */
		Ptable tbl;
		int i, bl;
		Pname n, cn;
		char *s;

		puttok(LC);
		tbl = this->memtbl;
		this->memtbl = 0;
		bl = 1;
		for(n = get_mem(tbl, i=1) ;n ;n = get_mem(tbl,++i)) {

			if (n->u1.tp == (Ptype)any_type) continue;
			/* avoid double declarartion of temporaries from inlines */
			s = n->u2.string;
			if (s[0] != '_' || s[1] != 'X') {
				name_dcl_print(n, 0);
				bl = 0;
			}

			if (bl &&(cn = is_cl_obj(n->u1.tp))
			&& has_dtor((Pclass)cn->u1.tp))
				bl = 0;
		}
		if (bl){
			Pstmt sl;

			sl = this->s_list;
			this->s_list = 0;
			stmt_print(this);
			this->memtbl = tbl;
			puttok(RC);
			if (sl){
				this->s_list = sl;
				stmt_print(sl);
			}
		}
		else {
			stmt_print(this);
			this->memtbl = tbl;
			puttok(RC);
		}
		return;
	}
	switch (this->base){
	default:
		V1.u1.i = (int)this->base;
		errorFIPC('i', "S::print(base=%k)", &V1, ea0, ea0, ea0);

	case ASM:
		fprintf(out_file, "asm(\"%s\");\n", this->u2.e);
		break;

	case DCL:
		name_dcl_print(this->u1.d, SM);
		break;

	case BREAK:
	case CONTINUE:
		puttok(this->base);
		puttok(SM);
		break;

	case DEFAULT:
		puttok(this->base);
		puttok(COLON);
		stmt_print(this->s);
		break;

	case SM:
		if (this->u2.e){
			expr_print(this->u2.e);
			if (this->u2.e->base == ICALL && this->u2.e->u3.e2)
				break;	/* a block: no SM */
		}
		puttok(SM);
		break;

	case WHILE:
		puttok(WHILE);
		putcond();
		if (this->s->s_list){
			puttok(LC);
			stmt_print(this->s);
			puttok(RC);
		}
		else 
			stmt_print(this->s);
		break;

	case DO:
		puttok(DO);
		stmt_print(this->s);
		puttok(WHILE);
		putcond();
		puttok(SM);
		break;

	case SWITCH:
		puttok(SWITCH);
		putcond();
		stmt_print(this->s);
		break;

	case RETURN:
#ifdef RETBUG
		if (this->u3.empty && this->u1.ret_tp) {
			/* fudge to bypass C bug (see simpl.c) */
			Pname cn;
			cn = is_cl_obj(this->u1.ret_tp);
			fprintf(out_file,
				"{struct %s _plain_silly;return _plain_silly;}",
				cn->u2.string);
		}
		else
			
#endif
		{
		puttok(RETURN);
		if (this->u2.e){
			if (this->u1.ret_tp && this->u1.ret_tp!=this->u2.e->u1.tp){
				Ptype tt;

				tt = this->u1.ret_tp;
			gook:
				switch (tt->base){
				case TYPE:
					tt = ((Pbase)tt)->b_name->u1.tp;
					goto gook;
				case COBJ:
					break;	/* cannot cast to struct */
				case RPTR:
				case PTR:
					if (((Pptr)tt)->typ ==
						((Pptr)this->u2.e->u1.tp)->typ)
						break;

				default:
					if (this->u2.e->u1.tp == 0 ||
					type_check(this->u1.ret_tp,this->u2.e->u1.tp,0))
					{
						int oc;

						oc = Cast;
						putch('(');
						Cast = 1;
						type_print(this->u1.ret_tp);
						Cast = oc;
						putch(')');
					}
				}
			}
			EPRINT(this->u2.e);
		}
		puttok(SM);
		}
		while (this->s_list && this->s_list->base == SM)
			this->s_list = this->s_list->s_list;
		break;

	case CASE:
		puttok(CASE);
		EPRINT(this->u2.e);
		puttok(COLON);
		stmt_print(this->s);
		break;

	case GOTO:
		puttok(GOTO);
		name_print(this->u1.d);
		puttok(SM);
		break;

	case LABEL:
		name_print(this->u1.d);
		putch(':');
		stmt_print(this->s);
		break;

	case IF:
	{
		int val;

		val = QUEST;
		if (this->u2.e->base == ANAME){
			Pname a;
			Pexpr arg;

			a = (Pname)this->u2.e;
			arg = aval(a);

			if (arg)
				switch (arg->base){
				case ZERO:
					val = 0;
					break;
				case ADDROF:
				case G_ADDROF:
					val = 1;
					break;
				case IVAL:
					val = arg->u2.i1 != 0;
				}
		}

		switch (val){
		case 1:
			stmt_print(this->s);
			break;
		case 0:
			if (this->u3.else_stmt)
				stmt_print(this->u3.else_stmt);
			else 
				puttok(SM);	/* null statement */
			break;
		default:
			puttok(IF);
			putcond();
			if (this->s->s_list){
				puttok(LC);
				stmt_print(this->s);
				puttok(RC);
			}
			else 
				stmt_print(this->s);
			if (this->u3.else_stmt){
				puttok(ELSE);
				if (this->u3.else_stmt->s_list){
					puttok(LC);
					stmt_print(this->u3.else_stmt);
					puttok(RC);
				}
				else 
					stmt_print(this->u3.else_stmt);
			}
		}
		break;
	}

	case FOR:
	{
		int fi;

		fi = (this->u3.for_init &&(this->u3.for_init->base != SM
					|| this->u3.for_init->memtbl
					|| this->u3.for_init->s_list));

		if (fi){
			puttok(LC);
			stmt_print(this->u3.for_init);
		}
		putstring("for(");
		if (fi == 0 && this->u3.for_init)
			expr_print(this->u3.for_init->u2.e);
		putch(';');	/* to avoid newline: not puttok(SM) */
		if (this->u2.e) expr_print(this->u2.e);
		putch(';');
		if (this->u1.e2) expr_print(this->u1.e2);
		puttok(RP);
		stmt_print(this->s);
		if (fi) puttok(RC);
		break;
	}

	case PAIR:
		if (this->s && this->u2.s2){
			puttok(LC);
			stmt_print(this->s);
			stmt_print(this->u2.s2);
			puttok(RC);
		}
		else {
			if (this->s) stmt_print(this->s);
			if (this->u2.s2) stmt_print(this->u2.s2);
		}
		break;

	case BLOCK:
		puttok(LC);
		if (this->u1.d) name_dcl_print(this->u1.d, SM);
		if (this->memtbl && this->u2.own_tbl){
			int i;
			Pname n;

			n = get_mem(this->memtbl, i = 1);
			for(;n ;n = get_mem(this->memtbl,++i)) {
				if (n->u1.tp && n->n_union==0
				&& n->u1.tp != (Ptype)any_type)
					switch (n->n_scope){
					case ARGT:
					case ARG:
						break;
					default:
						name_dcl_print(n, 0);
					}
			}
		}
		if (this->s) stmt_print(this->s);
		putstring("}\n");
		if (last_ll && this->where.line) last_line.line++;
	}

	if (this->s_list)stmt_print(this->s_list);
}

/*
 *	print the declarations of the entries in the order they were inserted
 *	ignore labels(tp==0)
 */
static void tbl_dcl_prt(this, s, pub)
Ptable this;
TOK s;
TOK pub;
{
	register Pname *np, n;
	register int i;

	if (this == 0)return;

	np = this->entries;
	for(i = 1 ;i < this->free_slot ;i++) {
		n = np[i];
		switch (s){
		case 0:
			name_dcl_print(n, 0);
			break;
		case EQ:
			if (n->u1.tp && n->n_scope == pub)
				name_dcl_print(n, 0);
			break;
		case NE:
			if (n->u1.tp && n->n_scope != pub)
				name_dcl_print(n, 0);
			break;
		}
	}
}
