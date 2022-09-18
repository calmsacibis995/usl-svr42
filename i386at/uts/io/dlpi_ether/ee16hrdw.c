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

static char prog_copyright[] = "Copyright 1991 Intel Corp. 468473-010";

#ident	"@(#)uts-x86at:io/dlpi_ether/ee16hrdw.c	1.12"
#ident	"$Header: $"

/*
 *  This file contains all of the hardware dependent code for EtherExpress.
 *  It is the companion file to ../../io/dlpi_ether.c
 */

#include <io/dlpi_ether/dlpi_ee16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/ee16.h>
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

#define BCOPY(from, to, len) bcopy((caddr_t)(from),(caddr_t)(to),(size_t)(len))
#define BCMP(s1, s2, len) bcmp((char*)(s1),(char*)(s2),(size_t)(len))

#define BYTE 0
#define WORD 1

extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t		DLsaps[];
extern	struct ifstats	*DLifstats;

extern	int		DLstrlog;
extern	char	DLid_string[];
extern	int		DLboards;
extern	char	DLcopyright[];
extern	int		ee16intr_to_index[];
extern	int		ee16timer_id;
extern	int		ee16wait_scb();
extern	ushort	ntohs(), htons();

STATIC	void	ee16tx_done(), ee16chk_ring();
void			bcopy_to_buffer(), bcopy_from_buffer();
STATIC	int		ee16ru_restart();
extern  ushort  ee16init_586(), ee16config_586();
void			outb();
struct	debug	ee16debug = {0, 0, 0, 0, 0};
static	ushort	rb;

void	ee16bdspecioctl(), ee16bdspecclose();

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*ee16ifp;
STATIC	int		ee16bpf_ioctl(), ee16bpf_output(), ee16bpf_promisc();

extern	void	bpf_tap(), outb();
#endif

/* ee16bdspecclose is called from DLclose->dlpi_ether.c */
void
ee16bdspecclose(q)
queue_t *q;
{
	return;
}

/* ee16bdspecioctl is called from DLioctl->dlpi_ether.c */
void
ee16bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *ioctl_req= (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
} /* end of ee16bdspecioctl */

/* Static command list chaining is implemented (refer 586 manual).
 * A packet to be sent is either 
 *   1. sent off directly (with a channel attention to 586)
 *   2. inserted in the command list (if not full), the interrupt
 *      service routine checks the command list 
 *   3. inserted in the queue (and removed by getq) when the command
 *      list is full. This case is also handled by the ISR.
 * An interrupt is generated either when a transmit command is completed
 * or when a packet has been received.
 */

/******************************************************************************
 *  ee16xmit_packet()
 *
 *  This routine is called from DLunitdata_req() and ee16tx_done(). It assumes
 *  we are at STREAMS spl interrupt level.
 */

