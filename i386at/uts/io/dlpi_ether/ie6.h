/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_IE6_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_IE6_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/dlpi_ether/ie6.h	1.7"
#ident	"$Header: $"

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

/**************************************************************************
 *
 *  The following ie6 (3C503) structures are defined for clarity.  Most
 *  operation are done through inb() and outb() functions and therefore
 *  can not be done though direct structure member references.
 *
 *************************************************************************/

/**************************************
 *********** Page 0 of NIC ************
 **************************************/
/*
 *  Write side register structure/defines
 */
struct	page0_write {
	uchar_t	cr;		/* Control Register			*/
#define	    STP		0x01		/* Stop				*/
#define	    STA		0x02		/* Start			*/
#define	    TXP		0x04		/* Transmit Packet		*/
#define	    RD0		0x08		/* Remote DMA Command		*/
#define	    RD1		0x10		/* Remote DMA Command		*/
#define	    RD2		0x20		/* Remote DMA Command		*/
#define		REMOTE_RD	RD0	/* Remote Read DMA		*/
#define		REMOTE_WR	RD1	/* Remote Write DMA		*/
#define		SEND_PKT    (RD0 | RD1)	/* Send Packet			*/
#define		DMA_ABORT	RD2	/* Abort/Complete Remote DMA	*/
#define	    PS0		0x40		/* Page Select			*/
#define	    PS1		0x80		/* Page Select			*/
#define		PAGE_0		0x00	/* Page 0 Select		*/
#define		PAGE_1		PS0	/* Page 1 Select		*/
#define		PAGE_2		PS1	/* Page 2 Select		*/
	uchar_t	pstart;		/* Page Start				*/
	uchar_t	pstop;		/* Page Stop				*/
	uchar_t	bnry;		/* Boundary Offset			*/
	uchar_t	tpsr;		/* Transmit Page Start			*/
	uchar_t	tbc_lsb;	/* Transmit count LSB			*/
	uchar_t	tbc_msb;	/* Transmit count MSB			*/
	uchar_t	isr;		/* Interrupt Status Register		*/
	uchar_t	rsar_lsb;	/* Remote DMA (start) Address		*/
	uchar_t	rsar_msb;	/* Remote DMA (start) Address		*/
	uchar_t	rbcr_lsb;	/* Remote DMA Byte Count LSB		*/
	uchar_t	rbcr_msb;	/* Remote DMA Byte Count MSB		*/
	uchar_t	rcr;		/* Receive Configuration		*/
#define	    SEP		0x01		/* Save Error Packets		*/
#define	    AR		0x02		/* Accept Runt Packets		*/
#define	    AB		0x04		/* Accept Broadcast Packets 	*/
#define	    AM		0x08		/* Accept Multicast Packets 	*/
#define	    PRO		0x10		/* Promiscuous Mode		*/
#define	    MON		0x20		/* Monitor Mode			*/
	uchar_t	tcr;		/* Transmit Configuration		*/
#define	    CRC		0x01		/* Inhibit CRC			*/
#define	    LB0		0x02		/* Encoded Loopback		*/
#define	    LB1		0x04		/* 				*/
#define		NO_LPBK		0x00	/* Normal Operation		*/
#define		INT_LPBK	LB0	/* Interal Loopback		*/
#define		EXT_LPBK1	LB1	/* External Loopback LPBK = 1	*/
#define		EXT_LPBK2   (LB1 | LB0)	/* External Loopback LPBK = 0	*/
#define	    ATD		0x08		/* Auto-transmit Disable	*/
#define	    OFST	0x10		/* Collision Offset Enable	*/
	uchar_t	dcr;		/* Data Configuration			*/
#define	    WTS		0x01		/* Word Transfer Select		*/
#define	    BOS		0x02		/* Byte Order Select		*/
#define	    LAS		0x04		/* Long Address Select		*/
#define	    LS		0x08		/* Loopback Select		*/
#define	    AIR		0x10		/* Auto-Initialize Remote	*/
#define	    FT0		0x20		/* FIFO Threshold Select	*/
#define	    FT1		0x40		/* 				*/
#define		FT_2_BYTES	0x00	/*  2 Byte Threshold		*/
#define		FT_4_BYTES	FT0	/*  4 Byte Threshold		*/
#define		FT_8_BYTES	FT1	/*  8 Byte Threshold		*/
#define		FT_12_BYTES (FT0 | FT1)	/* 12 Byte Threshold		*/
	uchar_t	imr;		/* Interrupt Mask			*/
#define	    PRXE	0x01		/* Packet Receive		*/
#define	    PTXE	0x02		/* Packet Transmit		*/
#define	    RXEE	0x04		/* Receive Error		*/
#define	    TXEE	0x08		/* Transmit Error		*/
#define	    OVWE	0x10		/* Overwrite Warning		*/
#define	    CNTE	0x20		/* Counter Overflow		*/
#define	    RDCE	0x40		/* DMA Complete			*/
};

