/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. 468473-010";

#ident	"@(#)uts-x86at:io/dlpi_ether/el16hrdw.c	1.12"
#ident	"$Header: $"

/*
 *  This file contains all of the hardware dependent code for the 3c507.
 *  It is the companion file to ../../io/dlpi_ether.c
 */

#include <io/dlpi_ether/dlpi_el16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/el16.h>
#include <io/strlog.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/tcpip/if.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#define BCOPY(from, to, len)	bcopy((caddr_t)(from), (caddr_t)(to), \
	(size_t)(len))
#define BCMP(s1, s2, len)	bcmp((char *)(s1), (char *)(s2), \
	(size_t)(len))

extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t		DLsaps[];
extern	struct ifstats	*DLifstats;

extern	int		DLstrlog;
extern	char	DLid_string[];
extern	int		DLboards;
extern	char	DLcopyright[];
extern	int		el16intr_to_index[];
extern	int		el16timer_id;
extern	int		el16wait_scb();
extern	ushort	ntohs(), htons();

STATIC	void	el16tx_done(), el16chk_ring();
STATIC	int		el16ru_restart();
extern 	int	el16config_586(), el16init_586();
void	outb();
struct	debug	el16debug = {0, 0, 0, 0, 0};
void	el16bdspecclose(), el16bdspecioctl();
extern	int	bus_p;

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*el16ifp;
STATIC	int		el16bpf_ioctl(), el16bpf_output(), el16bpf_promisc();

extern	void	bpf_tap(), outb();
#endif

/* el16bdspecclose is called from DLclose->dlpi_ether.c */
void
el16bdspecclose(q)
queue_t *q;
{
	return;
} /* end of el16bdspecioctl */

/* el16bdspecioctl is called from DLioctl->dlpi_ether.c */
void
el16bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *ioctl_req = (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
} /* end of el16bdspecioctl */

/*
 *  el16xmit_packet()
 *
 *  This routine is called from DLunitdata_req() and el16tx_done(). It assumes
 *  we are at STREAMS spl interrupt level.
 */

el16xmit_packet (bd, mp)
DL_bdconfig_t *bd;
mblk_t	*mp;
{
	bdd_t *bdd = (bdd_t *) bd->bd_dependent1;

	scb_t	*p_scb;
	tbd_t	*p_tbd;
	cmd_t	*p_cmd;
	char	*p_txb;

	char	*src;
	char	*dest;
	mblk_t	*mp_tmp;

	int		tx_size = 0;
	int		msg_size = 0;
	int		start = 0;

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"el16xmit_packet on board %x",(int)bd));

	/*
	 * If ring buffer is empty
	 */
	if (bdd->head_cmd == NULL) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"el16xmit_packet: ring EMPTY"));
		bdd->head_cmd = bdd->tail_cmd = bdd->ring_buff;
		start = 1;
	}

	/*
	 * else there's at least one command pending in ring. Note that we don't
	 * check whether queue is full or not. When the queue is full, we flag the
	 * interface to TX_BUSY. Here we trust the dlpi_ether.c to put the next
	 * message in a queue.
	 */
	else {
		bdd->tail_cmd = bdd->tail_cmd->next;

		if (bdd->tail_cmd->next == bdd->head_cmd) {
			DL_LOG(strlog(DL_ID,200,1,SL_TRACE,"el16xmit_packet: ring FULL"));
			el16debug.ring_full++;
			bd->flags |= TX_BUSY;
		}
	}

	/*
	 * set up pointers
	 */
	p_cmd = (cmd_t *) (bdd->virt_ram + bdd->tail_cmd->ofst_cmd);
	p_tbd = (tbd_t *) (bdd->virt_ram + p_cmd->prmtr.prm_xmit.xmt_tbd_ofst);
	p_txb = bdd->virt_ram + p_tbd->tbd_buff;

	/*
	 * fill in 82586's transmit command block
	 */
	p_cmd->cmd_status   = 0;
	p_cmd->cmd_cmd      = CS_EL | CS_CMD_XMIT | CS_INT;
	p_cmd->cmd_nxt_ofst = 0xFFFF;

	/*
	 * copy dest. address and sap address to the transmit command structure
	 */
	src  = (char *)(mp->b_rptr);
	dest = (char *)(p_cmd->prmtr.prm_xmit.xmt_dest);
	mybcopy (src, dest, 6);
	p_cmd->prmtr.prm_xmit.xmt_length = *(short *)(mp->b_rptr+12);

	/*
	 * copy data to tx buffer.
	 */
	for (mp_tmp = mp->b_cont; mp_tmp != NULL; mp_tmp = mp_tmp->b_cont) {
		msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
		mybcopy ((caddr_t) mp_tmp->b_rptr, p_txb, msg_size);
		p_txb          += msg_size;
		tx_size        += msg_size;
		/* mp_tmp->b_rptr += msg_size; */
	}

	p_tbd->tbd_count = tx_size | CS_EOF;

