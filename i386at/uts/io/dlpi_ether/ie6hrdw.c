/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/dlpi_ether/ie6hrdw.c	1.14"
#ident	"$Header: $"
/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. xxxxxx";


/******************************************************************************
 *  This file contains all of the hardware dependent code for the 3c503.
 *  It is the companion file to ../../io/dlpi_ether.c
 */

/*
 *  Device dependent symbol names.
 */
#include <util/types.h>
#include <util/debug.h>
#include <io/stream.h>
#include <util/param.h>
#include <svc/errno.h>
#include <util/sysmacros.h>
#include <io/stropts.h>
#include <io/strstat.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/tcpip/strioc.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/tcpip/if.h>
#include <net/dlpi.h>
#include <mem/immu.h>
#include <io/ddi.h>
#include <svc/systm.h>
#ifndef lint
#include <net/tcpip/byteorder.h>
#endif
#include <io/rtc/rtc.h>
#include <util/cmn_err.h>
#include <io/dlpi_ether/ie6.h>
#include <io/dlpi_ether/dlpi_ie6.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <mem/kmem.h>
#include <util/inline.h>

extern	DL_bdconfig_t	ie6config[];
extern	DL_sap_t	ie6saps[];
extern	int		ie6boards;
extern	int		ie6_cable_type[];
extern	int		ie6inetstats;
extern	int		ie6strlog;
extern	struct	ifstats	*ifstats;
extern	char		*ie6_ifname;
extern	int		ie6_multisize;
extern	struct ie6maddr	ie6multiaddrs[];

extern	void		DLprint_eaddr();
extern	ushort_t	ntohs(), htons();

STATIC	int	ie6ga_init(), ie6nic_init(), ie6chk(), ie6nic_workaround();
STATIC	void	ie6stop_nic(), ie6set_nic_eaddr(), ie6chk_ring(),
		ie6watchdog(), ie6tx_done();
STATIC mblk_t*	ie6mkbuf();

/*
 *  These are inline functions unless C_PIO is defined.
 */
extern	void	ie6pio_read(), ie6pio_write();

/*
 *  Conversion tables.
 */
STATIC	ushort_t	ie6io_convert[] = { 0x300, 0x310, 0x330, 0x350,
					    0x250, 0x280, 0x2a0, 0x2e0 };

STATIC	uchar_t		ie6intr_convert[] = { 0, 0, IRQ2, IRQ3, IRQ4, IRQ5,
					      0, 0, 0, IRQ2 };

STATIC	int		ie6intr_to_index[] = { -1, -1, -1, -1, -1,
					       -1, -1, -1, -1, -1 };
STATIC	struct	ifstats	*ie6ifstats;

#define	MB_SIZE(p)	(p->b_wptr - p->b_rptr)

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*ie6ifp;
STATIC	int	ie6bpf_ioctl(), ie6bpf_output(), ie6bpf_promisc();

extern	void	bpf_tap(), bpfattach();

#define	bpf	bd_dependent5
#endif

/*
 * A couple of useful macros...
 */
#define BCOPY(from, to, len) bcopy((caddr_t)(from),(caddr_t)(to),(size_t)(len))
#define BCMP(s1, s2, len) bcmp((char*)(s1),(char*)(s2),(size_t)(len))

unsigned char	ie6hash();

void	ie6bdspecclose(), ie6bdspecioctl();

/* ie6bdspecclose is called from DLclose->dlpi_ether.c */
void
ie6bdspecclose(q)
queue_t *q;
{
	return;
} /* end of ie6bdspecclose */

/* ie6bdspecioctl is called from DLioctl->dlpi_ether.c */
void
ie6bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *ioctl_req = (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
} /* end of ie6bdspecioctl */

/*
 * Header, wrapper, and function declarations and definitions for loadability
 */

#include <util/mod/moddefs.h>
#define DRVNAME		"ie6 - Loadable ie6 ethernet driver"

STATIC	int	ie6_load(), ie6_unload();

MOD_DRV_WRAPPER(ie6, ie6_load, ie6_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

int ie6init();
STATIC void ie6uninit();

STATIC int
ie6_load(void)
{
	int	ret_code;

	cmn_err(CE_NOTE, "!MOD: in ie6_load()");

	if ((ret_code=ie6init()) != 0)
		return (ret_code);
	mod_drvattach(&ie6_attach_info);
	return(0);
}

STATIC int
ie6_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in ie6_unload()");

	mod_drvdetach(&ie6_attach_info);
	ie6uninit();
	return(0);
}

STATIC void
ie6uninit()
{
	DL_bdconfig_t	*bd = ie6config;
	int i;

	if (ie6inetstats)
		if (ie6ifstats)
			kmem_free (ie6ifstats, sizeof(struct ifstats) * ie6boards);
	for (i = 0; i < ie6boards; i++, bd++)
		untimeout (bd->timer_id);
}

/****************************************************************
 *  v1.0 - Initial Beta release
 *  v1.1 - BPF support added
 */
	char	ie6id_string[] = "IE-6 (3C503) v1.1";
static	char	ie6copyright[] = "Copyright (c) 1991 Intel Corp., All Rights Reserved";

/*
 *  NIC workaround defines
 */
#define	FRAME_OK	0
#define	SKIP_FRAME	1
#define	RESET_RING	2

/******************************************************************************
 *  ie6init()
 */
int
ie6init()
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	DL_sap_t	*sap;
	int		i, x;
	int		ret_code;

	/*
	 *  Let them know we have arrived.
	 */
	cmn_err(CE_CONT, "%s %s\n", ie6id_string, ie6copyright);

	/*
	 *  Allocate internet stats structure if that's ok with the user.
	 */
	if (ie6inetstats)
		ie6ifstats = (struct ifstats*)kmem_zalloc(
			(sizeof(struct ifstats) * ie6boards), KM_NOSLEEP);
#ifdef NBPFILTER
	ie6ifp = (struct ifnet*)kmem_zalloc((sizeof(struct ifnet) * ie6boards),
								KM_NOSLEEP);
