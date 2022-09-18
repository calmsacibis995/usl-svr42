/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/error.c	1.2"

/*
 * error.c :
 *
 *	write error messages
 *	Until scan_started != 0 no context can be assumed
 */

#include "cfront.h"
#include "size.h"

int error_count = 0;
static int no_of_warnings;
char scan_started = 0;

static char *abbrev_tbl []= {
	" argument",
	" base",
	" class",
	" declaration",
	" expression",
	" function",
	"G",
	"H",
	" initialize",
	"J",
	" constructor",
	" list",
	" member",
	" name",
	" object",
	" pointer",
	" qualifie",
	" reference",
	" statement",
	" type",
	" undefined",
	" variable",
	" with",
	" expected",
	"Y",
	"Z",
};

struct ea *ea0 = 0;

static void print_loc();
static void print_context();
static struct ea *new_ea();

void error_init()
{
	static char errbuf [BUFSIZ];
	setbuf(stderr, errbuf);
	ea0 = (struct ea *) new(sizeof(struct ea));
}

#define INTERNAL 127

void ext(a)
int a;
{
	/* if(a == INTERNAL) abort(); */
	if (error_count == 0) error_count = 1;
	exit(error_count);
}

static void print_loc()
{
	Loc *sl;
	Loc *dl;

	sl = Cstmt ?(&Cstmt->where): 0;
	dl = Cdcl ?(&Cdcl->where): 0;

	if (sl && dl && sl->file == dl->file)	/* Cstmt and Cdcl in same file */
		if (sl->line <= dl->line)
			put(dl, out_file);
		else 
			put(sl, out_file);
	else if (sl && sl->file == curr_file)	/* Cstmt in current file */
		put(sl, out_file);
	else if (dl && dl->file == curr_file)	/* Cdcl in current file */
		put(dl, out_file);
	else 
		put(&curloc, out_file);
}

static void print_context()
{
	(void) putc('\n',out_file);
}

static char in_error = 0;
static Loc dummy_loc;

void yyerror(s)
const char *s;
{
	errorIloc(0, &dummy_loc, s, ea0, ea0, ea0, ea0);
}

int error(s, a0, a1, a2, a3)
const char *s;
struct ea *a0, *a1, *a2, *a3;
{
	return errorIloc(0, &dummy_loc, s, a0, a1, a2, a3);
}

int errorFloc(lc, s, a0, a1, a2, a3)
Loc *lc;
const char *s;
struct ea *a0, *a1, *a2, *a3;
{
	return errorIloc(0, lc, s, a0, a1, a2, a3);
}

int errorFIPC(t, s, a0, a1, a2, a3)
int t;
const char *s;
struct ea *a0, *a1, *a2, *a3;
{
	return errorIloc(t, &dummy_loc, s, a0, a1, a2, a3);
}

int suppress_error = 0;

/*
 *	"int" not "void" because of "pch" in lex.c
 *
 *	legal error types are:
 *		'w'		warning	(not counted in error count)
 *		'd'		debug
 *		's'		"not implemented" message
 *   		0		error 
 *  		'i'		internal error(causes abort)
 *		't'		error while printing error message
 */
int errorIloc(t, lc, s, a0, a1, a2, a3)
int t;
Loc *lc;
const char *s;
struct ea *a0, *a1, *a2, *a3;
{
	FILE *of;
	struct ea argv[4];
	struct ea *a;
	int c;

	if (suppress_error) return 0;

	if (in_error++)
		if (t!='t' || 4<in_error) {
			fprintf(stderr, "\nOops!, error while handling error\n");
			ext(13);
		}
		else if (t == 't')
			t = 'i';

	of = out_file;
	out_file = stderr;

	if (!scan_started || t=='t')
		putch('\n');
	else if (lc != &dummy_loc)
		put(lc, out_file);
	else 
		print_loc();

	switch (t){
	case 0:
		putstring("error: ");
		break;
	case 'w':
		no_of_warnings++;
		putstring("warning: ");
		break;
	case 's':
		putstring("sorry, not implemented: ");
		break;
	case 'i':
		if (error_count++){
			fprintf(out_file,
			"sorry, %s cannot recover from earlier errors\n",
			prog_name);
			ext(INTERNAL);
		}
		else 
			fprintf(out_file, "internal %s error: ", prog_name);
	}


	_vec_new((char *)argv, 4, sizeof(struct ea),(char *)new_ea);
	a = argv;
	argv[0] = *a0;
	argv[1] = *a1;
	argv[2] = *a2;
	argv[3] = *a3;

	while (c = *s++) {
		if ('A' <= c && c <= 'Z')
			putstring(abbrev_tbl[c-'A']);
		else if (c == '%'){
			int x;
			Ptype tt;
			Pname nn;

			switch (c = *s++){
			case 'k':
				x = a->u1.i;
				a++;
				if (0 < x && x <= MAXTOK && keys[x])
					fprintf(out_file, " %s", keys[x]);
				else 
					fprintf(out_file, " token(%d)", x);
				break;
			case 't':	/* Ptype */
				tt = (Ptype)a->u1.p;
				a++;
				if (tt){
					TOK pm;
					extern int ntok;
					int nt;

					pm = emode;
					nt = ntok;
					emode = ERROR;
					putch(' ');
					type_dcl_print(tt, 0);
					emode = pm;
					ntok = nt;
				}
				break;
			case 'n':	/* Pname */
				nn = (Pname)a->u1.p;
				a++;
				if (nn){
					TOK pm;

					/* suppress generated names */
					if (nn->u2.string[0] == '_' &&
					    nn->u2.string[1] == 'C')
						break;

					pm = emode;
					emode = ERROR;
					putch(' ');
					name_print(nn);
					emode = pm;
				}
				else
					putstring(" ?");
				break;
			case 'p':	/* pointer */
			{
				char *p, *f;

				p = a->u1.p;
				a++;
				f = sizeof(char*)==sizeof(int)?" %d":" %ld";
				fprintf(out_file, f, p);
				break;
			}
			case 'c':	/* char assumed passed as an int */
			{
				char x;

				x = a->u1.i;
				a++;
				putch(x);
				break;
			}
			case 'd':	/* int */
			{
				int i;

				i = a->u1.i;
				a++;
				fprintf(out_file, " %d", i);
				break;
			}
			case 's':	/* char * */
			{
				char *st;

				st = a->u1.p;
				a++;
				putst(st);
				break;
			}
			}
		}
		else 
			putch(c);
	}

	if (!scan_started) ext(4);

	switch (t){
	case 'd':
	case 't':
	case 'w':
		putch('\n');
		break;
	default:
		print_context();
	}
	fflush(stderr);

	/* now we may want to carry on */
	out_file = of;

	switch (t){
	case 't':
		if (--in_error) return 0;
	case 'i':
		ext(INTERNAL);
	case 0:
	case 's':
		if (MAXERR < ++error_count) {
			fprintf(stderr, "Sorry, too many errors\n");
			ext(7);
		}
	}

	in_error = 0;
	return 0;
}


static struct ea *new_ea(this)
struct ea *this;
{
	if (this == 0)
		this = (struct ea *) new(sizeof(struct ea));
	return this;
}
