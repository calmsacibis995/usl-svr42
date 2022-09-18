/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:cfront/src/size.h	1.1"

/*************************************************************************

size.h:
	includes ../incl/$(MACH)/mach_size.h

***************************************************************************/

#ifndef GRAM
extern BI_IN_WORD;
extern BI_IN_BYTE;
				/*	byte sizes */
extern SZ_CHAR;
extern AL_CHAR;

extern SZ_SHORT;
extern AL_SHORT;

extern SZ_INT;
extern AL_INT;

extern SZ_LONG;
extern AL_LONG;

extern SZ_FLOAT;
extern AL_FLOAT;

extern SZ_DOUBLE;
extern AL_DOUBLE;

extern SZ_STRUCT;	/* minimum struct size */
extern AL_STRUCT;

extern SZ_WORD;

extern SZ_WPTR;
extern AL_WPTR;

extern SZ_BPTR;
extern AL_BPTR;	

extern char* LARGEST_INT;
extern int F_SENSITIVE;	/* is field alignment sensitive to the type of the field? */
extern int F_OPTIMIZED;	/* can the compiler fit a small int field into a char? */
#endif

#define KTBLSIZE	123	/*	initial keyword table size */
#define GTBLSIZE	257	/*	initial global name table size */
#define CTBLSIZE	12	/*	initial class table size */
#define TBLSIZE		20	/*	initial block table size */
#define BLMAX		50	/*	max block nesting */
#define TBUFSZ		48*1024	/*	(lex) input buffer size */
#define MAXFILE		255	/*	max include file nesting */
#define MAXERR		13	/* maximum number of errors before terminating */

#ifndef GRAM
#define CHUNK	8*1024
char* chunk();
#endif

/*
 *	default sizes:
 *
 *	machine dependent size, refer to ../incl/$MACH to know what exact
 *	sizes are. Porting cfront should create macine specific directory
 *	under ../incl except for they are identical to default sizes.
 */

#if u3b | u3b2 | u3b5 | m32
/* AT&T 3Bs */
#include "m32/mach_size.h"

#elif i386
/* Intel 80386 chip */
#include "i386/mach_size.h"

#elif i860
/* Intel 80860 chip */
#include "i860/mach_size.h"

#elif uts
/* Amdahl running UTS */
#include "uts/mach_size.h"

#elif m88k
/* Motorola 88100 */
#include "m88k/mach_size.h"

#elif sparc
/* sun sparc */
#include "sparc/mach_size.h"

#elif sun
#include "sun/mach_size.h"

#elif mc68k
/* most M68K boxes */
#include "mc68k/mach_size.h"

#elif apollo 
#include "apollo/mach_size.h"

#elif iAPX286 && LARGE
/* Intel 80286 large model */
#include "iapx286/mach_size.h"

#elif vax
/* VAX (running V8) */
#include "vax/mach_size.h"

#else
/* defaults: 0 => error */
#include "default/mach_size.h"

#endif