#endif
		
	/*
	 *  Initialize the configured boards.
	 */
	for (i = 0, bd = ie6config; i < ie6boards; bd++, i++) {
		/*
		 *  Make sure BOARD_PRESENT flag is cleared in case of error.
		 */
		bd->flags = 0;

		/* 
		 *  Make sure the board was installed properly
		 */
		if (!ie6chk(bd)) {
			cmn_err(CE_WARN,
			    "%s board %d at I/0 base address 0x%x not found",
						ie6id_string, i, bd->io_start);
			ret_code = 6;
			continue;
		} else {
			cmn_err(CE_CONT, "%s board %d was found ",
							ie6id_string, i);
			ret_code = 0;
		}

		/*
		 *  Initialize DLconfig.
		 */
		bd->bd_number     = i;
		bd->sap_ptr       = &ie6saps[ i * bd->max_saps ];
		bd->tx_next       = 0;
		bd->promisc_cnt   = 0;
		bd->multicast_cnt = 0;
		bd->bd_dependent1 = 0;
		bd->bd_dependent2 = 0;
		/*
		 * Allocate space for dependent structure in support of
		 * of multicast addressing...
		 */
		if((bd->bd_dependent3 = (caddr_t)kmem_zalloc(sizeof(bdd_t),KM_NOSLEEP)) == NULL) {
			bd->flags |= BOARD_DISABLED;
			cmn_err(CE_CONT,"Unable to allocate dependent structure.\n");
			continue;
		}
		bdd = (bdd_t *)bd->bd_dependent3;
		bdd->ie6_multiaddrs = &ie6multiaddrs[i * ie6_multisize];
		/* end of multicast init */

		bd->bd_dependent4 = 0;
		bd->bd_dependent5 = 0;

		bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

		/*
		 *  Initialize SAP structure info.
		 */
		for (sap = bd->sap_ptr, x = 0; x < bd->max_saps; x++, sap++) {
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
		 *  Initialize the Gate Array and NIC.
		 *
		 *  N.B.  Order of evaluation is important.
		 */

		bd->mib.ifAdminStatus = DL_UP;			/* SNMP */

		if (ie6ga_init(bd) || ie6nic_init(bd, 1)) {
			bd->mib.ifOperStatus = DL_DOWN;		/* SNMP */
			cmn_err(CE_WARN, "data link initialization failed");
			ret_code = -1;
			continue;
		} else {
			bd->flags = BOARD_PRESENT;
			bd->mib.ifOperStatus = DL_UP;		/* SNMP */
			ret_code = 0;
		}


		/*
		 *  Enable interrupts.
		 */
		GA_WRITE(bd->io_start, gacfr, TCM);

		/*
		 *  Initalize internet stat statucture.
		 */
		if (ie6ifstats) {
			bd->ifstats = &ie6ifstats[ i ];
			ie6ifstats[ i ].ifs_name   = ie6_ifname;
			ie6ifstats[ i ].ifs_unit   = (short)i;
			ie6ifstats[ i ].ifs_mtu    = USER_MAX_SIZE;
			ie6ifstats[ i ].ifs_active = 1;
			ie6ifstats[ i ].ifs_next   = ifstats;
			ifstats = &ie6ifstats[ i ];
		}
#ifdef NBPFILTER
		if (ie6ifp) {
			static struct bpf_devp dev =
				{ DLT_EN10MB, sizeof(DL_ether_hdr_t) };

			ie6ifp[ i ].if_name   = ie6_ifname;
			ie6ifp[ i ].if_unit   = (short)i;
			ie6ifp[ i ].if_mtu    = USER_MAX_SIZE;
			ie6ifp[ i ].if_flags  = IFF_UP;
			ie6ifp[ i ].if_output = ie6bpf_output;
			ie6ifp[ i ].if_ioctl  = ie6bpf_ioctl;
			ie6ifp[ i ].if_next   = (struct ifnet*) bd;
			ie6ifp[ i ].if_ctlin  = ie6bpf_promisc;

			bpfattach(&bd->bpf, &ie6ifp[ i ], &dev);
		}
#endif
	}

	return (ret_code);
}

/******************************************************************************
 *  ie6ga_init()
 */
STATIC
ie6ga_init(bd)
DL_bdconfig_t	*bd;
{
	int		i;
	uchar_t		xcvr;
	int		base_io = bd->io_start;

	/*
	 *  Since we think the board is present, it should be safe
	 *  to reset it.
	 */
	GA_WRITE(base_io, cr, RST | XSEL);
	GA_WRITE(base_io, cr, XSEL);
	GA_WRITE(base_io, cr, XSEL);    /* 3Com example does this twice */

	/*
	 *  Read and store the ethernet address as well as display it
	 *  on the console.
	 */
	GA_WRITE(base_io, cr, EALO | XSEL);
	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		bd->eaddr.bytes[ i ] = EADDR_READ(base_io, i);

	DLprint_eaddr(bd->eaddr.bytes);

	/*
	 *  Set the xcvr for the user selected type.
	 */
	xcvr = ie6_cable_type[ bd->bd_number ] ? 0 : XSEL;
	GA_WRITE(base_io, cr, xcvr);
	
	/*
	 *  Setup memory configuration for recive buffer and DMA burst timer.
	 */
	GA_WRITE(base_io, pstr, RCVSTART);
	GA_WRITE(base_io, pspr, RCVSTOP);
	GA_WRITE(base_io, dqtr, DRQTIMERVALUE);

	/*
	 *  Set xmit DMA address.
	 */
	GA_WRITE(base_io, dma_msb, XMITSTART);
	GA_WRITE(base_io, dma_lsb, 0);

	/*
	 *  Make sure the requested interrupt level is valid.
	 */
	if ((bd->irq_level > (sizeof(ie6intr_convert) - 1)) ||
	    (ie6intr_convert [ bd->irq_level ] == 0)        ||
	    (ie6intr_to_index[ bd->irq_level ] != -1)) {
		cmn_err(CE_WARN,
			"%s board %d - invalid interrupt value in space.c (%d)",
				ie6id_string, bd->bd_number, bd->irq_level);
		return (1);
	}

	/*
	 *  Disable interrupts for now.
	 */
	GA_WRITE(base_io, gacfr, NIM | TCM);

	/*
	 *  Set the interrupt and mark the fact that a board has taken
	 *  this interrupt number.
	 */
	GA_WRITE(base_io, idcfr, ie6intr_convert[ bd->irq_level ]);
	ie6intr_to_index[ bd->irq_level ] = bd->bd_number;

	/*
	 *  Put a safe value in the vector registers.
	 */
	GA_WRITE(base_io, vptr2, 0xff);
	GA_WRITE(base_io, vptr1, 0xff);
	GA_WRITE(base_io, vptr0, 0xff);

	return (0);
}

