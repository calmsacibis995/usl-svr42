/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:mem/mem.cf/Space.c	1.9"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <vm/page.h>
#include <sys/tuneable.h>
#include <sys/map.h>
#include <sys/param.h>

struct tune	tune = {
		GPGSLO,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		FDFLUSHR,
		MINARMEM,
		MINASMEM,
		MAXDMAPAGE,
		FLCKREC,
		0,
		0
};

int minpagefree = MINPAGEFREE;
int maxpgio = MAXPGIO;

u_int pages_pp_maximum = PAGES_UNLOCK;

/* Overflow lists for pageio_setup() from sched and pageout. */
struct buf pgoutbuf[PGOVERFLOW];
struct buf notpgoutbuf[NOTPGOVERFLOW];
int npgoutbuf = PGOVERFLOW;
int nnotpgoutbuf = NOTPGOVERFLOW;

int piosegsz = PIOSEGSZ;		/* map sizes in pages for pio   */
int syssegsz = SYSSEGSZ;		/*	sysseg			*/
int segmapsz = SEGMAPSZ;		/*	segmap			*/

struct map	sptmap[SPTMAP];
struct map	piomap[PIOMAP];
int	piomapsz = PIOMAP;
int	piomaxsz = PIOMAXSZ;

/*
 * Initialize paging tuneables here. We will define DESFREEMULT
 * and LOTSFREEMULT as constants here rather than in a header file
 * since it will allow changes to the relative values of paging
 * tunables without requiring rebuilding from source and also
 * without confusing the users/administrators with more tunables
 * that are unlikely to need adjustment in most cases.
 *
 * Minfree is minimal amount of free memory which is tolerable.
 * This is used in SUNOS and in SVR4 to start swapping.
 * tune.t_gpgslo comes from 3.2, it was the point where swapping
 * started, since the two values represent the same concept,
 * set them equal.
 */

#define DESFREEMULT	2
#define LOTSFREEMULT	3

int	minfree = GPGSLO;
int	desfreemult = DESFREEMULT;
int	lotsfreemult = LOTSFREEMULT;

