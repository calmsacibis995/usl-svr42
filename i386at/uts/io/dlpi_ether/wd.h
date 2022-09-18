/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_DLPI_ETHER_WD_H	/* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_WD_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/wd.h	1.2"
#ident	"$Header: $"

#ifdef	_KERNEL_HEADERS

#ifndef	_UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h> /* REQUIRED */

#endif	/* _KERNEL_HEADERS */

/*
 * Module: WD8003
 * Project: System V ViaNet
 *
 *		Copyright (c) 1987 by Western Digital Corporation.
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Western Digital Corporation
 *			2445 McCabe Way
 *			Irvine, California 92714
 */

/* Hardware definitions for the WD8003 and 8390 LAN controller		*/

/* WD8003 Definitions and Statistics					*/

#define TX_BUF_LEN	(6*256)
#define	SEG_LEN		(1024 * 8)
#define WDMINSEND	( 60) 

/* WD8003 Commands							*/

#define SFTRST	0x80			/* software reset command	*/
#define MEMENA	0x40			/* memory enable switch		*/
 
/* WD8003 register locations						*/

#define	UBA_STA		8
#define	WD_BYTE		0xE

/* 8390 Registers: Page 1						*/
/* NOTE: All addresses are offsets from the command register (cmd_reg)	*/

#define	PSTART	0x1
#define	PSTOP	0x2
#define	BNRY	0x3
#define	TPSR	0x4
#define	TBCR0	0x5
#define	TBCR1	0x6
#define	ISR	0x7
#define RBCR0	0xA
#define RBCR1	0xB
#define	RCR	0xC
#define	TCR	0xD
#define	DCR	0xE
#define	IMR	0xF
#define	RSR	0xC
#define	CNTR0	0xD
#define CNTR1	0xE
#define	CNTR2	0xF

/* 8390 Registers: Page 2						*/
/* NOTE: All addresses are offsets from the command register (cmd_reg)	*/

#define	PAR0	0x1
#define	CURR	0x7
#define MAR0	0x8

/* 8390 Commands							*/

#define	PAGE_0	0x00
#define	PAGE_1	0x40
#define	PAGE_2	0x80
#define	PAGE_3	0xC0

#define	PG_MSK	0x3F			/* Used to zero the page select
					   bits in the command register */

#define	STA	0x2			/* Start 8390			*/
#define STP	0x1			/* Stop 8390			*/
#define	TXP	0x4			/* Transmit Packet		*/
#define	ABR	0x20			/* Value for Remote DMA CMD	*/

/* 8390 ISR conditions							*/

#define	PRX	0x1
#define	PTX	0x2
#define	RXE	0x4
#define	TXE	0x8
#define	OVW	0x10
#define	CNT	0x20

/* 8390 IMR bit definitions						*/

#define	PRXE	0x1
#define	PTXE	0x2
#define	RXEE	0x4
#define	TXEE	0x8
#define	OVWE	0x10
#define	CNTE	0x20
#define	RDCE	0x40

/* 8390 DCR bit definitions						*/

#define	WTS	0x1
#define	BOS	0x2
#define	LAS	0x4
#define	BMS	0x8
#define	FT0	0x20
#define	FT1	0x40


/* 8390 TCR bit definitions						*/

#define	CRC	0x1
#define	LB0_1	0x2
#define	ATD	0x8
#define	OFST	0x10


/* RCR bit definitions							*/

#define	SEP	0x1
#define	AR	0x2
#define	AB	0x4
#define	AM	0x8
#define	PRO	0x10
#define	MON	0x20

/* TSR bit definitions							*/
#define TSR_OK		0x1
#define TSR_COL 	0x4
#define TSR_ABT 	0x8
#define TSR_CRS 	0x10
#define TSR_FU		0x20
#define TSR_CDH		0x40
#define TSR_OWC		0x80

/* 8390 Register initialization values					*/

