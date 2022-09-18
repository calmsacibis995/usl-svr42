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

static char prog_copyright[] = "Copyright 1991 Intel Corp. 469176-010";

#ident	"@(#)uts-x86at:io/dlpi_ether/i596init.c	1.4"
#ident	"$Header: $"

/* hardware dependent code for i596 */

#include <io/dlpi_ether/dlpi_i596.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/i596.h>
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
#include <util/inline.h>

extern	DL_bdconfig_t   DLconfig[];
extern	DL_sap_t        DLsaps[];
extern	int	            DLboards;
extern	int	            DLinetstats;
extern	int	            DLstrlog;

extern	struct  ifstats *ifstats;
extern	struct  debug   i596debug;
extern	char            *i596_ifname;

extern	void	wakeup();

/* Check for validity of interrupts */
int i596intr_to_index[] = {-1,-1,-1,-1,-1,-1,-1,-1,
                           -1,-1,-1,-1,-1,-1,-1,-1,
                           -1,-1,-1,-1,-1,-1,-1,-1};
int	i596timer_id;
int	i596noactivity_cnt;

struct	ifstats	*i596ifstats;

STATIC void     i596tx_done(), i596init_rx(), i596init_tx(), i596free_rxbufs();
ushort   		i596config_596();
ushort		    i596diagnose_596();

STATIC int      i596ia_setup_596();
int				i596ack_596();
ushort		    i596wait_scb();
ushort		    i596init_596();

void			i596dump_flash();
void            i596watch_dog();

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*i596ifp;
extern 	int     i596bpf_ioctl(), i596bpf_output(), i596bpf_promisc();
extern	void    bpfattach();
#endif

extern int i596cable_type;

/* Configuration definitions. */
extern unsigned short	i596_n_cmd;
extern unsigned short	i596_n_tbd;
extern unsigned short	i596_n_fd;
extern unsigned short	i596_n_rbd;
extern unsigned short	i596_rcvbufsiz;
/* Port address definitions. */
extern unsigned short	i596_station_addr;
extern unsigned short	i596_port1_addr;
extern unsigned short	i596_port2_addr;
extern unsigned short	i596_chan_attn_addr;
extern unsigned short	i596_clr_int_addr;
extern unsigned short	i596_cmd_chain_enable;
extern unsigned short	i596_enet_in_flash;

#ifdef MDV_DEBUG
/* Default Ethernet address - used for development on MDV. */
unsigned char i596_default_eaddr[6] = { 0x00, 0xAA, 0x00, 0x00, 0x0B, 0xAD };
#endif

char	i596_mem[MAX_RAM_SIZE+0xf];

char	i596id_string[] = "i596 V1.0";
char	i596copyright[] = "Copyright (c) 1991 Intel Corp., All Rights Reserved";

/*
 * Header, wrapper, and function declarations and definitions for loadability
 */

#include <util/mod/moddefs.h>
#define DRVNAME		"i596 - Loadable i596 ethernet driver"

STATIC	int	i596_load(), i596_unload();

MOD_DRV_WRAPPER(i596, i596_load, i596_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

int i596init(), i596start();
STATIC void i596uninit();

STATIC int
i596_load(void)
{
	int ret_code;

	cmn_err(CE_NOTE, "!MOD: in i596_load()");

	if ((ret_code = i596init()) != 0)
		return (ret_code);
	if ((ret_code = i596start()) != 0) {
		DL_bdconfig_t	*bd = i596config;

		untimeout(i596timer_id);
		if (i596inetstats && i596ifstats)
			kmem_free(i596ifstats, sizeof(struct ifstats) * i596boards);
		if (bd->bd_dependent1) 
			kmem_free((int *)bd->bd_dependent1, sizeof(bdd_t));
		return (ret_code);
	}
	mod_drvattach(&i596_attach_info);
	return(0);
}

STATIC int
i596_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in i596_unload()");

	mod_drvdetach(&i596_attach_info);
	i596uninit();
	return(0);
}

/*************************************************************************
 * i596init ()
 * Number of boards            	Configuration of boards
 *     	1              	from system configuration parameters
 *   	>1              assume pre-configuration of boards
 */