ee16xmit_packet (bd, mp)
DL_bdconfig_t *bd;
mblk_t	*mp;
{
	bdd_t	*bdd      = (bdd_t *) bd->bd_dependent1;

ushort	p_tbd;
ushort	p_cmd;
ushort	p_txb;

char	*src;
ushort	dest;
mblk_t	*mp_tmp;

ushort	tx_size = 0;
ushort	msg_size = 0;
ushort	start = 0;
ushort  base_io = bd->io_start;
DL_mac_hdr_t *hdr = (DL_mac_hdr_t *)mp->b_rptr;

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"ee16xmit_packet on board %x",(int)bd));

	/* ring buffer is empty: therefore, we need to issue a channel attention
	 * signal for the current packet 
	 */
	if (bdd->head_cmd == NULL) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"ee16xmit_packet: ring EMPTY"));
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

		if (bdd->tail_cmd->next == bdd->head_cmd) {
			DL_LOG(strlog(DL_ID,200,1,SL_TRACE,"ee16xmit_packet: ring FULL"));
			ee16debug.ring_full++;
			bd->flags |= TX_BUSY;
		}
	}

	/* set up offsets */
	p_cmd = bdd->tail_cmd->ofst_cmd;

	/* tbd ptr guaranteed to be valid since ring buffer is used for transmits
	 * only - other commands use bdd->gen_cmd */
	read_buffer(WORD, p_cmd + 6, base_io, p_tbd);	/* read tbd offset */
	read_buffer(WORD, p_tbd + 4, base_io, p_txb);

	/* fill in 82586's transmit command block */
	write_buffer(WORD, bdd->tail_cmd->ofst_cmd, 0, base_io);	/* status */
	outw(base_io + DXREG, CS_EL | CS_CMD_XMIT | CS_INT);	/* command */
	outw(base_io + DXREG, 0xffff);							/* link */

	/* copy dest. address and sap address to the transmit command structure */
	/* src points to the destination address in the incoming frame */

	src = (char *)(hdr->dst.bytes);
	dest = bdd->tail_cmd->ofst_cmd + 6 + 2; /* add sizeof(xmt_tbd_ofst):2 */
	bcopy_to_buffer (src, dest, (ushort) DL_MAC_ADDR_LEN, base_io);
	/* set the xmit_length field */
	write_buffer(WORD, dest + sizeof(net_addr_t),hdr->mac_llc.ether.len_type, base_io); 
	mp->b_rptr += LLC_EHDR_SIZE;

	/* copy data to tx buffer */
	for (mp_tmp = mp; mp_tmp != NULL; mp_tmp = mp_tmp->b_cont) {
		msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
		bcopy_to_buffer ((caddr_t) mp_tmp->b_rptr, p_txb, msg_size, base_io);
		p_txb          += msg_size;
		tx_size        += msg_size;
		/* rptr is used by the BPF code below */
		/* mp_tmp->b_rptr += msg_size; */
	}

	/* 
		check size: its an error if message size > 1520 bytes
		This check is redundant ... done in the independent part

	if (tx_size > (ushort) sap->max_spdu) {					
		freemsg(mp);
		return (-1);
	}
	*/

	write_buffer(WORD, p_tbd, tx_size | CS_EOF, base_io);	

#ifdef NBPFILTER
	if (bd->bpf) {
		mblk_t	*mp_tap;

		if ((mp_tap = allocb(tx_size+MAC_HDR_LEN, BPRI_MED)) != NULL) {
			bcopy ((caddr_t)src,(caddr_t)mp_tap->b_wptr,DL_MAC_ADDR_LEN);
			mp_tap->b_wptr += DL_MAC_ADDR_LEN;
			bcopy ((caddr_t)bd->eaddr,(caddr_t)mp_tap->b_wptr,DL_MAC_ADDR_LEN);
			mp_tap->b_wptr += DL_MAC_ADDR_LEN;
			read_buffer(WORD, dest+sizeof(net_addr_t), base_io, 
                 * (ushort *) mp_tap->b_wptr);
			mp_tap->b_wptr += sizeof (ushort);

			for (mp_tmp=mp->b_cont; mp_tmp; mp_tmp=mp_tmp->b_cont) {
				msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
				bcopy((caddr_t)mp_tmp->b_rptr,(caddr_t)mp_tap->b_wptr,msg_size);
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
		if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
			cmn_err (CE_WARN, "ee16xmit_packet: scb command not cleared");
			bd->timer_val = 0;			/* force board to reset */
			freemsg(mp);
			return (-1);
		}
		write_buffer(WORD, bdd->ofst_scb + 2, SCB_CUC_STRT, base_io); /* cmd */
		outw(base_io + DXREG, bdd->tail_cmd->ofst_cmd);	/* cbl ofst */
		outb (bd->io_start + CA_CTRL, 1);
	}
	freemsg(mp);
	return (0);
}
/******************************************************************************
 *  ee16intr()
 */

