/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/s_string.h	1.6.2.2"
#ident "@(#)s_string.h	1.7 'attmail mail(1) command'"
/* extensible strings */

#ifndef _string_h
#define _string_h
#include <string.h>

typedef struct string {
	char *base;	/* base of string */
	char *end;	/* end of allocated space+1 */
	char *ptr;	/* ptr into string */
} string;

#ifdef lint
# ifdef __STDC__
extern string *s_clone(string*);
extern int s_curlen(string*);
extern void s_delete(string*);
extern string *s_dup(string*);
extern int s_getc(string*);
extern int s_peek(string*);
extern int s_putc(string*, int);
extern string *s_reset(string*);
extern string *s_restart(string*);
extern void s_skipc(string*);
extern void s_terminate(string*);
extern char *s_to_c(string*);
extern char *s_ptr_to_c(string*);
# else
extern string *s_clone();
extern int s_curlen();
extern void s_delete();
extern string *s_dup();
extern int s_getc();
extern int s_peek();
extern int s_putc();
extern string *s_reset();
extern string *s_restart();
extern void s_skipc();
extern void s_terminate();
extern char *s_to_c();
extern char *s_ptr_to_c();
# endif
#else
# define s_clone(s)	s_copy((s)->ptr)
# define s_curlen(s)	((s)->ptr - (s)->base)
# define s_delete(s)	(s_free(s), (s) = 0)
# define s_dup(s)	s_copy((s)->base)
# define s_getc(s)	(*((s)->ptr++))
# define s_peek(s)	(*((s)->ptr))
# define s_putc(s,c)	(((s)->ptr<(s)->end) ? (*((s)->ptr)++ = (c)) : s_grow((s),(c)))
# define s_reset(s)	((s) ? (*((s)->ptr = (s)->base) = '\0' , (s)) : s_new())
# define s_restart(s)	((s)->ptr = (s)->base , (s))
# define s_skipc(s)	((s)->ptr++)
# define s_space(s)	((s)->end - (s)->base)
# define s_terminate(s)	(((s)->ptr<(s)->end) ? (*(s)->ptr = 0) : (s_grow((s),0), (s)->ptr--, 0))
# define s_to_c(s)	((s)->base)
# define s_ptr_to_c(s)	((s)->ptr)
#endif /* lint */

#ifdef __STDC__
extern string *s_append(string *to, const char *from);
extern string *s_array(char *, int len);
extern string *s_copy(const char *);
extern void s_free(string*);
extern int s_grow(string *sp, int c);
extern string *s_new(void);
extern string *s_parse(string *from, string *to);
extern char *s_read_line(FILE *fp, string *to);
extern int s_read_to_eof(FILE *fp, string *to);
extern string *s_seq_read(FILE *fp, string *to, int lineortoken);
extern void s_skipwhite(string *from);
extern string *s_tok(string*,char*);
extern void s_tolower(string*);
extern string *s_xappend(string *to, const char *from1, const char *from2, ...);
#else
extern string *s_append();
extern string *s_array();
extern string *s_copy();
extern void s_free();
extern int s_grow();
extern string *s_new();
extern string *s_parse();
extern char *s_read_line();
extern int s_read_to_eof();
extern string *s_seq_read();
extern void s_skipwhite();
extern string *s_tok();
extern void s_tolower();
extern string *s_xappend();
#endif

/* controlling the action of s_seq_read */
#define TOKEN	0	/* read the next whitespace delimited token */
#define LINE	1	/* read the next logical input line */
#define s_getline(a,b)	s_seq_read(a,b,LINE)
#define s_gettoken(a,b)	s_seq_read(a,b,TOKEN)

#endif