int
i596init ()
{
	DL_bdconfig_t   *bd;
	DL_sap_t        *sap;
	ushort         	i,k;	
	ushort_t        x,irq;
	ushort         	base_io;
	ulong           ram_start, ram_end;
	bdd_t           *bdd;
	int				abort = 0;

	cmn_err (CE_CONT, "%s %s\n", i596id_string, i596copyright);

	/* Allocate internet stats structure */
	if (i596inetstats)
       	i596ifstats = (struct ifstats *) kmem_zalloc 
                          ((sizeof(struct ifstats) * i596boards), KM_SLEEP);

#ifdef NBPFILTER
	i596ifp = (struct ifnet *) kmem_zalloc 
                          ((sizeof (struct ifnet) * i596boards), KM_SLEEP);
#endif

	bd = i596config;
	base_io = bd->io_start;

	/* initialize flags and mib structure */
	bd->flags = BOARD_PRESENT;
	bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

	i596noactivity_cnt = 0;

	if (i596_enet_in_flash) {
		if (i596get_eaddr(bd) == 0) {
			cmn_err(CE_CONT,"%s enabled. ",i596id_string);
		} else {
			cmn_err(CE_WARN,"%s failed Ethernet address acquisition logic.",
				i596id_string);
			bd->flags = BOARD_DISABLED;
			if (i596inetstats && i596ifstats)
				kmem_free(i596ifstats, sizeof(struct ifstats) * i596boards);
			return(1);
		}
	} else if (i596_station_addr) {
		cmn_err (CE_CONT,"%s enabled. ",i596id_string);
		for (i=0; i<CSMA_LEN; i++)
			bd->eaddr.bytes[i] = inb(i596_station_addr+(i*2));
	} else {
#ifdef MDV_DEBUG
		cmn_err (CE_CONT,"%s MDV enabled. ",i596id_string);
		for (i=0; i<CSMA_LEN; i++)
			bd->eaddr.bytes[i] = i596_default_eaddr[i];
#else
		cmn_err(CE_WARN,"%s not configured for Ethernet address acquisition.",
			 i596id_string);
		bd->flags = BOARD_DISABLED;
		if (i596inetstats && i596ifstats)
				kmem_free(i596ifstats, sizeof(struct ifstats) * i596boards);
		return(1);
#endif
	}

	i596print_eaddr(bd->eaddr.bytes);

	i596timer_id = timeout(i596watch_dog, 0, drv_usectohz(I596_TIMEOUT));

	/* Initialize DLconfig */
	bd->bd_number     = 0;
	bd->sap_ptr       = &i596saps[0];
	bd->tx_next       = 0;
	bd->timer_val     = -1;
	bd->promisc_cnt   = 0;
	bd->multicast_cnt = 0;
	bd->bd_dependent1 = (caddr_t)kmem_alloc(sizeof(bdd_t), KM_SLEEP);
	bdd = (bdd_t *)bd->bd_dependent1;
	for (k = 0; k < MULTI_ADDR_CNT; k++)
		bdd->i596_multiaddr[k].status = 0;
	bd->bd_dependent2 = 0;
	bd->bd_dependent3 = 0;
	bd->bd_dependent4 = 0;
	bd->bd_dependent5 = 0;

	i596intr_to_index[bd->irq_level] = 0;

	/* Initialize SAP structure info */
	for (sap=bd->sap_ptr, x=0; x < (uchar_t)bd->max_saps; x++, sap++) {
		sap->state          = DL_UNBOUND;
		sap->sap_addr       = 0;
		sap->read_q         = NULL;
		sap->write_q        = NULL;
		sap->flags          = 0;
		sap->max_spdu       = USER_MAX_SIZE;
		sap->min_spdu       = USER_MIN_SIZE;
		sap->mac_type       = DL_ETHER;
		sap->service_mode   = DL_CLDLS;
		sap->provider_style = DL_STYLE1;;
		sap->bd		    	= bd;
	}

	/* Initalize internet stat statucture */
	if (i596ifstats) {
		bd->ifstats = &i596ifstats[0];
		i596ifstats[0].ifs_name   = i596_ifname;
		i596ifstats[0].ifs_unit   = (short)0;
		i596ifstats[0].ifs_mtu    = USER_MAX_SIZE;
		i596ifstats[0].ifs_active = 1;
		i596ifstats[0].ifs_next   = ifstats;
		i596ifstats[0].ifs_ipackets = i596ifstats[0].ifs_opackets = 0;
		i596ifstats[0].ifs_ierrors  = i596ifstats[0].ifs_oerrors  = 0;
		i596ifstats[0].ifs_collisions = 0;
		ifstats = &i596ifstats[0];
	}

	bdd->ram_size = MAX_RAM_SIZE;
	bdd->virt_ram = (caddr_t)(((uint)i596_mem + 0xf) & 0xfffffff0);

	outw(i596_port1_addr, (ushort)0);		/* reset the 82596 */
	drv_usecwait(1000);
	outw(i596_port2_addr, (ushort)0);
	drv_usecwait(1000);

	i596debug.reset_count = 0;

	bd->mib.ifAdminStatus = DL_UP;		/* SNMP */

#ifdef NBPFILTER
	if (i596ifp) {
		static struct bpf_devp dev = { DLT_EN10MB, sizeof(DL_ether_hdr_t) };

		i596ifp[ 0 ].if_name   = i596_ifname;
		i596ifp[ 0 ].if_unit   = (short)0;
		i596ifp[ 0 ].if_mtu    = USER_MAX_SIZE;
		i596ifp[ 0 ].if_flags  = IFF_UP;
		i596ifp[ 0 ].if_output = i596bpf_output;
		i596ifp[ 0 ].if_ioctl  = i596bpf_ioctl;
		i596ifp[ 0 ].if_next   = (struct ifnet*) bd;
		i596ifp[ 0 ].if_ctlin  = i596bpf_promisc;

		bpfattach(&bd->bpf, &i596ifp[ 0 ], &dev);
	}
#endif
	return(0);
}