void
ee16intr (level)
int	level;
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	ushort		cmd;
	ushort		base_io;
	int		index,i;
	ushort		scb_status;

	/* map irq level to the proper board. Make sure it's ours */
	if ((index = ee16intr_to_index [level]) == -1) {
		cmn_err (CE_WARN, "%s spurious interrupt", ee16id_string);
		return;
	}

	/* get pointers to structures */
	bd = &ee16config [index];
	base_io = bd->io_start;
	bdd = (bdd_t *) bd->bd_dependent1;

	if (bdd->head_cmd != 0) {
		read_buffer(WORD, bdd->head_cmd->ofst_cmd, base_io, cmd); /* status */
	}
	/* If scb command field doesn't get cleared, reset the board */
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
		cmn_err (CE_WARN, "ee16intr: scb command not cleared");
		bd->timer_val = 0;     /* cause board to reset */
		return;
	}
	read_buffer(WORD, bdd->ofst_scb, base_io, scb_status);
	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"EE16INTR: SCB status: %x;",scb_status));

	/* acknowledge 82586 interrupt */
	if ((scb_status & SCB_INT_MSK) != 0) {
		write_buffer(WORD, bdd->ofst_scb + 2, scb_status & SCB_INT_MSK, base_io);
		outb (base_io + CA_CTRL, 1);
	}

	/* If scb command field doesn't get cleared, reset the board */
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
		cmn_err (CE_WARN, "ee16intr: scb command not cleared");
		bd->timer_val = 0;     
		return;
	}

	if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
		ee16chk_ring (bd);

	if (scb_status & (SCB_INT_CX | SCB_INT_CNA))
		ee16tx_done (bd);
	else if ((bdd->head_cmd != 0) && (cmd & CS_CMPLT)) {
		ee16debug.tx_cmplt_missed++;
		cmn_err(CE_WARN,"ee16intr: tx completion missed\n");
		ee16tx_done (bd);
	} 

	/* clear interrupt */
	i = inb(base_io + SEL_IRQ);
	/* lower the intr_enable bit */
	outb(base_io + SEL_IRQ, i & 0xf7);
	/* raise the intr_enable bit */
	outb(base_io + SEL_IRQ, i | 0x08);

	/* Store error statistics in the MIB structure */
	read_buffer(WORD, bdd->ofst_scb + 8, base_io, 
			bd->mib.ifSpecific.etherCRCerrors);
	bd->mib.ifSpecific.etherAlignErrors   = inw(base_io + DXREG);
	bd->mib.ifSpecific.etherMissedPkts    = inw(base_io + DXREG);
	bd->mib.ifSpecific.etherOverrunErrors = inw(base_io + DXREG);
	bd->mib.ifInErrors = 
	   bd->mib.ifSpecific.etherCRCerrors + bd->mib.ifSpecific.etherAlignErrors +
	   bd->mib.ifSpecific.etherMissedPkts + bd->mib.ifSpecific.etherOverrunErrors;

	if (ee16ifstats)
		bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;

}

/******************************************************************************
 *  ee16tx_done()
 */