/******************************************************************************
 *  ie6nic_init()
 */
STATIC
ie6nic_init(bd, reset)
DL_bdconfig_t	*bd;
int		reset;
{
	int		base_io = bd->io_start;
	int		i;
	uchar_t		rcv_cnfg;

	/*
	 *  Stop the NIC.  Returns with NIC at register page 0.
	 */
	ie6stop_nic(base_io);

	/*
	 *  Set data configuration register for 8 byte transfers and
	 *  loopback off.
	 */
	NIC_WRITE(base_io, p0.dcr, LS | FT_8_BYTES);

	/*
	 *  Reset transmit byte count.
	 */
	NIC_WRITE(base_io, p0.rbcr_lsb, 0);
	NIC_WRITE(base_io, p0.rbcr_msb, 0);

	/*
	 *  Set receiver configuration.  Allow broadcast is board has not been
	 *  disabled.  It will take an IOCTL request to set multicast and/or
	 *  promiscuous modes.
	 */
	if (!(bd->flags & BOARD_DISABLED))
		rcv_cnfg = AB;
	else
		rcv_cnfg = MON;
	if (bd->promisc_cnt)
		rcv_cnfg |= PRO;
	if (bd->multicast_cnt)
		rcv_cnfg |= AM;

	NIC_WRITE(base_io, p0.rcr, rcv_cnfg);

	/*
	 *  Put it into loopback mode for now.
	 */
	NIC_WRITE(base_io, p0.tcr, INT_LPBK);

	/*
	 *  Do the following if doing a reset of the NIC.
	 */
	if (reset) {
		/*
		 *  Set receive ring and transmit pages.
		 */
		NIC_WRITE(base_io, p0.bnry, RCVSTOP - 1);
		NIC_WRITE(base_io, p0.pstart, RCVSTART);
		NIC_WRITE(base_io, p0.pstop, RCVSTOP);
		NIC_WRITE(base_io, p0.tpsr, XMITSTART);
	
		/*
		 *  Clear interrupt register and set the interrupt mask.
		 */
		NIC_WRITE(base_io, p0.isr, 0xff);
		NIC_WRITE(base_io, p0.imr, INT_MASK);

		/*
		 *  Set DMA address.
		 */
		NIC_WRITE(base_io, p0.cr, STP | DMA_ABORT | PAGE_1);
		NIC_WRITE(base_io, p1.curr, RCVSTART);

		/*
		 *  Set the ethernet address registers.
		 */
		ie6set_nic_eaddr(bd);

		/*
		 *  Turn off all multicast addresses.
		 */
		for (i = 0; i < NIC_MAR_SIZE; i++)
			NIC_WRITE(base_io, p1.mar[ i ], 0);
	}

	/*
	 *  Ready or not, here we come.
	 *  Address page 0 and start the NIC.
	 */
	NIC_WRITE(base_io, p1.cr, STP | DMA_ABORT | PAGE_0);
	NIC_WRITE(base_io, p0.cr, STA | DMA_ABORT | PAGE_0);
	NIC_WRITE(base_io, p0.tcr, NO_LPBK);
	
	return (0);
}

/******************************************************************************
 *  ie6stop_nic()
 */
STATIC void
ie6stop_nic(base_io)
int	base_io;
{
	NIC_WRITE(base_io, p0.cr, STP | DMA_ABORT | PAGE_0);
   /*
   **   This seams to hang on occation!
   **	while ((NIC_READ(base_io, p0.isr) & RSTI) == 0)
   **		;
   */
}

/******************************************************************************
 *  ie6set_nic_eaddr()
 */
STATIC void
ie6set_nic_eaddr(bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	uchar_t		cr_save, cr_new;
	int		i;

	/*
	 *  Save with page we were on before we started then set the
	 *  ethernet address.
	 */
	cr_new  = cr_save = NIC_READ(base_io, p0.cr);
	cr_new |= PS0;
	cr_new &= (~PS1);
	NIC_WRITE(base_io, p0.cr, cr_new);

	for (i = 0; i < NIC_PAR_SIZE; i++)
		NIC_WRITE(base_io, p1.par[ i ], bd->eaddr.bytes[ i ]);

	/*
	 *  Go back to the page we were on when called.
	 */
	NIC_WRITE(base_io, p1.cr, cr_save);
}

/******************************************************************************
 *  ie6chk()
 *
 *  This is a rather weak check to see if the board is installed.  But, if it's
 *  good enough for 3Com, it's good enough for me.  It will return 1 if the
 *  board is found, 0 if not.
 */
STATIC
ie6chk(bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	uchar_t		i, x;

	/*
	 *  See if the I/O address configured in space.c agrees with what
	 *  we find in the base configuration register.
	 */
	for (i = 0x80, x = 0; i ; i >>= 1, x++)
		if (base_io == ie6io_convert[x])
			break;

	/*
	 *  It better match one of the above or space.c has a bogus value!
	 */
	if (!i) {
		cmn_err(CE_WARN, "%s space.c I/0 base address 0x%x is invalid",
						ie6id_string, bd->io_start);
		return (0);
	}
		
	/*
	 *  Make sure the GateArray and we agree.  Since we don't support
	 *  memory mapped configurations, make sure the PROM configuration
	 *  register reads zero.  If both of these test pass, we assume
	 *  we have a properly configured board and space.c file.
	 */
	if ((i == GA_READ(base_io, bcfr)) && (GA_READ(base_io, pcfr) == 0))
		return (1);
	else
		return (0);
}

/******************************************************************************
 *  ie6xmit_packet()
 *
 *  This routine is called from DLunitdate_req() and ie6tx_done().  It assumes
 *  we are at STREAMS spl interrupt level.
 */
