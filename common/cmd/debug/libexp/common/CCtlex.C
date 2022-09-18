/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libexp/common/CCtlex.C	1.6"

#include <string.h>
#include <ctype.h>
#include "CC.h"
#include "CCtokdefs.h"
#include "Interface.h"
#include "Language.h"
#include "Reg.h"
#include "lex_util.h"
#include "Value.h"

#define OP_ENTRY(e, s, t, v) e = v,
enum {
#include "CCops.h"
};

static Lex_input input;
static Buff 	 tbuff;
static Language  language;

YYSTYPE CCtretval;
#define Q(a,b) ( (CCtretval.i=(int)(b)), (a) )

void
CCtlex_init(char *instring, Language lang)
{
    language = lang;

    input.init(instring);

    int len = strlen(instring);

    tbuff.min_size(len + 1); // enough for longest possible token
			   // in this input stream.
}

// Note: Only keywords that are part of an expression are recognized.
//       Thus "asm", "if", "case", ... are considered to be identifiers.
//
// MORE: Special handling of "const" and "volitile".
//       Should there a non-ascii preference (or language choice).

#define KWTABSZ 23
static struct kw {
	int   kw_val;	// integer value of keyword from %token decl in yacc
	int   kw_val2;	// secondary code returned as "value" of keyword
	const char *kw_str;	// string representing keyword
	int   kw_ansiC;	// true iff an ansi-C keyword.
} yykey[KWTABSZ] = {    // List MUST be alphabetical by kw_str - see iskey().
	SCTYPE,		S_AUTO,		"auto",		1,
	FTYPE,		S_CHAR,		"char",		1,
	AGGR,		S_CLASS,	"class",	0,
	SCTYPE,		S_CONST,	"const",	1,
	DELETE,		DELETE,		"delete",	0,
	FTYPE,		S_DOUBLE,	"double",	1,
	ENUM,		ENUM,		"enum",		1,
	SCTYPE,		S_EXTERN,	"extern",	1,
	FTYPE,		S_FLOAT,	"float",	1,
	FTYPE,		S_INT,		"int",		1,
	FTYPE,		S_LONG,		"long",		1,
	NEW,		NEW,		"new",		0,
	OPERATOR,	OPERATOR,	"operator",	0,
	SCTYPE,		S_REG,		"register",	1,
	FTYPE,		S_SHORT,	"short",	1,
	FTYPE,		S_SIGNED,	"signed",	1,
	SIZEOF,		SIZEOF,		"sizeof",	1,
	SCTYPE,		S_STATIC,	"static",	1,
	AGGR,		S_STRUCT,	"struct",	1,
	AGGR,		S_UNION,	"union",	1,
	FTYPE,		S_UNSIGNED,	"unsigned",	1,
	FTYPE,		S_VOID,		"void",		1,
	SCTYPE,		S_VOLATILE,	"volatile",	1,
};

static struct kw *
iskey(const char *s)
{
register int lo   = 0;
register int hi   = KWTABSZ - 1;

    while (lo <= hi) {        // do binary search of keyword table.

	register int mid = (lo + hi)/2;
	register int cmp = strcmp(s, yykey[mid].kw_str);

	if (cmp < 0) {
	    hi = mid - 1;
	} else if (cmp > 0) {
	    lo = mid + 1;
	} else {
	    if (language == CPLUS || yykey[mid].kw_ansiC) {
		return &yykey[mid];	// found it.
	    } else {
		return 0;		// not an (ansi) C keyword.
	    }
	}
    }
    return 0; // not found.
}

static char *
senter(const char *s, int len = -1)
{
    if (len < 0) len = strlen(s);

    register char *p = new char[len + 1];

    memcpy(p, s, len);  // string constants may have embedded null bytes.
    p[len] = '\0';      // null terminate strings - a useful invariant.
    return p;
}

static void
line_comment() // process a '//' comment
{
    register int c = input.cget();

    while (1) {
	switch (c) {
	case EOF:
	    printe(ERR_internal, E_ERROR, "line_comment", __LINE__);
		       // A line feed is appended in the Expr ctor.
		       // This should not be possible.
	case '\n':
	    // '\n' will become next char in tlex()
	    return;
	}
	c = input.cget();
    }
}

