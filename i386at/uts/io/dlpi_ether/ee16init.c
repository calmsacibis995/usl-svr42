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

static char prog_copyright[] = "Copyright 1991 Intel Corp. 468802-010";

#ident	"@(#)uts-x86at:io/dlpi_ether/ee16init.c	1.14"
#ident	"$Header: $"

/* hardware dependent code for ee16 */

#include <io/dlpi_ether/dlpi_ee16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/ee16.h>
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
extern	struct  debug   ee16debug;
extern	char            *ee16_ifname;

/* Check for validity of interrupts */
int ee16intr_to_index[] = {-1,-1,-1,-1,-1,-1,-1,-1,
                           -1,-1,-1,-1,-1,-1,-1,-1};
int	ee16timer_id;

struct	ifstats	*ee16ifstats;

STATIC void     ee16tx_done(), ee16init_rx(), ee16init_tx();
STATIC void     shift_bits_out();
ushort   		ee16config_586(), read_eeprom();
STATIC ushort_t shift_bits_in(), write_eeprom();
STATIC void     raise_clock(), lower_clock(), eeprom_cleanup();

STATIC int      ee16chk(),ee16ia_setup_586();
int		ee16ack_586();

ushort		    ee16wait_scb();
ushort		    ee16init_586();

void			bcopy_to_buffer();
void            ee16watch_dog();
STATIC int	mem_ofst;

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*ee16ifp;
extern 	int     ee16bpf_ioctl(), ee16bpf_output(), ee16bpf_promisc();
extern	void    bpfattach();
#endif

extern int ee16cable_type;

char	ee16id_string[] = "EE16 v1.1";
char	ee16copyright[] = "Copyright (c) 1991 Intel Corp., All Rights Reserved";

/*
 * Header, wrapper, and function declarations and definitions for loadability
 */

#include <util/mod/moddefs.h>
#define DRVNAME		"ee16 - Loadable ee16 ethernet driver"

STATIC	int	ee16_load(), ee16_unload();

MOD_DRV_WRAPPER(ee16, ee16_load, ee16_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

int ee16init();
STATIC void ee16uninit();

STATIC int
ee16_load(void)
{
	int ret_code;

	cmn_err(CE_NOTE, "!MOD: in ee16_load()");

	if ((ret_code = ee16init()) != 0) {
		ee16uninit();
		return (ret_code);
	}
	mod_drvattach(&ee16_attach_info);
	return(0);
}

STATIC int
ee16_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in ee16_unload()");

	mod_drvdetach(&ee16_attach_info);
	ee16uninit();
	return(0);
}

/*************************************************************************
 * ee16init ()
 * Number of boards            	Configuration of boards
 *     	1              	from system configuration parameters
 *   	>1              assume pre-configuration of boards
 */

