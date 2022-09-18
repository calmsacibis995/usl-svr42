/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KDB_KDB_DEBUGGER_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_KDB_DEBUGGER_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/kdb/kdb/debugger.h	1.6"
#ident	"$Header: $"

#define DBSTKSIZ    32
#define VARTBLSIZ   64
#define LINBUFSIZ   161
#define MAXSTRSIZ   128
#define STRSPCSIZ   512

#define EOF         -1
#define EOL	    -2

typedef unsigned char   uchar;

extern short    dbgetchar();
extern void	dbunget();
extern char *	dbpeek();
extern short    dbgetitem();
extern char     *dbstrdup();
extern char     *dbstralloc();
extern void     dbstrfree();
extern int      dbextname();
extern int      dbstackcheck();
extern int      dbtypecheck();
extern void     dberror();
extern void     dbprintitem();
extern void	dbtellsymname();

extern char     dbverbose;
extern ushort   dbibase;
extern ushort   dbtos;
extern struct item dbstack[];

struct item {
    union {
	uchar byte[4];
	ushort word[2];
	ulong number;
	char *string;
    } value;
    unsigned type : 3;
};

/* item types */

/* #define EOF         -1 */
/* #define NULL         0 */

#define NUMBER          1
#define STRING          2
#define NAME            3

#define TYPEMAX         3

struct variable {
    char *name;
    struct item item;
    unsigned type;
};

/* Variable types */
#define VAR_VAR		1	/* normal variable */
#define VAR_MACRO	2	/* command macro */


/* Breakpoint info */
#define MAX_BRKNUM	20	/* Highest logical breakpoint number */
#define MAX_UBRKNUM	19	/* Highest user-visible breakpoint number */
#define MAX_HBRKNUM	3	/* Highest hardware breakpoint number */
#define TMP_BRKNUM	(MAX_UBRKNUM + 1)

struct brkinfo {
	unsigned char	state;		/* breakpoint state */
	unsigned char	type;		/* breakpoint type */
	unsigned char	saved_opc;	/* saved opcode byte */
	unsigned short	tcount;		/* trace count */
	unsigned long	addr;		/* breakpoint address */
	char		*cmds;		/* commands to execute, if any */
};

/* breakpoint types (these must match hardware bit values) */
#define BRK_INST	0
#define BRK_MODIFY	1
#define BRK_IO		2		/* supported on P5 only */
#define BRK_ACCESS	3

/* breakpoint states */
#define BRK_CLEAR	0
#define BRK_ENABLED	1
#define BRK_DISABLED	2
#define BRK_LOADED	3

/* register types */
#define REG_TYPE	0x3000
#define REG_E		0x0000	/* full 32-bit register */
#define REG_X		0x1000	/* low 16 bits of register */
#define REG_H		0x2000	/* next-to-lowest 8 bits of register */
#define REG_L		0x3000	/* low 8 bits of register */


#define LCASEBIT    0x20
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')
#define issym(c)   (((c) >= 'A' && (c) <= 'Z') || \
			((c) >= 'a' && (c) <= 'z') || \
			((c) >= '0' && (c) <= '9') || \
			(c) == '_' || (c) == '.' || (c) == '%' || (c) == '?')

#endif /* _UTIL_KDB_KDB_DEBUGGER_H */