#define	INIT_IMR	PRXE + PTXE + RXEE + TXEE
#define INIT_DCR	BMS + FT1
#define	INIT_TCR	0
#define	INIT_RCR	AB + AM
#define	RCRMON		MON

/* Misc. Commands & Values						*/

#define	CLR_INT		0xFF		/* Used to clear the ISR */
#define	NO_INT		0		/* no interrupts conditions */
#define ADDR_LEN	6
#define NETPRI		PZERO+3

/* PS2 specific defines */
/* Defines for PS/2 Microchannel POS ports */

#define SYS_ENAB	0x94		/* System board enable / setup */
#define ADAP_ENAB	0x96		/* Adaptor board enable / setup */
#define POS_0		0x100		/* POS reg 0 - adaptor ID lsb */
#define POS_1		0x101		/* POS reg 1 - adaptor ID msb */
#define POS_2		0x102		/* Option Select Data byte 1 */
#define POS_3		0x103		/* Option Select Data byte 2 */
#define POS_4		0x104		/* Option Select Data byte 3 */
#define POS_5		0x105		/* Option Select Data byte 4 */
#define POS_6		0x106		/* Subaddress extension lsb */
#define POS_7		0x107		/* Subaddress extension msb */

#define CCR		0x5
#define EIL		0x4

/* Defines for Adaptor Board ID's for Microchannel */

#define WD_ID	0xABCD			/* generic id for WD test */
#define ETI_ID	0x6FC0			/* 8003et/a ID */
#define STA_ID	0x6FC1			/* 8003st/a ID */
#define WA_ID	0x6FC2			/* 8003w/a ID */

#define NUM_ID	4

#define PS2_RAMSZ	16384

/* Driver-specific Data Structures and Macros */

struct wdmaddr {
   unsigned char filterbit;	/* the hashed value of entry */
   unsigned char entry[6];	/* multicast addresses are 6 bytes */
};

union crc_reg {			/* structure for software crc */
	unsigned int value;
	struct {
		unsigned a0	:1;
		unsigned a1	:1;
		unsigned a2	:1;
		unsigned a3	:1;
		unsigned a4	:1;
		unsigned a5	:1;
		unsigned a6	:1;
		unsigned a7	:1;
		unsigned a8	:1;
		unsigned a9	:1;
		unsigned a10	:1;
		unsigned a11	:1;
		unsigned a12	:1;
		unsigned a13	:1;
		unsigned a14	:1;
		unsigned a15	:1;
		unsigned a16	:1;
		unsigned a17	:1;
		unsigned a18	:1;
		unsigned a19	:1;
		unsigned a20	:1;
		unsigned a21	:1;
		unsigned a22	:1;
		unsigned a23	:1;
		unsigned a24	:1;
		unsigned a25	:1;
		unsigned a26	:1;
		unsigned a27	:1;
		unsigned a28	:1;
		unsigned a29	:1;
		unsigned a30	:1;
		unsigned a31	:1;
	} bits;
};

typedef struct bdd {
   short    wd_index;		/* board index */
   long	    wd_memsize;		/* memory size */
   caddr_t  wd_rambase;		/* pointer to mapped version */
   unchar   wd_boardtype;	/* Starlan = 2; Ethernet = 3 */
   int	    wd_init;		/* board status */
   int	    wd_str_open;	/* number of streams open */
   short    wd_minors;		/* number of minor devices allowed */
   unchar   wd_nxtpkt;		/* page # of next packet to remove */
   long	    wd_ncount;		/* count of bufcalls */
   long	    wd_devmode;		/* device mode (e.g. PROM) */
   short    wd_firstd;		/* first minor device for this major */
   struct   wdmaddr *wd_multip; /* last referenced multicast address */
   struct   wdmaddr *wd_multiaddrs;	/* array of multicast addresses */
} bdd_t;

/* Debug Flags and other info */

