/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/sp/sp.cf/Space.c	1.4"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>
#include <sys/stream.h>

/*
 * WARNING: Original code had this defined in sp.c. It is still defined there.
 * We define it here so we can declare sp_sp.
 */
struct sp {
	queue_t *sp_rdq;		/* this stream's read queue */
	queue_t *sp_ordq;		/* other stream's read queue */
};

struct sp	sp_sp[SP_UNITS];
int		spcnt = SP_UNITS;
