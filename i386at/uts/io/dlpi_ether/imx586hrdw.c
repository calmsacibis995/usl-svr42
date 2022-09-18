/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/dlpi_ether/imx586hrdw.c	1.12"
#ident	"$Header: $"
/*
**	vi:set ts=4 sw=4:
*/

/*	Copyright (c) 1987, 1988, 1989 Intel Corp.		*/
/*	  All Rights Reserved  	*/
/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 *	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 */

#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/dlpi_imx586.h>
#include <io/dlpi_ether/imx586.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/tcpip/if.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <svc/systm.h>

#ifndef lint
#include <net/tcpip/byteorder.h>
#endif
#include <proc/cred.h>
#include <io/ddi.h>

/*
 * Header, wrapper, and function declarations and definitions
 * for loadability support...
 */

#include <util/mod/moddefs.h>

#define	DRVNAME	"imx586 - Loadable imx586 Ethernet driver"

STATIC	int	imx586_load(), imx586_unload();
void		imx586init();
STATIC 	void	imx586uninit();

MOD_DRV_WRAPPER(imx586, imx586_load, imx586_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

STATIC int
imx586_load(void)
{
	cmn_err(CE_NOTE, "!MOD: in imx586_load()");

	imx586init();
	mod_drvattach(&imx586_attach_info);
	return(0);
}

STATIC int
imx586_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in imx586_unload()");

	mod_drvdetach(&imx586_attach_info);
	imx586uninit();
	return(0);
}

#define BCOPY(from, to, len) bcopy((caddr_t)(from),(caddr_t)(to),(size_t)(len))
#define BCMP(s1, s2, len) bcmp((char*)(s1),(char*)(s2),(size_t)(len))

void		imx586init();
STATIC 	void	imx586uninit();

int	imx586timer_id;

