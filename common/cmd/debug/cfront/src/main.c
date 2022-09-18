/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/main.c	1.1"
/***********************************************************************

main.c:

	Initialize global environment
	Read argument line
	Start compilation
	Clean up end exit

**************************************************************************/

#include <time.h>
#include <ctype.h>

long start_time, stop_time;

#include "cfront.h"

char *prog_name = "<<cfront 1.2.4 8/23/87>>";

char *src_file_name = 0;

bit old_fct_accepted;
bit fct_void;

char *line_format = "\n# %d \"%s\"\n";

struct ea V1, V2, V3, V4;

#ifdef unix
#include <signal.h>

int core_dump()
{
	if (error_count){
		fprintf(stderr, "sorry, cannot recover from previous error\n");
	}
	else 
		errorFIPC('i', "bus error(or something nasty like that)",
			ea0, ea0, ea0, ea0);
	ext(99);
}
#endif

Plist isf_list;
Pstmt st_ilist;
Pstmt st_dlist;
Ptable sti_tbl;
Ptable std_tbl;

int Nspy = 0;
int Nfile = 1, Nline = 0, Ntoken = 0;
int Nfree_store = 0, Nalloc = 0, Nfree = 0;
int Nname = 0;
int Nn = 0, Nbt = 0, Nt = 0, Ne = 0, Ns = 0, Nc = 0, Nstr = 0, Nl = 0;

extern int NFn, NFtn, NFbt, NFpv, NFf, NFe, NFs, NFc, NFl;
int vtbl_opt = -1;	/* how to deal with vtbls:
			 * -1 static and defined
			 * 0 external and supposed to be defined elsewhere
			 * 1 external and defined
			 */

int simpl_init();
int typ_init();
int syn_init();
int lex_init();
int error_init();
int print_free();
int read_align();
int print_align();

void spy(s)
char *s;
{
	if (s) fprintf(stderr,"%s:\n",s);
	fprintf(stderr,"files=%d lines=%d tokens=%d\n",Nfile,Nline,Ntoken);
	fprintf(stderr,"Names: distinct=%d global=%d type=%d\n",
		Nname,max(gtbl),max(ktbl));
	fflush(stderr);
	if (start_time && stop_time){
		fprintf(stderr,"start time: %s",ctime(&start_time));
		fprintf(stderr,"stop time:  %s",ctime(&stop_time));
		fprintf(stderr,"real time delay %ld: %d lines per second\n",
			stop_time-start_time, Nline/(stop_time-start_time) );
		fflush(stderr);
	}
	print_free();
	fflush(stderr);
	fprintf(stderr,"sizeof: n=%d bt=%d f=%d pv=%d s=%d e=%d c=%d l=%d\n",
		sizeof(struct name), sizeof(struct basetype), sizeof(struct fct),
		sizeof(struct ptr), sizeof(struct stmt), sizeof(struct expr),
		sizeof(struct classdef), sizeof(struct elist));
	fprintf(stderr,"alloc(): n=%d bt=%d t=%d e=%d s=%d c=%d l=%d str=%d\n",
		Nn,Nbt,Nt,Ne,Ns,Nc,Nl,Nstr);
	fprintf(stderr,"free(): n=%d bt=%d t=%d e=%d s=%d c=%d l=%d\n",
		NFn,NFbt,NFpv + NFf,NFe,NFs, NFc,NFl);
	fflush(stderr);
	fprintf(stderr,"%d errors\n",error_count);
	fflush(stderr);
}

Pname dcl_list;		/* declarations generated while declaring something else */

char *st_name();	/* generates names of static ctor, dtor callers */

/*
 *	run the appropriate stages
 */
