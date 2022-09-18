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

#ident	"@(#)uts-x86at:io/dlpi_ether/el16init.c	1.20"
#ident	"$Header: $"

/*
 *  This file contains the initialization of the hardware dependent code
 *  for the 3c507.
 */

#include <io/dlpi_ether/dlpi_el16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/el16.h>
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

extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t		DLsaps[];
extern	int				DLboards;
extern	int				DLinetstats;
extern	int				DLstrlog;

extern	struct	ifstats	*ifstats;
extern	struct	debug	el16debug;
extern	char	*el16_ifname;

/*
 *  Interrupt conversion table.
 */
int el16intr_to_index[] = {
				-1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1 };
int	el16timer_id;
unsigned int	bus_p;
int	slot_num;

struct	ifstats	*el16ifstats;

extern void	el16watch_dog();
STATIC	void	el16tx_done(),  el16init_rx(),
				el16init_tx(), el16idpg();

STATIC	int		el16mem_chk(), el16chk(), 
				el16ia_setup_586(), get_slot();
int el16ack_586();

unsigned	inb ();
void		outb(), el16print_eaddr();

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*el16ifp;
extern 	int		el16bpf_ioctl(), el16bpf_output(), el16bpf_promisc();
extern	void	bpfattach();
#endif

extern int el16cable_type;
extern int el16saad;
extern int el16zws;

char	el16id_string[] = "EL-16 (3C507) v1.1";
char	el16copyright[] = "Copyright (c) 1991 Intel Corp., All Rights Reserved";
char	elmcid_string[] = "EL-16 (3C523) v1.1";
char	elmccopyright[] = "Copyright (c) 1991 UNIX System Laboratories Inc., All Rights Reserved";

/*
 * Header, wrapper, and function declarations and definitions for loadability
 */

#include <util/mod/moddefs.h>
#define DRVNAME		"el16 - Loadable el16 ethernet driver"

STATIC	int	el16_load(), el16_unload();

MOD_DRV_WRAPPER(el16, el16_load, el16_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

int el16init();
STATIC void el16uninit();

STATIC int
el16_load(void)
{
	int ret_code;

	cmn_err(CE_NOTE, "!MOD: in el16_load()");

	if ((ret_code = el16init()) != 0)  {
		el16uninit();
		return(ret_code);
	}
	mod_drvattach(&el16_attach_info);
	return(0);
}

STATIC int
el16_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in el16_unload()");

	mod_drvdetach(&el16_attach_info);
	el16uninit();
	return(0);
}

/*
 * el16init ()
 * Number of boards		Configuration of boards
 * ----------------		-----------------------
 *  	1			from system config parameters
 *	>1			assume pre-configuration of boards
 */

int
el16init ()
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	DL_sap_t		*sap;
	int			i, j, k;	
	uchar_t			irq;
	char			x;
	int				base_io;
	ulong			ram_start, ram_end;
	bdd_t			*p;
	char	pos_reg;	/* contents of 3C523 POS register */
	unsigned option;		/* temp for POS information */
	int	ret_code;
	int	err_flg;

	if ((drv_gethardware(IOBUS_TYPE, &bus_p)) < 0) {
		cmn_err(CE_WARN,"el16: can't decide bus type");
		return (2);
	}
	if (bus_p & BUS_MCA) {
		cmn_err (CE_CONT, "%s\n", elmcid_string);
		cmn_err (CE_CONT, "%s\n", elmccopyright);
		cmn_err (CE_CONT, "%s\n", el16copyright);
	}
	else
		cmn_err (CE_CONT, "%s %s\n", el16id_string, el16copyright);

	/* Allocate internet stats structure */
	if (el16inetstats)
		el16ifstats = (struct ifstats *) kmem_zalloc ((sizeof(struct ifstats) * el16boards), KM_SLEEP);

#ifdef NBPFILTER
	el16ifp = (struct ifnet *) kmem_zalloc ((sizeof (struct ifnet) * el16boards), KM_SLEEP);
