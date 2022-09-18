/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_IMX586_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_IMX586_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/imx586.h	1.6"
#ident	"$Header: $"

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

#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

/*
 * 82586 is the starlan controller chip
 */

/*
 * SCB STAT (we call it INT) bits
 * indicate the nature of an incoming interrupt from 82586
 */
#define	SCB_INT_MSK	0xf000	/* SCB STAT bit mask */
#define	SCB_INT_CX	0x8000	/* CX bit, CU finished a command with "I" set */
#define	SCB_INT_FR	0x4000	/* FR bit, RU finished receiving a frame */
#define	SCB_INT_CNA	0x2000	/* CNA bit, CU not active */
#define	SCB_INT_RNR	0x1000	/* RNR bit, RU not ready */

/* 
 * SCB Command Unit STAT bits
 */
#define	SCB_CUS_MSK	0x0700	/* SCB CUS bit mask */
#define	SCB_CUS_IDLE	0x0000	/* CU idle */
#define	SCB_CUS_SUSPND	0x0100	/* CU suspended */
#define	SCB_CUS_ACTV	0x0200	/* CU active */

/* 
 * SCB Receive Unit STAT bits
 */
#define	SCB_RUS_MSK	0x0070	/* SCB RUS bit mask */
#define	SCB_RUS_IDLE	0x0000	/* RU idle */
#define SCB_RUS_SUSPND	0x0010	/* RU suspended */
#define SCB_RUS_NORESRC 0x0020	/* RU no resource */
#define	SCB_RUS_READY	0x0040	/* RU ready */

/*
 * SCB ACK bits
 * these bits are used to acknowledge an interrupt from 82586
 */
#define SCB_ACK_MSK	0xf000	/* SCB ACK bit mask */
#define SCB_ACK_CX	0x8000	/* ACK_CX,  acknowledge a completed cmd */
#define SCB_ACK_FR	0x4000	/* ACK_FR,  acknowledge a frame reception */
#define	SCB_ACK_CNA	0x2000	/* ACK_CNA, acknowledge CU not active */
#define SCB_ACK_RNR	0x1000	/* ACK_RNR, acknowledge RU not ready */

/* 
 * SCB Command Unit commands
 */
#define	SCB_CUC_MSK	0x0700	/* SCB CUC bit mask */
#define	SCB_CUC_STRT	0x0100	/* start CU */
#define	SCB_CUC_RSUM	0x0200	/* resume CU */
#define	SCB_CUC_SUSPND	0x0300	/* suspend CU */
#define	SCB_CUC_ABRT	0x0400	/* abort CU */

/* 
 * SCB Receive Unit commands 
 */
#define SCB_RUC_MSK	0x0070	/* SCB RUC bit mask */
#define	SCB_RUC_STRT	0x0010	/* start RU */
#define	SCB_RUC_RSUM	0x0020	/* resume RU */
#define	SCB_RUC_SUSPND	0x0030	/* suspend RU */
#define	SCB_RUC_ABRT	0x0040	/* abort RU */

/*
 * SCB software reset bit
 */
#define SCB_RESET	0x0080	/* RESET, reset chip same as hardware reset */

/*
 * general defines for the command and descriptor blocks
 */
#define CS_CMPLT	0x8000	/* C bit, completed */
#define CS_BUSY		0x4000	/* B bit, Busy */
#define CS_OK		0x2000	/* OK bit, error free */
#define CS_ABORT	0x1000	/* A bit, abort */
#define CS_EL		0x8000	/* EL bit, end of list */
#define CS_SUSPND	0x4000	/* S bit, suspend */
#define CS_INT		0x2000	/* I bit, interrupt */
#define	CS_STAT_MSK	0x3fff	/* Command status mask */
#define CS_EOL		0xffff	/* set for fd_rbd_ofst on unattached FDs */
#define CS_EOF		0x8000	/* EOF (End Of Frame) in the TBD and RBD */
#define	CS_RBD_CNT_MSK	0x3fff	/* actual count mask in RBD */

/*
 * 82586 commands
 */