void run()
{
	Pname n, m;
	int i;
	Ptype t;
	char *ctor_name, *dtor_name;

	i = 1;
	while (n = syn()){
		Pname nn;
		Pname nx;

		if (n == (Pname)1) continue;

	/*	Now process list of declarations.
	 *	Make sure the type stays around for all elements ofthe list.
	 *	example:
	 *		double cos(double x);
	 *		double cos(double x), sin(double x);
	 */
		if (n->n_list) PERM(n->u1.tp);

		for(nn = n ;nn ;nn = nx) {
			Pname dx, d;

			nx = nn->n_list;
			nn->n_list = 0;

			if (name_dcl(nn, gtbl, EXTERN) == 0) continue;

			if (error_count)continue;

			name_simpl(nn);

			/* handle generated declarations */
			for(d = dcl_list ;d ;d = dx) {
				dx = d->n_list;
				name_dcl_print(d, 0);
				name__dtor(d);
			}
			dcl_list = 0;

			if (nn->base) name_dcl_print(nn, 0);

			switch (nn->u1.tp->base){ /* clean up */
			default:
			{	Pexpr i;

				i = nn->u3.n_initializer;
				if (i && i != (Pexpr)1)
					if (TO_DEL(i)) expr_del(i);
				break;
			}

			case FCT:
			{	Pfct f;

				f = (Pfct)nn->u1.tp;
				if (f->body &&(debug || f->f_inline == 0)) {
					if (TO_DEL(f->body))
						stmt_del((Pstmt)f->body);
					/* f->body = 0;  leave to detect
					 * re-definition, but do not use it
					 */
				}
				break;
			}

			case CLASS:
			{	Pclass cl;
				register Pname p;
				Pname px;

				cl = (Pclass)nn->u1.tp;
				for(p = cl->mem_list ;p ;p = px) {
					px = p->n_list;
					if (p->u1.tp) {
						Pfct f;

						switch (p->u1.tp->base){
						case FCT:
							f = (Pfct)p->u1.tp;
							if (f->body &&
							(debug || f->f_inline == 0)){
								if(TO_DEL(f->body))
									stmt_del((Pstmt)f->body);
								f->body = 0;
							}
						case CLASS:
						case ENUM:
							break;
						case COBJ:
						case EOBJ:
							if(TO_DEL(p)) name_del(p);
							break;
						default:
							name__dtor(p);
						}
					}
					else 
						name__dtor(p);
				}
				cl->mem_list = 0;
				cl->permanent = 3;
				break;
			}
			}
			if (TO_DEL(nn)) name_del(nn);
		}
		lex_clear();
	}

	switch (no_of_undcl){
	case 0:
		break;
	case 1:
		V1.u1.p = (char *)undcl;
		errorFIPC('w', "undeclaredF%n called", &V1, ea0, ea0, ea0);
		break;
	default:
		V1.u1.i = (int)no_of_undcl;
		V2.u1.p = (char *)undcl;
		errorFIPC('w', "%d undeclaredFs called; for example%n",
			&V1, &V2, ea0, ea0);
	}

	switch (no_of_badcall){
	case 0:
		break;
	case 1:
		V1.u1.p = (char *)badcall;
		errorFIPC('w',"laredWoutAs calledWAs", &V1, ea0, ea0, ea0);
		break;
	default:
		V1.u1.i = no_of_badcall;
		V2.u1.p = (char *)badcall;
		errorFIPC('w', "%d Fs declaredWoutAs calledWAs; for example%n",
			&V1, &V2, ea0, ea0);
	}

	if (fct_void == 0) {
		for(m=get_mem(gtbl,i=1) ;m ;m = get_mem(gtbl,++i)) {
			if (m->base == TNAME || m->n_sto == EXTERN
			|| m->n_stclass == ENUM)
				continue;

			t = m->u1.tp;
			if (t == 0)continue;
		ll:
			switch (t->base){
			case TYPE:
				t = ((Pbase)t)->b_name->u1.tp;
				goto ll;
			case CLASS:
			case ENUM:
			case COBJ:
			case OVERLOAD:
			case VEC:
				continue;
			case FCT:
				if (((Pfct)t)->f_inline ||((Pfct)t)->body == 0)
					continue;
			}

			if (m->n_addr_taken == 0 && m->n_used == 0
			&& tconst(m->u1.tp) == 0)
				if (m->n_sto == STATIC) {
					V1.u1.p = (char *)m;
					errorIloc('w', &m->where,
						"%n defined but not used",
						&V1, ea0, ea0, ea0);
				}
		}	/* for loop */
	}	/* if block */

	ctor_name = 0;
	dtor_name = 0;

	/* make an "init" function;
	 * it calls all constructors for static objects
	 */
	if (st_ilist){
		Pname n;
		Pfct f;

		n = new_name(st_name("_STI"));
		f = fct_ctor((Ptype)void_type, 0, 1);
		n->u1.tp = (Ptype)f;
		f->body = (Pstmt)new_block(st_ilist->where, 0, 0);
		f->body->s = st_ilist;
		f->body->memtbl = sti_tbl;
		n->n_sto = EXTERN;
		name_dcl(n, gtbl, EXTERN);
		name_simpl(n);
		name_dcl_print(n, 0);
		ctor_name = n->u2.string;
	}

	if (st_dlist){	/* make a "done" function;
			 * it calls all destructors for static objects
			 */
		Pname n;
		Pfct f;

		n = new_name(st_name("_STD"));
		f = fct_ctor((Ptype)void_type, 0, 1);
		n->u1.tp = (Ptype)f;
		f->body = (Pstmt)new_block(st_dlist->where, 0, 0);
		f->body->s = st_dlist;
		f->body->memtbl = std_tbl;
		n->n_sto = EXTERN;
		name_dcl(n, gtbl, EXTERN);
		name_simpl(n);
		name_dcl_print(n, 0);
		dtor_name = n->u2.string;
	}

#ifdef PATCH
		/*For fast load: make a static "__link" */
	if (ctor_name || dtor_name)
	{
		printf("static struct __link { struct __link * next;\n");
		printf("char(*ctor)(); char(*dtor)(); } __LINK = \n");
		printf("{(struct __link *)0, %s, %s };\n",
			ctor_name ? ctor_name : "0",
			dtor_name ? dtor_name : "0");
	}
#endif

	if (debug == 0){	/* print inline function definitions */
		Plist l;
		Pname n;
		Pfct f;

		for(l=isf_list ;l ;l = l->l) {
			n = l->f;
			f = (Pfct)n->u1.tp;

			if (f->base == OVERLOAD){
				n = ((Pgen)f)->fct_list->f;	/* first fct */
				f = (Pfct)n->u1.tp;
			}

			if (n->n_addr_taken ||(f->f_virtual && vtbl_opt != 0)) {
				putline(&n->where);
				type_dcl_print(n->u1.tp, n);
			}
		}
	}
	fprintf(out_file, "\n/* the end */\n");
}