/*
 *  Write side register offset defines
 */
#define	NIC_CR		0x00	/* Control Register			*/
#define	NIC_PSTART	0x01	/* Page Start				*/
#define	NIC_PSTOP	0x02	/* Page Stop				*/
#define	NIC_BNRY	0x03	/* Boundary Offset			*/
#define	NIC_TPSR	0x04	/* Transmit Page Start			*/
#define	NIC_TBC_LSB	0x05	/* Transmit count (LSB byte)		*/
#define	NIC_TBC_MSB	0x06	/* Transmit count (MSB byte)		*/
#define	NIC_ISR		0x07	/* Interrupt Status Register		*/
#define	NIC_RSAR_LSB	0x08	/* Remote DMA (start) Address		*/
#define	NIC_RSAR_MSB	0x09	/* Remote DMA (start) Address		*/
#define	NIC_RBCR_LSB	0x0a	/* Remote DMA Byte Count		*/
#define	NIC_RBCR_MSB	0x0b	/* Remote DMA Byte Count		*/
#define	NIC_RCR		0x0c	/* Receive Configuration		*/
#define	NIC_TCR		0x0d	/* Transmit Configuration		*/
#define	NIC_DCR		0x0e	/* Data Configuration			*/
#define	NIC_IMR		0x0f	/* Interrupt Mask			*/

/*
 *  Read side register structure/defines
 */
struct page0_read {
	uchar_t	cr;		/* Control Register			*/
	uchar_t	clda_lsb;	/* Current Local DMA Address (LSB)	*/
	uchar_t	clda_msb;	/* Current Local DMA Address (MSB)	*/
	uchar_t	bnry;		/* Boundary Offset			*/
	uchar_t	tsr;		/* Transmit Status			*/
#define	    PTX		0x01		/* Packet Transmitted		*/
/*			0x02		   not defined			*/
#define	    COL		0x04		/* Collision			*/
#define	    ABT		0x08		/* Transmit Aborted		*/
#define	    CRS		0x10		/* Carrier Sense Lost		*/
#define	    FU		0x20		/* FIFO Underrun		*/
#define	    CDH		0x40		/* Collision Detect Heart Beat	*/
#define	    OWC		0x80		/* Out of Window Collisions	*/
	uchar_t	ncr;		/* Number of Collisions			*/
	uchar_t	fifo;		/* Loop Back FIFO			*/
	uchar_t	isr;		/* Interrupt Status Register		*/
#define	    PRXI	0x01		/* Packet Received		*/
#define	    PTXI	0x02		/* Packet Transmitted		*/
#define	    RXEI	0x04		/* Receive Error		*/
#define	    TXEI	0x08		/* Transmit Error		*/
#define	    OVWI	0x10		/* Overwrite Warning		*/
#define	    CNTI	0x20		/* Counter Overflow		*/
#define	    RDCI	0x40		/* Remote DMA Complete		*/
#define	    RSTI	0x80		/* Reset Status			*/
	uchar_t	crda_lsb;	/* Current Remote DMA Address (LSB)	*/
	uchar_t	crda_msb;	/* Current Remote DMA Address (MSB)	*/
	uchar_t	n0;		/* not used				*/
	uchar_t	n1;		/* not used				*/
	uchar_t	rsr;		/* Receive Status			*/
#define	    PRX		0x01		/* Packet Received Intact	*/
#define	    CRCE	0x02		/* CRC Error			*/
#define	    FAE		0x04		/* Frame Alignment Error	*/
#define	    FOE		0x08		/* FIFO Overrun			*/
#define	    MPAE	0x10		/* Missed Packet		*/
#define	    PHY		0x20		/* Physical or Multicast	*/
#define	    DIS		0x40		/* Receiver Disabled		*/
#define	    DFR		0x80		/* Deferring			*/
	uchar_t	fae_cnt;	/* Frame Alignment Errors Tally Counter	*/
	uchar_t	crc_cnt;	/* CRC Errors Taly Counter 		*/
	uchar_t	missed_cnt;	/* Missed Packet Errors	 Tally Counter	*/
};