#ifdef NBPFILTER
	if (bd->bpf) {
		mblk_t	*mp_tap;

		if ((mp_tap = allocb(tx_size+MAC_HDR_LEN, BPRI_MED)) != NULL) {
			mybcopy ((caddr_t)src,(caddr_t)mp_tap->b_wptr,DL_MAC_ADDR_LEN);
			mp_tap->b_wptr += DL_MAC_ADDR_LEN;
			mybcopy ((caddr_t)bd->eaddr,(caddr_t)mp_tap->b_wptr,DL_MAC_ADDR_LEN);
			mp_tap->b_wptr += DL_MAC_ADDR_LEN;
			* (ushort *) mp_tap->b_wptr = p_cmd->prmtr.prm_xmit.xmt_length;
			mp_tap->b_wptr += sizeof (ushort);

			for (mp_tmp=mp->b_cont; mp_tmp; mp_tmp=mp_tmp->b_cont) {
				msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
				mybcopy((caddr_t)mp_tmp->b_rptr,(caddr_t)mp_tap->b_wptr,msg_size);
				mp_tap->b_wptr += msg_size;
			}

			bpf_tap(bd->bpf, mp_tap->b_rptr, msg_size);
			freemsg(mp_tap);
		}
	}
#endif

	bd->mib.ifOutOctets += tx_size + MAC_HDR_LEN;	/* SNMP */

	/*
	 * if we need to start the 82586, issue a channel attention
	 */
	if (start) {
		p_scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
		if (el16wait_scb (p_scb, 1000)) {
			cmn_err (CE_WARN, "el16xmit_packet: scb command not cleared");
			bd->timer_val = 0;			/* force board to reset */
			freemsg(mp);
			return (-1);
		}
		p_scb->scb_cmd      = SCB_CUC_STRT;
		p_scb->scb_cbl_ofst = bdd->tail_cmd->ofst_cmd;
		if (bus_p & BUS_MCA) {
			outb (bd->io_start + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
			outb (bd->io_start + CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
		}
		else
			outb (bd->io_start + CHAN_ATN_OFST, 1);
	}
	freemsg(mp);
	return (0);
}

/*
 *  el16intr()
 */

void
el16intr (level)
int	level;
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	scb_t		*scb;
	cmd_t		*cmd;
	int			base_io;
	int			index;
	ushort		scb_status;

	/*
	 * map irq level to the proper board. Make sure it's ours.
	 */
	if ((index = el16intr_to_index [level]) == -1) {
		cmn_err (CE_WARN, "%s spurious interrupt", el16id_string);
		return;
	}

	/*
	 * get pointers to structures
	 */
	bd = &el16config [index];
	base_io = bd->io_start;
	bdd = (bdd_t *) bd->bd_dependent1;
	scb = (scb_t *) (bdd->virt_ram + bdd->ofst_scb);

	if (bdd->head_cmd != 0)
		cmd = (cmd_t *) (bdd->virt_ram + bdd->head_cmd->ofst_cmd);
	else
		cmd = (cmd_t *) NULL;


	/*
	 * If scb command field doesn't get cleared, reset the board.
	 */
	if (el16wait_scb (scb, 1000)) {
		cmn_err (CE_WARN, "el16intr: scb command not cleared");
		bd->timer_val = 0;							/* cause board to reset */
		return;
	}
	scb_status = scb->scb_status;
	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"EL16INTR: SCB status: %x;",scb_status));

	/*
	 * acknowledge 82586 interrupt
	 */
	if ((scb->scb_cmd = scb_status & SCB_INT_MSK) != 0)
		if (bus_p & BUS_MCA) {
			outb (base_io + CTRL_REG_OFST, CR_CA|CR_RST|CR_IEN|CR_BS);
			outb (base_io + CTRL_REG_OFST, CR_RST|CR_IEN|CR_BS);
		}
		else
			outb (base_io + CHAN_ATN_OFST, 1);

	if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
		el16chk_ring (bd);

	if (scb_status & (SCB_INT_CX | SCB_INT_CNA))
		el16tx_done (bd);
	else if ((cmd != (cmd_t *) NULL) && (cmd->cmd_status & CS_CMPLT)) {
		el16debug.tx_cmplt_missed++;
		el16tx_done (bd);
	}

	/*
	 *  Store error statistics in the MIB structure.
	 */
	bd->mib.ifSpecific.etherAlignErrors   = scb->scb_aln_err;
	bd->mib.ifSpecific.etherCRCerrors     = scb->scb_crc_err;
	bd->mib.ifSpecific.etherMissedPkts    = scb->scb_rsc_err;
	bd->mib.ifSpecific.etherOverrunErrors = scb->scb_ovrn_err;
	bd->mib.ifInErrors = scb->scb_aln_err + scb->scb_crc_err + scb->scb_rsc_err + scb->scb_ovrn_err;

	if (el16ifstats)
		bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;

	if (!(bus_p & BUS_MCA))
		outb (base_io + CLEAR_INTR, 1);					/* clear interrupt */
}

