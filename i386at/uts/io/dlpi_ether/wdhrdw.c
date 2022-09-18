/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Module: WD8003
 * Project: System V ViaNet
 *
 *
 *	Copyright (c) 1987, 1988, 1989 by Western Digital Corporation and
 *	Interactive Systems Corporation.
 *	All rights reserved.  Contains confidential information and
 *	trade secrets proprietary to
 *		Western Digital Corporation
 *		2445 McCabe Way
 *		Irvine, California  92714
 *
 *		Interactive Systems Corporation
 *		2401 Colorado Avenue
 *		Santa Monica, California  90404
 */

#ident	"@(#)uts-x86at:io/dlpi_ether/wdhrdw.c	1.9"
#ident	"$Header: $"

/*
 * Streams driver for WD 8003 Ethernet/Starlan controller
 * Implements an extended version of the AT&T Data Link Provider Interface
 * IEEE 802.2 Class 1 type 1 protocol is implemented and supports
 * receipt and response to XID and TEST but does not generate them
 * otherwise.
 * Ethernet encapsulation is also supported by binding to a SAP greater
 * than 0xFF.
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
#include <io/ddi_i386at.h>
#include <svc/systm.h>
#ifndef lint
#include <net/tcpip/byteorder.h>
#endif
#include <io/rtc/rtc.h>
#include <util/cmn_err.h>
#include <io/dlpi_ether/wd.h>
#include <io/dlpi_ether/dlpi_wd.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <mem/kmem.h>

/*
 * Header, wrapper, and function declarations and definitions
 * for loadability support...
 */

#include <util/mod/moddefs.h>

#define	DRVNAME	"wd - Loadable wd Ethernet driver"

STATIC	int	wd_load(), wd_unload();
void		wdinit();
STATIC 	void	wduninit();

MOD_DRV_WRAPPER(wd, wd_load, wd_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

STATIC int
wd_load(void)
{
	cmn_err(CE_NOTE, "!MOD: in wd_load()");

	wdinit();
	mod_drvattach(&wd_attach_info);
	return(0);
}

STATIC int
wd_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in wd_unload()");

	mod_drvdetach(&wd_attach_info);
	wduninit();
	return(0);
}

#define nextsize(len) ((len+64)*2)

/* This define is to make this file compile */
/* will be clarified later */
#define dl_max_idu dl_reserved2

/* AT systems initialize in space.c; PS/2 systems determine info at init time*/

extern int wdboards;			/* number of boards */
extern DL_sap_t wdsaps[];		/* queue specific parameters */
extern DL_bdconfig_t wdconfig[];	/* board specific parameters */
extern int wd_multisize;		/* number of multi addrs per board */
extern struct wdmaddr wdmultiaddrs[];   /* mulicast addr entries */

char wdid_string[] = "WD (wd8003) v1.1";
int  slot_count;

extern int wdinetstats;
extern char *wd_ifname;
extern struct ifstats *ifstats;
struct ifstats *wdifstats;
extern int wdstrlog;

extern int	wdxmit_packet();
extern int	wdbind(), wdunbind(), wdunitdata(), wdinforeq();
extern void	wdbcopy(), wdinit_board(),wdfill_stat();
static int	get_slot();
unsigned int	bus_p;

extern void	bcopy(), bzero();
extern int	initqparms();
extern int 	DLis_validsnap();

STATIC void	wdwatchdog();

/*
 *  This is for lint.
 */
#define BCOPY(from, to, len) bcopy((caddr_t)(from),(caddr_t)(to),(size_t)(len))
#define BCMP(s1, s2, len) bcmp((char*)(s1),(char*)(s2),(size_t)(len))

char wdcopyright[] = "Copyright 1987, 1988, 1989 Interactive Systems Corporation\n\tand Western Digital Corporation\nAll Rights Reserved.";
  /*
   * initialize the necessary data structures for the driver
   * to get called.
   * the hi/low water marks will have to be tuned
   */

extern int wdopen(), wdclose(), wdwput(), wdrsrv();
extern wdsched();
extern unsigned char wdhash();

