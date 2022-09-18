/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_TOKEN_IBMTOK_H
#define	_IO_DLPI_TOKEN_IBMTOK_H

#ident	"@(#)uts-x86:io/dlpi_token/ibmtok.h	1.8"
#ident	"$Header: $"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define VPKTSZ		(3*256)
#define HIWAT		(32*VPKTSZ)
#define LOWAT		(8*VPKTSZ)
#define MAXPKTLLC	1497	/* max packet when using LLC1 */
#define MAXDPKT		MAXPKTLLC	/* Maximum data size */
#define MAXPKT		(MAXDPKT+(2*LLC_EHDR_SIZE)+MAX_ROUTE_FLD) 
#define MINSEND		1

#define MAC_ADDR_LEN		6	/* length of 802(.3/.4/.5) address */

struct tokdev {
   unsigned short    tok_flags;	   /* flags to indicate various status' */
   unsigned short    tok_type;	   /* LLC or Ether */
   queue_t	     *tok_qptr;	   /* stream associated with open device */
   unsigned short    tok_state;    /* state variable for DL_INFO */
   unsigned short    tok_sap;	   /* sap or ethertype depending on itok_type */
   unsigned short    tok_no;	   /* index number from front of array */
   struct tokparam   *tok_macpar;  /* board specific parameters */
   struct tokstat    *tok_stats;   /* driver and board statistics */
   struct tokdevstat *tok_dstats;  /* SAP statistics */
   unsigned short    tok_snap[3];  /* for SNAP and other extentions */
   unsigned short    tok_rws;	   /* receive window size - for LLC2 */
   unsigned short    tok_sws;	   /* send window size - for LLC2 */
   unsigned short    tok_rseq;	   /* receive sequence number - for LLC2 */
   unsigned short    tok_sseq;	   /* send sequence number - for LLC2 */
   unsigned short    tok_station_id;
};

/* status bits */
#define S_OPEN	0x01	/* minor device is opened */
#define S_PROM	0x02	/* promiscuous mode enabled */
#define S_XWAIT	0x04	/* waiting to be rescheduled */
#define S_SU	0x80	/* opened by priviledged user */
#define S_RWAIT	0x100	/* waiting for read reschedule */

struct tokparam {
   short    tok_index;		/* board index */
   short    tok_int;		/* interrupt vector number */
   short    tok_ioaddr;		/* I/O port for device */
   short    tok_minors;		/* number of minor devices allowed */
   short    tok_chan;		/* DMA channel */
   short    tok_major;		/* major device number */
   int      tok_noboard;	/* board present flag.                 */
   int	    tok_init;		/* board status.                       */
   unsigned ram_addr;           /* Start of shared memory area.        */
   unsigned bios_addr;          /* Where the bios starts.              */
   ushort  tok_adapter_state;
/* Stuff above this line is initialized in /etc/conf/pack.d/tok/space.c */
   addr_t   ram_area;           /* The whole shared RAM area.          */
   addr_t   aca_area;           /* Memory mapped control area.         */
                                /* This is the place that contains     */
                                /* all the memory mapped registers !!! */
   addr_t   aip_area;           /* Memory mapped identification area.  */
   addr_t   SRB_area;           /* Ptr. to our request block.          */
   addr_t   ASB_area;           /* Ptr. to the adapter status block.   */
   addr_t   ARB_area;           /* Ptr. to the adapter request block.  */
   addr_t   SSB_area;           /* Ptr. to our status block.           */
   struct aip aip;              /* Holding place for adapter info.     */
   short    intr_refresh;       /* Address of intr. re-strobe.         */
   short    station_id;         /* Returned from SAP OPEN.             */
   short    tok_ring_num;	/* Ring number of this board.          */
   ushort   segment_num;	/* Ring Number, Bridge number.         */
   int	    tok_str_open;	/* number of streams open.             */
   int	    tok_maxpkt;		/* Maximum size of transmit packet.    */
   long	    tok_nextq;		/* next queue to be scheduled.         */
   mblk_t   *cur_msg;           /* Current message.                    */
   long	    tok_proms;		/* number of promiscuous streams */
   short    tok_firstd;		/* first minor device for this major */
   unchar   tok_macaddr[MAC_ADDR_LEN];		/* machine address */
   int	    tok_multicnt;	/* number of defined multicast/group addrs */
   unchar   tok_multiaddr[MAC_ADDR_LEN];	/* multicast address */
   unchar   tok_groupaddr[MAC_ADDR_LEN];	/* group address */
   ushort   snap_station_id;
   ushort   snap_bound;
   ushort   sched_flags;
   ushort   tok_reset_flag;
   ushort   tok_open_flag;
   ushort   tok_close_flag;
   ushort   tok_timer_val;
   ulong    tok_ram_size;
   ushort   tok_timeouts;
   ushort   tok_reset_retries;
};

