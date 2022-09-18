/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_DPT_H	/* wrapper symbol for kernel use */
#define _IO_HBA_DPT_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/dpt.h	1.7"
#ident  "$Header: dpt.h 1.1 91/11/01 $"

/*      Copyright (c) 1988, 1989  Intel Corporation     */
/*      All Rights Reserved     */

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*	Copyright (c) 1990 Distributed Processing Technology	*/
/*      All Rights Reserved     */

/*************************************************************************
**                                                                      **
**  The host adapter major/minor device numbers are defined as          **
**                                                                      **
**         MAJOR      MINOR                                             **
**      | MMMMMMMM | nnndddss |                                         **
**                                                                      **
**         M ==> Assigned by idinstall (major device number).           **
**         n ==> Relative Host Adapter number.                   (0-7)  **
**         d ==> target device ID, Drive or Bridge controller ID.(0-7)  **
**         s ==> sub device LUN, for bridge controllers.         (0-3)  **
**                                                                      **
**  Defines to extract the above information follow.                    **
**                                                                      **
*************************************************************************/

#define MINOR_DEV(dev)		getminor(dev)

/*************************************************************************
**                 General Implementation Definitions                   **
*************************************************************************/
#define MAX_EQ   MAX_TCS * MAX_LUS      /* Max equipage per controller  */
#define NDMA		12		/* Number of DMA lists		*/
#define NCPS            64              /* Number of command packets    */
#define SCSI_ID		7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif
#define BYTE            unsigned char

#define DMA0_3MD	0x000B			/* 8237A DMA Mode Reg (0-3)   */
#define DMA4_7MD	0x00D6			/* 8237A DMA Mode Reg (4-7)   */
#define CASCADE_DMA	0x00C0			/* Puts DMA in Cascade Mode   */
#define DMA0_3MK	0x000A			/* 8237A DMA mask register    */
#define DMA4_7MK	0x00D4			/* 8237A DMA mask register    */


/*************************************************************************
**             Controller IO Register Offset Definitions                **
*************************************************************************/
#define HA_COMMAND      0x07            /* Command register             */
#define HA_STATUS       0x07            /* Status register              */
#define HA_DMA_BASE     0x02            /* LSB for DMA Physical Address */
#define HA_ERROR        0x01            /* Error register               */
#define HA_DATA         0x00            /* Data In/Out register         */
#define HA_AUX_STATUS   0x08            /* Auxiliary Status Reg on 2012 */

#define HA_AUX_BUSY     0x01            /* Aux Reg Busy bit.            */
#define HA_AUX_INTR     0x02            /* Aux Reg Interrupt Pending.   */

#define HA_ST_ERROR     0x01            /* HA_STATUS register bit defs  */
#define HA_ST_INDEX     0x02
#define HA_ST_CORRCTD   0x04
#define HA_ST_DRQ       0x08
#define HA_ST_SEEK_COMP 0x10
#define HA_ST_WRT_FLT   0x20
#define HA_ST_READY     0x40
#define HA_ST_BUSY      0x80

#define HA_ER_DAM       0x01            /* HA_ERROR register bit defs   */
#define HA_ER_TRK_0     0x02
#define HA_ER_ABORT     0x04
#define HA_ER_ID        0x10
#define HA_ER_DATA_ECC  0x40
#define HA_ER_BAD_BLOCK 0x80

/*************************************************************************
**                   Controller Commands Definitions                    **
*************************************************************************/
#define CP_READ_CFG_PIO 0xF0
#define CP_PIO_CMD      0xF2
#define CP_TRUCATE_CMD  0xF4
#define CP_EATA_RESET   0xF9
#define CP_READ_CFG_DMA 0xFD
#define CP_DMA_CMD      0xFF

#define CP_ABORT_MSG    0x06

/*************************************************************************
**                EATA Command/Status Packet Definitions                **
*************************************************************************/
#define HA_DATA_IN      0x80
#define HA_DATA_OUT     0x40
#define HA_SCATTER      0x08
#define HA_AUTO_REQ_SEN 0x04
#define HA_HBA_INIT     0x02
#define HA_SCSI_RESET   0x01

#define HA_ERR_SELTO	0x01			/* ha stat definitions	*/
#define HA_ERR_CMDTO	0x02
#define HA_ERR_RESET	0x03
#define HA_ERR_INITPWR	0x04
#define HA_ERR_SHUNG	0x08