/*
 *  Read side register offset defines
 */
#define	NIC_CR		0x00	/* Control Register			*/
#define	NIC_CLDA_LSB	0x01	/* Current Local DMA Address (LSB)	*/
#define	NIC_CLDA_MSB	0x02	/* Current Local DMA Address (MSB)	*/
#define NIC_BNDY	0x03	/* Boundary Offset			*/
#define	NIC_TSR		0x04	/* Transmit Status			*/
#define	NIC_NCR		0x05	/* Number of Collisions			*/
#define	NIC_FIFO	0x06	/* Loop Back FIFO			*/
#define	NIC_ISR		0x07	/* Interrupt Status Register		*/
#define	NIC_CRDA_LSB	0x08	/* Current Remote DMA Address (LSB)	*/
#define	NIC_CRDA_MSB	0x09	/* Current Remote DMA Address (MSB)	*/
/*			0x0a	   not used				*/
/*			0x0b	   not used				*/
#define	NIC_RSR		0x0c	/* Receive Status			*/
#define	NIC_FAE_CNT	0x0d	/* Frame Alignment Errors Tally Counter	*/
#define	NIC_CRC_CNT	0x0e	/* CRC Errors Tally Counter		*/
#define	NIC_MISSED_CNT	0x0f	/* Missed packet Errors			*/

/**************************************
 *********** Page 1 of NIC ************
 **************************************/
/*
 *  Page 1 register structure/defines.  It is the same for read and write.
 */
#define	NIC_PAR_SIZE	6
#define	NIC_MAR_SIZE	8
struct page1 {
	uchar_t	cr;			/* Control Register		*/
	uchar_t	par[ NIC_PAR_SIZE ];	/* Physical Ethernet Address	*/
	uchar_t	curr;			/* Current Page			*/
	uchar_t	mar[ NIC_MAR_SIZE ];	/* Multicast Address Registers	*/
};

/*
 *  Page 1 register offset defines
 */
#define	NIC_CR		0x00	/* Control Register			*/
#define	NIC_PAR0	0x01	/* Physical Ethernet Address 0		*/
#define	NIC_PAR1	0x02	/* Physical Ethernet Address 1		*/
#define	NIC_PAR2	0x03	/* Physical Ethernet Address 2		*/
#define	NIC_PAR3	0x04	/* Physical Ethernet Address 3		*/
#define	NIC_PAR4	0x05	/* Physical Ethernet Address 4		*/
#define	NIC_PAR5	0x06	/* Physical Ethernet Address 5		*/
#define	NIC_CURR	0x07	/* Cuurent Page				*/
#define	NIC_MAR0	0x08	/* Multicast Address Register 0		*/
#define	NIC_MAR1	0x09	/* Multicast Address Register 1		*/
#define	NIC_MAR2	0x0a	/* Multicast Address Register 2		*/
#define	NIC_MAR3	0x0b	/* Multicast Address Register 3		*/
#define	NIC_MAR4	0x0c	/* Multicast Address Register 4		*/
#define	NIC_MAR5	0x0d	/* Multicast Address Register 5		*/
#define	NIC_MAR6	0x0e	/* Multicast Address Register 6		*/
#define	NIC_MAR7	0x0f	/* Multicast Address Register 7		*/