int
i596start ()
{   
	DL_bdconfig_t   *bd;

	bd = i596config;
	if (bd->flags & BOARD_PRESENT) {
		if (i596init_596(bd)) {
			cmn_err(CE_WARN, 
			  "i596init: 82596 did not respond to the initialization process");
			bd->flags = BOARD_DISABLED;
			return(1);
		} else
			bd->mib.ifOperStatus = DL_UP;	/* SNMP */
	}
	return(0);
}

/*************************************************************************
 * i596init_596 ()
 *
 * initialize the 82596's scp, iscp, and scb; reset the 82596;
 * do IA setup command to add the ethernet address to the 82596 IA table;
 * and enable the Receive Unit.
 */

ushort
i596init_596(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd      = (bdd_t *) bd->bd_dependent1;
	ushort	ofst_scp  = (MAX_RAM_SIZE - sizeof(scp_t)) & 0xFFF0; 
	ushort	ofst_iscp = ofst_scp - sizeof(iscp_t);
	ushort	ofst_scb  = ofst_iscp - sizeof(scb_t);
	scp_t	*scp	  = (scp_t *) (bdd->virt_ram + ofst_scp);
	iscp_t	*iscp	  = (iscp_t *)(bdd->virt_ram + ofst_iscp);
	scb_t	*scb	  = (scb_t *) (bdd->virt_ram + ofst_scb);
	ushort	base_io   = bd->io_start;
	paddr_t	scp_paddr;
	int		oldpri;
	int		i;

	oldpri = splhi();
	i596debug.reset_count++;

	outw(i596_port1_addr, (ushort)0);		/* reset the 82596 */
	drv_usecwait(1000);
	outw(i596_port2_addr, (ushort)0);
	drv_usecwait(1000);

	scp_paddr = kvtophys(bdd->virt_ram) + ofst_scp + 2;
	outw(i596_port1_addr, (ushort)(scp_paddr & 0xffff));
	drv_usecwait(1000);
	outw(i596_port2_addr, (ushort)(scp_paddr >> 16));
	drv_usecwait(1000);

	/* fill in scp */
	scp->scp_zeros		= 0;
#ifndef COMPAT_MODE
	scp->scp_sysbus		= CSW_BIT | MODE_32BIT;
#else
	scp->scp_sysbus		= CSW_BIT;
#endif
	scp->scp_unused[0]	= 0;
	scp->scp_unused[1]	= 0;
	scp->scp_iscp_paddr	= kvtophys(bdd->virt_ram) + ofst_iscp;

	/* fill in iscp */
	iscp->iscp_busy 	= 1;
	iscp->iscp_scb_ofst	= ofst_scb;
	iscp->iscp_scb_base	= kvtophys(bdd->virt_ram);

	/* fill in scb */
	scb->scb_status 	= 0;
	scb->scb_cmd 		= 0;
	scb->scb_cbl_ofst 	= 0xffff;
	scb->scb_rfa_ofst 	= 0xffff;
	scb->scb_crc_err	= 0;
	scb->scb_aln_err	= 0;
	scb->scb_rsc_err	= 0;
	scb->scb_ovrn_err	= 0;
#ifndef COMPAT_MODE
	scb->scb_rcdt_err	= 0;
	scb->scb_shrt_err	= 0;
	scb->scb_ton_timer	= 0;
	scb->scb_toff_timer	= 0;
#endif

	bdd->ofst_scb = ofst_scb;
	bdd->gen_cmd = ofst_scb - sizeof(cmd_t);

	/* Only initialize tx/rx on first reset. */
	if (i596debug.reset_count == 1) {
		i596init_tx(bdd);
		i596init_rx(bdd);
	}

	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	/* wait for iscp busy to be cleared */
	for (i=1000; i; i--) {					
		if (!iscp->iscp_busy)
			break;
		drv_usecwait(10);
	}
	if (i == 0)	{		/* if bit isn't cleared */
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: invalid iscp_busy %x\n",
			 iscp->iscp_busy);
#endif
		i596free_rxbufs(bdd);
		return(1);		/* return error */
    } 

	for (i=1000; i; i--) {					/* wait for scb status */
		if (scb->scb_status == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait(10);
	}
	if (i == 0) {							/* if CX & CNA aren't set */
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: invalid scb_status %x\n",
			 scb->scb_status);
#endif
		i596free_rxbufs(bdd);
		return(2);		/* return error */
	}

	if (i596ack_596(scb, base_io)) {
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: ack failed\n");
#endif
		i596free_rxbufs(bdd);
		return (2);
	}