#define CS_CMD_MSK	0x07	/* command bits mask */
#define	CS_CMD_NOP	0x00	/* NOP */
#define	CS_CMD_IASET	0x01	/* Individual Address Set up */
#define	CS_CMD_CONF	0x02	/* Configure */
#define	CS_CMD_MCSET	0x03	/* Multi-Cast Setup */
#define	CS_CMD_XMIT	0x04	/* transmit */
#define	CS_CMD_TDR	0x05	/* Time Domain Reflectomete */
#define CS_CMD_DUMP	0x06	/* dump */
#define	CS_CMD_DGNS	0x07	/* diagnose */

/*
 * Definitions of the configuration parameters of the CSMA chip.
 * Note that in general it is not possible to change these
 * without corresponding kernel changes (re-compilation)
 */

#define CS_LEN_MSK	0x000f
#define CS_LEN_SHIFT	0

#define CSMA_MSK	0x0700 
#define CSMA_SHIFT	8

#define FIFO_MSK	0x0f00
#define FIFO_SHIFT	8

#define RDY_SYNC	0x0040		/* internal synchronization */
#define RDY_ASYNC	0x0000		/* external synch. */
#define RDY_MSK		0x0040

#define SAVE_BF		0x0080		/* Save bad frames */
#define NO_SAVE_BF	0x0000		/* Don't save them */
#define BF_MSK		0x0080

#define RFD_ADDR	0x0000		/* Addresses in frame descriptors */
#define BUF_ADDR	0x0800		/* Addresses in buffers */
#define ADDR_MSK	0x0800

#define PREAM_2		0x0000		/* Preamble length	 2 */
#define PREAM_4		0x1000		/* 			 4 */
#define PREAM_8		0x2000		/* 			 8 */
#define PREAM_16	0x3000		/* 			16 */
#define PREAM_MSK	0x3000

#define INT_LB		0x4000		/* Internal loopback */
#define EXT_LB		0x8000		/* External loopback */
#define NO_LB		0x0000		/* No loopback */
#define LB_MSK		0xc000

#define LIN_P_SHIFT	0
#define LIN_P_MSK	0x0007

#define ACR_SHIFT	4
#define ACR_MSK		0x0070

#define BOFF_802_3	0x0000		/* Backoff per IEEE 802.3 */
#define BOFF_OTHER	0x0080		/* Alternate method */
#define BOFF_MSK	0x0080

#define	IFGAP_MSK	0xff00	
#define IFGAP_SHIFT	8

#define SLOT_SHIFT	0
#define SLOT_MSK	0x07ff

#define RETRY_MSK	0xf000
#define RETRY_SHIFT	12

#define LIS_PROMISC	0x0001		/* Promiscuous listening */
#define LIS_SELECT	0x0000		/* Be selective */
#define LIS_MSK	0x0001

#define BCAST_DISABLE	0x0002		/* Disable broadcasts */
#define BCAST_ENABLE	0x0000		/* Permit broadcasts */
#define BCAST_MSK	0x0002

#define ENC_MANCH	0x0004		/* Manchester encoding */
#define ENC_NRZ		0x0000		/* NRZ encoding */
#define ENC_MSK	0x0004

#define XNOCS_ON	0x0008		/* Xmit even on no carr. sense */
#define XNOCS_OFF	0x0000		/* Stop xmit. on no carr. sense */
#define XNOCS_MSK	0x0008

#define CRC_INS		0x0000		/* Insert CRC */
#define NO_CRC_INS	0x0010		/* No CRC insertion */
#define CRC_INS_MSK	0x0010

#define CRC_AUTODIN	0x0000		/* Use Autodin II CRC */
#define CRC_CCITT	0x0020		/* Use CCITT CRC */
#define CRC_MSK	0x0020

#define BSTUFF_IEEE	0x0000		/* Use IEEE bit-stuffing */
#define BSTUFF_HDLC	0x0040		/* Use HDLC bit-stuffing */
#define BSTUFF_MSK	0x0040

#define PAD_ON		0x0080		/* Pad xmissions */
#define PAD_OFF		0x0000		/* Don't pad xmissions */
#define PAD_MSK	0x0080

#define CRSF_MSK	0x0700
#define CRSF_SHIFT	8

