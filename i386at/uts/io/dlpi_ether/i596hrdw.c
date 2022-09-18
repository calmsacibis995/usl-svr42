/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)uts-x86at:io/dlpi_ether/i596hrdw.c	1.3"
#ident	"$Header: $"

/*
 *  This file contains all of the hardware dependent code for the 82596.
 *  It is the companion file to ../../io/dlpi_ether.c
 */

#include <io/dlpi_ether/dlpi_i596.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/i596.h>
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

extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t		DLsaps[];
extern	struct ifstats	*DLifstats;

extern	int		DLstrlog;
extern	char	DLid_string[];
extern	int		DLboards;
extern	char	DLcopyright[];
extern	int		i596intr_to_index[];
extern	int		i596timer_id;
extern	int		i596noactivity_cnt;
extern	int		i596wait_scb();
extern	ushort	ntohs(), htons();

STATIC	void	i596tx_done(), i596chk_ring(), printbuf();
void			bcopy_to_buffer(), bcopy_from_buffer();
STATIC	int		i596ru_restart();
extern  ushort  i596init_596(), i596config_596();
struct	debug	i596debug = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static	ushort	rb;

void	i596bdspecioctl(), i596bdspecclose();

extern unsigned short	i596_rcvbufsiz;
extern unsigned short	i596_chan_attn_addr;
extern unsigned short	i596_clr_int_addr;
extern unsigned short	i596_cmd_chain_enable;

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*i596ifp;
STATIC	int		i596bpf_ioctl(), i596bpf_output(), i596bpf_promisc();

extern	void	bpf_tap(), outb();
#endif

/* i596bdspecclose is called from DLclose->dlpi_ether.c */
void
i596bdspecclose(q)
queue_t *q;
{
	return;
}

/* i596bdspecioctl is called from DLioctl->dlpi_ether.c */
void
i596bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *ioctl_req= (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
} /* end of i596bdspecioctl */

/* Static command list chaining is implemented (refer 596 manual).
 * A packet to be sent is either 
 *   1. sent off directly (with a channel attention to 596)
 *   2. inserted in the command list (if not full), the interrupt
 *      service routine checks the command list 
 *   3. inserted in the queue (and removed by getq) when the command
 *      list is full. This case is also handled by the ISR.
 * An interrupt is generated either when a transmit command is completed
 * or when a packet has been received.
 */

/******************************************************************************
 *  i596xmit_packet()
 *
 *  This routine is called from DLunitdata_req() and i596tx_done(). It assumes
 *  we are at STREAMS spl interrupt level.
 */

i596xmit_packet (bd, mp)
DL_bdconfig_t *bd;
mblk_t	*mp;
{
	DL_mac_hdr_t		*hdr      = (DL_mac_hdr_t *)mp->b_rptr;
	bdd_t				*bdd      = (bdd_t *) bd->bd_dependent1;

	scb_t	*p_scb;
	tbd_t	*p_tbd;
	cmd_t	*p_cmd;
	char	*p_txb;

	char	*src;
	char	*dest;
	mblk_t	*mp_tmp;

	ushort	tx_size = 0;
	ushort	msg_size = 0;
	ushort	start = 0;

	caddr_t				vaddr;
	ulong				count;
	int					y;
	register int		i;
	register long		fraglen, thispage;
	register ulong_t	nbpp;
	paddr_t				addr, base;
	caddr_t				basevaddr;
	unsigned long		*p_userdata;

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"i596xmit_packet on board %x",bd));

	/* ring buffer is empty: therefore, we need to issue a channel attention
	 * signal for the current packet 
	 */
	if (bdd->head_cmd == NULL) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596xmit_packet: ring EMPTY"));
		bdd->head_cmd = bdd->tail_cmd = bdd->ring_buff;
		start = 1; 
	}

	/*
	 * else there's at least one command pending in ring: the packet is
	 *   inserted in the command list (if it is not full).
	 * Note that we don't check whether queue is full or not: if after insertion,
	 *   the list is full, we set the TX_BUSY flag. This causes dlpi_ether to
	 *   insert the packets in the (streams) q and the packet is removed by the
	 *   interrupt routine when there is enough space in the command list.
	 * Insertion in the command list is done from the tail end and commands are
	 *   removed from the command list from the head.
	 */
	else {
		bdd->tail_cmd = bdd->tail_cmd->next;
		DL_LOG(strlog(DL_ID,200,1,SL_TRACE,"i596xmit_packet: command pending"));

		if (bdd->tail_cmd->next == bdd->head_cmd) {
			DL_LOG(strlog(DL_ID,200,1,SL_TRACE,"i596xmit_packet: ring FULL"));
			i596debug.ring_full++;
			bd->flags |= TX_BUSY;
		}
	}

	/* set up pointers */
	p_cmd = (cmd_t *)(bdd->virt_ram + bdd->tail_cmd->ofst_cmd);
	p_tbd = (tbd_t *)(bdd->virt_ram + p_cmd->prmtr.prm_xmit.xmt_tbd_ofst);
	DL_LOG(strlog(DL_ID, 0, 0, SL_TRACE,
			"i596: xmt_packet: p_cmd=%x p_tbd=%x\n", p_cmd, p_tbd));

	/* fill in 82596's transmit command block */
	p_cmd->cmd_status	= 0;
