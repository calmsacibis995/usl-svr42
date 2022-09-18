/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_TOKEN_IBMTOKHW_H
#define	_IO_DLPI_TOKEN_IBMTOKHW_H

#ident	"@(#)uts-x86:io/dlpi_token/ibmtokhw.h	1.6"
#ident	"$Header: $"

/* Misc. constants. */
#define FALSE                               0
#define TRUE                                1
#define MAX_BUFFER_LENGTH                 256
#define MAX_NBR_TX_BUFFERS                  1
#define BIOS_ADDR_SIZE                      2
#define IBM_CHANNEL_IDENTIFIER "PICO6110990 "
#define CHANNEL_IDENTIFIER_LENGTH          12      /* Bytes. */
#define K16                                16384
#define K64				   65536
#define ZERO_OUT_START_ADDR                65024   /* 64k - 512 bytes. */
#define ZERO_OUT_LENGTH                      512   /* Bytes.           */
/* End of misc. constants. */

/* Misc. types. */
/* End of misc. types. */

/* BIOS address calculation constants. */
#define LOWEST_BIOS_ADDR       0xC0000     /* Lowest possible addr.      */
#define BIOS_ADDR_WITHIN_RANGE 0xC0        /* Get the 2 first BIOS bits. */
#define GET_BIOS_BITS          0x3C        /* Get the last 4 bits.       */
#define CALC_BIOS_ADDR           11        /* Leftshift with this much.  */
#define BIOS_LENGTH            0x2000      /* ROM size.                  */
/* End of BIOS constants. */

/* I/O offsets on the adapter. */
#define RESET_LATCH_OFFSET   1
#define RESET_RELEASE_OFFSET 2
/* End of I/O addresses. */

/* Values for I/O addresses. */
#define RESET_VALUE   0x01
#define RELEASE_VALUE 0x01
/* End of values for I/O addresses. */

/* Relative memory mapped area locations and offsets. */
#define BIOS_AREA_OFFSET 0x0000  /* BIOS code indication offset in the BIOS.  */
#define ACA_AREA_OFFSET  0x1E00  /* ACA area offset in the BIOS.              */
#define AIP_AREA_OFFSET  0x1F00  /* AIP area offset in the BIOS.              */
#define BIOS_SIZE        2       /* Length of the BIOS_OFFSET area.           */
                                 /* (Just get the first 2 byte : 0x55aa.)     */
#define ACA_AREA_SIZE  128       /* Length of the ACA OFFSET area.            */
#define AIP_AREA_SIZE  255       /* Length of the AIP OFFSET area.            */

#define ASB_ADDRESS_OFFSET  0x08 /* Start of Adapter Status Block addr offset.*/
#define SRB_ADDRESS_OFFSET  0x0A /* ...      System Request Block  ...        */
#define ARB_ADDRESS_OFFSET  0x0C /* ...      Adapter Request Block ...        */
#define SSB_ADDRESS_OFFSET  0x0E /* ...      System Status Block   ...        */

#define ASB_AREA_SIZE       0x12 /* Adapter Status Block size (bytes).        */
#define SRB_AREA_SIZE       0x1C /* Adapter Status Block size (bytes).        */
#define ARB_AREA_SIZE       0x1C /* Adapter Status Block size (bytes).        */
#define SSB_AREA_SIZE       0x14 /* Adapter Status Block size (bytes).        */

/*
 *	Bit positions for options field
 */

#define	OP_WRAP		0x8000;	/* Wrap Interface.            */
#define	OP_DHE		0x4000;	/* Disable Hard Error.        */
#define	OP_DSE		0x2000;	/* Disable Soft Error.        */
#define	OP_PADMF	0x1000;	/* Pass Adapter MAC Frames.   */
#define	OP_PATMF	0x0800;	/* Pass Attention MAC Frames. */
#define	OP_CONT		0x0100;	/* Contender.                 */
#define	OP_PBMF		0x0080;	/* Pass Beacon MAC Frames.    */
#define OP_RPL          0x0020; /* Remote Program Load.       */
#define OP_REL          0x0010; /* Early token release.       */