/**************************************
 *********** Page 2 of NIC ************
 **************************************
 *
 *  Page 2 register structure/defines.  Page 2 registers should only
 *  be accessed for diagnostic purposes.
 */

/*
 *  Write side register structure/defines
 */
struct page2_write {
	uchar_t	cr;		/* Control Register			*/
	uchar_t	clda_lsb;	/* Current Local DMA Address (LSB)	*/
	uchar_t	clda_msb;	/* Current Local DMA Address (MSB)	*/
	uchar_t	rnpp;		/* Remote Next Packet Pointer		*/
	uchar_t	n0;		/* not used				*/
	uchar_t	lnpp;		/* Local Next Packet Pointer		*/
	uchar_t	ac_msb;		/* Address Counter (MSB)		*/
	uchar_t	ac_lsb;		/* Address Counter (LSB)		*/
	uchar_t	n1;		/* not used				*/
	uchar_t	n2;		/* not used				*/
	uchar_t	n3;		/* not used				*/
	uchar_t	n4;		/* not used				*/
	uchar_t	n5;		/* not used				*/
	uchar_t	n6;		/* not used				*/
	uchar_t	n7;		/* not used				*/
	uchar_t	n8;		/* not used				*/
};

/*
 *  Write side register offset defines
 */
#define	NIC_CR		0x00	/* Control Register			*/
#define	NIC_CLDA_LSB	0x01	/* Current Local DMA Address (LSB)	*/
#define	NIC_CLDA_MSB	0x02	/* Current Local DMA Address (MSB)	*/
#define	NIC_RNPP	0x03	/* Remote Next Packet Pointer		*/
/*			0x04	   not used				*/
#define	NIC_LNPP	0x05	/* Local Next Packet Pointer		*/
#define	NIC_AC_MSB	0x06	/* Address Counter (MSB)		*/
#define	NIC_AC_LSB	0x07	/* Address Counter (LSB)		*/
/*			0x08	   not used				*/
/*			0x09	   not used				*/
/*			0x0a	   not used				*/
/*			0x0b	   not used				*/
/*			0x0c	   not used				*/
/*			0x0d	   not used				*/
/*			0x0e	   not used				*/
/*			0x0f	   not used				*/

/*
 *  Read side register structure/defines
 */
struct page2_read {
	uchar_t	cr;		/* Control Register			*/
	uchar_t	pstart;		/* Page Start				*/
	uchar_t	pstop;		/* Page Stop				*/
	uchar_t	rnpp;		/* Remote Next Packet Pointer		*/
	uchar_t	tpsr;		/* Transmit Page Start			*/
	uchar_t	lnpp;		/* Local Next Packet Pointer		*/
	uchar_t	ac_msb;		/* Address Counter (MSB)		*/
	uchar_t	ac_lsb;		/* Address Counter (LSB)		*/
	uchar_t	n0;		/* not used				*/
	uchar_t	n1;		/* not used				*/
	uchar_t	n2;		/* not used				*/
	uchar_t	n3;		/* not used				*/
	uchar_t	rcr;		/* Receive Configuration		*/
	uchar_t	tcr;		/* Transmit Configuration		*/
	uchar_t	dcr;		/* Data Configuration			*/
	uchar_t	imr;		/* Interrupt Mask			*/
};

/*
 *  Read side register offset defines
 */