/*
 *  el16tx_done()
 */

STATIC void
el16tx_done (bd)
DL_bdconfig_t	*bd;
{
	bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
	cmd_t		*cmd;
	scb_t		*scb;
	DL_sap_t	*sap;
	mblk_t		*mp;
	int			x;
	int			next;

	DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"el16tx_done: for board %d", bd->bd_number));
	/*
	 * If ring is empty, we have nothing to process.
	 */
	if (bdd->head_cmd == 0) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"el16tx_done: null head pointer"));
		return;
	}

	do {
		cmd = (cmd_t *) (bdd->virt_ram + bdd->head_cmd->ofst_cmd);

		if (el16ifstats)
			bd->ifstats->ifs_opackets++;

		/*
		 *  Read the tx status register and see if there were any problems.
		 */
		if (!(cmd->cmd_status & CS_OK)) {
			bd->mib.ifOutErrors++;
			if (el16ifstats)
				bd->ifstats->ifs_oerrors++;
		}

		if (cmd->cmd_status & CS_COLLISIONS) {
			bd->mib.ifSpecific.etherCollisions++;
			if (el16ifstats)
				bd->ifstats->ifs_collisions++;
		}

		if (cmd->cmd_status & CS_CARRIER)
			bd->mib.ifSpecific.etherCarrierLost++;

		if (cmd->cmd_cmd & (CS_INT | CS_EL))
			break;
		bdd->head_cmd = bdd->head_cmd->next;
	} while (1);

	/*
	 * if last command in ring buffer
	 */
	if (bdd->head_cmd == bdd->tail_cmd) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"el16tx_done: last command"));
		bdd->head_cmd = bdd->tail_cmd = 0;
		return;
	}

	/*
	 * else there are more commands pending to be sent to the board
	 */
	else {
		ring_t	*next_cmd;
		next_cmd = bdd->head_cmd = bdd->head_cmd->next;
		while (next_cmd != bdd->tail_cmd) {
			cmd = (cmd_t *) (bdd->virt_ram + next_cmd->ofst_cmd);
			cmd->cmd_cmd &= ~(CS_EL | CS_INT);
			cmd->cmd_nxt_ofst = next_cmd->next->ofst_cmd;
			next_cmd = next_cmd->next;
		}

		bd->flags &= ~TX_BUSY;
		scb = (scb_t *) (bdd->virt_ram + bdd->ofst_scb);
		if (el16wait_scb (scb, 1000)) {
			cmn_err (CE_WARN, "el16tx_done: scb command not cleared");
			bd->timer_val = 0;					/* force board to reset */
			return;
		}
		scb->scb_cmd = SCB_CUC_STRT;
		scb->scb_cbl_ofst = bdd->head_cmd->ofst_cmd;
		if (bus_p & BUS_MCA) {
			outb (bd->io_start + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
			outb (bd->io_start + CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
		}
		else
			outb (bd->io_start + CHAN_ATN_OFST, 1);
	}

	/*
	 *  Indicate outstanding transmit is done and see if there is work
	 *  waiting on our queue.
	 */
	if (bd->flags & TX_QUEUED) {
		next = bd->tx_next;
		sap  = &el16saps [next];

		for (x=0; x<bd->max_saps; x++) {
			if (++next == bd->max_saps)
				next = 0;

			if ((sap->state == DL_IDLE) && (mp = getq(sap->write_q))){
				(void) el16xmit_packet(bd, mp);
				bd->tx_next = next;
				bd->mib.ifOutQlen--;
				return;
			}

			if (next == 0)
				sap = el16saps;
			else
				sap++;
		}

		/*
		 *  Nobody's left to service, make the queue empty.
		 */
		el16debug.q_cleared++;
		bd->flags &= ~TX_QUEUED;
		bd->mib.ifOutQlen = 0;
		bd->tx_next = 0;
	}
}

/*
 *  el16chk_ring (bd)
 */

STATIC void
el16chk_ring (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	fd_t	*fd;
	rbd_t	*first_rbd, *last_rbd;
	mblk_t	*mp;
	DL_sap_t *sap;
	int		x, nrbd;
	long	length;

	DL_LOG (strlog (DL_ID, 103, 3, SL_TRACE, "el16chk_ring:"));

	/* for all fds */
	for (fd=bdd->begin_fd; (char*)fd != bdd->virt_ram+0xffff; fd=bdd->begin_fd) {
		if (!(fd->fd_status & CS_CMPLT)) {
			DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "fd status: NOT CS_CMPLT"));
			break;
		}

		if (DLifstats)
			bd->ifstats->ifs_ipackets++;

		length = nrbd = 0;
		first_rbd = last_rbd = (rbd_t *) (bdd->virt_ram + fd->fd_rbd_ofst);

		if (fd->fd_rbd_ofst != 0xffff) {
			/*
			 * find length of data and last rbd holding
			 * the received frame.
			 */
			while ((last_rbd->rbd_status & CS_EOF) != CS_EOF) {
				nrbd++;
				length += last_rbd->rbd_status & CS_RBD_CNT_MSK;
				if ((last_rbd->rbd_size   & CS_EL) != CS_EL) 
					last_rbd = (rbd_t *)(bdd->virt_ram+last_rbd->rbd_nxt_ofst);
			    else {
					cmn_err(CE_WARN, "out of receive buffers\n");
					break;
				}
			}
			length += last_rbd->rbd_status & CS_RBD_CNT_MSK;
			nrbd++;

			if (fd->fd_status & CS_OK) {
				if ((mp = allocb (MAC_HDR_LEN+length, BPRI_MED)) == NULL) {
					DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "no receive resources"));
					bd->mib.ifInDiscards++;				/* SNMP */
					bd->mib.ifSpecific.etherRcvResources++;/* SNMP */

				} else {
					rbd_t 	*temp_rbd;
					int		i, l;

					DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "putq "));

					/* fill in the mac header */
					mybcopy((caddr_t)fd->fd_dest,(caddr_t)mp->b_wptr,MAC_HDR_LEN);
					mp->b_wptr += MAC_HDR_LEN;

					/* fill in the data from rcv buffers */
					for (i=0,temp_rbd=first_rbd; i<nrbd; i++) {
						l = temp_rbd->rbd_status & CS_RBD_CNT_MSK;	
						mybcopy ((caddr_t)bdd->virt_ram+first_rbd->rbd_buff, (caddr_t)mp->b_wptr, l);
						mp->b_wptr += l;
						temp_rbd->rbd_status = 0;
						temp_rbd = (rbd_t *) (bdd->virt_ram+temp_rbd->rbd_nxt_ofst);
					}

					if (!DLrecv(mp, bd->sap_ptr))
						bd->mib.ifInOctets += MAC_HDR_LEN + length;
				}
			} else if (el16ifstats)
				bd->ifstats->ifs_ierrors++;

			/* re-queue rbd */
			bdd->begin_rbd = (rbd_t *) (bdd->virt_ram + last_rbd->rbd_nxt_ofst);

			last_rbd->rbd_status = 0;
			last_rbd->rbd_size   |= CS_EL;
			last_rbd->rbd_nxt_ofst = 0xffff;
			bdd->end_rbd->rbd_nxt_ofst = (char *)first_rbd - bdd->virt_ram;
			bdd->end_rbd->rbd_size &= ~CS_EL;
			bdd->end_rbd = last_rbd;
		}

		/* re-queue fd */
		bdd->begin_fd = (fd_t *) (bdd->virt_ram + fd->fd_nxt_ofst);
		bdd->end_fd->fd_nxt_ofst = (char *)fd - bdd->virt_ram;
		bdd->end_fd->fd_cmd      = 0;
		bdd->end_fd = fd;

		fd->fd_status   = 0;
		fd->fd_cmd      = CS_EL;
		fd->fd_nxt_ofst = 0xffff;
		fd->fd_rbd_ofst = 0xffff;

	}
	(void) el16ru_restart (bd);
}

