/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:io/iaf/iaf.c	1.4.2.3"
#ident	"$Header: $"
/*
 *	STREAMS module to hold IAF attribute-value-assertions
 */
#include <io/iaf/iaf.h>
#include <io/conf.h>
#include <io/stream.h>
#include <util/param.h>
#include <util/types.h>

#ifdef DEBUG
#define PRINT_DEBUG(a)	printf a
#else
#define PRINT_DEBUG(a)
#endif

static struct module_info rminfo = { 0x6e7f, "iaf", 0, INFPSZ, 0, 0 };
static struct module_info wminfo = { 0x6e7f, "iaf", 0, INFPSZ, 0, 0 };

static int iafopen(), iafclose(), iafrput(), iafwput();

static struct qinit rinit = {
	iafrput, NULL, iafopen, iafclose, NULL, &rminfo, NULL
};

static struct qinit winit = {
	iafwput, NULL, NULL, NULL, NULL, &wminfo, NULL
};

struct streamtab iafinfo = { &rinit, &winit, NULL, NULL };

int iafdevflag = D_OLD;

/*ARGSUSED1*/
static int
iafopen(q, dev, flag, sflag)
queue_t *q;
dev_t    dev;
int      flag;
int      sflag;
{
	mblk_t *bp;	   /* pointer to block of private storage */
	struct iaf *iafp;  /* pointer to data within the block    */

	/*	Return success if already opened...	*/

	if (q->q_ptr != NULL) {
      		return (0);
	}

	/*	Prepare M_DATA block for IOCACK message	*/
	/*	for later return.			*/

     	if ((bp = allocb(sizeof(struct iaf), BPRI_MED)) == NULL) {
		PRINT_DEBUG(( "allocb failed!!!\n"));
      		return (OPENFAIL);
      	} else {
		bp->b_wptr = bp->b_rptr + sizeof(struct iaf);
		iafp = (struct iaf *)bp->b_rptr;
		iafp->count = 0;
		iafp->size = 0;
		iafp->data[0] = '\0';
	}

	/*
	 *	Associate the block with the read and write
	 *	queues for this module so other procedures can
	 *	get to it...
	 */

	q->q_ptr = WR(q)->q_ptr = (caddr_t)bp;

	return(0);
}

static int
iafwput(q, mp)
queue_t *q;
mblk_t  *mp;
{
	mblk_t     *privatep;	/* pointer to private data for queue */
	struct iaf *iafp;	/* pointer to data within the block    */

	/*	Only process these special ioctl's...	*/

	if (mp->b_datap->db_type == M_IOCTL) {

		struct iocblk *iocblkp;

		iocblkp = (struct iocblk *)mp->b_rptr;

		switch (iocblkp->ioc_cmd) {

		   case SETAVA:

			if ((q->q_ptr == NULL) || (mp->b_cont == NULL)) {
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
			}

			/* free the old message */

			privatep = (mblk_t *)q->q_ptr;
			freemsg(privatep);
	
			/* save the current one */

			q->q_ptr = RD(q)->q_ptr = (caddr_t) mp->b_cont;
			mp->b_cont = NULL;

			/* set up the IOCACK reply	*/

			mp->b_datap->db_type = M_IOCACK;
			iocblkp = (struct iocblk *)mp->b_rptr;
			iocblkp->ioc_count = 0;
			iocblkp->ioc_rval = 0;
			iocblkp->ioc_error = 0;
			qreply(q, mp);
			return(0);

		   case GETAVA:

			if ((q->q_ptr == NULL) || (mp->b_cont == NULL)) {
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
			}

			/* if user doesn't have enough space, tell them	*/

			{
			int have_size, need_size;

			have_size = ((struct iaf*)mp->b_cont->b_rptr)->size;
			privatep = (mblk_t *)q->q_ptr;
			need_size = ((struct iaf*)privatep->b_rptr)->size;
			privatep = (mblk_t *)q->q_ptr;
			if ( have_size < need_size ) {
                        	mp->b_datap->db_type = M_IOCACK;
                        	iocblkp->ioc_rval = need_size;
				qreply(q, mp);
				return(0);
			}
			}
			
			/* get rid of their data part of message */

			if (mp->b_cont)
				freemsg(mp->b_cont);
			mp->b_cont = NULL;
			iocblkp->ioc_count = 0;

			/* replace it with a dup of the stored message */

			privatep = (mblk_t *)q->q_ptr;
			mp->b_cont = dupmsg(privatep);

			/* set up the IOCACK reply	*/

			mp->b_datap->db_type = M_IOCACK;
			iafp = (struct iaf *)privatep->b_rptr;
			iocblkp->ioc_count = 2 * sizeof(int) + iafp->size * sizeof(char);
			iocblkp->ioc_error = 0;
			iocblkp->ioc_rval = 0;
			qreply(q, mp);
			return(0);

		   default:
			break;
		}
	}
	putnext(q, mp);
	return(0);
}

static int
iafrput(q, mp)
queue_t *q;
mblk_t  *mp;
{
	putnext(q, mp);
	return(0);
}

/*ARGSUSED1*/
static int
iafclose(q, flag)
queue_t *q;
int      flag;
{
	/* Free the private message */

	freemsg(q->q_ptr);
	q->q_ptr = WR(q)->q_ptr = NULL;
	return(0);
}
