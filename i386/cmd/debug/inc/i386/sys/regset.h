/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_REGSET_H
#define _SYS_REGSET_H

#ident	"@(#)debugger:inc/i386/sys/regset.h	1.1"

/* private copy to get around cfront bug */

/* General register access (386) */

typedef int greg_t;

#define NGREG   19

typedef	greg_t	gregset_t[NGREG];

/*
 * The following picks up the defines for '386 registers.
 *
 */

#include <sys/reg.h>

typedef struct fpregset {
    union {
        struct /* fp extension state */
        {
            int state[27];  /* 287/387 saved state */
            int status;     /* status word saved at exception */
        } fpchip_state;
        struct /* for emulator(s) */
        {
            char    fp_emul[246];
            char    fp_epad[2];
        } fp_emul_space;
        int f_fpregs[62];       /* union of the above */
    } fp_reg_set;
    long    f_wregs[33];            /* saved weitek state */
} fpregset_t;

#define NDEBUGREG	8

typedef struct dbregset {
	unsigned	debugreg[NDEBUGREG];
} dbregset_t;


#endif	/* _SYS_REGSET_H */
