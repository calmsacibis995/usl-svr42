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
#ident	"@(#)debugger:libexp/common/CCops.h	1.3"

// Operators for C and C++

// The caller must define the macro "OP_ENTRY(enum, string, val)"
// and include this file each time the values are required.
//  Yes, this is a dirty trick - but is makes it easier to
//  keep the strings and enum names in sync.

OP_ENTRY( N_ADDR,	"ADDR",		"&",	 	10 )
OP_ENTRY( N_AGGR,	"AGGR",		"AGGR",		15 )
OP_ENTRY( N_AND,	"AND",		"&",		20 )
OP_ENTRY( N_ANDAND,	"ANDAND",	"&&",	 	30 )
OP_ENTRY( N_AS,		"AS",		"=",	 	40 )
OP_ENTRY( N_ASAND,	"ASAND",	"&=", 		50 )
OP_ENTRY( N_ASDIV,	"ASDIV",	"/=", 		60 )
OP_ENTRY( N_ASLS,	"ASLS",		"<<=",	 	70 )
OP_ENTRY( N_ASMINUS,	"ASMINUS",	"-=", 		80 )
OP_ENTRY( N_ASMOD,	"ASMOD",	"%=", 		90 )
OP_ENTRY( N_ASMUL,	"ASMUL",	"*=",		100 )
OP_ENTRY( N_ASOR,	"ASOR",		"|=",		110 )
OP_ENTRY( N_ASPLUS,	"ASPLUS",	"+=",		120 )
OP_ENTRY( N_ASRS,	"ASRS",		">>=",		130 )
OP_ENTRY( N_ASXOR,	"ASXOR",	"^=",		140 )
OP_ENTRY( N_AT,		"AT_SIGN",	"@",		145 )
OP_ENTRY( N_CALL,	"CALL",		"()",		150 )	/* func  call */
OP_ENTRY( N_CAST,	"CAST",		"cast",         160 )
OP_ENTRY( N_CCON,	"CCON",		"CCON",		170 )
OP_ENTRY( N_COM,	"COM",		",",		180 )	/* comma opr */
OP_ENTRY( N_DELETE,	"DELETE",	"delete",	190 )
OP_ENTRY( N_DEREF,	"DEREF",	"*", 		200 )	/* * */
OP_ENTRY( N_DIV,	"DIV",		"/",	 	210 )
OP_ENTRY( N_DOT,	"DOT",		".",		220 )
OP_ENTRY( N_DOTREF,	"DOTREF",	".*",		230 )
OP_ENTRY( N_DT_ARY,	"DT_ARY",	"DT_ARY",	231 )
OP_ENTRY( N_DT_PTR,	"DT_PTR",	"DT_PTR",	232 )
OP_ENTRY( N_DT_FCN,	"DT_FCN",	"DT_FCN",	233 )
OP_ENTRY( N_DT_MPTR,	"DT_MPTR",	"DT_MPTR",	234 )
OP_ENTRY( N_DT_REF,	"DT_REF",	"DT_REF",	235 )
OP_ENTRY( N_ELLIPSIS,	"ELLIPSIS",	"...",		236 )
OP_ENTRY( N_ENUM,	"ENUM",		"enum",		239 )/*union,struct,class */
OP_ENTRY( N_EQ,		"EQ",		"==",		240 )
OP_ENTRY( N_FCON,	"FCON",		"FCON",		250 )
OP_ENTRY( N_GE,		"GE",		">=",		260 )
OP_ENTRY( N_GT,		"GT",		">",		270 )
OP_ENTRY( N_ICON,	"ICON",		"ICON",		280 )
OP_ENTRY( N_ID,		"ID",		"ID",		290 )
OP_ENTRY( N_INDEX,	"INDEX",	"[]",		300 )	/* [] */
OP_ENTRY( N_LE,		"LE",		"<=",		310 )
OP_ENTRY( N_LS,		"LS",		"<<",		320 )
OP_ENTRY( N_LT,		"LT",		"<",		330 )
OP_ENTRY( N_MEM,	"MEM",		"::",		340 )
OP_ENTRY( N_MINUS,	"MINUS",	"-",		360 )
OP_ENTRY( N_MOD,	"MOD",		"%",	 	370 )
OP_ENTRY( N_MUL,	"MUL",		"*",	 	380 )
OP_ENTRY( N_NE,		"NE",		"!=",		390 )
OP_ENTRY( N_NEW,	"NEW",		"new",		400 )
OP_ENTRY( N_NOT,	"NOT",		"!",		410 )
OP_ENTRY( N_NTYPE,	"NTYPE",	"NTYPE",	415 )
OP_ENTRY( N_OPERATOR,	"OPERATOR",	"operator",	420 )
OP_ENTRY( N_OR,		"OR",		"|",		430 )
OP_ENTRY( N_OROR,	"OROR",		"||",		440 )
OP_ENTRY( N_PLUS,	"PLUS",		"+",	 	450 )
OP_ENTRY( N_POSTMIMI,	"POSTMINI",	"--",		460 )
OP_ENTRY( N_POSTPLPL,	"POSTPLPL",	"++",		470 )
OP_ENTRY( N_PREMIMI,	"PREMIMI",	"--",		480 )
OP_ENTRY( N_PREPLPL,	"PREPLPL",	"++",		490 )
OP_ENTRY( N_QUEST,	"QUEST",	"?:",		510 )
OP_ENTRY( N_REF,	"REF",		"->",		520 )
OP_ENTRY( N_REFREF,	"REFREF",	"->*",		530 )
OP_ENTRY( N_REG_ID,	"REG_ID",	"REG_ID",	535 )
OP_ENTRY( N_DEBUG_ID,	"DEBUG_ID",	"DEBUG_ID",	537 )
OP_ENTRY( N_USER_ID,	"USER_ID",	"USER_ID",	539 )
OP_ENTRY( N_RS,		"RS",		">>",		540 )
OP_ENTRY( N_SIZEOF,	"SIZEOF",	"sizeof",	550 )
OP_ENTRY( N_STRING,	"STRING",	"STRING",	560 )
OP_ENTRY( N_TILDE,	"TILDE",	"~",		580 )
OP_ENTRY( N_TYPE,	"TYPE",		"TYPE",		585 )
OP_ENTRY( N_UMINUS,	"UMINUS",	"-",		590 )
OP_ENTRY( N_UPLUS,	"UPLUS",	"+",		600 )
OP_ENTRY( N_XOR,	"XOR",		"^",		610 )
OP_ENTRY( N_ZCONS,	"ZCONS",	"ZCONS",	620 )/* list connector*/