#endif

	/*
	 * If there's 1 board, we can configure its IO base address.
	 */
	if (!(bus_p & BUS_MCA)) {
		if (el16boards < 2) {
			base_io = el16config->io_start;
	
			/* configure IO base address */
			outb (EL16_PORT, 0);		/* set to START state */
			el16idpg ();				/* change to RESET state */
			el16idpg ();				/* change to IOLOAD state */

			/*
		 	 * base_io = (value) x 10H + 200H. --> value = (base_io - 200H) / 10H
		 	 * Look at EtherLink 16 Technical Reference Manual page 2-5.
		 	 */
			outb (EL16_PORT, base_io-0x200 >> 4);	/* load IO Base address */

			/* configure RAM base address, SAD, and 0WS */
			ram_start = el16config->mem_start;
			ram_end = el16config->mem_end;

			if ((x = el16mem_chk (ram_start, ram_end)) == -1) {
				cmn_err (CE_WARN, "RAM base address configuration: 0x%x - 0x%x NOT supported", ram_start, ram_end);
				return (126);
			}
			if (el16saad)
				x |= EL16_SAD;
			if (el16zws)
				x |= EL16_ZWS;
	
			outb (base_io+RAM_CFG_OFST, x);

			/* configure interrupt request level */
			switch (irq = (uchar_t) el16config->irq_level) {
			case 3: case 5: case 7: case 9: case 10: case 11: case 12: case 15:
				break;
			default:
				cmn_err (CE_WARN, "Unsupported interrupt level %d selected", irq);
				return (126);
			}
			outb (base_io+INT_CFG_OFST, irq);

			/* configure cable type */
			x = inb (base_io+ROM_CFG_OFST);

			if (el16cable_type)			/* thick ethernet */
				x &= ~EL16_BNC;
			else						/* thin ethernet */
				x |= EL16_BNC;

			outb (base_io+ROM_CFG_OFST, x);

			/* transition the adapter to RUN state */
			outb (EL16_PORT, 0);
			el16idpg ();
			outb (EL16_PORT, 0);

		}	/* if one board */

		else {	/* more than one board */
			outb (EL16_PORT, 0);		/* set to START state */
			el16idpg ();				/* change to RESET state */
			outb (EL16_PORT, 0);		/* change to RUN state */
		}
	}	/* not MCA */	

	/*
	 *  Initialize the configured boards.
	 */

	slot_num = 0;
	err_flg = 0;
	for (i=0, bd=el16config; i<el16boards; bd++, i++) {
		if (bus_p & BUS_MCA) {
			if ((pos_reg = get_slot(MC_ID, bd)) != -1) { 
				/* configure RAM base addr */
				option = (unsigned)pos_reg;
				option = (option>>3)&3;
				switch(option){
				case	0: option = 0xc0000; break;
				case	1: option = 0xc8000; break;
				case	2: option = 0xd0000; break;
				case	3: option = 0xd8000; break;
				}
				bd->mem_start = (paddr_t)option;
				bd->mem_end = bd->mem_start + 0x4000 - 1;

				/* configure I/O base addr */
				option = (unsigned)pos_reg;
				option = (option >> 1)&3; /* io base in bits 1,2 */
				switch(option){
				case	0:	option = 0x300; break;
				case	1:	option = 0x1300; break;
				case	2:	option = 0x2300; break;
				case	3:	option = 0x3300; break;
				}
				bd->io_start = (ulong_t)option;
				bd->io_end = bd->io_start + 7;
				
			}
			else {
				cmn_err(CE_WARN,"el16: Board not present");
				return (6);
			}
			bd->flags = BOARD_PRESENT;
			ret_code = 0;
		} /* if MCA */
		else
			bd->flags = 0;

		base_io = bd->io_start;

		/* initialize flags and mib structure */
		bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

		if (!(bus_p & BUS_MCA)) {
		 	/* Check the board's configuration */
			if ((x = el16chk (bd)) == 1) {
				cmn_err (CE_WARN, "%s board %d at base address 0x%x was NOT found.", el16id_string, i, bd->io_start);
				err_flg = 1;
				continue;
			} else if (x == 2) {
				cmn_err (CE_WARN, "%s board %d configuration does NOT match the system's configuration", el16id_string, i);
				bd->flags = BOARD_PRESENT;
				err_flg = 1;
				continue;
			} else {
				cmn_err (CE_CONT,"%s board %d ",el16id_string,i);
				if (inb (base_io+ROM_CFG_OFST) & C_TYPE_MSK)
					cmn_err (CE_CONT, "(BNC)");
				else
					cmn_err (CE_CONT, "(AUI)");
				bd->flags = BOARD_PRESENT;
				ret_code = 0;

				/* store and print ethernet address */
				outb (base_io + CTRL_REG_OFST, CR_VB1);
			}
		}	/* if not MCA */

		for (j=0; j<CSMA_LEN; j++)
			bd->eaddr.bytes[j] = inb (base_io+j);

		DLprint_eaddr (bd->eaddr.bytes);

		/*
		 * timeout every 5 seconds
		 */
		el16timer_id = timeout (el16watch_dog, 0, drv_usectohz (EL16_TIMEOUT));

		/*
		 *  Initialize DLconfig.
		 */
		bd->bd_number     = i;
		bd->sap_ptr       = &el16saps[ i * bd->max_saps ];
		bd->tx_next       = 0;
		bd->timer_val     = -1;
		bd->promisc_cnt   = 0;
		bd->multicast_cnt = 0;
		bd->bd_dependent1 = (caddr_t) kmem_alloc (sizeof (bdd_t), KM_SLEEP);
		bdd = (bdd_t *)bd->bd_dependent1;
		for (k = 0; k < MULTI_ADDR_CNT; k++ )
			bdd->el16_multiaddr[k].status = 0;
		bd->bd_dependent2 = 0;
		bd->bd_dependent3 = 0;
		bd->bd_dependent4 = 0;
		bd->bd_dependent5 = 0;

		el16intr_to_index [bd->irq_level] = i;

		/*
		 *  Initialize SAP structure info.
		 */
		for (sap=bd->sap_ptr, j=0; j<bd->max_saps; j++, sap++) {
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
			sap->bd		    = bd;
		}

		/*
		 *  Initalize internet stat statucture.
		 */
		if (el16ifstats) {
			bd->ifstats = &el16ifstats[ i ];
			el16ifstats[ i ].ifs_name   = el16_ifname;
			el16ifstats[ i ].ifs_unit   = (short)i;
			el16ifstats[ i ].ifs_mtu    = USER_MAX_SIZE;
			el16ifstats[ i ].ifs_active = 1;
			el16ifstats[ i ].ifs_next   = ifstats;
			el16ifstats[i].ifs_ipackets = el16ifstats[i].ifs_opackets = 0;
			el16ifstats[i].ifs_ierrors  = el16ifstats[i].ifs_oerrors  = 0;
			el16ifstats[i].ifs_collisions = 0;
			ifstats = &el16ifstats[ i ];
		}
		if (bus_p & BUS_MCA)
			outb (base_io + CTRL_REG_OFST, CR_BS);
		p = (bdd_t *) bd->bd_dependent1;
		p->ram_size = bd->mem_end - bd->mem_start + 1;
		p->virt_ram = (addr_t) physmap (bd->mem_start, p->ram_size, KM_SLEEP);
		p->virt_ram -= (MAX_RAM_SIZE - p->ram_size);

		bd->mib.ifAdminStatus = DL_UP;

		if (el16init_586 (bd)) {
			cmn_err(CE_WARN, "el16init: the 82586 did not respond to the initialization process");
			bd->flags = BOARD_DISABLED;
			ret_code = 14;
		}
		else {
			bd->mib.ifOperStatus = DL_UP;
			ret_code = 0;
		}

#ifdef NBPFILTER
		if (el16ifp) {
			static struct bpf_devp dev =
				{ DLT_EN10MB, sizeof(DL_ether_hdr_t) };

			el16ifp[ i ].if_name   = el16_ifname;
			el16ifp[ i ].if_unit   = (short)i;
			el16ifp[ i ].if_mtu    = USER_MAX_SIZE;
			el16ifp[ i ].if_flags  = IFF_UP;
			el16ifp[ i ].if_output = el16bpf_output;
			el16ifp[ i ].if_ioctl  = el16bpf_ioctl;
			el16ifp[ i ].if_next   = (struct ifnet*) bd;
			el16ifp[ i ].if_ctlin  = el16bpf_promisc;

			bpfattach(&bd->bpf, &el16ifp[ i ], &dev);
		}
#endif
		if (!(bus_p & BUS_MCA))
			outb(base_io + CLEAR_INTR, 1);
	}
	if (err_flg != 0)
		ret_code = 6;
	return (ret_code);
}