STATIC void
ee16tx_done (bd)
DL_bdconfig_t	*bd;
{
	bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
	ushort_t	cmd_status;
	DL_sap_t	*sap;
	mblk_t		*mp;
	int			x;
	int			next;
	ushort_t	base_io;

	base_io = bd->io_start;
	DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"ee16tx_done: for board %d", bd->bd_number));

	/* If ring is empty, we have nothing to process */
	if (bdd->head_cmd == 0) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"ee16tx_done: null head pointer"));
		return;
	}

	read_buffer(WORD, bdd->head_cmd->ofst_cmd, base_io, cmd_status);

	if (ee16ifstats)
		bd->ifstats->ifs_opackets++;

	/* Read the tx status register and see if there were any problems */
	if (!(cmd_status & CS_OK)) {
		bd->mib.ifOutErrors++;
		if (ee16ifstats)
			bd->ifstats->ifs_oerrors++;
	}

	if (cmd_status & CS_COLLISIONS) {
		bd->mib.ifSpecific.etherCollisions++;
		if (ee16ifstats)
			bd->ifstats->ifs_collisions++;
	}
	if (cmd_status & CS_CARRIER)
		bd->mib.ifSpecific.etherCarrierLost++;

	/* return if last command in ring buffer */
	if (bdd->head_cmd == bdd->tail_cmd) {
		DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"ee16tx_done: last command"));
		bdd->head_cmd = bdd->tail_cmd = 0;
		/* end of transmission: start the receive unit if not ready */
		/* ee16ru_restart(bd); */
		return;
	}

	/* there are more commands pending to be sent to the board */
	else {
		bdd->head_cmd = bdd->head_cmd->next;
		bd->flags &= ~TX_BUSY;
		if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
			cmn_err (CE_WARN, "ee16tx_done: scb command not cleared");
			bd->timer_val = 0;	
			return;
		}
		write_buffer(WORD, bdd->ofst_scb + 2, SCB_CUC_STRT, base_io); /* command */
		outw(base_io + DXREG, bdd->head_cmd->ofst_cmd);
		outb (bd->io_start + CA_CTRL, 1);
	}

	/*
	 *  Indicate outstanding transmit is done and see if there is work
	 *  waiting on our queue.
	 */
	if (bd->flags & TX_QUEUED) {
		next = bd->tx_next;
		sap  = &ee16saps [next];

		for (x=0; x<bd->max_saps; x++) {
			if (++next == bd->max_saps)
				next = 0;

			if ((sap->state == DL_IDLE) && (mp = getq(sap->write_q))){
				(void) ee16xmit_packet(bd, mp);
				bd->tx_next = next;
				bd->mib.ifOutQlen--;
				return;
			}

			if (next == 0)
				sap = ee16saps;
			else
				sap++;
		}

		/* make the queue empty */
		ee16debug.q_cleared++;
		bd->flags &= ~TX_QUEUED;
		bd->mib.ifOutQlen = 0;
		bd->tx_next = 0;
	}
}

/******************************************************************************
 *  ee16chk_ring (bd)
 */