/*
 *	OPEN Constants for parameter block defined above
 */

#define OPEN_COPY_ALL	0x0200		/* Copy all non MAC frames bit.   */
#define OPEN_MAX_GSAP        0          /* Max. number of group SAP's.    */
#define OPEN_MAX_GMEM        0          /* Max. number of SAP's in group. */
/* All following timer tick values are multiples of 40 ms.                */
/* OPEN_xx_TICK_1 's are values (1 - 5), OPEN_xx_TICK_2's are (6 - 10).   */
#define OPEN_T1_TICK_1       5          /* DLC T1 timer tick.             */
#define OPEN_T2_TICK_1       1          /* DLC T2 time between ticks.     */
#define OPEN_Ti_TICK_1      25          /* DLC Ti time between ticks.     */
#define OPEN_T1_TICK_2      25          /* DLC T1 time between ticks.     */
#define OPEN_T2_TICK_2      10          /* DLC T2 time between ticks.     */
#define OPEN_Ti_TICK_2     125          /* DLC T2 time between ticks.     */

#define SAP_OPTIONS 	0x6C

/* (The following defintions are offsets within the ACA area,
    the offsets are to the different registers, to which you
    can (write,read,set,reset) certain bits.
*/
#define RRR_EVEN_OFFSET  0       /* Ram Relocation Register (double register).*/
#define RRR_ODD_OFFSET   1       /* (Where to position the shared RAM)        */
#define WRBR_EVEN_OFFSET 2       /* Write Region Base Register (double).      */
#define WRBR_ODD_OFFSET  3       /* (Start of the writeable area in RAM).     */
#define WWOR_EVEN_OFFSET 4       /* Write Window Open Register (double).      */
#define WWOR_ODD_OFFSET  5       /* (Start of a write window)                 */
#define WWCR_EVEN_OFFSET 6       /* Write Window Close Register (double)      */
#define WWCR_ODD_OFFSET  7       /* (End of a write window)                   */
#define ISRP_EVEN_OFFSET 8       /* Interrupt Status Register PC (double).    */
#define ISRP_ODD_OFFSET  9       /* (Used by the adapter for the PC)          */
#define ISRA_EVEN_OFFSET 0x0A    /* Interrupt Status Register Adapter (double)*/
#define ISRA_ODD_OFFSET  0x0B    /* (Used by us towards the Adapter)          */
#define TCR_EVEN_OFFSET  0x0C    /* Timer Control Register (double).          */
#define TCR_ODD_OFFSET   0x0D   
#define TVR_EVEN_OFFSET  0x0E    /* Timer Value Register (double).            */
#define TVR_ODD_OFFSET   0x0F
#define SRPR_EVEN_OFFSET 0x18    /* Shared RAM Page Register (double).        */
#define SRPR_ODD_OFFSET  0x19    /* (which of the four pages is in use)       */
#define ISRP_EVEN_RESET  0x28    /* Set this bit for an ISRP even-byte reset. */
#define ISRP_ODD_RESET   0x29    /* Set this bit for an ISRP odd-byte reset.  */
#define ISRA_ODD_RESET   0x2B    /* Set this bit for an ISRA odd-byte reset.  */
#define TCR_EVEN_RESET   0x2C    /* Set this bit for a TCR even-byte reset.   */
#define ISRP_EVEN_SET    0x48    /* Set this bit for an ISRP even-byte set.   */
#define ISRP_ODD_SET     0x49    /* Set this bit for an ISRP odd-byte set.    */
#define ISRA_ODD_SET     0x4B    /* Set this bit for an ISRA odd-byte set.    */
#define TCR_EVEN_SET     0x4C    /* Set this bit for a TCR even-byte set.     */
  
#define PAGE_0    0x00
#define PAGE_1    0x40
#define PAGE_2    0x80
#define PAGE_3    0xC0

#define ENABLE_SW_INTR 0x40
#define ENABLE_IRQ_CONTROL 0x80

#define MAC_ADDR_OFFSET 0x10

#define SRB_RESPONSE     0x20
#define INITIAL_SRB_SIZE 0x40