#define	NIC_CR		0x00	/* Control Register			*/
#define	NIC_PSTART	0x01	/* Page Start				*/
#define	NIC_PSTOP	0x02	/* Page Stop				*/
#define	NIC_RNPP	0x03	/* Remote Next Packet Pointer		*/
#define	NIC_TPSR	0x04	/* Transmit Page Start			*/
#define	NIC_LNPP	0x05	/* Local Next Packet Pointer		*/
#define	NIC_AC_MSB	0x06	/* Address Counter (MSB)		*/
#define	NIC_AC_LSB	0x07	/* Address Counter (LSB)		*/
/*			0x08	   not used				*/
/*			0x09	   not used				*/
/*			0x0a	   not used				*/
/*			0x0b	   not used				*/
#define	NIC_RCR		0x0c	/* Receive Configuration		*/
#define	NIC_TCR		0x0d	/* Transmit Configuration		*/
#define	NIC_DCR		0x0e	/* Data Configuration			*/
#define	NIC_IMR		0x0f	/* Interrupt Mask			*/

/**************************************************************************
 *  Composit Write NIC structure for page 0, 1 and 2.
 */
struct nic_write {
    union {
	struct page0_write	p0;
	struct page1		p1;
	struct page2_write	p2;
    } page;
};

/**************************************************************************
 *  Composit Read NIC structure for page 0, 1 and 2.
 */
struct nic_read {
    union {
	struct	page0_read	p0;
	struct	page1		p1;
	struct	page2_read	p2;
    } page;
};

/**************************************************************************
 *  Composit Read/Write NIC structure
 */
typedef	struct {
    union {
	struct nic_write	write;	/* Registers for write ops	*/
	struct nic_read		read;	/* Registers for read ops	*/
    } op;
} nic_t;

/**************************************************************************
 *  The gate array structure starts at the base address plus 0x400.
 */
#define	GA_OFFSET	0x400

/**************************************************************************
 *  Gate array write side structure/defines
 */
struct	ga_write {
	uchar_t	pstr;		/* Page Start Register			*/
	uchar_t	pspr;		/* Page Stop Register			*/
	uchar_t	dqtr;		/* DMA Request Timer Register		*/
	uchar_t	n0;		/* not used				*/
	uchar_t	n1;		/* not used				*/
	uchar_t	gacfr;		/* Gate Array Configuration Register	*/
#define	    MBS0	0x01		/* Memory Bank Select 1		*/
#define	    MBS1	0x02		/* Memory Bank Select 2		*/
#define	    MBS2	0x04		/* Memory Bank Select 3		*/
#define	    RSEL	0x08		/* RAM Select			*/
#define	    TEST	0x10		/* GA Vendor Test Only		*/
#define	    OWS		0x20		/* Zero Wait State		*/
#define	    TCM		0x40		/* Terminal Count Mask		*/
#define	    NIM		0x80		/* NIC Interrupt Mask		*/
	uchar_t	cr;		/* Control Register			*/
#define	    RST		0x01		/* Software Reset		*/
#define	    XSEL	0x02		/* Xcvr Select			*/
#define	    EALO	0x04		/* Ethernet Addr Low		*/
#define	    EAHI	0x08		/* Ethernet Addr High		*/
#define	    SHARE	0x10		/* Interrupt Sharing		*/
#define	    DBSEL	0x20		/* Double Buffer Select		*/
#define	    DDIR	0x40		/* DMA Direction		*/
#define		UPLOAD		0x00	/* DMA from ie6 to system	*/
#define		DOWNLOAD	DDIR	/* DMA from system to ie6	*/
#define	    START	0x80		/* Start DMA			*/
	uchar_t	n2;		/* not used				*/
	uchar_t	idcfr;		/* Interrupt/DMA Configuration Register	*/
#define	    DRQ1	0x01		/* DMA Request 1		*/
#define	    DRQ2	0x02		/* DMA Request 2		*/
#define	    DRQ3	0x04		/* DMA Request 3		*/
/*			0x08		   not used			*/
#define	    IRQ2	0x10		/* Interrupt Request 2		*/
#define	    IRQ3	0x20		/* Interrupt Request 3		*/
#define	    IRQ4	0x40		/* Interrupt Request 4		*/
#define	    IRQ5	0x80		/* Interrupt Request 5		*/
	uchar_t	dma_msb;	/* DMA Address Register MSB		*/
	uchar_t	dma_lsb;	/* DMA Address Register LSB		*/
	uchar_t	vptr2;		/* Vector Pointer Register high		*/
	uchar_t	vptr1;		/* Vector Pointer Register mid		*/
	uchar_t	vptr0;		/* Vector Pointer Register low		*/
	uchar_t	rf_msb;		/* Register File (FIFO) MSB		*/
	uchar_t	rf_lsb;		/* Register File (FIFO) LSB		*/
};