/*
 *  el16chk()
 *
 *  Checks whether the 3COM's signature exists and match the software
 *  RAM base address to the hardware's.
 */

STATIC
el16chk (bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	char	sign[6];
	uchar_t	i, x;

	/*
	 * check for 3Com's signature: ensure VB0 and VB1 (in CR) are 0
	 */
	x = inb(base_io + CTRL_REG_OFST);
	if (x & 0x03)
		outb(base_io + CTRL_REG_OFST, x & 0xfc);

	for (i=0; i<CSMA_LEN; i++)
		sign[i] = (char) inb (base_io+i);
	
	if (strncmp (sign, "*3COM*", CSMA_LEN))
		return (1);

	/*
	 * check for RAM base address configuration
	 */
	x = inb (base_io+RAM_CFG_OFST) & 0x3F;

	if (x != (el16mem_chk (bd->mem_start,bd->mem_end) & 0x3F)) {
		cmn_err (CE_WARN, "Mismatch RAM base address settings");
		return (2);
	}
	
	/*
	 * check for Interrupt Request level configuration
	 */
	x = inb (base_io+INT_CFG_OFST) & 0x0F;

	if (x != bd->irq_level) {
		cmn_err (CE_WARN, "Mismatch Interrupt Request level settings");
		return (2);
	}
	return (0);
}

