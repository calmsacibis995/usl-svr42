/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/xque/xque.c	1.6"
#ident	"$Header: $"

#include <io/event/event.h>
#include <io/xque/xque.h>
#include <mem/as.h>
#include <mem/immu.h>
#include <mem/seg_objs.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

xqInfo *xqhead;

extern int segobjs_create();
int xquemap();
void xq_settime();
int xq_enqueue();
int  scoaddevent();

/* This routine is used to set up an event queue.
   It allocates space for the queue, maps it to a user address,
   and initializes the structure.
   Xq_init returns the user virtual address or NULL if there was an error. */

caddr_t
xq_init(qinfo, qsize, signo, errorp)
xqInfo *qinfo;
int qsize;
int signo;
int *errorp;
{
	xqEventQueue *q;
	register int npages;
	int oldpri;
	addr_t vaddr;
	struct segobjs_crargs dev_a;
	struct as *as;

	if ((as = u.u_procp->p_as) == (struct as *)NULL)
		cmn_err(CE_PANIC,"xque: no as allocated\n");

	/* Compute size for requested # of elements; round up to page boundary */
	npages = btopr(sizeof(xqEventQueue) + (qsize - 1) * sizeof(xqEvent));

	/* Allocate the pages */
	if ((q = (xqEventQueue *)kseg(npages)) == NULL) {
		*errorp = ENOMEM;
		return NULL;
	}

	/* Compute the actual # of elements */
	q->xq_size = (ptob(npages) - sizeof(xqEventQueue)) / sizeof(xqEvent) + 1;
	qinfo->xq_psize = q->xq_size;
	qinfo->xq_npages = npages;

	qinfo->xq_proc = u.u_procp;
	qinfo->xq_pid = u.u_procp->p_pidp->pid_id;
	qinfo->xq_signo = signo;
	qinfo->xq_ptail = q->xq_tail = q->xq_head = 0;
	qinfo->xq_queue = q;
	qinfo->xq_qtype  = XQUE;
	qinfo->xq_addevent  = xq_enqueue;
	qinfo->xq_devices   = QUE_MOUSE|QUE_KEYBOARD;
	qinfo->xq_xlate     = 0;
	qinfo->xq_qaddr = (char *) q;
	q->xq_sigenable = 0;

	map_addr(&vaddr, ptob(npages), 0, 0);
	if (vaddr == NULL) {
		unkseg(q);
		*errorp = EFAULT;
		return NULL;
	}

	dev_a.mapfunc = xquemap;
	dev_a.arg = (caddr_t) qinfo;
	dev_a.offset = 0;
	dev_a.prot = PROT_READ|PROT_WRITE|PROT_USER;
	dev_a.maxprot = PROT_READ|PROT_WRITE|PROT_USER;

	*errorp = as_map(as, vaddr, ptob(npages), segobjs_create, (caddr_t)&dev_a);
	if (*errorp) {
		qinfo->xq_queue = NULL;
		unkseg(q);
		return NULL;
	}

	qinfo->xq_uaddr = vaddr;

	oldpri = splhi();
	if (xqhead == (xqInfo *)NULL) /* first vt to enable queue mode */
		qinfo->xq_next = (xqInfo *)NULL;
	else {
		qinfo->xq_next = xqhead;
		qinfo->xq_next->xq_prev = qinfo;
	}

	xqhead = qinfo;
	qinfo->xq_prev = (xqInfo *)NULL;
	xq_settime(0);
	splx(oldpri);

	return vaddr;
}

/* This routine is used to clean up when done with an event queue.
   It undoes the user mapping, frees the space, and clears the pointer. */

xq_close(qinfo)
register xqInfo *qinfo;
{
	register caddr_t qaddr = (caddr_t)qinfo->xq_queue;
	if(qinfo->xq_qtype == XQUE) {
		if (xqhead == qinfo) { /* delete first vt in list */
			xqhead = qinfo->xq_next;
			if (xqhead != (xqInfo *)NULL)
				xqhead->xq_prev = (xqInfo *)NULL;
		} else { /* delete vt elsewhere in list */
			qinfo->xq_prev->xq_next = qinfo->xq_next;
			if (qinfo->xq_next != (xqInfo *)NULL) /* last in list */
				qinfo->xq_next->xq_prev = qinfo->xq_prev;
		}
		if (u.u_procp->p_pidp->pid_id == qinfo->xq_pid) {
			as_unmap(u.u_procp->p_as,qinfo->xq_uaddr,ptob(qinfo->xq_npages));
		}
		unkseg(qaddr);
	}
	qinfo->xq_queue = NULL;
	qinfo->xq_qtype = 0;
	qinfo->xq_proc = (struct proc *)NULL;
	qinfo->xq_pid = 0;
}