/*
 *  Write side register absolute offset defines for Gate Array
 */
#define	GA_PSTR		0x400	/* Page Start Register			*/
#define	GA_PSPR		0x401	/* Page Stop Register			*/
#define	GA_DQTR		0x402	/* DMA Request Timer Register		*/
/*			0x403	   not used				*/
/*			0x404	   not used				*/
#define	GA_GACFR	0x405	/* Gate Array Configuration Register	*/
#define	GA_CR		0x406	/* Control Register			*/
/*			0x407	   not used				*/
#define	GA_IDCFR	0x408	/* Interrupt/DMA Configuration Register	*/
#define	GA_DAM_MSB	0x409	/* DMA Address Register MSB		*/
#define	GA_DAM_LSB	0x40a	/* DMA Address Register LSB		*/
#define	GA_VPTR2	0x40b	/* Vector Pointer Register high		*/
#define	GA_VPTR1	0x40c	/* Vector Pointer Register mid		*/
#define	GA_VPTR0	0x40d	/* Vector Pointer Register low		*/
#define	GA_RF_MSB	0x40e	/* Register File (FIFO) MSB		*/
#define	GA_RF_LSB	0x40f	/* Register File (FIFO) LSB		*/

/*
 *  Write side register relative offset defines for Gate Array
 */
#define	GA_PSTR_OFF	0x00	/* Page Start Register			*/
#define	GA_PSPR_OFF	0x01	/* Page Stop Register			*/
#define	GA_DQTR_OFF	0x02	/* DMA Request Timer Register		*/
/*			0x03	   not used				*/
/*			0x04	   not used				*/
#define	GA_GACFR_OFF	0x05	/* Gate Array Configuration Register	*/
#define	GA_CR_OFF	0x06	/* Control Register			*/
/*			0x07	   not used				*/
#define	GA_IDCFR_OFF	0x08	/* Interrupt/DMA Configuration Register	*/
#define	GA_DAM_MSB_OFF	0x09	/* DMA Address Register MSB		*/
#define	GA_DAM_LSB_OFF	0x0a	/* DMA Address Register LSB		*/
#define	GA_VPTR2_OFF	0x0b	/* Vector Pointer Register high		*/
#define	GA_VPTR1_OFF	0x0c	/* Vector Pointer Register mid		*/
#define	GA_VPTR0_OFF	0x0d	/* Vector Pointer Register low		*/
#define	GA_RF_MSB_OFF	0x0e	/* Register File (FIFO) MSB		*/
#define	GA_RF_LSB_OFF	0x0f	/* Register File (FIFO) LSB		*/

/**************************************************************************
 *  Gate array read side structure/defines
 */