#define HA_STATUS_MASK  0x7F
#define HA_IDENTIFY_MSG 0x80
#define HA_DISCO_RECO   0x40			/* Disconnect/Reconnect	*/

#define SUCCESS         0x01                     /* Successfully completed  */
#define FAILURE         0x02                     /* Completed with error    */
#define START           0x01                     /* Start the CP command    */
#define ABORT           0x02                     /* Abort the CP command    */

struct cp_bits {
	BYTE SReset:1;
	BYTE HBAInit:1;
	BYTE ReqSen:1;
	BYTE Scatter:1;
	BYTE Resrvd:2;
	BYTE DataOut:1;
	BYTE DataIn:1;
};

struct EATACommandPacket {
	union {
	  struct cp_bits bit;
	  unsigned char byte;
	} CPop;
	BYTE    ReqLen;
	BYTE    Unused[5];
	BYTE    CPID;
	BYTE    CPmsg0;
	BYTE    CPmsg1;
	BYTE    CPmsg2;
	BYTE    CPmsg3;
	BYTE    CPcdb[12];
	BYTE    CPdataLen[4];
	union {
	   struct dpt_ccb *vp;         /*  Command Packet Vir Address. */
	   BYTE     va[4];             /*  Command Packet Other Info.  */
	} CPaddr;
	paddr_t CPdataDMA;
	paddr_t CPstatDMA;
	paddr_t CP_ReqDMA;
};

typedef struct Status_Packet {
	BYTE    SP_Controller_Status;
	BYTE    SP_SCSI_Status;
	BYTE    unused[2];
	BYTE    SP_Inv_Residue[4];
	union {
	   struct dpt_ccb *vp;         /*  Command Packet Vir Address. */
	   BYTE     va[4];             /*  Command Packet Other Info.  */
	} CPaddr;
	BYTE    SP_ID_Message;
	BYTE    SP_Que_Message;
	BYTE    SP_Tag_Message;
	BYTE    SP_Messages[9];
} scsi_stat_t;


/*******************************************************************************
** ReadConfig data structure - this structure contains the EATA Configuration **
*******************************************************************************/
struct RdConfig {
	BYTE ConfigLength[4];		/* Len in bytes after this field.     */
	BYTE EATAsignature[4];
	BYTE EATAversion;

	BYTE OverLapCmds:1;		/* TRUE if overlapped cmds supported  */
	BYTE TargetMode:1;		/* TRUE if target mode supported      */
	BYTE TrunNotNec:1;
	BYTE MoreSupported:1;
	BYTE DMAsupported:1;		/* TRUE if DMA Mode Supported	      */
	BYTE DMAChannelValid:1;		/* TRUE if DMA Channel Valid.         */
	BYTE ATAdevice:1;
	BYTE HBAvalid:1;		/* TRUE if HBA field is valid	      */

	BYTE PadLength[2];
	BYTE HBA[4];
	BYTE CPlength[4];		/* Command Packet Length 	      */
	BYTE SPlength[4];		/* Status Packet Length		      */
	BYTE QueueSize[2];		/* Controller Que depth		      */
	BYTE SG_Size[4];

	BYTE IRQ_Number:4;		/* IRQ Ctlr is on ... ie 14,15,12     */
	BYTE IRQ_Trigger:1;		/* 0 =Edge Trigger, 1 =Level Trigger  */
	BYTE Secondary:1;		/* TRUE if ctlr not parked on 0x1F0   */
	BYTE DMA_Channel:2; 		/* DMA Channel used if PM2011         */
	BYTE pad[512 - 31];
};

/*
 * Controller Command Block
 */
struct dpt_ccb {
	union {                        /*** EATA Packet sent to ctlr ***/
	  struct cp_bits bit;          /*                              */
	  unsigned char byte;          /*  Operation Control bits.     */
	} CPop;                        /*                              */
	BYTE ReqLen;                   /*  Request Sense Length.       */
	BYTE Unused[5];                /*  Reserved                    */
	BYTE CPID;                     /*  Target SCSI ID              */
	BYTE CPmsg0;                   /*  Identify/DiscoReco... Msg   */
	BYTE CPmsg1;                   /*                              */
	BYTE CPmsg2;                   /*                              */
	BYTE CPmsg3;                   /*                              */
	BYTE CPcdb[12];                /*  Embedded SCSI CDB.          */
	BYTE CPdataLen[4];             /*  Transfer Length.            */
	union {
	   struct dpt_ccb *vp;             /*  Command Packet Vir Address. */
	   BYTE     va[4];             /*  Command Packet Other Info.  */
	} CPaddr;
	paddr_t CPdataDMA;             /*  Data Physical Address.      */
	paddr_t CPstatDMA;             /*  Status Packet Phy Address.  */
	paddr_t CP_ReqDMA;             /*  ReqSense Data Phy Address.  */

