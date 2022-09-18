/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/table.c	1.2"

#include "cfront.h"

/*
 *	keys[]  holds the external form for tokens with fixed representation 
 *	illegal tokens and those with variable representation have 0 entries
 *
 *	the class table functions assume that new initializes store to 0
 */
char *keys [MAXTOK+1];

/*
 *	create a symbol table with "size" entries
 *	the scope of table is enclosed in the scope of "nx"
 *
 *	both the vector of class name pointers and the hash table
 *	are initialized containing all zeroes
 *	
 *	to simplify hashed lookup entries[0] is never used
 *	so the size of "entries" must be "size+1" to hold "size" entries
 */
Ptable new_table(sz, nx, n)
short sz;
Ptable nx;
Pname n;
{
	Ptable this;

	this = (Ptable)new(sizeof(struct table));
	this->base = TABLE;
	this->t_name = n;
	this->size = sz = (sz <= 0 ? 2 :(sz + 1));

	this->entries = (Pname *)new(sizeof(Pname) * sz);
	this->hashsize = sz = (sz * 3) / 2;
	this->hashtbl = (short *)new(sizeof(short) * sz);
	this->next = nx;
	this->free_slot = 1;
	return this;
}

/*
 *	look for "s" in table, ignore entries which are not of "k" type
 *	look and insert MUST be the same lookup algorithm
 */
Pname look(this, s, k)
Ptable this;
char *s;
TOK k;
{
	Ptable t;
	register char *p, *q;
	register int i;
	Pname n, *np;
	int rr, mx, firsti;
	short *hash;

	p = s;
	i = 0;
	while (*p)
		i += (i + *p++);
	rr = (0 <= i)? i:(-i);

	for(t = this ;t ;t = t->next) {
		/* in this and all enclosing scopes look for name"s" */
		np = t->entries;
		mx = t->hashsize;
		hash = t->hashtbl;
		firsti = i = rr % mx;

		do {
			if (hash[i] == 0) goto not_found;
			n = np[hash[i]];
			if (n == 0)
				errorFIPC('i', "hashed lookup", ea0,
					ea0, ea0, ea0);
			p = n->u2.string;
			q = s;
			while (*p && *q)
				if (*p++!= *q++) goto nxt;
			if (*p == *q) goto found;
		nxt:
			if (mx <= ++i) i = 0;	/* wrap around */
		} while (i != firsti);

	found:
		for(;n ;n = n->n_tbl_list) {
			/* for all name "s"s look for a key match */
			if (n->n_key == k) return n;
		}

	not_found: ;
	}

	return 0;	/* not found && no enclosing scope */
}

bit Nold;	/* non-zero if last insert() field */

/*
 *	the lookup algorithm MUST be the same as look
 *	if nx is found return the older entry otherwise a copy of nx;
 *	Nold = (nx found) ? 1 : 0;
 */
Pname insert(this, nx, k)
Ptable this;
Pname nx;
TOK k;
{
	register char *p;
	register int i;
	Pname n, *np, *link;
	int firsti, mx;
	short *hash;
	char *s;

	np = this->entries;
	mx = this->hashsize;
	hash = this->hashtbl;
	s = nx->u2.string;

	if (s == 0){
		V1.u1.p = (char *)this;
		V2.u1.i = (int)k;
		errorFIPC('i', "%d->insert(0,%d)", &V1, &V2, ea0, ea0);
	}
	nx->n_key = k;
	if (nx->n_tbl_list || nx->u4.n_table){
		V1.u1.p = (char *)nx;
		errorFIPC('i', "%n in two tables", &V1, ea0, ea0, ea0);
	}
	/* use simple hashing with linear search for overflow */

	p = s;
	i = 0;
	while (*p) i += (i + *p++);
	if (i < 0) i = (-i);
	firsti = i = i % mx;

	do {	/* look for name "s" */
		if (hash[i]== 0){
			hash[i] = this->free_slot;
			goto add_np;
		}
		n = np[hash[i]];
		if (n == 0)
			errorFIPC('i', "hashed lookup", ea0 , ea0, ea0, ea0);
		if (strcmp(n->u2.string, s) == 0) goto found;

		if (mx <= ++i) i = 0;	/* wrap around */
	} while (i != firsti);

	error((char *)"N table full", ea0, ea0, ea0, ea0);

found:
	for(;;) {
		if (n->n_key == k){
			Nold = 1;
			return n;
		}
		if (n->n_tbl_list)
			n = n->n_tbl_list;
		else {
			link = &(n->n_tbl_list);
			goto re_allocate;
		}
	}

add_np:
	if (this->size <= this->free_slot){
		grow(this, 2 * this->size);
		return insert(this, nx, k);
	}

	link = &(np[this->free_slot++]);

re_allocate:
	{
		Pname nw;
		char *ps;

		nw = new_name(0);
		*nw = *nx;
		ps = (char *)new(strlen(s)+1);	/* copy string to safer store */
		strcpy(ps, s);
		Nstr++;
		nw->u2.string = ps;
		nw->u4.n_table = this;
		*link = nw;
		Nold = 0;
		Nname++;
		return nw;
	}
}