/*
 * el16mem_chk
 *
 * checks RAM base address settings. If NOT supported return -1, else
 * returns the RAM configuration register settings. Look at the EL-16
 * Technical Reference Manual, page 2-10 for the RAM config registers
 * settings.
 */

STATIC
el16mem_chk (start, end)
ulong start, end;
{
	ulong	size = end - start + 1;
	ushort	ram_cfg = (start>>12) & 0x18;

	switch (start) {
	case 0xC0000: case 0xC8000: case 0xD0000:
		switch (size) {
		case 0x4000: case 0x8000: case 0xC000: case 0x10000:
			ram_cfg |= (size>>14) - 1;
			break;
		default:
			return (-1);
		}
		break;

	case 0xD8000:
		switch (size) {
		case 0x4000: case 0x8000:
			ram_cfg |= (size>>14) - 1;
			break;
		default:
			return (-1);
		}
		break;

	case 0xF00000: case 0xF20000: case 0xF40000: case 0xF60000:
		if (size == 0x10000)
			ram_cfg = 0x30 | (start>>17 & 0x03);
		else
			return (-1);
		break;

	case 0xF80000:
		if (size == 0x10000)
			ram_cfg = 0x38;
		else
			return (-1);
		break;

	default:
		return (-1);
	}
	return (ram_cfg);
}

/*
 * el16init_586 ()
 *
 * initialize the 82586's scp, iscp, and scb; reset the 82586;
 * do IA setup command to add the ethernet address to the 82586 IA table;
 * and enable the Receive Unit.
 */