#define CDTF_MSK	0x7000
#define CDTF_SHIFT	12

#define CRS_INT		0x0800		/* Carrier sense internal */
#define CRS_EXT		0x0000		/* Carrier sense external */
#define CRS_MSK		0x0800

#define CDT_INT		0x8000		/* Collision detect internal */
#define CDT_EXT		0x0000		/* Collision detect external */
#define CDT_MSK		0x8000

#define MINF_SHIFT	0

/*
 * Build up the official STARLAN configuration
 */

#define	CSMA_LEN	6		/* number of octets in addresses */
#define FIFO_LIM	8		/* DMA starts at this point */
#define IFGAP		96		/* Interframe gap */
#define SLOT_TIME	512		/* Slot time */
#define N_RETRY		15		/* # of re-tries if collision */
#define CRSF		0		/* Carrier Sense Filter */
#define CDTF		0		/* Intervals for collision detect */
#define CONF_LEN	12		/* Length of configuration params. */
#define LIN_PRI		0		/* Linear priority */
#define ACR		0		/* Accelerated contention resolution */
#define MIN_FRAME	64		/* Minimum frame length */


#define CC_FIFO		((CONF_LEN << CS_LEN_SHIFT) | \
			 (FIFO_LIM << FIFO_SHIFT))

#define CC_ADD_MODE	( RDY_SYNC			| \
			  NO_SAVE_BF			| \
			  RFD_ADDR			| \
			  PREAM_8			| \
			  NO_LB				| \
			 (CSMA_LEN << CSMA_SHIFT)	  \
			)
			
#define CC_PRI_DATA	( BOFF_802_3			| \
			 (LIN_PRI << LIN_P_SHIFT)	| \
			 (ACR << ACR_SHIFT)		| \
			 (IFGAP << IFGAP_SHIFT)		  \
			)
			
#define CC_SLOT		((SLOT_TIME << SLOT_SHIFT)	| \
			 (N_RETRY << RETRY_SHIFT)	  \
			)
			
#define CC_HARDWARE	( LIS_SELECT			| \
			  BCAST_ENABLE			| \
			  ENC_MANCH			| \
			  XNOCS_ON			| \
			  CRC_INS			| \
			  CRC_AUTODIN			| \
			  BSTUFF_IEEE			| \
			  PAD_OFF			| \
			  CRS_EXT			| \
			  CDT_EXT			| \
			 (CRSF << CRSF_SHIFT)		| \
			 (CDTF << CDTF_SHIFT)		  \
			)
			
			  
#define CC_MINFRAME	((MIN_FRAME << MINF_SHIFT))

/*
 * A macro to help set up default configurations. Useage is
 *	conf_t conf = STARLAN_CONF;
 */
#define STARLAN_CONF	{\
	CC_FIFO,\
	CC_ADD_MODE,\
	CC_PRI_DATA,\
	CC_SLOT,\
	CC_HARDWARE,\
	CC_MINFRAME\
}

#define SCP_BUS_WIDTH	0x01		/* 8 bit bus */

#define MULTI_ADDR_CNT	16		/* Maximum number of mcast addresses */

/*
 * 82568 data structure definition
 *
 * NOTE: Only the first 13 bits of the physical addresses are set. These
 *       are the offset of the structure from the base of the shared memory
 *       segment. (0 - 0x1fff)
 *	 
 */

/*
 * physical CSMA network address type
 */
typedef unsigned char	net_addr_t[CSMA_LEN];

/*
 * System Configuration Pointer (SCP)
 *	This table reside in a fixed address on the memory (0xfffff6).
 *	The 82586 reads the bus width and address of the ISCP from
 *	this table.
 * 
 */
typedef struct {
	ushort 	scp_sysbus;	/* system bus width */
	ushort	scp_unused[2];	/* unused area */
	/*
	 * paddr_t	scp_iscp_paddr;	ISCP physical address
	 */
	ushort	scp_iscp;
	ushort scp_iscp_base;
} scp_t;

/*
 * Intermediate System Configuration Pointer (ISCP)
 */