struct tokconfig {
  int ioaddr;
  int irq;
  int mem;
};

/* Debug Flags and other info */

#define SYSCALL	0x01		/* trace syscall functions */
#define PUTPROC	0x02		/* trace put procedures */
#define SRVPROC	0x04		/* trace service procedures */
#define RECV	0x08		/* trace receive processing */
#define RCVERR	0x10		/* trace receive errors */
#define DLPRIM	0x20		/* trace DL primitives */
#define INFODMP	0x40		/* dump info req data */
#define DLPRIMERR	0x80		/* mostly llccmds errors */
#define DLSTATE	0x100		/* print state chages */
#define TRACE	0x200		/* trace loops */
#define INTR	0x400		/* trace interrupt processing */
#define BOARD	0x800		/* trace access to the board */
#define LLC1	0x1000		/* trace llc1 processing */
#define SEND	0x2000		/* trace sending */
#define BUFFER	0x4000		/* trace buffer/canput fails */
#define SCHED	0x8000		/* trace scheduler calls */
#define XTRACE	0x10000		/* trace itoksend attempts */
#define INIT	0x20000		/* trace initilization*/
#define ERROR	0x40000		/* trace errors and lost packets */
#define SRCROUT	0x80000		/* trace packets with routing info */

/* define llc class 1 and mac structures and macros */

struct tok_machdr {
	unsigned char mac_dst[6];
	unsigned char mac_src[6];
	unsigned char ibm_route[18];
};

union llc_header {
	struct llctype {
		unsigned char	llc_dsap;
		unsigned char	llc_ssap;
		unsigned char	llc_control;
		unsigned char	llc_info[3];
	}llc_sap;
	struct llc_snap {
		unsigned char	llc_dsap;
		unsigned char	llc_ssap;
		unsigned char	llc_control;
		unsigned char	org_code[3];
		unsigned short	ether_type; 
		unsigned char	llc_info[3];
	}llc_snap;
	unsigned char ibm_route[18];
};

struct mac_llc_hdr {
	unsigned char mac_dst[6];
	unsigned char mac_src[6];
	union llc_header llc;
};

struct llc_info {
	ushort	ssap;			/* Source SAP */
	ushort	dsap;			/* Destination SAP */
	ushort	snap;			/* SNAP field */
	ushort	control;		/* LLC control Field */
	struct	tok_machdr *mac_ptr;	/* Pointer to the MAC header */
	union	llc_header *llc_ptr;	/* Pointer to the LLC header */
	unchar	*data_ptr;		/* Pointer to first byte of data */
	ushort	rsize;			/* Number of byte in routing field */
	ushort	b_cast;			/* Broadcast field if routing field */
	ushort	direction;		/* Direction bit */
	ushort	lf;			/* Size of longest frame */
};

typedef struct tok_machdr machdr_t;

#define LLC_SAP_LEN		1	/* length of sap only field */
#define LLC_LSAP_LEN		2	/* length of sap/type field  */
#define	LLC_CNTRL_LEN		1	/* Length of control field */
#define LLC_SNAP_LEN		5	/* Length of LLC SNAP fields */
#define LLC_SAP_H_LEN		3	/* Length of LLC SAP header */
#define LLC_SNAP_H_LEN		5	/* Length of LLC SNAP header */

/* Length of MAC address fields */
#define MAC_HDR_SIZE	(MAC_ADDR_LEN+MAC_ADDR_LEN)

/* Length of LAN header size (minimum). */
#define MIN_LAN_HDR_SIZE 0x0E

/* Length of 802.2 LLC Header */
#define LLC_HDR_SIZE	(MAC_HDR_SIZE+LLC_SAP_LEN+LLC_SAP_LEN+LLC_CNTRL_LEN)

/* Length of extended LLC header with SNAP fields */
#define LLC_EHDR_SIZE	(LLC_HDR_SIZE + LLC_SNAP_LEN)

/* Length of LLI address message fields */
#define LLC_LIADDR_LEN		(MAC_ADDR_LEN+LLC_SAP_LEN)
#define LLC_ENADDR_LEN		(MAC_ADDR_LEN+LLC_LSAP_LEN)
#define	LLC_ENR_MAX_LEN		(LLC_ENADDR_LEN+MAX_ROUTE_FLD)
#define	LLC_LIR_MAX_LEN		(LLC_LIADDR_LEN+MAX_ROUTE_FLD)

