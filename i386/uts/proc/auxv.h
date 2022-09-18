/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * This file defines values used in passing information to
 * the dynamic linker during exec.
 */

#ifndef _PROC_AUXV_H	/* wrapper symbol for kernel use */
#define _PROC_AUXV_H	/* subject to change without notice */
#ident	"@(#)uts-x86:proc/auxv.h	1.4"
#ident	"$Header: $"

typedef struct
{
       int     a_type;
       union {
               long    a_val;
#ifdef __STDC__
               void    *a_ptr;
#else
               char    *a_ptr;
#endif
               void    (*a_fcn)();
       } a_un;
} auxv_t;

/*
 * The information is passed on the stack. The following
 * values indicate the type of data that immediately follows
 * on the stack.
 */
#define AT_NULL		0
#define AT_IGNORE	1	/* ignore the next word */
#define AT_EXECFD	2	/* file descriptor for the executable */
#define AT_PHDR		3	/* address of the program header table */
#define AT_PHENT	4	/* size of each program header table entry */
#define AT_PHNUM	5	/* number of program header table entries */
#define AT_PAGESZ	6	/* getpagesize(2) */
#define AT_BASE		7	/* ld.so base addr */
#define AT_FLAGS	8	/* processor flags */
#define AT_ENTRY	9	/* a.out entry point */

#endif	/* _PROC_AUXV_H */