#ifndef COMPAT_MODE
	p_cmd->cmd_cmd		= CS_CMD_XMIT | FLEX_MODE | CS_INT | CS_EL;
#else
	p_cmd->cmd_cmd		= CS_CMD_XMIT | CS_INT | CS_EL;
#endif
	p_cmd->cmd_nxt_ofst = 0xffff;

	/* copy dest. address and sap address to the transmit command structure */
	/* src points to the destination address in the incoming frame */

	src = (char *)(hdr->dst.bytes);
	dest = (char *)(p_cmd->prmtr.prm_xmit.xmt_dest);
	bcopy(src, dest, (ushort)DL_MAC_ADDR_LEN);
	p_cmd->prmtr.prm_xmit.xmt_length = hdr->mac_llc.ether.len_type;
	mp->b_rptr += LLC_EHDR_SIZE;

	/* set up tbd */
	nbpp = ptob(1);					/* XXX will not work for V.3.2 */
	for (mp_tmp = mp; mp_tmp != NULL; mp_tmp = mp_tmp->b_cont) {
		count = msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
		vaddr = (caddr_t)(mp_tmp->b_rptr);
		DL_LOG(strlog(DL_ID, 0, 0, SL_TRACE,
			"i596: xmt_packet: vaddr=%x count=%x\n", vaddr, count));
		/*
		 * Insure contiguous physical memory.
		 */
		for (i = 0; count != 0; i++) {
			base = vtop(vaddr, NULL);	/* Compute physical addr of segment */
			basevaddr = vaddr;
			fraglen = 0;				/* Zero bytes so far */
			do {
				thispage = min(count, (nbpp - ((nbpp - 1) & ((ulong_t)vaddr))));
				fraglen += thispage;			/* This many more contiguous */
				vaddr += thispage;				/* Bump virtual address */
				count -= thispage;				/* Recompute amount left */
				if (!count)
					break;						/* End of request */
				addr = vtop(vaddr, NULL);		/* Get next page's address */
			} while (base + fraglen == addr);

DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"xmt_packet: base=%x fraglen=%x.", base, fraglen));
			p_tbd->tbd_buff = base & 0xffff;
			p_tbd->tbd_buff_base = base >> 16;
			p_tbd->tbd_count = fraglen;

			if (p_tbd->tbd_buff % 2) {
				p_txb = (char *)p_tbd + 8;
				*p_txb = *((unsigned char *)basevaddr);
		
				/* fill the one byte in a one tbd */
				p_tbd->tbd_count		= 1;
				p_userdata				= (unsigned long *)(p_txb);
				p_tbd->tbd_buff			= kvtophys(p_userdata) & 0xffff;
				p_tbd->tbd_buff_base	= kvtophys(p_userdata) >> 16;

				if (fraglen > 1)
				{
					/* fill the rest in another tbd */
					p_tbd++;
					p_tbd->tbd_count     = fraglen - 1;
					p_userdata			 =
						(unsigned long *)((unsigned char *)basevaddr + 1);
					p_tbd->tbd_buff      = kvtophys(p_userdata) & 0xffff;
					p_tbd->tbd_buff_base = kvtophys(p_userdata) >> 16;
				}
			}
			p_tbd++;
		}
		tx_size += msg_size;
	}
	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"xmt_packet: tx_size=%x\n", tx_size));

	/*
	if (tx_size > (ushort) sap->max_spdu) {					
		freemsg(mp);
		return (-1);
	}
	*/

	(--p_tbd)->tbd_count |= CS_EOF;
	bdd->tail_cmd->xmit_mp = mp;
	DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596xmit_packet: xmit_mp %x", mp));

