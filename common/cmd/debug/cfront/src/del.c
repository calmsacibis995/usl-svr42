/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/del.c	1.1"
/************************************************************

del.c:

	walk the trees to reclaim storage

**************************************************************/

#include "cfront.h"

void name_del(this)
Pname this;
{
	Pexpr i;

	i = this->u3.n_initializer;
	NFn++;
	if (TO_DEL(this->u1.tp)) type_del(this->u1.tp);
	if (i && i != (Pexpr)1) if (TO_DEL(i)) expr_del(i);
	this->n_tbl_list = name_free;
	name_free = this;
}

void type_del(this)
Ptype this;
{
	this->permanent = 3;	/* do not delete twice */
	switch (this->base){
	case TNAME:
	case NAME:
		V1.u1.p = (char *)this;
		V2.u1.p = (char *)((Pname)this)->u2.string;
		V3.u1.i = (int)this->base;
		errorFIPC('i', "%d->T::del():N %s %d", &V1, &V2, &V3, ea0);
	case FCT:
	{
		Pfct f;

		f = (Pfct)this;
		if (TO_DEL(f->returns)) type_del(f->returns);
		break;
	}
	case VEC:
	{
		Pvec v;

		v = (Pvec)this;
		if (TO_DEL(v->dim)) expr_del(v->dim);
		if (TO_DEL(v->typ)) type_del(v->typ);
		break;
	}
	case PTR:
	case RPTR:
	{
		Pptr p;

		p = (Pptr)this;
		if (TO_DEL(p->typ)) type_del(p->typ);
		break;
	}
	}

	delete((char *)this);
}

void expr_del(this)
Pexpr this;
{
	this->permanent = 3;
	switch (this->base){
	case IVAL:
		if (this == one)return;
	case FVAL:
	case THIS:
	case ICON:
	case FCON:
	case CCON:
	case STRING:
	case TEXT:
		goto dd;
	case DUMMY:
	case ZERO:
	case NAME:
		return;
	case CAST:
	case SIZEOF:
	case NEW:
	case VALUE:
		if (TO_DEL(this->u4.tp2)) type_del(this->u4.tp2);
		break;
	case REF:
	case DOT:
		if (TO_DEL(this->u2.e1)) expr_del(this->u2.e1);
		if (this->u4.mem) if (TO_DEL(this->u4.mem)) name_del(this->u4.mem);
		if (this->u3.e2) if (TO_DEL(this->u3.e2)) expr_del(this->u3.e2);
		goto dd;
	case QUEST:
		if (TO_DEL(this->u4.cond)) expr_del(this->u4.cond);
		break;
	case ICALL:
		delete((char *)this->u4.il);
		goto dd;
	}

	if (TO_DEL(this->u2.e1)) expr_del(this->u2.e1);
	if (TO_DEL(this->u3.e2)) expr_del(this->u3.e2);

dd:
	this->u2.e1 = expr_free;
	expr_free = this;
	NFe++;
}

void stmt_del(this)
Pstmt this;
{
	this->permanent = 3;
	switch (this->base){
	case SM:
	case WHILE:
	case DO:
	case RETURN:
	case CASE:
	case SWITCH:
		if (TO_DEL(this->u2.e)) expr_del(this->u2.e);
		break;
	case PAIR:
		if (TO_DEL(this->u2.s2)) stmt_del(this->u2.s2);
		break;
	case BLOCK:
		if (TO_DEL(this->u1.d)) name_del(this->u1.d);
		if (TO_DEL(this->s)) stmt_del(this->s);
		if (this->u2.own_tbl)
			if (TO_DEL(this->memtbl)) table_del(this->memtbl);
		if (TO_DEL(this->s_list)) stmt_del(this->s_list);
		goto dd;
	case FOR:
		if (TO_DEL(this->u2.e)) expr_del(this->u2.e);
		if (TO_DEL(this->u1.e2)) expr_del(this->u1.e2);
		if (TO_DEL(this->u3.for_init)) stmt_del(this->u3.for_init);
		break;
	case IF:
		if (TO_DEL(this->u2.e)) expr_del(this->u2.e);
		if (TO_DEL(this->u3.else_stmt)) stmt_del(this->u3.else_stmt);
		break;
	}

	if (TO_DEL(this->s)) stmt_del(this->s);
	if (TO_DEL(this->s_list)) stmt_del(this->s_list);
dd:
	this->s_list = stmt_free;
	stmt_free = this;
	NFs++;
}

void table_del(this)
Ptable this;
{
	register int i;

	for(i = 1 ;i < this->free_slot ;i++) {
		Pname n;

		n = this->entries[i];
		if (n == 0) errorFIPC('i', "table.del(0)", ea0, ea0, ea0, ea0);
		if (n->n_stclass == STATIC)continue;
		switch (n->n_scope){
		case ARG:
		case ARGT:
			break;
		default:
		{
			char *s;

			s = n->u2.string;
			if (s &&(s[0]!='_' || s[1]!='X')) delete(s);
			/* delete n; */
			name_del(n);
		}
		}
	}
	delete((char *)this->entries);
	delete((char *)this->hashtbl);
	delete((char *)this);
}