/*
 * el16ru_restart ()
 */

STATIC int
el16ru_restart (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd    	= (bdd_t *) bd->bd_dependent1;
	scb_t	*scb		= (scb_t *) (bdd->virt_ram + bdd->ofst_scb );
	fd_t	*begin_fd	= bdd->begin_fd;

	/*
	 * RU already running -- leave it alone
	 */
	if ((scb->scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return (1);

	/*
	 * Receive queue is exhausted, need to reset the board
	 */
	if (begin_fd == NULL) {
		bd->timer_val = 0;
		return (0);
	}

	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * el16chk_ring () and thus calls el16ru_restart() which will
	 * then start the RU if necessary.
	 */
	if (begin_fd->fd_status & CS_CMPLT)
		return (1);

	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on page 2-15
	 */
	el16debug.rcv_restart_count++;
	begin_fd->fd_rbd_ofst = (ushort) ((char *)bdd->begin_rbd - bdd->virt_ram);

	if (el16wait_scb (scb, 1000)) {
		cmn_err (CE_WARN, "ru-restart: scb command not cleared");
		bd->timer_val = 0;					/* force board to reset */
		return (0);
	}

	scb->scb_rfa_ofst = (ushort) ((char *)bdd->begin_fd - bdd->virt_ram);
	scb->scb_cmd      = SCB_RUC_STRT;
	if (bus_p & BUS_MCA) {
		outb (bd->io_start + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
		outb (bd->io_start + CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
	}
	else
		outb (bd->io_start + CHAN_ATN_OFST, 1);

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"RU re-started"));
	return (1);
}

/*
 * el16watch_dog (bd)
 */

void
el16watch_dog ()
{
	DL_bdconfig_t	*bd;
	int				i, j;

	/*
	 * reset timeout to 5 seconds
	 */

	el16timer_id = timeout (el16watch_dog, 0, drv_usectohz (EL16_TIMEOUT));

	for (i=0, bd=el16config; i<el16boards; bd++, i++) {
		if (bd->timer_val > 0)
			bd->timer_val--;

		if (bd->timer_val == 0) {
			bd->timer_val = -1;
			cmn_err (CE_NOTE,"%s board %d timed out.",el16id_string,bd->bd_number);

			for (j=0; j<10; j++)
				if (el16init_586 (bd) == 0)
					return;

			cmn_err (CE_WARN,"%s board %d will not reset.",el16id_string,bd->bd_number);
		}
	}
}

/*
 * el16promisc_on (bd)
 */

el16promisc_on (bd)
DL_bdconfig_t	*bd;
{
	int		old;

	/*
	 *  If already in promiscuous mode, just return.
	 */
	if (bd->promisc_cnt++)
		return (0);

	old = splstr();

	if (el16config_586 (bd, PRO_ON))
		return (1);

	splx (old);
	return (0);
}

/*
 * el16promisc_off (bd)
 */

el16promisc_off (bd)
DL_bdconfig_t	*bd;
{
	int		old;

	/*
	 *  If the board is not in a promiscuous mode, just return;
	 */
	if (!bd->promisc_cnt)
		return (0);

	/*
	 *  If this is not the last promiscuous SAP, just return;
	 */
	if (--bd->promisc_cnt)
		return (0);

	/*
	 *  Save the current page then go to page two and read the current
	 *  receive configuration.
	 */
	old = splstr();

	if (el16config_586 (bd, PRO_OFF))
		return (1);

	splx (old);
	return (0);
}

#ifdef ALLOW_SET_EADDR
/******************************************************************************
 *  el16set_eaddr()
 */
el16set_eaddr (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	return (1);	/* not supported yet */
}
#endif

/******************************************************************************
 *  el16add_multicast()
 */
el16add_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
register int i;
int rval,oldlevel;


        oldlevel = splstr();
        if ( (bd->multicast_cnt >= MULTI_ADDR_CNT) || (!maddr->bytes[0] & 0x1)){                splx(oldlevel);
                return 1;
        }
        if ( el16is_multicast(bd,maddr)) {
                splx(oldlevel);
                return 0;
        }
        for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
                if (!mcp->status)
                        break;
        }
        mcp->status = 1;
        bd->multicast_cnt++;
        bcopy((caddr_t)maddr->bytes,(caddr_t)mcp->entry,DL_MAC_ADDR_LEN);
        rval = el16set_multicast(bd);
        splx(oldlevel);
        return rval;
}

