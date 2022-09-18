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

#ifndef _IO_DLPI_ETHER_I596_H /* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_I596_H /* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/i596.h	1.4"
#ident  "$Header: $"

#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#define CA_CTRL			0x06		/* Channel Attention */

/*
 * SCB STAT (we call it INT) bits
 * indicate the nature of an incoming interrupts from 82596
 */
#define	SCB_INT_MSK		0xf000	/* SCB STAT bit mask */
#define	SCB_INT_CX		0x8000	/* CX bit, CU finished a command with "I" set */
#define	SCB_INT_FR		0x4000	/* FR bit, RU finished receiving a frame */
#define	SCB_INT_CNA		0x2000	/* CNA bit, CU not active */
#define	SCB_INT_RNR		0x1000	/* RNR bit, RU not ready */

/* 
 * SCB Command Unit STAT bits
 */
#define	SCB_CUS_MSK		0x0700		/* SCB CUS bit mask */
#define	SCB_CUS_IDLE	0x0000	/* CU idle */
#define	SCB_CUS_SUSPND	0x0100	/* CU suspended */
#define	SCB_CUS_ACTV	0x0200	/* CU active */

/* 
 * SCB Receive Unit STAT bits
 */
#define	SCB_RUS_MSK		0x0070	/* SCB RUS bit mask */
#define	SCB_RUS_IDLE	0x0000	/* RU idle */
#define SCB_RUS_SUSPND	0x0010	/* RU suspended */
#define SCB_RUS_NORESRC 0x0020	/* RU no resource */
#define	SCB_RUS_READY	0x0040	/* RU ready */

/*
 * SCB ACK bits
 * these bits are used to acknowledge an interrupt from 82596
 */
#define SCB_ACK_MSK		0xf000	/* SCB ACK bit mask */
#define SCB_ACK_CX		0x8000	/* ACK_CX,  acknowledge a completed cmd */
#define SCB_ACK_FR		0x4000	/* ACK_FR,  acknowledge a frame reception */
#define	SCB_ACK_CNA		0x2000	/* ACK_CNA, acknowledge CU not active */
#define SCB_ACK_RNR		0x1000	/* ACK_RNR, acknowledge RU not ready */

/* 
 * SCB Command Unit commands
 */
#define	SCB_CUC_MSK		0x0700	/* SCB CUC bit mask */
#define	SCB_CUC_STRT	0x0100	/* start CU */
#define	SCB_CUC_RSUM	0x0200	/* resume CU */
#define	SCB_CUC_SUSPND	0x0300	/* suspend CU */
#define	SCB_CUC_ABRT	0x0400	/* abort CU */

/* 
 * SCB Receive Unit commands 
 */
#define SCB_RUC_MSK		0x0070	/* SCB RUC bit mask */
#define	SCB_RUC_STRT	0x0010	/* start RU */
#define	SCB_RUC_RSUM	0x0020	/* resume RU */
#define	SCB_RUC_SUSPND	0x0030	/* suspend RU */
#define	SCB_RUC_ABRT	0x0040	/* abort RU */

/*
 * SCB software reset bit
 */
#define SCB_RESET	0x0080		/* RESET, reset chip same as hardware reset */

/*
 * general defines for the command and descriptor blocks
 */
#define CS_CMPLT		0x8000	/* C bit, completed */
#define CS_BUSY			0x4000	/* B bit, Busy */
#define CS_OK			0x2000	/* OK bit, error free */
#define CS_ABORT		0x1000	/* A bit, abort */
#define CS_EL			0x8000	/* EL bit, end of list */
#define CS_SUSPND		0x4000	/* S bit, suspend */
#define CS_INT			0x2000	/* I bit, interrupt */
#define	CS_STAT_MSK		0x3fff	/* Command status mask */
#define CS_EOL			0xffff	/* set for fd_rbd_ofst on unattached FDs */
#define CS_EOF			0x8000	/* EOF (End Of Frame) in the TBD and RBD */
#define	CS_RBD_CNT_MSK	0x3fff	/* actual count mask in RBD */

#define	CS_COLLISIONS	0x000f
#define	CS_CARRIER		0x0400
#define	CS_ERR_STAT		0x07e0

/*
 * 82596 commands
 */
#define CS_CMD_MSK		0x07	/* command bits mask */
#define	CS_CMD_NOP		0x00	/* NOP */
#define	CS_CMD_IASET	0x01	/* Individual Address Set up */
#define	CS_CMD_CONF		0x02	/* Configure */
#define	CS_CMD_MCSET	0x03	/* Multi-Cast Setup */
#define	CS_CMD_XMIT		0x04	/* transmit */
#define	CS_CMD_TDR		0x05	/* Time Domain Reflectometer */
#define CS_CMD_DUMP		0x06	/* dump */
#define	CS_CMD_DGNS		0x07	/* diagnose */