bit warn = 1;	/* printout warning messages */
bit debug;	/* code generation for debugger */
char *afile = "";

int no_of_undcl = 0, no_of_badcall = 0;
Pname undcl, badcall;

/*
 *	read options, initialize, and run
 */
main(argc, argv)
int argc;
char *argv[];
{
	extern char *mktemp();
	register char *cp;
	short i;

	/* _main(); */
#ifdef unix
	signal(SIGILL, (void (*)())core_dump);
	signal(SIGIOT, (void (*)())core_dump);
	signal(SIGEMT, (void (*)())core_dump);
	signal(SIGFPE, (void (*)())core_dump);
	signal(SIGBUS, (void (*)())core_dump);
	signal(SIGSEGV, (void (*)())core_dump);
#endif

	error_init();

	for(i = 1 ;i < argc ;++i) {
		switch (*(cp=argv[i])) {
		case '+':
			while (*++cp) {
				switch (*cp){
				case 'w':
					warn = 0;
					errorFIPC('w',
				"+w option will not be supported in future releases",
						ea0, ea0, ea0, ea0);
					break;
				case 'd':
					debug = 1;
					errorFIPC('w',
				"+d option will not be supported in future releases",
						ea0, ea0, ea0, ea0);
					break;
				case 'f':
					src_file_name = cp + 1;
					goto xx;
				case 'x':	/* read cross compilation table */
					if (read_align(afile = cp+1)) {
						fprintf(stderr,
							"bad size-table(option +x)");
						exit(999);
					}
					goto xx;
				case 'V':	/* C compatability */
					fct_void = old_fct_accepted = 1;
fprintf(stderr, "\nwarning: +V option will not be supported in future releases\n");
					break;
				case 'e':
					switch (*++cp) {
					case '0':
					case '1':
						vtbl_opt = *cp-'0';
						break;
					default:
						fprintf(stderr,"bad +e option");
						exit(999);
					}
					break;
				case 'S':
					Nspy++;
					break;
				case 'L':
					break;
				default:
					fprintf(stderr,
					"%s: unexpected option: +%c ignored\n",
						prog_name, *cp);
				}
			}
		xx:
			break;
		default:
			fprintf(stderr,"%s: bad argument \"%s\"\n", prog_name, cp);
			exit(999);
		}
	}

	/* strips leading \n */
	fprintf(out_file,(line_format + 1), 1, src_file_name ? src_file_name: "");
	fprintf(out_file,"\n/* %s */\n", prog_name);
	if (src_file_name)fprintf(out_file,"/* < %s */\n", src_file_name);

	if (Nspy){
		start_time = time(0);
		print_align(afile);
	}
	fflush(stderr);
	otbl_init();
	lex_init();
	syn_init();
	typ_init();
	simpl_init();
	scan_started = 1;
	putline(&curloc);
	run();
	if (Nspy){
		stop_time = time(0);
		spy(src_file_name);
	}

	exit((0 <= error_count && error_count < 127) ? error_count: 127);
}

/*
 *	make name "pref|source_file_name|_" or "pref|source_file_name|_"
 *	where non alphanumeric characters are replaced with '_'
 */
char *st_name(pref)
char *pref;
{
	int prefl, strl;
	char *name, *p;

	prefl = strlen(pref);
	strl = prefl + 2;
	if (src_file_name) strl += strlen(src_file_name);
	name = (char *) new(strl);
	strcpy(name, pref);
	if (src_file_name) strcpy(name+prefl, src_file_name);
	name[strl-2] = '_';
	name[strl-1] = 0;
	p = name;
	while (*++p) if (!isalpha(*p) && !isdigit(*p)) *p = '_';
	return name;
}


Pstmt new_block(ll, nn, ss)
Loc ll;
Pname nn;
Pstmt ss;
{
	Pstmt Xthis_block;

	Xthis_block = 0;
	Xthis_block = new_stmt(Xthis_block, BLOCK, ll, ss);
	Xthis_block->u1.d = nn;

	return Xthis_block;
}