#ifdef NBPFILTER
	if (bd->bpf) {
		mblk_t	*mp_tap;

		if ((mp_tap = allocb(tx_size+MAC_HDR_LEN, BPRI_MED)) != NULL) {
			bcopy ((caddr_t)src,(caddr_t)mp_tap->b_wptr,DL_MAC_ADDR_LEN);
			mp_tap->b_wptr += DL_MAC_ADDR_LEN;
			bcopy ((caddr_t)bd->eaddr,(caddr_t)mp_tap->b_wptr,DL_MAC_ADDR_LEN);
			mp_tap->b_wptr += DL_MAC_ADDR_LEN;
            *(ushort *)mp_tap->b_wptr = p_cmd->prmtr.prm_xmit.xmt_length;
			mp_tap->b_wptr += sizeof(ushort);

			for (mp_tmp=mp->b_cont; mp_tmp; mp_tmp=mp_tmp->b_cont) {
				msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
				bcopy((caddr_t)mp_tmp->b_rptr,(caddr_t)mp_tap->b_wptr,msg_size);
				mp_tap->b_wptr += msg_size;
			}

			DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596xmit_packet: bpf_tap"));
			bpf_tap(bd->bpf, mp_tap->b_rptr, msg_size);
			freemsg(mp_tap);
		}
	}
#endif

	bd->mib.ifOutOctets += tx_size + MAC_HDR_LEN;	/* SNMP */

	i596debug.tx_ringed++;
	/*
	 * if we need to start the 82596, issue a channel attention
	 */
	if (start) {
		p_scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
		if (i596wait_scb(p_scb, 1000)) {
			bd->timer_val = 0;			/* force board to reset */
			freemsg(mp);
			return (-1);
		}
		bd->timer_val = 3;
		p_scb->scb_cmd = SCB_CUC_STRT;
		p_scb->scb_cbl_ofst = bdd->tail_cmd->ofst_cmd;
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596xmit_packet: SCB_CUC_STRT ofst_cmd=%x", bdd->tail_cmd->ofst_cmd));
		outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */
	}
	return (0);
}
/******************************************************************************
 *  i596intr()
 */

void
i596intr (level)
int	level;
{
	DL_bdconfig_t	*bd;
	bdd_t			*bdd;
	scb_t			*scb;
	int				base_io;
	int				index,i;
	ushort			scb_status;


	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"I596INTR:"));

	/* Needed for panther system. */
	if (i596_clr_int_addr) {
		DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596intr: clear int port %x", i596_clr_int_addr));
		outw(i596_clr_int_addr, 0);		/* write to port clears int */
	}

	/* map irq level to the proper board. Make sure it's ours */
	if ((index = i596intr_to_index[level]) == -1) {
		cmn_err (CE_WARN, "%s spurious interrupt", i596id_string);
		return;
	}

	/* get pointers to structures */
	bd = &i596config[index];
	base_io = bd->io_start;
	bdd = (bdd_t *)bd->bd_dependent1;
	scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);

	/* If scb command field doesn't get cleared, reset the board */
	if (i596wait_scb(scb, 1000)) {
		bd->timer_val = 0;     			/* cause board reset */
		return;
	}

	scb_status = scb->scb_status;
	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"I596INTR: SCB status: %x;",scb_status));

	/* acknowledge interrupt */
	if ((scb->scb_cmd = (scb_status & SCB_INT_MSK)) != 0)
		outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */
	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"I596INTR: SCB cmd: %x;",scb->scb_cmd));

	if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
		i596chk_ring(bd);

	if (scb_status & (SCB_INT_CX | SCB_INT_CNA))
		i596tx_done(bd);

	/* Store error statistics in the MIB structure */
	bd->mib.ifSpecific.etherAlignErrors   = scb->scb_aln_err;;
	bd->mib.ifSpecific.etherCRCerrors	  = scb->scb_crc_err;;
	bd->mib.ifSpecific.etherMissedPkts    = scb->scb_rsc_err;;
	bd->mib.ifSpecific.etherOverrunErrors = scb->scb_ovrn_err;;
	bd->mib.ifInErrors = 
	   bd->mib.ifSpecific.etherCRCerrors + bd->mib.ifSpecific.etherAlignErrors +
	   bd->mib.ifSpecific.etherMissedPkts + bd->mib.ifSpecific.etherOverrunErrors;

	if (i596ifstats)
		bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;
}