int
ee16init ()
{   
	DL_bdconfig_t   *bd;
	DL_sap_t        *sap;
	ushort         	i,j,k;	
	ushort_t        x,irq;
	ushort         	base_io;
	ulong           ram_start, ram_end;
	bdd_t           *p;
	int		ret_code;
	bdd_t	*bdd;

	cmn_err (CE_CONT, "%s %s\n", ee16id_string, ee16copyright);

	/* Allocate internet stats structure */
	if (ee16inetstats)
       	ee16ifstats = (struct ifstats *) kmem_zalloc 
                          ((sizeof(struct ifstats) * ee16boards), KM_SLEEP);

#ifdef NBPFILTER
	ee16ifp = (struct ifnet *) kmem_zalloc 
                          ((sizeof (struct ifnet) * ee16boards), KM_SLEEP);
#endif

	/* If there's only 1 board, configure from system parameters */
	if (ee16boards == 1) {
		ushort	cksum;

	    base_io = ee16config->io_start;

		/* reset ASIC on board */
		outb(base_io + EE_CTRL, RESET_586);
		drv_usecwait(500);
		outb(base_io + EE_CTRL, RESET_586 | GA_RESET);
		drv_usecwait(500);
		outb(base_io + EE_CTRL, RESET_586);

	    /* RESET_586 is set at power_on. It needs to be explicitly reset 
		 * when done w/ configuring EEPROM. Configure IO base address: write 
		 * to BASE_IO_REG of EEPROM. The equations below are to map the
		 * base_io to the format required by the EEPROM
		 */
		if (base_io <0x300)
		   	j = 7 - ((ushort) (base_io - 0x200) / (ushort) 16);
		else 
		   	j = 0x0f - ((ushort) (base_io - 0x300) / (ushort) 16);

		/* disable boot eprom , set 16 bit, set IOCHRDY late */
		j |= 0x0c10;
		if (!ee16cable_type) 
			j |= 0x1000;

		/* Need not program eeprom if base_io is same */
		if (!ee16chk(base_io)) {
			/* find out the configured base io address */
			ushort bio;
			for (bio = 0x200; bio <= 0x3f0; bio += 0x10)
				if (ee16chk(bio))
					break;
	    		if (!write_eeprom(bio, BASE_IO_REG, j))
				cmn_err(CE_WARN,"write to eeprom failed\n");

			/* reset board */
			outb(bio + EE_CTRL, RESET_586);
			drv_usecwait(300);
			outb(bio + EE_CTRL, RESET_586 | GA_RESET);
			drv_usecwait(300);
			outb(bio + EE_CTRL, RESET_586);
			drv_usecwait(300);
			if (!ee16chk(base_io)) {
				cmn_err(CE_CONT,"\n\n");
				cmn_err(CE_NOTE,"EE16: Board not found\n");
				return (6);
			}
		} else {

			/* may need to write cable type into eeprom */
		    	i = read_eeprom(base_io, BASE_IO_REG);
			if (ee16cable_type && !(i & AUI_BIT))
				/* AUI & eeprom pgmed as AUI: do nothing */
				;
			else if (!ee16cable_type && (i & AUI_BIT))
				;  /* BNC & pgmed as other: do nothing */
			else 
	    		if (!write_eeprom(base_io, BASE_IO_REG, j))
					cmn_err(CE_WARN,"write to eeprom failed\n");
		}
	 
		/* ethernet cable type: ee16cable_type: 0=>BNC; 1=>AUI */
		if (!ee16cable_type) {
			/* if eeprom not pgmed as BNC, make it BNC */
			if (!read_eeprom(base_io, 5)) {
	        	if (!write_eeprom(base_io, 5, 0))
					cmn_err(CE_WARN,"write to eeprom failed\n");
			}
		}

		/* This in is needed to avoid a race condition in HW */
		j = inb(base_io + AUTOID);
  	    /* warm up dram */
		outw(base_io + RDPTR, 0);
		for (i=0; i<16; i++)
	    	j = inb(base_io + DXREG);

		/* reset ASIC on board */
		outb(base_io + EE_CTRL, RESET_586);
		drv_usecwait(300);
		outb(base_io + EE_CTRL, RESET_586 | GA_RESET);
		drv_usecwait(300);
		outb(base_io + EE_CTRL, RESET_586);
		drv_usecwait(300);

		/* checksum for eeprom */
		cksum = 0;
		for (j=0; j<0x3f; j++) 
		    cksum += read_eeprom(base_io, j);	
		if (cksum + read_eeprom(base_io, 0x3f) != 0xbaba) {
			if (cksum <= 0xbaba)
			    write_eeprom(base_io, 0x3f, 0xbaba - cksum);
			else
			    write_eeprom(base_io, 0x3f,0xbaba+1+(0xffff - cksum));
		}
	}	

	/* Initialize the configured boards */
	for (i=0, bd=ee16config; i<(ushort)ee16boards; bd++, i++) {

		base_io = bd->io_start;

		if (ee16boards > 1) {
			/* reset ASIC on board */
			outb(base_io + EE_CTRL, RESET_586);
			drv_usecwait(300);
			outb(base_io + EE_CTRL, RESET_586 | GA_RESET);
			drv_usecwait(300);
			outb(base_io + EE_CTRL, RESET_586);
			drv_usecwait(300);
		}

		/* initialize flags and mib structure */
		bd->flags = 0;
		bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

		if (!ee16chk (base_io)) {
			cmn_err(CE_WARN,"%s board %d at base address %x not found",
						   ee16id_string, i, bd->io_start);
			ret_code = 6;
			continue;
		}

		cmn_err (CE_CONT,"%s board %d ",ee16id_string,i);
		x = read_eeprom(base_io,  ETHER_TYPE_REG);
		if (!(x & AUI_BIT)) 
			cmn_err (CE_CONT, "(AUI) ");
		else
			cmn_err (CE_CONT, "(BNC) ");

		bd->flags = BOARD_PRESENT;
		ret_code = 0;

		/* print 3 words of ethernet address */
		for (j=0; j<3; j++) {
			ushort t; 
			t = read_eeprom (base_io, INTEL_EADDR_L+j);
			/* change the byte ordering */
			bd->eaddr.words[2-j] = 
				((ushort)(t & 0xff) << 8) | ((ushort)(t & 0xff00) >> 8);
		}
		ee16print_eaddr(bd->eaddr.bytes);

	    /* Release the RESET_586 bit : dont release it while
         * accessing the eeprom */
	    outb(base_io+EE_CTRL, 0); 

		/* timeout every 5 seconds */
		ee16timer_id = timeout (ee16watch_dog, 0, drv_usectohz (EE16_TIMEOUT));

		/* Initialize DLconfig */
		bd->bd_number     = i;
		bd->sap_ptr       = &ee16saps[ i * bd->max_saps ];
		bd->tx_next       = 0;
		bd->timer_val     = -1;
		bd->promisc_cnt   = 0;
		bd->multicast_cnt = 0;
		bd->bd_dependent1 =(caddr_t)kmem_alloc(sizeof(bdd_t), KM_SLEEP);
		bdd = (bdd_t *)bd->bd_dependent1;
		for (k = 0; k < MULTI_ADDR_CNT; k++)
                          bdd->ee16_multiaddr[i].status = 0;
		bd->bd_dependent2 = 0;
		bd->bd_dependent3 = 0;
		bd->bd_dependent4 = 0;
		bd->bd_dependent5 = 0;

		ee16intr_to_index [bd->irq_level] = i;

		/* Initialize SAP structure info */
		for (sap=bd->sap_ptr,x=0;x< (uchar_t)bd->max_saps; x++, sap++) {
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
		if (ee16ifstats) {
			bd->ifstats = &ee16ifstats[i];
			ee16ifstats[i].ifs_name   = ee16_ifname;
			ee16ifstats[i].ifs_unit   = (short)i;
			ee16ifstats[i].ifs_mtu    = USER_MAX_SIZE;
			ee16ifstats[i].ifs_active = 1;
			ee16ifstats[i].ifs_next   = ifstats;
			ee16ifstats[i].ifs_ipackets = ee16ifstats[i].ifs_opackets = 0;
			ee16ifstats[i].ifs_ierrors  = ee16ifstats[i].ifs_oerrors  = 0;
			ee16ifstats[i].ifs_collisions = 0;
			ifstats = &ee16ifstats[i];
		}

		bd->mib.ifAdminStatus = DL_UP;		/* SNMP */

		if (j = ee16init_586 (bd)) {
			cmn_err(CE_WARN, 
			  "%d ee16init: 82586 did not respond to the initialization process",j);
			bd->flags = BOARD_DISABLED;
			ret_code = 14;
		} else {
			bd->mib.ifOperStatus = DL_UP;	/* SNMP */
			ret_code = 0;
		}

#ifdef NBPFILTER
		if (ee16ifp) {
			static struct bpf_devp dev = { DLT_EN10MB, sizeof(DL_ether_hdr_t) };

			ee16ifp[ i ].if_name   = ee16_ifname;
			ee16ifp[ i ].if_unit   = (short)i;
			ee16ifp[ i ].if_mtu    = USER_MAX_SIZE;
			ee16ifp[ i ].if_flags  = IFF_UP;
			ee16ifp[ i ].if_output = ee16bpf_output;
			ee16ifp[ i ].if_ioctl  = ee16bpf_ioctl;
			ee16ifp[ i ].if_next   = (struct ifnet*) bd;
			ee16ifp[ i ].if_ctlin  = ee16bpf_promisc;

			bpfattach(&bd->bpf, &ee16ifp[ i ], &dev);
		}
#endif
	}
	return (ret_code);
}

/*************************************************************************
 *  ee16chk()
 *
 *  Checks the validity of board signature
 */

STATIC
ee16chk (base_io)
ushort_t	base_io;
{
	ushort_t	j;
	int 		i;

	/* get it into a known state: lower nibble = 0 
     * In the worst case this should happen in 15 attempts */
	for (i=0; i<20; i++) {
        j = inb(base_io + AUTOID);
		if (!(j & 0x0f)) break;
	}
	if (i == 20) return 0;
    if (j & 0xf0 != 0xa0) return 0;
	if (inb (base_io+AUTOID) != 0xb1) return 0;
	if (inb (base_io+AUTOID) != 0xa2) return 0;
	if (inb (base_io+AUTOID) != 0xb3) return 0;
	return (1);
}

/*************************************************************************/
find_mem_size(base_io)
ushort	base_io;
{
	ushort i;

	i = inb(base_io + AUTOID);
	outw(base_io + WRPTR, 0);
	outb(base_io + DXREG, 0);
	i = inb(base_io + AUTOID);
	outw(base_io + WRPTR, 0x8000);	/* 32K */
	outb(base_io + DXREG, 0);

	i = inb(base_io + AUTOID);
	outw(base_io + WRPTR, 0);
	outb(base_io + DXREG, 0xaa);
	i = inb(base_io + AUTOID);
	outw(base_io + RDPTR, 0x8000);	/* 32K */
	if (inb(base_io + DXREG) == 0xaa) 
		mem_ofst = 0x8000;
	else
		mem_ofst = 0;
}
/*************************************************************************
 * ee16init_586 ()
 *
 * initialize the 82586's scp, iscp, and scb; reset the 82586;
 * do IA setup command to add the ethernet address to the 82586 IA table;
 * and enable the Receive Unit.
 */

ushort
ee16init_586 (bd)
DL_bdconfig_t *bd;
{
	/* High addresses of Buffer RAM structure:
	 *      CB : for general (non xmit) commands
	 *      SCB
	 * 		ISCP
	 * 		SCP
	 *		  --- End of MAX_RAM_SIZE (64K)
	 */

	bdd_t	*bdd      = (bdd_t *) bd->bd_dependent1;
	ushort	ofst_scp  = MAX_RAM_SIZE - sizeof (scp_t);	
	ushort	ofst_iscp = ofst_scp - sizeof (iscp_t);	
	ushort	ofst_scb  = ofst_iscp - sizeof (scb_t);
	ushort	base_io  = bd->io_start;
	ushort	i, irq, rb;
	iscp_t	iscp;
	scb_t	scb;

	find_mem_size(base_io);
	ee16debug.reset_count++;

	/* fill in scp : 16 bit accesses */
	write_buffer(BYTE, ofst_scp + 0, 0, base_io);			/* sysbus */
	write_buffer(WORD, ofst_scp + 6, ofst_iscp, base_io);	/* iscp */	
	outb(base_io + DXREG, 0); 		/* WRPTR autoincremented */

	/* fill in iscp */
	iscp.iscp_busy 		= 1;
	iscp.iscp_scb_ofst 	= ofst_scb;
	iscp.iscp_scb_base 	= 0;
	bcopy_to_buffer((char *)&iscp, ofst_iscp, sizeof(iscp_t), base_io);

	/* fill in scb */
	scb.scb_status 		= 0;
	scb.scb_cmd 		= 0;
	scb.scb_cbl_ofst 	= 0xffff;
	scb.scb_rfa_ofst 	= 0xffff;
	scb.scb_crc_err		= 0;
	scb.scb_aln_err		= 0;
	scb.scb_rsc_err		= 0;
	scb.scb_ovrn_err	= 0;
	bcopy_to_buffer((char *)&scb, ofst_scb, sizeof(scb_t), base_io);

	bdd->ofst_scb = ofst_scb;
	bdd->gen_cmd = ofst_scb - sizeof (cmd_t);	/* CB offset */

	ee16init_tx (bdd, base_io);
	ee16init_rx (bdd, base_io);
	
	/* start the 82586, by resetting the 586 & issuing a CA */
	outb (base_io + EE_CTRL, RESET_586);	/* reset the 82586 */
	outb (base_io + EE_CTRL, 0);			/* release from reset */

	outb (base_io + CA_CTRL, 1);			/* channel attention */

	/* wait for iscp busy to be cleared */
	for (i=1000; i; i--) {					
		read_buffer(BYTE, ofst_iscp + 0, base_io, rb);		/* iscp_busy */
		if (!(rb & 1))	
			break;
		drv_usecwait (10);
	}
	if (i == 0)	{		/* if bit isn't cleared */
        cmn_err(CE_WARN, "ISCP busy not cleared\n");
		return (1);		/* return error */
    } 

	for (i=1000; i; i--) {					/* wait for scb status */
		read_buffer(WORD, ofst_scb + 0, base_io, rb);
		if (rb == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait (10);
	}
	if (i == 0)								/* if CX & CNA aren't set */
		return (2);							/* return error */

	if (ee16ack_586 (ofst_scb, base_io))
		return (2);

	/* configure 586 with default parameters */
	if (ee16config_586 (bd, PRO_OFF))
		return (2);

	/* do IA Setup command and enable 586 Receive Unit */
	if (ee16ia_setup_586 (bd, bd->eaddr.bytes))
		return (2);

	/* enable 586 Receive Unit */
	write_buffer(WORD, ofst_scb + 0, 0, base_io);	/* status */
	outw(base_io + DXREG, SCB_RUC_STRT);	/* command: autoincrement */
	write_buffer(WORD, ofst_scb + 6, bdd->begin_fd, base_io);	/* rfa */

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (ofst_scb, 1000, base_io))
		return (2);

	if (ee16ack_586 (ofst_scb, base_io))
		return (2);

	/* enable interrupt */
	switch (irq = (uchar_t) bd->irq_level) {
	  case 3:
	  case 4: 
	  case 5: 
	      irq--;       	/* irq-- is sent to SEL_IRQ if intr irq desired */
	      break;
	  case 9: 
	  case 2:
	      irq = 1;
		  break;
	  case 10: 
	  case 11: 
	      irq -= 5;
	      break;
	  default:
	      cmn_err(CE_WARN,"Invalid interrupt value %d\n",irq);
	      return;
	}
	outb (base_io+SEL_IRQ, irq | 0x08);  
	return (0);
}

/*************************************************************************/
STATIC void
ee16init_tx (bdd, base_io)
bdd_t    *bdd;
ushort_t base_io;
{
	ushort	ofst_txb = 0;
	ushort	ofst_tbd = 0;
	ushort	ofst_cmd = 0;
	ring_t	*ring;
	ushort	i, j;
	tbd_t	tbd;
	cmd_t	cmd;

	/* the number of tbds and cmds are set to be the same since
	 * each cmd usually requires one tbd */
	bdd->n_tbd = bdd->n_cmd = 3;

	ofst_cmd = bdd->ofst_cmd = mem_ofst;
	ofst_tbd = bdd->ofst_tbd = ofst_cmd + sizeof(cmd_t);
	ofst_txb = bdd->ofst_txb = ofst_tbd + sizeof(tbd_t);

	/* allocate ring buffer as an array of ring_t */
	ring = bdd->ring_buff = 
			(ring_t *) kmem_alloc (bdd->n_cmd*sizeof(ring_t), KM_SLEEP);

	/* initialize cmd, tbd, and ring structures */
	for (i=0; i<bdd->n_cmd; i++, ring++) {

		/* initialize cmd */
		cmd.cmd_status		= 0;
		cmd.cmd_cmd			= CS_EL;
		cmd.cmd_nxt_ofst	= 0xffff;
		cmd.prmtr.prm_xmit.xmt_tbd_ofst 	= ofst_tbd;
		bcopy_to_buffer((char *)&cmd, ofst_cmd, sizeof(cmd_t), base_io);

		/* initialize tbd */
		tbd.tbd_count		= 0;
		tbd.tbd_nxt_ofst	= 0xffff;
		tbd.tbd_buff		= ofst_txb;
		tbd.tbd_buff_base	= 0;
		bcopy_to_buffer((char *)&tbd, ofst_tbd, sizeof(tbd_t), base_io);
		
		ring->ofst_cmd = ofst_cmd;
		ring->next = ring + 1;
		ofst_cmd += sizeof(cmd_t) + sizeof(tbd_t) + TBD_BUF_SIZ;
		ofst_tbd += sizeof(cmd_t) + sizeof(tbd_t) + TBD_BUF_SIZ;
		ofst_txb += sizeof(cmd_t) + sizeof(tbd_t) + TBD_BUF_SIZ;
	}

	/* complete ring buffer by making the last next pointer points to the first */
	(--ring)->next = bdd->ring_buff;
	bdd->head_cmd = 0;
	bdd->tail_cmd = 0;
}

/*************************************************************************
 * ee16init_rx (bdd, base_io)
 *   retain ee16 buffer configuration for receive side
 */

STATIC void
ee16init_rx (bdd, base_io)
bdd_t     *bdd;
ushort_t  base_io;
{
	ushort	ofst_rxb = 0;
	ushort	ofst_rbd = 0;
	ushort	ofst_fd  = 0;
	int		mem_left;
	ushort	i, rb;
	fd_t	fd;
	rbd_t	rbd;

	/* receive buffer starts at the end of transmit data area */
	ofst_rxb = bdd->ofst_rxb = 
				 mem_ofst + (bdd->n_cmd * (sizeof(cmd_t) + sizeof(tbd_t) + TBD_BUF_SIZ));
	mem_left = bdd->gen_cmd - bdd->ofst_rxb; 

	/* calculate number of rbd and fd */
	bdd->n_rbd = bdd->n_fd = mem_left/(sizeof(fd_t)+sizeof(rbd_t)+RBD_BUF_SIZ); 

	/* rbd's follow rxb's and fd's follow rbd's */
	ofst_rbd = bdd->ofst_rbd = ofst_rxb + bdd->n_rbd*RBD_BUF_SIZ;
	ofst_fd  = bdd->ofst_fd  = ofst_rbd + bdd->n_rbd*sizeof(rbd_t);

	bdd->begin_fd = ofst_fd;
	bdd->begin_rbd = ofst_rbd;

	/* initialize fds */
	for (i=0; i<bdd->n_fd; i++) {
		fd.fd_status		= 0;
		fd.fd_cmd			= 0;
		fd.fd_nxt_ofst		= ofst_fd + sizeof(fd_t);
		fd.fd_rbd_ofst		= 0xffff;
		bcopy_to_buffer((char *)&fd, ofst_fd, sizeof(fd_t), base_io);
		ofst_fd 			+= sizeof (fd_t);
	}

	/* init first fd's rbd */
	write_buffer(WORD, bdd->begin_fd + 6, bdd->begin_rbd, base_io);	

	bdd->end_fd = ofst_fd - sizeof (fd_t);

	/* init command and link ptr of last fd */
	write_buffer(WORD, bdd->end_fd + 2, CS_EL, base_io);	
	outw(base_io + DXREG, 0xffff);
   
	/* initialize all rbds */
	for (i=0; i<bdd->n_rbd; i++) {
		rbd.rbd_status		= 0;
		rbd.rbd_nxt_ofst	= ofst_rbd + sizeof(rbd_t);
		rbd.rbd_buff		= ofst_rxb;
		rbd.rbd_buff_base	= 0;
		rbd.rbd_size		= RBD_BUF_SIZ;
		bcopy_to_buffer((char *)&rbd, ofst_rbd, sizeof(rbd_t), base_io);
		ofst_rbd			+= sizeof(rbd_t);
		ofst_rxb			+= RBD_BUF_SIZ;
	}

	bdd->end_rbd = ofst_rbd - sizeof (rbd_t);

	/* link field of last rbd is null */
	write_buffer(WORD, bdd->end_rbd + 2, 0xffff, base_io);	

	/* EL is set for last rbd */
	read_buffer(WORD, bdd->end_rbd + 8, base_io, rb);
	write_buffer(WORD, bdd->end_rbd + 8, rb | CS_EL, base_io);
}

/*************************************************************************/
ushort
ee16config_586 (bd, prm_flag)
DL_bdconfig_t *bd;
ushort_t prm_flag;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ushort	base_io = bd->io_start;

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	write_buffer(WORD, bdd->ofst_scb + 0, 0, base_io);	/* status */
	outw(base_io + DXREG, SCB_CUC_STRT);	/* command : auto-incr */
	outw(base_io + DXREG, bdd->gen_cmd);	/* cbl : auto-incr */

	write_buffer(WORD, bdd->gen_cmd + 0, 0, base_io);	/* status */
	outw(base_io + DXREG, CS_CMD_CONF | CS_EL);	/* command : auto-incr */
	outw(base_io + DXREG, 0xffff);				/* link : auto-incr */

	/* default config page 2-28 586 book */
	/* fifo byte */
	write_buffer(WORD, bdd->gen_cmd + 6, 0x080c, base_io);
	outw(base_io + DXREG, 0x2600);			/* add mode : auto-incr */
	outw(base_io + DXREG, 0x6000);			/* pri data : auto-incr */
	outw(base_io + DXREG, 0xf200);			/* slot : auto-incr */
	outw(base_io + DXREG, 0x0000 | prm_flag);	/* hrdwr : auto-incr */
	outw(base_io + DXREG, 0x0040);			/* min len : auto-incr */

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	if (ee16ack_586 (bdd->ofst_scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************/
ee16ia_setup_586 (bd, eaddr)
DL_bdconfig_t *bd;
uchar_t eaddr[];
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ushort	base_io = bd->io_start;
	ushort		i;
	
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	write_buffer(WORD, bdd->ofst_scb + 0, 0, base_io);	/* status */
	outw(base_io + DXREG, SCB_CUC_STRT);	/* command : auto-incr */
	outw(base_io + DXREG, bdd->gen_cmd);	/* cbl : auto-incr */
	outw(base_io + DXREG, bdd->ofst_fd);	/* rfa : auto-incr */
	
	write_buffer(WORD, bdd->gen_cmd + 0, 0, base_io);	/* status */
	outw(base_io + DXREG, CS_CMD_IASET | CS_EL); /* command : auto-incr */
	outw(base_io + DXREG, 0xffff);				 /* link : auto-incr */

	/* add sizes of xmit_t and conf_t to the current offset */
	write_buffer(BYTE, bdd->gen_cmd + 6, eaddr[0], base_io);
	for (i=1; i<6; i++)
	    outb(base_io + DXREG, eaddr[i]);

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	if (ee16ack_586 (bdd->ofst_scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************
 * ee16wait_scb (scb, how_long, base_io)
 *
 * Acceptance of a Control Command is indicated by the 82586 clearing
 * the SCB command field page 2-16 of the intel microcom handbook.
 */
ushort
ee16wait_scb (scb, n, base_io)
ushort_t 	scb; 	/* scb offset in the buffer */
ushort		n;
ushort_t 	base_io;
{
	ushort  i = 0;
	ushort	rb;

	while (i++ != n * 1000) {
		read_buffer(WORD, scb + 2, base_io, rb); 	/* cmd */
		if (rb == 0)
			return (0);
	}
	return (1);
}

/*************************************************************************/
int
ee16ack_586 (scb, base_io)
ushort_t scb;	/* ofst of scb */
ushort   base_io;
{
	ushort_t i;
	
	read_buffer(WORD, scb + 0, base_io, i);	     /* status */
	i &= SCB_INT_MSK;
	if (i != 0) {	
	    write_buffer(WORD, scb + 2, i, base_io); /* cmd */
		outb (base_io + CA_CTRL, 1);			 /* channel attention */
		return (ee16wait_scb (scb, 10000, base_io));
	}
	return (0);
}

/*************************************************************************
 * read_eeprom(): read data from reg in eeprom
 *    Refer to the assembly routines provided with EE16
 *    for enlightenment!
 */

ushort
read_eeprom(base_io, reg)
ushort base_io;
ushort reg;
{
	short x;
    ushort data;

    /* mask off 586 access */
	x = inb(base_io + EE_CTRL);
    x |= RESET_586;
    x &= ~GA_RESET;
	outb(base_io + EE_CTRL, x);

    /* select EEPROM, mask off ASIC and reset bits, set EECS */
	x = inb(base_io + EE_CTRL);
	x &= ~(GA_RESET | EEDI | EEDO | EESK);
	x |= EECS; 
	outb(base_io + EE_CTRL,x);

	/* write the read opcode and register number in that order 
     * The opcode is 3bits in length; reg is 6 bits long */
    shift_bits_out(EEPROM_READ_OPCODE, 3, base_io);
	shift_bits_out(reg, 6, base_io);
	data = shift_bits_in(base_io);

	eeprom_cleanup(base_io);
	return data;
}

/*************************************************************************
 * write_eeprom(): write data to reg in eeprom
 */

STATIC
ushort_t
write_eeprom(base_io, reg, data)
ushort base_io;
ushort reg;
ushort data;
{
	ushort_t x,i;

    /* mask off 586 access */
	x = inb(base_io + EE_CTRL);
    x |= RESET_586;
    x &= ~GA_RESET;
	outb(base_io + EE_CTRL, x);

    /* select EEPROM, mask off ASIC and reset bits, set EECS */
	x = inb(base_io + EE_CTRL);
	x &= ~(GA_RESET | EEDI | EEDO | EESK);
	x |= EECS;
	outb(base_io + EE_CTRL, x);

	/* write the erase/write enable opcode */
    shift_bits_out(EEPROM_EWEN_OPCODE, 5, base_io);
    shift_bits_out(EEPROM_EWEN_OPCODE, 4, base_io);	/* 4 dont cares */

    /* lower chip select for 1 usecond */
    x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
    outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
    outb(base_io + EE_CTRL, x | EECS);

    shift_bits_out(EEPROM_ERASE_OPCODE, 3, base_io);
	shift_bits_out(reg, 6, base_io);    /* send eeprom location */

    /* lower chip select for 1 usecond */
    x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
    outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
    outb(base_io + EE_CTRL, x | EECS);

	/* wait for > 10 ms for eedo to go high: command done */
    drv_usecwait(20000); /* wait for 20 ms */
	if (!(inb(base_io + EE_CTRL) | EEDO))
		return 0;

    /* lower chip select for 1 usecond */
    x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
    outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
    outb(base_io + EE_CTRL, x | EECS);

    shift_bits_out(EEPROM_WRITE_OPCODE, 3, base_io);
	shift_bits_out(reg, 6, base_io);    /* send eeprom location */
	shift_bits_out(data, 16, base_io);	/* write data */

    /* lower chip select for 1 usecond */
    x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
    outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
    outb(base_io + EE_CTRL, x | EECS);

	/* wait for >= 10 ms for eedo to go high : command done */
    drv_usecwait(20000); /* wait for 20 ms */
    if (!(inb(base_io + EE_CTRL) | EEDO))
         return 0;

    /* lower chip select for 1 usecond */
    x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
    outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
    outb(base_io + EE_CTRL, x | EECS);

    shift_bits_out(EEPROM_EWDS_OPCODE, 5, base_io);
    shift_bits_out(EEPROM_EWDS_OPCODE, 4, base_io);	/* dont cares */
	eeprom_cleanup(base_io);
	return 1;
}

/*************************************************************************
 * shift count bits of data to eeprom 
 */
STATIC
void
shift_bits_out(data, count, base_io)
ushort data;
ushort count;
ushort base_io;
{
	ushort_t i,x,mask;

    mask = 0x01 << (count - 1);
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EEDO | EEDI); 
	do {
	  x &= ~EEDI;
	  if (data & mask) 
	    x |= EEDI;
	  outb(base_io + EE_CTRL, x);
 	  drv_usecwait(10);
	  raise_clock(base_io, &x);
      lower_clock(base_io, &x); 
	  mask = mask >> 1;
    } while (mask);

    x &= ~EEDI;
	outb(base_io + EE_CTRL, x);
}

/*************************************************************************
 * raise eeprom clock 
 */
STATIC
void
raise_clock(base_io, x)
ushort base_io;
ushort *x;
{
    *x = *x | EESK;
	outb(base_io + EE_CTRL, *x);
 	drv_usecwait(10);
}

/*************************************************************************
 * lower eeprom clock 
 */
STATIC
void
lower_clock(base_io, x)
ushort base_io;
ushort *x;
{
    *x = *x & ~EESK;
	outb(base_io + EE_CTRL, *x);
 	drv_usecwait(10);
}

/*************************************************************************
 * shift count bits of data in from eeprom 
 */
STATIC 
ushort_t
shift_bits_in(base_io)
ushort base_io;
{
    ushort x,d,i;
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EEDO | EEDI); 
    d = 0;
	for (i=0; i<16; i++) {
	  d = d << 1;
	  raise_clock(base_io, &x); 
	  x = inb(base_io + EE_CTRL) & ~(GA_RESET | EEDI); 
	  if (x & EEDO)
		  d |= 1;
      lower_clock(base_io, &x);
	}
	return d;
}

/*************************************************************************/
STATIC
void
eeprom_cleanup(base_io)
ushort base_io;
{
    ushort x;
	x = inb(base_io + EE_CTRL) & ~GA_RESET;
	x &= ~(EECS | EEDI);
	outb(base_io + EE_CTRL, x);
	raise_clock(base_io, &x);
	lower_clock(base_io, &x);
}

STATIC void
ee16uninit()
{
	DL_bdconfig_t	*bd;
	ushort i;

	if (ee16inetstats)
		if (ee16ifstats)
			kmem_free (ee16ifstats, sizeof(struct ifstats) * ee16boards);
	for (i=0, bd=ee16config; i < (ushort)ee16boards; bd++, i++)
		if (bd->bd_dependent1) 
			kmem_free ((int *)bd->bd_dependent1, sizeof(bdd_t));
	untimeout (ee16timer_id);
}