#ifdef DIAG
	/* configure 596 in internal loopback mode */
	if (i596config_596(bd, PRO_OFF, LOOP_ON)) {
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: config loopback failed\n");
#endif
		i596free_rxbufs(bdd);
		return (2);
	}

	if (i596diagnose_596(bd)) {
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: diagnose failed\n");
#endif
		i596free_rxbufs(bdd);
		return (2);
	}
#endif

	/* configure 596 with default parameters */
	if (i596config_596(bd, PRO_OFF, LOOP_OFF)) {
		splx(oldpri);
/*#ifdef DEBUG*/
		cmn_err(CE_WARN,"i596init_596: config failed\n");
/*#endif*/
		i596free_rxbufs(bdd);
		return (2);
	}

	/* do IA Setup command and enable 596 Receive Unit */
	if (i596ia_setup_596(bd, bd->eaddr.bytes)) {
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: ia_setup failed\n");
#endif
		i596free_rxbufs(bdd);
		return (2);
	}

	/* enable 596 Receive Unit */
	scb->scb_status		= 0;
	scb->scb_cmd		= SCB_RUC_STRT;
	scb->scb_rfa_ofst	= (ushort)((char *)bdd->begin_fd - bdd->virt_ram);

	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	if (i596wait_scb(scb, 1000)) {
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: channel attn failed\n");
#endif
		i596free_rxbufs(bdd);
		return(2);
	}

	if (i596ack_596(scb, base_io)) {
		splx(oldpri);
#ifdef DEBUG
		cmn_err(CE_WARN,"i596init_596: ack failed\n");
#endif
		i596free_rxbufs(bdd);
		return(2);
	}

	/* Needed for panther system. */
	if (i596_clr_int_addr) {
		outw(i596_clr_int_addr, 0);		/* write to port clears int */
	}

	splx(oldpri);
	return(0);
}