#define WDSYSCALL	0x01	/* trace syscall functions */
#define WDPUTPROC	0x02	/* trace put procedures */
#define WDSRVPROC	0x04	/* trace service procedures */
#define WDRECV		0x08	/* trace receive processing */
#define WDRCVERR	0x10	/* trace receive errors */
#define WDDLPRIM	0x20	/* trace DL primitives */
#define WDINFODMP	0x40	/* dump info req data */
#define WDDLPRIMERR	0x80	/* mostly llccmds errors */
#define WDDLSTATE      0x100	/* print state chages */
#define WDTRACE	       0x200	/* trace loops */
#define WDINTR	       0x400	/* trace interrupt processing */
#define WDBOARD	       0x800	/* trace access to the board */
#define WDLLC1	      0x1000	/* trace llc1 processing */
#define WDSEND	      0x2000	/* trace sending */
#define WDBUFFER      0x4000	/* trace buffer/canput fails */
#define WDSCHED       0x8000	/* trace scheduler calls */
#define WDXTRACE      0x10000	/* trace wdsend attempts */
#define WDMULTHDW     0x20000	/* trace multicast register filter bits */
#define WDDEBUG 0

/* other useful macros */
#define HIGH(x) ((x>>8)&0xFF)
#define LOW(x)	(x&0xFF)

/* recoverable error conditions */
#define WDE_OK		0		  /* normal condition */
#define WDE_SYSERR	0x1000		  /* or'd into an errno value */
#define WDE_ERRMASK	0x0fff		  /* mask to get errno value */
#define WDE_NOBUFFER	(WDE_SYSERR|ENOSR) /* couldn't allocb */
#define WDE_INVALID	DL_OUTSTATE	  /* operation isn't valid at this time */
#define WDE_BOUND	DL_BOUND	  /* stream is already bound */
#define WDE_BLOCKED	WDE_SYSERR	  /* blocked at next queue */

/*
 * WD 800X event statistics
 */

#define WDS_NSTATS	16

struct wdstat {
    ulong	wds_nstats;	/* number of stat fields */
    /* non-hardware */
    ulong	wds_nobuffer;	/* 0 */
    ulong	wds_blocked;	/* 1 */
    ulong	wds_blocked2;	/* 2 */
    ulong	wds_multicast;	/* 3 */

   /* transmit */
   ulong	wds_xpkts;	/* 4 */
   ulong	wds_xbytes;	/* 5 */
   ulong	wds_excoll;	/* 6 */
   ulong	wds_coll;	/* 7 */
   ulong	wds_fifounder;	/* 8 */
   ulong	wds_carrier;	/* 9 */

   /* receive */
   ulong	wds_rpkts;	/* 10 */
   ulong	wds_rbytes;	/* 11 */
   ulong	wds_crc;	/* 12 */
   ulong	wds_align;	/* 13 */
   ulong	wds_fifoover;	/* 14 */
   ulong	wds_lost;	/* 15 */
};

/* ioctl functions to board */
/* NET_ names are for WD/ViaNet compatibility */

#define NET_INIT	(('D' << 8) | 1)
#define NET_UNINIT	(('D' << 8) | 2)
#define NET_GETBROAD	(('D' << 8) | 3)
#define DLGBROAD	(('D' << 8) | 3)
#define NET_GETSTATUS	(('D' << 8) | 4)
#define DLGSTAT		(('D' << 8) | 4)
#define NET_ADDR	(('D' << 8) | 5)
#define DLGADDR		(('D' << 8) | 5)
#define NET_SETPROM	(('D' << 8) | 6)
#define DLPROM		(('D' << 8) | 6)
#define DLSADDR		(('D' << 8) | 7)
#define NET_WDBRDTYPE	(('D' << 8) | 8)
#define NET_WDSTATUS	(('D' << 8) | 9)
#define DLSMULT 	(('D' << 8) | 10)
#define DLDMULT 	(('D' << 8) | 11)
#define DLGMULT		(('D' << 8) | 12)

#endif /* _IO_DLPI_ETHER_WD_H */
