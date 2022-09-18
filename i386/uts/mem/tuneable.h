/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_TUNEABLE_H	/* wrapper symbol for kernel use */
#define _MEM_TUNEABLE_H	/* subject to change without notice */

#ident	"@(#)uts-x86:mem/tuneable.h	1.5"
#ident	"$Header: $"

/*
 * A collection of tuneable parameters that
 * (for historical reasons) are somewhat unrelated.
 * This structure should be eliminated in favor of
 * separate variables; binary driver compatibility
 * constraints have prevented us from doing so to date.
 */

typedef struct tune {
	int	t_gpgslo;	/* If freemem < t_gpgslow, then start	*/
				/* to steal pages from processes.	*/
	int	t_pad[7];	/* Padding for driver compatibility.	*/
	int	t_fsflushr;	/* The rate at which fsflush is run in	*/
				/* seconds.				*/
	int	t_minarmem;	/* The minimum available resident (not	*/
				/* swappable) memory to maintain in 	*/
				/* order to avoid deadlock.  In pages.	*/
	int	t_minasmem;	/* The minimum available swappable	*/
				/* memory to maintain in order to avoid	*/
				/* deadlock.  In pages.			*/
	int	t_dmalimit;	/* Last (exclusive) DMAable page number	*/
	int	t_flckrec;	/* Max number of active frlocks.	*/
	int	t_dmabase;	/* First (inclusive) DMAable page num	*/
	int	t_devnondma;	/* Non-zero => some device memory is	*/
				/* non-DMAable				*/
} tune_t;

extern tune_t tune;

#endif	/* _MEM_TUNEABLE_H */