typedef struct {
	ushort	iscp_busy;	/* 1 means 82586 is initializing */
				/* only the first 8 bit are used */
	ushort	iscp_scb_ofst;	/* offset of the scb in the shared memory */
	paddr_t	iscp_scb_base;	/* base of shared memory */
} iscp_t;

/*
 * System Control Block	(SCB)
 */
typedef struct {
	ushort	scb_status;	/* STAT, CUS, RUS */
	ushort	scb_cmd;	/* ACK, CUC, RUC */
	ushort	scb_cbl_ofst;	/* CBL (Command Block List) offset */
	ushort	scb_rfa_ofst;	/* RFA (Receive Frame Area) offset */
	ushort	scb_crc_err;	/* count of CRC errors. */
	ushort	scb_aln_err;	/* count of alignment errors */
	ushort	scb_rsc_err;	/* count of no resource errors */
	ushort	scb_ovrn_err;	/* count of overrun errors */
} scb_t;

/*
 * Configure command parameter structure
 */
typedef struct {
	ushort	cnf_fifo_byte;	/* BYTE CNT, FIFO LIM (TX_FIFO) */
	ushort	cnf_add_mode;	/* SRDY, SAV_BF, ADDR_LEN, AL_LOC, PREAM_LEN */
				/* INT_LPBCK, EXT_LPBK */
	ushort	cnf_pri_data;	/* LIN_PRIO, ACR, BOF_MET, INTERFRAME_SPACING */
	ushort	cnf_slot;	/* SLOT_TIME, RETRY NUMBER */
	ushort	cnf_hrdwr;	/* PRM, BC_DIS, MANCH/NRZ, TONO_CRS, NCRC_INS */
				/* CRC, BT_STF, PAD,CRSF,CRS_SRC,CDTF,CDT_SRC */
	ushort	cnf_min_len;	/* Min_FRM_LEN */
} conf_t;

/*
 * Transmit commad parameters structure
 */
typedef struct {
	ushort	xmt_tbd_ofst;	/* Transmit Buffer Descriptor offset */
	net_addr_t xmt_dest;	/* Destination Address */
	ushort	xmt_length;	/* length of the frame */
} xmit_t;

/*
 * Dump command parameters structure
 */
typedef struct {
	ushort	dmp_buf_ofst;	/* dump buffer offset */
} dump_t;

/* Multicast address set up command structure	*/

#define	DL_MAC_ADDR_LEN	6 /* defined in dlpi_ether.h */

typedef struct {
	ushort	mc_cnt;
	char	mc_addr[DL_MAC_ADDR_LEN * MULTI_ADDR_CNT];
} mcad_t;



/*
 * General Action Command structure
 */
typedef struct {
	ushort	cmd_status,		/* C, B, command specific status */
		cmd_cmd,		/* EL, S, I, opcode */
		cmd_nxt_ofst;		/* pointer to the next command block */
	union {
		dump_t	prm_dump;	/* dump */
		xmit_t	prm_xmit;	/* transmit */
		conf_t	prm_conf;	/* configure */
		net_addr_t prm_ia_set;	/* individual address setup */
		mcad_t	prm_mcad;	/* Multicast address setup */
	} prmtr;	/* parameters */
} cmd_t;

/*
 * Tramsmit Buffer Descriptor (TBD)
 */
typedef struct {
	ushort	tbd_count;	/* End Of Frame(EOF), Actual count(ACT_COUNT) */
	ushort	tbd_nxt_ofst;	/* offset of next TBD */
	/*
	 * paddr_t	tbd_buff;	buffer address
	 */
	ushort	tbd_buff;
	ushort tbd_buff_base;
} tbd_t;

/*
 * Receive Buffer Descriptor
 */
typedef struct {
	ushort	rbd_status;	/* EOF, ACT_COUNT feild valid (F), ACT_COUNT */
	ushort	rbd_nxt_ofst;	/* offset of next RBD */
	/*
	 * paddr_t	rbd_buff;	buffer address
	 */
	ushort	rbd_buff;
	ushort rbd_buff_base;
	ushort	rbd_size;	/* EL, size of the buffer */
} rbd_t;

/*
 * Frame Descriptor (FD)
 */
