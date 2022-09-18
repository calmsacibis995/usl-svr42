/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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
#ident	"@(#)debugger:libexp/common/CCtokdefs.h	1.3"

// WARNING: must be kept in sync with %token definitions in CCgram.Y

#ifndef CCTOKDEFS_H
#define CCTOKDEFS_H

#define EOFTOK		 0
#define DELETE		 1
#define ENUM		 2
#define NEW		 3
#define OPERATOR	 4
#define SIZEOF		 5
#define THIS		 6
#define LP		 7
#define RP		 8
#define LB		 9
#define RB		10
#define REF		11
#define DOT		12
#define NOT		13
#define COMPL		14
#define MUL		15
#define REFMUL		16
#define AND		17
#define PLUS		18
#define MINUS		19
#define ER		20
#define OR		21
#define ANDAND		22
#define OROR		23
#define QUEST		24
#define COLON		25
#define ASSIGN		26
#define ASSIGNOP	27
#define CM		28
#define SM		29
#define LC		30
#define RC		31
#define ID		32
#define REG_ID		33
#define DEBUG_ID	52
#define USER_ID		53
#define STRING		34
#define CONSTANT	35
#define RELOP		36
#define EQUOP		37
#define DIVOP		38
#define SHIFTOP		39
#define ICOP		40
#define AGGR		41
#define FTYPE		42
#define SCTYPE		43
#define ILLEGAL		44
#define EMPTY		45
#define LOW		46
#define FDEF		47
#define DECL_MARKER	48
#define ELLIPSIS	49
#define MEM		50
#define TSCOPE		51
#define AT_SIGN		54
#define FN_SUFFIX	55

// Secondary token codes, returned as "value" of token
enum {
    S_CHAR,
    S_CONST,
    S_DOUBLE,
    S_EXTERN,
    S_FLOAT,
    S_INT,
    S_LONG,
    S_REG,
    S_SHORT,
    S_SIGNED,
    S_STATIC,
    S_UNSIGNED,
    S_VOID,
    S_VOLATILE,
    S_CLASS,
    S_STRUCT,
    S_UNION,
    S_ENUM,
    S_AUTO,
};

#endif