#define	MAX_ROUTE_FLD	18	/* Maximum of 18 bytes of routing info */

union llc_bind_fmt {
   struct llca {
      unsigned char  lbf_addr[MAC_ADDR_LEN];
      unsigned short lbf_sap;
   } llca;
   struct llcb {
      unsigned char  lbf_addr[MAC_ADDR_LEN];
      unsigned short lbf_sap;
      unsigned long  lbf_xsap;
      unsigned long  lbf_type;
   } llcb;
   struct llcc {
      unsigned char lbf_addr[MAC_ADDR_LEN];
      unsigned char lbf_sap;
   } llcc;
};

#define MAXSAPVALUE	0xFF	/* largest LSAP value */

#define	DL_802		0x01	/* for type field 802.2 type packets */
#define	DL_SNAP		0x02	/* for type field 802.2 SNAP type packets */
#define DL_ROUTE	0x04	/* to indicate IBM routing field */
#define DL_RESPONSE	0x08	/* to indicate a response type packet */

/* Source routing defines */

/* Broadcast bits */

#define S_NB_NB		0x0	/* Non-Broadcast packet */
#define S_AR_NB		0x100	/* All-Routes broadcast, Non-broadcast return */
#define S_SR_NB		0x110	/* Single-Route , Non-broadcast return */
#define S_SR_AR		0x111	/* Single-Route , All-Routes return */

/* Direction bit */

#define S_LTOR		0	/* Parse from Left to Right */
#define S_RTOL		1	/* Parse from Right to Left */

/* Largest Frame Size */

#define	S_516	0x0000	/* Up to 516 octects in the information field */
#define	S_1500	0x0010	/* Up to 1500 octects in the information field */
#define	S_2052	0x0100	/* Up to 2052 octects in the information field */
#define	S_4472	0x0110	/* Up to 4472 octects in the information field */
#define	S_8191	0x1000	/* Up to 8191 octects in the information field */

#define	S_MYMAX	S_1500	/* This is the MAX of this bridge */

/* Structure to hold segment number information. 
 * Segment number filed holds the Ting number and the Bridge number.
 *
 * 	| <-------- 16 bits ---------> |
 * 	|------------------------------|
 *	|      RN        |     BN      |
 *	|----------------|-------------|
 *	| <-- 12 bits -->|<- 4 bits -->|
 */

struct seg_num {
	ushort segment_num;		/* Ring Number, Bridge number */
	struct tokparam *b_info;	/* Pointer to board info */
}; 

/* other useful macros */

#define HIGH(x) ((x>>8)&0xFF)
#define LOW(x)	(x&0xFF)

/* recoverable error conditions */

#define E_OK		0	/* normal condition */
#define E_NOBUFFER	1	/* couldn't allocb */
#define E_INVALID	2	/* operation isn't valid at this time */
#define E_BOUND		3	/* stream is already bound */
#define E_BLOCKED	4	/* blocked at next queue */
#define	E_NOINIT	5	/* Couldn't init/open token card */

/* LLC specific data - should be in separate header (later) */

#define LLC_UI		0x03	/* unnumbered information field */
#define LLC_XID		0xAF	/* XID with P == 0 */
#define LLC_TEST	0xE3	/* TEST with P == 0 */

#define LLC_P		0x10	/* P bit for use with XID/TEST */
#define LLC_XID_FMTID	0x81	/* XID format identifier */
#define LLC_SERVICES	0x80	/* Services supported */
#define LLC_GLOBAL_SAP	0XFF	/* Global SAP address */
#define LLC_NULL_SAP	0X00	/* NULL SAP address */
#define LLC_SNAP_SAP	0xAA	/* SNAP SAP */
#define LLC_GROUP_ADDR	0x01	/* indication in DSAP of a group address */
#define LLC_RESPONSE	0x01	/* indication in SSAP of a response */

#define LLC_XID_INFO_SIZE	3 /* length of the INFO field */

/* event statistics */

struct tokstat {
    /* non-hardware */
    ulong	toks_nobuffer;	/* 1 */
   /* transmit */
   ulong	toks_xpkts;	/* 2 */
   ulong	toks_xbytes;	/* 3 */
   /* receive */
   ulong	toks_rpkts;	/* 4 */
   ulong	toks_rbytes;	/* 5 */
   ulong	toks_intrs;	/* 6 */
};

/* SAP statistics */
struct tokdevstat {
   /* transmit */
   ulong	toks_xpkts;	/* 1 */
   ulong	toks_xbytes;	/* 2 */
   /* receive */
   ulong	toks_rpkts;	/* 3 */
   ulong	toks_rbytes;	/* 4 */
};