/*
 * Build up the default configuration
 */

#define	CSMA_LEN		6		/* number of octets in addresses */
#define FIFO_LIM		8		/* DMA starts at this point */
#define IFGAP			96		/* Interframe gap */
#define SLOT_TIME		512		/* Slot time */
#define N_RETRY			15		/* # of re-tries if collision */
#define CRSF			0		/* Carrier Sense Filter */
#define CDTF			0		/* Intervals for collision detect */
#define CONF_LEN		12		/* Length of configuration params. */
#define LIN_PRI			0		/* Linear priority */
#define ACR				0		/* Accelerated contention resolution */
#define MIN_FRAME		64		/* Minimum frame length */

#define SCP_BUS_WIDTH	0x01	/* 8 bit bus */

#define MULTI_ADDR_CNT		16

/*
 * 82568 data structure definition
 *
 * NOTE: Only the first 16 bits of the physical addresses are set. These
 *       are the offset of the structure from the base of the shared memory
 *       segment. (0 - 0xffff)
 *	 
 */

/*
 * physical CSMA network address type
 */
typedef unsigned char	net_addr_t[CSMA_LEN];

/*
 *	System Configuration Pointer (SCP)
 *	NOTE: This is different for the 82596.
 */
typedef struct {
	ushort_t	scp_zeros;
	ushort_t 	scp_sysbus;		/* system bus width */
	ushort_t	scp_unused[2];	/* unused area */
	paddr_t		scp_iscp_paddr;	/* ISCP physical address */
} scp_t;

/*
 * Intermediate System Configuration Pointer (ISCP)
 */
typedef struct {
	ushort_t	iscp_busy;		/* 1 means 82596 is initializing */
	ushort_t	iscp_scb_ofst;	/* offset of the scb in the shared memory */
	paddr_t		iscp_scb_base;	/* base of shared memory */
} iscp_t;

/*
 * System Control Block	(SCB)
 */
typedef struct {
	ushort_t	scb_status;		/* STAT, CUS, RUS */
	ushort_t	scb_cmd;		/* ACK, CUC, RUC */
	ushort_t	scb_cbl_ofst;	/* CBL (Command Block List) offset */
	ushort_t	scb_rfa_ofst;	/* RFA (Receive Frame Area) offset */
#ifndef COMPAT_MODE
	ulong_t		scb_crc_err;	/* count of CRC errors. */
	ulong_t		scb_aln_err;	/* count of alignment errors */
	ulong_t		scb_rsc_err;	/* count of no resource errors */
	ulong_t		scb_ovrn_err;	/* count of overrun errors */
	ulong_t		scb_rcdt_err;	/* count of collisions detected */
	ulong_t		scb_shrt_err;	/* count of short frames */
	ushort_t	scb_ton_timer;	/* T-on timer */
	ushort_t	scb_toff_timer;	/* T-off timer */
#else
	ushort_t	scb_crc_err;	/* count of CRC errors. */
	ushort_t	scb_aln_err;	/* count of alignment errors */
	ushort_t	scb_rsc_err;	/* count of no resource errors */
	ushort_t	scb_ovrn_err;	/* count of overrun errors */
#endif
} scb_t;

/*
 * Configure command parameter structure
 */
typedef struct {
	ushort_t	cnf_fifo_byte;	/* BYTE CNT, FIFO LIM (TX_FIFO) */
	ushort_t	cnf_add_mode;	/* SRDY, SAV_BF, ADDR_LEN, AL_LOC, PREAM_LEN */
	ushort_t	cnf_pri_data;	/* LIN_PRIO, ACR, BOF_MET, INTERFRAME_SPACING */
	ushort_t	cnf_slot;		/* SLOT_TIME, RETRY NUMBER */
	ushort_t	cnf_hrdwr;		/* PRM, BC_DIS, MANCH/NRZ, TONO_CRS, NCRC_INS */
	ushort_t	cnf_min_len;	/* Min_FRM_LEN */
#ifndef COMPAT_MODE
	ushort_t	cnf_dcr_num;	/* DCR SLOT */
#endif
} conf_t;

/*
 * Transmit commad parameters structure
 */
typedef struct {
	ushort_t	xmt_tbd_ofst;	/* Transmit Buffer Descriptor offset */
#ifndef COMPAT_MODE
	ushort_t	xmt_tcb_count;	/* 82596 Transmit Command Block count */
	ushort_t	xmt_reserved;	/* 82596 reserved field */
#endif
	net_addr_t	xmt_dest;		/* Destination Address */
	ushort_t	xmt_length;		/* length of the frame */
} xmit_t;