/******************************************************************************
 *  i596tx_done()
 */

STATIC void
i596tx_done (bd)
DL_bdconfig_t	*bd;
{
	bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
	cmd_t		*cmd;
	scb_t		*scb;
	DL_sap_t	*sap;
	mblk_t		*mp;
	int			x;
	int			next;
	ring_t		*next_cmd;

	DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596tx_done: for board %d", bd->bd_number));

	/* If ring is empty, we have nothing to process */
	if (bdd->head_cmd == 0) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596tx_done: null head pointer"));
		return;
	}

	i596debug.tx_done++;
	bd->timer_val = -1;	
	do
	{
		DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596tx_done: free xmit_mp %x", bdd->head_cmd->xmit_mp));
		i596debug.tx_free_mp++;
		freemsg(bdd->head_cmd->xmit_mp);
		bdd->head_cmd->xmit_mp = NULL;
		if (i596ifstats)
			bd->ifstats->ifs_opackets++;
		cmd = (cmd_t *)(bdd->virt_ram + bdd->head_cmd->ofst_cmd);

		/* Read the tx status register and see if there were any problems */
		DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596tx_done: cmd_status %x", cmd->cmd_status));
		if (!(cmd->cmd_status & CS_OK)) {
			bd->mib.ifOutErrors++;
			if (i596ifstats)
				bd->ifstats->ifs_oerrors++;
		}
		if (cmd->cmd_status & CS_COLLISIONS) {
			bd->mib.ifSpecific.etherCollisions++;
			if (i596ifstats)
				bd->ifstats->ifs_collisions++;
		}
		if (cmd->cmd_status & CS_CARRIER) {
			bd->mib.ifSpecific.etherCarrierLost++;
			DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596tx_done: No Carrier Sense signal during transmission."));
		}

		if (cmd->cmd_cmd & (CS_INT | CS_EL))
			break;
		bdd->head_cmd = bdd->head_cmd->next;
	} while (1);

	if (bdd->head_cmd == bdd->tail_cmd) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596tx_done: last command"));
		bdd->head_cmd = bdd->tail_cmd = 0;
		return;
	} else {
		/* there are more commands pending to be sent to the board */
		if (i596_cmd_chain_enable) {
			next_cmd = bdd->head_cmd = bdd->head_cmd->next;
			/* if there is more than one cmd, chain them. */
			while (next_cmd != bdd->tail_cmd) {
				DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596tx_done: linking cmd %x", next_cmd->ofst_cmd));
				i596debug.tx_chained++;
				cmd = (cmd_t *)(bdd->virt_ram + next_cmd->ofst_cmd);
				cmd->cmd_cmd &= ~(CS_EL | CS_INT);
				cmd->cmd_nxt_ofst = next_cmd->next->ofst_cmd;
				next_cmd = next_cmd->next;
			}
		} else
			bdd->head_cmd = bdd->head_cmd->next;
		bd->flags &= ~TX_BUSY;
		scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
		if (i596wait_scb(scb, 1000)) {
			bd->timer_val = 0;	
			return;
		}
		bd->timer_val = 3;
		scb->scb_cmd = SCB_CUC_STRT;
		scb->scb_cbl_ofst = bdd->head_cmd->ofst_cmd;
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596tx_done: SCB_CUC_STRT ofst_cmd=%x", bdd->tail_cmd->ofst_cmd));
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596tx_done: SCB_CUC_STRT"));
		outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */
	}

	/*
	 *  Indicate outstanding transmit is done and see if there is work
	 *  waiting on our queue.
	 */
	if (bd->flags & TX_QUEUED) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596tx_done: TX_QUEUED"));
		next = bd->tx_next;
		sap  = &i596saps [next];

		for (x=0; x<bd->max_saps; x++) {
			if (++next == bd->max_saps)
				next = 0;

			if ((sap->state == DL_IDLE) && (mp = getq(sap->write_q))){
				(void) i596xmit_packet(sap->bd, mp);
				bd->tx_next = next;
				bd->mib.ifOutQlen--;
				return;
			}

			if (next == 0)
				sap = i596saps;
			else
				sap++;
		}

		/* make the queue empty */
		i596debug.q_cleared++;
		bd->flags &= ~TX_QUEUED;
		bd->mib.ifOutQlen = 0;
		bd->tx_next = 0;
	}
}

