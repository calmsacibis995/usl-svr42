/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Signature_h
#define _Signature_h
#ident	"@(#)debugger:inc/common/Signature.h	1.2"

/* The signatures represent the types of data in each message passed
 * between the debugger and the GUI, so a signature of SIG_str_word
 * means the message consists of a string (char *), followed by an
 * integral value (probably an unsigned long).  Those are the only
 * two types supported; floating pt. values are formatted and passed
 * in strings
 * 
 * This file is used directly by Msgtab.[Ch], and several others, but
 * is also one of the inputs to Msg.awk, an awk script which creates
 * the functions, and their prototypes, that send and receive messages
 * 
 * NOTE - when you add a new signature, you may also have to edit
 * libint/common/Mformat.C - make sure that it has enough variable names
 * to cover all the cases
 * ALSO - this file is included by a C file, so should not have
 * C++isms
 */

enum Signature
{
	SIG_invalid,
	SIG_none,	/* special case, no arguments */
	SIG_str,
	SIG_str_str,
	SIG_str_str_str,
	SIG_str_str_str_str,
	SIG_str_str_str_str_str,
	SIG_str_str_str_word_str,
	SIG_str_str_str_word_str_str_str,
	SIG_str_word,
	SIG_word,
	SIG_word_str,
	SIG_word_str_str,
	SIG_word_str_str_str,
	SIG_word_str_str_str_str,
	SIG_word_str_str_str_str_str,
	SIG_word_str_str_str_word_str_str,
	SIG_word_str_str_str_word_str_str_str,
	SIG_word_str_str_word_str_str,
	SIG_word_str_str_word_str_str_str,
	SIG_word_word,
	SIG_word_word_str,
	SIG_word_word_str_word,
	SIG_word_word_word_str_str,
	SIG_last,
};

#ifndef _Word_
#define _Word_
typedef unsigned long Word;
#endif

#endif /* _Signature_h */