/* Bit postions for the register ISRP odd*/

#define	INTR_ADAPTER_CHECK	0x40
#define INTR_SRB_RESPONSE	0x20
#define	INTR_ASB_FREE		0x10
#define	INTR_ARB_COMMAND	0x08
#define	INTR_SSB_RESPONSE	0x04
#define	INTR_BRIDGE_FOWARD	0x02

/* Bit postions for the register ISRP even */

#define	INTR_TIMER_INTR		0x10
#define INTR_ERROR_INTR		0x08
#define	INTR_WORKSTATION_ACCESS	0x04

/* Bit positions for the register ISRA even */

#define INTR_ADAPTER_ACCESS	0x20
#define INTR_DEADMAN_TIMER_EXP	0x10
#define	INTR_PROCESSOR_CHECK	0x04

/* Adapter commands for Direct and DLC (IEEE 802.2 SAP and station i/f.). */
#define SRB_INTERRUPT         0x00
#define SRB_MODIFY            0x01
#define SRB_RESTORE           0x02
#define SRB_OPEN              0x03
#define SRB_CLOSE             0x04
#define SRB_SET_GROUP_ADDR    0x06
#define SRB_SET_FUNC_ADDR     0x07
#define SRB_READ_LOG          0x08
#define SRB_TX_DIR_FRAME      0x0A
#define SRB_TX_I_FRAME        0x0B
#define SRB_TX_UI_FRAME       0x0D
#define SRB_TX_XID_CMD        0x0E
#define SRB_TX_XID_RESP_FINAL 0x0F
#define SRB_TX_XID_RESP       0x10
#define SRB_TX_TEST_CMD       0x11
#define SRB_RESET             0x14
#define SRB_OPEN_SAP          0x15
#define SRB_CLOSE_SAP         0x16
#define SRB_REALLOCATE        0x17
#define SRB_OPEN_STATION      0x19
#define SRB_CLOSE_STATION     0x1A
#define SRB_CONNECT_STATION   0x1B
#define SRB_DLC_MODIFY        0x1C
#define SRB_FLOW_CONTROL      0x1D
#define SRB_STATISTICS        0x1E
#define SRB_RECEIVED_DATA     0x81
#define SRB_TRANSMIT_DATA     0x82
#define SRB_DLC_STATUS        0x83
#define SRB_RING_CHANGE       0x84

/* AIP area offsets and sizes. */
#define ENCODED_ADDR_OFFSET  0x00
#define CHANNEL_ID_OFFSET    0x30
#define ADAPTER_TYPE_OFFSET  0xA0
#define DATA_RATE_OFFSET     0xA2
#define EARLY_TOKEN_OFFSET   0xA4
#define RAM_SIZE_OFFSET      0xA6
#define RAM_PAGING_OFFSET    0xA8
#define DHB_4MB_SIZE_OFFSET  0xAA
#define DHB_16MB_SIZE_OFFSET 0xAC

#define AIP_CHECKSUM1_RANGE  0x60
#define AIP_CHECKSUM2_RANGE  0xF0
#define ENCODED_ADDR_LENGTH    12    /* Bytes spred over 2 * the area. */

#define RAM_AVAILABLE_IS_64  0x0B
#define GET_RAM_SIZE_RRR     0x0F    /* Get the RAM size from RRR(2,3). */
/* End of AIP locations and offsets. */

/* SRB offsets. */
#define SRB_RETCODE_OFFSET    0x02
#define SRB_STATION_ID_OFFSET 0x04
/* End of SRB offsets. */

/* SRB return codes. */
#define SRB_SUCCESS           0x00
#define SRB_INITIATED         0xfe
#define SRB_IN_PROGRESS       0xff

/* SSB return codes */
#define SSB_SUCCESS           0x00

/* SSB offsets */
#define SSB_RETCODE_OFFSET    0x02