STATIC void
ee16chk_ring (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	fd_t	fd_s;
	ushort  first_rbd, last_rbd;
	mblk_t	*mp;
	DL_sap_t *sap;
	int		x,nrbd;
	ushort	length;
	ushort	sap_id;
	ushort  fd;
	ushort  base_io = bd->io_start;

	DL_LOG (strlog (DL_ID, 103, 3, SL_TRACE, "ee16chk_ring:"));

	/* for all fds */
	for (fd=bdd->begin_fd; fd != 0xffff; fd=bdd->begin_fd)
	{
		bcopy_from_buffer((char *)&fd_s, fd, sizeof(fd_t), base_io);
		if (!(fd_s.fd_status & CS_CMPLT)) {
			DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "fd status: NOT CS_CMPLT"));
			break;
		}

		if (DLifstats)
			bd->ifstats->ifs_ipackets++;

		length = nrbd = 0;
		first_rbd = last_rbd = fd_s.fd_rbd_ofst;

		if (fd_s.fd_rbd_ofst != 0xffff) {
			/*
			 * find length of data and last rbd 
			 * holding the received frame.
			 */
			read_buffer(WORD,last_rbd,base_io, rb);
			while ((rb & CS_EOF) != CS_EOF) {
				nrbd++;
				read_buffer(WORD,last_rbd,base_io, rb);
				length += rb & CS_RBD_CNT_MSK;
				/* write_buffer( WORD, last_rbd, 0, base_io);*/ /* status */
				/* sanity check */
				read_buffer(WORD, last_rbd + 8, base_io, rb); 
				if ((rb & CS_EL) != CS_EL) {
					read_buffer(WORD, last_rbd + 2, base_io, last_rbd); /* next rbd */
				} else {
					cmn_err(CE_WARN,"out of receive buffers\n");
					break;
				}
				read_buffer(WORD,last_rbd,base_io, rb);
			} 
			read_buffer(WORD,last_rbd,base_io, rb);
			length += rb & CS_RBD_CNT_MSK;
			nrbd++;
			/* write_buffer( WORD, last_rbd, 0, base_io);*/ /* status */

			if (fd_s.fd_status & CS_OK) {
				short  temp_rbd;
				int    i,l;

				if ((mp = allocb (MAC_HDR_LEN+length, BPRI_MED)) == NULL) {
                  			DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "no receive resources"));
                     			bd->mib.ifInDiscards++; /* SNMP */
                                        bd->mib.ifSpecific.etherRcvResources++; /* SNMP */
				} else {
					/* fill in the mac header */
					bcopy((caddr_t) fd_s.fd_dest, (caddr_t)mp->b_wptr, MAC_HDR_LEN);
					mp->b_wptr += MAC_HDR_LEN;
					/* fill in the data from rcv buffer(s)*/
					for (i=0,temp_rbd=first_rbd; i<nrbd; i++) {
						read_buffer(WORD,temp_rbd,base_io, rb);
						l = rb & CS_RBD_CNT_MSK;
						read_buffer(WORD, temp_rbd + 4, base_io, rb); 
						bcopy_from_buffer ((caddr_t) mp->b_wptr,rb,l,base_io);
						mp->b_wptr += l;
						write_buffer(WORD,temp_rbd,0,base_io);/*status*/
						read_buffer(WORD, temp_rbd + 2, base_io, temp_rbd);
					}
					if (!ee16recv(mp,bd->sap_ptr))
						bd->mib.ifInOctets+= MAC_HDR_LEN+length;
				}
			} else if (ee16ifstats)
				bd->ifstats->ifs_ierrors++;
			
			/* re-queue rbd */
			read_buffer(WORD, last_rbd + 2, base_io, bdd->begin_rbd); /* rbd_nxt */
			write_buffer(WORD, last_rbd, 0, base_io);	/* status */
			outw(base_io + DXREG, 0xffff);		/* next rbd ofst */
			read_buffer(WORD, last_rbd + 8, base_io, rb);
			write_buffer(WORD, last_rbd + 8, rb | CS_EL, base_io);	

			write_buffer(WORD, bdd->end_rbd + 2, first_rbd, base_io);
			read_buffer(WORD, bdd->end_rbd + 8, base_io, rb);
			write_buffer(WORD, bdd->end_rbd + 8, rb & ~CS_EL, base_io);	
			bdd->end_rbd = last_rbd;
		}

		bdd->begin_fd = fd_s.fd_nxt_ofst;

		/* re-queue fd */
		fd_s.fd_status   = 0;
		fd_s.fd_cmd      = CS_EL;
		fd_s.fd_nxt_ofst = 0xffff;
		fd_s.fd_rbd_ofst = 0xffff;
		bcopy_to_buffer((char *)&fd_s, fd, sizeof(fd_t), base_io);

		write_buffer(WORD, bdd->end_fd + 4, fd, base_io);	/* nextfd */
		write_buffer(WORD, bdd->end_fd + 2, 0, base_io);	/* command */
		bdd->end_fd = fd;
	}
	ee16ru_restart (bd);
}

/******************************************************************************
 * ee16ru_restart ()
 */

