/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/tqueue.h	1.1"

#include <stdio.h>

typedef struct toknode	* Ptoknode;
struct toknode {
	TOK      tok;			/* token for parser */
	YYSTYPE  retval;			/* $arg */
	Ptoknode next;
	Ptoknode last;
};
extern Ptoknode front;
extern Ptoknode rear;

extern void addtok();			/* add tok to rear of Q */
extern TOK  deltok();			/* take tok from front of Q */

extern void tlex();
extern TOK lalex();

extern YYSTYPE yylval;
extern TOK tk;				/* last token returned; */

extern char* image();