/* This utility routine labels an event with a unique timestamp. */

static clock_t
get_timestamp()
{
	return (hrestime.tv_sec * 1000) + (hrestime.tv_nsec / 1000000);
}

/* This routine is used to add an event to a queue.
   It will set the xq_time field of the event; all other fields
   should already have been set.
   No check is made to see if the queue is full. */

int
xq_enqueue(qinfo, ev)
register xqInfo *qinfo;
xqEvent *ev;
{
	register xqEventQueue *q = qinfo->xq_queue;
	clock_t get_timestamp();

	ev->xq_time = get_timestamp();
	q->xq_curtime = ev->xq_time;

	q->xq_events[qinfo->xq_ptail] = *ev;
	if (q->xq_sigenable && qinfo->xq_ptail == q->xq_head)
		psignal(qinfo->xq_proc, qinfo->xq_signo);
	q->xq_tail = qinfo->xq_ptail = (qinfo->xq_ptail + 1) % qinfo->xq_psize;
	return 1;
}

/*
 * Called every 30 seconds (approximately) that there is at least one vt 
 * in queue mode.  Calculate the current time since 1/1/70 GMT in milliseconds,
 * and run down the list of vts updating the xq_curtime field of all vts in
 * queue mode with this value.
 */

void
xq_settime(timo)
int timo;
{
	register clock_t milliseconds;
	register xqInfo *tqinfo;
	static timeout_pending = 0;
	clock_t get_timestamp();

	milliseconds = get_timestamp();
	tqinfo = xqhead;
	while (tqinfo != (xqInfo *)NULL) {
		tqinfo->xq_queue->xq_curtime = milliseconds;
		tqinfo = tqinfo->xq_next;
	}
	if (xqhead != (xqInfo *)NULL && (timo || !timeout_pending)) {
		timeout_pending = 1;
		timeout(xq_settime, (caddr_t) 1, drv_usectohz(30*1000000));
	} else
		timeout_pending = 0;
}

int
xquemap(qinfo, off, prot)
xqInfo *qinfo;
register off_t off;
int prot;
{
	int	pf;
	caddr_t addr;

	if (btop(off) >= qinfo->xq_npages)
		return -1;

	pf=(int)vtop((long)off+(char *)qinfo->xq_qaddr, 0);
	return btoct(pf);
}

/* This routine is used to set up an event queue.
   It allocates space for the queue, maps it to a user address,
   and initializes the structure.
   Xq_init returns the user virtual address or NULL if there was an error. */

addr_t
allocate_scoq(evchanp)
struct evchan	*evchanp;
{
	QUEUE *q;
	register int npages;
	int oldpri;
	addr_t vaddr;
	struct segobjs_crargs dev_a;
	struct as *as;
	xqInfo  *qinfo = &evchanp->eq_xqinfo;

	if ((as = u.u_procp->p_as) == (struct as *)NULL)
		cmn_err(CE_PANIC,"xque: no as allocated\n");

	npages = btopr(sizeof(QUEUE));

	/* Allocate the pages */
	if ((q = (QUEUE *)kseg(npages)) == NULL) {
		u.u_error = ENOMEM;
		return NULL;
	}

	qinfo->xq_psize = QSIZE;
	qinfo->xq_npages = npages;

	qinfo->xq_proc = u.u_procp;
	qinfo->xq_pid = u.u_procp->p_pidp->pid_id;
	qinfo->xq_signo = 0;
	qinfo->xq_ptail = q->tail = q->head = 0;
	
	qinfo->xq_qtype = SCOQ;
	qinfo->xq_addevent = scoaddevent;
	qinfo->xq_devices  = 0;
	qinfo->xq_xlate    = 1;
	qinfo->xq_qaddr = (char *) q;

