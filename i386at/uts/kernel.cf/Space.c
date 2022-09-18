/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:kernel.cf/Space.c	1.6"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/var.h>


/*
 * Tables and initializations
 */

extern	int	oeminit(),
		cinit(),
		pinit(),
		vfsinit(),
		strinit(),
		aioinit();
extern void	binit();
extern void	finit();

int (*init_tbl[])() = {
		oeminit,
	  	cinit,
	  	(int (*) ()) binit,
		pinit,
	  	vfsinit,
	  	(int (*) ()) finit,
#ifndef NOSTREAMS
	  	strinit,
#endif
		aioinit,
	  	0,
};


#ifndef NUMSXT
#define NUMSXT	0
#endif
#ifndef XSDSEGS
#define XSDSEGS	0
#endif
#ifndef XSDSLOTS
#define XSDSLOTS 0
#endif

struct var	v = {
		NBUF,
		NCALL,
		NPROC,
		DNLCSIZE,
		0,
		MAXCLSYSPRI,
		NCLIST,
		MAXUP,
		NHBUF,
		NHBUF-1,
		NPBUF,
		SPTMAP,
		MAXPMEM,
		NAUTOUP,
		BUFHWM,
		NSCRN,
		NEMAP,
		NUMSXT,
		XSDSEGS,
		XSDSLOTS
};


/*
 * Line Discipline Switch Table (clist compatibility)
 * order: open close read write ioctl rxint txint modemint
 *
 * NOTE: This has to stay in kernel/space.c, since some old add-on
 *	 packages will look for it there.
 */
extern int 	ttopen(),
		ttclose(),
		ttread(),
		ttwrite(),
		ttioctl(),
		ttin(),
		ttout(),
		nulldev();

extern	int	xtin(),
		xtout(),
		sxtin(),
		sxtout();

struct	linesw	linesw[] = {
/* 	tty ------------- */
			 ttopen,
			 ttclose,
			 ttread,
			 ttwrite,
			 ttioctl,
			 ttin,
			 ttout,
			 nulldev,
/*              xt ------------- */
			 nulldev,
			 nulldev,
			 nulldev,
			 nulldev,
			 nulldev,
			 xtin,
			 xtout,
			 nulldev,
/*		sxt -------------*/
			 nulldev,
			 nulldev,
			 nulldev,
			 nulldev,
			 nulldev,
			 sxtin,
			 sxtout,
			 nulldev,
 };
int     linecnt = {3};

/* This has to be in kernel/space.c since it's referenced by add-ons */
int	eua_lim_ma = 2;


#include <sys/sockmod.h>
#include <sys/osocket.h>

struct odomain *osoc_family = 0; /* SCO socket emulation protocol translation structure */