/******************************************************************************
 *  el16set_multicast()
 */
el16set_multicast(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	mcad_t	mcad;
	mcat_t	*mcp = &(bdd->el16_multiaddr[0]);
	scb_t	*scb = (scb_t *) (bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *) (bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io = bd->io_start;
	int		i, j, k;
	
	if (el16wait_scb (scb, 1000))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd      = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;				/* ofst is general cmd */
	
	cmd = (cmd_t *) (bdd->virt_ram + bdd->gen_cmd);
	cmd->cmd_status   = 0;
	cmd->cmd_cmd      = CS_CMD_MCSET | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	for (i = 0, j = 0; i < MULTI_ADDR_CNT; i++, mcp++)
		if (mcp->status) {
			bcopy((caddr_t)mcp->entry, (caddr_t)&(mcad.mc_addr[j]), DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	/* end for */
	mcad.mc_cnt = j;
	for (i=0; i<j; i++)
		cmd->prmtr.prm_mcad.mc_addr[i] = mcad.mc_addr[i];

	if (bus_p & BUS_MCA) {
		outb (base_io + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS);
		outb (base_io + CTRL_REG_OFST, CR_RST|CR_BS);
	}
	else
		outb (base_io + CHAN_ATN_OFST, 1);

	if (el16wait_scb (scb, 1000))
		return (1);

	if (el16ack_586 (scb, base_io))
		return (1);

	return (0);
}

/******************************************************************************
 *  el16get_multicast()
 */
el16get_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
register mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
register int i;
unsigned char *dp;                                                              int found = 0;

        if((int)(mp->b_wptr - mp->b_rptr) == 0)
                found = bd->multicast_cnt;
        else {
                dp = mp->b_rptr;
                for (i = 0;(i < MULTI_ADDR_CNT) && (dp < mp->b_wptr);i++,mcp++)
                        if (mcp->status) {
                                BCOPY(mcp->entry,dp,DL_MAC_ADDR_LEN);
                                dp += DL_MAC_ADDR_LEN;
                                found++;
                        }
                mp->b_wptr = dp;
        }
        return found;
}

/******************************************************************************
 *  el16del_multicast()
 */
el16del_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
register int i;
bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
int rval, oldlevel;

        oldlevel = splstr();

        if (!el16is_multicast(bd,maddr))
                rval = 1;
	else {
        	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++)
			if ( (mcp->status) && (BCMP(maddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) )
                        	break;
        	mcp->status = 0;
		bd->multicast_cnt--;
        	rval = el16set_multicast(bd);
        }
        splx(oldlevel);
        return rval;
}

/******************************************************************************
 *  el16disable()
 */
el16disable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  el16enable()
 */
el16enable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  el16reset()
 */
el16reset (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  el16is_multicast()
 */
/*ARGSUSED*/
el16is_multicast (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
register mcat_t *mcp = &bdd->el16_multiaddr[0];
register int i;
int rval = 0;
int oldlevel;

	oldlevel = splstr();
	if (bd->multicast_cnt) {
		for (i = 0; i < MULTI_ADDR_CNT;i++,mcp++) {
			if ( (mcp->status) && (BCMP(eaddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) ) {
				rval = 1;
				break;
			}
		}
	}
	splx(oldlevel);
	return (rval);
}

/******************************************************************************
 *  Support routines for Berkeley Packet Filter (BPF).
 */
#ifdef NBPFILTER

STATIC
el16bpf_ioctl (ifp, cmd, addr)
struct	ifnet	*ifp;
int	cmd;
{
	return(EINVAL);
}

STATIC
el16bpf_output (ifp, buf, dst)
struct	ifnet	*ifp;
uchar_t	*buf;
struct	sockaddr *dst;
{
	return(EINVAL);
}

STATIC
el16bpf_promisc (ifp, flag)
struct	ifnet	*ifp;
int	flag;
{
	if (flag)
		return(el16promisc_on((DL_bdconfig_t*)ifp->if_next));
	else
		return(el16promisc_off((DL_bdconfig_t*)ifp->if_next));
}
#endif

#ifdef C_PIO
void
mybcopy(src, dest, count)
char *src;
char *dest;
int  count;
{
	int i;
	for (i=0; i< count; i++)
		dest[i] = src[i];
}
#endif