/******************************************************************************
 *  i596chk_ring (bd)
 */

STATIC void
i596chk_ring (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	fd_t	*fd;
	rbd_t	*p_rbd, *first_rbd, *last_rbd;
	mblk_t	*mp;
	DL_sap_t *sap;
	int		x,nrbd;
	ushort	length;
	ushort	sap_id;
	paddr_t	paddr;
	ushort	bytes_in_msg;

	DL_LOG(strlog (DL_ID, 103, 3, SL_TRACE, "i596chk_ring:"));

	/* for all fds */
	for (fd=bdd->begin_fd; fd != NULL; fd=bdd->begin_fd)
	{
		DL_LOG(strlog(DL_ID,103,3,SL_TRACE,"chk_ring: fd: %x;",fd));
		DL_LOG(strlog(DL_ID,103,3,SL_TRACE,"chk_ring: fd_status: %x;",fd->fd_status));
		if (!(fd->fd_status & CS_CMPLT)) {
			DL_LOG(strlog(DL_ID,103,3,SL_TRACE,"chk_ring: not CS_CMPLT"));
			break;
		}

		if (DLifstats)
			bd->ifstats->ifs_ipackets++;

		length = 0;
		last_rbd = first_rbd = p_rbd = (rbd_t *)(bdd->virt_ram+fd->fd_rbd_ofst);

		if (fd->fd_rbd_ofst != 0xffff) {
			if (fd->fd_status & CS_OK) {
				DL_LOG(strlog(DL_ID,103,3,SL_TRACE,"chk_ring: fd_status CS_OK"));

				if ((mp = allocb (MAC_HDR_LEN, BPRI_MED)) == NULL) {
					DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "chk_ring: no receive resources"));
					bd->mib.ifInDiscards++;                     /* SNMP */
					bd->mib.ifSpecific.etherRcvResources++;     /* SNMP */

				} else {
					DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "chk_ring: putq "));

					/* fill in the mac header */
					bcopy((caddr_t)fd->fd_dest, (caddr_t)mp->b_wptr,
						       MAC_HDR_LEN);
					mp->b_wptr += MAC_HDR_LEN;

					/* link in the data from rcv buffer(s) */
					do {
						bytes_in_msg = p_rbd->rbd_status & CS_RBD_CNT_MSK;
DL_LOG(strlog(DL_ID,103,3,SL_TRACE,"chk_ring: bytes_in_msg: %d;",bytes_in_msg));
						p_rbd->rx_mp->b_wptr += bytes_in_msg;
						linkb(mp, p_rbd->rx_mp);
						if ((p_rbd->rx_mp = allocb(i596_rcvbufsiz+4, BPRI_MED)) == NULL) {
							DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "allocb failed "));
							p_rbd->rx_mp = unlinkb(mp);
							p_rbd->rx_mp->b_wptr = p_rbd->rx_mp->b_rptr;
							freemsg(mp);
							mp = NULL;
							break;
						}
						paddr = kvtophys(p_rbd->rx_mp->b_rptr);
						if (paddr % 2) {
							/* 596 can't handle odd buffer */
							paddr = kvtophys(++p_rbd->rx_mp->b_rptr);
							p_rbd->rx_mp->b_wptr++;
						}
						p_rbd->rbd_buff			= paddr & 0xffff;
						p_rbd->rbd_buff_base	= paddr >> 16;
						if (((p_rbd->rbd_status & CS_EOF) == CS_EOF) ||
							((p_rbd->rbd_size & CS_EL) == CS_EL))
							break;
						length += p_rbd->rbd_status & CS_RBD_CNT_MSK;
						p_rbd->rbd_status = 0;
						p_rbd = (rbd_t *)(bdd->virt_ram + p_rbd->rbd_nxt_ofst);
					} while (1);
					length += p_rbd->rbd_status & CS_RBD_CNT_MSK;
					last_rbd = p_rbd;
					if (!DLrecv(mp, bd->sap_ptr))
						bd->mib.ifInOctets += MAC_HDR_LEN + length;
				}
			} else if (i596ifstats) {
				bd->ifstats->ifs_ierrors++;
			}

			while (((last_rbd->rbd_status & CS_EOF) != CS_EOF) &&
					((last_rbd->rbd_size & CS_EL) != CS_EL)) {
				last_rbd->rbd_status = 0;
				last_rbd = (rbd_t *)(bdd->virt_ram + last_rbd->rbd_nxt_ofst);
			}

			/* re-queue rbd */
			bdd->begin_rbd = (rbd_t *)(bdd->virt_ram + last_rbd->rbd_nxt_ofst);

			last_rbd->rbd_status = 0;
			last_rbd->rbd_size |= CS_EL;
			last_rbd->rbd_nxt_ofst = 0xffff;

			bdd->end_rbd->rbd_nxt_ofst = (char *)first_rbd - bdd->virt_ram;
			bdd->end_rbd->rbd_size &= ~CS_EL;

			bdd->end_rbd = last_rbd;
		}

		/* re-queue fd */
		if (fd->fd_nxt_ofst == 0xffff) {
			bdd->begin_fd = NULL;
		}
		else
			bdd->begin_fd = (fd_t *)(bdd->virt_ram + fd->fd_nxt_ofst);
		bdd->end_fd->fd_nxt_ofst = (char *)fd - bdd->virt_ram;
