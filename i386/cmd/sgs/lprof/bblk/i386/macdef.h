/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:bblk/i386/macdef.h	1.1"

/* These #defines are used in declaring the coverage array for a function. */

/* #define strings used to declare the coverage array for a function */
#define _MAKELOC	"\t.local\t__coverage.%s\n"
#define _COVDEF		"\t.set\t__coverage.%s,.C%d\n"
#define _TROUBLE	"\t.set\t__trouble,.C%d\n"
#define _SIZE	 	"\t.size\t__coverage.%s,.C%d.len\n"

/* #define strings used to allocate memory space for the coverage array */
#define _DATA		"\t.data\n"                     /* data section */
#define _LABEL		".C%d:\n"                       /* memory label */
#define _LENDEF		"\t.set\t.C%d.len,.-.C%d\n"	/* define struct size */
#define _AWORD		"\t.4byte\t%d\n"		/* putout a number */
#define _ZERO		"\t.zero\t%d\n"                 /* memory allocation */
#define _ALIGN          "\t.align\t4\n"		/* word alignment for labels */
#define _PREVIOUS	"\t.previous\n"		/* restore prior scn */
#define	_SET		"\t.set\t.,.+%d\n"	/* memory allocation in bytes */
#define	_LONG		"\t.long\t0x0\n"	/* byte alignment */
#define _TEXT           "\t.text\n"		/* text section in COFF file */

#define BYTE4		4			/* four byte field */

/* the #define strings for the coverage instructions */
#define _LCOVERAGE      "\tinc\t.C%d+4+%d\n"		/* for lprof */

/* #define strings for saving condition codes */
#define _DEFSCC         ""
#define _SAVECC         "\tpushf\n"              /* save condition codes */
#define _RESTORECC      "\tpopf\n"               /* restore condition codes */

