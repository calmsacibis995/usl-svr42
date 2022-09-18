/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_REGSET_H	/* wrapper symbol for kernel use */
#define _PROC_REGSET_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/regset.h	1.4"
#ident	"$Header: $"

/* General register access (386) */

#ifdef _KERNEL_HEADERS

#ifndef _PROC_REG_H
#include <proc/reg.h>	/* COMPATIBILITY */
#endif

#elif defined(_KERNEL)

#include <sys/reg.h>	/* COMPATIBILITY */

#else

#include <sys/reg.h>	/* COMPATIBILITY */

#endif /* _KERNEL_HEADERS */


typedef	int	greg_t;
#define	NGREG	19
typedef	greg_t	gregset_t[NGREG];

/*
 * The following picks up the defines for 386 registers.
 *
 */

typedef struct fpregset {
    union {
	struct fpchip_state	/* fp extension state */
	{
            int state[27];  /* 287/387 saved state */
            int status;	    /* status word saved at exception */
	} fpchip_state;
	struct fp_emul_space	    /* for emulator(s) */
	{
            char    fp_emul[246];
            char    fp_epad[2];
	} fp_emul_space;
	int f_fpregs[62];	/* union of the above */
    } fp_reg_set;
    long    f_wregs[33];	    /* saved weitek state */
} fpregset_t;

#define NDEBUGREG	8

typedef struct dbregset {
	unsigned	debugreg[NDEBUGREG];
} dbregset_t;

#endif	/* _PROC_REGSET_H */
