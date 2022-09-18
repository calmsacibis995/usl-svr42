/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Language_h
#define Language_h
#ident	"@(#)debugger:inc/common/Language.h	1.2"

enum Language {
    UnSpec,	// unspecified, use current default
    C,		// debug C exprs.
    CPLUS,	// debug C++ subset.
};

class LWP;

Language	current_language(LWP *lwp = 0);
		// This routine arbitrates the value between %lang and %db_lang
const char	*language_name(Language);
int	 	set_language(const char *);
		// This routine sets the %lang value.
Language	current_context_language(LWP *lwp = 0);
		// This routine gets the %db_lang value.
Language	current_user_language();
		// This routine gets the %lang value.

#endif /*Language_h*/