typedef struct {
	ushort	fd_status;	/* C, B, OK, S6-S11 */
	ushort	fd_cmd;		/* End of List (EL), Suspend (S) */
	ushort	fd_nxt_ofst;	/* offset of next FD */
	ushort	fd_rbd_ofst;	/* offset of the RBD */
	net_addr_t fd_dest;	/* destination address */
	net_addr_t fd_src;	/* source address */
	ushort	fd_length;	/* length of the received frame */
} fd_t;

#define	MAC_HDR_LEN	(2 * sizeof(net_addr_t) + sizeof(ushort_t))

/*
 * dump buffer definition
 *
 * Dump Command does not work on "standard" STARLAN due to bug in
 * B-step version of 82586. May / may not work in other versions.
 * See Intel 82586 documentation / errata sheets for more details
 */
typedef struct {
ushort
	dmp_fifo_byte,		/* same as in conf. cmd, except for */
	dmp_add_mode,		/* bit 6 of fifo_byte */
	dmp_pri_data,
	dmp_slot,
	dmp_hware,
	dmp_min_len,
	dmp_iar10,		/* Individual address bytes */
	dmp_iar32,
	dmp_iar54,
	dmp_last_stat,		/* status word of last cmd */
	dmp_txcr10,		/* xmit CRC generator */
	dmp_txcr32,
	dmp_rxcr10,		/* rcv CRC generator */
	dmp_rxcr32,
	dmp_tmp10,		/* Internal temporaries */
	dmp_tmp32,
	dmp_tmp54,
	dmp_rsr,		/* receive status register */
	dmp_hash10,		/* Hash table */
	dmp_hash32,
	dmp_hash54,
	dmp_hash76,
	dmp_last_tdr,		/* Status of last TDR command */
	dmp_fill [8],		/* Mostly 0 (!) */
	dmp_addr_len,
	dmp_n_rb_sz,		/* Size of next receive buffer */
	dmp_n_rb_hi,		/* High byte of next RB address */
	dmp_n_rb_lo,		/* Lo byte of    "   "     "    */
	dmp_c_rb_sz,		/* # of bytes in last buff used */
	dmp_la_rbd,		/* Look ahead buff. des. (N+2) */
	dmp_n_rbd,		/* Next RBD address */
	dmp_c_rbd,		/* Current (last filled) rbd */
	dmp_c_rb_ebc,		/* current rb empty byte count */
	dmp_n_fd_addr,		/* next + 1 free frame descriptor */
	dmp_c_fd_addr,		/* next free frame descriptor */
	dmp_tmp,
	dmp_n_tb_cnt,		/* last tb cnt of completed cmd */
	dmp_d_buf_addr,		/* Address of buffer in dump cmd */
	dmp_n_tb_addr,		/* Next Xmit buff address */
	dmp_l_tbd_addr,		/* next xmit buff descriptor */
	dmp_n_tbd_addr,		/* current xmit buff descriptor */
	dmp_tmp1,
	dmp_n_cb_addr,		/* next command block address */
	dmp_c_cb_addr,		/* current cmd blk address */
	dmp_tmp2,
	dmp_scb_addr,		/* Address of SCB */
	dmp_fill2 [6],
	dmp_fifo_lim,
	dmp_fill3 [3],
	dmp_rus_req,
	dmp_cus_req,
	dmp_rus,
	dmp_fill4 [6],
	dmp_buff_hi,		/* High address of dump buffer */
	dmp_buff_lo,		/* Lo address of dump buffer */
	dmp_dma_bc,		/* Receive dma byte count */
	dmp_br_buff,		/* Base + buffer */
	dmp_r_dma_hi,		/* receive dma address */
	dmp_r_dma_lo,		/*   "	    "	  "    */
	dmp_fill5 [7];
} dumpbuf_t;

/*
 * Board Specific #defines
 */