ie6xmit_packet(bd, mp)
DL_bdconfig_t *bd;
mblk_t	*mp;
{
	int			base_io   = bd->io_start;
	mblk_t			*mp_tmp;
	int			size;
	uchar_t			page, page_save;
	uchar_t			start_dma;

	/*
	 *  Make sure we are looking at page 0.
	 */
	page  = page_save = NIC_READ(base_io, p0.cr);
	page &= (~PAGE_1 & ~PAGE_2);
	NIC_WRITE(base_io, p0.cr, page | PAGE_0);

	/*
	 *  Set the transmit size.
	 */
	size = msgdsize(mp);
	bd->LAST_TX_SIZE = (caddr_t)size;
	NIC_WRITE(base_io, p0.tbc_lsb, (uchar_t) size);
	NIC_WRITE(base_io, p0.tbc_msb, (uchar_t) (size >> 8));
	NIC_WRITE(base_io, p0.tpsr, XMITSTART);

	/*
	 *  Set xmit DMA address then start DMA.
	 */
	start_dma = (GA_READ(base_io, cr) & ~DDIR) | DOWNLOAD | START;

	GA_WRITE(base_io, dma_msb, XMITSTART);
	GA_WRITE(base_io, dma_lsb, 0);
	GA_WRITE(base_io, cr, start_dma);

	/*
	 *  Copy data to tx buffer.
	 */

	for (mp_tmp = mp; mp_tmp; mp_tmp = mp_tmp->b_cont)
		ie6pio_write(base_io, (caddr_t)mp_tmp->b_rptr, MB_SIZE(mp_tmp));

	GA_WRITE(base_io, cr, start_dma & ~START);	/* flush the FIFO */

	/*
	 *  Start a watchdog timer for the TX lockup problem the NIC has.
	 */
	bd->TX_RETRIES = 0;
	bd->timer_id   = timeout(ie6watchdog, (caddr_t)bd, TX_TIMEOUT);
	bd->flags     |= TX_BUSY;

	/*
	 *  Send the frame on it's way while setting the NIC page back where
	 *  we found it.  Now that it is out of our hands, free the message.
	 */
	NIC_WRITE(base_io, p0.cr, STA | TXP | (page_save & (PAGE_1 | PAGE_2)));

#ifdef NBPFILTER
	if (bd->bpf) {
		mblk_t	*mp_tap;

		if ((mp_tap = allocb(size, BPRI_MED)) != NULL) {

			for (mp_tmp=mp; mp_tmp; mp_tmp=mp_tmp->b_cont) {
				bcopy((caddr_t)mp_tmp->b_rptr,
				      (caddr_t)mp_tap->b_wptr, MB_SIZE(mp_tmp));
				mp_tap->b_wptr += MB_SIZE(mp_tmp);
			}

			bpf_tap(bd->bpf, mp_tap->b_rptr, size);
			freemsg(mp_tap);
		}
	}
#endif
	bd->mib.ifOutOctets += size;	/* SNMP */
	freemsg(mp);
	return (0);
}

/******************************************************************************
 *  ie6intr()
 */
void
ie6intr(level)
int	level;
{
	DL_bdconfig_t	*bd;
	int		base_io;
	int		index;
	uchar_t		intr_status;
	uchar_t		page, page_save;
	uchar_t		tally, err_count;

	/*
	 *  Map the interrupt level to the proper board.  Make sure it's
	 *  from a board we configured.
	 */
	if ((index = ie6intr_to_index[ level ]) == -1) {
		cmn_err(CE_WARN,
		    "%s interrupt from unconfigured board - interrupt level %d",
							ie6id_string, level);
		return;
	}

	bd = &ie6config[ index ];
	base_io = bd->io_start;

	/*
	 *  Make sure we are looking at page 0.
	 */
	page  = page_save = NIC_READ(base_io, p0.cr);
	page &= (~PAGE_1 & ~PAGE_2);
	NIC_WRITE(base_io, p0.cr, page | PAGE_0);
	
	/*
	 *  Disable interrupts then get the interrupt status and clear it.
	 */
	NIC_WRITE(base_io, p0.imr, 0);
	intr_status = NIC_READ(base_io, p0.isr);
	NIC_WRITE(base_io, p0.isr, intr_status);

	/*
	 *  See if we received a packet by check for overwrite warning or
	 *  packet receive.
	 */
	if (intr_status & (PRXI | OVWI))
		ie6chk_ring(bd, intr_status);
	
	/*
	 *  See if we are done transmitting a packet.
	 */
	if (intr_status & (PTXI | TXEI))
		ie6tx_done(bd);
		
	/*
	 *  Restore the page we were on.
	 */
	page  = NIC_READ(base_io, p0.cr) & (~PAGE_1 & ~PAGE_2);
	page |= (page_save & (PAGE_1 | PAGE_2));
	NIC_WRITE(base_io, p0.cr, page);

	/*
	 *  Read and reset tally counters.
	 */
	tally = NIC_READ(base_io, p0.fae_cnt);
	bd->mib.ifSpecific.etherAlignErrors += tally;	/* SNMP */
	err_count = tally;

	tally = NIC_READ(base_io, p0.crc_cnt);
	bd->mib.ifSpecific.etherCRCerrors += tally;	/* SNMP */
	err_count += tally;

	tally = NIC_READ(base_io, p0.missed_cnt);
	bd->mib.ifSpecific.etherMissedPkts += tally;	/* SNMP */
	err_count += tally;
	
	bd->mib.ifInErrors += err_count;		/* SNMP */

	if (ie6ifstats)
		bd->ifstats->ifs_ierrors += err_count;

	/*
	 *  Reset the interrupt mask.
	 */
	NIC_WRITE(base_io, p0.imr, INT_MASK);

	return;
}

/******************************************************************************
 *  ie6chk_ring()
 */