/* datalink layer i/o controls */
#define DLIOC		('D'<<8)
#define DLGBROAD	(('D' << 8) | 3)  /* get broadcast address entry */
#define DLGDEVSTAT	(('D' << 8) | 4)  /* get statistics for the board*/
#define DLGADDR		(('D' << 8) | 5)  /* get physical addr of interface */
#define DLPROM		(('D' << 8) | 6)  /* toggle promiscuous mode */
#define DLSADDR		(('D' << 8) | 7)  /* set physical addr of interface */
#define DLGCLRSTAT	(('D' << 8) | 8)  /* get statistics and zero entries */
#define DLSMULT		(('D' << 8) | 9)  /* set multicast address entry */
#define DLRESET		(('D' << 8) | 10) /* reset to power up condition */
#define DLGSAP		(('D' << 8) | 11) /* get driver sap value */
#define DLGMULT		(('D' << 8) | 12) /* get multicast address entry */
#define DLGBRDTYPE	(('D' << 8) | 13) /* get board type */
#define DLDMULT 	(('D' << 8) | 14) /* delete multicast address entry */
#define DLDIAG	 	(('D' << 8) | 15) /* Set diagnostic level */
#define DLGSTAT		(('D' << 8) | 16) /* get statistics for a sap*/

/* Ioctl's for Lachman */
#define MACIOC(x)	(('M' << 8) | (x))
#define MACIOC_DIAG	MACIOC(1)	/* Set diagnostic level */
#define MACIOC_UNIT	MACIOC(2)	/* Unit Select */
#define MACIOC_SETMCA	MACIOC(3)	/* multicast setup */
#define MACIOC_DELMCA	MACIOC(4)	/* multicast delete */
#define MACIOC_DELAMCA	MACIOC(5)	/* flush multicast table */
#define MACIOC_GETMCA	MACIOC(6)	/* get multicast table */
#define MACIOC_GETSTAT	MACIOC(7)	/* dump statistics */
#define MACIOC_GETADDR	MACIOC(8)	/* get mac address */
#define MACIOC_SETADDR	MACIOC(9)	/* set mac address */

/* The following structures define generic command block structures employed
   by the token ring driver to process individual commands
*/

typedef struct {
        unsigned char ssap;
	unsigned short tok_no;
} srb_open_sap_t;

typedef struct {
	unsigned short station_id;
	unsigned short tok_no;
} srb_close_sap_t;

typedef struct {
	unsigned short station_id;
	unsigned short lan_hdr_size;
	unsigned char dsap;
	unsigned char correlator;
} srb_tx_ui_frame_t;

/* srb_cmd should be one of SRB_TX_UI_FRAME, SRB_OPEN_SAP or SRB_CLOSE_SAP */

typedef struct {
	unsigned short srb_cmd;
	union {
                srb_open_sap_t		open_sap;
		srb_close_sap_t		close_sap;
		srb_tx_ui_frame_t	xmit_frame;
	} srb_cmd_param;
}srb_cmd_t;

/* The following definitions identify the states of the adapter as it pertains
   to the availability of the transmission buffer
*/

#define 	TX_QUEUED		0x08
#define 	TX_BUSY			0x04

/* The following definitions list the various states of the adapter */
  
#define		ADAPTER_DISABLED	0x0
#define 	ADAPTER_RESETTING	0x1
#define		ADAPTER_CLOSED		0x2
#define		ADAPTER_OPENING		0x3
#define		ADAPTER_OPENED		0x4
#define		ADAPTER_CLOSING		0x5

#define		ADAP_ID		0xE001
#define		ADAP_ENAB	0x96
#define		POS_LSB		0x100
#define		POS_MSB		0x101
#define		POS_0		0x102
#define		POS_1		0x103
#define		POS_2		0x104
#define		POS_3		0x105

#define		TOK_TIMEOUT	3000000
#define		TOK_MAX_TIMEOUTS	5

#define 	ADAPTER_OPEN_FAILED	0x1
#define 	ADAPTER_OPEN_TIMEOUT	0x2
#define		ADAPTER_RESET_TIMEOUT	0x3
#define		ADAPTER_RESET_FAILED	0x4

#define		TOK_MAX_RETRIES		4

/* 
   DLPI version 2 allows implementation dependent error codes. Since some
   of the token ring errors cannot be directly mapped into dlpi error codes,
   new error codes are obtained by directly offsetting the retcodes from 
   from various commands from the base 0x80.
*/

#define 	DLPI_TOKEN_ERROR_BASE	0x80

#endif
