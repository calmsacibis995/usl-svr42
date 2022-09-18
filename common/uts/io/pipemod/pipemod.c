/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/pipemod/pipemod.c	1.2.2.3"
#ident	"$Header: $"

/*
 * This module switches the read and write flush bits for each
 * M_FLUSH control message it receives. Its intended usage is to
 * properly flush a STREAMS-based pipe.
 */

#include <io/conf.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

STATIC int pipeopen(), pipeclose(), pipeput();

STATIC struct module_info pipe_info = {
	1003,
	"pipe",
	0,
	INFPSZ,
	STRHIGH,
	STRLOW };
STATIC struct qinit piperinit = { 
	pipeput,
	NULL,
	pipeopen,
	pipeclose,
	NULL,
	&pipe_info,
	NULL };
STATIC struct qinit pipewinit = { 
	pipeput,
	NULL,
	NULL,
	NULL,
	NULL,
	&pipe_info,
	NULL};
struct streamtab pipeinfo = { &piperinit, &pipewinit };

int pipedevflag = D_OLD;

/*ARGSUSED*/
STATIC int
pipeopen(rqp, dev, flag, sflag)
	queue_t *rqp;
	dev_t dev;
	int flag;
	int sflag;
{
	return 0;
}

/*ARGSUSED*/
STATIC int
pipeclose(q)
	queue_t *q;
{
	return 0;
}

/*
 * Use same put procedure for write and read queues.
 * If mp is an M_FLUSH message, switch the FLUSHW to FLUSHR and
 * the FLUSHR to FLUSHW and send the message on.  If mp is not an
 * M_FLUSH message, send it on with out processing.
 */
STATIC int
pipeput(qp, mp)
	queue_t *qp;
	mblk_t *mp;
{
	switch(mp->b_datap->db_type) {
		case M_FLUSH:
			if (!(*mp->b_rptr & FLUSHR && *mp->b_rptr & FLUSHW)) {
				if (*mp->b_rptr & FLUSHW) {
					*mp->b_rptr |= FLUSHR;
					*mp->b_rptr &= ~FLUSHW;
				} else {
					*mp->b_rptr |= FLUSHW;
					*mp->b_rptr &= ~FLUSHR;
				}
			}
			break;

		default:
			break;
	}
	putnext(qp,mp);
	return 0;
}
