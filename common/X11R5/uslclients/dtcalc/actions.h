/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcalc:actions.h	1.1"
/*
 * $XConsortium: actions.h,v 1.5 91/01/10 11:51:05 rws Exp $
 * 
 * actions.h - action table declaring externally available procedures for xcalc
 *
 * Copyright 1989 by the Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Donna Converse, MIT X Consortium
 */

extern void
    add(), back(), bell(), clearit(), cosine(), decimal(),
    degree(), digit(), divide(), e(), enter(), epower(), equal(),
    exchange(), factorial(), 
    inverse(), leftParen(), logarithm(), multiply(), naturalLog(),
    negate(), nop(), off(), pi(), power(), quit(), recall(),
    reciprocal(), rightParen(), roll(), scientific(), selection(), sine(),
    square(), squareRoot(), store(), subtract(), sum(),
    tangent(), tenpower(), XexchangeY();

/*
 * 	calculator action table
 */

XtActionsRec	Actions[] = {
{"add",		add},		/* addition */
{"back",	back},		/* HP-specific backspace */
{"bell",	bell},		/* ring bell */
{"clear",	clearit},	/* TI-specific clear calculator state */
{"cosine",	cosine},	/* trigonometric function cosine */
{"decimal",	decimal},	/* decimal point */
{"degree",	degree},	/* degree, radian, grad switch */
{"digit",	digit},		/* numeric key */
{"divide",	divide},	/* division */
{"e",		e},		/* the natural number e */
{"enter",	enter},		/* HP-specific enter */
{"epower",	epower},	/* e raised to a power */
{"equal",	equal},		/* TI-specific = */
{"exchange",	exchange},	/* TI-specific exchange memory and display */
{"factorial",	factorial},	/* factorial function */
{"inverse", 	inverse},	/* inverse */
{"leftParen",	leftParen},	/* TI-specific left parenthesis */
{"logarithm",	logarithm},	/* logarithm base 10 */
{"multiply",	multiply},	/* multiplication */
{"naturalLog",	naturalLog},	/* natural logarithm base e */
{"negate",	negate},	/* change sign */
{"nop",		nop},		/* no operation, rings bell */
{"off",		off},		/* clear state */
{"pi",		pi},		/* the number pi */
{"power",	power},		/* raise to an arbitrary power */
{"quit",	quit},		/* quit */
{"recall",	recall},	/* memory recall */
{"reciprocal",  reciprocal},	/* reciprocal function */
{"rightParen",	rightParen},	/* TI-specific left parenthesis */
{"roll",	roll},		/* HP-specific roll stack */
{"scientific",	scientific},	/* scientfic notation (EE) */
{"selection",	selection},	/* copy selection */
{"sine",	sine},		/* trigonometric function sine */
{"square",	square},	/* square */
{"squareRoot",	squareRoot},	/* square root */
{"store",	store},		/* memory store */
{"subtract", 	subtract},	/* subtraction */
{"sum",		sum},		/* memory summation */
{"tangent",	tangent},	/* trigonometric function tangent */
{"tenpower",	tenpower},	/* 10 raised to to an arbitrary power */
{"XexchangeY",	XexchangeY}	/* HP-specific exchange X and Y registers */
};
