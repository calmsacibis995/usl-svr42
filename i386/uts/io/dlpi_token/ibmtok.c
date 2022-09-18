/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/dlpi_token/ibmtok.c	1.12"
#ident	"$Header: $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Module:ibmtok 
 *
 *	Copyright (c) 1992 Dell Corporation
 *	All rights reserved.  Contains confidential information and
 *	trade secrets proprietary to Dell Corporation and USL
 */


#ifdef _KERNEL
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <io/dma.h>
#include <fs/file.h>
#include <mem/immu.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <proc/signal.h>
#include <io/conf.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <net/dlpi.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#ifndef M_INTR
#include <sys/inline.h>
#endif /*M_INTR*/

#include <net/transport/socket.h>
#include <net/tcpip/byteorder.h>
#include <net/tcpip/if.h>

#ifdef M_INTR
#define	intr_disable		splstr
#define	intr_restore		splx
#endif /* M_INTR */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#include <io/dlpi_token/ibmtokhw.h>
#include <io/dlpi_token/ibmtok.h>
#include <util/mod/moddefs.h>
#endif /*_KERNEL */

#define		DRVNAME		"tok - Loadable ibm 16/4 token ring driver"
int ibmtok_load(), ibmtok_unload();
MOD_DRV_WRAPPER(ibmtok, ibmtok_load, ibmtok_unload, NULL, DRVNAME);

#if defined(TOKDEBUG)
extern uint tok_debug;
#endif

/* Static variables. */

extern int               tok_totminors;   /* Total # of minor devices.      */
extern ushort		 tok_open_options;/* Used when opening the adapter. */
extern unsigned          tok_nbr_rcv_buffers; /*         as above           */
extern unsigned          tok_nbr_tx_buffers;  /*         as above           */
extern unsigned          tok_rcv_buff_size;   /*         as above           */
extern unsigned          tok_tx_buff_size;    /*         as above           */
extern unsigned          tok_max_nbr_of_SAPs; /*         as above           */
extern struct tokdev     tokdevs[];	  /* queue specific parameters.     */
extern struct tokstat    tokstats[];	  /* board statistics.              */
extern struct tokdevstat tokdevstats[];   /* SAP statistics.                */
extern addr_t physmap();

int ibmtokdevflag = 0;		/* V4.x style driver */
static unsigned int 	bus_type;
static pos_reg[8];
short tok_board_0_slot = -1;
int toktimer_id;

unsigned long tokdebug = 0;

/* STREAMS related definitions. */
static struct module_info minfo = {
	  0, "ibmtok", 0, MAXPKT, HIWAT, LOWAT,
};

int ibmtokopen(), ibmtokclose(), ibmtokwput(), ibmtokrsrv();

/* read queue initial values */
static struct qinit rinit = {
	NULL, ibmtokrsrv, ibmtokopen, ibmtokclose, NULL, &minfo, NULL,
};

/* write queue initial values */
static struct qinit winit = {
	ibmtokwput, NULL, NULL, NULL, NULL, &minfo, NULL,
};

struct streamtab ibmtokinfo = { &rinit, &winit, NULL, NULL };
struct streamtab tok1info = { &rinit, &winit, NULL, NULL };
/* End of streams definitions. */

extern struct ifstats *ifstats;
extern struct ifstats toks_ifstats[];
extern int tokboardfirst[];