#ifndef COMPAT_MODE
		bdd->end_fd->fd_cmd = FLEX_MODE;
#else
		bdd->end_fd->fd_cmd = 0;
#endif
		bdd->end_fd = fd;

		fd->fd_status = 0;
#ifndef COMPAT_MODE
		fd->fd_cmd = FLEX_MODE | CS_EL;
#else
		fd->fd_cmd = CS_EL;
#endif
		fd->fd_nxt_ofst	= 0xffff;
		fd->fd_rbd_ofst	= 0xffff;
	}
	i596ru_restart(bd);
}

/******************************************************************************
 * i596ru_restart ()
 */

STATIC int
i596ru_restart(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd    	= (bdd_t *) bd->bd_dependent1;
	scb_t	*scb		= (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
	fd_t	*begin_fd	= bdd->begin_fd;

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"ru_restart: scb_status", scb->scb_status));
	/* RU already running -- leave it alone */
	if ((scb->scb_status & SCB_RUS_READY) == SCB_RUS_READY) {
		DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"ru_restart: SCB_RUS_READY"));
		return (1);
	}
#ifdef DEBUG
	cmn_err (CE_NOTE, "i596ru_restart: scb_status = %x", scb->scb_status);
#endif

	/* Receive queue is exhausted, need to reset the board */
	if (begin_fd == NULL) {
		cmn_err(CE_CONT, "fd Q exhausted: begining reset\n");
		bd->timer_val = 0;
		return (0);
	}

	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * i596chk_ring () and thus calls i596ru_restart() which will
	 * then start the RU if necessary.
	 */
	if (begin_fd->fd_status & CS_CMPLT) {
		DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"ru_restart: CS_CMPLT"));
		return (1);
	}

	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on page 2-15
	 */
	i596debug.rcv_restart_count++;
	begin_fd->fd_rbd_ofst = (ushort)((char *)bdd->begin_rbd - bdd->virt_ram);
	if (i596wait_scb(scb, 1000)) {
		bd->timer_val = 0;					/* force board to reset */
		return (0);
	}

	scb->scb_rfa_ofst = (ushort)((char *)bdd->begin_fd - bdd->virt_ram);
	scb->scb_cmd = SCB_RUC_STRT;
	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"RU re-started"));
	return (1);
}

