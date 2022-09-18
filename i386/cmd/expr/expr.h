/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)expr:i386/cmd/expr/expr.h	1.1"
#ident  "$Header: expr.h 1.1 91/08/13 $"

/*
 * 	Headerfile to expr
 */
# define A_STRING 258
# define NOARG 259
# define OR 260
# define AND 261
# define EQ 262
# define LT 263
# define GT 264
# define GEQ 265
# define LEQ 266
# define NEQ 267
# define ADD 268
# define SUBT 269
# define MULT 270
# define DIV 271
# define REM 272
# define MCH 273
# define MATCH 274
/* Enhanced Application Compatibility */
# define SUBSTR 275
# define LENGTH 276
# define INDEX 277

/* size of subexpression array */
#define MSIZE	512
#define error(c)	errxx()
#define EQL(x,y) !strcmp(x,y)

#define ERROR(c)	errxx()