/* 2 broadcast addresses for token ring */
unsigned char tokbaddr0[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char tokbaddr1[MAC_ADDR_LEN] = {0xC0, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

extern	struct OpenParam	tok_bopen[];


/* External declarations (most if not all are found in the space.c). */

extern tok_board_cnt;                       /* Nbr of sdevice entries.       */
extern struct tokparam  tokparams[];        /* Board specific parameters.    */

void tokwatch_dog(), tok_adapter_shutdown(),ibmtokuninit(); 
void tok_adapter_open(),tok_adapter_close(), tok_proc_adapter_error();
void ibmtokstart(), tokreset(), tok_proc_ssb(),tok_proc_close();
void tok_ok_ack(),tok_error_ack(),tok_bind_ack();
int ibmtokinit();

/* End of external declarations. */

#define nextsize(len)  ((len+64)*2)
/*
 * bcmp does a binary compare of string 1 with string 2 and
 * returns 0 for equal and 1 for false.  Only "n" bytes are compared.
 */

int
ibmtok_load(void)
{
	if (ibmtokinit() == -1) {
		cmn_err(CE_CONT,"IBM TOKEN RING ibmtokinit fails\n");
		return ENXIO;
	}
	mod_drvattach(&ibmtok_attach_info);
	(void)ibmtokstart();
	return (0);
}

int
ibmtok_unload(void)
{
struct tokparam *tokp = tokparams;
register int i;

	for (i = 0; i < tok_board_cnt; i++,tokp++) {
		if (tokp->tok_noboard == TRUE)
			continue;
		if (!((tokp->tok_adapter_state == ADAPTER_CLOSED)
			|| (tokp->tok_adapter_state == ADAPTER_DISABLED)))
			return EBUSY;
	}
	mod_drvdetach(&ibmtok_attach_info);
	ibmtokuninit();
	untimeout(toktimer_id);
	return (0);
}

int
ibmtokinit()
{
unsigned long cur_brd_nbr;  		/* Nbr of boards from space.c 	*/
unsigned i, j, l;         		/* Simple counters.		*/
struct tokparam *brd_ptr = tokparams;   /* Sdevice parameters from space.c +  */
struct tokdev *tok;
register struct ifstats *statp;

	printf ("Copyright (c) 1992 USL & DELL's UNIX IBM Token-Ring Driver\n");
	/* Initialize device information to all 0's */

	bzero((char*)tokdevs, sizeof(struct tokdev) * tok_totminors);
	bzero((char *)tokdevstats,sizeof(struct tokdevstat)*tok_totminors);
	bzero((char *)tokstats,sizeof(struct tokstat)* tok_board_cnt);

	/* Init minor devices structures */

	for (i=0;i<tok_totminors;i++) {
		tokdevs[i].tok_sap = -1;
		tokdevs[i].tok_dstats = &(tokdevstats[i]);
	}

  	for (cur_brd_nbr = 0; cur_brd_nbr < tok_board_cnt; cur_brd_nbr++,
								 brd_ptr++) {
		brd_ptr->tok_noboard = TRUE;
		brd_ptr->ram_area = NULL; brd_ptr->aca_area = NULL;
		brd_ptr->aip_area = NULL; brd_ptr->SRB_area = NULL;
		brd_ptr->ARB_area = NULL; brd_ptr->SSB_area = NULL;
		brd_ptr->ASB_area = NULL;
		if (find_board(brd_ptr) == -1)
			return -1;
		if (brd_ptr->tok_noboard == TRUE) {
			cmn_err(CE_CONT,"IBM TOKEN RING: Unable to find board:%d\n",cur_brd_nbr);
			/* There is no point in loading the driver if the
			   only board available cannot be validated.
			*/
			if (tok_board_cnt == 1)
				return -1;
			else
				continue;
		}
       		printf("tok%d: addr 0x%x irq %d bios 0x%x ram 0x%x\n", 
			cur_brd_nbr, brd_ptr->tok_ioaddr, brd_ptr->tok_int, 
			brd_ptr->bios_addr, brd_ptr->ram_addr);
     		/* Setup the rest of the board structure. */

		brd_ptr->tok_index    = cur_brd_nbr; /* Board index.     */
		brd_ptr->tok_init     = 0;	/* board not initialized yet. */
		brd_ptr->tok_str_open = 0;	/* number of streams open.    */
		brd_ptr->tok_nextq    = 0;	/* next queue to process.     */
		brd_ptr->tok_proms    = 0;	/* number of promiscuous strs */
		brd_ptr->tok_maxpkt   = MAXDPKT;/* maximum size of xmit pkt.  */
		brd_ptr->cur_msg        = NULL;
		brd_ptr->snap_bound	= 0;
		brd_ptr->tok_multicnt = 0;
		brd_ptr->sched_flags = 0;
		brd_ptr->tok_adapter_state = ADAPTER_DISABLED;
  		brd_ptr->tok_open_flag = 0;
  		brd_ptr->tok_close_flag = 0;
		brd_ptr->tok_timer_val = -1;
		brd_ptr->tok_reset_retries = 0;
		for (i=0;i<MAC_ADDR_LEN;i++)
			brd_ptr->tok_multiaddr[i] = 0;
		for (i=0;i<MAC_ADDR_LEN;i++)
			brd_ptr->tok_groupaddr[i] = 0;

		/* first minor device */
		brd_ptr->tok_firstd   = cur_brd_nbr * brd_ptr->tok_minors;	
		tok = &tokdevs[brd_ptr->tok_firstd];
		for (j = 0; j < brd_ptr->tok_minors; j++,tok++) {
			tok->tok_macpar = brd_ptr;
			tok->tok_stats = &tokstats[brd_ptr->tok_index];
		}
		statp = &toks_ifstats[brd_ptr->tok_index];
		bzero((char *) statp, sizeof(struct ifstats));
		statp->ifs_name = "ibmtok";
		statp->ifs_unit = brd_ptr->tok_index;
		statp->ifs_next = ifstats;
		ifstats         = statp;
		statp->ifs_mtu  = MAXDPKT;
		statp->ifs_active = 1;	/* flag as up */
		
  	} /* End of loop for every board. */
	return (0);
}

void
ibmtokuninit()
{
struct tokparam *tokp = tokparams;
register int i;

	for (i = 0; i < tok_board_cnt; i++,tokp++) {
		if (tokp->tok_noboard == TRUE)
			continue;
		ifstats = toks_ifstats[tokp->tok_index].ifs_next;
		toks_ifstats[tokp->tok_index].ifs_next = NULL;
		if (tokp->aca_area != NULL)
			physmap_free((addr_t) (tokp->aca_area),ACA_AREA_SIZE,0);
		if (tokp->ram_area != NULL)
			physmap_free((addr_t) (tokp->ram_area),tokp->tok_ram_size,0);
	}
}

void 
ibmtokstart()
{
struct tokparam *tokp = tokparams;
register int i;

	for (i = 0; i < tok_board_cnt; i++,tokp++) {
		if (tokp->tok_noboard == TRUE)
			continue;
		tokp->tok_timer_val = 15;
		(void)tokreset(tokp);
	}
	toktimer_id = timeout(tokwatch_dog,0,drv_usectohz(TOK_TIMEOUT));
}

/*
   Extract_aip_info :  Called from find_board.

   Collect the static adapter info from MMIO.  First check the adapter checksum.
   Extract :
            1. Encoded node address.
            2. Adapter type.
            3. Date rate.
            4. Can do Early Token Release ?
            5. RAM available.
            6. RAM paging info.
            7. DHB size available.
            8. Unmap the AIP area when done.
                         
*/
int 
extract_aip_info(tokp)
struct tokparam *tokp;
{
register i;
unsigned checksum;


  	/* Check that the BIOS is not screwed up (part 1). */
  	for (i = 0, checksum = 0; i <= AIP_CHECKSUM1_RANGE; i += 2)
		checksum += (*(tokp->aip_area + i)) & 0xf;

  	if (checksum & 0xf) {
		cmn_err(CE_WARN,"IBM TOKEN RING Adapter checksum_1 invalid\n");
    		return(AIP_BAD_CHECKSUM);
	}

  	/* Get the token ring node address. */
  	for (i = 0; i < ENCODED_ADDR_LENGTH; i++)
     		tokp->aip.encoded_addr[i] = (*(tokp->aip_area + 2 * i) & 0xf);

  	/* Get adapter type F = 1st., E = 2nd., .. 0 = 16th. */
  	tokp->aip.adapter_type = (*(tokp->aip_area + ADAPTER_TYPE_OFFSET))& 0xf;

  	/* Get the date rate (4 or 16 MB/s), F = 4 MB, E = 16 MB, D = both. */
  	tokp->aip.data_rate = (*(tokp->aip_area + DATA_RATE_OFFSET)) & 0xf;

  	/* Can do "Early Token Relase" ? */
  	tokp->aip.early_token_release = 
		(*(tokp->aip_area + EARLY_TOKEN_OFFSET)) & 0xf;

  	/* Available shared RAM. */
  	tokp->aip.total_RAM = (*(tokp->aip_area + RAM_SIZE_OFFSET)) & 0xf;
  	if (tokp->aip.total_RAM <= RAM_AVAILABLE_IS_64) {
    		/* RAM size is 63.5 or 64k bytes. */
    		if (tokp->aip.total_RAM & 0x01)
			/* IBM weirdness, we must write 0's */
      			tokp->aip.initialize_RAM_area = TRUE; 
			/* to the upper 512 bytes of RAM. */
    		tokp->aip.total_RAM = 64;
  	}
  	else
    		if (tokp->aip.total_RAM < GET_RAM_SIZE_RRR) 
      		/* RAM size is 8, 16 or 32k bytes. */
      			tokp->aip.total_RAM = 
				1 << ((~(tokp->aip.total_RAM & 0x0f)) + 2);
    	        else 
      			/* We must get the RAM size from the RRR-odd register.*/
      			tokp->aip.total_RAM = 
				1 << (((*tokp->aca_area + RRR_ODD_OFFSET) & 0x0C) + 3);
  	/* NEW, just set the total amount of RAM as indicated in RRR-ODD. */
  	tokp->aip.total_RAM = 1 << ((((*(tokp->aca_area + RRR_ODD_OFFSET))
                               & 0x0C) >> 2) + 3);

  	/* RAM paging supported and if so what page size. */
  	tokp->aip.RAM_paging = (*(tokp->aip_area + RAM_PAGING_OFFSET)) & 0xf;
  	tokp->aip.RAM_paging = (~(tokp->aip.RAM_paging & 0x01)) & 0x01;

  	/* DHB size available at 4 MB/s , F = 2048, E = 4096, D = 4464. */
  	tokp->aip.DHB_size_4_mbps = (*(tokp->aip_area + DHB_4MB_SIZE_OFFSET)) & 0xf;

  	/* DHB size available at 16 MB/s , F = 2048, E = 4096, .. , B = 17960. */
  	tokp->aip.DHB_size_16_mbps = (*(tokp->aip_area + DHB_16MB_SIZE_OFFSET)) & 0xf;


  	/* Check that the BIOS is not screwed up (part 2). */
  	for (i = 0, checksum = 0; i <= AIP_CHECKSUM2_RANGE; i += 2)
     		checksum += (*(tokp->aip_area + i)) & 0xf;

  	if (checksum & 0xf) {
		cmn_err(CE_WARN,"IBM TOKEN RING Adapter checksum_2 invalid\n");

    		return(AIP_BAD_CHECKSUM);
	}

  	/* Now we are done with the adapter aip area. Release it. */
  	physmap_free((addr_t) tokp->aip_area, AIP_AREA_SIZE, 0);
  	tokp->aip_area = (addr_t) NULL;

 	return(0);
}

/*
  Tokreset :       Called from the first open or from certain ioctl's.
                   1. Check if we have found a board.
                   2. Set "reset latch".
                   3. Delay 50 ms.
                   4. Set "release latch".
                   5. Setup the RAM area (RRR-register).
                   7. Interrupt adapter, i.e. set interrupt bit ISRP bit 6
		      and the IRQ control bit.
*/

void
tokreset(tokp)
struct tokparam *tokp;
{
  	tokp->tok_reset_flag = 0;
	tokp->tok_adapter_state = ADAPTER_RESETTING;
  	/* Reset the adapter (latch). */
  	outb(tokp->tok_ioaddr + RESET_LATCH_OFFSET, RESET_VALUE);

  	/* Delay at least 50 ms. */
     	tokwaitloop(5000);

  	/* Release the latch. */
  	outb(tokp->tok_ioaddr + RESET_RELEASE_OFFSET, RELEASE_VALUE);

  	/* Setup the shared RAM location (RRR-register). */

	if (bus_type == BUS_ISA)
 		*(tokp->aca_area + RRR_EVEN_OFFSET) = (tokp->ram_addr >> 12); 

  	/* Enable software interrupts and set the error control bit.
	   This should be done for both AT and MCA. This important for the AT 
	   because we should not be dealing with NMIs (CHCK). Furthermore the 
    	   the ref manual indicates that this bit should always be turned on 
	   for MCA.
	*/
  	*(tokp->aca_area + ISRP_EVEN_SET) = ENABLE_SW_INTR | ENABLE_IRQ_CONTROL;
}

/* Swab swaps byte in a short */
ushort 
tokswab(w_to_swab)
ushort	w_to_swab;
{
  ushort temp;
  unchar *ctmp;
	
  ctmp = (unchar *) &w_to_swab;
	
  temp = 0;
  temp = (ushort) ctmp[0];
  temp <<= 8;
  temp |= (ushort) ctmp[1];
  return (temp);
}

void
tok_adapter_open(tokp)
struct tokparam *tokp;
{
struct OpenParam *p_ptr;
register         i;
unchar	   *b_ptr;
unsigned         offset;
unsigned 	init_offset;


	tokp->tok_adapter_state = ADAPTER_OPENING;
	tokp->tok_open_flag = 0;
  	init_offset = ((*(tokp->aca_area + WRBR_EVEN_OFFSET)) & 0xff) << 8;
  	init_offset += ((*(tokp->aca_area + WRBR_ODD_OFFSET)) & 0xff);
  	tokp->SRB_area = physmap((paddr_t) (tokp->ram_addr + init_offset), 
                     INITIAL_SRB_SIZE, KM_NOSLEEP);

	p_ptr = &(tok_bopen[tokp->tok_index]);
        p_ptr->command = SRB_OPEN;
	p_ptr->options = htons(tok_open_options);
	/* Node address */
	b_ptr = (unchar *) &(p_ptr->naddr_hi);
	for (i=0;i<MAC_ADDR_LEN;i++,b_ptr++)
		*b_ptr = tokp->tok_macaddr[i];
        /* The initial group and functional addresses are 0
           they can be changed with proper ioctl's.         */
	p_ptr->gaddr_hi       = 0;	  	/* Group address High byte.   */
	p_ptr->gaddr_lo       = 0;        	/* Group address Low byte.    */
	p_ptr->faddr_hi       = 0;        	/* Functional addr High byte. */
	p_ptr->faddr_lo       = 0;        	/* Functional addr Low byte.  */
	p_ptr->r_buff_size    = tokswab(tok_rcv_buff_size); 
	p_ptr->xmit_buff_size = tokswab(tok_tx_buff_size);
	p_ptr->nbr_r_buf      = tokswab(tok_nbr_rcv_buffers);
	p_ptr->nbr_xmit_buf   = tokswab(tok_nbr_tx_buffers);
	p_ptr->max_sap        = tok_max_nbr_of_SAPs;  /* Max. number of SAP's.*/
	p_ptr->max_station    = 0;   /* Max. number of links.   */

        /* Setup the SRB with the open parameters. */
        bcopy(p_ptr, tokp->SRB_area, sizeof(struct OpenParam));

        /* Send off the "Open Adapter" SRB. */
        *(tokp->SRB_area+2) = 0xfe; 
        *(tokp->aca_area + ISRA_ODD_SET) = 0x28;

	tokp->tok_timer_val = 12;
}

/*
 * ibmtokopen is called by the stream head to open a minor device.
 * CLONEOPEN is supported and opens the lowest numbered minor device
 * which is not currently opened.  Specific device open is allowed to
 * open an already opened stream.
 */

int 
ibmtokopen(q, dev, flag, sflag,credp)
queue_t	*q;
dev_t	*dev;
int 	flag;
int 	sflag;
struct cred	*credp;
{
struct tokdev     *tok;
struct tokparam   *tokp = tokparams;
mblk_t            *mp;
struct stroptions *opts;
register int      i;
int old;
major_t	devmajor = getmajor(*dev);
minor_t devminor = getminor(*dev);

	/* find the appropriate board by its major device number */

	for (i = tok_board_cnt; i; tokp++, i--) {
    		if (tokp->tok_major == devmajor)
       			break;
	}

    	/* Fail the open if wrong major number or if board not present
	   or if board is not reset*/

	if (i == 0 || tokp->tok_noboard)
    		return ENXIO;
	old = splhi();
	if (!tokp->tok_reset_flag) {
		if (tokp->tok_adapter_state == ADAPTER_RESETTING) {
		        if(sleep((caddr_t)&tokp->tok_reset_flag,STIPRI|PCATCH)){
				splx(old);
				return EINTR;
			}
		}
	}
	if ( (tokp->tok_reset_flag == ADAPTER_RESET_TIMEOUT) || 
		(tokp->tok_reset_flag == ADAPTER_RESET_FAILED) ) {
		cmn_err(CE_WARN,"IBM TOKEN RING: Adapter cannot be initialised\n");
		splx(old);
		return ENXIO;
	}
	if (tokp->tok_init == FALSE) {
		if (tokp->tok_adapter_state == ADAPTER_CLOSED)
			(void)tok_adapter_open(tokp);
		if (sleep((caddr_t)&tokp->tok_open_flag,STIPRI|PCATCH)) {
			splx(old);
			return EINTR;
		}
	}
	if (tokp->tok_open_flag & (ADAPTER_OPEN_TIMEOUT|ADAPTER_OPEN_FAILED)) {
		splx(old);
		return ENXIO;
	}
	splx(old);
  	tokp->tok_str_open++;

  	/* determine type of open, CLONE or otherwise */

  	if (sflag == CLONEOPEN) {
    		for (i=0; i < tokp->tok_minors; i++)
       			if (tokdevs[tokp->tok_firstd+i].tok_qptr == NULL)
  	 			break;
		if (i == tokp->tok_minors)
			return ECHRNG;
		else
			devminor = (minor_t)i;
  	} else if (devminor > tokp->tok_minors)
		return ECHRNG;

  	if (!q->q_ptr) {
  		tok = &tokdevs[tokp->tok_firstd + devminor];
  		WR(q)->q_ptr = (caddr_t) tok;
  		q->q_ptr = (caddr_t)tok;
  		tok->tok_qptr = WR(q);
  	}

#ifndef LAI_TCP
  	/* update stream head for proper flow control */

  	if ((mp = allocb(sizeof(struct stroptions), BPRI_MED)) == NULL)
    		return ENOSR;

	 mp->b_datap->db_type = M_SETOPTS;
	 opts = (struct stroptions *)mp->b_rptr;
	 opts->so_flags = SO_HIWAT | SO_LOWAT | SO_MAXPSZ;
	 opts->so_readopt = 0;
	 opts->so_wroff   = 0;
	 opts->so_minpsz  = MINSEND;
	 opts->so_maxpsz  = MAXDPKT;
	 opts->so_hiwat   = HIWAT;
	 opts->so_lowat   = LOWAT;
	 mp->b_wptr = mp->b_rptr + sizeof(struct stroptions);
	 putnext(q, mp);

#endif /* not LAI_TCP */

  	/* initialize the state information needed to support DL interface */

  	tok->tok_state  = DL_UNBOUND;	/* it starts unbound */
  	tok->tok_flags  = S_OPEN;	/* and open */
  	tok->tok_type   = DL_802;	/* Assume 802.2 LLC type packets */
  	tok->tok_no     = tokp->tok_firstd + devminor;
  	tok->tok_flags |= (drv_priv(credp) == 0) ? S_SU : 0; 


	*dev = makedevice(devmajor,devminor);
  	return 0;
}

/*
 * ibmtokclose is called on last close to a stream.
 * it flushes any pending data (assumes higher level protocols handle
 * this properly) and resets state of the minor device to unbound.
 * The last close to the last open tok stream will result in the board
 * being shutdown.
 */

/* To do: ibmtokclose should perform a close_sap and probably sleep 
   till the tok_proc_close_sap confirms the close. Hence the tokclose
   will be a two step process: a)a possible close_sap b) a possible
   close adapter if it is the last close.
*/

ibmtokclose(q)
queue_t *q;
{
register struct tokdev *tok;
register struct tokparam *tokp;
short cmd_reg;
int inval;
int old;

	old = splstr();

	/* flush all queues */
	flushq(q, FLUSHALL);
	flushq(OTHERQ(q), FLUSHALL);

	/* mark as closed */
	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;
	tok->tok_qptr = NULL;
	tok->tok_state = DL_UNBOUND;	/* just in case */
	tok->tok_flags = 0;		/* nothing pending */
  	if (--tokp->tok_str_open == 0) {
		while(tokp->tok_init == TRUE) {
			tokp->tok_adapter_state = ADAPTER_CLOSING;
			(void)tok_adapter_close(tokp);
			if(sleep((caddr_t)&tokp->tok_close_flag,STIPRI|PCATCH)){
				splx(old);
				return EINTR;
			}
			toks_ifstats[tokp->tok_index].ifs_active = 0;
				/* Flag the interface as down */
		}
	}
	splx(old);
}

/*
 * toksched
 *	this function is called when the interrupt handler
 *	determines that the board is now capable of handling
 *	a transmit buffer.  it scans all queues for a specific board 
 * 	to determine the order in which they should be rescheduled, if at all.
 *	round-robin scheduling is done by default with modifications
 *	to determine which queue was really the first one to attempt
 *	to transmit.  Priority is not considered at present.
 */

toksched(ftok)
struct tokdev *ftok;		/* first device for this board */
{
register int i, x;
static int rrval = 0;	/* rrval is used to do pure round-robin */
register struct tokdev *tok;
register struct tokparam *ftokp;  /* board specific parameters */
int next,maxsaps;
mblk_t *mp;

	ftokp = ftok->tok_macpar;
	if (ftokp->sched_flags & TX_QUEUED) {
		next = ftokp->tok_nextq;
		tok = ftok + next;
		maxsaps = ftokp->tok_minors;
		for (x = maxsaps; x; x-- ) {
			if (++next == maxsaps)
				next = 0;
			if ( (tok->tok_state != DL_UNBOUND) &&(mp=getq(tok->tok_qptr)) ){
				ftokp->tok_nextq = next;
				tok_exec_cmd(tok,mp);
				return;
			}
			if (next == 0)
				tok = ftok;
			else
				tok++;
		}
	}
	ftokp->sched_flags &= ~TX_QUEUED;
}

/*
 * ibmtokwput is the "write put procedure"
 * It's purpose is to filter messages to ensure that only
 * M_PROTO/M_PCPROTO and M_IOCTL messages come down from above.
 * Others are discarded while the selected ones are queued to the
 * "write service procedure".
 */

ibmtokwput(q, mp)
queue_t *q;
mblk_t  *mp;
{
register struct tokdev *tok;
register struct tokparam *tokp;
int err;
union DL_primitives *dlp;

	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;
	switch (mp->b_datap->db_type) {
	case M_IOCTL:
	 	tokioctl(q, mp);
		break;
	case M_PROTO:
	case M_PCPROTO:
		if  ( (err = ((tokp->tok_init == FALSE) ? DL_NOTINIT:0)) ||
			(err = llccmds(q,mp)) ) {
				dlp = (union DL_primitives *)mp->b_rptr;
				(void)tok_error_ack(q,dlp->dl_primitive,err);
				freemsg(mp);
		}
		break;
	case M_FLUSH:
		/* Standard M_FLUSH code */
		if (*mp->b_rptr & FLUSHW) {
			flushq(q,FLUSHDATA);
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply (q,mp);
		} else 
			freemsg (mp);
		break;
	case M_DATA:
	default:
		freemsg(mp);		/* discard unknown messages */
	}
}

/*
 * ibmtokrsrv - "read queue service" routine
 * called when data is put into the service queue
 * determines additional processing that might need to be done
 */

ibmtokrsrv(q)
queue_t *q;
{
	mblk_t *mp;
	register struct tokdev *tok;


	tok = (struct tokdev *)q->q_ptr;	/* get minor device info */
	while ((mp = getq(q)) != NULL) {
		if (recv_llc(tok, q, mp) != E_OK){
			freemsg(mp);
	 	}
	}
}

/*
 *	p_llc_hdr() parses the llc header and returns information about the
 *	received packet. It looks for routing information and gets the SAP
 *	out of the header. Returns pointers to the MAC header and the LLC 
 *	portion of the header.
 */

ushort 
p_llc_hdr(p_ptr,pkt_info)
unchar	*p_ptr;			/* Pointer to packet */
struct llc_info	*pkt_info;	/* Pointer to LLC info structure */
{
	ushort	pkt_type = 0;

	pkt_info->mac_ptr = (struct tok_machdr *) p_ptr;
	if (pkt_info->mac_ptr->mac_src[0] & 0x80) {
		/* We have routing information get the size */
		pkt_type |= DL_ROUTE;
		pkt_info->rsize = pkt_info->mac_ptr->ibm_route[0] & 0x1f;
		pkt_info->b_cast = ((unsigned) (pkt_info->mac_ptr->ibm_route[0] & 0xE0)) >> 5;
		pkt_info->direction = ((unsigned) (pkt_info->mac_ptr->ibm_route[1] & 0x80)) >> 7;
		pkt_info->lf = ((unsigned) (pkt_info->mac_ptr->ibm_route[1] & 0xE8)) >> 3;

	} else {
		pkt_info->rsize = 0;
	}
	pkt_info->llc_ptr = (union llc_header *) (p_ptr+(2*MAC_ADDR_LEN)+pkt_info->rsize);
	
	pkt_info->ssap = pkt_info->llc_ptr->llc_sap.llc_ssap;
	pkt_info->dsap = pkt_info->llc_ptr->llc_sap.llc_dsap;
	pkt_info->control = pkt_info->llc_ptr->llc_sap.llc_control;
	pkt_info->snap = pkt_info->llc_ptr->llc_snap.ether_type;

	if ((pkt_info->ssap == LLC_SNAP_SAP)||(pkt_info->ssap == (LLC_SNAP_SAP|LLC_RESPONSE)) ||(pkt_info->dsap == LLC_SNAP_SAP)) {
		pkt_type |= DL_SNAP;
		if (pkt_info->ssap  & LLC_RESPONSE)
			pkt_type |= DL_RESPONSE;
		pkt_info->data_ptr = (unchar *)pkt_info->llc_ptr+LLC_SNAP_H_LEN+LLC_SAP_H_LEN;
		pkt_info->ssap = pkt_info->dsap = ntohs(pkt_info->snap);
	} else {
		pkt_type |= DL_802;
		if (pkt_info->ssap  & LLC_RESPONSE)
			pkt_type |= DL_RESPONSE;
		pkt_info->data_ptr = (unchar *)pkt_info->llc_ptr+LLC_SAP_H_LEN;
	}

	return (pkt_type);
	
}

/*
  *	w_snap_r_dlhdr():
 *	Write a DL_UNITDATA header with SNAP addressing and source routing
 */

static mblk_t * 
w_snap_r_dlhdr(info_ptr)
struct	llc_info *info_ptr;
{
struct llca	*llcap;
union DL_primitives *dlp;
char		*r_ptr;
mblk_t		*nmp;
register unsigned i;

	/* get a message block for new header information */
	if ((nmp = allocb(DL_UNITDATA_IND_SIZE+(2*LLC_ENADDR_LEN)+MAX_ROUTE_FLD,
			  BPRI_MED)) == NULL){
		return NULL;
	}
	dlp = (union DL_primitives *)nmp->b_rptr;

	/* insert real data for unitdata_ind message */
	/* destination address */
	llcap = (struct llca *)((caddr_t)dlp + DL_UNITDATA_IND_SIZE);
	bcopy(info_ptr->mac_ptr->mac_dst, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->dsap;
	/* Pointers in unitdata command block */
	dlp->unitdata_ind.dl_dest_addr_length = LLC_ENADDR_LEN;
	dlp->unitdata_ind.dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;

	llcap++;		
	/* get next position */
	/* source address */
	bcopy(info_ptr->mac_ptr->mac_src, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->ssap;

	/* Point to the routing field */
	r_ptr = (char *)++llcap;

	for(i = 0;i < info_ptr->rsize;i++)
		r_ptr[i] = info_ptr->mac_ptr->ibm_route[i]; 

	dlp->unitdata_ind.dl_src_addr_length = LLC_ENADDR_LEN + info_ptr->rsize;
	dlp->unitdata_ind.dl_src_addr_offset =DL_UNITDATA_IND_SIZE+ LLC_ENADDR_LEN;
	nmp->b_wptr = nmp->b_rptr + (DL_UNITDATA_IND_SIZE + 2*LLC_ENADDR_LEN+info_ptr->rsize);
	return (nmp);
}

/*
 *	w_snap_dlhdr():
 *	Write a DL_UNITDATA header with SNAP addressing
 */

static mblk_t *
w_snap_dlhdr(info_ptr)
struct	llc_info *info_ptr;
{
	struct llca	*llcap;
	mblk_t		*nmp;
	union DL_primitives *dlp;

	/* get a message block for new header information */
	if ((nmp = allocb(DL_UNITDATA_IND_SIZE + 2*LLC_ENADDR_LEN,
			  BPRI_MED)) == NULL){
		return NULL;
	}
	dlp = (union DL_primitives *)nmp->b_rptr;

	/* insert real data for unitdata_ind message */
	llcap = (struct llca *)((caddr_t)dlp + DL_UNITDATA_IND_SIZE);

	bcopy(info_ptr->mac_ptr->mac_dst, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->dsap;
	/* Write pointers in unitdata command block */
	dlp->unitdata_ind.dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
	dlp->unitdata_ind.dl_dest_addr_length = LLC_ENADDR_LEN;

	llcap++;			/* get next position */
	bcopy(info_ptr->mac_ptr->mac_src, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->ssap;
	dlp->unitdata_ind.dl_src_addr_length = LLC_ENADDR_LEN;
	dlp->unitdata_ind.dl_src_addr_offset=DL_UNITDATA_IND_SIZE + LLC_ENADDR_LEN;
	nmp->b_wptr = nmp->b_rptr + (DL_UNITDATA_IND_SIZE + 2*LLC_ENADDR_LEN);
	return (nmp);
}

/*
 *	w_sap_r_dlhdr():
 *	Write DL_UNITDATA header with SAP addressing and source routing
 */

static mblk_t *
w_sap_r_dlhdr(info_ptr)
struct	llc_info *info_ptr;
{
struct	llcc	*llccp;
union DL_primitives *dlp;
char		*r_ptr;
mblk_t		*nmp;
register unsigned i;

	/* get a message block for new header information */
	if ((nmp = allocb(DL_UNITDATA_IND_SIZE+(2*LLC_LIADDR_LEN)+MAX_ROUTE_FLD,
			  BPRI_MED)) == NULL) {
		return NULL;
	}

	dlp = (union DL_primitives *)nmp->b_rptr;

	/* insert real data for unitdata_ind message */
	/* Point to just past unidata_ind block */
	llccp = (struct llcc *)((caddr_t)dlp + DL_UNITDATA_IND_SIZE);
	/* Copy destination address and SAP first */
	bcopy(info_ptr->mac_ptr->mac_dst, llccp->lbf_addr, MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->dsap;
	/* Set pointers in unidata commadn block */
	dlp->unitdata_ind.dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
	dlp->unitdata_ind.dl_dest_addr_length = LLC_LIADDR_LEN;

	llccp++;			/* get next position */
	bcopy(info_ptr->mac_ptr->mac_src, llccp->lbf_addr, MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->ssap;

	/* Point to the routing field */
	llccp++;			/* get next position */
	r_ptr = (char *)llccp;

	for(i = 0;i < info_ptr->rsize;i++)
		r_ptr[i] = info_ptr->mac_ptr->ibm_route[i]; 

	dlp->unitdata_ind.dl_src_addr_length = LLC_LIADDR_LEN + info_ptr->rsize;
	dlp->unitdata_ind.dl_src_addr_offset = DL_UNITDATA_IND_SIZE+ LLC_LIADDR_LEN;
	nmp->b_wptr = nmp->b_rptr + (DL_UNITDATA_IND_SIZE + 2*LLC_LIADDR_LEN + info_ptr->rsize);
	return (nmp);
}

/*
 *	w_sap_dlhdr():
 *	Write DL_UNITDATA header with SAP addressing
 */

static mblk_t *
w_sap_dlhdr(info_ptr)
struct	llc_info *info_ptr;
{

	struct llcc	*llccp;
	mblk_t		*nmp;
	union DL_primitives *dlp;

	/* get a message block for new header information */
	if ((nmp = allocb(DL_UNITDATA_IND_SIZE + 2*LLC_LIADDR_LEN,
			  BPRI_MED)) == NULL){
		return NULL;
	}

	dlp = (union DL_primitives *)nmp->b_rptr;

	/* insert real data for unitdata_ind message */
	llccp = (struct llcc *)((caddr_t)dlp + DL_UNITDATA_IND_SIZE);
	bcopy(info_ptr->mac_ptr->mac_dst, llccp->lbf_addr, MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->dsap;
	/* unitdata commnad block pointers */
	dlp->unitdata_ind.dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
	dlp->unitdata_ind.dl_dest_addr_length = LLC_LIADDR_LEN;

	llccp++;			/* get next position */
	bcopy(info_ptr->mac_ptr->mac_src, llccp->lbf_addr, MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->ssap;

	dlp->unitdata_ind.dl_src_addr_length = LLC_LIADDR_LEN;
	dlp->unitdata_ind.dl_src_addr_offset = DL_UNITDATA_IND_SIZE+LLC_LIADDR_LEN;
	nmp->b_wptr = nmp->b_rptr + (DL_UNITDATA_IND_SIZE + 2*LLC_LIADDR_LEN);

	return (nmp);
}

/*
 *	pnext_msg():
 *	Constructs a UNITDATA control message and links data and sends
 *	the message up stream. Handles all type of llc headers;
 *	(i.e. SNAP,SAP + routing,non-routing)
 */

int 
pnext_msg(pkt_type,info_ptr,tok,q,mp,prim_type)
ushort	pkt_type;	/* Type of packet DL_802, DL_SNAP, DL_ROUTE */
struct	llc_info *info_ptr;
struct	tokdev *tok;
queue_t *q;
mblk_t	*mp;
long	prim_type;
{
	register mblk_t *nmp;
	union DL_primitives *dlp;

	if (!canput(q->q_next)){
		/* avoid next queue having small hiwater mark */
		/* really shouldn't flow control upward */
	 	freemsg(mp); 
	 	return E_OK;
	}

	/* Construct the DL header based on pkt_type */

	if (pkt_type & DL_SNAP) {
		if ((pkt_type & DL_ROUTE) && (info_ptr->rsize > 2)) 
			nmp = w_snap_r_dlhdr(info_ptr);
		else
			nmp = w_snap_dlhdr(info_ptr);
	} else {
		if ((pkt_type & DL_ROUTE) && (info_ptr->rsize > 2)) 
			nmp = w_sap_r_dlhdr(info_ptr);
		else
			nmp = w_sap_dlhdr(info_ptr);
	}
	if (nmp == NULL) {
		tok->tok_stats->toks_nobuffer++;
		return E_NOBUFFER;
	}

	/* need to make it control info */
	dlp = (union DL_primitives *)nmp->b_rptr;
	nmp->b_datap->db_type = M_PROTO;
	/* prim_type passed in from calling routine */
	dlp->unitdata_ind.dl_primitive = prim_type;

	/* Point to the data */
	mp->b_rptr = info_ptr->data_ptr;

	if (mp->b_rptr == mp->b_wptr) {
		mblk_t *nullblk;

		/* get rid of null block */
		nullblk = mp;
		mp = unlinkb(nullblk);
		freeb(nullblk);
	}

	linkb(nmp, mp);
	putnext(q, nmp);
	return (E_OK);
}

/*
 * process receipt of an LLC packet
 * UI,XID and TEST messages are supported
 */

recv_llc(tok, q, mp)
struct tokdev *tok;
queue_t *q;
mblk_t *mp;
{
register struct	llc_info *info_ptr;

ushort	pkt_type;	/* Type of packet DL_802, DL_SNAP, DL_ROUTE */
struct	llc_info	pkt_info;

	tok->tok_dstats->toks_rpkts++;
	tok->tok_dstats->toks_rbytes += msgdsize(mp);

	/* Parse the LLC header and get information back */
	info_ptr = &pkt_info;
	pkt_type = p_llc_hdr(mp->b_rptr,info_ptr);

	switch(info_ptr->control) {
	case LLC_UI:
		if((info_ptr->dsap==LLC_NULL_SAP)||(pkt_type&DL_RESPONSE)) {
			/* No UI message handled for NULL SAP's */
			/* No response PDU's for UI messages */
			freemsg(mp);
			return E_OK;
		}
		return (pnext_msg(pkt_type, info_ptr, tok, q, mp,
					DL_UNITDATA_IND));
		break;
	case LLC_XID:
	case LLC_XID|LLC_P:
		return (do_xid_msg(pkt_type,info_ptr,tok,q,mp));
		break;
	 case LLC_TEST:
	 case LLC_TEST|LLC_P:
		return(do_test_msg(pkt_type,info_ptr,tok,q,mp));
		break;
	 default:
		printf ("recv_llc:(Unknown LLC type)\n");
		freemsg(mp);
		break;
	}
	return E_OK;
}

/*
 */
llccmds(q, mp)
queue_t *q;
mblk_t *mp;
{
union DL_primitives *dlp;

	dlp = (union DL_primitives *)mp->b_rptr;

	switch ((int) dlp->dl_primitive) {
	case DL_BIND_REQ:
		return tokbind(q, mp);
	case DL_UNBIND_REQ:
		return tokunbind(q, mp);
	case DL_UNITDATA_REQ:
		return tokunitdata(q, mp, LLC_UI);
	case DL_TEST_REQ:
		if (dlp->test_req.dl_flag)
			return tokunitdata(q, mp, (LLC_TEST|LLC_P));
		else
			return tokunitdata(q, mp, LLC_TEST);
	case DL_XID_REQ:
	case DL_XID_RES:
		return tokxiddata(q, mp);    
	case DL_INFO_REQ:
		return tokinforeq(q, mp);
	}
	return DL_NOTSUPPORTED;
}


/*
 * tokbind determines if a SAP is already allocated and
 * whether it is legal to do the bind at this time
 */

tokbind(q, mp)
queue_t *q;
mblk_t *mp;
{
register int i;
int sap,old;
union DL_primitives *dlp;
register struct tokdev *tok, *ttok;
struct tokparam *tokp;
caddr_t psrb;
srb_cmd_t *ptr;
mblk_t *nmp;

	dlp = (union DL_primitives *)mp->b_rptr;
	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;
	sap =  dlp->bind_req.dl_sap;

	if (tok->tok_state != DL_UNBOUND)
		return DL_OUTSTATE;
	ttok = &tokdevs[tokp->tok_firstd];
	for (i=0; i<tok->tok_macpar->tok_minors; i++,ttok++) {
		/*
		 * exit if any other device for this board is already bound to
		 * the same SAP
		 */
		if ((ttok->tok_state != DL_UNBOUND) && (ttok->tok_sap == sap))
	 		return DL_BOUND;	/* not a valid bind */
	}


	/* need to determine the type of address being used */
	/* values <= 0xFF imply LLC 802.2 type packets and */
	/* values > 0xFF imply LLC 802.2 SNAP addressing */

	if (sap <= MAXSAPVALUE) {
		tok->tok_type = DL_802;
		if ( (sap & LLC_GROUP_ADDR) || 
                   (sap == LLC_GLOBAL_SAP) || (!sap)) {
			tok->tok_state = DL_UNBOUND;
			return DL_BADADDR;
		}
	} else
		tok->tok_type = DL_SNAP;

	old = splstr();

	tok->tok_sap = sap;
	tok->tok_state = DL_BIND_PENDING;

	mp->b_datap->db_type = M_DATA;
	ptr = (srb_cmd_t *)mp->b_rptr;
	ptr->srb_cmd = SRB_OPEN_SAP;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	ptr->srb_cmd_param.open_sap.tok_no = tok->tok_no;

	if (tok->tok_type == DL_802) {
		ptr->srb_cmd_param.open_sap.ssap = sap;
		if ( (tokp->sched_flags & TX_BUSY) || (tokp->cur_msg != NULL)) {
			tokp->sched_flags |= TX_QUEUED;
			putq(q,mp);
		} else
			allocate_SAP(tok,mp);
	} else {
		ptr->srb_cmd_param.open_sap.ssap = LLC_SNAP_SAP;
		if (tokp->snap_bound++ == 0) {
			if ( (tokp->sched_flags & TX_BUSY) || 
						(tokp->cur_msg != NULL)) {
				tokp->sched_flags |= TX_QUEUED;
				putq(q,mp);
			} else
				allocate_SAP(tok,mp);
		} else {
			if ((nmp = allocb(sizeof(union DL_primitives), 
							BPRI_MED)) == NULL) {
				tok->tok_state = DL_UNBOUND;
				cmn_err(CE_WARN, "IBM TOKEN RING running out of streams buffers");
				splx(old);
				return DL_SYSERR;
			}
			tok->tok_state = DL_IDLE;
			tok->tok_station_id = tokp->snap_station_id;
			(void)tok_bind_ack(tok,nmp);
		}
	}
	splx(old);
	return E_OK;
}

void
tok_ok_ack(tok,nmp)
struct tokdev *tok;
mblk_t *nmp;
{
union DL_primitives *dlp;

	nmp->b_datap->db_type = M_PCPROTO; /* acks are PCproto's */
	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->ok_ack.dl_primitive = DL_OK_ACK;
	dlp->ok_ack.dl_correct_primitive = DL_UNBIND_REQ;
	nmp->b_wptr = nmp->b_rptr + DL_OK_ACK_SIZE;
	qreply(tok->tok_qptr, nmp);
}

void
tok_bind_ack(tok,nmp)
struct tokdev *tok;
mblk_t *nmp;
{
union DL_primitives *dlp;

	nmp->b_datap->db_type = M_PCPROTO; /* acks are PCproto's */
	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->bind_ack.dl_primitive = DL_BIND_ACK;
	dlp->bind_ack.dl_sap = tok->tok_sap;
	dlp->bind_ack.dl_addr_length = LLC_LSAP_LEN+MAC_ADDR_LEN;
	dlp->bind_ack.dl_addr_offset = DL_BIND_ACK_SIZE;
	bcopy(&tok->tok_sap, ((caddr_t)dlp)+DL_BIND_ACK_SIZE+MAC_ADDR_LEN, 
							LLC_LSAP_LEN);
	nmp->b_wptr = nmp->b_rptr + 
			DL_BIND_ACK_SIZE + MAC_ADDR_LEN + LLC_LSAP_LEN;
	bcopy(tok->tok_macpar->tok_macaddr, 
		((caddr_t)dlp)+DL_BIND_ACK_SIZE, MAC_ADDR_LEN);
	qreply(tok->tok_qptr,nmp);
}
/*
 * tokunbind performs an unbind of an LSAP or ether type on the stream
 * The stream is still open and can be re-bound
 */

tokunbind(q, mp)
queue_t *q;
mblk_t *mp;
{
struct tokdev *tok;
union DL_primitives *dlp;
mblk_t *nmp;
caddr_t psrb;
struct tokparam *tokp;
int old,i;
srb_cmd_t *ptr;

	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;

	if (tok->tok_state != DL_IDLE)
		return DL_OUTSTATE;

	old = splstr();

	tok->tok_state = DL_UNBIND_PENDING;
	if ( (tok->tok_type == DL_802) || (--tokp->snap_bound == 0) ) {
		mp->b_datap->db_type = M_DATA;
		ptr = (srb_cmd_t *)mp->b_rptr;
		ptr->srb_cmd = SRB_CLOSE_SAP;
		ptr->srb_cmd_param.close_sap.station_id = tok->tok_station_id;
		ptr->srb_cmd_param.close_sap.tok_no = tok->tok_no;
		mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
		if ((tokp->sched_flags & TX_BUSY) ||
						(tokp->cur_msg != NULL)) {
			tokp->sched_flags |= TX_QUEUED;
			putq(q,mp);
		} else
			deallocate_SAP(tok,mp);
	} else {
		tok->tok_state = DL_UNBOUND;
		flushq( tok->tok_qptr, FLUSHALL);
		flushq( RD(tok->tok_qptr), FLUSHALL);
		if ((nmp= allocb(sizeof(union DL_primitives),BPRI_MED))!= NULL)
			(void)tok_ok_ack(tok,nmp);
	}
	splx(old);
	return E_OK;
}

/* allocate_SAP reserves space for a SAP on the adapter. */
int 
allocate_SAP(tok,mp)
struct tokdev *tok;
mblk_t *mp;
{
register i;
struct tokparam *tokp = tok->tok_macpar;
srb_cmd_t *ptr = (srb_cmd_t *)mp->b_rptr;

	tokp->sched_flags |= TX_BUSY;
  	/* Zero out the SRB. */
	 for (i = 0; i < SRB_AREA_SIZE; i++)
     		*(tok->tok_macpar->SRB_area + i) = 0x0;
  	*(tok->tok_macpar->SRB_area)  = SRB_OPEN_SAP;
  	*(tok->tok_macpar->SRB_area + 16) = ptr->srb_cmd_param.open_sap.ssap; 
  	*(tok->tok_macpar->SRB_area + 17) = SAP_OPTIONS;
  	*(tok->tok_macpar->SRB_area + 2)  = 0xFE;
  	*(tok->tok_macpar->aca_area + ISRA_ODD_SET) = 0x28;
	tok->tok_macpar->cur_msg = mp;
}

int
deallocate_SAP(tok,mp)
struct tokdev *tok;
mblk_t *mp;
{
int i;
struct tokparam *tokp = tok->tok_macpar;
srb_cmd_t *ptr = (srb_cmd_t *)mp->b_rptr;
unsigned short station_id = ptr->srb_cmd_param.close_sap.station_id;

	 tokp->sched_flags |= TX_BUSY;
	 for (i = 0; i < SRB_AREA_SIZE; i++)
     		*(tokp->SRB_area + i) = 0x0;
  	*(tokp->SRB_area) = SRB_CLOSE_SAP;
  	*(tokp->SRB_area + SRB_RETCODE_OFFSET) = SRB_INITIATED;
  	*(tokp->SRB_area + SRB_STATION_ID_OFFSET)   = station_id >> 8;
  	*(tokp->SRB_area + SRB_STATION_ID_OFFSET+1) = station_id & 0xff;
  	*(tokp->aca_area + ISRA_ODD_SET) = 0x28;
	tokp->cur_msg = mp;
}



mblk_t *
wsap_r_hdr(mp,tok,msg_type)
mblk_t *mp;
struct tokdev *tok;
int	msg_type;
{
register int		offset;
mblk_t			*nmp;
struct	mac_llc_hdr	*hdr;
struct	llctype		*sap_hdr;
unchar			*route_ptr;
short			r_cnt;
struct llcc 		*llcp;
union DL_primitives	*dlp;
int			i;
unsigned char *d_ptr = mp->b_rptr;
srb_cmd_t 		*psrb;
unsigned char 		dsap;


	dlp = (union DL_primitives *) d_ptr;
	if (msg_type == LLC_UI) {
		offset = (int)(dlp->unitdata_req.dl_dest_addr_offset);
		r_cnt = dlp->unitdata_req.dl_dest_addr_length - sizeof(struct llcc);
	} else {
		offset = (int)(dlp->test_req.dl_dest_addr_offset);
		r_cnt = dlp->test_req.dl_dest_addr_length - sizeof(struct llcc);
	}
	/* make a valid header for transmit */

	if ((nmp = allocb(LLC_HDR_SIZE+MAX_ROUTE_FLD, BPRI_MED)) == NULL)
		return NULL;

	llcp = (struct llcc *)((caddr_t)(d_ptr + offset));

	/* Copy source and destination MAC addresses */
	hdr = (struct mac_llc_hdr *)nmp->b_rptr;
	bcopy((caddr_t)llcp->lbf_addr, hdr->mac_dst, MAC_ADDR_LEN);
	bcopy(tok->tok_macpar->tok_macaddr, hdr->mac_src, MAC_ADDR_LEN);

	/* Make sure the source route bit in the source addr is set */
	hdr->mac_src[0] |= 0x80;

	/* Copy routing information */
	offset += sizeof(struct llcc);
	route_ptr = (unchar *)(d_ptr + offset);
	if ((r_cnt & 0x01) || (r_cnt < 2) || (r_cnt > 18)) {
		/* Odd source routing field, or bad length */
		freeb(nmp);
		return (NULL);
	}
	for (i=0;i<r_cnt;i++)
		hdr->llc.ibm_route[i] = route_ptr[i];

	dsap = llcp->lbf_sap;
	sap_hdr = (struct llctype *) ((char *)hdr + 2*MAC_ADDR_LEN + r_cnt); 
	sap_hdr->llc_dsap = dsap;
	sap_hdr->llc_ssap = tok->tok_sap;
	sap_hdr->llc_control = msg_type;
	nmp->b_wptr = nmp->b_rptr + LLC_HDR_SIZE + r_cnt;

	mp->b_datap->db_type = M_DATA;
	psrb = (srb_cmd_t *)mp->b_rptr;
	if (msg_type == LLC_UI)
		psrb->srb_cmd = SRB_TX_UI_FRAME;
	else
		psrb->srb_cmd = SRB_TX_TEST_CMD;
	psrb->srb_cmd_param.xmit_frame.dsap = dsap;
	psrb->srb_cmd_param.xmit_frame.lan_hdr_size = MIN_LAN_HDR_SIZE + r_cnt;
	psrb->srb_cmd_param.xmit_frame.station_id = tok->tok_station_id;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	/* Give back the new header */
	return (nmp);
}

mblk_t *
wsaphdr(mp,tok,msg_type)
mblk_t *mp;
struct tokdev *tok;
int msg_type;
{
union DL_primitives *dlp;
register struct mac_llc_hdr *hdr;
struct llcc *llcp;
mblk_t *nmp;
unsigned char *d_ptr = mp->b_rptr;
unsigned char dsap;
srb_cmd_t *psrb;

	/* make a valid header for transmit */

	if ((nmp = allocb(LLC_HDR_SIZE, BPRI_MED)) == NULL) {
		tok->tok_stats->toks_nobuffer++;
		return NULL;
	}

	dlp = (union DL_primitives *) d_ptr;
	if (msg_type == LLC_UI)
		llcp = (struct llcc *)(d_ptr + dlp->unitdata_req.dl_dest_addr_offset);
	else
		llcp = (struct llcc *)(d_ptr + dlp->test_req.dl_dest_addr_offset);

	/* Copy MAC addresses */

	hdr = (struct mac_llc_hdr *)nmp->b_rptr;
	bcopy((caddr_t)llcp->lbf_addr, hdr->mac_dst, MAC_ADDR_LEN);
	bcopy(tok->tok_macpar->tok_macaddr, hdr->mac_src, MAC_ADDR_LEN);

	dsap = llcp->lbf_sap;

	hdr->llc.llc_sap.llc_dsap = llcp->lbf_sap;
	hdr->llc.llc_sap.llc_ssap = tok->tok_sap;
	hdr->llc.llc_sap.llc_control = msg_type;
	nmp->b_wptr = nmp->b_rptr + LLC_HDR_SIZE;

	mp->b_datap->db_type = M_DATA;
	psrb = (srb_cmd_t *)mp->b_rptr;
	if (msg_type == LLC_UI)
		psrb->srb_cmd = SRB_TX_UI_FRAME;
	else
		psrb->srb_cmd = SRB_TX_TEST_CMD;
	psrb->srb_cmd_param.xmit_frame.dsap = dsap;
	psrb->srb_cmd_param.xmit_frame.lan_hdr_size = MIN_LAN_HDR_SIZE;
	psrb->srb_cmd_param.xmit_frame.station_id = tok->tok_station_id;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	return nmp;
}

mblk_t	*
wsnaphdr(mp,tok,msg_type)
mblk_t *mp;
struct tokdev *tok;
int	msg_type;
{
register struct mac_llc_hdr *hdr;
register int	offset;
mblk_t *nmp;
union DL_primitives *dlp;
unsigned char	*d_ptr = mp->b_rptr;
unsigned char dsap;
srb_cmd_t *psrb;
register int i;

	/* make a valid header for transmit */

	if ((nmp = allocb(LLC_EHDR_SIZE, BPRI_MED)) == NULL) {
		/* failed to get buffer */
		tok->tok_stats->toks_nobuffer++;
		return NULL;
	}

	dlp = (union DL_primitives *) d_ptr;
	if (msg_type == LLC_UI) {
		offset = dlp->unitdata_req.dl_dest_addr_offset;
		dsap = LLC_SNAP_SAP;
	} else {
		offset = dlp->test_req.dl_dest_addr_offset;
		dsap = *((unsigned char *)(d_ptr + offset + MAC_ADDR_LEN));
	}
	hdr = (struct mac_llc_hdr *)nmp->b_rptr;
	bcopy((caddr_t)(d_ptr + offset), hdr->mac_dst, MAC_ADDR_LEN);
	bcopy(tok->tok_macpar->tok_macaddr, hdr->mac_src, MAC_ADDR_LEN);

	hdr->llc.llc_snap.llc_dsap = dsap;
	hdr->llc.llc_snap.llc_ssap = LLC_SNAP_SAP;
	hdr->llc.llc_snap.org_code[0] = 0;
	hdr->llc.llc_snap.org_code[1] = 0;
	hdr->llc.llc_snap.org_code[2] = 0;
	hdr->llc.llc_snap.ether_type = ntohs(tok->tok_sap);
	hdr->llc.llc_snap.llc_control = msg_type;
	nmp->b_wptr = nmp->b_rptr + LLC_EHDR_SIZE;

	mp->b_datap->db_type = M_DATA;
	psrb = (srb_cmd_t *)mp->b_rptr;
	if (msg_type == LLC_UI)
		psrb->srb_cmd = SRB_TX_UI_FRAME;
	else
		psrb->srb_cmd = SRB_TX_TEST_CMD;
	psrb->srb_cmd_param.xmit_frame.dsap = dsap;
	psrb->srb_cmd_param.xmit_frame.lan_hdr_size = MIN_LAN_HDR_SIZE;
	psrb->srb_cmd_param.xmit_frame.station_id = tok->tok_station_id;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	
	return nmp;
}

/*
 *	Fill in a header with SNAP addressing and routing information.
 */

mblk_t	*
wsnap_r_hdr(mp,tok,msg_type)
mblk_t *mp;
struct tokdev *tok;
int	msg_type;
{
struct	mac_llc_hdr	*hdr;
int offset,i;
mblk_t			*nmp;
struct	llc_snap	*snap_hdr;
unchar			*route_ptr;
short			r_cnt;
union DL_primitives	*dlp;
unsigned char *d_ptr = mp->b_rptr,dsap;
srb_cmd_t *psrb;

	/* make a valid header for transmit */
	if ((nmp = allocb(LLC_EHDR_SIZE+MAX_ROUTE_FLD, BPRI_MED)) == NULL)
		return NULL;

	dlp = (union DL_primitives *) d_ptr;
	if (msg_type == LLC_UI) {
		offset = dlp->unitdata_req.dl_dest_addr_offset;
		dsap = LLC_SNAP_SAP;
		r_cnt = dlp->unitdata_req.dl_dest_addr_length - sizeof(struct llca);
	} else {
		offset = dlp->test_req.dl_dest_addr_offset;
		dsap = *((unsigned char *)(d_ptr + offset + MAC_ADDR_LEN));
		r_cnt = dlp->test_req.dl_dest_addr_length - sizeof(struct llca);
	}
	/* Copy source and destination MAC addresses */

	hdr = (struct mac_llc_hdr *)nmp->b_rptr;
	bcopy((caddr_t)(d_ptr + offset), hdr->mac_dst, MAC_ADDR_LEN);
	bcopy(tok->tok_macpar->tok_macaddr, hdr->mac_src, MAC_ADDR_LEN);

	/* Make sure the source route bit in the source addr is set */
	hdr->mac_src[0] |= 0x80;

	/* Copy routing information */
	offset += sizeof(struct llca);
	route_ptr = (unchar *)(d_ptr + offset);
	if ((r_cnt & 0x01) || (r_cnt < 2) || (r_cnt > 18)) {
		/* Odd source routing field, or bad length */
		freeb(nmp);
		return (NULL);
	}
	for (i=0;i<r_cnt;i++)
		hdr->llc.ibm_route[i] = route_ptr[i];

	snap_hdr = (struct llc_snap *) ((char *)hdr + 2*MAC_ADDR_LEN + r_cnt); 
	snap_hdr->llc_dsap = dsap;
	snap_hdr->llc_ssap = LLC_SNAP_SAP;
	snap_hdr->org_code[0] = 0;
	snap_hdr->org_code[1] = 0;
	snap_hdr->org_code[2] = 0;
	snap_hdr->ether_type = ntohs(tok->tok_sap);
	snap_hdr->llc_control = msg_type;
	nmp->b_wptr = nmp->b_rptr + LLC_EHDR_SIZE + r_cnt;

	mp->b_datap->db_type = M_DATA;
	psrb = (srb_cmd_t *)mp->b_rptr;
	if (msg_type == LLC_UI)
		psrb->srb_cmd = SRB_TX_UI_FRAME;
	else
		psrb->srb_cmd = SRB_TX_TEST_CMD;
	psrb->srb_cmd_param.xmit_frame.dsap = dsap;
	psrb->srb_cmd_param.xmit_frame.lan_hdr_size = MIN_LAN_HDR_SIZE + r_cnt;
	psrb->srb_cmd_param.xmit_frame.station_id = tok->tok_station_id;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	/* Give back the new header */
	return (nmp);
}

/*
 * 	tokunitdata sends a datagram
 *	destination address/lsap is in M_PROTO message (1st message)
 *	data is in remainder of message
 */

tokunitdata(q,mp,msg_type)
queue_t *q;
mblk_t *mp;
int	msg_type;
{
struct tokdev *tok;
mblk_t *nmp, *tmp;
union DL_primitives *dlp;
register struct mac_llc_hdr *hdr;
short	src_length,length;
struct tokparam *tokp;
int old;
ulong frame_length;

	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;
	if (tok->tok_state != DL_IDLE)
		return DL_OUTSTATE;

	dlp = (union DL_primitives *)mp->b_rptr;

	if (msg_type == LLC_UI)
		src_length = dlp->unitdata_req.dl_dest_addr_length;
	else
		src_length = dlp->test_req.dl_dest_addr_length;

	length = 0;
	tmp = mp->b_cont;

	while (tmp != NULL) {
		length +=  (tmp->b_wptr - tmp->b_rptr);
		tmp = tmp->b_cont;
	}

	/* Check the data length for maximum */
	
	if (length > tok->tok_macpar->tok_maxpkt)
		return DL_BADDATA;

	tmp = unlinkb(mp);
	mp->b_cont = NULL;

	if (tok->tok_type == DL_802) {
		if (src_length == LLC_LIADDR_LEN) {
			/* 802.2 header. No routing information */
			nmp = wsaphdr(mp,tok,msg_type);
		} else if (src_length <= LLC_LIR_MAX_LEN) {
			/* 802.2 header. With routing information */
			nmp = wsap_r_hdr(mp,tok,msg_type);
		} else {
			freemsg(tmp);
			return DL_BADADDR;
		}
	} else {
		if (src_length == LLC_ENADDR_LEN) {
			/* SNAP header. No Routing information */
			nmp = wsnaphdr(mp,tok,msg_type);
		} else if (src_length <= LLC_ENR_MAX_LEN) {
			/* SNAP header. With routing information */
			nmp = wsnap_r_hdr(mp,tok,msg_type);
		} else  {
			freemsg(tmp);
			return DL_BADADDR;
		}
	}

	if (nmp == NULL) {
		/* mp will be freed by llccmds */
		freemsg(tmp);
		return DL_SYSERR;
	}
	linkb(nmp,tmp);

	/* Statistics */
	frame_length = msgdsize(nmp);
	tok->tok_dstats->toks_xpkts++;
	tok->tok_dstats->toks_xbytes += frame_length;
    	tokstats[tokp->tok_index].toks_xpkts++;
    	tokstats[tokp->tok_index].toks_xbytes += frame_length;

	if (toklocal(nmp,tok)) {
		freeb(mp);
		tokloop(nmp,tok);
		return E_OK;
	}

	if (tokmulticast(nmp,tok))
		tokloop(dupmsg(nmp),tok);

	linkb(mp,nmp);

	old = splstr();

	if ( (tokp->sched_flags & TX_BUSY) || (tokp->cur_msg != NULL)) {
		tokp->sched_flags |= TX_QUEUED;
		putq(q,mp);
	} else {
		tok_setup_send(tok, mp);
	}
	splx(old);
	return E_OK;
}

/*
 * tokinforeq
 * generate the response to an info request
 */

tokinforeq(q, mp)
queue_t *q;
mblk_t  *mp;
{
struct tokdev *tok;
mblk_t *nmp;
register union DL_primitives *dlp;

	tok = (struct tokdev *)q->q_ptr;

	if ((nmp = allocb(sizeof(union DL_primitives), BPRI_MED)) == NULL) {
		tok->tok_stats->toks_nobuffer++;
		return E_NOBUFFER;
	}

	freemsg(mp);
	nmp->b_datap->db_type = M_PCPROTO; /* acks are PCproto's */

	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->info_ack.dl_primitive = DL_INFO_ACK;
	dlp->info_ack.dl_max_sdu = tok->tok_macpar->tok_maxpkt ;
	dlp->info_ack.dl_min_sdu = MINSEND;
	if (tok->tok_type == DL_802)
		dlp->info_ack.dl_addr_length = LLC_LIADDR_LEN;
	else
		dlp->info_ack.dl_addr_length = LLC_ENADDR_LEN;

	dlp->info_ack.dl_addr_offset = sizeof(dl_info_ack_t);
	dlp->info_ack.dl_mac_type = DL_TPR;
	dlp->info_ack.dl_service_mode = DL_CLDLS;
	dlp->info_ack.dl_provider_style = DL_STYLE1;
	dlp->info_ack.dl_current_state = tok->tok_state;
	dlp->info_ack.dl_version = DL_VERSION_2;
	dlp->info_ack.dl_growth = 0;
	dlp->info_ack.dl_qos_length = 0;
	dlp->info_ack.dl_qos_offset = 0;
	dlp->info_ack.dl_qos_range_length = 0;
	dlp->info_ack.dl_qos_range_offset = 0;

	nmp->b_wptr = nmp->b_rptr + DL_INFO_ACK_SIZE;
	qreply(q, nmp);
	return E_OK;
}


void
tok_adapter_close(tokp)
struct tokparam *tokp;
{
	int old ;
  	*(tokp->SRB_area) = SRB_CLOSE;
  	*(tokp->SRB_area + 2)  = 0xFE;
        *(tokp->aca_area + ISRA_ODD_SET) = 0x28;
}


/*
 * tok_setup_send  saves the streams msg, because we do not have the tx-buffer 
 * pointer yet (we get it later from the adapter)
 * it posts the tx-cmd. and then it is up to the interrupt handler to
 * copy the streams-data to the tx-buffer and kick off the actual tx.
*/

int 
tok_setup_send(tok,msg)
struct tokdev *tok;
mblk_t *msg;
{
struct tokparam *tokp;     /* Token device parameters.            */
caddr_t srb_ptr;   /* Pointer to the command block (srb). */
srb_cmd_t *psrb;

  	tokp = tok->tok_macpar;

  	if (tokp->tok_init == FALSE) {
		freemsg(msg);
		return 0;
	}
	tokp->sched_flags |= TX_BUSY;
  	tokp->cur_msg = msg;
	psrb = (srb_cmd_t *)msg->b_rptr;

  	*(tokp->SRB_area)               	    = psrb->srb_cmd; 
  	*(tokp->SRB_area + SRB_RETCODE_OFFSET)      = SRB_INITIATED;
  	*(tokp->SRB_area + SRB_STATION_ID_OFFSET)   
			= psrb->srb_cmd_param.xmit_frame.station_id >> 8;
  	*(tokp->SRB_area + SRB_STATION_ID_OFFSET+1) 
			= psrb->srb_cmd_param.xmit_frame.station_id & 0xff;

  	/* Send it. */

  	*(tokp->aca_area + ISRA_ODD_SET) = 0x28;
	tokp->tok_timer_val = 3;	/* A total of 3 * 3 secs */
  	return(0);
}

/*
 * toksend is called when a packet is ready to be transmitted. A pointer
 * to a M_PROTO or M_PCPROTO message that contains the packet is passed
 * to this routine as a parameter. The complete LLC header is contained
 * in the message block's control information block, and the remainder
 * of the packet is contained within the M_DATA message blocks linked to
 * the main message block.
 */

toksend(tokp, correlator)
struct tokparam *tokp;
char correlator;
{
register unsigned short length;     /* total length of packet.            */
mblk_t          *mb;       /* Ptr to message previously saved.   */
register mblk_t	*mp;	      /* ptr to msg block containing packet */
unchar		*txbuf;    /* ptr to tx buffer area on adapter.  */
register int	i,j;
int		efl;
unsigned	DHB_addr;
caddr_t		asb_ptr;
srb_cmd_t *psrb;

  	DHB_addr = ((*(tokp->ARB_area + 6) & 0xff) << 8) + 
               ((*(tokp->ARB_area + 7)) & 0xff);

  	txbuf = (unchar *) (tokp->ram_area + DHB_addr);
  	length = 0;
	psrb = (srb_cmd_t *)tokp->cur_msg->b_rptr;

	/* DEBUGGING ONLY */

 
	/* The following two checks are for debugging purposes only */

	if (correlator != psrb->srb_cmd_param.xmit_frame.correlator)
		cmn_err(CE_WARN,"Illegal command correlator in the ARB\n");

	if (tokp->station_id != psrb->srb_cmd_param.xmit_frame.station_id)
		cmn_err(CE_WARN,"Illegal station id in the ARB\n");


  	/* Adjust for AC + FC fields. */
  	*(txbuf) = 0x10;
  	*(txbuf+1) = 0x40;
   	/* Need to add in the length of the AC and FC fields */
  	length += 2;  

  	mb   = tokp->cur_msg->b_cont;
  	bcopy((caddr_t) mb->b_rptr, txbuf+2, (int)(mb->b_wptr - mb->b_rptr));
  	length += (unsigned int)(mb->b_wptr - mb->b_rptr);

  	mp = mb->b_cont;

  	/*
   	* load the rest of the packet onto the board by chaining through
   	* the M_DATA blocks attached to the M_PROTO header. The list
   	* of data messages ends when the pointer to the current message
   	* block is NULL
   	*/

   	while (mp != NULL) {
     		bcopy( (caddr_t) mp->b_rptr, txbuf + length,
		   (int)(mp->b_wptr - mp->b_rptr));
     		length += (unsigned int)(mp->b_wptr - mp->b_rptr);
     		mp = mp->b_cont;
   	}

   	/* Now we are done with the streams msg. */
	
   	/* Now setup the ASB response to the adapter. */
   	*(tokp->ASB_area)     = psrb->srb_cmd;
   	*(tokp->ASB_area + 1) = correlator;
   	*(tokp->ASB_area + 2) = 0;
   	*(tokp->ASB_area + 3) = 0;
   	*(tokp->ASB_area + 4) = 
			psrb->srb_cmd_param.xmit_frame.station_id >> 8;
   	*(tokp->ASB_area + 5) = 
			psrb->srb_cmd_param.xmit_frame.station_id & 0xff;
   	*(tokp->ASB_area + 6) = length >> 8;
   	*(tokp->ASB_area + 7) = length & 0xff;
   	*(tokp->ASB_area + 8) = psrb->srb_cmd_param.xmit_frame.lan_hdr_size;
   	*(tokp->ASB_area + 9) = psrb->srb_cmd_param.xmit_frame.dsap;

   	/* packet loaded; now start the adapter transmission */
  	*(tokp->aca_area + ISRA_ODD_SET) = 0x16;  
   	freemsg(tokp->cur_msg);
   	tokp->cur_msg = NULL;

  	return(0);
}

/*
 * toklocal checks to see if the message is addressed to this
 * system by comparing with the board's address
 */

toklocal(mp,tok)
mblk_t *mp;
struct tokdev *tok;
{
	return bcmp((caddr_t)(mp->b_rptr), tok->tok_macpar->tok_macaddr, MAC_ADDR_LEN) == 0;
}

tokbroadcast(mp)
mblk_t *mp;
{
	if (bcmp((caddr_t)(mp->b_rptr), tokbaddr0, MAC_ADDR_LEN) == 0) {
		return 1;
	}
	if (bcmp((caddr_t)(mp->b_rptr), tokbaddr1, MAC_ADDR_LEN) == 0) {
		return 1;
	}
	return 0;
}

/*
 * tokmulticast checks to see if a multicast address that is
 * being listened to is being addressed. This routine returns a 1 if the
 * passed address is a multicast address and a 2 if the passed address is
 * a group address.
 */

tokmulticast(mp,tok)
mblk_t *mp;
struct tokdev *tok;
{
register int i;
register struct tokparam *tokp = tok->tok_macpar;

	if (tokp->tok_multicnt == 0) {
		/* no multicast/group addrs defined */
		return(0);
	}

	if (bcmp((caddr_t)(mp->b_rptr), tokp->tok_multiaddr, MAC_ADDR_LEN) == 0) {
		/* This address is a multicast address */
		return(1);
	}
	if (bcmp((caddr_t)(mp->b_rptr), tokp->tok_groupaddr, MAC_ADDR_LEN) == 0) {
		return(2);
	}
	return(0);
}

/*
 * tokrecv is called by the device interrupt handler to process an
 * incoming packet.  The appropriate link level parser/protocol handler
 * is called if a stream is allocated for that packet
 */

tokrecv(mp,tok)
mblk_t *mp;
struct tokdev *tok;	/* base of device array for this board */
{
register struct tokparam *tokp = tok->tok_macpar;
register int i;
mblk_t *nmp;
ushort	pkt_type;	/* Type of packet DL_802, DL_SNAP, DL_ROUTE */
struct 	llc_info	pkt_info;

	/* Parse the LLC header and get information back */
	pkt_type = p_llc_hdr(mp->b_rptr,&pkt_info);

	for (i=0; i < tokp->tok_minors; i++, tok++) {
		/* skip an unopened stream or an open but not bound stream */
		if ((tok->tok_qptr == NULL) || (tok->tok_state != DL_IDLE))
			continue;
		else if ((tok->tok_sap == pkt_info.dsap)
		   	|| (pkt_info.dsap == LLC_GLOBAL_SAP)) {

			/* if no room in the queue, skip this queue */
			if (!canput(RD(tok->tok_qptr)))
				continue;
			nmp = dupmsg(mp);
			/* enqueue for the service routine to process */
			putq(RD(tok->tok_qptr), mp);
			mp = nmp;
			if (nmp == NULL)
		  		break;
		} else if (pkt_info.dsap == LLC_NULL_SAP) {
			if (!canput(RD(tok->tok_qptr)))
				continue;
			putq(RD(tok->tok_qptr), mp);
			return;
		} else
			;
	}
	if (mp != NULL)
		 freemsg(mp);
}
/*
 * tokloop puts a formerly outbound message onto the input
 * queue.  Needed since the board can't receive messages it sends
 * to itself
 */

tokloop(mp,tok)
mblk_t *mp;
struct tokdev *tok;
{
 	if (mp != NULL)
		tokrecv(mp,tok - tok->tok_no);  
}

/*
 * tokack builds an error acknowledment for the primitive
 * All operations have potential error acknowledgments
 * unitdata does not have a positive ack
 */

void
tok_error_ack(q, primitive, err)
queue_t *q;
ulong primitive;
int err;
{
mblk_t *nmp;
union DL_primitives *dlp, *orig;

	if ((nmp=allocb(sizeof(union DL_primitives), BPRI_MED)) == NULL) {
		cmn_err(CE_WARN,
			"IBM TOKEN RING: running out of streams buffers\n");
		return;
	}

	/* sufficient resources to make an ACK */

	dlp = (union DL_primitives *)nmp->b_rptr;
	dlp->error_ack.dl_primitive = DL_ERROR_ACK;
 	/* this is the failing opcode */
	dlp->error_ack.dl_error_primitive = primitive;
	dlp->error_ack.dl_errno = err;
	dlp->error_ack.dl_unix_errno = 0;

	nmp->b_wptr = nmp->b_rptr + DL_ERROR_ACK_SIZE;
	nmp->b_datap->db_type = M_PCPROTO; /* acks are PCproto's */
	qreply(q, nmp);
}

/*
 * tokioctl handles all ioctl requests passed downstream.
 * This routine is passed a pointer to the message block with
 * the ioctl request in it, and a pointer to the queue so it can
 * respond to the ioctl request with an ack.
 */

tokioctl(q, mb)
queue_t	*q;
mblk_t	*mb;
{
unsigned char *dp;
register int i,j;
struct iocblk *iocp;
struct tokdev  *tok;
mblk_t *bt;
register struct tokparam *tokp;
struct tokparam *tokp2;
int inval;

	iocp = (struct iocblk *) mb->b_rptr;
	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;
	mb->b_datap->db_type = M_IOCACK; /* assume ACK */

	switch (iocp->ioc_cmd) {
	/* Numbers for the board */
	case DLGSTAT:
		if (iocp->ioc_count < (sizeof(struct tokstat))) {
			iocp->ioc_error = EINVAL;
			goto iocnak;
		}
		dp = mb->b_cont->b_rptr;
		bcopy((caddr_t)(&(tokstats[tokp->tok_index])), dp,
							sizeof(struct tokstat));
		mb->b_cont->b_wptr= mb->b_cont->b_rptr + sizeof(struct tokstat);
		iocp->ioc_rval = 0;
		iocp->ioc_count = sizeof(struct tokstat);
		qreply(q, mb);
		break;
	/* Numbers for a particular minor device */
	case DLGDEVSTAT:
		if (iocp->ioc_count < (sizeof(struct tokdevstat))) {
			iocp->ioc_error = EINVAL;
			goto iocnak;
		}
		dp = mb->b_cont->b_rptr;
		bcopy((caddr_t)tok->tok_dstats,dp,sizeof(struct tokdevstat));
		mb->b_cont->b_wptr = mb->b_cont->b_rptr + sizeof(struct tokdevstat);
		iocp->ioc_rval = 0;
		iocp->ioc_count = sizeof(struct tokdevstat);
		qreply(q, mb);
		break;
	case DLGBROAD:
	case DLGADDR:
	case MACIOC_GETADDR:
		 {

		 /* if we don't have a six byte data buffer send NAK */
		 if (iocp->ioc_count != MAC_ADDR_LEN) {
	  		iocp->ioc_error = EINVAL;
	  		goto iocnak;
		 }

		 /* get address of location to put data */
		 dp = mb->b_cont->b_rptr;

		 /* see which address is requested & move it in buffer */
		 if (iocp->ioc_cmd == DLGBROAD) {
	 		/* copy broadcast address */
	 		for (i = 0; i < MAC_ADDR_LEN; i++) {
				*dp = tokbaddr0[i];
		 		dp++;
			}
	 	}
		else {
	 	/* copy host's physical address */
	 		for (i = 0; i < MAC_ADDR_LEN; i++) {
		 		*dp = tokp->tok_macaddr[i];
		 		dp++;
			}
	 	}
		 /* send the ACK */
		 qreply(q, mb);
		 break;
	 }
	 default:
		/* iocp_error = --some default error value--; */
	 iocnak:
		/* NAK the ioctl request */
		mb->b_datap->db_type = M_IOCNAK;
		qreply(q, mb);

	} /* end switch */

} /* end tokioctl */

tokwaitloop(m)
unsigned m;
{
  while ((m--) > 0)
    tenmicrosec();
}

int 
tok_proc_open(tokp)
struct tokparam *tokp;
{
unsigned offset;
int  i;
unsigned char retcode;
ushort open_error_code;
  
  	if ((*tokp->SRB_area) != SRB_OPEN)
    		return;   /* Not the right interrupt. */
  	if ((*(tokp->SRB_area + 2)) == 0x0) {
		tokp->tok_init = TRUE; 
    		cmn_err(CE_CONT,"IBM TOKEN RING: Adapter:%d successfully connected to the ring\n",tokp->tok_index);

    		/* Get the four offsets to the SSB, SRB, ARB and ASB. */
    		offset = ((*(tokp->SRB_area + ASB_ADDRESS_OFFSET) & 0xff) << 8) + (unsigned char)(*(tokp->SRB_area + ASB_ADDRESS_OFFSET + 1));
    		tokp->ASB_area = physmap((paddr_t) (tokp->ram_addr + offset),
                               ASB_AREA_SIZE, KM_NOSLEEP); 
    		offset = ((*(tokp->SRB_area + ARB_ADDRESS_OFFSET) & 0xff) << 8) + (unsigned char)(*(tokp->SRB_area + ARB_ADDRESS_OFFSET + 1));
    		tokp->ARB_area = physmap((paddr_t) (tokp->ram_addr + offset),
                               ARB_AREA_SIZE, KM_NOSLEEP); 
    		offset = ((*(tokp->SRB_area + SSB_ADDRESS_OFFSET) & 0xff) << 8) + (unsigned char)(*(tokp->SRB_area + SSB_ADDRESS_OFFSET + 1));
    		tokp->SSB_area = physmap((paddr_t) (tokp->ram_addr + offset),
                               SSB_AREA_SIZE, KM_NOSLEEP); 
    		offset = ((*(tokp->SRB_area + SRB_ADDRESS_OFFSET) & 0xff) << 8) + (unsigned char)(*(tokp->SRB_area + SRB_ADDRESS_OFFSET + 1));

    		/* NOW free the initial SRB area. */
    		physmap_free((addr_t) tokp->SRB_area, INITIAL_SRB_SIZE, 0);
    		tokp->SRB_area = physmap((paddr_t) (tokp->ram_addr + offset),
                               SRB_AREA_SIZE, KM_NOSLEEP); 
		tokp->tok_adapter_state = ADAPTER_OPENED;
  	} else {
		tokp->tok_open_flag |= ADAPTER_OPEN_FAILED;
		tokp->tok_adapter_state = ADAPTER_CLOSED;
		retcode = *(tokp->SRB_area + 2);
		if (retcode == 0x7) {
			open_error_code = 
				ntohs(*((ushort *)(tokp->SRB_area + 6)));
			cmn_err(CE_WARN,"IBM TOKEN RING: Adapter failed to connect to the ring-- Reason:0x%x\n",open_error_code);
		} else
			cmn_err(CE_WARN,"IBM TOKEN RING: Adapter failed to connect to the ring-- Reason:0x%x\n",retcode);
    		physmap_free((addr_t) tokp->SRB_area, INITIAL_SRB_SIZE, 0);
		tokp->SRB_area = NULL;
    	}
	wakeup(&tokp->tok_open_flag);
}

tok_ring_change(tokp,error)
struct tokparam *tokp;
ushort error;
{
 int            i;
 unsigned char  cur_bit;
 ushort         cur_err;
 char           adapter_is_closed = FALSE;
 static char    *error_msgs[] = {
   "","","","","",
   "Ring recovery, i.e. the adapter is transmitting/receiving claim tokens",
   "Only station on the ring",
   "Error counter overflow",
   "A remove MAC frame has been received",
   "",
   "An adapter hw error has been detected following the beacon auto-removal",
   "An open or short circuit has been detected in the lobe data path",
   "The adapter is transmitting beacon frames",
   "This adapter has transmitted a soft error report MAC frame",
   "Beacon frames are being transmitted or received",
   "Absence of any received signal detected"};

	for (cur_err = error, i = 0; cur_err; cur_err = cur_err >> 1, i++) {
		cur_bit = cur_err & 0x01;
		/* Check for a critical error (one that closed the adapter). */
		if ((cur_bit) && ((i == 8) || (i == 10) || (i == 11))) {
			cmn_err(CE_WARN,"IBM TOKEN RING: %s\n",error_msgs[i]);
			(void)tok_adapter_shutdown(tokp);
			return - 1;
		}
	}
	return 0;
}

void 
ibmtokintr(intvec)
int intvec;			/* interrupt vector number */
{
register struct tokparam *tokp = tokparams;
int      intreg;
int      call_toksched = 0;	/* set if scheduler should be called */
unsigned char retcode, correlator,SRB_opcode, SSB_opcode, ARB_opcode;
unchar   regtmp;
int	   i;

	for (i = tok_board_cnt; i; i--, tokp++)
		if (tokp->tok_int == intvec 
		   || (tokp->tok_int == 2 && intvec == 9))
			break;
	if (i == 0) {
		 cmn_err(CE_CONT,"tokintr: intvec wrong: %x\n", intvec);
		 return;
	}
	if ( (tokp->tok_adapter_state == ADAPTER_DISABLED) || (tokp->tok_adapter_state == ADAPTER_CLOSED) ) {
		cmn_err(CE_WARN,"IBM TOKEN RING spurious interrupts received\n");
		tokp->tok_reset_retries = 0;
		tokp->tok_adapter_state = ADAPTER_DISABLED;
		tokp->tok_timer_val = 10;
		(void)tokreset(tokp);
		return;
	}
        /* Get the interrupt type. */
        regtmp = *(tokp->aca_area + ISRP_ODD_OFFSET);

        /* Reset interrupt. */
        *(tokp->aca_area + ISRP_ODD_RESET) = ~regtmp;

	if (!tokp->tok_reset_flag) {
		tok_proc_reset(tokp);
		goto tok_enable;
	}
	tokp->tok_timeouts = 0;
	tokp->tok_timer_val = -1;

	if ( ((*(tokp->aca_area + ISRP_EVEN_OFFSET)) & 
		(INTR_TIMER_INTR|INTR_ERROR_INTR|INTR_WORKSTATION_ACCESS))
			||
		((*(tokp->aca_area + ISRP_ODD_OFFSET)) & INTR_ADAPTER_CHECK) ) {
			tok_proc_adapter_error(tokp);
			goto tok_enable;
	}
	if (regtmp & INTR_SRB_RESPONSE) {

		/* Get the completion code and opcode (from our own SRB). */
		SRB_opcode = (*(tokp->SRB_area)) & 0xff;
 		retcode = *(tokp->SRB_area + SRB_RETCODE_OFFSET);

		/* Turn off the scheduling flag */
		tokp->sched_flags &= ~TX_BUSY;
	
		switch(SRB_opcode) {
		case SRB_OPEN_SAP:
			tokp->station_id = ntohs(*((ushort *)(tokp->SRB_area + 4)));
			tok_proc_open_sap(tokp,retcode);
			call_toksched++; 
			break;
		case SRB_CLOSE_SAP:
			tokp->station_id = ntohs(*((ushort *)(tokp->SRB_area + 4)));
			tok_proc_close_sap(tokp,retcode);
			call_toksched++;
			break;
		case SRB_TX_UI_FRAME:
		case SRB_TX_TEST_CMD:
		case SRB_TX_XID_CMD:
		case SRB_TX_XID_RESP:
		case SRB_TX_XID_RESP_FINAL:
               	  	correlator = (*(tokp->SRB_area + 1)) & 0xff;
			tok_proc_xmit(tokp,correlator,retcode);
			break;
		case SRB_OPEN:
          		tok_proc_open(tokp);
			goto tok_enable;
		case SRB_CLOSE:
			tok_proc_close(tokp);
			goto tok_enable;
		}
	}
	if (regtmp & INTR_SSB_RESPONSE)
		(void)tok_proc_ssb(tokp);

	if (regtmp & INTR_ARB_COMMAND) {
            	ARB_opcode = (*(tokp->ARB_area)) & 0xff;
          	/* Check if we got any data. */
            	if (ARB_opcode == SRB_RECEIVED_DATA) {
              		tok_rcv(tokp);
            	} else if (ARB_opcode == SRB_TRANSMIT_DATA) {
              		correlator = *(tokp->ARB_area + 1) & 0xff;
			tokp->station_id = ntohs(*((ushort *)(tokp->ARB_area + 4)));
              		toksend(tokp, correlator);
              		call_toksched++; 
            	} else {
                	/* Free up the ARB. */
                	*(tokp->aca_area + ISRA_ODD_SET) = 0x02;
                	if (ARB_opcode == SRB_RING_CHANGE)
                  		if (tok_ring_change(tokp,((*(tokp->ARB_area + 6)) << 8) + (*(tokp->ARB_area + 7))) == -1) {
					call_toksched = 0;
					goto tok_enable;
				}
              	}
	}
	if (regtmp & INTR_ASB_FREE) {
         	if (((*(tokp->ASB_area)) & 0xff) == SRB_TX_UI_FRAME)
           		toks_ifstats[tokp->tok_index].ifs_opackets++;
	}
	/* keep track of total interrupt count for this board */
	tokstats[tokp->tok_index].toks_intrs++;
tok_enable:
        /* Re-enable the interrupt controller. */
        outb(tokp->intr_refresh, RELEASE_VALUE);   
        if (call_toksched)
		toksched(&tokdevs[tokp->tok_firstd]);    
}

/*
 * tok_rcv called when a packet has been received.
 */

int 
tok_rcv(tokp)
struct tokparam *tokp;
{
  unsigned i;                /* Simple counter.                              */
  unsigned cur_pos;          /* Used when copying data from incoming frames. */
  ushort   station_id;       /* ID of the receiving station.                 */
  ushort   offset;           /* Byte offset to first incoming data buffer.   */
  ushort   first;            /* Same as above.                               */
  ushort   frame_length;     /* Length of entire data frame.                 */
  ushort   buffer_length;    /* Length of frame in current buffer.           */
  ushort   lan_hdr_length;   /* Actual lan header size.                      */
  ushort   dlc_hdr_length;   /* Actual LLC header size.                      */
  ushort   next_buff_offset; /* Next buffer offset (if more than 1).         */
  uchar_t  ncb_type;         /* Type of message received.                    */
  caddr_t  dp,cp;            /* Destination and source data pointers.        */
  mblk_t   *bp;              /* Streams msg that will receive the data.      */
  caddr_t  asb_ptr;
  int      efl;

  efl = intr_disable();  
  first = offset = (((*(tokp->ARB_area + RCV_BUFFER_OFFSET)) & 0xff) << 8) +
            (*(tokp->ARB_area + RCV_BUFFER_OFFSET + 1) & 0xff);
  station_id = (((*(tokp->ARB_area + RCV_STATION_OFFSET)) & 0xff) << 8) +
            (*(tokp->ARB_area + RCV_STATION_OFFSET + 1) & 0xff);
  ncb_type = *(tokp->ARB_area + RCV_NCB_TYPE_OFFSET) & 0xff;
#if defined(TOKDEBUG)
	if (tok_debug&RECV) {
		printf("Packet received, Rcv_buffer at 0x%x\n",offset);
        }
#endif
  /* Get the length of the packet. */
  frame_length   = (((*(tokp->ARB_area + RCV_FRAME_LEN_OFFSET)) & 0xff) << 8) +
                    ((*(tokp->ARB_area + RCV_FRAME_LEN_OFFSET + 1)) & 0xff);
  lan_hdr_length = (*(tokp->ARB_area + LAN_HDR_LEN_OFFSET)) & 0xff;
  dlc_hdr_length = (*(tokp->ARB_area + DLC_HDR_LEN_OFFSET)) & 0xff;
  cp = tokp->ram_area + offset;


  if ((frame_length > (unsigned) MAXPKT) 
	|| (frame_length <= ((unsigned) LLC_HDR_SIZE))) {
	/* Garbage packet */
#if defined(TOKDEBUG)
	if (tok_debug&ERROR)
		printf ("Garbage packet length = %d\n",frame_length);
#endif
  }
  else {
    /* Good length packet */
    toks_ifstats[tokp->tok_index].ifs_ipackets++;
    tokstats[tokp->tok_index].toks_rpkts++;
    tokstats[tokp->tok_index].toks_rbytes += frame_length;

    /* get a buffer from streams and copy packet */
    if ((bp = allocb(frame_length, BPRI_MED)) != NULL ||
        (bp = allocb(nextsize(frame_length), BPRI_MED)) != NULL)  {
      dp = (caddr_t) bp->b_wptr;
      bp->b_wptr = bp->b_rptr + frame_length - 2;
			
      /* Note here that a frame can be scattered over more than one buffer. */
      cur_pos = 0;
      offset  = RCV_BUFF_HDR_LENGTH + 2;   /* Skip over FC and AC fields. */
      buffer_length = (((*(cp + BUFFER_LEN_OFFSET)) & 0xff) << 8) +
                         ((*(cp + BUFFER_LEN_OFFSET + 1)) & 0xff) - 2;
      next_buff_offset = NEXT_BUFFER_OFFSET;
      do {
        bcopy(cp + offset, dp + cur_pos, buffer_length);
        offset = (((*(cp + next_buff_offset)) & 0xff) << 8) +
                 ((*(cp + next_buff_offset + 1)) & 0xff);
        cur_pos += buffer_length;
        if (offset > 0) {
          cp = tokp->ram_area + offset;
          buffer_length = (((*(cp + BUFFER_LEN_OFFSET-2)) & 0xff) << 8) +
                           ((*(cp + BUFFER_LEN_OFFSET-2 + 1)) & 0xff);
          offset  = RCV_BUFF_HDR_LENGTH - 2; 
          next_buff_offset = NEXT_BUFFER_OFFSET - 2;
        }
#if defined(TOKDEBUG)
	if (tok_debug&RECV)
          printf("next buff offset %d length %d\n", offset, buffer_length); 
#endif
      } while (offset > 0);

      /* Send packet upstream */
      tokrecv(bp,&tokdevs[tokp->tok_firstd]);   
    } else {
	/* No buffers up count */
	tokstats[tokp->tok_index].toks_nobuffer++;
#if defined(TOKDEBUG)
	if (tok_debug&(BUFFER|ERROR)) 
	  printf ("tok_rcv: no buffers (%d)\n",frame_length);
#endif
	}
  } /* End good packet length. */

  /* Now give the adapter a response (in the ASB area). */
  *(tokp->ASB_area) = SRB_RECEIVED_DATA;
  *(tokp->ASB_area + 2) = 0; /* Always give a good completion code. */
  *(tokp->ASB_area + 4) = station_id >> 8;
  *(tokp->ASB_area + 5) = station_id & 0xff;
  *(tokp->ASB_area + 6) = first >> 8;
  *(tokp->ASB_area + 7) = first & 0xff;

   /* Tell the adapter that we are done. */
   *(tokp->aca_area + ISRA_ODD_SET) = 0x16;

   intr_restore(efl);  
}

int 
tok_proc_reset(tokp)
struct tokparam *tokp;
{
int i;
init_srb_rsp_t *psrb;
addr_t   tmp_area;
ushort bring_up_code,init_offset = 0;
unsigned char retval;

  	init_offset = ((*(tokp->aca_area + WRBR_EVEN_OFFSET)) & 0xff) << 8;
  	init_offset += ((*(tokp->aca_area + WRBR_ODD_OFFSET)) & 0xff);
  	tokp->SRB_area = physmap((paddr_t) (tokp->ram_addr + init_offset), 
                     INITIAL_SRB_SIZE, KM_NOSLEEP);
	psrb = (init_srb_rsp_t *)(tokp->SRB_area);
	if ( ((retval = psrb->command) != 0x80) ||
		( (retval == 0x80) && ((bring_up_code = 
					ntohs(psrb->bring_up_code)) != 0x00)) ){
		cmn_err(CE_CONT,"IBM TOKEN RING: Reset sequence failed\n");
		if (retval == 0x80)
			cmn_err(CE_WARN,"IBM TOKEN RING: Reason for reset failure: %x",bring_up_code);
		physmap_free((addr_t)tokp->SRB_area,INITIAL_SRB_SIZE,0);
		tokp->SRB_area = NULL;
		tokp->tok_reset_retries++;
		if (tokp->tok_reset_retries == TOK_MAX_RETRIES) {
			tokp->tok_reset_flag = ADAPTER_RESET_FAILED;
			tokp->tok_adapter_state = ADAPTER_DISABLED;
			tokp->tok_timer_val = -1;
			tokp->tok_timeouts = 0;
			goto tok_reset_finish;
		} else {
			cmn_err(CE_CONT,"IBM TOKEN RING Repeating the reset sequence\n");
			tokreset(tokp);
			return;
		}
	}
	tokp->tok_reset_retries = 0;
	tokp->tok_timer_val = -1;
	tokp->tok_reset_flag = 1;
	tokp->tok_adapter_state = ADAPTER_CLOSED;

  	/*Zero out an area in RAM if we have to (last 512 bytes on last page).*/

  	if ((tokp->aip.initialize_RAM_area) && (tokp->aip.total_RAM == 64)) {
    		tmp_area = physmap((paddr_t) (tokp->ram_addr + ZERO_OUT_START_ADDR), ZERO_OUT_LENGTH, KM_NOSLEEP);
    		for (i = 0; i < ZERO_OUT_LENGTH; i++)
       			*(tmp_area + i) = 0;

    		physmap_free((addr_t) tmp_area, ZERO_OUT_LENGTH, 0);
  	}
  	/* Map in the whole RAM area. */
	tokp->tok_ram_size = tokp->aip.total_RAM * 1024;
  	tokp->ram_area = physmap((paddr_t)(tokp->ram_addr),(tokp->aip.total_RAM * 1024) , KM_NOSLEEP);
	physmap_free((addr_t)tokp->SRB_area,INITIAL_SRB_SIZE,0);
tok_reset_finish:
	wakeup(&tokp->tok_reset_flag);
}

int 
tok_exec_cmd(tok,msg)
struct tokdev *tok;
mblk_t *msg;
{
struct tokparam *tokp;     /* Token device parameters.            */
srb_cmd_t *psrb;

  	tokp = tok->tok_macpar;

	psrb = (srb_cmd_t *)msg->b_rptr;
	switch(psrb->srb_cmd) {
	case SRB_TX_UI_FRAME:
	case SRB_TX_TEST_CMD:
	case SRB_TX_XID_CMD:
	case SRB_TX_XID_RESP:
	case SRB_TX_XID_RESP_FINAL:
		return (tok_setup_send(tok, msg));
	case SRB_OPEN_SAP:
		return (allocate_SAP(tok, msg));
	case SRB_CLOSE_SAP:		
		return (deallocate_SAP(tok, msg));
	default:
		/* Should add a call to CE_PANIC here */
		freemsg(msg);
		return (0);
	}
}

void
tok_proc_close(tokp)
struct tokparam *tokp;
{
     	tokp->tok_init = FALSE;
	tokp->tok_adapter_state = ADAPTER_CLOSED;
	wakeup(&tokp->tok_close_flag);
	if (tokp->ASB_area != NULL) {
    		physmap_free((addr_t) (tokp->ASB_area),ASB_AREA_SIZE, 0); 
		tokp->ASB_area = NULL;
	}
	if (tokp->ARB_area != NULL) {
    		physmap_free((addr_t) (tokp->ARB_area),ARB_AREA_SIZE, 0); 
		tokp->ARB_area = NULL;
	}
	if (tokp->SSB_area != NULL) {
    		physmap_free((addr_t) (tokp->SSB_area),SSB_AREA_SIZE, 0); 
		tokp->SSB_area = NULL;
	}
	if (tokp->SRB_area != NULL) {
    		physmap_free((addr_t) (tokp->SRB_area),SRB_AREA_SIZE, 0); 
		tokp->SRB_area = NULL;
	}
	cmn_err(CE_CONT,"IBM TOKEN RING: Adapter:%d closed successfully\n",tokp->tok_index);
}

int
tok_proc_open_sap(tokp,retcode)
struct tokparam *tokp;
unsigned char retcode;
{
srb_cmd_t *psrb;
ushort tok_no;
struct tokdev *tok;
mblk_t *nmp;
union DL_primitives *dlp;

	psrb = (srb_cmd_t *)tokp->cur_msg->b_rptr;
	tok_no = psrb->srb_cmd_param.open_sap.tok_no;
	tok = &tokdevs[tok_no];

	if (retcode == SRB_SUCCESS) {
		if ((nmp = allocb(sizeof(union DL_primitives), 
						BPRI_MED)) == NULL) {
			tok->tok_state = DL_UNBOUND;
			goto proc_bind_sap_finish;
		}
		tok->tok_state = DL_IDLE;
		tok->tok_station_id = tokp->station_id;
		if (tok->tok_type == DL_SNAP)
			tokp->snap_station_id= tokp->station_id;
		(void)tok_bind_ack(tok,nmp);
	} else {
		cmn_err(CE_WARN,"SRB_OPEN_SAP fails for sap:%x reason:%x\n", tok->tok_sap,retcode); 
		tok->tok_state = DL_UNBOUND;
		(void)tok_error_ack(tok->tok_qptr,DL_BIND_REQ,
			(DLPI_TOKEN_ERROR_BASE + retcode));
	}
proc_bind_sap_finish:
	freemsg(tokp->cur_msg);
	tokp->cur_msg = NULL;
}

tok_proc_close_sap(tokp,retcode)
struct tokparam *tokp;
unsigned char retcode;
{
srb_cmd_t *psrb;
ushort tok_no;
mblk_t *nmp;
struct tokdev *tok;
union DL_primitives *dlp;

	psrb = (srb_cmd_t *)tokp->cur_msg->b_rptr;
	tok_no = psrb->srb_cmd_param.close_sap.tok_no;
	tok = &tokdevs[tok_no];
	tok->tok_state = DL_UNBOUND;
	flushq( tok->tok_qptr, FLUSHALL);
	flushq( RD(tok->tok_qptr), FLUSHALL);

	if (retcode == SRB_SUCCESS) {
		if ((nmp = allocb(sizeof(union DL_primitives),BPRI_MED))== NULL)
			goto tok_proc_close_finish;
		(void) tok_ok_ack(tok,nmp);
	} else {
		cmn_err(CE_WARN,"SRB_CLOSE_SAP fails for sap:%x reason:%x\n", tok->tok_sap,retcode); 
		(void)tok_error_ack(tok->tok_qptr,DL_UNBIND_REQ,
			(DLPI_TOKEN_ERROR_BASE + retcode));
	}
tok_proc_close_finish:
	freemsg(tokp->cur_msg);
	tokp->cur_msg = NULL;
}

tok_proc_xmit(tokp,correlator,retcode)
struct tokparam *tokp;
unsigned char correlator;
unsigned char retcode;
{
srb_cmd_t *psrb = (srb_cmd_t *)tokp->cur_msg->b_rptr;

	if ( (retcode != SRB_SUCCESS) && (retcode != SRB_IN_PROGRESS) ) {
           	toks_ifstats[tokp->tok_index].ifs_oerrors++;
		freemsg(tokp->cur_msg);
		tokp->cur_msg = NULL;
	} else
		psrb->srb_cmd_param.xmit_frame.correlator = correlator;
}

/* The following function is very hw specific.
   It looks for the IBM bios:
              "User defined."
           1. If an sdevice entry has been specified by the user
              only look for a board at the indicated I/O addr and
              shared mem addr.

   The bios is located at "return value" from inb(0xa20 or 0xa24),
   the value is
                 (Bit (0,1) = IRQ, Bit (2..7) = bios location)
                    ( The bios is within C0000 .. DE000)
*/

int
find_board(board_info)
struct tokparam *board_info;
{
int              i, j;                /* Simple counters.                  */
addr_t           cur_pos;             /* For finding IBM board identifier. */
char             check_string[CHANNEL_IDENTIFIER_LENGTH+1];
                                        /* Find IBM identification.          */
unsigned int            cur_candidate;/* Memory hole address.              */
short                   io_addr;      /* Current io-addr to try.           */
unsigned                boundary;    /* Are we currently on a boundary ?  */
unsigned                boundary_size;/* Memory boundary (16k,..,64k).     */
extern void             ibmtokintr();    /* Handle to the interrupt routine.  */
unsigned  long	  refbios,hw_config;
unsigned long	ramaddr;
unsigned char temp = 0,ref_intr_patt= 0,ref_intr = 0;

  	board_info->tok_noboard = TRUE;

	if ((drv_gethardware(IOBUS_TYPE,&bus_type)) < 0) {
		cmn_err(CE_CONT,"IBM TOKEN RING Unable to identify the hardware\n");
		return -1;
	}
	if (bus_type & BUS_MCA) {
		if (find_slot(ADAP_ID) == -1)
			return -1;
		if (pos_reg[1] & 0x1)
			board_info->tok_ioaddr = IBMTR_IOADDR_1;
		else
			board_info->tok_ioaddr = IBMTR_IOADDR_0;
	} else {
		if ( (board_info->tok_ioaddr != IBMTR_IOADDR_0) && 
			(board_info->tok_ioaddr != IBMTR_IOADDR_1) ) {
				cmn_err(CE_CONT,"IBM TOKEN RING: Illegal value:%x of ioaddr\n",board_info->tok_ioaddr);
			return 0;
		}
	}
     	if ( (hw_config = inb(board_info->tok_ioaddr)) != 0xff) {
					/* Check for invalid setting */
		if (bus_type & BUS_MCA) {
			refbios = ((pos_reg[2] & BIOS_MCA_MASK) << BIOS_MCA_SHIFT);
			board_info->bios_addr = refbios;
		} else {
			refbios = ((hw_config & BIOS_AT_MASK) << BIOS_AT_SHIFT) | BIOS_BIT19;
			if (board_info->bios_addr != refbios) {
				cmn_err(CE_CONT,"IBM TOKEN RING: ROM Setting on the board:%x conflicts with System file entry:%x\n",refbios,board_info->bios_addr);
				return 0;
			}
		}
       		board_info->aip_area = physmap((paddr_t)(board_info->bios_addr +
                           AIP_AREA_OFFSET), AIP_AREA_SIZE, KM_NOSLEEP);
       		if (!board_info->aip_area) {
			cmn_err(CE_WARN,"IBM TOKEN RING: Unable to allocate memory for the driver\n");
         		return -1;
		}

       		/* Make sure that it is an IBM card. */
          	/* Only look at even positions and group 2 by 2. */

       		for (i = 0, j = 0, cur_pos = board_info->aip_area + CHANNEL_ID_OFFSET; i < CHANNEL_IDENTIFIER_LENGTH; i++, j += 4) 
          		check_string[i] = (((*(cur_pos + j)) & 0xf) << 4) +
                               ((*(cur_pos + j + 2)) & 0xf);
		if (bus_type & BUS_MCA) {
         		if (strncmp(check_string, IBM_MCA_CHANNEL_IDENTIFIER,
                      		CHANNEL_IDENTIFIER_LENGTH)) {
					cmn_err(CE_CONT,"IBM TOKEN RING: MCA signature identification failed\n");
					return 0;
			}
		} else {
         		if (strncmp(check_string, IBM_AT_CHANNEL_IDENTIFIER,
                      		CHANNEL_IDENTIFIER_LENGTH)) {
				cmn_err(CE_CONT,"IBM TOKEN RING: AT signature identification failed\n");
				return 0;
			}
		}

           	board_info->aca_area = physmap((paddr_t) (board_info->bios_addr + ACA_AREA_OFFSET) , ACA_AREA_SIZE,KM_NOSLEEP);
	
		if (board_info->aca_area == NULL) {
			cmn_err(CE_WARN,"IBM TOKEN RING: Unable to allocate memory for the driver\n");
         		return -1;
		}
		/* Disable all interrupts for now. The bit will be set just
		   before the reset is issued to the adapter
		*/
		*(board_info->aca_area + ISRP_EVEN_RESET) = (~(0x40)) & 0xff;
           	/* Find & save the interrupt vector. */
		if (bus_type & BUS_MCA) {
			/*Get the bit 0 of pos reg 4 */
			temp = pos_reg[2] & 0x1;
			ref_intr_patt = temp << 1;
			/*Get the bit 7 of pos reg 3 */
			temp = ((pos_reg[1] & 0x80) >> 7);
			ref_intr_patt |= temp;
			/* Convert the pattern to the appropriate intr */
			switch(ref_intr_patt) {
			case 0:
				ref_intr =  2;
				break;
			case 1:
				ref_intr  =  3;
				break;
			case 2:
				ref_intr =  10;
				break;
			case 3:
				ref_intr =  11;
				break;
			}
		} else {
			/* Need to get the lowest two bits */
			ref_intr_patt = hw_config & 0x3;
			switch(ref_intr_patt) {
			case 0:
				ref_intr = 2;
				break;
			case 1:
				ref_intr = 3;
				break;
			case 2:
				ref_intr = 6;
				break;
			case 3:
				ref_intr = 7;
				break;
			}
		}
           	/* Check if the user configured the sdevice correctly.*/
           	if ((board_info->tok_int != ref_intr) && (!((ref_intr == 2) && (board_info->tok_int == 9)))) {
			cmn_err(CE_CONT,"IBM TOKEN RING: INTERRUPT Setting on the board:%d  conflicts with System file entry:%d\n",ref_intr,board_info->tok_int);
             		return 0;
           	}
           	board_info->intr_refresh = 0x2F0 + ref_intr;
           	/* Save the encoded address (is nibbles). */
           	for (i = 0; i < MAC_ADDR_LEN; i++)
              		board_info->tok_macaddr[i] = ((*(board_info->aip_area + i * 4) & 0xf) << 4) + (*(board_info->aip_area + 2 + i * 4));

           	if (extract_aip_info(board_info) != 0)
             		return 0;
           	boundary_size  = board_info->aip.total_RAM < K16 ?
                                      K16 : board_info->aip.total_RAM; 
		if (bus_type & BUS_MCA) {
			cur_candidate = ((pos_reg[0] & RAM_MCA_MASK) << RAM_MCA_SHIFT);
			board_info->ram_addr = cur_candidate;
		} else 
           		cur_candidate = board_info->ram_addr;

		if ( (boundary = cur_candidate % boundary_size) ) {
			cmn_err(CE_CONT,"IBM TOKEN RING: Illegal RAM address:%x\n",board_info->ram_addr);
			return 0;
		}
           	board_info->tok_noboard = FALSE;
       } /* End of bios addr within range. */
       return 0;
}

static
find_slot(id)
unsigned id;
{
int j,k;
unsigned f_id;
unsigned char slot = 0,id_tmp;

	slot |= 0x8;
	for (j = 0; j < 8; j++) {
		outb(ADAP_ENAB,slot);
		f_id = (unsigned char)inb(POS_MSB);
		f_id = f_id << 8;
		id_tmp = inb(POS_LSB);
		f_id |= id_tmp;
		if (f_id == id) {
			if (tok_board_0_slot == -1)
				tok_board_0_slot = j;
			else if (tok_board_0_slot == j) {
				slot++;
				continue;
			}
			for (k = 0; k < 4; k++)
				pos_reg[k] = inb(POS_0 + k);
			outb(ADAP_ENAB,0);
			return (0);
		}
		slot++;
	}
	cmn_err(CE_CONT,"IBM TOKEN RING: The 16/4A adapter cannot be identified\n");
	outb(ADAP_ENAB,0);
	return (-1);
}

void
tokwatch_dog()
{
register int i;
struct tokparam *brd_ptr = tokparams;

	toktimer_id = timeout(tokwatch_dog,0,drv_usectohz(TOK_TIMEOUT));
	for (i = 0; i < tok_board_cnt; i++,brd_ptr++) {
		if (brd_ptr->tok_noboard == TRUE)
			continue;
		if (brd_ptr->tok_timer_val > 0)
			brd_ptr->tok_timer_val--;
		if (brd_ptr->tok_timer_val == 0) {
			switch(brd_ptr->tok_adapter_state) {
			case ADAPTER_RESETTING:
				cmn_err(CE_WARN,"IBM TOKEN RING: Adapter reset sequence timed out\n");
				brd_ptr->tok_timer_val = -1;
				brd_ptr->tok_adapter_state= ADAPTER_DISABLED;
				brd_ptr->tok_reset_flag = ADAPTER_RESET_TIMEOUT;
        			*(brd_ptr->aca_area + ISRP_EVEN_RESET) =
							 ((~(0x40)) & 0xff);
				wakeup(&(brd_ptr->tok_reset_flag));
				break;
			case ADAPTER_OPENING:
				cmn_err(CE_WARN,"IBM TOKEN RING: Adapter open command timed out\n");
				brd_ptr->tok_timer_val = -1;
				brd_ptr->tok_adapter_state = ADAPTER_CLOSED;
				brd_ptr->tok_open_flag |= ADAPTER_OPEN_TIMEOUT;
				wakeup(&(brd_ptr->tok_open_flag));
				break;
			default:
				brd_ptr->tok_timeouts++;
				if (brd_ptr->tok_timeouts == TOK_MAX_TIMEOUTS) {
					(void) tok_adapter_shutdown(brd_ptr);
					cmn_err(CE_WARN,"IBM TOKEN RING: WARNING BOARD NO: %d IS BEING DISABLED\n",brd_ptr->tok_index);
				}
			}
		}
	}
}

STATIC mblk_t * 
tok_test_snap_ind(info_ptr,prim_type,pkt_type)
struct	llc_info *info_ptr;
unsigned long prim_type;
ushort pkt_type;
{
register int i;
struct llca	*llcap;
union DL_primitives *dlp;
char		*r_ptr;
mblk_t		*nmp;
int rsize = 0,maxsize;


	if (pkt_type & DL_ROUTE) {
		if (info_ptr->rsize <= 2)
			return NULL;
		rsize = info_ptr->rsize;
	}

	maxsize =  (rsize == 0) ? 0:MAX_ROUTE_FLD;
	if ((nmp = allocb(sizeof(union DL_primitives)+maxsize+ 2*LLC_ENADDR_LEN,
				  BPRI_MED)) == NULL)
			return NULL;
	nmp->b_datap->db_type = M_PROTO;

	dlp = (union DL_primitives *)nmp->b_rptr;

	if (prim_type == DL_TEST_IND) {
		dlp->dl_primitive = DL_TEST_IND;
		llcap = (struct llca *)((caddr_t)dlp + DL_TEST_IND_SIZE);
		dlp->test_ind.dl_dest_addr_length = LLC_ENADDR_LEN;
		dlp->test_ind.dl_dest_addr_offset = DL_TEST_IND_SIZE;
		dlp->test_ind.dl_src_addr_length = LLC_ENADDR_LEN + rsize;
		dlp->test_ind.dl_src_addr_offset = DL_TEST_IND_SIZE +
                                                        LLC_ENADDR_LEN;
		dlp->test_ind.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_TEST_IND_SIZE + rsize +
				(2 * LLC_ENADDR_LEN);
	} else {
                dlp->dl_primitive = DL_TEST_CON;
		llcap = (struct llca *)((caddr_t)dlp + DL_TEST_CON_SIZE);
		dlp->test_con.dl_dest_addr_length = LLC_ENADDR_LEN;
		dlp->test_con.dl_dest_addr_offset = DL_TEST_CON_SIZE;
		dlp->test_con.dl_src_addr_length = LLC_ENADDR_LEN + rsize;
		dlp->test_con.dl_src_addr_offset = DL_TEST_CON_SIZE +
                                                        LLC_ENADDR_LEN;
		dlp->test_con.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_TEST_CON_SIZE + rsize +
				(2 * LLC_ENADDR_LEN);
	}
	bcopy(info_ptr->mac_ptr->mac_dst, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->dsap;
	llcap++;		
	bcopy(info_ptr->mac_ptr->mac_src, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->ssap;
	if (rsize) {
		llcap++;		
		r_ptr = (char *)llcap;
		for (i = 0; i < rsize; i++)
                        r_ptr[i] = info_ptr->mac_ptr->ibm_route[i];
	}
	return nmp;
}

STATIC mblk_t *
tok_test_sap_ind(info_ptr,prim_type,pkt_type)
struct	llc_info *info_ptr;
unsigned long prim_type;
unsigned short pkt_type;
{
register int i;
struct llcc	*llccp;
mblk_t		*nmp;
char		*r_ptr;
union DL_primitives *dlp;
int rsize = 0,maxsize;


	if (pkt_type & DL_ROUTE) {
		if (info_ptr->rsize <= 2)
			return NULL;
		rsize = info_ptr->rsize;
	}

	maxsize =  (rsize == 0) ? 0:MAX_ROUTE_FLD;
	if ((nmp = allocb(sizeof(union DL_primitives)+maxsize+ 2*LLC_LIADDR_LEN,
				  BPRI_MED)) == NULL)
			return NULL;
	nmp->b_datap->db_type = M_PROTO;
	dlp = (union DL_primitives *)nmp->b_rptr;

	if (prim_type == DL_TEST_IND) {
		dlp->dl_primitive = DL_TEST_IND;
		llccp = (struct llcc *)((caddr_t)dlp + DL_TEST_IND_SIZE);
		dlp->test_ind.dl_dest_addr_length = LLC_LIADDR_LEN;
		dlp->test_ind.dl_dest_addr_offset = DL_TEST_IND_SIZE;
		dlp->test_ind.dl_src_addr_length = LLC_LIADDR_LEN + rsize;
		dlp->test_ind.dl_src_addr_offset = DL_TEST_IND_SIZE +
							LLC_LIADDR_LEN;
		dlp->test_ind.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_TEST_IND_SIZE + rsize +
				(2 * LLC_LIADDR_LEN);
	} else {
		dlp->dl_primitive = DL_TEST_CON;
		llccp = (struct llcc *)((caddr_t)dlp + DL_TEST_CON_SIZE);
		dlp->test_con.dl_dest_addr_length = LLC_LIADDR_LEN;
		dlp->test_con.dl_dest_addr_offset = DL_TEST_CON_SIZE;
		dlp->test_con.dl_src_addr_length = LLC_LIADDR_LEN + rsize;
		dlp->test_con.dl_src_addr_offset = DL_TEST_CON_SIZE +
							LLC_LIADDR_LEN;
		dlp->test_con.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_TEST_CON_SIZE + rsize +
				(2 * LLC_LIADDR_LEN);
	}
	bcopy(info_ptr->mac_ptr->mac_dst,llccp->lbf_addr,MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->dsap;
	llccp++;
	bcopy(info_ptr->mac_ptr->mac_src,llccp->lbf_addr,MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->ssap;
	if (rsize) {
		llccp++;
		r_ptr = (char *)llccp;
		for (i = 0; i < rsize; i++)
			r_ptr[i] = info_ptr->mac_ptr->ibm_route[i];
	}
	return nmp;
}

STATIC int 
do_test_msg(pkt_type,info_ptr,tok,q,mp)
ushort	pkt_type;	/* Type of packet DL_802, DL_SNAP, DL_ROUTE */
struct	llc_info *info_ptr;
struct tokdev *tok;
queue_t *q;
mblk_t *mp;
{
mblk_t *nmp;
struct mac_llc_hdr *newhdr;
unsigned long prim_type;

	if (info_ptr->dsap != LLC_NULL_SAP) {
		prim_type = (pkt_type & DL_RESPONSE) ? DL_TEST_CON:DL_TEST_IND; 
		if (pkt_type & DL_SNAP)
			nmp = tok_test_snap_ind(info_ptr,prim_type,pkt_type);
		else 
			nmp = tok_test_sap_ind(info_ptr,prim_type,pkt_type);
		if (nmp == NULL)
			return E_NOBUFFER;
		mp->b_rptr = info_ptr->data_ptr;
		if (mp->b_rptr == mp->b_wptr) {
			mblk_t *nullblk;
			nullblk = mp;
			mp = unlinkb(nullblk);
			freeb(nullblk);
		}
		linkb(nmp,mp);
		putnext(q,nmp);
		return E_OK;
	} else
		return E_INVALID;	
}

STATIC mblk_t *
tok_xid_sap_hdr(mp,tok,src_length,msg_type,adap_cmd)
mblk_t *mp;
struct tokdev *tok;
int src_length;
unsigned char msg_type,adap_cmd;
{
register struct mac_llc_hdr *hdr;
register int offset;
union DL_primitives *dlp;
struct llcc *llcp;
mblk_t *nmp;
unsigned char *d_ptr = mp->b_rptr,*route_ptr;
unsigned char dsap;
srb_cmd_t *psrb;
int i,r_cnt = 0;
ushort maxsize = 0;
struct llctype *sap_hdr;

	/* make a valid header for transmit */

	dlp = (union DL_primitives *) d_ptr;

	if (src_length > LLC_LIADDR_LEN) {
		r_cnt = dlp->xid_req.dl_dest_addr_length - sizeof(struct llcc);
		if ((r_cnt & 0x01) || (r_cnt < 2) || (r_cnt > 18)) {
			/* Odd source routing field, or bad length */
			return (NULL);
		}
		maxsize = MAX_ROUTE_FLD;
	}

	if ((nmp = allocb((LLC_HDR_SIZE + LLC_XID_INFO_SIZE + maxsize), 
							    BPRI_MED)) == NULL)
		return NULL;
	hdr = (struct mac_llc_hdr *)nmp->b_rptr;

	llcp = (struct llcc *)(d_ptr + dlp->xid_req.dl_dest_addr_offset);
	bcopy((caddr_t)llcp->lbf_addr, hdr->mac_dst, MAC_ADDR_LEN);
	bcopy(tok->tok_macpar->tok_macaddr, hdr->mac_src, MAC_ADDR_LEN);
	dsap = llcp->lbf_sap;

	if (r_cnt) {
		hdr->mac_src[0] |= 0x80;
		route_ptr = (unsigned char *) (d_ptr + 
			dlp->test_req.dl_dest_addr_offset+ sizeof(struct llcc));
		for (i = 0; i < r_cnt ; i++)
			hdr->llc.ibm_route[i] = route_ptr[i];
	}

	sap_hdr = (struct llctype *)((char *)hdr + (2 * MAC_ADDR_LEN) + r_cnt);

	sap_hdr->llc_dsap = dsap;
	sap_hdr->llc_ssap =  tok->tok_sap |((adap_cmd == SRB_TX_XID_CMD) ? 1:0);
	sap_hdr->llc_control = msg_type;
	sap_hdr->llc_info[0] = LLC_XID_FMTID;
	sap_hdr->llc_info[1] = LLC_SERVICES;
	sap_hdr->llc_info[2] = tok->tok_rws;

	nmp->b_wptr = nmp->b_rptr + LLC_HDR_SIZE + LLC_XID_INFO_SIZE + r_cnt;

	mp->b_datap->db_type = M_DATA;
	psrb = (srb_cmd_t *)mp->b_rptr;
	psrb->srb_cmd = adap_cmd;
	psrb->srb_cmd_param.xmit_frame.dsap = dsap;
	psrb->srb_cmd_param.xmit_frame.lan_hdr_size = MIN_LAN_HDR_SIZE + r_cnt;
	psrb->srb_cmd_param.xmit_frame.station_id = tok->tok_station_id;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	return nmp;
}

STATIC mblk_t *
tok_xid_snap_hdr(mp,tok,src_length,msg_type,adap_cmd)
mblk_t *mp;
struct tokdev *tok;
int src_length;
unsigned char msg_type,adap_cmd;
{
register struct mac_llc_hdr *hdr;
register int offset;
union DL_primitives *dlp;
struct llca *llcp;
mblk_t *nmp;
unsigned char *d_ptr = mp->b_rptr,*route_ptr;
unsigned char dsap;
srb_cmd_t *psrb;
int i,r_cnt = 0;
ushort maxsize = 0;
struct llc_snap *snap_hdr;

	/* make a valid header for transmit */

	dlp = (union DL_primitives *) d_ptr;
	llcp = (struct llca *)(d_ptr + dlp->xid_req.dl_dest_addr_offset);
	dsap = llcp->lbf_sap;

	if  ( (dsap != LLC_NULL_SAP) && (dsap != LLC_GLOBAL_SAP) )
		dsap = LLC_SNAP_SAP;

	if (src_length > LLC_ENADDR_LEN) {
		r_cnt = dlp->xid_req.dl_dest_addr_length - sizeof(struct llca);
		if ((r_cnt & 0x01) || (r_cnt < 2) || (r_cnt > 18)) {
			/* Odd source routing field, or bad length */
			return (NULL);
		}
		maxsize = MAX_ROUTE_FLD;
	}

	if ((nmp = allocb((LLC_EHDR_SIZE + LLC_XID_INFO_SIZE + MAX_ROUTE_FLD), 
						          BPRI_MED)) == NULL) {
		tok->tok_stats->toks_nobuffer++;
		return NULL;
	}
	hdr = (struct mac_llc_hdr *)nmp->b_rptr;

	bcopy((caddr_t)llcp->lbf_addr, hdr->mac_dst, MAC_ADDR_LEN);
	bcopy(tok->tok_macpar->tok_macaddr, hdr->mac_src, MAC_ADDR_LEN);

	if (r_cnt) {
		hdr->mac_src[0] |= 0x80;
		route_ptr = (unsigned char *) (d_ptr + 
			dlp->test_req.dl_dest_addr_offset+ sizeof(struct llca));
		for (i = 0; i < r_cnt ; i++)
			hdr->llc.ibm_route[i] = route_ptr[i];
	}
	snap_hdr =(struct llc_snap*)((char *)hdr + (2 * MAC_ADDR_LEN) + r_cnt);

	snap_hdr->llc_dsap = dsap;
	snap_hdr->llc_ssap = LLC_SNAP_SAP;
	snap_hdr->llc_control = msg_type;
	snap_hdr->llc_info[0] = LLC_XID_FMTID;
	snap_hdr->llc_info[1] = LLC_SERVICES;
	snap_hdr->llc_info[2] = tok->tok_rws;
	snap_hdr->org_code[0] = 0;
	snap_hdr->org_code[1] = 0;
	snap_hdr->org_code[2] = 0;
	snap_hdr->ether_type = ntohs(tok->tok_sap);

	nmp->b_wptr = nmp->b_rptr + LLC_EHDR_SIZE + LLC_XID_INFO_SIZE + r_cnt;

	mp->b_datap->db_type = M_DATA;
	psrb = (srb_cmd_t *)mp->b_rptr;
	psrb->srb_cmd = adap_cmd;
	psrb->srb_cmd_param.xmit_frame.dsap = dsap;
	psrb->srb_cmd_param.xmit_frame.lan_hdr_size = MIN_LAN_HDR_SIZE + r_cnt;
	psrb->srb_cmd_param.xmit_frame.station_id = tok->tok_station_id;
	mp->b_wptr = mp->b_rptr + sizeof(srb_cmd_t);
	return nmp;
}

/* Processes all out bound XID messages. This routine in turn will call
   either tok_xid_sap_hdr or tok_xid_snap_hdr to format and and return
   an XID pdu
*/ 

STATIC int
tokxiddata(q,mp)
queue_t *q;
mblk_t *mp;
{
struct tokdev *tok;
mblk_t *nmp, *tmp;
union DL_primitives *dlp;
register struct mac_llc_hdr *hdr;
short	src_length,length;
struct tokparam *tokp;
int old;
unsigned char adap_cmd;
unsigned char msg_type; 

	tok = (struct tokdev *)q->q_ptr;
	tokp = tok->tok_macpar;
	if (tok->tok_state != DL_IDLE)
		return DL_OUTSTATE;

	dlp = (union DL_primitives *)mp->b_rptr;

	msg_type = LLC_XID;
	if ((int)dlp->dl_primitive == DL_XID_REQ) {
		adap_cmd = SRB_TX_XID_CMD;
		src_length = dlp->xid_req.dl_dest_addr_length;
		if (dlp->xid_req.dl_flag)
			msg_type |= LLC_P;
	} else {
		src_length = dlp->xid_res.dl_dest_addr_length;
		if (dlp->xid_res.dl_flag) {
			adap_cmd = SRB_TX_XID_RESP_FINAL;
			msg_type |= LLC_P;
		} else 
			adap_cmd = SRB_TX_XID_RESP;
	}
	length = 0;
	tmp = mp->b_cont;

	while (tmp != NULL) {
		length +=  (tmp->b_wptr - tmp->b_rptr);
		tmp = tmp->b_cont;
	}

	/* Check the data length for maximum */
	
	if (length > tok->tok_macpar->tok_maxpkt)
		return DL_BADDATA;

	tmp = unlinkb(mp);
	mp->b_cont = NULL;

	if (tok->tok_type == DL_802) {
		if (src_length <= LLC_LIR_MAX_LEN)
			nmp = tok_xid_sap_hdr(mp,tok,src_length,msg_type,adap_cmd);
		else {
			freemsg(tmp);
			return DL_BADADDR;
		}
	} else {
		if (src_length <= LLC_ENR_MAX_LEN)
			nmp = tok_xid_snap_hdr(mp,tok,src_length,msg_type,adap_cmd);
		else {
			freemsg(tmp);
			return DL_BADADDR;
		}
	}

	if (nmp == NULL) {
		freemsg(tmp);
		return DL_SYSERR;
	}
	linkb(nmp,tmp);

	/* Statistics */
	tok->tok_dstats->toks_xpkts++;
	tok->tok_dstats->toks_xbytes += msgdsize(nmp);

	if (toklocal(nmp,tok)) {
		freeb(mp);
		tokloop(nmp,tok);
		return E_OK;
	}
	if (tokmulticast(nmp,tok))
		tokloop(dupmsg(nmp),tok);

	linkb(mp,nmp);

	old = splstr();

	if ( (tokp->sched_flags & TX_BUSY) || (tokp->cur_msg != NULL)) {
		tokp->sched_flags |= TX_QUEUED;
		putq(q,mp);
	} else {
		tok_setup_send(tok, mp);
	}
	splx(old);
	return E_OK;
}


STATIC mblk_t *
tok_xid_sap_ind(info_ptr,prim_type,pkt_type)
struct	llc_info *info_ptr;
unsigned long prim_type;
unsigned short pkt_type;
{
register int i;
struct llcc	*llccp;
mblk_t		*nmp;
char		*r_ptr;
union DL_primitives *dlp;
int rsize = 0,maxsize;


	if (pkt_type & DL_ROUTE) {
		if (info_ptr->rsize <= 2)
			return NULL;
		rsize = info_ptr->rsize;
	}

	maxsize =  (rsize == 0) ? 0:MAX_ROUTE_FLD;
	if ((nmp = allocb(sizeof(union DL_primitives)+maxsize+ 2*LLC_LIADDR_LEN,
				  BPRI_MED)) == NULL)
			return NULL;
	nmp->b_datap->db_type = M_PROTO;
	dlp = (union DL_primitives *)nmp->b_rptr;

	if (prim_type == DL_XID_IND) {
		dlp->dl_primitive = DL_XID_IND;
		llccp = (struct llcc *)((caddr_t)dlp + DL_XID_IND_SIZE);
		dlp->xid_ind.dl_dest_addr_length = LLC_LIADDR_LEN;
		dlp->xid_ind.dl_dest_addr_offset = DL_XID_IND_SIZE;
		dlp->xid_ind.dl_src_addr_length = LLC_LIADDR_LEN + rsize;
		dlp->xid_ind.dl_src_addr_offset = DL_XID_IND_SIZE +
							LLC_LIADDR_LEN;
		dlp->xid_ind.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_XID_IND_SIZE + rsize +
				(2 * LLC_LIADDR_LEN);
	} else {
		dlp->dl_primitive = DL_XID_CON;
		llccp = (struct llcc *)((caddr_t)dlp + DL_XID_CON_SIZE);
		dlp->xid_con.dl_dest_addr_length = LLC_LIADDR_LEN;
		dlp->xid_con.dl_dest_addr_offset = DL_XID_CON_SIZE;
		dlp->xid_con.dl_src_addr_length = LLC_LIADDR_LEN + rsize;
		dlp->xid_con.dl_src_addr_offset = DL_XID_CON_SIZE +
							LLC_LIADDR_LEN;
		dlp->xid_con.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_XID_CON_SIZE + rsize +
				(2 * LLC_LIADDR_LEN);
	}
	bcopy(info_ptr->mac_ptr->mac_dst,llccp->lbf_addr,MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->dsap;
	llccp++;
	bcopy(info_ptr->mac_ptr->mac_src,llccp->lbf_addr,MAC_ADDR_LEN);
	llccp->lbf_sap = info_ptr->ssap;
	if (rsize) {
		llccp++;
		r_ptr = (char *)llccp;
		for (i = 0; i < rsize; i++)
			r_ptr[i] = info_ptr->mac_ptr->ibm_route[i];
	}
	return nmp;
}

STATIC mblk_t * 
tok_xid_snap_ind(info_ptr,prim_type,pkt_type)
struct	llc_info *info_ptr;
unsigned long prim_type;
ushort pkt_type;
{
register int i;
struct llca	*llcap;
union DL_primitives *dlp;
char		*r_ptr;
mblk_t		*nmp;
int rsize = 0,maxsize;


	if (pkt_type & DL_ROUTE) {
		if (info_ptr->rsize <= 2)
			return NULL;
		rsize = info_ptr->rsize;
	}

	maxsize =  (rsize == 0) ? 0:MAX_ROUTE_FLD;
	if ((nmp = allocb(sizeof(union DL_primitives)+maxsize+ 2*LLC_ENADDR_LEN,
				  BPRI_MED)) == NULL)
			return NULL;
	nmp->b_datap->db_type = M_PROTO;

	dlp = (union DL_primitives *)nmp->b_rptr;

	if (prim_type == DL_XID_IND) {
		dlp->dl_primitive = DL_XID_IND;
		llcap = (struct llca *)((caddr_t)dlp + DL_XID_IND_SIZE);
		dlp->test_ind.dl_dest_addr_length = LLC_ENADDR_LEN;
		dlp->test_ind.dl_dest_addr_offset = DL_XID_IND_SIZE;
		dlp->test_ind.dl_src_addr_length = LLC_ENADDR_LEN + rsize;
		dlp->test_ind.dl_src_addr_offset = DL_XID_IND_SIZE +
                                                        LLC_ENADDR_LEN;
		dlp->test_ind.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_XID_IND_SIZE + rsize +
				(2 * LLC_ENADDR_LEN);
	} else {
                dlp->dl_primitive = DL_XID_CON;
		llcap = (struct llca *)((caddr_t)dlp + DL_XID_CON_SIZE);
		dlp->test_con.dl_dest_addr_length = LLC_ENADDR_LEN;
		dlp->test_con.dl_dest_addr_offset = DL_XID_CON_SIZE;
		dlp->test_con.dl_src_addr_length = LLC_ENADDR_LEN + rsize;
		dlp->test_con.dl_src_addr_offset = DL_XID_CON_SIZE +
                                                        LLC_ENADDR_LEN;
		dlp->test_con.dl_flag = (info_ptr->control & LLC_P)? 1 : 0;
		nmp->b_wptr = nmp->b_rptr + DL_XID_CON_SIZE + rsize +
				(2 * LLC_ENADDR_LEN);
	}
	bcopy(info_ptr->mac_ptr->mac_dst, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->dsap;
	llcap++;		
	bcopy(info_ptr->mac_ptr->mac_src, llcap->lbf_addr, MAC_ADDR_LEN);
	llcap->lbf_sap = info_ptr->ssap;
	if (rsize) {
		llcap++;		
		r_ptr = (char *)llcap;
		for (i = 0; i < rsize; i++)
                        r_ptr[i] = info_ptr->mac_ptr->ibm_route[i];
	}
	return nmp;
}

STATIC int 
do_xid_msg(pkt_type,info_ptr,tok,q,mp)
ushort	pkt_type;	/* Type of packet DL_802, DL_SNAP, DL_ROUTE */
struct	llc_info *info_ptr;
struct tokdev *tok;
queue_t *q;
mblk_t *mp;
{
mblk_t *nmp;
struct mac_llc_hdr *newhdr;
unsigned long prim_type;
register ushort i;

dl_xid_con_t *xidp;
struct llca *llcap;
struct llcc *llccp;
unsigned char *route_ptr;
unsigned int offset = 0;

	if (info_ptr->dsap != LLC_NULL_SAP) {
		prim_type = (pkt_type & DL_RESPONSE) ? DL_XID_CON:DL_XID_IND; 
		if (pkt_type & DL_SNAP)
			nmp = tok_xid_snap_ind(info_ptr,prim_type,pkt_type);
		else 
			nmp = tok_xid_sap_ind(info_ptr,prim_type,pkt_type);
		if (nmp == NULL)
			return E_NOBUFFER;
		mp->b_rptr = info_ptr->data_ptr + LLC_XID_INFO_SIZE;
		if (mp->b_rptr == mp->b_wptr) {
			mblk_t *nullblk;
			nullblk = mp;
			mp = unlinkb(nullblk);
			freeb(nullblk);
		}
		linkb(nmp,mp);
		putnext(q,nmp);
		return (E_OK);
	} else
		return E_INVALID;
}

void
tok_proc_adapter_error(tokp)
struct tokparam *tokp;
{
unsigned char isrp_odd,isrp_even,isra_even,isra_odd;
unsigned offset = 0;
ushort reason_code; 

	cmn_err(CE_CONT,"IBM TOKEN RING:** Fatal adapter error **\n");

  	offset = ((*(tokp->aca_area + WWCR_EVEN_OFFSET)) & 0xff) << 8;
  	offset += ((*(tokp->aca_area + WWCR_ODD_OFFSET)) & 0xff);

	isrp_odd = *(tokp->aca_area + ISRP_ODD_OFFSET);
	isrp_even = *(tokp->aca_area + ISRP_EVEN_OFFSET);
	isra_odd = *(tokp->aca_area + ISRA_ODD_OFFSET);
	isra_even = *(tokp->aca_area + ISRA_EVEN_OFFSET);
	if (isrp_odd & INTR_ADAPTER_CHECK) {
		if (isrp_even & INTR_ERROR_INTR) {
			/* Must be a machine check */
			tokwaitloop(25000);
		}
		reason_code = ntohs((*((ushort *) (tokp->ram_area + offset))));
		/* If the reason code is not set set it to Adapter Inoperative*/
		if (!reason_code)
			reason_code = 0x8000;
		cmn_err(CE_CONT,"Adapter error reason code:%x\n",reason_code);
	}
	/* Disable all upper layer protocols */
	(void) tok_adapter_shutdown(tokp);
	return;
}

void 
tok_adapter_shutdown(tokp)
struct tokparam *tokp;
{
struct tokdev *tok;
register int j;
mblk_t *mp;

	cmn_err(CE_CONT,"IBM TOKEN RING: Adapter performing an abortive shutdown\n");
	(void)tok_proc_close(tokp);
        *(tokp->aca_area + ISRP_EVEN_RESET) = ((~(0x40)) & 0xff);
	tok = &tokdevs[tokp->tok_firstd];
	
	for (j =0; j < tokp->tok_minors; j++,tok++) {
		if (tok->tok_state == DL_UNBOUND)
			continue;
		tok->tok_state = DL_UNBOUND;
		if ( (mp = allocb(sizeof(ulong),BPRI_HI)) == NULL)
			break;
		mp->b_datap->db_type = M_ERROR;
		mp->b_rptr = mp->b_datap->db_base;
		mp->b_wptr = mp->b_rptr + sizeof(char);
		*mp->b_rptr = ENXIO;
		flushq(tok->tok_qptr,FLUSHALL);
		flushq(OTHERQ(tok->tok_qptr), FLUSHALL);
		putnext((RD(tok->tok_qptr)),mp);
	}
	tokp->tok_reset_retries = 0;
	tokp->tok_adapter_state = ADAPTER_DISABLED;
	tokp->tok_timer_val = 10;
	(void)tokreset(tokp);
}

void
tok_proc_ssb(tokp)
struct tokparam *tokp;
{
transmit_ssb_t *pssb = (transmit_ssb_t *)(tokp->SSB_area);
unsigned char frame_status, addr_recog, retcode, transmit_error, opcode;
ushort station_id;

	if(pssb->retcode != SSB_SUCCESS) {
		station_id = ntohs(pssb->station_id);
		if (pssb->retcode == TRANSMIT_FRAME_ERROR) {
			frame_status = pssb->transmit_error;
			if (tokdebug && (frame_status & ADDR_RECOG_NOT_COPIED))
				cmn_err(CE_CONT,"IBM TOKEN RING, destination adapter could not copy a frame..loss of packet\n");
		}
	}
	*(tokp->aca_area + ISRA_ODD_SET) = 0x1;
}