/******************************************************************************
 * i596watch_dog()
 */

void
i596watch_dog()
{
	DL_bdconfig_t	*bd;
	int				i, j;

	i596timer_id = timeout(i596watch_dog, 0, drv_usectohz(I596_TIMEOUT));

	for (i=0, bd=i596config; i<i596boards; bd++, i++) {
		if (bd->timer_val > 0)
			bd->timer_val--;

		if (bd->timer_val == 0) {
			bd->timer_val = -1;
			DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596watch_dog: timed out"));
#ifdef DEBUG
			{
			bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
			scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);

			cmn_err (CE_CONT,"i596watch_dog: tx timed out, scb_status=%x.\n",
				 scb->scb_status);
			}
#endif

			for (j=0; j<10; j++)
				if (i596init_596(bd) == 0) {
					bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
					ring_t	*last_cmd = bdd->head_cmd;
					int		last_cmd_count = 0;
					cmd_t	*cmd;

					do {
						if (last_cmd->xmit_mp == NULL)
							break;
						last_cmd_count++;
						cmd = (cmd_t *)(bdd->virt_ram + last_cmd->ofst_cmd);
#ifdef DEBUG
						cmn_err (CE_CONT,
							"i596watch_dog: cmd_cmd=%x cmd_status=%x.\n",
								cmd->cmd_cmd, cmd->cmd_status);
#endif
						cmd->cmd_status = 0;
						if (cmd->cmd_cmd & (CS_INT | CS_EL))
							break;
						last_cmd = last_cmd->next;
					} while (1);
					if (last_cmd->xmit_mp) {
						scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);

						if (i596wait_scb(scb, 1000)) {
							bd->timer_val = 0;	
							return;
						}
						bd->timer_val = 6;
						scb->scb_cmd = SCB_CUC_STRT;
						scb->scb_cbl_ofst = bdd->head_cmd->ofst_cmd;
						DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"i596watch_dog: re-transmit of %x command(s).\n", last_cmd_count));
						outw(i596_chan_attn_addr, (ushort)1);	/* chan att */
					}
					return;
				}

			cmn_err (CE_WARN,"%s will not reset.", i596id_string);
		} else if ((bd->flags & BOARD_PRESENT) && (i596noactivity_cnt++ > 10)) {
			bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
			scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);

			if ((scb->scb_status & SCB_RUS_READY) != SCB_RUS_READY) {
#ifdef DEBUG
				cmn_err(CE_WARN,"i596watch_dog: RU not ready, scb_status=%x.",						scb->scb_status);
#endif
				i596ru_restart(bd);
				i596noactivity_cnt = 0;
			}
		}
	}
}

/******************************************************************************
 * i596promisc_on (bd)
 */

i596promisc_on (bd)
DL_bdconfig_t	*bd;
{
	int		old;

	/* If already in promiscuous mode, just return */
	if (bd->promisc_cnt++)
		return (0);

	old = splstr();

	if (i596config_596 (bd, PRO_ON, LOOP_OFF))
		return (1);

	splx (old);
	return (0);
}

/******************************************************************************
 * i596promisc_off (bd)
 */

i596promisc_off (bd)
DL_bdconfig_t	*bd;
{
	int		old;

	/* return if the board is not in a promiscuous mode */
	if (!bd->promisc_cnt)
		return (0);

	/* return if this is not the last promiscuous SAP */
	if (--bd->promisc_cnt)
		return (0);

	old = splstr();

	if (i596config_596 (bd, PRO_OFF, LOOP_OFF))
		return (1);

	splx (old);
	return (0);
}

#ifdef ALLOW_SET_EADDR
/******************************************************************************
 *  i596set_eaddr()
 */
i596set_eaddr (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	return (1);	/* not supported yet */
}
#endif

/******************************************************************************
 *  i596get_multicast()
 */
