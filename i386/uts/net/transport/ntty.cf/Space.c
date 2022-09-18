/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:net/transport/ntty.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/tihdr.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/types.h>
#include <config.h>	/* to collect tunable parameter below */

struct Ntty
{
	tcflag_t cflags;/* copy of cflags to keep setty happy */
	int state;	/* state of ntty device */
	mblk_t *mp;	/* pointer to preallocated message block */
	mblk_t *ioc_mp; /* saved reply for stty 0 - disconnect request */
	tcflag_t brk_flag; /* break flags in iflags */
};

struct Ntty ntty[NT_UNITS];
int    nt_cnt = NT_UNITS;