/* Receive buffer offsets. */
#define RCV_STATION_OFFSET    0x04
#define RCV_BUFFER_OFFSET     0x06
#define LAN_HDR_LEN_OFFSET    0x08
#define DLC_HDR_LEN_OFFSET    0x09
#define RCV_FRAME_LEN_OFFSET  0x0A
#define NEXT_BUFFER_OFFSET    0x02
#define BUFFER_LEN_OFFSET     0x06
#define RCV_BUFF_HDR_LENGTH   0x08
#define RCV_NCB_TYPE_OFFSET   0x0C
/* End of receive buffer offsets. */

#define  HDW_ADDR_LENGTH    12       /* Length of the hardware addr in bytes. */
#define  CHAN_ID_LENGTH     12       /* Length of the channel id in bytes.    */

struct aip {
  char encoded_addr[HDW_ADDR_LENGTH]; /* Universally administered 48-bit addr.*/
  uchar_t adapter_type;               /* F = 1st, E = 2nd, .. , 0 = 16th.     */
  uchar_t data_rate;                  /* F = 4Mbps, E = 16 Mbps, D = both.    */
  uchar_t early_token_release;        /* F = no,E = 4Mbps,D = 16Mbps,C = Both */
  uchar_t total_RAM;                  /* Total available shared RAM.          */
  uchar_t RAM_paging;                 /* Support for RAM paging and page size.*/
  uchar_t initialize_RAM_area;        /* Need to reset upper 512bytes ?       */
  uchar_t DHB_size_4_mbps;            /* Maximum DHB size at 4 Mbps.          */
  uchar_t DHB_size_16_mbps;           /* Maximum DHB size at 16 Mbps.         */
};

/* End of AIP BIOS structures. */

struct aca {
  short RRR_register;                 /* RAM Relocation register.             */
  short WRBR_register;                /* Write Region Base Register.          */
  short WWOR_register;                /* Write Window Open Register.          */
  short WWCR_register;                /* Write Window Close Register.         */
  short ISRP_register;                /* Interrupt Status Register PC.        */
  short ISRA_register;                /* Interrupt Status Register Adapter.   */
  short TCR_register;                 /* Timer Control Register.              */
  short TVR_register;                 /* Timer Value Register.                */
  short SRPR_register;                /* Shared Ram Page Register.            */
};
 
/* End of ACA BIOS structures. */

/* ERROR_CODES */
#define AIP_BAD_CHECKSUM  1

/* Paging macro's. */
#define ADJUST2PAGE(offset) (offset &= 0x3FFF)
#define SET_PAGE(ptr,page)  ((*(ptr->aca_area + SRPR_EVEN_OFFSET)) = page)
#define GET_PAGE(offset)    ((offset >> 8) & 0xC0)

struct OpenParam {
        unchar  command;        /* Open command (X'03).            */
        unchar  reserved[7];    /* Not used (by us).               */
	ushort	options;	/* Open options.                   */
	ushort	naddr_hi;	/* Node address High byte.         */
	ushort	naddr_mid;	/* Node address Middle byte.       */
	ushort	naddr_lo;	/* Node address Low byte.          */
	ushort	gaddr_hi;	/* Group address High byte.        */
	ushort	gaddr_lo;	/* Group address Low byte.         */
	ushort	faddr_hi;	/* Functional address High byte.   */
	ushort	faddr_lo;	/* Functional address Low byte.    */
	ushort	nbr_r_buf;      /* Xmit buffer cnt.                */
	ushort	r_buff_size;	/* Receive buffer size.            */
	ushort	xmit_buff_size;	/* Xmit buffer size.               */
	unchar	nbr_xmit_buf;   /* Xmit buffer cnt.                */
        unchar  reserved2;      /* Not used (by us).               */
        unchar  max_sap;        /* Max. number of SAP's opened.    */
        unchar  max_station;    /* Max. number of link stations.   */
        unchar  max_gsap;       /* Max. number of group SAP's.     */
        unchar  max_gmem;       /* Max. number of SAP's per group. */
        unchar  T1_tick_one;    /* Timer value (1 - 5) * 40ms.     */
        unchar  T2_tick_one;    /* Timer value (1 - 5) * 40ms.     */
        unchar  Ti_tick_one;    /* Timer value (1 - 5) * 40ms.     */
        unchar  T1_tick_two;    /* Timer value (6 - 10) * 40ms.    */
        unchar  T2_tick_two;    /* Timer value (6 - 10) * 40ms.    */
        unchar  Ti_tick_two;    /* Timer value (6 - 10) * 40ms.    */
	unchar	prodid[18];     /* Product ID address.             */
};