	map_addr(&vaddr, ptob(npages), 0, 0);
	if (vaddr == NULL) {
		unkseg(q);
		u.u_error = EFAULT;
		return NULL;
	}

	dev_a.mapfunc = xquemap;
	dev_a.arg = (caddr_t) qinfo;
	dev_a.offset = 0;
	dev_a.prot = PROT_READ|PROT_WRITE|PROT_USER;
	dev_a.maxprot = PROT_READ|PROT_WRITE|PROT_USER;

	u.u_error = as_map(as, vaddr, ptob(npages), segobjs_create, (caddr_t)&dev_a);
	if (u.u_error ) {
		u.u_error = EFAULT;
		qinfo->xq_qaddr = NULL;
		unkseg(q);
		return NULL;
	}

	qinfo->xq_uaddr = vaddr;
#ifdef DEBUG
	cmn_err(CE_NOTE, "user address for SCO Q is %x", qinfo->xq_uaddr);
#endif
	return vaddr;
}


/* This routine is used to clean up when done with an event queue.
   It undoes the user mapping, frees the space, and clears the pointer. */

int
scoq_close(qinfo)
register xqInfo *qinfo;
{
	register caddr_t qaddr = (caddr_t)qinfo->xq_qaddr;
#ifdef DEBUG
cmn_err(CE_NOTE, "calling scoq_close");	
#endif

	as_unmap(u.u_procp->p_as,qinfo->xq_uaddr,ptob(qinfo->xq_npages));
	qinfo->xq_qaddr = NULL;
	qinfo->xq_qaddr = NULL;
	qinfo->xq_proc = (struct proc *)NULL;
	qinfo->xq_pid = 0;
	unkseg(qaddr);
}


int
scoaddevent(qinfo, ev)
register xqInfo *qinfo;
xqEvent *ev;
{
        register QUEUE *q = (QUEUE *) qinfo->xq_qaddr;
        clock_t get_timestamp();
	EVENT	scoev;
	struct  evchan *evchanp = (struct  evchan *) qinfo->xq_private;
	void event_wakeup();

#ifdef DEBUG
cmn_err(CE_NOTE, "calling scoaddevent %x ACTIVE? %s",
		evchanp, evchanp->eq_state != EVCH_ACTIVE ? "NO" : "YES");
#endif

	if(evchanp->eq_state != EVCH_ACTIVE){
		if( ev->xq_type == XQ_KEY)
			return 0;	
		return 1;
	}

	EV_TIME(scoev) = get_timestamp();
	EV_DX(scoev)  = ev->xq_x;
	EV_DY(scoev)  = -ev->xq_y;
	EV_BUFCNT(scoev) = 1;

       	switch(ev->xq_type)
	{
		case 	XQ_BUTTON: 
			{
				char	buttons = 0;

				if((ev->xq_code&01) == 0)
					buttons |= RT_BUTTON;
				if((ev->xq_code&02) == 0)
					buttons |= MD_BUTTON;
				if((ev->xq_code&04) == 0)
					buttons |= LT_BUTTON;
				if(buttons == qinfo->xq_buttons)
					return 1;
				EV_TAG(scoev) = T_BUTTON;
				EV_BUTTONS(scoev) = buttons;
				qinfo->xq_buttons = buttons;
			}
			break;

		case 	XQ_MOTION: 
			EV_TAG(scoev) = T_REL_LOCATOR;
			break;

		case 	XQ_KEY: 
			EV_TAG(scoev) = T_STRING;
			EV_BUF(scoev)[0] = ev->xq_code;
			break;
		default :
			cmn_err(CE_NOTE, "EVENT of unknown type %d",
				       	ev->xq_type);
			return 0;
	}

	if((EV_TAG(scoev)&evchanp->eq_emask) == 0)
	{
#ifdef DEBUG
		cmn_err(CE_NOTE, "EVENT of type %d is rejected", EV_TAG(scoev));
#endif
		return 0;
	}
	if (((q->head + 1) % QSIZE) == q->tail)
		q->tail = ((q->tail + 1) % QSIZE);
	q->queue[q->head] = scoev;
	q->head = (q->head + 1) % QSIZE;

	event_wakeup(evchanp);
	return 1;
}