	BYTE CP_OpCode;
	BYTE CP_Controller_Status;
	BYTE CP_SCSI_Status;
	BYTE sense[sizeof(struct sense)];/* Sense data                  */
	paddr_t         c_addr;         /* CB physical address          */
	unsigned short	c_index;	/* CB array index		*/
	unsigned short	c_active;	/* Command sent to controller	*/
	time_t		c_start;	/* Timestamp for start of cmd	*/
	time_t		c_time;		/* Timeout count (msecs)	*/
	struct sb      *c_bind;		/* Associated SCSI block	*/
	struct dpt_ccb *c_next;		/* Pointer to next free CB	*/
};

#define MAX_CMDSZ	12

#define	NO_ERROR	0x00		/* No adapter detected error	*/
#define	NO_SELECT	0x11		/* Selection time out		*/
#define	TC_PROTO	0x14		/* TC protocol error		*/

#define MAX_DMASZ       32
#define pgbnd(a)        (NBPP - ((NBPP - 1) & (int)(a)))

typedef struct {
	union {
	    BYTE Addr[4];
	    ulong l;
	} Phy;
	union {
	    BYTE bytes[4];
	    ulong l;
	} Len;
} SG_vect;

struct ScatterGather {
	unsigned int SG_size;                /* List size (in bytes)        */
	struct   ScatterGather *d_next;      /* Points to next free list    */
	SG_vect  d_list[MAX_DMASZ];
};

typedef struct ScatterGather dpt_dma_t;

/*
 * SCSI Request Block structure
 */
struct dpt_srb {
	struct xsb     *sbp;		/* Target drv definition of SB	*/
	struct dpt_srb *s_next;		/* Next block on LU queue	*/
	struct dpt_srb *s_priv;		/* Private ptr for dynamic alloc*/
					/* routines DO NOT USE or MODIFY*/
	struct dpt_srb *s_prev;		/* Previous block on LU queue	*/
	dpt_dma_t      *s_dmap;		/* DMA scatter/gather list	*/
	paddr_t         s_addr;         /* Physical data pointer        */
	BYTE            s_CPopCtrl;     /* Additional Control info	*/
};

typedef struct dpt_srb dpt_sblk_t;

/*
 * Logical Unit Queue structure
 */
struct dpt_scsi_lu {
	struct dpt_srb *q_first;	/* First block on LU queue	*/
	struct dpt_srb *q_last;		/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int		q_count;	/* Outstanding job counter	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long            q_param;        /* Target driver event param    */
	int		q_active;	/* Number of concurrent jobs	*/
};

#define	QBUSY		0x01
#define	QSUSP		0x04
#define	QSENSE		0x08		/* Sense data cache valid */
#define	QPTHRU		0x10

#define queclass(x)	((x)->sbp->sb.sb_type)
#define	QNORM		SCB_TYPE

/*
 * Host Adapter structure
 */
struct dpt_scsi_ha {
	unsigned short  ha_state;           /* Operational state           */
	unsigned short  ha_id;              /* Host adapter SCSI id        */
	int             ha_vect;            /* Interrupt vector number     */
	unsigned long   ha_base;            /* Base I/O address            */
	int             ha_npend;           /* # of jobs sent to HA        */
	struct dpt_ccb  *ha_ccb;   		    /* Controller command blocks   */
	struct dpt_ccb  *ha_cblist;         /* Command block free list     */
	struct dpt_scsi_lu  *ha_dev;  		/* Logical unit queues         */
};

#define C_SANITY	0x8000

/*
**	Macros to help code, maintain, etc.
*/

#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,t,l)	dpt_sc_ha[c].ha_dev[SUBDEV(t,l)]
#define SLOT_ID_ADDR(s)	(s * 4096 + 0xC80)
#define SLOT_BASE_IO_ADDR(s)	(SLOT_ID_ADDR(s) + 8)

#endif /* _IO_HBA_DPT_H */
