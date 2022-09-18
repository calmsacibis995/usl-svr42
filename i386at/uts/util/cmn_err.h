/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_CMN_ERR_H	/* wrapper symbol for kernel use */
#define _UTIL_CMN_ERR_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:util/cmn_err.h	1.6"
#ident	"$Header: $"

/* common error handling severity levels */

#define CE_CONT		0	/* continue */
#define CE_NOTE		1	/* notice */
#define CE_WARN		2	/* warning */
#define CE_PANIC	3	/* panic */

/* codes for where output should go */

#define	PRW_BUF		0x01	/* output to putbuf */
#define	PRW_CONS	0x02	/* output to console */

/* variables */

extern char *panicstr;		/* panic string pointer */
extern char putbuf[];		/* putchar circular buf, defined in kernel.cf */
extern int putbufsz;		/* size of putbuf, defined in kernel.cf */
extern int putbufrpos;		/* index of next byte to read in putbuf */
extern int putbufwpos;		/* index of next byte to write in putbuf */

/* variable argument list macros */

/*
**	These macros are machine/CCS specific.  These definitions
**	should match the definitions the CCS folk made in stdarg.h,
**	a user header file.  In some cases, however, extra work (code)
**	may need to be done for a kernel to use varargs.
*/

#define VA_LIST _VOID *

#define VA_START(list, name) list = \
  (_VOID*)((char*)&name+((sizeof(name)+(sizeof(int)-1))&~(sizeof(int)-1)))

#define VA_ARG(list, mode) ((mode *) \
  (list=(_VOID*)((char*)list+sizeof(mode))))[-1]

#define VA_END(list)	(void)0

/*
 * Store output character in NVRAM, for machines having the capability.
 * If not supported, define NVRAMPUTCHAR as a null macro.  It is used
 * in cmn_err.c to avoid making that code unnecessarily machine-dependent.
 */
#define NVRAMPUTCHAR(c)		/* not supported in this implementation */

#if defined(__STDC__)

/*PRINTFLIKE2*/
/* handles messages based on specified severity level  */
extern void cmn_err(int, char *, ...);	

/*PRINTFLIKE1*/
/* scaled-down version of C library function, used for kernel diagnostics  */
extern void printf(char *, ...);

/*PRINTFLIKE1*/
/* panic on system unresolvable fatal errors */
extern void panic(char *, ...);		

/* handles system call failures due to insufficient memory */
extern void nomemmsg(char *, int, int, int);	

#else

extern void cmn_err();
extern void printf();
extern void nomemmsg();
extern void panic();

#endif	/* __STDC__ */

#endif	/* _UTIL_CMN_ERR_H */