int
i596get_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
register mcat_t  *mcp = &(bdd->i596_multiaddr[0]);
register int i;
unsigned char *dp;
int found = 0;

	if((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		dp = mp->b_rptr;
		for (i = 0;(i < MULTI_ADDR_CNT) && (dp < mp->b_wptr);i++,mcp++)
			if (mcp->status) {
				bcopy((caddr_t)mcp->entry, (caddr_t)dp, DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
	}
	return found;
}

/******************************************************************************
 *  i596set_multicast()
 */
int
i596set_multicast(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *)(bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io = bd->io_start;
	mcat_t 	*mcp = &(bdd->i596_multiaddr[0]);
	register int i;
	register int j;

	if (i596wait_scb(scb, 1000))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd	  = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;

	cmd->cmd_status   = 0;
	cmd->cmd_cmd	  = CS_CMD_MCSET | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	for (i =0,j = 0;i < MULTI_ADDR_CNT;i++,mcp++) {
		if (mcp->status) {
			bcopy((caddr_t)mcp->entry, (caddr_t)&(cmd->prmtr.prm_mcad.mc_addr[j]), DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	}
	cmd->prmtr.prm_mcad.mc_cnt = j;

	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	if (i596wait_scb(scb, 1000))
		return (1);

	if (i596ack_596(scb, base_io))
		return (1);

	return (0);
}

/******************************************************************************
 *  i596add_multicast()
 */
int
i596add_multicast(bd, maddr)
DL_bdconfig_t *bd;
DL_eaddr_t *maddr;
{
bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
mcat_t 	*mcp = &(bdd->i596_multiaddr[0]);
register int i;
int rval,oldlevel;

	oldlevel = splstr();
	if ( (bd->multicast_cnt >= MULTI_ADDR_CNT) || (!maddr->bytes[0] & 0x1)){
		splx(oldlevel);
		return 1;
	}
	if ( i596is_multicast(bd,maddr)) {
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
	rval = i596set_multicast(bd);
	splx(oldlevel);
	return rval;
}

/******************************************************************************
 *  i596del_multicast()
 */
int
i596del_multicast(bd, maddr)
DL_bdconfig_t *bd;
DL_eaddr_t *maddr;
{
register int i;
bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
mcat_t 	*mcp = &(bdd->i596_multiaddr[0]);
int rval, oldlevel;

	oldlevel = splstr();

	if (!i596is_multicast(bd,maddr))
		rval = 1;
	else {
		for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++)
			if ( (mcp->status) && (bcmp((caddr_t)maddr->bytes, (caddr_t)mcp->entry, DL_MAC_ADDR_LEN) == 0) )
                        break;
		mcp->status = 0;
		bd->multicast_cnt--;
		rval = i596set_multicast(bd);
	}
	splx(oldlevel);
	return rval; 
}

/******************************************************************************
 *  i596disable()
 */
i596disable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  i596enable()
 */
i596enable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  i596reset()
 */
i596reset (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  i596is_multicast()
 */
/*ARGSUSED*/
i596is_multicast(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
bdd_t *bdd= (bdd_t *)bd->bd_dependent1;
register mcat_t *mcp = &bdd->i596_multiaddr[0]; 
register int i;
int rval = 0;
int oldlevel;

	oldlevel = splstr();
	if (bd->multicast_cnt) {
		for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
			if ( (mcp->status) && (bcmp((caddr_t)eaddr->bytes, (caddr_t)mcp->entry, DL_MAC_ADDR_LEN) == 0) ) {
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
i596bpf_ioctl (ifp, cmd, addr)
struct	ifnet	*ifp;
int	cmd;
{
	return(EINVAL);
}

STATIC
i596bpf_output (ifp, buf, dst)
struct	ifnet	*ifp;
uchar_t	*buf;
struct	sockaddr *dst;
{
	return(EINVAL);
}

STATIC
i596bpf_promisc (ifp, flag)
struct	ifnet	*ifp;
int	flag;
{
	if (flag)
		return(i596promisc_on((DL_bdconfig_t*)ifp->if_next));
	else
		return(i596promisc_off((DL_bdconfig_t*)ifp->if_next));
}
#endif