int
el16init_586 (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd      = (bdd_t *) bd->bd_dependent1;
	ushort	ofst_scp  = MAX_RAM_SIZE - sizeof (scp_t);
	ushort	ofst_iscp = ofst_scp - sizeof (iscp_t);
	ushort	ofst_scb  = ofst_iscp - sizeof (scb_t);

	scp_t	*scp  = (scp_t *) (bdd->virt_ram + ofst_scp);
	iscp_t	*iscp = (iscp_t *)(bdd->virt_ram + ofst_iscp);
	scb_t	*scb  = (scb_t *) (bdd->virt_ram + ofst_scb);

	ushort	base_io  = bd->io_start;
	int		i;

	el16debug.reset_count++;

	/* fill in scp */

	scp->scp_sysbus    = 0;
	scp->scp_iscp      = ofst_iscp;
	scp->scp_iscp_base = 0;

	/* fill in iscp */

	iscp->iscp_busy     = 1;
	iscp->iscp_scb_ofst = ofst_scb;
	iscp->iscp_scb_base = 0;

	/* fill in scb */

	scb->scb_status   = 0;
	scb->scb_cmd      = 0;
	scb->scb_cbl_ofst = 0xffff;
	scb->scb_rfa_ofst = 0xffff;
	scb->scb_crc_err  = 0;
	scb->scb_aln_err  = 0;
	scb->scb_rsc_err  = 0;
	scb->scb_ovrn_err = 0;

	bdd->ofst_scb = ofst_scb;
	bdd->gen_cmd = ofst_scb - sizeof (cmd_t);

	el16init_tx (bdd);
	el16init_rx (bdd);
	
	/*
	 * start the 82586, by resetting the 586 then issuing a channel
	 * attention
	 */
	outb (base_io + CTRL_REG_OFST, 0);				
	outb (base_io + CTRL_REG_OFST, CR_RST);				
	if (bus_p & BUS_MCA) {
		outb (base_io + CTRL_REG_OFST, CR_RST|CR_CA|CR_BS);			/* channel attention */
		outb (base_io + CTRL_REG_OFST, CR_RST|CR_BS);		/* deassert channel attention */
	}
	else
		outb (base_io + CHAN_ATN_OFST, 1);	/* channel attention */

	for (i=1000; i; i--) {					/* wait for iscp busy */
		if (!iscp->iscp_busy)				/* to be cleared      */
			break;
		drv_usecwait (10);
	}
	if (i == 0)								/* if bit isn't cleared */
		return (1);							/* return error */

	for (i=1000; i; i--) {					/* wait for scb status */
		if (scb->scb_status == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait (10);
	}
	if (i == 0)								/* if CX & CNA aren't set */
		return (2);							/* return error */

	if (el16ack_586 (scb, base_io))
		return (2);

	/* configure 586 */
	if (el16config_586(bd, PRO_OFF))
		return(2);

	/*
	 * do IA Setup command and enable 586 Receive Unit
	 */
	if (el16ia_setup_586 (bd, bd->eaddr.bytes))
		return (2);

	/*
	 * enable 586 Receive Unit
	 */
	scb->scb_status   = 0;
	scb->scb_cmd      = SCB_RUC_STRT;
	scb->scb_rfa_ofst = bdd->ofst_fd;

	if (bus_p & BUS_MCA) {
		outb (base_io + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS);
		outb (base_io + CTRL_REG_OFST, CR_RST|CR_BS);
	}
	else
		outb (base_io + CHAN_ATN_OFST, 1);

	if (el16wait_scb (scb, 1000))
		return (2);

	if (el16ack_586 (scb, base_io))
		return (2);

	/*
	 * enable interrupt
	 */
	if (bus_p & BUS_MCA)
		outb (base_io + CTRL_REG_OFST, CR_RST | CR_IEN | CR_BS);
	else {
		outb(base_io + CLEAR_INTR, 1);
		outb (base_io + CTRL_REG_OFST, CR_RST | CR_IEN);
		outb(base_io + CLEAR_INTR, 1);
    }	
	return (0);
}

/*
 * el16init_tx (bdd)
 */

STATIC void
el16init_tx (bdd)
bdd_t *bdd;
{
	ushort	ofst_txb = 0;
	ushort	ofst_tbd = 0;
	ushort	ofst_cmd = 0;

	ring_t	*ring;
	cmd_t	*cmd;
	tbd_t	*tbd;

	ushort	i, j;

	/* figure out # of tbd, cb and where they are in RAM */

	switch (bdd->ram_size) {
	case 0x4000: case 0x8000:	bdd->n_tbd = 1;	bdd->n_cmd = 3;	break;
	case 0xC000: case 0x10000:	bdd->n_tbd = 1;	bdd->n_cmd = 6;	break;
	}

	/* init current tbd buffer and tbd offset */

	ofst_txb = bdd->ofst_txb = MAX_RAM_SIZE - bdd->ram_size;
	ofst_tbd = bdd->ofst_tbd = ofst_txb + (bdd->n_tbd*bdd->n_cmd*TBD_BUF_SIZ);
	ofst_cmd = bdd->ofst_cmd = ofst_tbd + (bdd->n_tbd*bdd->n_cmd*sizeof(tbd_t));

	/* set up tbds, cmds */

	cmd = (cmd_t *) (bdd->virt_ram + ofst_cmd);
	tbd = (tbd_t *) (bdd->virt_ram + ofst_tbd);

	/* allocate ring buffer */

	ring = bdd->ring_buff = (ring_t *) kmem_alloc (bdd->n_cmd*sizeof(bdd_t), KM_SLEEP);

	/* initialize cmd, tbd, and ring structures */

	for (i=0; i<bdd->n_cmd; i++, cmd++, ring++) {
		cmd->cmd_status   = 0;
		cmd->cmd_cmd      = CS_EL;
		cmd->cmd_nxt_ofst = 0xffff;
		cmd->prmtr.prm_xmit.xmt_tbd_ofst = ofst_tbd;

		for (j=0; j<bdd->n_tbd; j++, tbd++) {
			ofst_tbd += sizeof (tbd_t);
			tbd->tbd_count     = 0;
			tbd->tbd_nxt_ofst  = 0xffff;
			tbd->tbd_buff      = ofst_txb;
			tbd->tbd_buff_base = 0;
			ofst_txb += TBD_BUF_SIZ;
		}
		ring->ofst_cmd = ofst_cmd;
		ring->next = ring + 1;
		ofst_cmd += sizeof (cmd_t);
	}

	/*
	 * complete ring buffer by making the last next pointer points to the first
	 */

	(--ring)->next = bdd->ring_buff;

	bdd->head_cmd = 0;
	bdd->tail_cmd = 0;
}

/*
 * el16init_rx (bdd)
 */

STATIC void
el16init_rx (bdd)
bdd_t *bdd;
{
	fd_t	*fd;
	rbd_t	*rbd;

	ushort	ofst_rxb = 0;
	ushort	ofst_rbd = 0;
	ushort	ofst_fd  = 0;

	int		mem_left;
	ushort	i;

	/* figure out receive buffer offset, how much memory that can be used */

	ofst_rxb = bdd->ofst_rxb = bdd->ofst_cmd + bdd->n_cmd*sizeof(cmd_t);
	mem_left = bdd->ofst_scb - bdd->ofst_rxb - sizeof (cmd_t);

	/* calculate number of rbd and fd we can have */

	bdd->n_rbd = bdd->n_fd = mem_left/(sizeof(fd_t)+sizeof(rbd_t)+RBD_BUF_SIZ);

	ofst_rbd = bdd->ofst_rbd = ofst_rxb + bdd->n_rbd*RBD_BUF_SIZ;
	ofst_fd  = bdd->ofst_fd  = ofst_rbd + bdd->n_rbd*sizeof(rbd_t);

	/* initialize fd, rbd, begin_fd, end_fd pointers */

	fd  = (fd_t *)  (bdd->virt_ram + ofst_fd);
	rbd = (rbd_t *) (bdd->virt_ram + ofst_rbd);

	bdd->begin_fd = fd;
	bdd->end_fd   = fd + bdd->n_fd - 1;

	bdd->begin_rbd = rbd;
	bdd->end_rbd   = rbd + bdd->n_rbd - 1;

	/*
	 * initialize all fds
	 */
	for (i=0; i<bdd->n_fd; i++, fd++) {
		fd->fd_status = 0;
		fd->fd_cmd    = 0;
		ofst_fd += sizeof (fd_t);
		fd->fd_nxt_ofst = ofst_fd;
		fd->fd_rbd_ofst = 0xffff;		
	}
	bdd->begin_fd->fd_rbd_ofst = bdd->ofst_rbd;	/* init first fd's rbd */

	(--fd)->fd_nxt_ofst = 0xffff;				/* init last fd */
	fd->fd_cmd          = CS_EL;

	/*
	 * initialize all rbds
	 */
	for (i=0; i<bdd->n_rbd; i++, rbd++) {
		rbd->rbd_status    = 0;
		ofst_rbd += sizeof (rbd_t);
		rbd->rbd_nxt_ofst  = ofst_rbd;
		rbd->rbd_buff      = ofst_rxb;
		ofst_rxb += RBD_BUF_SIZ;
		rbd->rbd_buff_base = 0;
		rbd->rbd_size      = RBD_BUF_SIZ;
	}

	(--rbd)->rbd_nxt_ofst  = 0xffff;	/* last rbd points to ground */
	rbd->rbd_size         |= CS_EL;		/* eof on the last rbd */
}

/*
 * el16config_586
 */

int
el16config_586 (bd, prm_flag)
DL_bdconfig_t *bd;
ushort_t prm_flag;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	scb_t	*scb = (scb_t *) (bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *) (bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io = bd->io_start;

	if (el16wait_scb (scb, 1000))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd      = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;				/* ofst is general cmd */

	cmd->cmd_status   = 0;
	cmd->cmd_cmd      = CS_CMD_CONF | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	/*
	 * default config page 2-28 586 book
	 */
	cmd->prmtr.prm_conf.cnf_fifo_byte = 0x080c;
	cmd->prmtr.prm_conf.cnf_add_mode  = 0x2600;
	cmd->prmtr.prm_conf.cnf_pri_data  = 0x6000;
	cmd->prmtr.prm_conf.cnf_slot      = 0xf200;
	cmd->prmtr.prm_conf.cnf_hrdwr     = (0x0000 | prm_flag);
	cmd->prmtr.prm_conf.cnf_min_len   = 0x0040;

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

/*
 * el16ia_setup_586 (bd, addr)
 */

el16ia_setup_586 (bd, eaddr)
DL_bdconfig_t *bd;
uchar_t eaddr[];
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	scb_t	*scb = (scb_t *) (bdd->virt_ram + bdd->ofst_scb);
	cmd_t	*cmd = (cmd_t *) (bdd->virt_ram + bdd->gen_cmd);
	ushort	base_io = bd->io_start;
	int		i;
	
	if (el16wait_scb (scb, 1000))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd      = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->gen_cmd;				/* ofst is general cmd */
	scb->scb_rfa_ofst = bdd->ofst_fd;
	
	cmd = (cmd_t *) (bdd->virt_ram + bdd->gen_cmd);
	cmd->cmd_status   = 0;
	cmd->cmd_cmd      = CS_CMD_IASET | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	for (i=0; i<6; i++)
		cmd->prmtr.prm_ia_set[i] = eaddr[i];

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

/*
 * el16idpg ()
 *
 * generate ID pattern and write the pattern to the adapter
 */

STATIC void
el16idpg ()
{
	uchar_t id = 0xff;
	uchar_t count = 0xff;

	while (count--) {
		outb (EL16_PORT, id);		/* write to ID port */

		if (id & 0x80) {			/* if MSB is 1 */
			id <<= 1;
			id ^= 0xe7;
		}
		else {						/* else */
			id <<= 1;
		}
	}
}

/*
 * el16wait_scb (scb, how_long)
 *
 * Acceptance of a Control Command is indicated by the 82586 clearing
 * the SCB command field page 2-16 of the intel microcom handbook.
 */

el16wait_scb (scb, n)
scb_t	*scb;
int		n;
{
	int i = 0;

	while (i++ != n * 1000) {
		if (scb->scb_cmd == 0)
			return (0);
	}
	return (1);
}

/*
 * el16ack_586 (scb, base_io)
 */

int
el16ack_586 (scb, base_io)
scb_t *scb;
ushort base_io;
{
	if ((scb->scb_cmd = scb->scb_status & SCB_INT_MSK) != 0) {
		if (bus_p & BUS_MCA) {
			outb (base_io + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS);		/* channel attention */
			outb (base_io + CTRL_REG_OFST, CR_RST|CR_BS);		
		}
		else
			outb (base_io + CHAN_ATN_OFST, 1);		/* channel attention */
		return (el16wait_scb (scb, 10000));
	}
	return (0);
}

/*
 * This code gets called only if we're running on a PS/2 system. 
 * get_slot looks through the adapters which are present on the bus
 * and sees if a board exists with the adapter id passed to the routine
 * If no board is found, -1 is returned otherwise, the the POS reg 
 * information in pos_regs.
 */

STATIC int
get_slot (id, bd)
unsigned id;	/* Adapter id */
DL_bdconfig_t *bd;
{
     unsigned char 	slot;
     unsigned char 	id_tmp;
     unsigned 		f_id;
     unsigned char	pos_reg;

     slot = slot_num;	/* Start looking at slot 0 (1) */

     /* Turn on the Adapter setup bit */
     slot |= 0x8;

     for ( ; slot_num < 8; slot_num++) {	/* Check all 8 slots */
	  outb (ADAP_ENAB,slot);		/* Select adapter by slot */
	  f_id = (unsigned char) inb (POS_MSB);	
	  f_id = f_id << 8;
	  id_tmp = inb (POS_LSB);			
	  f_id |= id_tmp;

	  if (f_id == id) {
		/* adapter found, get the POS info */
		pos_reg = inb(POS_0);
		
		/* enable the card if not yet */
		if (!(pos_reg & 1)) {
			pos_reg |= 1;
			outb (POS_0, pos_reg);
		}
		switch ((uchar_t) bd->irq_level) {
			case 12: outb(POS_1, 0x01); break;
			case  7: outb(POS_1, 0x02); break;
			case  3: outb(POS_1, 0x04); break;
			case  9: outb(POS_1, 0x08); break;
			default: cmn_err (CE_WARN, "Unsupported interrupt level %d selected", bd->irq_level);
				return(-1);
		}
				
		/* configure cable type */
		cmn_err (CE_CONT,"%s board in slot %d ",elmcid_string,slot_num+1);
		if (el16cable_type) {
			pos_reg |= 0x20;	/* DIX */
			outb (POS_0, pos_reg);
			cmn_err (CE_CONT, "(DIX)");
		}
		else {
			pos_reg &= ~0x20;	/* BNC */
			outb (POS_0, pos_reg);
			cmn_err (CE_CONT, "(BNC)");
		}
		cmn_err (CE_CONT, " was found. \n");

	  	outb (ADAP_ENAB,0); 	/* de_select adapter */
		slot_num++;
	  	return (pos_reg);
	  }
	  slot++;
     }
     outb (ADAP_ENAB,0);	/* de_select adapter */
     return (-1);			/* No adapter found */
}

STATIC void
el16uninit()
{
	DL_bdconfig_t	*bd;
	int i;

	if (el16inetstats)
		if (el16ifstats)
			kmem_free (el16ifstats, sizeof(struct ifstats) * el16boards);
	for (i=0, bd=el16config; i < el16boards; bd++, i++)
		if (bd->bd_dependent1) 
			kmem_free ((int *)bd->bd_dependent1, sizeof(bdd_t));
	untimeout (el16timer_id);
}