/*************************************************************************/
STATIC void
i596init_tx (bdd, base_io)
bdd_t    *bdd;
ushort_t base_io;
{
	ushort	ofst_tbd = 0;
	ushort	ofst_cmd = 0;
	ring_t	*ring;
	cmd_t	*cmd;
	tbd_t	*tbd;
	ushort	i, j;

	bdd->n_tbd = i596_n_tbd;
	bdd->n_cmd = i596_n_cmd;

	ofst_tbd = bdd->ofst_tbd = 0;
	ofst_cmd = bdd->ofst_cmd = ofst_tbd + (bdd->n_cmd*bdd->n_tbd*sizeof(tbd_t));

	/* set up the tbds, cmds */
	cmd = (cmd_t *)(bdd->virt_ram + ofst_cmd);
	tbd = (tbd_t *)(bdd->virt_ram + ofst_tbd);

	/* allocate ring buffer as an array of ring_t */
	ring = bdd->ring_buff = 
			(ring_t *)kmem_alloc (bdd->n_cmd*sizeof(ring_t), KM_SLEEP);

	/* initialize cmd, tbd, and ring structures */
	for (i=0; i<bdd->n_cmd; i++, cmd++, tbd++, ring++) {
		/* initialize cmd */
		cmd->cmd_status		= 0;
		cmd->cmd_cmd		= CS_EL;
		cmd->cmd_nxt_ofst	= 0xffff;
		cmd->prmtr.prm_xmit.xmt_tbd_ofst 	= ofst_tbd;
#ifndef COMPAT_MODE
		cmd->prmtr.prm_xmit.xmt_tcb_count 	= 8;	/* Dest addr & length */
		cmd->prmtr.prm_xmit.xmt_reserved 	= 0;
#endif

		/* initialize tbd */
		for (j=0; j<bdd->n_tbd; j++, tbd++) {
			ofst_tbd += sizeof(tbd_t);
			tbd->tbd_count		= 0;
			tbd->tbd_nxt_ofst	= ofst_tbd;
			tbd->tbd_buff		= 0;
			tbd->tbd_buff_base	= 0;
			tbd->sane_buff		= 0;
		}
		(--tbd)->tbd_nxt_ofst	= 0xffff;
		
		ring->ofst_cmd = ofst_cmd;
		ring->next = ring + 1;
		ring->xmit_mp = NULL;
		ofst_cmd += sizeof(cmd_t);
	}

	/* complete ring buffer by making the last next pointer point to first */
	(--ring)->next = bdd->ring_buff;
	bdd->head_cmd = 0;
	bdd->tail_cmd = 0;
}

STATIC mblk_t *
i596allocbuf(size)
int	size;
{
	mblk_t	*mp;

	while ((mp = allocb(size, BPRI_MED)) == NULL ) {
		if (!bufcall(size, BPRI_MED, wakeup, &mp)) {
			timeout(wakeup, (caddr_t)&mp, drv_usectohz(500000));
		}
		if (sleep((caddr_t)&mp, STOPRI|PCATCH)) {
			return(NULL);
		}
	}
	return(mp);
}

/*************************************************************************
 * i596init_rx (bdd, base_io)
 *   retain i596 buffer configuration for receive side
 */

STATIC void
i596init_rx (bdd, base_io)
bdd_t     *bdd;
ushort_t  base_io;
{
	ushort	ofst_rbd = 0;
	ushort	ofst_fd  = 0;
	fd_t	*fd;
	rbd_t	*rbd;
	int		mem_left;
	ushort	i;
	paddr_t	paddr;

	/* rbd's start at the end of transmit data area */
	ofst_rbd = bdd->ofst_rbd = bdd->ofst_cmd + (bdd->n_cmd*sizeof(cmd_t));

	/* calculate number of rbd's and fd's */
	mem_left = bdd->gen_cmd - bdd->ofst_rbd;
	bdd->n_fd = bdd->n_rbd = mem_left/(sizeof(fd_t)+sizeof(rbd_t));
	if (bdd->n_fd > i596_n_fd)
		bdd->n_fd = i596_n_fd;
	if (bdd->n_rbd > i596_n_rbd)
		bdd->n_rbd = i596_n_rbd;

	/* fd's follow rbd's */
	ofst_fd = bdd->ofst_fd = ofst_rbd + (bdd->n_rbd*sizeof(rbd_t));

	fd	= (fd_t *)(bdd->virt_ram + ofst_fd);
	rbd	= (rbd_t *)(bdd->virt_ram + ofst_rbd);

	bdd->begin_fd = fd;

	/* initialize fds */
	for (i=0; i < bdd->n_fd; i++, fd++) {
		ofst_fd 			+= sizeof (fd_t);
		fd->fd_status		= 0;
#ifndef COMPAT_MODE
		fd->fd_cmd			= FLEX_MODE;
#else
		fd->fd_cmd			= 0;
#endif
		fd->fd_nxt_ofst		= ofst_fd;
		fd->fd_rbd_ofst		= 0xffff;
#ifndef COMPAT_MODE
		fd->fd_actual_cnt	= 0;
		fd->fd_size			= 14;		/* Dest/src addr & length */
#endif
	}
	bdd->end_fd		= --fd;

	fd->fd_nxt_ofst	= 0xffff;
#ifndef COMPAT_MODE
	fd->fd_cmd		= FLEX_MODE | CS_EL;
#else
	fd->fd_cmd		= CS_EL;
#endif

	/* init first fd's rbd */
	bdd->begin_fd->fd_rbd_ofst = bdd->ofst_rbd;

	bdd->begin_rbd = rbd;

	/* initialize all rbds */
	for (i=0; i < bdd->n_rbd; i++, rbd++) {
		if (rbd->rx_mp = i596allocbuf(i596_rcvbufsiz+4)) {
			paddr = kvtophys(rbd->rx_mp->b_rptr);
			ofst_rbd			+= sizeof(rbd_t);
			rbd->rbd_status		= 0;
			rbd->rbd_nxt_ofst	= ofst_rbd;
			rbd->rbd_buff		= paddr & 0xffff;
			rbd->rbd_buff_base	= paddr >> 16;
			rbd->rbd_size		= i596_rcvbufsiz;
		}
		else {
			cmn_err(CE_CONT, "i596init_rx: allocated %x of %x rcv buffers\n", i, bdd->n_rbd);
			break;
		}
	}

	if (i==0) {
		cmn_err(CE_WARN, "i596init_rx: could NOT allocate rcv buffers");
		return;
	}

	bdd->end_rbd = --rbd;

	rbd->rbd_nxt_ofst = 0xffff;
	rbd->rbd_size	 |= CS_EL;
}