extern	struct	ifstats	*ifstats;	/* per-interface statistics for inet */	
extern	char	*imx586_ifname;		/* name of interface	 */
unsigned char imx586broadaddr[LLC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

extern	DL_sap_t	DLsaps[];
extern	DL_bdconfig_t	DLconfig[];
extern	int		DLboards;
extern	int		DLinetstats;	/* inet statistics on/off flag */

extern	unsigned char	imx586default_add[];
extern	int		imx586_0_mem_start, imx586_0_static_ram,
			imx586_0_irq_level, imx586_0_major,
			imx586_1_mem_start, imx586_1_static_ram,
			imx586_1_irq_level, imx586_1_major,
			imx586_2_mem_start, imx586_2_static_ram,
			imx586_2_irq_level, imx586_2_major,
			imx586_3_mem_start, imx586_3_static_ram,
			imx586_3_irq_level, imx586_3_major;

extern	int	DLstrlog;
struct	ifstats	*imx586stat;	/* statistics for imx586 interface */
STATIC	int	imx586_reset(), imx586rcv_packet(), imx586put_packet(), imx586ru_start();
int	imx586xmit_packet();

STATIC	void	imx586_watchdog(), imx586re_q_fd(), imx586build_cu(), imx586build_ru();

void	imx586intr(), imx586init();

static void	write_word(), write_byte(), chan_attn();
static ushort	read_word();
static int	prom_address(), wait_scb();

ushort	virt_to_imx586();
int	diagnose_586(), config_586();
void	int586_on(), int586_off();
char *	imx586_to_virt();
int	imx586bcopy();

void	imx586bdspecioctl(), imx586bdspecclose();

int imx586interrupt[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

char	imx586copyright[] = "Copyright 1987, 1988, 1989 Intel Corporation";
char	imx586id_string[] = "IMX586 v2.10";

void
imx586init()
{
int i, x, z, zero_address, ff_ff_ff_address;
register int board;
char *p_board;
ushort y;
DL_bdconfig_t *bd;
DL_sap_t *p_sap;
bdd_t *bdd;

	/* 
	 * first write then read RAM to see if
	 * half-card is really in AT backplane
	 */
#ifdef	TYPEISINTHEMSG
	cmn_err(CE_CONT,
"IMX586 v2.9x Copyright (c) 1987, 1988, 1989, 1990 Intel Corp.,\n"); 
	cmn_err(CE_CONT, "All Rights Reserved\n");

#else
	cmn_err(CE_CONT,
"IMX586 v2.9 Copyright (c) 1987, 1988, 1989, 1990 Intel Corp.,\n"); 
	cmn_err(CE_CONT, "All Rights Reserved\n");

#endif
	if (imx586inetstats)
		imx586stat = (struct ifstats *)kmem_zalloc((sizeof(struct ifstats) * imx586boards), KM_NOSLEEP);
	for (x = 0, bd=imx586config; x < imx586boards; x++,bd++) {
		if (x == 0) {
			bd->irq_level       = imx586_0_irq_level;
			bd->mem_start       = (paddr_t)imx586_0_mem_start;
			bd->major           = imx586_0_major;

			if (imx586_0_irq_level < 16 && imx586_0_irq_level > 0)
				imx586interrupt[imx586_0_irq_level] = 0;
		}

		if (x == 1) {
			bd->irq_level       = imx586_1_irq_level;
			bd->mem_start       = (paddr_t )imx586_1_mem_start;
			bd->major           = imx586_1_major;

			if (imx586_1_irq_level < 16 && imx586_1_irq_level > 0)
				imx586interrupt[imx586_1_irq_level] = 1;
		}

		if (x == 2) {
			bd->irq_level       = imx586_2_irq_level;
			bd->mem_start       = (paddr_t )imx586_2_mem_start;
			bd->major           = imx586_2_major;

			if (imx586_2_irq_level < 16 && imx586_2_irq_level > 0)
				imx586interrupt[imx586_2_irq_level] = 2;
		}

		if (x == 3) {
			bd->irq_level       = imx586_3_irq_level;
			bd->mem_start       = (paddr_t)imx586_3_mem_start;
			bd->major           = imx586_3_major;

			if (imx586_3_irq_level < 16 && imx586_3_irq_level > 0)
				imx586interrupt[imx586_3_irq_level] = 3;
		}

		/*
		 * Initialize DLconfig...
		 */
		bzero((caddr_t)&bd->mib,sizeof(DL_mib_t));	
		bd->mib.ifOperStatus = DL_DOWN;
		bd->mib.ifAdminStatus = DL_DOWN;
		bd->io_start	= 0; /* not used in imx586 */
		bd->io_end	= 0; /* not used in imx586 */
		bd->mem_end	= 0; /* not used in imx586 */
		bd->max_saps	= 32;
		bd->bd_number	= x;
		bd->flags	= BOARD_DISABLED;
		bd->sap_ptr	= &imx586saps[ x * bd->max_saps ];
		bd->tx_next	= 0;
		bd->timer_val	= -1;
		bd->promisc_cnt	= 0; /* not used in imx586 */
		bd->multicast_cnt	= 0; /* not used in imx586 */
		if ( (bd->bd_dependent1	= (char *)kmem_zalloc(sizeof (bdd_t), KM_SLEEP)) == NULL) {
			cmn_err(CE_CONT,"Unable to allocate dependent strcutres\n");
			continue;
		}
		bd->bd_dependent2	= 0; /* not used in imx586 */
		bd->bd_dependent3	= 0; /* not used in imx586 */
		bd->bd_dependent4	= 0; /* not used in imx586 */
		bd->bd_dependent5	= 0; /* not used in imx586 */

		/*
		 * Initialize p_static_ram field of bdd structure. This was once
		 * done in the if statements at the beginning of this fcn.
		 */
		
		bdd = (bdd_t *)bd->bd_dependent1;
		bdd->cnf_fifo_byte = 0x080c;
		bdd->cnf_add_mode  = 0x2600;
		bdd->cnf_pri_data  = 0x6000;
		bdd->cnf_slot      = 0xf200;
		bdd->cnf_hrdwr     = 0x0000;
		bdd->cnf_min_len   = 0x0040;

		if (x == 0 )
			bdd->p_static_ram = (char *)imx586_0_static_ram;
		if (x == 1 )
			bdd->p_static_ram = (char *)imx586_1_static_ram;
		if (x == 2 )
			bdd->p_static_ram = (char *)imx586_2_static_ram;
		if (x == 3 )
			bdd->p_static_ram = (char *)imx586_3_static_ram;

		/* 
		 * see if board is configed in per /etc/conf/pack.d/imx586/space.c
		 */
		if ((int)bd->mem_start == 0xffffffff)
			continue;

		/*
		 * setup virtual pointers for command and static RAM areas disabling
		 * i486 page cache.
		 */

#define	PG_PCD	0x00000010	/* should be in immu.h */

		p_board = (char *)bd->mem_start;
		bdd->p_virt_cmd_prom =
				(char *)sptalloc(btoc(0x8000), PG_V | PG_PCD, btoc(p_board), 0);

		p_board = (char *)bdd->p_static_ram;
		bdd->p_virt_static_ram =
				(char *)sptalloc(btoc(0x8000), PG_V | PG_PCD, btoc(p_board), 0);

		/*
		 * reset to insure board is in 8 bit mode (for reading PROM)
		 */
		p_board  = bdd->p_virt_cmd_prom;
		p_board += OFFSET_RESET;

		write_byte(p_board, 1);
		drv_usecwait(10);	/* 10 usec delay (must be > 4 clock cycles of 586)*/
		write_byte(p_board, 0);

		p_board  = bdd->p_virt_static_ram;
		p_board += OFFSET_SCB;
		write_word((ushort*)p_board, 0x5a5a);
		y = read_word((ushort*)p_board);

		if ( y != 0x5a5a ) {
			cmn_err(CE_CONT, "IMX586 board %d is missing\n", x);
			continue;
		} else {
			bd->mib.ifAdminStatus = DL_UP;
			/*
			 * prom_address index should increment by one, however
			 * the imx586 board is stuck in word mode, thus ++ by 2
			 */
			for (i = 0; i < MULTI_ADDR_CNT; i++)
				bdd->imx586_multiaddr[i].status = 0;
			for (i = 0; i < CSMA_LEN; i++)
				bd->eaddr.bytes[i] = prom_address(x, (i * 2));

			zero_address     = 0;
			ff_ff_ff_address = 0;

			for (z = 0; z < DL_MAC_ADDR_LEN; z++) {
				if (bd->eaddr.bytes[z] == 0)
					zero_address++;
				if (bd->eaddr.bytes[z] == 0xff)
					ff_ff_ff_address++;
			}

			if ((zero_address     == DL_MAC_ADDR_LEN) ||
				(ff_ff_ff_address == DL_MAC_ADDR_LEN)) {
				cmn_err(CE_WARN, "invalid enet id, using address in /etc/conf/pack.d/imx586/space.c");

				/*
				 * Temporary patch for pre-release imx586 boards!!!!!
				 * Defined default_add[] in space.c
				 */
				for (z = 0; z < 6; z++)
					bd->eaddr.bytes[z]=imx586default_add[z];
			}

			cmn_err(CE_CONT, "IMX586 board %d was found. ", x);

			/*
			 * setup the ptype_t structures
			 */
			for (p_sap = bd->sap_ptr, z = 0; z < bd->max_saps; p_sap++, z++) {
				p_sap->state		= DL_UNBOUND;
				p_sap->mac_type	    	= DL_ETHER;
				p_sap->sap_addr   	= 0;
				p_sap->read_q	    	= NULL;
				p_sap->write_q    	= NULL;
				p_sap->flags	    	= 0;
				p_sap->max_spdu	    	= DL_MAX_PACKET;
				p_sap->min_spdu	    = USER_MIN_SIZE;
				p_sap->service_mode   = DL_CLDLS;
				p_sap->provider_style = DL_STYLE1;
				p_sap->bd		= bd;
				p_sap->next_sap = NULL;
			}
			if (imx586stat) {
				imx586stat[x].ifs_name = imx586_ifname;
				imx586stat[x].ifs_unit = (short)x;
				imx586stat[x].ifs_active = 1;
				imx586stat[x].ifs_mtu  = DL_MAX_PACKET;
				imx586stat[x].ifs_next = ifstats;
				ifstats = &imx586stat[x];
			}
			bd->valid_sap = NULL;
        		bd->mib.ifAdminStatus = DL_UP;
        		bd->mib.ifOperStatus =  DL_UP;
			imx586print_eaddr(bd->eaddr.bytes);
		}
		(void)imx586_reset(x);
		bd->flags = BOARD_PRESENT;
		bd->mib.ifOperStatus = DL_UP;
	}
	imx586timer_id = timeout(imx586_watchdog, 0, HZ);
	return;
} /* end of imx586init() */

/* imx586bdspecclose is called from DLclose->dlpi_ether.c */
void
imx586bdspecclose(q)
queue_t *q;
{
	DL_sap_t *sap = (DL_sap_t *)q->q_ptr;
	int bd;

	if (imx586inetstats && imx586stat) {
		bd = imx586interrupt[sap->bd->irq_level];
		imx586stat[bd].ifs_active      = 0;
		imx586stat[bd].ifs_ipackets    = 0;
		imx586stat[bd].ifs_ierrors     = 0;
		imx586stat[bd].ifs_opackets    = 0;
		imx586stat[bd].ifs_oerrors     = 0;
		imx586stat[bd].ifs_collisions  = 0;
	}
	return;
} /* end of imx586bdspecclose() */

/* imx586bdspecioctl is called from DLioctl->dlpi_ether.c */

void
imx586bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
        struct iocblk *ioctl_req = (struct iocblk *)mp->b_rptr;
	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;

} /* end of imx586bdspecioctl */


imx586put_packet(board_number, p_fd)
int board_number;
fd_t *p_fd;
{
int x, y;
mblk_t *mp;
rbd_t *p_rbd;
ushort *p_buffer;
ushort bytes_in_msg;
int i;
ushort sap_id;
DL_sap_t *sap;
DL_bdconfig_t *bd = &imx586config[board_number];

	p_rbd = (rbd_t *)imx586_to_virt(board_number, p_fd->fd_rbd_ofst);

	if (p_rbd == NULL) {			/* Frame is too short */
		bd->mib.ifInErrors++;     	/* SNMP */
		return(FALSE);		
	}

	p_buffer = (ushort *)imx586_to_virt(board_number,p_rbd->rbd_buff);
	if (p_buffer == NULL) {
		int586_off(board_number);
		bd->timer_val = 0;
		return(FALSE);
	}
	sap_id = p_fd->fd_length;
	for (bytes_in_msg = 0; p_rbd;
			p_rbd = (rbd_t *)imx586_to_virt(board_number,p_rbd->rbd_nxt_ofst))
	{
		bytes_in_msg += (p_rbd->rbd_status & CS_RBD_CNT_MSK);
	}

	p_rbd = (rbd_t *)imx586_to_virt(board_number, p_fd->fd_rbd_ofst);

	/*
	 * pad for 4 byte boundry moves
	 */
	mp = allocb((MAC_HDR_LEN + bytes_in_msg + 4), BPRI_MED);

	/*
	 * if null throw the packet on floor - its a connectionless service
	 */
	if (mp == NULL) {
		bd->mib.ifInDiscards++;                /* SNMP */
                bd->mib.ifSpecific.etherRcvResources++;/* SNMP */
		return(FALSE);
	}

	/*
	 * FIRST>>>FIRST>>>FIRST>>> copy destination address to msg block
	 */
	/* This is done in two steps because the copy routine works only
	   on 4 byte boundaries and we need the len/type field in the
	   DLrecv routine.
	*/

	imx586bcopy(p_fd->fd_dest, mp->b_wptr, (2 * DL_MAC_ADDR_LEN));
	mp->b_wptr +=  (2 * DL_MAC_ADDR_LEN);
	bcopy((caddr_t)&sap_id,(caddr_t)mp->b_wptr,sizeof(ushort));
	mp->b_wptr += sizeof(ushort);

	/*
	 * SECOND>>>SECOND>>>SECOND>>> copy rcv data to msg block
	 */
	bytes_in_msg = p_rbd->rbd_status & CS_RBD_CNT_MSK;

	/* all rcv buffers except the last one will have RCVBUFSIZE
	 * bytes of user data. RCVBUFSIZE has been set to a multiple of[
	 * 4, thus adding 3 will NOT cause imx586bcopy to copy any extra
	 * bytes. If the very last rcv buffer holds an uneven multiple
	 * of four bytes, then extra bytes will be copied (but ignored)
	 * Note that code above pads 4 extra bytes on the end of mp
	 */
	do {
		imx586bcopy(p_buffer, mp->b_wptr, (bytes_in_msg + 3));
		mp->b_wptr += bytes_in_msg;

		if (((p_rbd->rbd_status & CS_EOF) == CS_EOF) || 
		    ((p_rbd->rbd_size   & CS_EL)  == CS_EL)) 
			break;

		p_rbd = (rbd_t *)imx586_to_virt(board_number, p_rbd->rbd_nxt_ofst);

		if (p_rbd) {
			p_buffer = (ushort *)imx586_to_virt(board_number, p_rbd->rbd_buff);
			if (p_buffer == NULL) {
			    int586_off(board_number);
			    bd->timer_val = 0;
			    return(FALSE);
			}
			bytes_in_msg = p_rbd->rbd_status & CS_RBD_CNT_MSK;
		}
	} while (p_rbd);

	if (mp->b_wptr == mp->b_rptr) {
		bd->mib.ifInErrors++;     	/* SNMP */
		cmn_err(CE_NOTE, "IMX586 rcv'ed 0 length msg");
		freemsg(mp);
		return FALSE;
	}

	/*
	 * THIRD>>>THIRD>>>THIRD>>> queue data--let DLrsrv worry about it
	 */
	if (!imx586recv(mp,bd->sap_ptr)) {
		bd->mib.ifInOctets += (int)(mp->b_wptr - mp->b_rptr); /*SNMP*/
		return(TRUE);
	} else
		return (FALSE);
} /* end of imx586put_packet */

imx586_reset(board_number)
int board_number;
{
	DL_bdconfig_t *bd;
	DL_sap_t *sap;
	bdd_t *bdd;
	char *p_hwcomm, *p_cmd_prom, *p_static_ram;
	scb_t *p_scb;
	int x, oldpri;

	bd = &imx586config[board_number];
	bdd = (bdd_t *)bd->bd_dependent1;

	/*
	 * imx586_reset() is called during the very first open() and from
	 * imx586_watchdog() and imx586dl_data_req.  It is NEVER called directly
	 * from the isr the isr aka imx586intr() will trip the watchdog() to do
	 * the reset). imx586_reset() does not use the interrupt to reset the 586.
	 * This * means the isr is simpler because it does not handle reset 
	 * interrupts -- only xmt and rcv interrupts. imx586_reset()
	 * does need splstr() splx() protection because it must execute 
	 * without interruption.
	 */

	p_cmd_prom   = bdd->p_virt_cmd_prom;
	p_static_ram = bdd->p_virt_static_ram;

	/*
	 * first shut off interrupts from board,
	 * drop chan att - shouldn't be raised
	 */
	int586_off(board_number);
	p_hwcomm = (p_cmd_prom + OFFSET_CHAN_ATT);
	write_word((ushort*)p_hwcomm, CMD_0);

	oldpri = splhi();

	/*
	 *  Some 82C501 parts that don't like comming up with loop back enabled.  
	 *  The fix is to toggle the esi loop back mode.  We will force it out
	 *  of loopback here and then go into loop back after reseting the board.
	 */
	p_hwcomm = (p_cmd_prom + OFFSET_NORMMODE);
	write_word((ushort*)p_hwcomm, CMD_1);

	/*
	 * hardware reset the 586
	 */
	p_hwcomm = (p_cmd_prom + OFFSET_RESET);
	write_word((ushort*)p_hwcomm, CMD_1);
	drv_usecwait(100);	/* 100 usec delay */
	write_word((ushort*)p_hwcomm,CMD_0);
	drv_usecwait(100);	/* 100 usec delay */

	bd->timer_val = -1;
	bdd->round_robin = 0;

	/*
	 * esi loopback - until diagnostics are run
	 */
	p_hwcomm= (p_cmd_prom + OFFSET_NORMMODE);
	write_word((ushort*)p_hwcomm, CMD_0);

	/*
	 * 16 bit - for at bus
	 */
	p_hwcomm = (p_cmd_prom + OFFSET_16B_XFER);
	write_word((ushort*)p_hwcomm, CMD_1);

	/*
	 * initialize all 586 data structs
	 */
	imx586build_cu(board_number);	/* inits scp, iscp, scb, cb, tbd and tbuf */
	imx586build_ru(board_number);	/* inits scb, fd's, rbd's and rbufs */

	/*
	 * chan attention to feed 586 its data structs
	 */
	chan_attn(board_number);

	/*
	 *  wait up to 10 msec for response.
	 */
	p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
	for (x = 1000; x; x--)
	{
		if (p_scb->scb_status == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait(10);	/* wait 10 usec before next try */
	}

	/*
	 * see if board failed
	 */
	if (x == 0)
	{
		splx(oldpri);
		return(FALSE);
	}

	p_scb->scb_cmd = (SCB_ACK_CX | SCB_ACK_CNA);
	chan_attn(board_number);

	/*
	 * 586 cmd number 7, busy waits for execution
	 */
	if (diagnose_586(board_number) == FALSE)
	{
		splx(oldpri);
		return(FALSE);
	}

	/*
	 * cmd # 2, load default config and host id
	 */
	if (config_586(board_number) == FALSE)
	{
		splx(oldpri);
		return(FALSE);
	}

	/*
	 * Insert code for loopback test here
	 */

	/*
	 * 586 is ready, turn on interrupt, turn loopback off and RU on
	 */
	p_hwcomm = (p_cmd_prom + OFFSET_NORMMODE);
	write_word((ushort*)p_hwcomm, CMD_1);
	int586_on(board_number);

	/*
	 * start receive unit on the 586
	 * NOTE: order of evaluation is important.
	 */
	if ((wait_scb(board_number, 100) == TRUE) ||	/* wait up to 1 msec */
		(imx586ru_start(board_number) == FALSE))
	{
		splx(oldpri);
		return(FALSE);
	}

	/*
	 * since imx586_reset can be called from imx586_watchdog(), flush the
	 * write queues.
	 */
	for (x = 0, sap=bd->sap_ptr; x < bd->max_saps; sap++, x++)
		if ( sap->write_q != NULL )
			flushq(sap->write_q, FLUSHALL);
	
	/* Call to the reset nulls out the multicast hash table and this should
	   be loaded again
	*/
	imx586set_multicast(bd);

	splx(oldpri);

	/*
	 * board was successfully reset and initialized,
	 * also CU is idle and RU has been started
	 */
	return(TRUE);

} /* end of imx586_reset() */

void
imx586_watchdog()
{
	DL_bdconfig_t *bd;
	int x, y, oldpri;

	/*
	 * perpetuate the callout
	 */
	imx586timer_id = timeout(imx586_watchdog, 0, HZ);

	for (x = 0, bd = imx586config; x < imx586boards; bd++, x++) {

		oldpri = splstr();

		if (bd->timer_val > 0 )
			bd->timer_val--;

		if (bd->timer_val == 0) {
			bd->timer_val = -1;
			splx(oldpri);
			cmn_err(CE_NOTE, "IMX586: board %d timed out. Board or ethernet cable may be off line.", x);

			/*
			 * board is not responding with interrupts so rest it.
			 */
			for (y = 0; y < 10; y++)
				if (imx586_reset(x) == TRUE)
					return;

			cmn_err(CE_WARN, "IMX586 board %d will not reset", x);
			return;
		}

	splx(oldpri);
	}
	return;
} /* end of imx586_watchdog() */

void
imx586intr(unix_level)
int unix_level;
{
DL_bdconfig_t *bd;
DL_sap_t *sap;
bdd_t *bdd;
int board_number, next, x;
ushort scb_status;
char *p_static_ram;
scb_t *p_scb;
cmd_t *p_cb;
mblk_t *p_msg;
uint collisions;

	board_number = imx586interrupt[unix_level];
	if (board_number > imx586boards - 1) {
		cmn_err(CE_NOTE, "Interrupt from IMX586 board %d not configured in.", board_number);
		return; /* intr not configed in */
	}
	bd = &imx586config[board_number];
	if ((!bd->flags & BOARD_PRESENT)) {
		cmn_err(CE_WARN, "IMX586 board %d not seated.", board_number);
		return;
	}
	bdd = (bdd_t *)bd->bd_dependent1;

	p_static_ram = bdd->p_virt_static_ram;
	p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
	p_cb  = (cmd_t *)(p_static_ram + OFFSET_CU);

	/*
	 * If at any point we don't get the IMX586 to respond, casuse a reset.
	 */
	if (wait_scb(board_number, 100))	/* wait up to 1 msec */
		goto f1;

	scb_status = p_scb->scb_status;
	if ((scb_status & SCB_INT_MSK) == 0)
		return;			/* spurious interrupt */

	p_scb->scb_cmd = (scb_status & SCB_INT_MSK); /* ack the status bits */
	chan_attn(board_number);

	if (scb_status & (SCB_INT_FR | SCB_INT_RNR)) {
		if (wait_scb(board_number, 100))	/* wait up to 1 msec */
			goto f1;
		/*
		 * Indicates that a packet has been received
		 * reset card if receiver in bad state
		 */
		if (imx586rcv_packet(board_number) == FALSE)
			goto f1;
	}

 	/* Indicates that a packet has been transmitted.  */

	if (scb_status & SCB_INT_CNA) {
		if (wait_scb(board_number, 100))	/* wait up to 1 msec */
			goto f1;

		/*
		 * see if CB is still busy
		 */
		if ((p_cb->cmd_status & CS_CMPLT) == 0)
			return;
		bd->flags &= ~TX_BUSY;
		/*
		 * check for OK status and carrier sense
		 */

		if (!(p_cb->cmd_status & CS_OK)) {
			bd->mib.ifOutErrors++;

			if (p_cb->cmd_status & NOCRSMASK)
				bd->mib.ifSpecific.etherCarrierLost++;
			if (p_cb->cmd_status & UNDERRUNMASK)
				bd->mib.ifSpecific.etherUnderrunErrors++;

			/* The transmission is aborted if the number of 
			   collisions reaches a maxmimum of 16 and the the 
			   MAXCOLMASK bit is turned on.
			*/
			if (p_cb->cmd_status & MAXCOLMASK)
				collisions = 16;
			
		} else  {
			if (p_cb->cmd_status & NOCRSMASK)
				cmn_err(CE_WARN, "IMX586 transmission link error - check cable.");
			collisions = p_cb->cmd_status & COLLMASK;
		}

		bd->mib.ifSpecific.etherCollisions += collisions;
 		if (imx586inetstats && imx586stat)
			imx586stat[board_number].ifs_collisions += collisions; 

		bd->timer_val = (-1);

		if (bd->flags & TX_QUEUED) {
			next = bd->tx_next;
			sap = &imx586saps[next];
			p_msg = NULL;
			for (x = 0; x < bd->max_saps; x++) {
				if (++next == bd->max_saps)
					next = 0;
				if ( (sap->state == DL_IDLE) &&
						(p_msg = getq(sap->write_q)) ) {
					bd->tx_next = next;
					(void)imx586xmit_packet(sap->bd, p_msg);
					bd->mib.ifOutQlen--; /* SNMP  */
					return;
				}
				if (next == 0)
					sap = imx586saps;
				else
					sap++;
			}
			bd->flags &= ~TX_QUEUED;
			bd->mib.ifOutQlen = 0;	/* SNMP */
		}
	} /* end of SCB_INT_CNA */

	bd->mib.ifSpecific.etherAlignErrors = p_scb->scb_aln_err;
	bd->mib.ifSpecific.etherCRCerrors = p_scb->scb_crc_err;
	bd->mib.ifSpecific.etherMissedPkts = p_scb->scb_rsc_err;
	bd->mib.ifSpecific.etherOverrunErrors = p_scb->scb_ovrn_err;
	bd->mib.ifInErrors = bd->mib.ifSpecific.etherAlignErrors
				+ bd->mib.ifSpecific.etherCRCerrors
				+ bd->mib.ifSpecific.etherMissedPkts
				+ bd->mib.ifSpecific.etherOverrunErrors;

	return;

f1:
	/*
	 *  If we get here, cause the IMX586 to reset.
	 */
	bd->timer_val = 0;

	/*
	 * it will be turned on during reset
	 */
	int586_off(board_number);

	return;
} /* end of imx586intr() */

imx586rcv_packet(board_number)
int board_number;
{
fd_t *p_fd;
DL_bdconfig_t *bd = &imx586config[board_number];
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	/* first... & last... are same as begin_rbd... & end... for one fd */
	rbd_t *p_first_rbd, *p_last_rbd;

	for (p_fd = bdd->begin_fd; p_fd != NULL; p_fd = bdd->begin_fd)
	{
		if (p_fd->fd_status & CS_CMPLT)
		{
			bdd->begin_fd =
				    (fd_t *)imx586_to_virt( board_number,p_fd->fd_nxt_ofst);

			p_first_rbd =(rbd_t *)imx586_to_virt(board_number,p_fd->fd_rbd_ofst);

			/* 
			 * see p2-46 of 586 manual
			 */
			if (p_fd->fd_rbd_ofst != 0xffff) {
				p_last_rbd = p_first_rbd;
				while ((p_last_rbd->rbd_status & CS_EOF) != CS_EOF &&
					   (p_last_rbd->rbd_size   & CS_EL)  != CS_EL)
					p_last_rbd = (rbd_t *)imx586_to_virt(board_number,
						    						p_last_rbd->rbd_nxt_ofst);

				bdd->begin_rbd =
				  (rbd_t *)imx586_to_virt(board_number,p_last_rbd->rbd_nxt_ofst);

				p_last_rbd->rbd_nxt_ofst = 0xffff;
				if (p_fd->fd_status & CS_OK) {
				 	if(imx586put_packet(board_number,p_fd)){
				 		if (imx586inetstats && imx586stat)
							imx586stat[board_number].ifs_ipackets++;
					} else {
				 		if (imx586inetstats && imx586stat)
							imx586stat[board_number].ifs_ierrors++;
					}
				} else	{
					if (imx586inetstats && imx586stat)
						imx586stat[board_number].ifs_ierrors++;
				}
			}
			imx586re_q_fd(board_number,p_fd);
		}
		else
			break;
	}

	return (imx586ru_start(board_number));
} /* end of imx586rcv_packet */


void
imx586re_q_fd(board_number, p_fd)
int board_number;
fd_t *p_fd;
{
rbd_t *p_last_rbd, *p_first_rbd;
DL_bdconfig_t *bd = &imx586config[board_number];
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	/*
	 * C. Yager's driver example in 586 ref man makes sure
	 * that two fd and two rbd are enqueued before restarting
	 * the RU. The design of this driver ensures that all full
	 * fd/rbds are emptied then re_q ed at interrupt time. Thus
	 * this driver never needs to check for a minimum level
	 * of fd/rbds before calling imx586ru_start(). See chapter 4 of
	 * microcomm handbook.
	 */
	p_first_rbd = (rbd_t *)imx586_to_virt(board_number,p_fd->fd_rbd_ofst);

	p_fd->fd_status   = 0;
	p_fd->fd_cmd      = CS_EL; /* its going to be the last fd on the list */
	p_fd->fd_nxt_ofst = 0xffff;
	p_fd->fd_rbd_ofst = 0xffff;

	/*
	 * this can never happen !!!!!!!!!!!!!!!
	 * if (bdd->begin_fd == NULL)
	 *	  bdd->begin_fd = bdd->end_fd = p_fd;
	 */

	/*
	 * ...end_fd->fd_nxt_ofst MUST be linked
	 * before ...fd_cmd, see p3-8 586 man
	 */
	bdd->end_fd->fd_nxt_ofst = virt_to_imx586(board_number, (char*)p_fd);
	bdd->end_fd->fd_cmd		 = 0;	/* no last now */
	bdd->end_fd				 = p_fd;

	if (p_first_rbd != NULL)
	{
		for (p_last_rbd=p_first_rbd;
				(p_last_rbd->rbd_status & CS_EOF)!= CS_EOF &&
				(p_last_rbd->rbd_size   & CS_EL) != CS_EL;
		    			p_last_rbd= (rbd_t *)imx586_to_virt(board_number,
													p_last_rbd->rbd_nxt_ofst))
		{
			/*
			 * clear eof and act count
			 */
			p_last_rbd->rbd_status = 0;
		}

		p_last_rbd->rbd_status = 0;
		p_last_rbd->rbd_size  |= CS_EL; /* new end of rbd list */

		/*
		 * can this ever happen ??????? !!!!!!!!!!!!!!
		 */
		if (bdd->begin_rbd == NULL)
		{
			bdd->begin_rbd = p_first_rbd;
			bdd->end_rbd   = p_last_rbd;
		}
		else
		{
			/*
			 * end_rbd->rbd_nxt_ofst MUST be linked before ~CS_EL is done
			 */
			bdd->end_rbd->rbd_nxt_ofst =
								virt_to_imx586(board_number, (char*)p_first_rbd);
			bdd->end_rbd->rbd_size	  &= ~CS_EL;
			bdd->end_rbd			   = p_last_rbd;
		}
	}

	return;
} /* end of imx586re_q_fd() */

imx586ru_start(board_number)
	int board_number;
{
	scb_t *p_scb;
	char *p_static_ram;
	fd_t *begin_fd;
	DL_bdconfig_t *bd= &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	begin_fd     = bdd->begin_fd;
	p_static_ram = bdd->p_virt_static_ram;
	p_scb        = (scb_t *)( p_static_ram + OFFSET_SCB );

	/*
	 * RU already running -- leave it alone
	 */
	if ((p_scb->scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return (TRUE);

	/*
	 * Receive queue is exhausted, need to reset the board
	 */
	if (begin_fd == NULL)
	{
		return (FALSE);
	}

	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * imx586rcv_packet() and thus calls imx586ru_start() which will then
	 * start the RU if necessary.
	 */
	if (begin_fd->fd_status & CS_CMPLT)
		return (TRUE);

	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on p2-15
	 */
	begin_fd->fd_rbd_ofst = virt_to_imx586( board_number,
												(char*)(bdd->begin_rbd));

	if (wait_scb(board_number, 100))	/* wait up to 1 msec */
		return (FALSE);

	p_scb->scb_rfa_ofst = virt_to_imx586(board_number, (char*)begin_fd);
	p_scb->scb_cmd      = SCB_RUC_STRT;
	chan_attn(board_number);

	return (TRUE);
} /* end of imx586ru_start */

/*
 * imx586xmit_packet() is called ONLY by imx586intr() and imx586dl_data_req().
 * Also, both these routines call wait_scb() just before calling
 * imx586xmit_packet() -- therefore, no need to call wait_scb() as the first
 * statement of imx586xmit_packet().
 */
int
imx586xmit_packet(bd, mp) /*arg count has been changed for DLPI compatibility */
DL_bdconfig_t *bd;
mblk_t *mp;
{
int i, board_number, p_type, y, jj, bytes_left_over;
unsigned long *p_userdata, *p_xmtdata;
pack_ulong_t partial_long;
ushort sap_id;
char *p_static_ram;
ushort *p_addr, *p_destaddr;
int bytes_in_msg;
scb_t *p_scb;
cmd_t *p_cb;
tbd_t *p_tbd;
mblk_t *mp2;
DL_bdconfig_t *bd2;
bdd_t *bdd;
DL_mac_hdr_t *hdr = (DL_mac_hdr_t *)mp->b_rptr;


	bdd = (bdd_t *)bd->bd_dependent1;

	/*
	 * Ugly hack to determine board_number, a variable no longer passed
	 * to this function.
	 */

	board_number = bd->bd_number;

	bd->flags |= TX_BUSY; /* for DLPI compatibility */

	p_static_ram = bdd->p_virt_static_ram;
	p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
	p_cb  = (cmd_t *)(p_static_ram + OFFSET_CU);
	p_tbd = (tbd_t *)(p_static_ram + OFFSET_TBD);
	mp2 = mp;

	bd->timer_val = 3;
	/* .cmd_timer * timeout() value in imx586_watchdog sec max for 586 */

	/*
	 * FIRST>>>FIRST>>>FIRST>>> fill in the 586 command block
	 */
	p_cb->cmd_status   = 0;
	p_cb->cmd_cmd      = CS_EL | CS_CMD_XMIT | CS_INT;
	p_cb->cmd_nxt_ofst = OFFSET_CU;
	/*
	 * only one cb and it points to itself
	 */
	p_cb->prmtr.prm_xmit.xmt_tbd_ofst = OFFSET_TBD;

	p_addr     = (ushort *)(hdr->dst.bytes);
	p_destaddr = (ushort *)(p_cb->prmtr.prm_xmit.xmt_dest);

	/*
	 * bcopy( p_addr,p_destaddr, DL_MAC_ADDR_LEN ); wont work on imx586 !!!!
	 */

	/* Need to separate out the dest mac addr and the type/len field.
	   The source mac addr will be added by the hardware. However the
	   frame handed down from the independent part contains the entire
	   frame and hence the need to offset the read pointer by 12 bytes 
	 */
 
	for (y = 0; y < (DL_MAC_ADDR_LEN / 2); y++)
		*p_destaddr++ = *p_addr++;
	p_cb->prmtr.prm_xmit.xmt_length = hdr->mac_llc.ether.len_type;

	bd->mib.ifOutOctets += msgdsize(mp);

	mp->b_rptr += LLC_EHDR_SIZE;

#ifdef TYPEISINTHEMSG 
	/*
	 * Support for type field intended by upper modules being in the msg
	 */ 
	p_type = *((unsigned short *)(mp->b_rptr + off + DL_MAC_ADDR_LEN ));
	p_cb->prmtr.prm_xmit.xmt_length=p_type;
#endif

	/*
	 * Fill the transmit buffer descriptor
	 */
	p_tbd->tbd_count     = (msgdsize(mp)) | CS_EOF;
	p_tbd->tbd_nxt_ofst  = 0xffff;
	p_tbd->tbd_buff      = OFFSET_TBUF;
	p_tbd->tbd_buff_base = 0;

	/*
	 * Put user data in transmit buffer
	 */
	p_xmtdata = (unsigned long *)(p_static_ram + OFFSET_TBUF);

	/*
	 * data in 386 ram does not have to be word
	 * aligned but data on imx586 does
	 *
	 * if msg ends with a partial long, this is written to imx586 board
	 * the <4 garbage bytes will be ignored by 586
	 */
	while (mp2) {
		bytes_in_msg = mp2->b_wptr - mp2->b_rptr;
		p_userdata = (unsigned long *)(mp2->b_rptr);
		bytes_left_over = bytes_in_msg & 3;

		imx586bcopy(p_userdata, p_xmtdata, (bytes_in_msg - bytes_left_over));

		p_xmtdata   += (bytes_in_msg - bytes_left_over) / 4;
		mp2->b_rptr += (bytes_in_msg - bytes_left_over);

		if (bytes_left_over) {
			for (jj = 0; jj < 4; jj++) {
				partial_long.c.a[jj] = *(mp2->b_rptr);
				(mp2->b_rptr)++;

				if (mp2->b_rptr >= mp2->b_wptr)	
					mp2 = mp2->b_cont;

				if (mp2 == NULL)
					break;
			}
			*p_xmtdata++ = partial_long.c.b;
		}
		else
			mp2 = mp2->b_cont;
	}

	/*
	 * Make 586 do a transmit
	 */
	p_scb->scb_cmd = SCB_CUC_STRT;
	chan_attn(board_number);

	if (imx586inetstats && imx586stat)
		imx586stat[board_number].ifs_opackets++;

	freemsg(mp);
	return (0);
} /* end of imx586xmit_packet */

/*
 * Special-purpose utility routines for programming the 82586
 */

static void
write_word(virtual_addr, value)
ushort *virtual_addr;
ushort value;
{
	*virtual_addr = value;
	return;
} /* end of write_word() */

static void
write_byte(virtual_addr, value)
char *virtual_addr;
char value;
{
	*virtual_addr = value;
	return;
} /* end of write_byte() */

static ushort
read_word(virtual_addr)
ushort *virtual_addr;
{
	ushort value;
	value = *virtual_addr;
	return(value);
} /* end of read_word() */

void
imx586build_cu(board_number) /* inits scp, iscp, scb, cb, tbd and tbuf */
int board_number;
{
	char *p_ram, *p_static_ram;
	cmd_t *p_cb;
	tbd_t *p_tbd;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_static_ram = bdd->p_virt_static_ram;

	/*
	 * set up data structs as listed above
	 */
	p_ram = (p_static_ram + OFFSET_SCP);
	((scp_t *)p_ram)->scp_sysbus = 0;

	/*
	 * 16 bit bus see page 2-12 of 586 ref
	 */
	((scp_t *)p_ram)->scp_iscp      = (OFFSET_ISCP);
	((scp_t *)p_ram)->scp_iscp_base = 0;

	p_ram= (p_static_ram + OFFSET_ISCP);
	((iscp_t *)p_ram)->iscp_busy     = 1;
	((iscp_t *)p_ram)->iscp_scb_ofst = OFFSET_SCB;
	((iscp_t *)p_ram)->iscp_scb_base = 0;

	p_ram= (p_static_ram + OFFSET_SCB);
	((scb_t *)p_ram)->scb_status   = 0;
	((scb_t *)p_ram)->scb_cmd      = 0;
	((scb_t *)p_ram)->scb_cbl_ofst = OFFSET_CU;
	((scb_t *)p_ram)->scb_rfa_ofst = OFFSET_RU;
	((scb_t *)p_ram)->scb_crc_err  = 0;
	((scb_t *)p_ram)->scb_aln_err  = 0;
	((scb_t *)p_ram)->scb_rsc_err  = 0;
	((scb_t *)p_ram)->scb_ovrn_err = 0;

	p_cb               = (cmd_t *)(p_static_ram + OFFSET_CU);
	p_cb->cmd_status   = 0;
	p_cb->cmd_cmd      = CS_EL;
	p_cb->cmd_nxt_ofst = OFFSET_CU;
	/* just to be safe - its not needed for "simple" command processing */

	p_tbd=(tbd_t *)(p_static_ram + OFFSET_TBD);
	p_tbd->tbd_count     = 0;
	p_tbd->tbd_nxt_ofst  = 0xffff; /*"simple" cmnd processing ref page 3-6*/
	p_tbd->tbd_buff  = 0;	/* gets proper value in imx586xmit_packet() */
	p_tbd->tbd_buff_base = 0;

	return;
} /* end of imx586build_cu() */

/* builds linear linked lists of fd's and
 * rbd's see page 4-32 of 1986 intel microcomm handbook
 */
void
imx586build_ru(board_number)
int board_number; 
{
fd_t *p_fd;
int x;
typedef struct {
	rbd_t r;
	char rbd_pad[2];	/* puts rbuffer[] on 4 byte boundry */
	char rbuffer[RCVBUFSIZE];
} ru_t;
ru_t *p_rbd;
DL_bdconfig_t *bd = &imx586config[board_number];
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_fd = (fd_t *)(bdd->p_virt_static_ram + OFFSET_RU);
	bdd->begin_fd = p_fd;
	for (x = 0; x < N_FD; x++) {
		p_fd->fd_status   = 0;
		p_fd->fd_cmd      = 0;
		p_fd->fd_nxt_ofst = virt_to_imx586(board_number, (char *)(p_fd + 1));
		p_fd->fd_rbd_ofst = 0xffff; /* must be 0xffff see page 2-46 */
		p_fd++;
	}
	/*
	 * point to &fd[N_FD-1]
	 */
	bdd->end_fd   = --p_fd;
	p_fd->fd_nxt_ofst = 0xffff;	/* nothing to point to */
	p_fd->fd_cmd      = CS_EL;	/* end of list         */

	p_rbd = (ru_t *)(bdd->p_virt_static_ram + OFFSET_RBD);
	bdd->begin_rbd = (rbd_t *)p_rbd;

	/*
	 * the first fd will point to the linked list of rbd's
	 */
	/* bdd->begin_fd->fd_rbd_ofst= virt_to_imx586(board_number, (char *)p_rbd); */
	p_fd->fd_rbd_ofst= virt_to_imx586(board_number, (char *)p_rbd);
	for (x = 0; x < N_RBD; x++) {
		p_rbd->r.rbd_status = 0;
		p_rbd->r.rbd_nxt_ofst  = virt_to_imx586(board_number,(char *)(p_rbd+1));
		p_rbd->r.rbd_buff= virt_to_imx586(board_number, p_rbd->rbuffer);
		p_rbd->r.rbd_buff_base = 0;
		p_rbd->r.rbd_size = RCVBUFSIZE;
		p_rbd++;
	}
	bdd->end_rbd       = (rbd_t *)(--p_rbd);
	p_rbd->r.rbd_nxt_ofst  = 0xffff;     /* last rbd points to ground */
	p_rbd->r.rbd_size     |= CS_EL;      /* eof on the last rbd */

	return;
} /* end of imx586build_ru() */

/*
 * does 586 op-code number 7
 */
diagnose_586(board_number)
int board_number;
{
	char  *p_static_ram;
	scb_t *p_scb;
	cmd_t *p_cb;
	int x;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_static_ram = bdd->p_virt_static_ram;
	p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
	p_cb  = (cmd_t *)(p_static_ram + OFFSET_CU);

	/*
	 *  wait up to 10 msec for scb to be ready
	 */
	if (wait_scb(board_number, 1000))
		return (FALSE);

	p_scb->scb_cmd = ((p_scb->scb_status) & SCB_INT_MSK);
	if (p_scb->scb_cmd)
		chan_attn(board_number);	/* ack the status bits */

	/*
	 *  wait up to 10 msec for scb to be ready
	 */
	if (wait_scb(board_number, 1000))
		return (FALSE);

	p_cb->cmd_status = 0; /* 586 will write this */
	p_cb->cmd_cmd    = CS_CMD_DGNS | CS_EL;
	p_scb->scb_cmd   = SCB_CUC_STRT;
	chan_attn(board_number);

	/*
	 *  wait up to 10 msec for response.
	 */
	for (x = 100; x; x--) {
		if (p_cb->cmd_status & CS_OK)
			break;
		drv_usecwait(10);	/* wait 10 usec before next try */
	}
	if (x == 0)
		return(FALSE);

	p_scb->scb_cmd = (p_scb->scb_status) & SCB_INT_MSK;
	if (p_scb->scb_cmd)
		chan_attn(board_number);

	return(TRUE);
} /* end of diagnose_586() */

int
imx586_eaddr(bd)
DL_bdconfig_t *bd;
{
ushort *p_addr, *p_addr2;
char *p_static_ram;
scb_t *p_scb;
cmd_t *p_cb;
int x;

bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
int board_number = bd->bd_number;

	p_static_ram = bdd->p_virt_static_ram;
	p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
	p_cb  = (cmd_t *)(p_static_ram + OFFSET_CU);

	if (wait_scb(board_number, 1000))	/* wait up to 10 msec */
		return (FALSE);

	p_scb->scb_cmd = (p_scb->scb_status) & SCB_INT_MSK;
	if (p_scb->scb_cmd)
		chan_attn(board_number); /* ack the stat bits */

	/*
	 *  wait up to 10 msec for scb to be ready
	 */
	if (wait_scb(board_number, 1000))
		return (FALSE);

	p_cb->cmd_status = 0; /* 586 will write this */
	p_cb->cmd_cmd    = CS_CMD_IASET | CS_EL;

	p_addr = (ushort *)&(bd->eaddr.bytes[0]);
	p_addr2 = (ushort *)&(p_cb->prmtr.prm_ia_set[0]);

	for (x = 0; x < (DL_MAC_ADDR_LEN / 2); x++)
		*p_addr2++ = *p_addr++;

	p_scb->scb_cmd = SCB_CUC_STRT;
	chan_attn(board_number);
	/*
	 *  wait up to 10 msec for response.
	 */
	for (x = 100; x; x--) {
		if (p_cb->cmd_status & CS_OK)
			break;
		drv_usecwait(10);	/* wait 10 usec before next try */
	}

	if (x == 0)
		return(FALSE);

	p_scb->scb_cmd = (p_scb->scb_status) & SCB_INT_MSK;
	chan_attn(board_number);

	return(TRUE);
}

int
imx586_hdwopts(bd)
DL_bdconfig_t *bd;
{
ushort *p_addr, *p_addr2;
char *p_static_ram;
scb_t *p_scb;
cmd_t *p_cb;
int x;

bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
int board_number = bd->bd_number;


	p_static_ram = bdd->p_virt_static_ram;
	p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
	p_cb  = (cmd_t *)(p_static_ram + OFFSET_CU);

	/*
	 *  wait up to 10 msec for scb to be ready
	 */
	if (wait_scb(board_number, 1000))
		return (FALSE);

	p_scb->scb_cmd = (p_scb->scb_status) & SCB_INT_MSK;
	if (p_scb->scb_cmd)
		chan_attn(board_number); /* ack the stat bits */

	/*
	 *  wait up to 10 msec for scb to be ready
	 */
	if (wait_scb(board_number, 1000))
		return (FALSE);

	p_cb->cmd_status = 0; /* 586 will write this */
	p_cb->cmd_cmd    = CS_CMD_CONF | CS_EL;

	/*
	 * default config p2-28 586 book
	 */

	p_cb->prmtr.prm_conf.cnf_fifo_byte =  bdd->cnf_fifo_byte;
	p_cb->prmtr.prm_conf.cnf_add_mode  = bdd->cnf_add_mode;
	p_cb->prmtr.prm_conf.cnf_pri_data  = bdd->cnf_pri_data;
	p_cb->prmtr.prm_conf.cnf_slot      = bdd->cnf_slot;
	p_cb->prmtr.prm_conf.cnf_hrdwr     = bdd->cnf_hrdwr;
	p_cb->prmtr.prm_conf.cnf_min_len   = bdd->cnf_min_len;

	p_scb->scb_cmd = SCB_CUC_STRT;
	chan_attn(board_number);

	/*
	 *  wait up to 10 msec for response.
	 */
	for (x = 100; x; x--) {
		if (p_cb->cmd_status & CS_OK)
			goto c1;
		drv_usecwait(10);	/* wait 10 usec before next try */
	}

	return(FALSE);
c1:
	p_scb->scb_cmd = (p_scb->scb_status) & SCB_INT_MSK;
	if (p_scb->scb_cmd)
		chan_attn(board_number);
	return (TRUE);
}

/*
 * std config then an ia-setup (read prom)
 */
config_586(board_number)
int board_number;
{
DL_bdconfig_t *bd = &imx586config[board_number];

	if (imx586_hdwopts(bd) == FALSE)
		return (FALSE);
	return (imx586_eaddr(bd));
} /* end of config_586() */

static
prom_address(board_number, index)
int board_number, index;
{
	char *p_cmd_prom;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_cmd_prom  = bdd->p_virt_cmd_prom;
	p_cmd_prom += OFFSET_ADDR_PROM;
	p_cmd_prom += index;

	return(*p_cmd_prom);
} /* end of prom_address() */

static void
chan_attn(board_number)
int board_number;
{
	char *p_hwcomm, *p_cmd_prom;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_cmd_prom = bdd->p_virt_cmd_prom;
	p_hwcomm   = (p_cmd_prom + OFFSET_CHAN_ATT);

	/*
	 * first byte of word is 1 - this sets the CA
	 * second byte of word is zero - this clears CA
	 */
	write_word((ushort*)p_hwcomm, 0x01);

	return;
} /* end of chan_attn() */

/*
 * Acceptance of a Control Command is indicated
 * by the 82586 clearing the SCB command field
 * page 2-16 of the intel microcom handbook
 */
static
wait_scb(board_number, how_long)
int board_number;
int	how_long;
{
scb_t *p_scb;
char *p_static_ram;
DL_bdconfig_t *bd = &imx586config[board_number];
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_static_ram = bdd->p_virt_static_ram;
	p_scb        = (scb_t *)(p_static_ram + OFFSET_SCB);

	/*
	 *  Wait as long as the caller wants.
	 */
	while (how_long--) {
		if (p_scb->scb_cmd == 0)
			return(FALSE);
		drv_usecwait(10);	/* wait 10 usec before next try */
	}
	return(TRUE);
} /* end of wait_scb() */

/*
 * interrupt on out at 586 card
 */
void
int586_on(board_number)
int board_number;
{
	char *p_hwcomm, *p_cmd_prom;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_cmd_prom = bdd->p_virt_cmd_prom;
	p_hwcomm   = (p_cmd_prom + OFFSET_INT_ENAB);
	write_word((ushort*)p_hwcomm, CMD_1);

	return;
} /* end of int586_on() */

/*
 * interrupt off out at 586 card
 */
void
int586_off(board_number)
int board_number;
{
	char *p_hwcomm, *p_cmd_prom;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	p_cmd_prom = bdd->p_virt_cmd_prom;
	p_hwcomm   = (p_cmd_prom + OFFSET_INT_ENAB);
	write_word((ushort*)p_hwcomm, CMD_0);

	return;
} /* end of int586_off() */

ushort
virt_to_imx586(board_number, kernel_virt_addr)
int		board_number;
char	*kernel_virt_addr;
{
	char *p_static_ram;
	ushort imx586;
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	/*
	 * 586 uses 0xffff for null as "c" uses 0 for null
	 */
	if (kernel_virt_addr == NULL)
		return(0xffff);

	if ((board_number > imx586boards-1) || (board_number < 0))
		return(0xffff); /* error */

	p_static_ram = bdd->p_virt_static_ram;
	if (p_static_ram > kernel_virt_addr)
		return(0xffff);	/* error */

	imx586 = (ushort)(kernel_virt_addr - p_static_ram);
	if (imx586 >0x7fff)
		return(0xffff); /* error */

	return(imx586);
} /* end of virt_to_imx586() */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
char *
imx586_to_virt(board_number, imx586_addr)
int board_number;
ushort imx586_addr;
{
	DL_bdconfig_t *bd = &imx586config[board_number];
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;

	/*
	 * 586 uses 0xffff for null as "c" uses 0 for null
	 */
	if (imx586_addr == 0xffff)
		return (NULL);

	if (board_number > imx586boards - 1 || board_number < 0)
		return(NULL); /* error */

	if (imx586_addr > 0x7fff)
		return(NULL);

	return(bdd->p_virt_static_ram+imx586_addr);
} /* end of imx586_to_virt() */


/* imx586promisc_off */
int
imx586promisc_off(bd)
DL_bdconfig_t *bd;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
int rval,oldlevel;

	oldlevel = splhi();

	bdd->cnf_hrdwr &= (~LIS_PROMISC);

	rval = imx586_hdwopts(bd);
	rval ^= rval;

	splx(oldlevel);
	return (rval);
}

/* imx586promisc_on */
int
imx586promisc_on(bd)
DL_bdconfig_t *bd;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
int rval,oldlevel;

	oldlevel = splhi();

	bdd->cnf_hrdwr |= LIS_PROMISC;

	rval = imx586_hdwopts(bd);
	rval ^= rval;

	splx(oldlevel);
	return (rval);
}

/* imx586set_eaddr */
int
imx586set_eaddr(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
int rval,oldlevel = splstr();

	BCOPY(eaddr->bytes,bd->eaddr.bytes,DL_MAC_ADDR_LEN);
	rval = imx586_eaddr(bd);
	rval ^= rval;
	splx(oldlevel);
	return(rval);
}

/*
* imx586add_multicast()
*
*/

int
imx586add_multicast(bd, maddr)
DL_bdconfig_t *bd;
DL_eaddr_t *maddr;
{
bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
mcat_t 	*mcp = &(bdd->imx586_multiaddr[0]);
register int i;
int rval,oldlevel;


	oldlevel = splstr();
	if ( (bd->multicast_cnt >= MULTI_ADDR_CNT) || (!maddr->bytes[0] & 0x1)){
		splx(oldlevel);
		return 1;
	}
	if ( imx586is_multicast(bd,maddr)) {
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
	rval = imx586set_multicast(bd);
	splx(oldlevel);
	return rval;
}

/* 
* imx586del_multicast()
*/

int
imx586del_multicast(bd, maddr)
DL_bdconfig_t *bd;
DL_eaddr_t *maddr;
{
register int i;
bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
mcat_t 	*mcp = &(bdd->imx586_multiaddr[0]);
int rval, oldlevel;


	oldlevel = splstr();

        if (!imx586is_multicast(bd,maddr))
                return 1;
        for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
		if (!mcp->status)
			continue;
                if ( BCMP(maddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0)
                        break;
        }
	mcp->status = 0;
	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
                mcp->entry[i] = 0x00;
	rval = imx586set_multicast(bd);
	splx(oldlevel);
	return rval; 
}

/* 
* imx586set_multicast()
*/

int
imx586set_multicast(bd)
DL_bdconfig_t *bd;
{
bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
mcat_t 	*mcp = &(bdd->imx586_multiaddr[0]);
register int i;
register int j;
DL_bdconfig_t *bd2;
int board_number;
char  *p_static_ram;
scb_t *p_scb;
cmd_t *p_cb;
ushort *p_addr, *p_addr2;
int x;
unsigned char *ptr;

/* The calling function should call splstr() and splx()		*/

	board_number = bd->bd_number;
        p_static_ram = bdd->p_virt_static_ram;
        p_scb = (scb_t *)(p_static_ram + OFFSET_SCB);
        p_cb  = (cmd_t *)(p_static_ram + OFFSET_CU);

        /*
         *  wait up to 10 msec for scb to be ready
         */
        if (wait_scb(board_number, 1000))
                return (1);

        p_scb->scb_cmd = ((p_scb->scb_status) & SCB_INT_MSK);
        if (p_scb->scb_cmd)
                chan_attn(board_number);        /* ack the status bits */

        if (wait_scb(board_number, 1000))
                return (1);
 	p_cb->cmd_status = 0; /* 586 will write this */
        p_cb->cmd_cmd    = CS_CMD_MCSET | CS_EL;
	mcp = &(bdd->imx586_multiaddr[0]); 

	for (i =0,j = 0;i < MULTI_ADDR_CNT;i++,mcp++) {
		if (mcp->status) {
			p_addr = (ushort *)&(mcp->entry[0]);
			p_addr2 = (ushort *)&(p_cb->prmtr.prm_mcad.mc_addr[j]);
			for (x = 0; x < (DL_MAC_ADDR_LEN / 2); x++)
				*p_addr2++ = *p_addr++;
			j += DL_MAC_ADDR_LEN;
		}
	}
	p_cb->prmtr.prm_mcad.mc_cnt = j;

        p_scb->scb_cmd   = SCB_CUC_STRT;
        chan_attn(board_number);
	for (x = 100; x; x--) {
                if (p_cb->cmd_status & CS_OK)
                        break;
                drv_usecwait(10);       /* wait 10 usec before next try */
        }
        if (x == 0)
                return(1);

        p_scb->scb_cmd = (p_scb->scb_status) & SCB_INT_MSK;
        if (p_scb->scb_cmd)
                chan_attn(board_number);

	return(0);
}

/*	imx586is_multicast()
*
*/

int
imx586is_multicast(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
bdd_t *imxp= (bdd_t *)bd->bd_dependent1;
register mcat_t *mcp = &imxp->imx586_multiaddr[0]; 
register int i;
int oldlevel;

	oldlevel = splstr();

	if (bd->multicast_cnt == 0)
		return (0);
	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
		if (mcp->status == 0x00)
			continue;
		if (BCMP(eaddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) {
			splx(oldlevel);
			return (1);
		}
	}
	splx(oldlevel);
	return (0);
}

STATIC void
imx586uninit()
{
	DL_bdconfig_t *bd = imx586config;
	int i;

	if (imx586inetstats)
		if (imx586stat)
			kmem_free(imx586stat, sizeof(struct ifstats) * imx586boards);
	for (i = 0; i < imx586boards; i++, bd++)
		if (bd->bd_dependent1)
			kmem_free((int *)bd->bd_dependent1, sizeof(bdd_t));
	untimeout(imx586timer_id);
}

/* not supported */
int
imx586disable(bd)
DL_bdconfig_t *bd;
{
	return(1);
}

/* not supported */
int
imx586enable(bd)
DL_bdconfig_t *bd;
{
	return(1);
}

imx586reset(bd)
DL_bdconfig_t *bd;
{
int rval,oldlevel;

	oldlevel = splstr();
	rval = imx586_reset(bd->bd_number);
	rval ^= rval;
	splx(oldlevel);
	return (rval);
}

int
DLget_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
register mcat_t  *mcp = &(bdd->imx586_multiaddr[0]);
register int i;
unsigned char *dp;
int found = 0;

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