struct	ga_read {
	uchar_t	pstr;		/* Page Start Register			*/
	uchar_t	pspr;		/* Page Stop Register			*/
	uchar_t	dqtr;		/* DMA Request Timer Register		*/
	uchar_t	bcfr;		/* Base Configuration Register		*/
#define	    IOB2E0	0x01		/* I/O Base set at 2E0		*/
#define	    IOB2A0	0x02		/* I/O Base set at 2A0		*/
#define	    IOB280	0x04		/* I/O Base set at 280		*/
#define	    IOB250	0x08		/* I/O Base set at 250		*/
#define	    IOB350	0x10		/* I/O Base set at 350		*/
#define	    IOB330	0x20		/* I/O Base set at 330		*/
#define	    IOB310	0x40		/* I/O Base set at 310		*/
#define	    IOB300	0x80		/* I/O Base set at 300		*/
	uchar_t	pcfr;		/* EPROM Configuration Register		*/
#define	    C8XXX	0x10		/* Memory Base C8000		*/
#define	    CCXXX	0x20		/* Memory Base CC000		*/
#define	    D8XXX	0x40		/* Memory Base D8000		*/
#define	    DCXXX	0x80		/* Memory Base DC000		*/
	uchar_t	gacfr;		/* Gate Array Configuration Register	*/
	uchar_t	cr;		/* Control Register			*/
	uchar_t	streg;		/* Status Register			*/
#define	    REV0	0x01		/* Gate Array Rev 0		*/
#define	    REV1	0x02		/* Gate Array Rev 1		*/
#define	    REV2	0x04		/* Gate Array Rev 2		*/
#define	    DIP		0x08		/* DMA In Progress		*/
#define	    DTC		0x10		/* DMA Terminal Count		*/
#define	    OFLW	0x20		/* FIFO Overflow		*/
#define	    UFLW	0x40		/* FIFO Underflow		*/
#define	    DPRDY	0x80		/* Data Port Ready		*/
	uchar_t	idcfr;		/* Interrupt/DMA Configuration Register	*/
	uchar_t	dma_msb;	/* DMA Address Register MSB		*/
	uchar_t	dma_lsb;	/* DMA Address Register LSB		*/
	uchar_t	vptr2;		/* Vector Pointer Register high		*/
	uchar_t	vptr1;		/* Vector Pointer Register mid		*/
	uchar_t	vptr0;		/* Vector Pointer Register low		*/
	uchar_t	rf_msb;		/* Register File (FIFO) MSB		*/
	uchar_t	rf_lsb;		/* Register File (FIFO) LSB		*/
};

/*
 *  Read side register absolute offset defines for Gate Array
 */
#define	GA_PSTR		0x400	/* Page Start Register			*/
#define	GA_PSPR		0x401	/* Page Stop Register			*/
#define	GA_DQTR		0x402	/* DMA Request Timer Register		*/
#define	GA_BCFR		0x403	/* Base Configuration Registser		*/
#define	GA_PCFR		0x404	/* EPROM Configuration Register 	*/
#define	GA_GACFR	0x405	/* Gate Array Configuration Register	*/
#define	GA_CR		0x406	/* Control Register			*/
#define	GA_STREG	0x407	/* Status Register			*/
#define	GA_IDCFR	0x408	/* Interrupt/DMA Configuration Register	*/
#define	GA_DAM_MSB	0x409	/* DMA Address Register MSB		*/
#define	GA_DAM_LSB	0x40a	/* DMA Address Register LSB		*/
#define	GA_VPTR2	0x40b	/* Vector Pointer Register high		*/
#define	GA_VPTR1	0x40c	/* Vector Pointer Register mid		*/
#define	GA_VPTR0	0x40d	/* Vector Pointer Register low		*/
#define	GA_RF_MSB	0x40e	/* Register File (FIFO) MSB		*/
#define	GA_RF_LSB	0x40f	/* Register File (FIFO) LSB		*/

/*
 *  Read side register relative offset defines for Gate Array
 */
