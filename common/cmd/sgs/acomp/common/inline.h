/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/inline.h	1.2"
/* inline.h */

#ifndef NO_AMIGO
extern int do_inline;
extern int delete_ok;
extern void inline_endf();
extern void inline_begf();
extern void inline_eof();
extern void inline_address_function();
extern void inline_flags();
#endif
#define ASSIGN_CASES \
	case ASG PLUS: \
	case ASG MINUS: \
	case ASG MUL: \
	case ASG DIV: \
	case ASG MOD: \
	case ASG OR: \
	case ASG AND: \
	case ASG ER: \
	case ASG LS: \
	case ASG RS: \
	case INCR: \
	case DECR: \
	case ASSIGN