#define OFFSET_NORMMODE 0x3000  /* 0=esi loopback, 1 normal data xfer */
#define OFFSET_CHAN_ATT 0x3002  /* 0=clear 586 channel attention, 1 = set */
#define OFFSET_RESET    0x3004  /* 0=clear 586 h/w reset, 1=set */
#define OFFSET_INT_ENAB 0x3006  /* 0=disable board interrupts 1=enable */
#define OFFSET_16B_XFER 0x3008  /* 0=8bit xfer, 1=16bit xfer (for at) */
#define OFFSET_SYS_TYPE 0x300a  /* 0=pc or pc/xt, 1=at */
#define OFFSET_INT_STAT 0x300c  /* 0=board's interrupt active, 1=inactive */ 
#define OFFSET_ADDR_PROM 0x2000 /* first byte of on-board ethernet id */

#define EXTENDED_ADDRESS 0x20000 /* used when board addr is above 1 meg */
#define OFFSET_SCP      0x7ff6  /* 586's scp points to the iscp */
#define OFFSET_ISCP     0x7fee  /* this points to the system control block */
#define OFFSET_SCB      0x7fde  /* points to rcv frame and command unit areas */
#define OFFSET_RU       0x4000  /* the RAM for frame descriptors, receive
                                   buffer descriptors & rcv buffers is fixed at
                                   0x4000 to 0x7800 on the half-card */
#define OFFSET_RBD	0x4228	/* rbd+rbuf must start on 32 bit boundry */
#define OFFSET_CU       0x7814  /* RAM offset for CBLs,  TBDs, etc  
                                   xmt area is from 0x7814 to 0x7f00 
                                   from 0x7f01 to 0x7fff is scp, iscp, scb */ 
#define OFFSET_TBD      0x7914   /* allows 256 bytes for command block */
#define OFFSET_TBUF     0x7924   /* start of user data to send*/
#define N_FD              25     /* 856 frame descriptor */
#define N_RBD             25     /* 586 rcv buffer descriptor */
#define RCVBUFSIZE       532     /* 532 * 2 = max size used by tcp */ 
#define DL_DEAD         0xffff  /* suppliments 82586.h datalink states */

#define CMD_0           0       /* used on 586 half-card command registers */
#define CMD_1           0xffff  /* the mate of the above */
#define TRUE            1     
#define FALSE           0

/* The following definitions are used to gather statistics for the imx driver */

#define 	COLLMASK		0x000F
#define		MAXCOLMASK		0x0020
#define		DEFERMASK		0x0080
#define		UNDERRUNMASK		0x0100
#define		OUT_OF_RESOURCES	0x0200
#define		NOCRSMASK		0x0400



/*
 * The following is the board specific structure
 */

typedef struct mcat{
	unsigned char status;
	unsigned char entry[DL_MAC_ADDR_LEN];      /* multicast addresses are 6 bytes */
}mcat_t;

typedef struct bdd {
	int	round_robin;  /* used by interrupt() get xmt packet */
	fd_t	*begin_fd, *end_fd; /* keeps tabs of fd for host cpu */
	rbd_t	*begin_rbd, *end_rbd; /* ditto, see rcv_ & xmt_packet*/
	char	*p_virt_cmd_prom; /* virtual address of above */
	char	*p_static_ram; /* phys addr of 16KB static ram */
	char	*p_virt_static_ram; /* virtual address of above */
	ushort_t	cnf_fifo_byte;
	ushort_t	cnf_add_mode;
	ushort_t	cnf_pri_data;
	ushort_t	cnf_slot;
	ushort_t	cnf_hrdwr;
	ushort_t	cnf_min_len;
	mcat_t	imx586_multiaddr[MULTI_ADDR_CNT];
} bdd_t;


/*
 * All accesses to RAM on the IMXLAN586 board MUST be 16-bit. The following
 * structure is used to pack and unpack unsigned longs.
 */

typedef struct {
	union {
		char		a[4];
		unsigned long	b;
	}
	c;
} pack_ulong_t;


/*
 * Since the PC AT has 16 interrupt levels, a table will be built
 * with the 586 half-card interrupts in it for quick reference during
 * interrupt service routine time. note: 12 entries will be 0xffff the
 * other four will be either 1, 2, 3 or 4. 0xffff means no board at this
 * level.
 */
/* extern int interrupt_586[16]; ...declared in Space.c now*/

#endif /* _IO_DLPI_ETHER_IMX586_H */