#define	GA_PSTR_OFF	0x00	/* Page Start Register			*/
#define	GA_PSPR_OFF	0x01	/* Page Stop Register			*/
#define	GA_DQTR_OFF	0x02	/* DMA Request Timer Register		*/
#define	GA_BCFR_OFF	0x03	/* Base Configuration Registser		*/
#define	GA_PCFR_OFF	0x04	/* EPROM Configuration Register 	*/
#define	GA_GACFR_OFF	0x05	/* Gate Array Configuration Register	*/
#define	GA_CR_OFF	0x06	/* Control Register			*/
#define	GA_STREG_OFF	0x07	/* Status Register			*/
#define	GA_IDCFR_OFF	0x08	/* Interrupt/DMA Configuration Register	*/
#define	GA_DAM_MSB_OFF	0x09	/* DMA Address Register MSB		*/
#define	GA_DAM_LSB_OFF	0x0a	/* DMA Address Register LSB		*/
#define	GA_VPTR2_OFF	0x0b	/* Vector Pointer Register high		*/
#define	GA_VPTR1_OFF	0x0c	/* Vector Pointer Register mid		*/
#define	GA_VPTR0_OFF	0x0d	/* Vector Pointer Register low		*/
#define	GA_RF_MSB_OFF	0x0e	/* Register File (FIFO) MSB		*/
#define	GA_RF_LSB_OFF	0x0f	/* Register File (FIFO) LSB		*/

/**************************************************************************
 *  Composit Read/Write Gate Array structure
 */
typedef	struct {
    union {
    	struct ga_write	write;	/* Registers for write ops		*/
    	struct ga_read	read;	/* Registers for read ops		*/
    } op;
} ga_t;

/**************************************************************************
 *  Composit IEA board structure.
 */
typedef	struct {
	union {
		nic_t	nic;
		uchar_t	eprom[ 16 ];
	} map;
	uchar_t	gap[ GA_OFFSET - sizeof(nic_t)];
	ga_t	ga;
} iea_map_t;

/**************************************************************************
 *  NIC frame information header that starts each frame.
 */
typedef struct {
	uchar_t		status;	/* frame status			*/
	uchar_t		next;	/* pointer/offset to next frame */
	ushort_t	len;	/* length of frame including CRC*/
} nic_info;

/**************************************************************************
 *  NIC Ethernet address
 */
typedef union {
	uchar_t		bytes[ 6 ];
	ushort_t	words[ 3 ];
} eaddr_t;

/**************************************************************************
 *  NIC Ethernet header
 */
typedef	struct {
	eaddr_t		dst;		/* destination address		*/
	eaddr_t		src;		/* source address		*/
	ushort_t	len_type;	/* len/type field		*/
} ether_hdr_t;

/**************************************************************************
 *  Combined NIC and MAC header.
 */
typedef struct {
	nic_info	nic;
	ether_hdr_t	mac;
} comb_hdr;

/**************************************************************************
 *  Some useful defines and macros
 */
#define	NIC_SELECT		0x00	/* Select NIC registers		*/
#define	EPROM_LOW_SELECT	0x04	/* Select lower halve of EPROM	*/ 
#define	EPROM_HIGH_SELECT	0x08	/* Select higher halve of EPROM */ 

#define	nic_write	map.nic.op.write.page
#define	nic_read	map.nic.op.read.page
#define	ga_write	ga.op.write
#define	ga_read		ga.op.read

#define	NIC_WRITE(base, member, val)	\
		outb((int)&(((iea_map_t*)(base))->nic_write.member), val)
#define	NIC_READ(base, member)		\
		inb((int)&(((iea_map_t*)(base))->nic_read.member))
#define	GA_WRITE(base, member,val)	\
		outb((int)&(((iea_map_t*)(base))->ga_write.member), val)
#define	GA_READ(base, member)		\
		inb((int)&(((iea_map_t*)(base))->ga_read.member))
#define	EADDR_READ(base, index)		\
		inb((int)&(((iea_map_t*)(base))->map.eprom[ index ]))

#define	LEN_TO_PAGES(len)	((len >> 8) + ((len & 0xff) ? 1 : 0))

/*
 * Data structures to support multicast addresses.
 */
struct ie6maddr {
	unsigned char filterbit;
	unsigned char entry[6];
};

typedef struct bdd {
	struct ie6maddr *ie6_multip;
	struct ie6maddr *ie6_multiaddrs;
} bdd_t;

union crc_reg {
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

#endif	/* _IO_DLPI_ETHER_IE6_H */
