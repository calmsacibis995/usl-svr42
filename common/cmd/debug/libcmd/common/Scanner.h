/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libcmd/common/Scanner.h	1.3"
#ifndef Scanner_h
#define Scanner_h
//
// NAME
//	Scanner, Token
//
// ABSTRACT
//	Lexical analyzer for the command language
//
// DESCRIPTION
//	The Scanner accepts input a line at a time, and produces
//	an list of Token pointers.
//	All data items are private.  Scanner is a friend of Token.
//

#include "string.h"
#include "Link.h"

enum TokType
{
	NoTok = 0,
	DEC,
	HEX,
	OCT,
	CHARCONST,
	STRING,
	NAME,
	AMPERSAND,
	ARROW,
	AT,
	BACKSLASH,
	BANG,
	BAR,
	COLON,
	COMMA,
	DOLLAR,
	DOT,
	EQUAL,
	GRAVE,
	GREATER,
	GREATEREQUAL,
	HAT,
	LBRACK,
	LCURLY,
	LESS,
	LPAREN,
	MINUS,
	NL,
	PERCENT,
	PLUS,
	POUND,
	QUESTION,
	RBRACK,
	RCURLY,
	RPAREN,
	SEMI,
	SLASH,
	STAR,
	TILDE,
	WHITESPACE,

	BADNUMBER,
	BADTOKEN,
	BADSTRING,
	BADCHAR
};

class Token;

extern char *fmtstring( const char * );
extern char *fmtchar( const char * );
extern Token *find_alias( const char * );
extern void make_alias( const char *, Token * );
extern void rm_alias( const char * );

extern char *print_tok_list( Token * );
extern void print_alias( const char * );

class Token : public Link
{
	TokType	 _op;	
	short	 _line;	// line number (usually 1)
	short	 _pos;	// charater offset (zero based)
	union
	{
		long	 _val;	// the "value", if decimal
		char	*_str;	// the "value", if name or string, or hex or
				// octal number; we preserve the string form
				// for hex or octal numbers in case they
				// are part of a file name (0010.c) and 
				// reprinting in ASCII would lose leading 0s or
				// case information (0xab vs 0xAb)
	} _value;

	friend class Scanner;
public:
		 Token(TokType op, int line, int pos)
			{ _op = op; _line = line;
			  _pos = pos; _value._val = 0; }
		 Token(Token *t);
		~Token();

	Token	*next();
	Token	*prev();

	TokType	 op()	{ return _op; }
	int	 line()	{ return _line; }
	int	 pos()	{ return _pos; }
	long	 val();
	char	*str()	{ return _value._str; }
	void	set_str(char *s) { _value._str = s; }
	void	makenum(char *s);

	Token	*alias() { return (_op == NAME)
				? find_alias( _value._str ) : 0;
			 }

	char	*print();
	Token	*subst( Token * );
	Token	*clone();

#ifdef DEBUG
	char	*dump();
#endif
};

class Scanner
{
	Token	*token_head;	// the list of Token pointers
	Token	*token_tail;	// the last Token in the list
	int	 lineno;	// line number within the current command
public:
		 Scanner()	{ lineno = 0; token_head = 0; token_tail = 0; }
		~Scanner()	{ clear(); }

	void	 scan( const char * );
	void	 clear();
	Token	*token()	{ return token_head; }
};

#endif /* Scanner_h */