/*
 * Dump command parameters structure
 */
typedef struct {
	ushort_t	dmp_buf_ofst;	/* dump buffer offset */
} dump_t;

#define	DL_MAC_ADDR_LEN	6
typedef struct mcat {
        unsigned char status;
        unsigned char entry[DL_MAC_ADDR_LEN];  /* Multicast addrs are 6 bytes */
} mcat_t;

typedef struct {
        ushort  mc_cnt;
        char    mc_addr[DL_MAC_ADDR_LEN * MULTI_ADDR_CNT];
} mcad_t;

/*
 * General Action Command structure
 */
typedef struct {
	ushort_t	cmd_status,		/* C, B, command specific status */
				cmd_cmd,		/* EL, S, I, opcode */
				cmd_nxt_ofst;	/* pointer to the next command block */
	union {
		xmit_t	prm_xmit;		/* transmit */
		conf_t	prm_conf;		/* configure */
		net_addr_t prm_ia_set;	/* individual address setup */
		mcad_t  prm_mcad;       /* Multicast address setup */
	} prmtr;
} cmd_t;

/*
 * Tramsmit Buffer Descriptor (TBD)
 */
typedef struct {
	ushort_t	tbd_count;		/* End Of Frame(EOF), Actual count(ACT_COUNT) */
	ushort_t	tbd_nxt_ofst;	/* offset of next TBD */
	ushort_t	tbd_buff;
	ushort_t	tbd_buff_base;
	ushort_t	sane_buff;
	ushort_t	reserved;
} tbd_t;

/*
 * Receive Buffer Descriptor
 */
typedef struct {
	ushort_t	rbd_status;		/* EOF, ACT_COUNT feild valid (F), ACT_COUNT */
	ushort_t	rbd_nxt_ofst;	/* offset of next RBD */
	ushort_t	rbd_buff;
	ushort_t	rbd_buff_base;
	ushort_t	rbd_size;		/* EL, size of the buffer */
	ushort_t	reserved;
	mblk_t		*rx_mp;
} rbd_t;

/*
 * Frame Descriptor (FD)
 */
typedef struct {
	ushort_t	fd_status;		/* C, B, OK, S6-S11 */
	ushort_t	fd_cmd;			/* End of List (EL), Suspend (S) */
	ushort_t	fd_nxt_ofst;	/* offset of next FD */
	ushort_t	fd_rbd_ofst;	/* offset of the RBD */
#ifndef COMPAT_MODE
	ushort_t	fd_actual_cnt;	/* 82596 Actual Count of data in FD */
	ushort_t	fd_size;		/* 82596 Size of buffer in FD */
#endif
	net_addr_t	fd_dest;		/* destination address */
	net_addr_t	fd_src;			/* source address */
	ushort_t	fd_length;		/* length of the received frame */
} fd_t;


/*
 * ring
 */
typedef struct ring {
	ushort_t ofst_cmd;
	struct ring	*next;
	mblk_t *xmit_mp;
} ring_t;

#define MAX_RAM_SIZE 0x2000

/*
 * The following is the board dependent structure
 */

typedef struct bdd {
	char		*virt_ram;
	ulong		ram_size;
	ushort_t	ofst_scb;
	ushort_t	gen_cmd;

	ushort_t	n_tbd;
	ushort_t	n_cmd;
	ushort_t	ofst_tbd;
	ushort_t	ofst_cmd;

	ring_t		*ring_buff;
	ring_t		*head_cmd;
	ring_t		*tail_cmd;

	ushort_t	n_fd;
	ushort_t	n_rbd;
	ushort_t	ofst_rbd;
	ushort_t	ofst_fd;

	fd_t		*begin_fd;
	fd_t		*end_fd;
	rbd_t		*begin_rbd;
	rbd_t		*end_rbd;

	mcat_t		i596_multiaddr[MULTI_ADDR_CNT];
} bdd_t;

#define MAC_HDR_LEN sizeof(net_addr_t) + sizeof(net_addr_t) + sizeof(ushort_t)

#define I596_TIMEOUT	3000000 /* 3 sec */

#define PRO_ON  1
#define PRO_OFF 0

#define LOOP_ON  1
#define LOOP_OFF 0

struct debug {
	int		ring_full;
	int		reset_count;
	int		q_cleared;
	int		rcv_restart_count;
	int		tx_cmplt_missed;
	int		tx_ringed;
	int		tx_done;
	int		tx_free_mp;
	int		tx_chained;
	int		wait_count;
};

#endif	/* _IO_DLPI_ETHER_I596_H */