void grow(this, g)
Ptable this;
int g;
{
	short *hash;
	register int j, i;
	int mx, firsti;
	register Pname *np;
	Pname n;
	char *s, *q;
	register char *p;


	if (g <= this->free_slot){
		V1.u1.i = g;
		V2.u1.i = (int)this->free_slot;
		errorFIPC('i', "table.grow(%d,%d)", &V1, &V2, ea0, ea0);
	}
	if (g <= this->size) return;
	this->size = mx = g + 1;

	np = (Pname *)new(sizeof(Pname) * mx);
	for(j = 0 ;j < this->free_slot ;j++) np[j] = this->entries[j];
	delete((char *)this->entries);
	this->entries = np;

	delete((char *)this->hashtbl);
	this->hashsize = mx = (g * 3)/ 2;
	hash = this->hashtbl = (short *)new(sizeof(short) * mx);

	for(j = 1 ;j < this->free_slot ;j++) {
		s = np[j]->u2.string;
		p = s;
		i = 0;
		while (*p) i += (i + *p++);
		if (i < 0) i = (-i);
		firsti = i = i % mx;

		do {	/* look for name "s" */
			if (hash[i] == 0){
				hash[i] = j;
				goto add_np;
			}
			n = np[hash[i]];
			if (n == 0)
				errorFIPC('i', "hashed lookup", ea0,
					ea0, ea0, ea0);
			p = n->u2.string;
			q = s;
			while (*p && *q) if (*p++!= *q++) goto nxt;
			if (*p == *q) goto found;
		nxt:
			if (mx <= ++i) i = 0;	/* wrap around */
		} while (i != firsti);
		errorFIPC('i', "rehash??", ea0, ea0 , ea0, ea0);

	found:
		errorFIPC('i', "rehash failed", ea0, ea0, ea0, ea0);

	add_np: ;
	}
}

Pclass Ebase;
Pclass Epriv;	/* extra return value from lookc */

/*
 *	like look().
 *
 *	look and insert MUST be the same lookup algorithm
 */
Pname lookc(this, s)
Ptable this;
char *s;
{
	Ptable t;
	register char *p;
	register char *q;
	register int i;
	Pname n, *np, ttname;
	int rr, mx, firsti;
	short *hash;


	Ebase = 0;
	Epriv = 0;

	/* use simple hashing with linear search for overflow */

	p = s;
	i = 0;
	while (*p) i += (i + *p++);
	rr = (0 <= i)? i:(- i);

	for(t = this ;t ;t = t->next) {
		/* in this and all enclosing scopes look for name "s" */
		np = t->entries;
		mx = t->hashsize;
		hash = t->hashtbl;
		firsti = i = rr % mx;
		ttname = t->t_name;

		do {
			if (hash[i] == 0) goto not_found;
			n = np[hash[i]];
			if (n == 0)
				errorFIPC('i', "hashed lookup",
					ea0, ea0, ea0, ea0);
			p = n->u2.string;
			q = s;
			while (*p && *q)
				if (*p++!= *q++) goto nxt;
			if (*p == *q) goto found;
		nxt:
			if (mx <= ++i) i = 0;		/* wrap around */
		} while (i != firsti);
	found:
		do {	/* for all name "s"s look for a key match */
			if (n->n_key == 0) {
				if (ttname) {
					if (n->base == PUBLIC)
						n = n->u5.n_qualifier;
					else if (n->n_scope == 0)
						Epriv = (Pclass)ttname->u1.tp;
				}
				return n;
			}
		} while (n = n->n_tbl_list);

	not_found:
		if (ttname){
			Pclass cl;

			cl = (Pclass)ttname->u1.tp;
			if (cl && cl->clbase && cl->pubbase == 0)
				Ebase = (Pclass)cl->clbase->u1.tp;
		}
	}

	Ebase = Epriv = 0;
	return 0;	/* not found && no enclosing scope */
}


/*
 *	return a pointer to the i'th entry, or 0 if it does not exist
 */
Pname get_mem(this, i)
Ptable this;
int i;
{
	return (i<=0 || this->free_slot<=i) ? 0 : this->entries[i];
}

/*
 *	make "s" a new keyword with the representation(token) "toknum"
 *	"yyclass" is the yacc token(for example new_key("int",INT,TYPE); )
 *	"yyclass==0" means yyclass=toknum;
 */