static void
block_comment() // process a '/* ... */' comment
{
    register int c = input.cget();

    while (1) {
	switch( c ) {
	case EOF:
	    printe(ERR_comment_form, E_ERROR);
	    return;
	case '*':
	    if ((c = input.cget()) =='/') return;
	    break;
	case '/':
	    if ((c = input.cget())== '*')
	    printe(ERR_comment_form, E_WARNING);
	    break;
	default:
	    c = input.cget();
	}
    }
}

int
CCtlex(int& start_pos)
{
    int  extended;
    tbuff.reset();

nexttok:	// loop for whitespace and comments

    start_pos = input.index();

    register int lxchar = input.cget();

    switch( lxchar ) {
    case '@': return Q(AT_SIGN, AT_SIGN);
    case '(': return Q( LP, LP );
    case ')': return Q( RP, RP );
    case '{': return Q( LC, LC );
    case '}': return Q( RC, RC );
    case '[': return Q( LB, LB );
    case ']': return Q( RB, RB );
    case '~': return Q( COMPL, N_TILDE );
    case ',': return Q( CM, N_COM );
    case ';': return Q( SM, 0 );
    case EOF: return Q(EOFTOK, EOFTOK);
    case '?':
	// ANSI trigraph sequences are handled by the preprocessor,
	// if at all...
	return Q(QUEST,QUEST);
    case '^':
	if ((lxchar = input.cget()) == '=')
	    return Q(ASSIGNOP,N_ASXOR);
	else {
	    input.pushback(lxchar);
	    return Q( ER, N_XOR );
	}
    case '*':
	if ((lxchar = input.cget()) == '=')
	    return Q(ASSIGNOP,N_ASMUL);
	else {
	    input.pushback(lxchar);
	    return Q( MUL, N_MUL );
	}
    case '%':
    case '$':
	char prefix_char = lxchar;
	lxchar = input.cget();
	if (prefix_char == '%' && lxchar == '=')
	    return Q(ASSIGNOP,N_ASMOD);
	else if (isalpha(lxchar)) {  // assume a variable name
	    // This forces the user to insert a space to get the
	    // debugger to recognize a mod operator.
	    // We could guess mod operator if the name following is
	    // not defined, but the user would get an error message
	    // in terms of a unknown program name rather than an
	    // unknown debug variable name.
	    tbuff.save(prefix_char);
	    do {
		tbuff.save(lxchar);
		lxchar = input.cget();
	    } while (isalnum(lxchar) || lxchar == '_');
	    input.pushback(lxchar);
	    tbuff.mark_end();

	    char *s = senter(tbuff.ptr());
	    if (prefix_char == '%')
		if (regref(s) == REG_UNK)
     		    return Q( DEBUG_ID, s );
		else
		    return Q( REG_ID, s);
	    else
		return Q( USER_ID, s);
	}
	else if (prefix_char == '%') {
	    input.pushback(lxchar);
	    return Q( DIVOP, N_MOD );
	}
	else {
	    printe(ERR_debug_var_form, E_ERROR);
	    return Q( ILLEGAL, ILLEGAL );
	}
    case '.':
	if ((lxchar = input.cget()) == '.') {
	    if ((lxchar = input.cget()) != '.') {
		    printe(ERR_ellipsis, E_WARNING);
		    input.pushback(lxchar);
	    }
	    return Q(ELLIPSIS,ELLIPSIS);
	}
	else if (isdigit(lxchar)) // floating constant
	    goto dotfloat_const;
	else if (lxchar == '*')
	    return Q(REFMUL,N_DOTREF);
	else {
	    input.pushback(lxchar);
	    return Q(DOT,N_DOT);
	}
    case '"': {
	//MORE: deal with strings with embedded null bytes.
	//      this will require changes in struct Etree
	//      and struct yystype.

	input.getstr('"', tbuff);
	char *s = senter(tbuff.ptr(), tbuff.size());
	return Q( STRING, s );
    }
    case '\'': {
	//MORE: ?? multi-byte chars.
	//MORE:? international char set??
	input.getstr('\'', tbuff);
	char *s = senter(tbuff.ptr(), tbuff.size());
	CCtretval.c.init(CK_CHAR, s); // N_CCON
	return CONSTANT;
	}
    case '/':
	switch (lxchar = input.cget()) {
	case '*':
	    block_comment();
	    goto nexttok;	// loop to get next token
	case '/':
	    line_comment();
	    input.pushback('\n');
	    goto nexttok;	// loop to get next token
	case '=':
	    return Q(ASSIGNOP,N_ASDIV);
	default:
	    input.pushback(lxchar);
	    return Q(DIVOP,N_DIV);
	}
    case '\n':
    case '\t':
    case '\b':
    case '\f':
    case '\r':
    case 11:	// this should be '\v', but cfront 1.2 doesn't handle it
    case ' ':
	goto nexttok;
    case '=':
	if ((lxchar = input.cget()) == '=') {
	    return Q(EQUOP,N_EQ);
	} else {
	    input.pushback(lxchar);
	    return Q(ASSIGN,N_AS);
	}
    case ':':
	if ((lxchar = input.cget()) == ':') {
	    return Q(MEM,MEM);
	} else {
	    input.pushback(lxchar);
	    return Q(COLON,0);
	}
    case '!':
	if ((lxchar = input.cget()) == '=') {
	    return Q(EQUOP,N_NE);
	} else {
	    input.pushback(lxchar);
	    return Q(NOT,N_NOT);
	}
    case '>':
	switch(lxchar = input.cget()) {
	case '>':
	    if ((lxchar = input.cget()) == '=')
		return Q(ASSIGNOP,N_ASRS);
	    else {
		input.pushback(lxchar);
		return Q(SHIFTOP,N_RS);
	    }
	case '=':
	    return Q(RELOP,N_GE);
	default:
	    input.pushback(lxchar);
	    return Q(RELOP,N_GT);
	}
    case '<':
	switch (lxchar = input.cget()) {
	case '<':
	    if ((lxchar = input.cget()) == '=')
		return Q(ASSIGNOP,N_ASLS);
	    else {
		input.pushback(lxchar);
		return Q(SHIFTOP,N_LS);
	    }
	case '=':
	    return Q(RELOP,N_LE);
	default:
	    input.pushback(lxchar);
	    return Q(RELOP,N_LT);
	}
    case '&':
	switch (lxchar = input.cget()) {
	case '&':
	    return Q(ANDAND,N_ANDAND);
	case '=':
	    return Q(ASSIGNOP,N_ASAND);
	default:
	    input.pushback(lxchar);
	    return Q(AND,N_AND);
	}
    case '|':
	switch (lxchar = input.cget()) {
	case '|':
	    return Q(OROR,N_OROR);
	case '=':
	    return Q(ASSIGNOP,N_ASOR);
	default:
	    input.pushback(lxchar);
	    return Q(OR,N_OR);
	}
    case '+':
	switch (lxchar = input.cget()) {
	case '+':
	    return Q(ICOP,N_PREPLPL);
	case '=':
	    return Q(ASSIGNOP,N_ASPLUS);
	default:
	    input.pushback(lxchar);
	    return Q(PLUS,N_PLUS);
	}
    case '-':
	switch (lxchar = input.cget()) {
	case '-':
	    return Q(ICOP,N_PREMIMI);
	case '=':
	    return Q(ASSIGNOP,N_ASMINUS);
	case '>':
	    if ((lxchar = input.cget()) == '*')
		return Q(REFMUL,N_REFREF);
	    else {
		input.pushback(lxchar);
		return Q(REF,N_REF);
	    }
	default:
	    input.pushback(lxchar);
	    return Q(MINUS,N_MINUS);
	}
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': 
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': 
    case 'y': case 'z': 
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': 
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': 
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': 
    case 'Y': case 'Z': 
    case '_':  {
	// Collect id or keyword.
	do {
	    tbuff.save(lxchar);
	    lxchar = input.cget();
	} while (isalnum(lxchar) || lxchar == '_');
	input.pushback(lxchar);
	tbuff.mark_end();

	struct kw *kwp;
	
	if ((kwp = iskey(tbuff.ptr())) != 0) {
	    return Q( kwp->kw_val, kwp->kw_val2 ); // Keyword.
	} else {
	    char *s = senter(tbuff.ptr());
	    return Q( ID, s );
	}
    }

    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9': 
    {
	if( lxchar=='0' ) {	// hex, octal, or floating
	    tbuff.save('0');
	    lxchar = input.cget();
	    if (lxchar == 'x' || lxchar == 'X' ) { // hex
		do {
		    tbuff.save(lxchar);
		    lxchar = input.cget();
		} while (isxdigit(lxchar));
		goto int_end;
	    }

	    int notoct = 0;
	    while (isdigit(lxchar)) {
		if ((lxchar == '8') || (lxchar == '9') ) 
			notoct = 1;
		tbuff.save(lxchar);
		lxchar = input.cget();
	    }
	    if (lxchar=='.')                
		goto prefloat_const;
	    if (lxchar=='e' || lxchar=='E') 
		goto exponent;
	    if (notoct) {
		printe(ERR_octal_const, E_WARNING);
	    }
	    goto int_end;
	} /* end if 0 */

	do {	// collect decimal int or float prefix
	    tbuff.save(lxchar);
	    lxchar = input.cget();
	} while (isdigit(lxchar));
	if( lxchar=='.' ) 
		goto prefloat_const;
	if( lxchar=='e' || lxchar=='E' ) 
		goto exponent;
	// decimal int, get suffix (if any)
int_end:
	int is_unsigned;
	if (tolower(lxchar) == 'u' || tolower(lxchar) == 'l') {
	    int tmp = tolower(lxchar);
	    is_unsigned = (tmp == 'u');
	    tbuff.save(lxchar);
	    lxchar = input.cget();
	    if (tolower(lxchar) == 'u' || tolower(lxchar) == 'l') {
		if (tmp == tolower(lxchar)) {
		    input.pushback(lxchar);
		}
		else {	// both long and unsigned
		    is_unsigned = 1;
		    tbuff.save( lxchar );
		}
	    } else {
		input.pushback(lxchar);
	    }
	} else {
	    is_unsigned = 0;
	    input.pushback(lxchar);
	}
	tbuff.mark_end();
	if (is_unsigned)
		CCtretval.c.init(CK_UINT, tbuff.ptr()); // N_ICON
	else
		CCtretval.c.init(CK_INT, tbuff.ptr()); // N_ICON
	return CONSTANT;

dotfloat_const:	// ".123" -- see case '.' above
	// create constant "0.123"
	tbuff.save('0');
	goto float_const;
prefloat_const:
	lxchar = input.cget();
float_const:
	tbuff.save('.');
	while( isdigit( lxchar ) ) {
	    tbuff.save(lxchar);
	    lxchar = input.cget();
	}
exponent:
	extended = 0;
	if( lxchar=='e' || lxchar=='E' ) 
	{
		tbuff.save(lxchar);
		lxchar = input.cget();
		if (lxchar == '-' || lxchar == '+') {
		    tbuff.save(lxchar);
		    lxchar = input.cget();
		}
		if (! isdigit(lxchar)) {
		    tbuff.save('0');	// fix it up
		    printe(ERR_float_const, E_WARNING);
		}
		while (isdigit(lxchar)) {
		    tbuff.save(lxchar);
		    lxchar = input.cget();
		}
	}
	if( tolower( lxchar ) == 'f') {
	    tbuff.save(lxchar);
	} else if (tolower( lxchar ) == 'l' ) {
		extended = 1;
	} else {
	    input.pushback(lxchar);
	}
	tbuff.mark_end();
	if (extended)
		CCtretval.c.init(CK_XFLOAT, tbuff.ptr()); // N_FCON
	else
		CCtretval.c.init(CK_FLOAT, tbuff.ptr()); // N_FCON
	return CONSTANT;
    }	// end number part
    default:
    // Illegal token.
	{
	char buf[5];
	if( isgraph( lxchar ) )

	    sprintf(buf, "%c", lxchar);
	else
	    sprintf(buf, "0%o", lxchar);
	printe(ERR_illegal_char, E_ERROR, buf);
	return Q( ILLEGAL, ILLEGAL );
	}
    } // end switch and loop
}

void
CClex_position()
{
    input.show(CCpos);
}