STATIC void
ie6chk_ring(bd, intr_status)
DL_bdconfig_t	*bd;
uchar_t		intr_status;
{
	int		base_io = bd->io_start;
	comb_hdr	hdr;
	uchar_t		cur_page, cr_tmp;
	uchar_t		stop_dma, start_dma;
	int		ring_chk = FRAME_OK;
	int		i;
	mblk_t		*mp;
	DL_sap_t	*sap;
	ushort_t	sap_id;
	
	/*
	 *  If we have an overwrite warning indication, stop the NIC.
	 */
	if (intr_status & OVWI) {
		ie6stop_nic(base_io);
		bd->mib.ifSpecific.etherOverrunErrors++;	/* SNMP */
		DL_LOG(strlog(DL_ID, 101, 2, SL_TRACE,
			"ie6chk_ring - Overrun occured"));
	}

	/*
	 *  Since we process all received frames in the ring during a single
	 *  interrupt, the first unprocessed page should be one greater than
	 *  the page value set in the boundary register (adjusted for ring
	 *  wrap).  We'll stop processing when we hit the value in the
	 *  "current DMA" register.
	 *
	 *  N.B.  We'll have to keep re-reading the "curr" register since
	 *        the NIC can be receiving while we're processing.  We don't
	 *	  update the boundary register as we pull frames.  The current
	 *	  thinking is it isn't cost effective.
	 */
	cur_page = NIC_READ(base_io, p0.bnry) + 1;
	if (cur_page == RCVSTOP)
		cur_page = RCVSTART;

	/*
	 *  Get local DMA start stop values.
	 */
	start_dma = (GA_READ(base_io, cr) & ~DDIR) | UPLOAD | START;
	stop_dma  = start_dma & ~START;

	/*
	 *  Pull out all received packets.
	 *
	 *  N.B.  We will start in page 1 and stay there.  Any called routines
	 *        that address the NIC will have to respect this.
	 */
	for (NIC_WRITE(base_io, p0.cr, STA | DMA_ABORT | PAGE_1);
	 		(cur_page != NIC_READ(base_io, p1.curr)) &&
			(ring_chk != RESET_RING);
					cur_page = hdr.nic.next) {
		/*
		 *  Keep statistices
		 */
		if (DLifstats)
			bd->ifstats->ifs_ipackets++;

		/*
		 *  Stop any previous DMA, setup the DMA address and
		 *  direction, then start DMA and read in the NIC and MAC
		 *  headers.
		 */
		GA_WRITE(base_io, cr, stop_dma);
		GA_WRITE(base_io, dma_msb, cur_page);
		GA_WRITE(base_io, dma_lsb, 0);
		GA_WRITE(base_io, cr, start_dma);
		ie6pio_read(base_io, (uchar_t*)&hdr, sizeof(hdr));

		/*
		 *  Check for bad packets caused by NIC bugs.
		 */
		if (ring_chk = ie6nic_workaround(bd, &hdr, cur_page)) {
			DL_LOG(strlog(DL_ID, 101, 2, SL_TRACE,
				"ie6chk_ring - NIC workaround %d needed", ring_chk));
			continue;
		}

		/*
		 *  Try to allocate a buffer.  The size will include the
		 *  MAC header (which we'll hide later) but not the CRC.
		 */
		if ((mp = ie6mkbuf(bd, &hdr)) == NULL) {
			DL_LOG(strlog(DL_ID, 101, 1, SL_TRACE,
				"ie6chk_ring - No receive buffer resources"));
			bd->mib.ifInDiscards++;			/* SNMP */
			bd->mib.ifSpecific.etherRcvResources++;	/* SNMP */
			continue;
		}

		if (!DLrecv(mp,bd->sap_ptr))
			bd->mib.ifInOctets += hdr.nic.len;	/* SNMP */

#ifdef NBPFILTER
		if (bd->bpf)
			bpf_tap(bd->bpf, mp->b_rptr, MB_SIZE(mp));
#endif
	}
	/*
	 *  Make sure we leave DMA stopped.
	 */
	GA_WRITE(base_io, cr, stop_dma);

	/*
	 *  If we had an error that requires the NIC to be reset, do it.
	 *  Else if we had an overwrite warning, restart the NIC.
	 *  Else, set the new boundary.
	 */
	if (ring_chk == RESET_RING)
		(void)ie6nic_init(bd, 1);
	else if (intr_status & OVWI)
		(void)ie6nic_init(bd, 0);
	else {
		/*
		 *  Get it into page 0.
		 */
		cr_tmp = NIC_READ(base_io, p1.cr) & (~PAGE_1 & ~PAGE_2);
		NIC_WRITE(base_io, p0.cr, cr_tmp | PAGE_0);

		if ( cur_page == RCVSTART)
			NIC_WRITE(base_io, p0.bnry, RCVSTOP - 1);
		else
			NIC_WRITE(base_io, p0.bnry, cur_page - 1);
	}
	return;
}

/******************************************************************************
 *  ie6nic_workaround()
 */