void new_key(s, toknum, yyclass)
char *s;
TOK toknum;
TOK yyclass;
{
	Pname n, nn;

	n = new_name(s);
	nn = insert(ktbl, n, 0);

	nn->base = toknum;
	nn->u1.syn_class = (yyclass) ? yyclass: toknum;
	keys[(toknum == LOC)? yyclass: toknum] = s;
	name__dtor(n);
}


/*
 *	find the true name for "n", implicitly define if undefined
 *	f==1:	n()
 *	f==2:	r->n or o.n
 *	f==3:	&n
 *
 */
Pexpr find_name(this, n, f)
Ptable this;
register Pname n;
bit f;
{
	Pname q;
	register Pname qn;
	register Pname nn;
	Pclass cl;		/* class specified by q */
	Pexpr r;

	q = n->u5.n_qualifier;
	qn = 0;

	if (n->u4.n_table){
		nn = n;
		n = 0;
		if (f == 3) f = 0;
		goto xx;
	}

	if (q){
		Ptable tbl;

		if (q == sta_name)
			tbl = gtbl;
		else {
			Ptype t;

			t = (Ptype)((Pclass)q->u1.tp);
			if (t == 0){
				V1.u1.p = (char *)q;
				errorFIPC('i', "Qr%n'sT missing",
				    &V1, ea0, ea0, ea0);
			}
			if (q->base == TNAME){
				if (t->base != COBJ){
					V1.u1.i = (int)t->base;
					V2.u1.p = (char *)q;
					error("badT%k forQr%n", &V1, &V2,
						ea0, ea0);
					goto nq;
				}
				t = ((Pbase)t)->b_name->u1.tp;
			}
			if (t->base != CLASS){
				V1.u1.p = (char *)q;
				V2.u1.i = (int)t->base;
				error("badQr%n(%k)", &V1, &V2, ea0, ea0);
				goto nq;
			}
			cl = (Pclass)t;
			tbl = cl->memtbl;
		}

		qn = look(tbl, n->u2.string, 0);
		if (qn == 0){
			n->u5.n_qualifier = 0;
			nn = 0;
			goto def;
		}

		if (q == sta_name){	/* explicity global */
			use(qn);
			name__dtor(n);
			return (Pexpr)qn;
		}

		/* else check visibility */
	}
	else 
		if (f == 3) f = 0;

nq:
	if (cc->tot) {
		Ptable tbl;

		tbl = this;
		for(;;) { /* loop necessary to get past local re-definitions */
			nn = lookc(tbl, n->u2.string);

			if (nn == 0) goto qq;	/* try for friend */

			switch (nn->n_scope){
			case 0:
			case PUBLIC:
				if (nn->n_stclass == ENUM) break;

				if (nn->u1.tp->base == OVERLOAD)break;

				if (Ebase && cc->cot->clbase
				&& Ebase != (Pclass)cc->cot->clbase->u1.tp
				&& !has_friend(Ebase, cc->nof)) {
					V1.u1.p = (char *)n;
					error("%n is from a privateBC", &V1,
						ea0, ea0, ea0);
				}
				if (Epriv && Epriv != cc->cot
				&& !has_friend(Epriv, cc->nof)
				&& !(nn->n_protect && baseofname(Epriv, cc->nof)))
				{
					V1.u1.p = (char *)n;
					if (nn->n_protect)
						V2.u1.p = "protected";
					else
						V2.u1.p = "private";
					error("%n is %s", &V1, &V2, ea0, ea0);
				}
			}

			if (qn == 0 || qn == nn) break;

			if ((tbl = tbl->next) == 0){
				if (qn->n_scope == PUBLIC || has_friend(cl, cc->nof))
				{

					nn = qn;
					break;
				}
				else {
					if (f != 3){
						V1.u1.p = (char *)n;
						error("QdN%n not in scope",
						&V1, ea0, ea0, ea0);
						goto def;
					}
					break;
				}
			}
		}
	xx:
		if (nn == 0) goto def;
		use(nn);
		if (2 <= f){
			if (qn && nn->n_stclass == 0)
				switch (nn->n_scope) {
				case 0:
				case PUBLIC:	/* suppress virtual */
					switch (qn->u1.tp->base){
					case FCT:
					case OVERLOAD:
						if (f == 3) return (Pexpr)qn;
						*n = *qn;
						n->u5.n_qualifier = q;
						return (Pexpr)n;
					}
				}
			if (nn->u4.n_table == gtbl) {
				V1.u1.p = (char *)n;
				error("M%n not found", &V1, ea0, ea0, ea0);
			}
			if (n) name__dtor(n);
			return (Pexpr)nn;
		}

		switch (nn->n_scope){
		case 0:
		case PUBLIC:
			switch (nn->n_stclass){
			case 0:
				if (qn){	/* suppress virtual */
					switch (qn->u1.tp->base){
					case FCT:
					case OVERLOAD:
						*n = *qn;
						n->u5.n_qualifier = q;
						nn = n;
						n = 0;
					}
				}

				if (cc->c_this == 0){
					switch (nn->n_oper){
					case CTOR:
					case DTOR:
						break;
					default:
						/* in static member initializer */
						V1.u1.p = (char *)nn;
						error("%n cannot be used here",
						&V1, ea0, ea0, ea0);
						return (Pexpr)nn;
					}
				}

				if (n) name__dtor(n);
				r = new_ref(REF, cc->c_this, nn);
				use(cc->c_this);
				r->u1.tp = nn->u1.tp;
				return r;
			}
		default:
			if (n) name__dtor(n);
			return (Pexpr)nn;
		}
	}
qq:
	if (qn) {
		/*
		 *	check for p->base::mem :
		 *	nasty where derived::mem is public
		 *	and base::mem is private
		 *	NOT DONE
		 */
		if (qn->n_scope == 0 && !has_friend(cl, cc->nof)) {
			V1.u1.p = (char *)qn;
			error("%n is private", &V1, ea0, ea0, ea0);
			if (n) name__dtor(n);
			return (Pexpr)qn;
		}

		switch (qn->n_stclass){
		case STATIC:
			break;
		default:
			switch (qn->u1.tp->base){
			case FCT:
			case OVERLOAD:	/* suppress virtual */
				if (f == 1){
					V1.u1.p = (char *)qn;
					error("O missing for%n",&V1,ea0,ea0,ea0);
				}
				if (f == 3) return (Pexpr)qn;
				*n = *qn;
				n->u5.n_qualifier = q;
				return (Pexpr)n;
			default:
				if (f < 2){
					V1.u1.p = (char *)qn;
					error("O missing for%n",&V1,ea0,ea0,ea0);
				}
			}
		}

		if (n) name__dtor(n);
		return (Pexpr)qn;
	}

	if (nn = lookc(this, n->u2.string)){
		switch (nn->n_scope){
		case 0:
		case PUBLIC:
			if (nn->n_stclass == ENUM) break;

			if (nn->u1.tp->base == OVERLOAD) break;

			if (Ebase && !has_friend(Ebase, cc->nof)) {
				V1.u1.p = (char *)n;
				error("%n is from privateBC", &V1,ea0,ea0,ea0);
			}
			if (Epriv && !has_friend(Epriv, cc->nof)
			&& !(nn->n_protect && baseofname(Epriv, cc->nof))) {
				V1.u1.p = (char *)n;
				if (nn->n_protect)
					V2.u1.p = "protected";
				else
					V2.u1.p = "private";
				error("%n is %s", &V1, &V2, ea0, ea0);
			}
		}
	}

	if (nn){
		if (f == 2 && nn->u4.n_table == gtbl) {
			V1.u1.p = (char *)n;
			error("M%n not found", &V1, ea0, ea0, ea0);
		}
		use(nn);
		if (n) name__dtor(n);
		return (Pexpr)nn;
	}

def:	/* implicit declaration */
	n->u5.n_qualifier = 0;
	if (f == 1) {	/* function */
		if (n->u1.tp)
			errorFIPC('i', "find_name(fct_type?)",
				ea0, ea0, ea0, ea0);
		n->u1.tp = (Ptype)fct_ctor((Ptype)defa_type, 0, 0);
		n->n_sto = EXTERN;
	}
	else {
		n->u1.tp = (Ptype)any_type;
		if (this != any_tbl)
			if (cc->not && !strcmp(n->u2.string, cc->not->u2.string)
		  	&& (cc->cot->defined &(DEFINED|SIMPLIFIED) ) == 0) {
				V1.u1.p = (char *)cc->not;
				error("C%n isU", &V1, ea0, ea0, ea0);
			} 
			else 
			{
				V1.u1.p = (char *)n;
				error("%n isU", &V1, ea0, ea0, ea0);
			}
	}

	nn = name_dcl(n, gtbl, EXTERN);
	nn->n_list = 0;
	use(nn);
	use(nn);	/* twice to cope with "undef = 1;" */
	if (n) name__dtor(n);

	if (f == 1)
		if (fct_void){
			if (no_of_undcl++ == 0) undcl = nn;
		}
		else 
		{
			V1.u1.p = (char *)nn;
			errorFIPC('w', "undeclaredF%n called", 
	    			&V1, ea0, ea0, ea0);
		}
	return (Pexpr)nn;
}


Pexpr new_ref(ba, a, b)
TOK ba;
Pexpr a;
Pname b;
{
	Pexpr Xthis_ref;

	Xthis_ref = 0;
	Xthis_ref = (Pexpr)new_expr((Pexpr)Xthis_ref, ba, a, 0);
	Xthis_ref->u4.mem = b;

	return Xthis_ref;
}
