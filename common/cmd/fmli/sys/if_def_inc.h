/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fmli:sys/if_def_inc.h	1.2"

#define	MAX_IF_DEPTH	256	/* maximum number of nested if's */

/*
 * Possible "if" states
 */
#define	IN_A_CONDITION	1	/* we are evaluating if */
#define	IN_A_THEN	2	/* we are in the then portion */
#define	IN_AN_ELSE	4	/* we are in the else portion */
#define	IN_AN_ELIF_SKIP	8	/* we are in the elif (ignore conditional) */
#define	IF_IS_TRUE	16	/* the condition is true */

/*
 * Miscellaneous macros to set/test various "if/then/else" states
 */
#define ANY_IF_STATE 		(IN_A_CONDITION | \
				 IN_A_THEN | \
				 IN_AN_ELSE | \
				 IN_AN_ELIF_SKIP)