STATIC int
ee16ru_restart (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd    	= (bdd_t *) bd->bd_dependent1;
	ushort  base_io     = bd->io_start;

	/* RU already running -- leave it alone */
	read_buffer(WORD, bdd->ofst_scb, base_io, rb);
	if ((rb & SCB_RUS_READY) == SCB_RUS_READY)
		return (1);

	/* Receive queue is exhausted, need to reset the board */
	if (bdd->begin_fd == 0xffff) {
		cmn_err(CE_CONT, "fd Q exhausted: begining reset\n");
		bd->timer_val = 0;
		return (0);
	}

	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * ee16chk_ring () and thus calls ee16ru_restart() which will
	 * then start the RU if necessary.
	 */
	read_buffer(WORD, bdd->begin_fd, base_io, rb); /* status */
	if (rb & CS_CMPLT) 
		return (1);

	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on page 2-15
	 */
	ee16debug.rcv_restart_count++;
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
		cmn_err (CE_WARN, "ru-restart: scb command not cleared");
		bd->timer_val = 0;					/* force board to reset */
		return (0);
	}

	/* set rbd_ofst */
	write_buffer(WORD, bdd->begin_fd + 6, bdd->begin_rbd, base_io);

	write_buffer(WORD, bdd->ofst_scb, 0, base_io); /* status */
	write_buffer(WORD, bdd->ofst_scb+2, SCB_RUC_STRT, base_io); /*cmd*/
	write_buffer(WORD, bdd->ofst_scb + 6, bdd->begin_fd, base_io);
	outb (bd->io_start + CA_CTRL, 1);

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"RU re-started"));
	return (1);
}

/******************************************************************************
 * ee16watch_dog (bd)
 */

void
ee16watch_dog ()
{
	DL_bdconfig_t	*bd;
	int				i, j;

	/* reset timeout to 5 seconds */
	ee16timer_id = timeout (ee16watch_dog, 0, drv_usectohz (EE16_TIMEOUT));

	for (i=0, bd=ee16config; i<ee16boards; bd++, i++) {
		if (bd->timer_val > 0)
			bd->timer_val--;

		if (bd->timer_val == 0) {
			bd->timer_val = -1;
			cmn_err (CE_NOTE,"%s board %d timed out.",ee16id_string,bd->bd_number);

			for (j=0; j<10; j++)
				if (ee16init_586 (bd) == 0)
					return;

			cmn_err (CE_WARN,"%s board %d will not reset.",ee16id_string,bd->bd_number);
		}
	}
}

/******************************************************************************
 * ee16promisc_on (bd)
 */

ee16promisc_on (bd)
DL_bdconfig_t	*bd;
{
	int		old;

	/* If already in promiscuous mode, just return */
	if (bd->promisc_cnt++)
		return (0);

	old = splstr();

	if (ee16config_586 (bd, PRO_ON))
		return (1);

	splx (old);
	return (0);
}

/******************************************************************************
 * ee16promisc_off (bd)
 */

ee16promisc_off (bd)
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

	if (ee16config_586 (bd, PRO_OFF))
		return (1);

	splx (old);
	return (0);
}

#ifdef ALLOW_SET_EADDR
/******************************************************************************
 *  ee16set_eaddr()
 */
ee16set_eaddr (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	return (1);	/* not supported yet */
}
#endif

/******************************************************************************
*   ee16get_multicast()
*
*/
ee16get_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
register mcat_t  *mcp = &(bdd->ee16_multiaddr[0]);
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
 *
 *  ee16set_multicast()
 */