STATIC
ie6nic_workaround(bd, hdr_p, cur_page)
DL_bdconfig_t	*bd;
comb_hdr	*hdr_p;
uchar_t		cur_page;
{
	uchar_t		next1, next2;
	ushort_t	len = hdr_p->nic.len;

	/**********************************************
	 *  Do misaligned header workaround checks.
	 *********************************************/
	/*
	 *  See if the length field makes any sense.
	 *  N.B.  Length currently includes 4 byte CRC.
	 */
	if ((len < (ushort_t) (MIN_ETHER_SIZE + 4)) ||
	    (len > (ushort_t) (MAX_ETHER_SIZE + 4))) {
		DL_LOG(strlog(DL_ID, 102, 2, SL_TRACE,
			"ie6nic _workaround - bad length (%d)", len));
		return (RESET_RING);
	}

	/*
	 *  See if the next pointer ends up where the length of the
	 *  frame says it should. This should be equal to the high-order
	 *  size byte plus 1 or 2 (ajusted for wraparound).
	 */
	next1 = cur_page + LEN_TO_PAGES(len);

	/*
	 *  See if it wrapped the buffer ring.
	 */
	if (next1 >= (ushort_t) RCVSTOP) {
		next1 -= RCVPAGES;
		next2 = next1 + 1;
	} else {
		next2 = next1 + 1;
		if (next2 >= (ushort_t) RCVSTOP)
			next2 -= RCVPAGES;
	}

	/*
	 *  If they don't match, we got a problem.
	 */
	if ((next1 != hdr_p->nic.next) && (next2 != hdr_p->nic.next)) {
		DL_LOG(strlog(DL_ID, 102, 2, SL_TRACE,
			"ie6nic_workaround - next = %d should = %d len = %d",
						hdr_p->nic.next, next1, len));
		return (RESET_RING);
	}

	/**********************************************
	 *  Do active network startup bad packet check.
	 *********************************************/
	/*
	 *  Since we asked not to be bothered with error packets,
	 *  if an error is indicated, something is wrong.  We also
	 *  check for DIS and DFR since this is also a bug indicator.
	 */
	if (hdr_p->nic.status & (CRCE | FAE | FOE | DIS | DFR)) {
		DL_LOG(strlog(DL_ID, 102, 2, SL_TRACE,
			"ie6nic_workaround - Bad status (%x)",
							hdr_p->nic.status));
		return (SKIP_FRAME);
	}
	
#ifdef DO_ADDRESS_CHECK
	{
	register int	i;

	/**********************************************
	 *  Do the all even bytes moved downline by 2 check.
	 *********************************************/
	/*
	 *  If the interface is set for promiscuous mode, there is no
	 *  way to know if this is a good address.
	 */
	if (bd->promisc_cnt)
		return (FRAME_OK);

	/*
	 *  Check for broadcast/multicast first since this is more likely in
	 *  typical networks.
	 */
	if (hdr_p->nic.status & PHY) {        /* PHY bit true means multicast */
		if (!IS_MULTICAST(hdr_p->mac.dst)) {
			DL_LOG(strlog(DL_ID, 102, 2, SL_TRACE,
				"ie6nic_workaround - PHY mismatch"));
			return (SKIP_FRAME);
		}

		for (i = 0; i < (sizeof (eaddr_t) / 2); i++) {
			if (hdr_p->mac.dst.words[ i ] != 0xffff) {
				/*
				 *  Multicast address check would
				 *  go here.
				 */
				DL_LOG(strlog(DL_ID, 102, 2, SL_TRACE,
				    "ie6nic_workaround - broadcast mismatch"));
				return (SKIP_FRAME);
			}
		}
		return (FRAME_OK);
	}
			
	/*
	 *  Check for our address.
 	 */
	for (i = 0; i < (sizeof (eaddr_t) / 2); i++)
		if (bd->eaddr.words[i] != hdr_p->mac.dst.words[i]) {
			DL_LOG(strlog(DL_ID, 102, 2, SL_TRACE,
				"ie6nic_workaround - address mismatch"));
			return (SKIP_FRAME);
		}
	}
#endif
	return (FRAME_OK);
}

/******************************************************************************
 *  ie6tx_done()
 */
void
ie6tx_done(bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	DL_sap_t	*sap;
	mblk_t		*mp;
	uchar_t		tx_status;
	int		x, next, max_saps;

	DL_LOG(strlog(DL_ID, 103, 3, SL_TRACE, "ie6tx_done for board %d",
								bd->bd_number));

	/*
	 *  Stop the watchdog timer.
	 */
	untimeout(bd->timer_id);

	/*
	 *  Read the tx status register and see if there were any problems.
	 */
	if (ie6ifstats)
		bd->ifstats->ifs_opackets++;

	tx_status = NIC_READ(base_io, p0.tsr);

	if (!(tx_status & PTX)) {
		if (ie6ifstats)
			bd->ifstats->ifs_oerrors++;

		if (tx_status & COL) {
			bd->mib.ifSpecific.etherCollisions +=
					NIC_READ(base_io, p0.ncr);  /* SNMP */
			if (ie6ifstats)
				bd->ifstats->ifs_collisions++;
		}
		if (tx_status & ABT)
			bd->mib.ifSpecific.etherAbortErrors++;	    /* SNMP */
		if (tx_status & CRS)
			bd->mib.ifSpecific.etherCarrierLost++;	    /* SNMP */
		if (tx_status & FU)
			bd->mib.ifSpecific.etherUnderrunErrors++;   /* SNMP */
		if (tx_status & CDH)
			bd->mib.ifSpecific.CD_HEARTBEAT++;	    /* SNMP */
		if (tx_status & OWC)
			bd->mib.ifSpecific.OUT_OF_WINDOW_COLLISION++; /* SNMP */
		bd->mib.ifOutErrors++;				    /* SNMP */
	}

	/*
	 *  Indicate outstanding transmit is done and see if there is work
	 *  waiting on our queue.
	 */
	bd->flags &= ~TX_BUSY;

	if (bd->flags & TX_QUEUED) {
		next     = bd->tx_next;
		sap      = &ie6saps[ next ];
		max_saps = bd->max_saps;

		for (x = max_saps; x; x--) {
			if (++next == max_saps)
				next = 0;
			/*
			 *  If it is a bound SAP and has a message queued,
			 *  send it.
			 */
			if ((sap->state == DL_IDLE) && (mp = getq(sap->write_q))){
				(void)ie6xmit_packet(bd, mp);
				bd->tx_next = next;
				bd->mib.ifOutQlen--;		/* SNMP */
				return;
			}

			if(next == 0)
				sap = ie6saps;
			else
				sap++;
		}

		/*
		 *  Nobody's left to service, make the queue empty.
		 */
		bd->flags &= ~TX_QUEUED;
		bd->mib.ifOutQlen = 0;		/* SNMP */
	}
}

/******************************************************************************
 *  ie6watchdog()
 */