unsigned char wdbroadaddr[LLC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define SRC_ALIGN	0
#define DEST_ALIGN	1
extern inb();			/* i/o in byte routine */
extern void outb();			/* i/o out byte routine */

/* printether - print ethernet address in compact form */
static char *hex[] = {
   "0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"
};
static void
printether(addr)
     unsigned char *addr;
{
int i;

	for (i=0; i < 6; i++, addr++){
		if (i!=0)
			cmn_err(CE_CONT, "!:");
      		cmn_err(CE_CONT, "!%s%s", hex[(*addr >> 4)&0xf], hex[*addr&0xf]);
   	}
   	cmn_err(CE_CONT, "\n");
}

/*
 * wdinit is called at boot time to do any initialization needed
 * it is not called with any parameters and is called only once.
 * if multiple boards exist, they must get handled at this time.
 */

void
wdinit()
{
extern	caddr_t sptalloc();
unsigned char tmp;
register int i, j;
int x;
bdd_t *wdpp;
DL_sap_t *sap;
struct bdconfig *wdp;

/* the following are for MCA support */
int slot_num;	/* Slot # found for adapter */
char pos_regs[8];	/* Array of POS register data */
unsigned option;	/* temp for POS information */
static unsigned adaptid[] = { ETI_ID, STA_ID, WA_ID, WD_ID };

    cmn_err(CE_CONT, "%s\n", wdcopyright);
    cmn_err(CE_CONT, "!%s ", wdid_string);

	if (wdinetstats)
		wdifstats = (struct ifstats*) kmem_zalloc((sizeof(struct ifstats) * wdboards), KM_NOSLEEP);
    	else
		wdifstats = 0;

	if ((drv_gethardware(IOBUS_TYPE, &bus_p)) < 0) {
		cmn_err(CE_WARN,"wd: can't decide bus type");
		exit (1);
	}

	slot_count = 0;
    for (i = 0,wdp = wdconfig; i < wdboards; i++, wdp++) {
	if (bus_p & BUS_MCA) { /* for MCA support */
	    for (j = 0; j < NUM_ID; j++) {
		if ((slot_num = get_slot (adaptid[j],pos_regs, wdp)) != -1) {
			slot_count++;
		    	break;
		}
	    }
	    if (slot_num == -1) {
		cmn_err(CE_WARN,"wd: board (%d of %d) not present.", i+1, wdboards);
		wdp->flags |= BOARD_DISABLED;
		continue;
	    }
	    /* Now for this slot_num, get the POS register information */
       
	    option = (unsigned) pos_regs[2] & 0xFE;
	    wdp->io_start = option << 4;		/* Base I/O address */
	    option = (unsigned) (pos_regs[3] & 0xFC);
	    wdp->mem_start = (paddr_t) (option << 12);  /* RAM base address */
        } /* end of PS2 specific code */
	tmp = 0;
	wdp->flags = 0;
	bzero( (caddr_t)&wdp->mib,sizeof(DL_mib_t));
	for (j = 0; j < 8; j++)
	    tmp += inb(wdp->io_start + UBA_STA + j);
	if (tmp != (unsigned char) 0xFF) {
		cmn_err(CE_WARN, "wd: board not found\n");
		wdp->flags |= BOARD_DISABLED;
		wdp->mib.ifOperStatus =  DL_DOWN;
		wdp->mib.ifAdminStatus =  DL_DOWN;
		continue;
	}
	wdp->flags |= BOARD_PRESENT;
	wdp->bd_number = i;
	wdp->tx_next = 0;
	wdp->promisc_cnt = 0;
	wdp->multicast_cnt = 0;
	wdp->sap_ptr = &wdsaps[i * wdp->max_saps];
	if( (wdp->bd_dependent1 = (char *)kmem_zalloc(sizeof(bdd_t),KM_NOSLEEP))
			== NULL) {
		wdp->flags |= BOARD_DISABLED;
		cmn_err(CE_CONT,"Unable to allocate dependent structure\n");
		continue;
	}
	wdpp = (bdd_t *)wdp->bd_dependent1;
	wdpp->wd_firstd = i * wdp->max_saps;
	if (bus_p & BUS_MCA)  /* for MCA support */
		wdpp->wd_memsize = PS2_RAMSZ;
	else
		wdpp->wd_memsize = (wdp->mem_end - wdp->mem_start) + 1;
	wdpp->wd_rambase = (char *)sptalloc(btoc(wdpp->wd_memsize), PG_V,
					btoc(wdp->mem_start),0);
	wdpp->wd_multiaddrs = &wdmultiaddrs[i * wd_multisize];
	for (j = 0; j < DL_MAC_ADDR_LEN; j++)
	    wdp->eaddr.bytes[j] = inb(wdp->io_start + UBA_STA + j);

	wdpp->wd_boardtype = inb(wdp->io_start + UBA_STA + DL_MAC_ADDR_LEN);
	cmn_err(CE_CONT, "!type %d board %d address: ", wdpp->wd_boardtype, i);
	for (sap = wdp->sap_ptr,x = 0;x < wdp->max_saps;x++,sap++) {
		sap->state = DL_UNBOUND;
		sap->mac_type = DL_ETHER;
		sap->sap_addr = 0;
		sap->read_q = NULL;
		sap->write_q = NULL;
		sap->flags = 0;
		sap->max_spdu = DL_MAX_PACKET;
		sap->service_mode = DL_CLDLS;
		sap->provider_style = DL_STYLE1;
		sap->bd = wdp;
		sap->next_sap = NULL;
	}
	if (wdifstats) {
                wdifstats[i].ifs_name = wd_ifname;
                wdifstats[i].ifs_unit = (short)i;
                wdifstats[i].ifs_active = 1; 
                wdifstats[i].ifs_next = ifstats;
                wdifstats[i].ifs_mtu = USER_MAX_SIZE; 
                ifstats = &wdifstats[i];
	}
	wdp->valid_sap = NULL;
	wdp->mib.ifAdminStatus = DL_UP;
	wdinit_board(wdp);
	wdp->mib.ifOperStatus =  DL_UP;
	printether(wdp->eaddr.bytes);
    }
}

void
wdinit_board(wdp)
DL_bdconfig_t *wdp;
{
register int i;
register int inval;
short ctl_reg, cmd_reg;
bdd_t *wdpp = (bdd_t *)wdp->bd_dependent1;

	ctl_reg = wdp->io_start;
	cmd_reg = ctl_reg + 0x10;

   /* reset the 8003 & program the memory decode bits */
   	outb(ctl_reg, SFTRST);
   	outb(ctl_reg, 0);
   	outb(ctl_reg, (char)(((long) wdp->mem_start >> 13) & 0x3F) + MEMENA);

   /* initialize the 8390 lan controller device */
   	inval = inb(cmd_reg);
   	outb(cmd_reg, inval & PG_MSK);
	if (bus_p & BUS_MCA) { /* for MCA support */
		inval = inb(ctl_reg + CCR);
		outb(ctl_reg + CCR, inval | EIL);
		outb(cmd_reg + DCR, INIT_DCR | WTS);
	} else
		outb(cmd_reg + DCR, INIT_DCR);
   	outb(cmd_reg + RBCR0, 0);
   	outb(cmd_reg + RBCR1, 0);
   	outb(cmd_reg + RCR, RCRMON);
   	outb(cmd_reg + TCR, INIT_TCR);

   	outb(cmd_reg + PSTART, TX_BUF_LEN >> 8);
   	outb(cmd_reg + BNRY, TX_BUF_LEN >> 8);
   	outb(cmd_reg + PSTOP, wdpp->wd_memsize >> 8); /* ??? */

   	outb(cmd_reg + ISR, CLR_INT);
   	outb(cmd_reg + IMR, INIT_IMR);

   	inval = inb(cmd_reg);
   	outb(cmd_reg, (inval & PG_MSK) | PAGE_1);
   	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
     		outb(cmd_reg + PAR0 + i, wdp->eaddr.bytes[i]);
   /*
    *  clear the multicast filter bits
    */
   	for (i = 0; i < 8; i++)
     		outb(cmd_reg + MAR0 + i, 0);

   	outb(cmd_reg + CURR, TX_BUF_LEN >> 8);
   	wdpp->wd_nxtpkt = (unsigned char)((TX_BUF_LEN >> 8));

   	outb(cmd_reg, PAGE_0 + ABR + STA);
   	outb(cmd_reg + RCR, INIT_RCR);

   /* clear status counters */
}

wdpromisc_off(bd)
DL_bdconfig_t *bd;
{
short cmd_reg = bd->io_start + 0x10;
int oldlevel;

	bd->promisc_cnt--;
	oldlevel = splstr();
	if (bd->promisc_cnt <= 0) {
		outb(cmd_reg,PAGE_0);
		outb(cmd_reg + RCR,INIT_RCR);
	}
	splx(oldlevel);
	return 0;
}

wdpromisc_on(bd)
DL_bdconfig_t *bd;
{
short cmd_reg = bd->io_start + 0x10;
int oldlevel;

	bd->promisc_cnt++;

	oldlevel = splstr();
	if (bd->promisc_cnt == 1) {
		outb(cmd_reg,PAGE_0);
		outb(cmd_reg + RCR,INIT_RCR|PRO);
	}
	splx(oldlevel);
	return 0;
}

int
wdset_eaddr(bd,eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
	short cmd_reg;
	int oldlevel;
	register int i;
	
	cmd_reg = bd->io_start + 0x10;

	oldlevel = splstr();
	
	bcopy((caddr_t)eaddr->bytes,(caddr_t)bd->eaddr.bytes,DL_MAC_ADDR_LEN);
	outb(cmd_reg,PAGE_1);
	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		outb(cmd_reg+PAR0+i,bd->eaddr.bytes[i]); 
	outb(cmd_reg,PAGE_0);

	splx(oldlevel);

	return 0;
}


int
wdadd_multicast(bd,eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
bdd_t *wdpp = (bdd_t *)bd->bd_dependent1;
register struct wdmaddr * mcp = wdpp->wd_multiaddrs;
register int i;
int row,col,err = 0;
unsigned char val;
short cmd_reg;
int oldlevel;
int inval;

	if ((bd->multicast_cnt >= wd_multisize)|| (!(eaddr->bytes[0] & 0x1)))
		return 1;
	
	if (wdis_multicast(bd,eaddr))
		return 1;
	for (i = 0; i < wd_multisize; i++,mcp++) {
		if (mcp->entry[0] == 0x00)
			break;
	}
	bd->multicast_cnt++;
	bcopy((caddr_t)(eaddr->bytes),(caddr_t)mcp->entry,DL_MAC_ADDR_LEN);
	mcp->filterbit = wdhash(mcp->entry);
	row = mcp->filterbit / 8;
	col = mcp->filterbit % 8;
	cmd_reg = bd->io_start + 0x10;

	oldlevel = splstr();

	inval = inb(cmd_reg);
	outb(cmd_reg,(inval &PG_MSK)|PAGE_1); 	/*  Select Page 1 */

	val = inb(cmd_reg + MAR0 + row);
	val |= 0x01 << col;
	outb(cmd_reg+MAR0+row,val);

	inval = inb(cmd_reg);
	outb(cmd_reg,(inval &PG_MSK));		/* Put back Page 0 */

	splx(oldlevel);	 

	return (0);
}

int
wddel_multicast(bd,eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
bdd_t *wdpp = (bdd_t *)bd->bd_dependent1;
register struct wdmaddr * mcp = wdpp->wd_multiaddrs;
register int i;
int row,col,err = 0;
unsigned char val,index;
short cmd_reg;
int oldlevel;
int inval;

	if (!wdis_multicast(bd,eaddr))
		return 1;
	for (i = 0; i < wd_multisize; i++,mcp++) {
		if ( BCMP(eaddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0)
			break;
	}
	cmd_reg = bd->io_start + 0x10;

	oldlevel = splstr();

	for (i = 0; i < DL_MAC_ADDR_LEN; i++) 
		mcp->entry[i] = 0x00;
	mcp->filterbit = 0xff;
	bd->multicast_cnt--;
	index = wdhash(mcp->entry);

	inval = inb(cmd_reg);
	outb(cmd_reg,(inval &PG_MSK)|PAGE_1); 	/*  Select Page 1 */

	row = index / 8;
	col = index % 8;
	val = inb(cmd_reg + MAR0 + row);
	val &= ~(0x01 << col);
	outb(cmd_reg + MAR0 +row,val);

	inval = inb(cmd_reg);
	outb(cmd_reg,(inval &PG_MSK));		/* Put back Page 0 */
	splx(oldlevel);

	return (0);	
}

int
wdget_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
bdd_t *wdpp = (bdd_t *)bd->bd_dependent1;
register struct wdmaddr *mcp;
register int i;
int found = bd->multicast_cnt;
unsigned char *dp;

	if ((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		dp = mp->b_rptr;
		mcp = wdpp->wd_multiaddrs;
		for (i = 0;(i < wd_multisize) && (dp < mp->b_wptr);i++,mcp++)
			if (mcp->entry[0]) {
				BCOPY(mcp->entry,dp,DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
        }
	return found;
}

int
wddisable(bd)
DL_bdconfig_t *bd;
{
	return(1);
}

int
wdenable(bd)
DL_bdconfig_t *bd;
{
	return(1);
}

int
wdreset(bd)
DL_bdconfig_t *bd;
{
	return(1);
}

int
wdis_multicast(bd,eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
bdd_t *wdpp = (bdd_t *)bd->bd_dependent1;
register struct wdmaddr *mcp = wdpp->wd_multiaddrs;
register int i;
int oldlevel;
	if (bd->multicast_cnt == 0)
		return (0);
	oldlevel = splstr();
	for (i = 0; i < wd_multisize; i++,mcp++) {
		if (mcp->entry[0] == 0x00)
			continue;
		if (BCMP(eaddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0)
			return (1);
	}
	splx(oldlevel);
	return (0);
}

void
wdbdspecioctl(q,mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *iocp;
	DL_sap_t *sap = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t  *wdp = sap->bd;
	bdd_t *wdpp = (bdd_t *)wdp->bd_dependent1;
	mblk_t *bt;
	unsigned char *dp;
	struct wdstat *pstat;	
	
	iocp = (struct iocblk *)mp->b_rptr;

	switch(iocp->ioc_cmd) {
	case NET_WDBRDTYPE:
		if ((bt= allocb(sizeof(wdpp->wd_boardtype), BPRI_MED)) == NULL){
              		iocp->ioc_error = ENOSR;
              		goto iocnak;
      		}
		bcopy((caddr_t)&(wdpp->wd_boardtype),(caddr_t)bt->b_wptr,sizeof(wdpp->wd_boardtype));
		bt->b_wptr += sizeof(wdpp->wd_boardtype);
		linkb(mp,bt);
		iocp->ioc_count = sizeof(wdpp->wd_boardtype);
		break;
	case NET_ADDR:
		if (iocp->ioc_count != DL_MAC_ADDR_LEN) {
          		iocp->ioc_error = EINVAL;
          		goto iocnak;
       		}
		dp = mp->b_cont->b_rptr;
		bcopy( (caddr_t)wdp->eaddr.bytes,(caddr_t)dp,DL_MAC_ADDR_LEN);
		break;
		/*
		   This version of the wd driver does not maintain/update the 
		   stat fields in the wdstat structure. However, the following 
		   ioctls are provided for backward compatibility.
		*/
	case NET_GETSTATUS:
		if (mp->b_cont == NULL || iocp->ioc_count <= 0) {
			iocp->ioc_error = EINVAL;
			goto iocnak;
		}
		if ((bt = allocb(sizeof(struct wdstat), BPRI_MED)) == NULL){
              		iocp->ioc_error = ENOSR;
              		goto iocnak;
      		}
		pstat = (struct wdstat *)bt->b_rptr;
		(void)wdfill_stat(&(wdp->mib),pstat);
		BCOPY(pstat,mp->b_cont->b_rptr,min(iocp->ioc_count,sizeof(struct wdstat)));
		freeb(bt);
		break;
	case NET_WDSTATUS:
		if ((bt = allocb(sizeof(struct wdstat), BPRI_MED)) == NULL){
              		iocp->ioc_error = ENOSR;
              		goto iocnak;
      		}
		pstat = (struct wdstat *)bt->b_rptr;
		(void)wdfill_stat(&(wdp->mib),pstat);
      		bt->b_wptr += sizeof(struct wdstat);
      		linkb(mp,bt);
      		iocp->ioc_count = sizeof(struct wdstat);
		break;
	case NET_GETBROAD:
		if (iocp->ioc_count != DL_MAC_ADDR_LEN) {
          		iocp->ioc_error = EINVAL;
          		goto iocnak;
       		}
		dp = mp->b_cont->b_rptr;
		bcopy( (caddr_t)wdbroadaddr,(caddr_t)dp,DL_MAC_ADDR_LEN);
		qreply(q,mp);
		break;
	case DLSADDR: {
		if (iocp->ioc_count != DL_MAC_ADDR_LEN) {
			iocp->ioc_error = EINVAL;
			goto iocnak;
		}
		wdset_eaddr(wdp,(DL_eaddr_t *)mp->b_cont->b_rptr);
		break;
	}
	case NET_INIT:
	case NET_UNINIT:
		break;
	case DLGMULT: {
		register struct wdmaddr *mcp;
		register int i;
		int found = 0;
		if (iocp->ioc_count <= 0) {
			iocp->ioc_rval = wdp->multicast_cnt;
		} else {
			dp = mp->b_cont->b_rptr;
			mcp = wdpp->wd_multiaddrs;
			for (i = 0;(i < wd_multisize) && (dp < mp->b_cont->b_wptr);i++,mcp++)
             			if (mcp->entry[0]) {
                 			BCOPY(mcp->entry,dp,DL_MAC_ADDR_LEN);
                 			dp += DL_MAC_ADDR_LEN;
                 			found++;
             			}
          		iocp->ioc_rval = found;
          		mp->b_cont->b_wptr = dp;
		}
		break;
	}
	default:
	iocnak:
		mp->b_datap->db_type = M_IOCNAK;
	}
}


void
wdbdspecclose(q)
queue_t *q;
{
	return;
}

int
wdxmit_packet(wdp,mb)
DL_bdconfig_t *wdp;
mblk_t *mb;
{
register unsigned int length; /* total length of packet */
unsigned char        *txbuf; /* ptr to transmission buffer area on 8003 */
register mblk_t      *mp;    /* ptr to M_DATA block containing the packet */
register int i;
short cmd_reg;
bdd_t *wdpp = (bdd_t *) wdp->bd_dependent1;


	wdp->flags |= TX_BUSY;
	cmd_reg = wdp->io_start + 0x10;
	txbuf = (unsigned char *) wdpp->wd_rambase;
	length = 0;
   	wdbcopy( mb->b_rptr, txbuf, (int)(mb->b_wptr - mb->b_rptr), DEST_ALIGN);
	length += (unsigned int)(mb->b_wptr - mb->b_rptr);

	mp = mb->b_cont;
   /*
    * load the rest of the packet onto the board by chaining through
    * the M_DATA blocks attached to the M_PROTO header. The list
    * of data messages ends when the pointer to the current message
    * block is NULL
    */

	do {
  		wdbcopy( mp->b_rptr, txbuf + length,
			(int)(mp->b_wptr - mp->b_rptr), DEST_ALIGN);
		length += (unsigned int)(mp->b_wptr - mp->b_rptr);
      		mp = mp->b_cont;
	} while (mp != NULL);

	if (length < WDMINSEND)
     		length = WDMINSEND;

	/* Packet is loaded now tell the 8390 to start	*/
	wdp->timer_val = 3;
	wdp->timer_id = timeout(wdwatchdog,(caddr_t)wdp,(long)(HZ));
   	i = inb(cmd_reg);
	outb( cmd_reg, i & PG_MSK);
	outb( cmd_reg + TPSR, 0);
	outb( cmd_reg + TBCR0, (unsigned char) length);
	outb( cmd_reg + TBCR1, (unsigned char)(length >> 8));
	i = inb(cmd_reg);
	outb( cmd_reg, i | TXP);
	wdp->mib.ifOutOctets += length;
	/* transmission started; report success	*/
	freemsg(mb);
	return(0);
}


/*
 * wdintr is the interrupt handler for the WD8003. This routine pro-
 * cesses all types of interrupts that can be generated by the 8390
 * LAN controller.
 */

void
wdintr(irq)
int irq;			/* interrupt level */
{
register unsigned char int_reg, ts_reg;
unsigned char orig;
mblk_t	*bp;
int call_wdsched = 0;	/* set if scheduler should be called */
register int inval;
register DL_bdconfig_t *wdp = wdconfig;
register int i;
unsigned char maxpkt;
caddr_t rcv_start, rcv_stop;	
short cmd_reg;
char	collisions;
bdd_t *wdpp;
int tally,err_count = 0;
int rval;

	for (i = wdboards; i; i--, wdp++) 
       		if (wdp->irq_level == irq || (wdp->irq_level == 2 && irq == 9))
	   		break;

   	if ( (i == 0) || (!(wdp->flags & BOARD_PRESENT)) ) {
       		cmn_err(CE_WARN, "wdintr: irq wrong: %x", irq);
       		return;
   	}
	wdpp = (bdd_t *)wdp->bd_dependent1;
	cmd_reg = wdp->io_start + 0x10;
   	maxpkt = (wdpp->wd_memsize >> 8) - 1;	/* last valid packet */
   	rcv_start = wdpp->wd_rambase + TX_BUF_LEN;/*skip past transmit buffer */
   	rcv_stop = wdpp->wd_rambase + wdpp->wd_memsize;	/* want end of memory */

   /* disable interrupts */
	outb(cmd_reg + IMR, NO_INT);

   /* make sure CR is at page 0 */
   	orig = inb(cmd_reg);
   	outb(cmd_reg, orig & PG_MSK);

	if ((int_reg = inb(cmd_reg + ISR)) == NO_INT) {
		/* Spurious interrupt */
      		return;
   	}

   	/* mask off bits that will be handled */
   	outb(cmd_reg + ISR, int_reg);

   	if (int_reg & PRX) {
      		inval = inb(cmd_reg);
      		outb(cmd_reg, (inval & PG_MSK) | PAGE_1);
      		while (wdpp->wd_nxtpkt != (unsigned char) inb(cmd_reg + CURR)) {

	 		rcv_buf_t *rp, *ram_rp;
	 		static rcv_buf_t rbuf;
	 		unsigned short length;
			if (wdifstats)
				wdifstats[wdp->bd_number].ifs_ipackets++;
	 		/* set up ptr to packet & update nxtpkt */
	 		ram_rp = (rcv_buf_t *)
	   		(wdpp->wd_rambase + (int) (wdpp->wd_nxtpkt << 8));
	 		wdbcopy((char *)ram_rp,(char *)&rbuf,sizeof(rcv_buf_t),
				SRC_ALIGN);
	 		rp = &rbuf;
	 		if ((wdpp->wd_nxtpkt = (unsigned char)(rp->nxtpg)) > 
								maxpkt) {
  				cmn_err(CE_CONT,"Bad nxtpkt value: nxtpkt=%x\n",
					wdpp->wd_nxtpkt);
	    			break;
	 		}

	 		/* get length of packet w/o CRC field */
	 		length = LLC_LENGTH(&rp->pkthdr);
	 		if (length > DL_MAX_PACKET) {
				/* DL_ETHER */
		 		length = rp->datalen - 4;
	 		} else {
				/* DL_CSMACD */
		/* rp->datalen can be wrong (hardware bug) -- use llc length */
		/* the llc length is 18 bytes shorter than datalen... */
				length += 14;
	 		}

	 	if ( ((int)length > DL_MAX_PACKET+LLC_EHDR_SIZE) || 
				((int)length < LLC_EHDR_SIZE) ) {

	     		/* garbage packet? - toss it */
	     		/* set CR to page 0 & set BNRY to new value */
	     		wdp->mib.ifInErrors++;     /* SNMP */
	     		inval = inb(cmd_reg);
	     		outb(cmd_reg, inval & PG_MSK);
	     		if ((int)(wdpp->wd_nxtpkt-1) < (TX_BUF_LEN>>8))
	       			outb(cmd_reg + BNRY, (wdpp->wd_memsize>>8)-1);
	     		else
	       			outb(cmd_reg + BNRY, wdpp->wd_nxtpkt-1);
	     		break;
	 	}

	 	/* get buffer to put packet in & move it there */
	 	if ((bp = allocb(length, BPRI_MED)) != NULL ||
	     		(bp = allocb(nextsize(length), BPRI_MED)) != NULL) {
	    		caddr_t dp, cp;
	    		unsigned cnt;

	    		bp->b_cont = NULL;
	    		/* dp -> data dest; ram_rp -> llc hdr */
	    		dp = (caddr_t) bp->b_wptr;
	    		cp = (caddr_t) &ram_rp->pkthdr;

	    		/* set new value for b_wptr */
	    		bp->b_wptr = bp->b_rptr + length;

	    /*
	     * See if there is a wraparound. If there
	     * is remove the packet from its start to
	     * rcv_stop, set cp to rcv_start and remove
	     * the rest of the packet. Otherwise, re-
	     * move the entire packet from the given
	     * location.
	     */

	    		if (cp + length >= rcv_stop) {
	       			/* process a wraparound */
	       			cnt = (int)rcv_stop - (int)cp;
	       			wdbcopy(cp, dp, cnt, SRC_ALIGN);
	       			length -= cnt;
	       			cp = rcv_start;
	       			dp += cnt;	/* have to move this, too */
	    		}
	    		wdbcopy(cp, dp, length, SRC_ALIGN);

	    		/* Call service routine */
	    		if (!(rval = DLrecv(bp,wdp->sap_ptr)))
				wdp->mib.ifInOctets += (int) (bp->b_wptr 
							- bp->b_rptr);
	 	} else {
			wdp->mib.ifInDiscards++;		/* SNMP */
			wdp->mib.ifSpecific.etherRcvResources++;/* SNMP */
	 	}	 /* end if */

	 	/* set CR to page 0 & set BNRY to new value */
	 	inval = inb(cmd_reg);
	 	outb(cmd_reg, inval & PG_MSK);
	 	if (((int)wdpp->wd_nxtpkt-1) < (TX_BUF_LEN>>8))
	   		outb(cmd_reg + BNRY, (wdpp->wd_memsize>>8)-1);
	 	else
		   	outb(cmd_reg + BNRY, wdpp->wd_nxtpkt-1);
	
	 	inval = inb(cmd_reg);
	 	outb(cmd_reg, (inval & PG_MSK) | PAGE_1);

      		} /* end while */

   	} /* end if PRX int */


   	/* restore CR to page 0 */
   	inval = inb(cmd_reg);
   	outb(cmd_reg, inval & PG_MSK);

   	if (int_reg & RXE) {
		tally = inb(cmd_reg + CNTR0);
		wdp->mib.ifSpecific.etherAlignErrors 	+= tally; /* SNMP */
		err_count = tally;
		tally = inb(cmd_reg + CNTR1);
		wdp->mib.ifSpecific.etherCRCerrors 	+= tally; /* SNMP */
		err_count += tally;
		tally = inb(cmd_reg + CNTR2);
		wdp->mib.ifSpecific.etherMissedPkts 	+= tally; /* SNMP */
		wdp->mib.ifInErrors += err_count;
		if (wdifstats)
			wdifstats[wdp->bd_number].ifs_ierrors++;
   	}

   	if (int_reg & (PTX|TXE)) {
		if(wdp->timer_id != -1) {
			(void) untimeout(wdp->timer_id);
			wdp->timer_id = -1;
			wdp->timer_val = -1;
		} 
		if (wdifstats)
			wdifstats[wdp->bd_number].ifs_opackets++;
      		/* free the transmit buffer */
      		wdp->flags &= ~TX_BUSY;
      		call_wdsched++;
      		ts_reg = inb(cmd_reg + TPSR);
		if (!(ts_reg & TSR_OK)) {
			if (wdifstats)
				wdifstats[wdp->bd_number].ifs_oerrors++;
      			if (ts_reg&TSR_COL) {
				collisions = inb(cmd_reg + TBCR0);
				wdp->mib.ifSpecific.etherCollisions +=
					collisions;	/* SNMP */
				if (wdifstats)
					wdifstats[wdp->bd_number].ifs_collisions++;
      			}
      			if (ts_reg&TSR_ABT)
				wdp->mib.ifSpecific.etherAbortErrors++;/*SNMP */
      			if (ts_reg&TSR_CRS)
				wdp->mib.ifSpecific.etherCarrierLost++;/*SNMP */
      			if (ts_reg&TSR_FU)
				wdp->mib.ifSpecific.etherUnderrunErrors++;/*SNMP */
		}
   	}

	
	/* reschedule blocked writers */
   	/* it should be safe to do this  here */
   	if (call_wdsched)
     		wdsched(wdp->sap_ptr);		

   	outb(cmd_reg + IMR, INIT_IMR);
   	/* outb(cmd_reg, orig);	*/ /* put things back the way they were found */
}

wdsched (fwd)
DL_sap_t *fwd;		
{
register DL_sap_t *wd;
register DL_bdconfig_t *fwdp;  
int x,next,maxsaps;
mblk_t *mp;


   fwdp = fwd->bd;
   if ( fwdp->flags & TX_QUEUED) {
	next = fwdp->tx_next;
	wd = fwd + next;
	maxsaps = fwdp->max_saps;
	for (x = maxsaps; x ; x-- ) {
		if (++next == maxsaps)
			next = 0;
		if ( (wd->state == DL_IDLE) && 
				 ( (mp = getq(wd->write_q)) != NULL)) {
			fwdp->tx_next = next;
			wdxmit_packet(fwdp,mp);
			fwdp->mib.ifOutQlen--;	/* SNMP */
			return;
		}
		if (next == 0)
			wd = fwd;
		else
			wd++;
	}
	fwdp->flags &= ~TX_QUEUED;
	fwdp->mib.ifOutQlen = 0; /* SNMP */
   }
}


unsigned char
wdhash(addr)
unsigned char addr[];
{
	register int i, j;
	union crc_reg crc;
	unsigned char fb, ch;

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
        return((unsigned char)(crc.value >> 26));
}

STATIC void
wdfill_stat(mib,wd)
DL_mib_t *mib;
struct wdstat *wd;
{
	wd->wds_nstats = 16;
	wd->wds_nobuffer = mib->ifSpecific.etherRcvResources;
	wd->wds_blocked = mib->ifSpecific.etherReadqFull;
	wd->wds_blocked2 = 0; 
	wd->wds_multicast = mib->ifOutNUcastPkts;
	wd->wds_xpkts = mib->ifOutUcastPkts + mib->ifOutNUcastPkts;
	wd->wds_xbytes = mib->ifOutOctets;
	wd->wds_excoll = 0; 
	wd->wds_coll = mib->ifSpecific.etherCollisions;
	wd->wds_fifounder = mib->ifSpecific.etherUnderrunErrors;
	wd->wds_carrier = mib->ifSpecific.etherCarrierLost;
	wd->wds_rpkts = mib->ifInUcastPkts + 
				mib->ifInNUcastPkts;
	wd->wds_rbytes = mib->ifInOctets;
	wd->wds_crc = mib->ifSpecific.etherCRCerrors;
	wd->wds_align = mib->ifSpecific.etherAlignErrors;
	wd->wds_fifoover = mib->ifSpecific.etherOverrunErrors;
	wd->wds_lost = mib->ifSpecific.etherMissedPkts;
}

static int
get_slot (id, pos_regs, bd)
unsigned id;	/* Adapter id */
unsigned char 	*pos_regs; /* Returned POS register values */
DL_bdconfig_t *bd;
{
     int 		i;
     unsigned char 	slot;
     unsigned char 	id_tmp;
     unsigned 		f_id;
     unsigned		pos_addr;

     slot = slot_count;	/* Start looking at slot 0 (1) */

     /* Turn on the Adapter setup bit */
     slot |= 0x8;

     for (; slot_count < 8 ;slot_count++) {	/* Check all 8 slots */
	  outb (ADAP_ENAB,slot);		/* Select adapter by slot */
	  f_id = (unsigned char) inb (POS_1);	/* MSB of Adapter ID in POS_1 */
	  f_id = f_id << 8;
	  id_tmp = inb (POS_0);			/* LSB of ADapter ID in POS_0 */
	  f_id |= id_tmp;

	  if (f_id == id) {
	      /* Now that we found the adapter, get the POS info */
	      pos_addr = POS_0;
	      for (i = 0;i < 8;i++) {
		  pos_regs[i] = inb(pos_addr);
		  pos_addr++;
	      }
		switch (bd->irq_level) {
			case 3: outb(POS_5, 0x04); break;
	    		case 4: outb(POS_5, 0x05); break;
			case 10: outb(POS_5, 0x06); break;
			case 15: outb(POS_5, 0x07); break;
			default:
				cmn_err(CE_WARN, "Unsupported interrupt level %d selected", bd->irq_level); return (-1);
		} /*switch*/
	      outb (ADAP_ENAB,0);	/* de_select adapter */
	      return (slot_count);
	  }
	  slot++;
     }
     outb (ADAP_ENAB,0);	/* de_select adapter */
     return -1;			/* No adapter found */
}

STATIC void
wduninit()
{
	DL_bdconfig_t *wdp = wdconfig;
	int i;

	if (wdinetstats)
		if (wdifstats)
			kmem_free(wdifstats, sizeof(struct ifstats) * wdboards);
	for (i = 0; i < wdboards; i++, wdp++) {
		if (wdp->bd_dependent1)
			kmem_free((int *)wdp->bd_dependent1, sizeof(bdd_t));
		untimeout(wdp->timer_id);
	}
}

STATIC void
wdwatchdog(pbd)
caddr_t pbd;
{
DL_sap_t *sap;
int x;
DL_bdconfig_t *wdp = (DL_bdconfig_t *)pbd;

	if (wdp->timer_val > 0)
		wdp->timer_val--;
	if (wdp->timer_val == 0) {
		wdp->timer_val = -1;
		wdp->timer_id = -1;
		for (sap = wdp->sap_ptr,x = 0;x < wdp->max_saps;x++,sap++) {
			if ((sap->write_q != NULL) && (sap->state == DL_IDLE))
				flushq(sap->write_q,FLUSHDATA);
		}
      		wdp->flags &= ~TX_BUSY;
		cmn_err(CE_CONT,"wd board:%d timed out--check your cable connection",wdp->bd_number);
		wdinit_board(wdp);
	} else
		wdp->timer_id= timeout(wdwatchdog,(caddr_t)wdp,(long)(HZ));
}
