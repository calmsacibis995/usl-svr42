/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libexp/common/lex_util.h	1.2"

#ifndef LEX_UTIL_H
#define LEX_UTIL_H

#include "Value.h"

#define EOF (-1)

// -- Token collection buffer operations.


// -- Input stream manipulation operations.

class Lex_input {
	char	*_start;
	char	*_nextch;
	int	_saved;
public:
		Lex_input() { _start = _nextch = 0; _saved = 0; }
	void	init(char *start) { _start  = _nextch = start; _saved  = '\n'; }
	int	index() { return _nextch - _start;
    }
	int	cget();
	void	pushback(int c);
	void	show(int);
	void	getstr(register int openquote, Buff&);
};

#endif /*LEX_UTIL_H*/