STATIC void
ie6watchdog(bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	uchar_t		page_save;

	DL_LOG(strlog(DL_ID, 104, 3, SL_TRACE,
		"ie6watchdog called for board %d", bd->bd_number));

	/*
	 *  See if we really have busy hardware since we may have been
	 *  delayed getting here.
	 */
	if (!(bd->flags & TX_BUSY))
		return;

	/*
	 *  Check our retry count to see if we have some problem other than
	 *  the NIC TX lockup.
	 */
	bd->mib.ifSpecific.TX_TIMEOUTS++;		/* SNMP */

	if (++bd->TX_RETRIES == (caddr_t)MAX_TX_RETRIES) {
		cmn_err(CE_WARN,
		    "%s not generating transmit interrupts - disabling board",
								ie6id_string);
		ie6stop_nic(base_io);

		GA_WRITE(base_io, cr, RST | XSEL);
		GA_WRITE(base_io, cr, XSEL);
		GA_WRITE(base_io, cr, XSEL);

		bd->flags &= ~BOARD_PRESENT;
		bd->mib.ifOperStatus  = DL_DOWN;	/* SNMP */

		return;
	}

	/*
	 *  Save the current page then stop the NIC.
	 */
	page_save = NIC_READ(base_io, p0.cr);
	ie6stop_nic(base_io);
	
	/*
	 *  Set the last transmit size and send the frame on it's way.
	 */
	bd->timer_id = timeout(ie6watchdog, (caddr_t)bd, TX_TIMEOUT);
	NIC_WRITE(base_io, p0.tbc_lsb, (uchar_t) bd->LAST_TX_SIZE);
	NIC_WRITE(base_io, p0.tbc_msb, (uchar_t) bd->LAST_TX_SIZE >> 8);
	NIC_WRITE(base_io, p0.tpsr, XMITSTART);
	NIC_WRITE(base_io, p0.cr, STA | TXP | (page_save & (PAGE_1 | PAGE_2)));
}

/******************************************************************************
 *  ie6mkbuf()
 */
STATIC mblk_t*
ie6mkbuf(bd, hdr)
DL_bdconfig_t	*bd;
comb_hdr	*hdr;
{
	int	base_io = bd->io_start;
	mblk_t	*mp;

	/*
	 *  Adjust length to toss CRC and allocate a buffer.
	 */
	hdr->nic.len -= 4;
	if ((mp = allocb(hdr->nic.len, BPRI_MED)) == NULL)
		return (NULL);

	/*
	 *  Move the MAC header we already read.
	 */
	bcopy((caddr_t)&hdr->mac, (caddr_t)mp->b_wptr, sizeof(hdr->mac));
	mp->b_wptr   += sizeof(hdr->mac);
	hdr->nic.len -= sizeof(hdr->mac);

	/*
	 *  Call a fast programmed I/O read routine.
	 */
	ie6pio_read(base_io, mp->b_wptr, hdr->nic.len);
	mp->b_wptr += hdr->nic.len;
	return (mp);
}

/******************************************************************************
 *  ie6promisc_on()
 */
ie6promisc_on(bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	uchar_t		page_save, rcv_cnfg;
	int		old;

	/*
	 *  If already in promiscuous mode, just return.
	 */
	if (bd->promisc_cnt++)
		return (0);

	/*
	 *  Save the current page then go to page two and read the current
	 *  receive configuration.
	 */
	old = splstr();

	page_save = NIC_READ(base_io, p0.cr);
	NIC_WRITE(base_io, p0.cr, (page_save & ~(PAGE_0 | PAGE_1)) | PAGE_2);
	rcv_cnfg = NIC_READ(base_io, p2.rcr) | PRO;

	/*
	 *  Go to page 0 and set receive configuration for promiscuous mode.
	 */
	NIC_WRITE(base_io, p2.cr, (page_save & ~(PAGE_1 | PAGE_2)) | PAGE_0);
	NIC_WRITE(base_io, p0.rcr, rcv_cnfg);

	/*
	 * put things back the way we found them.
	 */
	NIC_WRITE(base_io, p0.cr, page_save);

	splx(old);

	return (0);
}

/******************************************************************************
 *  ie6promisc_off()
 */
ie6promisc_off(bd)
DL_bdconfig_t	*bd;
{
	int		base_io = bd->io_start;
	uchar_t		page_save, rcv_cnfg;
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

	page_save = NIC_READ(base_io, p0.cr);
	NIC_WRITE(base_io, p0.cr, (page_save & ~(PAGE_0 | PAGE_1)) | PAGE_2);
	rcv_cnfg = NIC_READ(base_io, p2.rcr) & ~PRO;

	/*
	 *  Go to page 0 and set receive configuration for promiscuous mode.
	 */
	NIC_WRITE(base_io, p2.cr, (page_save & ~(PAGE_1 | PAGE_2)) | PAGE_0);
	NIC_WRITE(base_io, p0.rcr, rcv_cnfg);

	/*
	 * put things back the way we found them.
	 */
	NIC_WRITE(base_io, p0.cr, page_save);

	splx(old);

	return (0);
}

#ifdef ALLOW_SET_EADDR
/******************************************************************************
 *  ie6set_eaddr()
 */
ie6set_eaddr(bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	return (1);	/* not supported yet */
}
#endif

/******************************************************************************
 *  ie6add_multicast()
 */
int
ie6add_multicast(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
	bdd_t *bdd = (bdd_t *)bd->bd_dependent3;
	register struct ie6maddr *mcp = bdd->ie6_multiaddrs;
	register int i;
	int row, col;
	unsigned char val;
	int oldlevel;
	uchar_t page, page_save;

	if ((bd->multicast_cnt >= ie6_multisize) || (!(eaddr->bytes[0] & 0x1)))
		return 1;
	if (ie6is_multicast(bd, eaddr))
		return 1;
	for (i = 0; i < ie6_multisize; i++,mcp++) {
		if (mcp->entry[0] == 0x00)
			break;
	}
	bd->multicast_cnt++;
	bcopy((caddr_t)(eaddr->bytes), (caddr_t)mcp->entry, DL_MAC_ADDR_LEN);
	mcp->filterbit = ie6hash(mcp->entry);
	row = mcp->filterbit / 8;
	col = mcp->filterbit % 8;

	oldlevel = splstr();
	page = page_save = NIC_READ(bd->io_start, p0.cr);
	page &= (~PAGE_0 & ~PAGE_2);
	NIC_WRITE(bd->io_start, p0.cr, page | PAGE_1);
	val = NIC_READ(bd->io_start, p1.mar[ row ]);
	val |= 0x01 << col;
	NIC_WRITE(bd->io_start, p1.mar[ row ], val);
	NIC_WRITE(bd->io_start, p0.cr, (page_save & (PAGE_0 | PAGE_2)));

	splx(oldlevel);	 

	return (0);
}

/******************************************************************************
 *  ie6del_multicast()
 */