ee16set_multicast(bd)
DL_bdconfig_t *bd;
{
bdd_t   *bdd = (bdd_t *) bd->bd_dependent1;
ushort  base_io = bd->io_start;
ushort i,j,total;
mcad_t mcad;
mcat_t *mcp = &(bdd->ee16_multiaddr[0]);
 
        if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
                return (1);

        write_buffer(WORD, bdd->ofst_scb + 0, 0, base_io);      /* status */
        outw(base_io + DXREG, SCB_CUC_STRT);    /* command : auto-incr */
        outw(base_io + DXREG, bdd->gen_cmd);    /* cbl : auto-incr */

        write_buffer(WORD, bdd->gen_cmd + 0, 0, base_io);       /* status */
        outw(base_io + DXREG, CS_CMD_MCSET | CS_EL); /* command : auto-incr */
        outw(base_io + DXREG, 0xffff);        /* link : auto-incr */

	for (i = 0,j = 0;i <  MULTI_ADDR_CNT;i++,mcp++)
		if (mcp->status) {
			bcopy((caddr_t)mcp->entry,(caddr_t)&(mcad.mc_addr[j]),DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	mcad.mc_cnt = j;
	write_buffer(WORD,bdd->gen_cmd + 6,mcad.mc_cnt,base_io);
	for (i = 0; i < j; i++)
		outb(base_io + DXREG,mcad.mc_addr[i]);
        outb (base_io + CA_CTRL, 1);

        if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
                return (1);

        if (ee16ack_586 (bdd->ofst_scb, base_io))
                return (1);

        return (0);
}

/******************************************************************************
 *  ee16add_multicast()
 */
ee16add_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
mcat_t  *mcp = &(bdd->ee16_multiaddr[0]);
register int i;
int rval,oldlevel;


        oldlevel = splstr();
        if ( (bd->multicast_cnt >= MULTI_ADDR_CNT) || (!maddr->bytes[0] & 0x1)){                splx(oldlevel);
                return 1;
        }
        if ( ee16is_multicast(bd,maddr)) {
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
        rval = ee16set_multicast(bd);
        splx(oldlevel);
        return rval;
}

/******************************************************************************
 *  ee16del_multicast()
 */
ee16del_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
register int i;
bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
mcat_t  *mcp = &(bdd->ee16_multiaddr[0]);
int rval, oldlevel;

        oldlevel = splstr();

        if (!ee16is_multicast(bd,maddr))
                rval = 1;
	else {
        	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++)
			if ( (mcp->status) && (BCMP(maddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) )
                        	break;
        	mcp->status = 0;
		bd->multicast_cnt--;
        	rval = ee16set_multicast(bd);
        }
        splx(oldlevel);
        return rval;
}

/******************************************************************************
 *  ee16disable()
 */
ee16disable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ee16enable()
 */
ee16enable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ee16reset()
 */
ee16reset (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ee16is_multicast()
 */
/*ARGSUSED*/
ee16is_multicast (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
register mcat_t *mcp = &bdd->ee16_multiaddr[0];
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
ee16bpf_ioctl (ifp, cmd, addr)
struct	ifnet	*ifp;
int	cmd;
{
	return(EINVAL);
}

STATIC
ee16bpf_output (ifp, buf, dst)
struct	ifnet	*ifp;
uchar_t	*buf;
struct	sockaddr *dst;
{
	return(EINVAL);
}

STATIC
ee16bpf_promisc (ifp, flag)
struct	ifnet	*ifp;
int	flag;
{
	if (flag)
		return(ee16promisc_on((DL_bdconfig_t*)ifp->if_next));
	else
		return(ee16promisc_off((DL_bdconfig_t*)ifp->if_next));
}
#endif

#ifdef C_PIO
/******************************************************************************/
void
bcopy_to_buffer(src, dest, count, base_io)
char   *src;
ushort dest;
ushort count;
ushort base_io;
{
	ushort i;

    write_buffer(BYTE, dest, *src, base_io);
	src++;
	for (i=1; i< count; i++,src++) 
		outb(base_io + DXREG, *src);	
}

/******************************************************************************/
void
bcopy_from_buffer(dest, src, count, base_io)
char   *dest;
ushort src;
ushort count;
ushort base_io;
{
	ushort i;

    /* (*dest) = (char )read_buffer(BYTE, src, base_io);*/
	/* This in is needed to avoid a race condition in the HW */
	i = inb(base_io + AUTOID);
	outw(base_io + RDPTR, src);
	*dest = (char) inb(base_io + DXREG);
	dest++;
	for (i=1; i< count; i++,dest++) 
		(*dest) = (char) inb(base_io + DXREG);
}

#endif		/* ifdef C_PIO */