/*************************************************************************/
ushort
i596diagnose_596(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *)(bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io  = bd->io_start;
	int		i;

	if (i596wait_scb(scb, 1000))
		return (1);

	if (i596ack_596(scb, base_io))
		return (1);

	if (i596wait_scb(scb, 1000))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd	  = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;

	cmd->cmd_status   = 0;
	cmd->cmd_cmd	  = CS_CMD_DGNS | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	for (i=0; i < 100; i++)
	{
		if (cmd->cmd_status & CS_OK)
			break;
		drv_usecwait(10);	/* wait 10 usec */
	}
	if (i == 100) {
		cmn_err(CE_NOTE, "i596diagnose_596: diagnose failed");
		return(1);
	}

	if (i596ack_596(scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************/
ushort
i596config_596 (bd, prm_flag, loop_flag)
DL_bdconfig_t *bd;
ushort_t prm_flag;
ushort_t loop_flag;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *)(bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io = bd->io_start;

	if (i596wait_scb(scb, 1000))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd	  = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;

	cmd->cmd_status   = 0;
	cmd->cmd_cmd	  = CS_CMD_CONF | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

#ifndef COMPAT_MODE
	cmd->prmtr.prm_conf.cnf_fifo_byte = 0xf80e;
	cmd->prmtr.prm_conf.cnf_add_mode  = 0x2600 | (loop_flag?0x4000:0x0000);
	cmd->prmtr.prm_conf.cnf_pri_data  = 0x6000;
	cmd->prmtr.prm_conf.cnf_slot	  = 0xf200;
	cmd->prmtr.prm_conf.cnf_hrdwr	  = 0x0000 | (prm_flag?0x0001:0x0000);
	cmd->prmtr.prm_conf.cnf_min_len	  = 0xff40;
	cmd->prmtr.prm_conf.cnf_dcr_num	  = 0x3f00;
#else
	cmd->prmtr.prm_conf.cnf_fifo_byte = 0x080c;
	cmd->prmtr.prm_conf.cnf_add_mode  = 0x2600 | (loop_flag?0x4000:0x0000);
	cmd->prmtr.prm_conf.cnf_pri_data  = 0x6000;
	cmd->prmtr.prm_conf.cnf_slot	  = 0xf200;
	cmd->prmtr.prm_conf.cnf_hrdwr	  = 0x0000 | (prm_flag?0x0001:0x0000);
	cmd->prmtr.prm_conf.cnf_min_len	  = 0x0040;
#endif

	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	if (i596wait_scb(scb, 1000))
		return (1);

	if (i596ack_596 (scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************/
STATIC int
i596ia_setup_596 (bd, eaddr)
DL_bdconfig_t *bd;
uchar_t eaddr[];
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	scb_t	*scb = (scb_t *)(bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *)(bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io = bd->io_start;
	ushort	i;
	
	if (i596wait_scb(scb, 1000))
		return (1);

	scb->scb_status	  = 0;
	scb->scb_cmd	  = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;
	scb->scb_rfa_ofst = bdd->ofst_fd;
	
	cmd->cmd_status	  = 0;
	cmd->cmd_cmd	  = CS_CMD_IASET | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	for (i=0; i<6; i++)
	    cmd->prmtr.prm_ia_set[i] = eaddr[i];

	outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */

	if (i596wait_scb(scb, 1000))
		return (1);

	if (i596ack_596(scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************
 * i596wait_scb (scb, how_long)
 *
 * Acceptance of a Control Command is indicated by the 82596 clearing
 * the SCB command field page 2-16 of the intel microcom handbook.
 */
ushort
i596wait_scb (scb, n)
scb_t 	*scb;
int		n;
{
	register int i = 0x10;		/* wait every 0x10 times */

	do {
		if (scb->scb_cmd == 0)
			return(0);
		if (--i == 0) {
			drv_usecwait(10);	/* wait 10 usec */
			i = 0x10;
			i596debug.wait_count++;
		}
	} while (n--);
	DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"i596wait_scb: failed"));
	return (1);
}

/*************************************************************************/
int
i596ack_596 (scb, base_io)
scb_t	*scb;
ushort  base_io;
{
	if ((scb->scb_cmd = scb->scb_status & SCB_INT_MSK) != 0) {	
		outw(i596_chan_attn_addr, (ushort)1);			 /* channel attention */
		return (i596wait_scb (scb, 10000));
	}
	return (0);
}

/*************************************************************************/
int
i596_infocmp(a, b, n)
char	*a, *b;
int		n;
{
	do
		if (*a++ != *b++)
			return(*--a - *--b);
	while (--n);
	return(0);
}

/*************************************************************************/
int
i596get_eaddr(bd)
DL_bdconfig_t *bd;
{
	addr_t	virt_INT15;		/* standard INT15 entry in F000 segment */
	Einfo_t	*Einfo;
	uchar_t	slot = 0;
	uchar_t	funct = 0;
	uchar_t	ret_code = 0;
	int		oldpri;
	int		i;

	/* Panther-specific Ethernet address acquisition logic */
	virt_INT15 = (addr_t)physmap(0xF0000, 0xFFFF, KM_SLEEP)+0xF859;
	Einfo = (Einfo_t *)kmem_zalloc(sizeof(Einfo_t), KM_SLEEP);
	oldpri = splhi();
	do {
		i596read_EISA_records(virt_INT15, slot, funct, Einfo, &ret_code);
		if (ret_code == 0) {
			if (i596_infocmp("NET,82596", Einfo->type_subt, 9) == 0) {
				for (i=0; i<CSMA_LEN; i++)
					bd->eaddr.bytes[i] = Einfo->lan_scsi_config.lan_eaddr[i];
				break;
			}	
			funct++;
		}
	} while (ret_code == 0);
	splx(oldpri);
	kmem_free(Einfo, sizeof(Einfo_t));
	physmap_free((virt_INT15-0xF859), 0xFFFF, KM_SLEEP);
	return(ret_code);
}

/*************************************************************************/
STATIC void
i596free_rxbufs(bdd)
bdd_t	*bdd;
{
	rbd_t	*rbd = bdd->begin_rbd;
	ushort	i;

	for (i=0; i < bdd->n_rbd; i++) {
		freeb(rbd->rx_mp);
		rbd = (rbd_t *)(bdd->virt_ram + rbd->rbd_nxt_ofst);
	}
}

/*************************************************************************/
STATIC void
i596uninit()
{
	DL_bdconfig_t	*bd = i596config;

	untimeout(i596timer_id);

	outw(i596_port1_addr, (ushort)0);		/* reset the 82596 */
	drv_usecwait(1000);
	outw(i596_port2_addr, (ushort)0);
	drv_usecwait(1000);

	i596free_rxbufs((bdd_t *)bd->bd_dependent1);

	if (i596inetstats && i596ifstats)
		kmem_free(i596ifstats, sizeof(struct ifstats) * i596boards);
	if (bd->bd_dependent1) 
		kmem_free ((int *)bd->bd_dependent1, sizeof(bdd_t));
}