struct adap_addrs {
	unchar	node_addrs[6];	/* Node address */
	unchar	group_addrs[4];	/* Group address */
	unchar	func_addrs[4];	/* Functional address */
};

/* Error Codes from Interrupt register */

#define	ERRINITAL	0x0		/* Initial Test Error */
#define	ERRROMCRC	0x1		/* Adapter ROM CRC Error */
#define	ERRRAM		0x2		/* Adapter RAM Error */
#define	ERRINST		0x3		/* Instruction Test Error */
#define	ERRCONTINT	0x4		/* Context/Interrupt Test */
#define	ERRPROTO	0x5		/* Protocol Handler Hardware */
#define	ERRSYSINT	0x6		/* System Interface Register */

/*  Interrupt Codes from Interrupt Register */
/*  The lower the number the higher the interrupt priority */

#define	INT_CHK		0x0		/* Adapter Check */
#define	INT_RING	0x4		/* Ring Status */
#define	INT_SCBCLR	0x6		/* SCB Clear */
#define	INT_CSTAT	0x8		/* Command Status */
#define	INT_RSTAT	0xA		/* Receive Status */
#define	INT_TSTAT	0xC		/* Transmit Status */

/*	Initialization options bit assignments */
/* 	These defines are byte swaped for init block */

#define	INIT_MUSTBE1	0x8000		/* Must always be set to 1 */
#define	INIT_P1		0x4000		/* Parity enable Bit 1 */
#define	INIT_P2		0x2000		/* Parity enable Bit 2 */
#define	INIT_BSXB	0x1000		/* Burst SCB/SSB */
#define	INIT_BLST	0x0800		/* Burst List */
#define	INIT_BSTAT	0x0400		/* Burst List Status */
#define	INIT_BRCV	0x0200		/* Burst Receive */
#define	INIT_BXMIT	0x0100		/* Burst Transmit */

#define	INIT_OPTIONS	INIT_MUSTBE1|INIT_BSXB|INIT_BLST|INIT_BSTAT|INIT_BRCV|INIT_BXMIT
#define PRODIDSIZE 18	/* # of bytes needed for Product ID buffer */

/* The IBM Token ring hardware has only two legal I/O addresses	*/

#define	IBMTR_IOADDR_0	0xA20	
#define	IBMTR_IOADDR_1	0xA24

/* 
   The Following definitions are used to extract the ROM address from the I/O
   base addresses.
*/

#define IBM_AT_CHANNEL_IDENTIFIER "PICO6110990 "
#define IBM_MCA_CHANNEL_IDENTIFIER "MARS63X4518 "

#define BIOS_AT_MASK		0xFC
#define BIOS_AT_SHIFT		11
#define BIOS_MCA_MASK		0xFE
#define BIOS_MCA_SHIFT		12
#define BIOS_BIT19		0x80000
#define RAM_MCA_MASK		0xFE
#define RAM_MCA_SHIFT		12

/* The following definitions are used to access the various structures in the
   shared RAM. Similar structures should be adopted for all adapter commands
   in the later versions of this product.
*/

typedef struct {
	unsigned char command;
	unsigned char init_status;
	unsigned char res1[4];
	ushort bring_up_code;
	ushort encoded_address;
	ushort level_address;
	ushort adapter_address;
	ushort param_address;
	ushort mac_address;
} init_srb_rsp_t;

#define	TRANSMIT_FRAME_ERROR	0x22
#define	ADDR_RECOG_NOT_COPIED	0x80

typedef struct {
	unsigned char command;
	unsigned char command_correlate;
	unsigned char retcode;
	unsigned char res1;
	unsigned short station_id;
	unsigned char transmit_error;
} transmit_ssb_t;

#endif