int
ie6del_multicast(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
	bdd_t *bdd = (bdd_t *)bd->bd_dependent3;
	register struct ie6maddr *mcp = bdd->ie6_multiaddrs;
	register int i;
	int row, col;
	unsigned char val, index;
	int oldlevel;
	uchar_t page, page_save;
	int base_io = bd->io_start;

	if (!ie6is_multicast(bd, eaddr))
		return 1;
	for (i = 0; i < ie6_multisize; i++, mcp++) {
		if (BCMP(eaddr->bytes, mcp->entry, DL_MAC_ADDR_LEN) == 0)
			break;
	}

	oldlevel = splstr();

	for (i = 0; i < DL_MAC_ADDR_LEN; i++) 
		mcp->entry[i] = 0x00;
	mcp->filterbit = 0xff;
	bd->multicast_cnt--;
	index = ie6hash(mcp->entry);
	page = page_save = NIC_READ(base_io, p0.cr);
	page &= (~PAGE_0 & ~PAGE_2);
	NIC_WRITE(base_io, p0.cr, page | PAGE_1);
	row = index / 8;
	col = index % 8;
	val = NIC_READ(base_io, p1.mar[ row ]);
	val &= ~(0x01 << col);
	NIC_WRITE(base_io, p1.mar[ row ], val);
	NIC_WRITE(base_io, p0.cr, (page_save & (PAGE_0 | PAGE_2)));

	splx(oldlevel);	 

	return (0);
}

/******************************************************************************
 *  ie6get_multicast()
 */
int
ie6get_multicast(bd, mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
	bdd_t *bdd = (bdd_t *)bd->bd_dependent3;
	register struct ie6maddr *mcp;
	register int i;
	int found = bd->multicast_cnt;
	unsigned char *dp;

	if ((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		dp = mp->b_rptr;
		mcp = bdd->ie6_multiaddrs;
		for (i = 0; (i < ie6_multisize) && (dp < mp->b_wptr); i++, mcp++)
			if (mcp->entry[0]) {
				BCOPY(mcp->entry, dp, DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
        }
	return found;
}

/******************************************************************************
 *  ie6disable()
 */
ie6disable(bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ie6enable()
 */
ie6enable(bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ie6reset()
 */
ie6reset(bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ie6is_multicast()
 */
int
ie6is_multicast(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
	bdd_t *bdd = (bdd_t *)bd->bd_dependent3;
	register struct ie6maddr *mcp = bdd->ie6_multiaddrs;
	register int i;
	int oldlevel;

	if (bd->multicast_cnt == 0)
		return (0);
	oldlevel = splstr();
	for (i = 0; i < ie6_multisize; i++, mcp++) {
		if (mcp->entry[0] == 0x00)
			continue;
		if (BCMP(eaddr->bytes, mcp->entry, DL_MAC_ADDR_LEN) == 0) {
			splx(oldlevel);
			return (1);
		}
	}
	splx(oldlevel);

	return (0);
}

unsigned char
ie6hash(addr)
unsigned char addr[];
{
	register int i, j;
	union crc_reg crc;
	uchar_t fb, ch;

        crc.value = (unsigned int) -1;
        for(i = 0; i < LLC_ADDR_LEN; i++) {
            ch = addr[i];
            for (j = 0; j < 8; j++) {
                fb = crc.bits.a31 ^ ((ch >> j) & 0x01);
                crc.bits.a25 ^= fb;
                crc.bits.a22 ^= fb;
                crc.bits.a21 ^= fb;
                crc.bits.a15 ^= fb;
                crc.bits.a11 ^= fb;
                crc.bits.a10 ^= fb;
                crc.bits.a9 ^= fb;
                crc.bits.a7 ^= fb;
                crc.bits.a6 ^= fb;
                crc.bits.a4 ^= fb;
                crc.bits.a3 ^= fb;
                crc.bits.a1 ^= fb;
                crc.bits.a0 ^= fb;
                crc.value = (crc.value << 1) | fb;
            }
        }
        return((uchar_t)(crc.value >> 26));
}

#ifdef C_PIO
/******************************************************************************
 *  ie6pio_read()
 */
STATIC void
ie6pio_read(base_io, dst, count)
register int		base_io;
register uchar_t	*dst;
	 int		count;
{
	int	i;
	int	limit = 1;	/* just to get past the first while */

	while (count && limit) {
		for (limit = 1000;
			((GA_READ(base_io, streg) & DPRDY) == 0) && limit;
									limit--)
			;
		for (i = count < 8 ? count : 8, count -= i; i; i--)
			*dst++ = GA_READ(base_io, rf_msb);
	}
	if (limit == 0)
		cmn_err(CE_WARN, "%s pio_read() read timeout", ie6id_string);
}
/******************************************************************************
 *  ie6pio_write()
 */
STATIC void
ie6pio_write(base_io, src, count)
register int		base_io;
register uchar_t	*src;
	 int		count;
{
	int	i;
	int	limit = 1;	/* just to get past the first while */

	while (count && limit) {
		for (limit = 1000;
			((GA_READ(base_io, streg) & DPRDY) == 0) && limit;
									limit--)
			;
		for (i = count < 8 ? count : 8, count -= i; i; i--)
			GA_WRITE(base_io, rf_msb, *src++);
	}
	if (limit == 0)
		cmn_err(CE_WARN, "%s pio_write() write timeout", ie6id_string);
}
#endif

/******************************************************************************
 *  Support routines for Berkeley Packet Filter (BPF).
 */
#ifdef NBPFILTER

STATIC
ie6bpf_ioctl(ifp, cmd, addr)
struct	ifnet	*ifp;
int	cmd;
{
	return(EINVAL);
}

STATIC
ie6bpf_output(ifp, buf, dst)
struct	ifnet	*ifp;
uchar_t	*buf;
struct	sockaddr *dst;
{
	return(EINVAL);
}

STATIC
ie6bpf_promisc(ifp, flag)
struct	ifnet	*ifp;
int	flag;
{
	if (flag)
		return(ie6promisc_on((DL_bdconfig_t*)ifp->if_next));
	else
		return(ie6promisc_off((DL_bdconfig_t*)ifp->if_next));
}
#endif
