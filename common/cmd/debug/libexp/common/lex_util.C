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
#ident	"@(#)debugger:libexp/common/lex_util.C	1.4"

#include <ctype.h>
#include "lex_util.h"
#include "Interface.h"
#include "Value.h"

static int
xvalue(int c)
{
    switch( c ) {
    case '0':	return 0;
    case '1':	return 1;
    case '2':	return 2;
    case '3':	return 3;
    case '4':	return 4;
    case '5':	return 5;
    case '6':	return 6;
    case '7':	return 7;
    case '8':	return 8;
    case '9':	return 9;
    case 'a':
    case 'A':	return 10;
    case 'b':
    case 'B':	return 11;
    case 'c':
    case 'C':	return 12;
    case 'd':
    case 'D':	return 13;
    case 'e':
    case 'E':	return 14;
    case 'f':
    case 'F':	return 15;
    }
}

void
Lex_input::getstr(register int c, Buff& tbuff)
{
    // Get the string or char constant, keeping a count of how
    // many characters there are.

    register int in_c = cget();

 while (1) {
	char	ferr[2];
	switch( in_c ) {
	case EOF:
	case '\n':
	no_end:
	    pushback(in_c);
	    	printe(ERR_quote_match, E_WARNING);
	    in_c = c;
	    // fall through...
	case '\'':
	case '"':
	    if (in_c == c) {
		// found matching ' or "
matching:
		return;
	    }
	    // fall through...
	default:
	    tbuff.save(in_c);
	    in_c = cget();
	    break;
	case '\\':
	    switch( in_c = cget() ) {
	    default:
		ferr[0] = in_c;
		ferr[1] = 0;
		printe(ERR_escape, E_WARNING, in_c);
		goto trans;
	    case 'a': in_c = '\a'; goto trans;
	    case 'b': in_c = '\b'; goto trans;
	    case 'f': in_c = '\f'; goto trans;
	    case 'n': in_c = '\n'; goto trans;
	    case 'r': in_c = '\r'; goto trans;
	    case 't': in_c = '\t'; goto trans;
	    case 'v': in_c = '\v'; goto trans;
	    case '\\': in_c = '\\'; goto trans;
	    case '/': in_c = '/'; goto trans;
	    case '\'': in_c = '\''; goto trans;
	    case '"': in_c = '"'; goto trans;
	    case '?': in_c = '?';
trans:
		tbuff.save(in_c);
		in_c = cget();
		break;
	    case EOF:
		goto no_end;
	    case '\n':
		if( c == '\'' ) goto no_end;
		printe(ERR_newline_string, E_WARNING);
		pushback('"');	// pretend it's a "..." "..." type
		goto matching;
	    case 'x': {
		int ch = 'x';	// \x == x
		in_c = cget();
		if (isxdigit(in_c)) {
		    ch = xvalue(in_c);
		    in_c = cget();
		    if (isxdigit(in_c)) {
			ch = ch * 16 + xvalue(in_c);
			in_c = cget();
			if (isxdigit(in_c)) {
			    ch = ch * 16 + xvalue(in_c);
			    in_c = cget();
			}
		    }
		}
		tbuff.save(ch);
		break;
	    }
	    case '0': case '1': case '2':
	    case '3': case '4': case '5':
	    case '6': case '7': {
		int ch = xvalue( in_c );
		in_c = cget();
		if (isdigit(in_c)
		    && in_c != '8' && in_c != '9') {
		    ch = ch * 8 + xvalue( in_c );
		    in_c = cget();
		    if (isdigit(in_c)
			&& in_c != '8' && in_c != '9') {
			ch = ch * 8 + xvalue(in_c);
			in_c = cget();
		    }
		}
		tbuff.save(ch);
		break;
	    }
	    } /* backslash case */
	}
    }
    //NOTREACHED
}

int
Lex_input::cget()
{
register int c = _saved;

    if (c != 0) {
	_saved = 0;
    } else if (*_nextch == '\0') {
	c = EOF;
    } else {
	c = *_nextch++;
    }
    return c;
}

void
Lex_input::pushback(int c)
{
    if (_nextch > _start && c == _nextch[-1]) {  // go back - no prb.

	--_nextch;
    } else {
	_saved = c;	// insert character, limit one.
    }
}

void
Lex_input::show(int index)
{
    /* assume line does not fold .
     * also assume exactly one line feed - at end.
     * Note: Expect linefeed at end of input string (_start).
    */
    char	buf[1024];

    sprintf(buf, "%s%*s^", _start, index, "");
    printe(ERR_syntax_loc, E_NONE, buf);
}
