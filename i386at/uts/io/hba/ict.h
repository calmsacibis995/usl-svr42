/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/ict.h	1.5"
#ident	"$Header: $"

#define	ICT_DEBUG	1
#define ICTSTAT	1

struct ICT_CONTROL {
	/***            ****
	** Port Addresses **
	****            ***/
	int	ict_status;
	int	ict_control;
	int	ict_command;
	int	ict_data;

	/***                          ****
	** Control Port Bit Definitions **
	****                          ***/
	char	ict_online;
	char	ict_reset;
	char	ict_request;
	char	ict_dma1_2;
	char	ict_dma_3;
	char	ict_intr_enable;
	char	ict_dma_enable;

	/***                         ****
	** Status Port Bit Definitions **
	****                         ***/
	char	ict_ready;
	char	ict_exception;
	char	ict_direction;

	/***                         ****
	** Set the Power On Reset time **
	****                         ***/
	int	ict_por_delay;

	/***                                        ****
	** The next two entries are for the supported **
	** ARCHIVE controllers.                       **
	****                                        ***/
	int	ict_dma_go;
	int	ict_reset_dma;

	/***                                      ****
	** Type of controller, currently supported: **
	**	0 -> Wangtek ISA                    **
	**	1 -> Archive ISA                    **
	****                                      ***/
	int	ict_type;

};

/***
** Supported Controller Boards
***/
#define	ICT_WANGTEK	0
#define	ICT_ARCHIVE	1
#define	ICT_MCA_ARCHIVE	2

/***                                          ****
** Wangtek Hardware Status Port Bit Definitions **
****                                          ***/

#define	W_S_READY	0x01	/** Read Status Bit      **/
#define	W_S_EXCEPTION	0x02	/** Exception Status Bit **/
#define	W_S_DIRECTION	0x03	/** Direction Status Bit **/

/***                                  ****
** Wangtek Control Port Bit Definitions **
****                                  ***/

#define	W_ONLINE	0x01	/** Offset for OnLine Output       **/
#define	W_RESET		0x02	/** Offset for Reset Line Output   **/
#define	W_REQUEST	0x04	/** Offset for Request Line Output **/
#define	W_DMA1_2	0x06	/** Offset for DMA Request 1 & 2   **/
#define	W_DMA3		0x08	/** Offset for DMA Request 3       **/
#define	W_CLEAR		0x00	/** Clear All                      **/

/***          ****
** QIC Commands **
****          ***/
#define ICT_REWIND	0x21
#define ICT_ERASE	0x22
#define	ICT_RETENSION	0x24
#define ICT_SEL24	0x27
#define ICT_SEL120	0x28
#define ICT_SEL150	0x29
#define ICT_WRITE	0x40
#define ICT_WRFILEM	0x60
#define ICT_READ	0x80
#define ICT_RDFILEM	0xA0
#define ICT_RD_STATUS	0xC0

/***               ****
** Emulated Commands **
***                ***/
#define	ICT_MSENSE	0x100

/***                                          ****
** Archive Hardware Status Port Bit Definitions **
****                                          ***/

#define	A_S_READY	0x0100	/** Read Status Bit      **/
#define	A_S_EXCEPTION	0x0040	/** Exception Status Bit **/
#define	A_S_DIRECTION	0x0003	/** Direction Status Bit **/
#define	A_S_DONE	0x0020
#define	A_S_NINT	0x0200

/***                                ****
** Controller Status Bits Definitions **
****                                ***/
#define	ICT_POR		0x01
#define	ICT_SBYTE1	0x80

#define ICT_SUCCESS	0
#define	ICT_FAILURE	1

#define	ICT_STATBUF_SZ	6

#define ICT_WAIT_LIMIT	500000
#define ICT_WR_MAXTOUT	90	/* Maximum seconds to wait for write */
				/* interrupt when at load point	*/
				/* (worst case known is WANGTEK PC02) */

extern int ict_bdinit();
extern int ict_drvinit();
extern int ict_cmd();
extern struct gdev_parm_block *ict_int();

#define	ICT_NOT_READY	5

#define	DMA_READ	0x45
#define	DMA_WRITE	0x49
#define	DMA_STR		0x0A
#define	DMA_STAT	0x08

#define	ICT_ASSERT( X )	ict_cntrl_mask |= (char)( X ); \
			outb( ict_cntrl.ict_control, ict_cntrl_mask );

#define	ICT_DEASSERT( X )	ict_cntrl_mask &= (char)~(X); \
				outb( ict_cntrl.ict_control, ict_cntrl_mask );

			/* Controller Exceptions */
#define	ICT_NCT	1		/* No Cartridge */
#define	ICT_EOF	2		/* Read a Filemark */
#define	ICT_EOM	3		/* End of Media */
#define	ICT_WRP	4		/* Write Protected */
#define	ICT_DFF	5		/* Device Fault Flag */
#define	ICT_RWA	6		/* Read or Write Abort */
#define	ICT_BBX	7		/* Read Error, Bad Block Xfer */
#define	ICT_FBX	8		/* Read Error, Filler Block Xfer */
#define	ICT_NDT	9		/* Read Error, No Data */
#define	ICT_NDE	10		/* Read Error, No Data & EOM */
#define	ICT_ILC	11		/* Illegal Command */
#define	ICT_PRR	12		/* Power On/Reset */
#define	ICT_MBD	13		/* Marginal Block Detected */
#define	ICT_UND	14		/* Undetermined Error */

#ifdef ICTSTAT
struct ICT_STAT {
	int ict_savecmd;
	int ict_drvcmd;
	int ict_errabort;
	int ict_writecnt;
	int ict_readcnt;
	int ict_nblocks;
	int ict_prev_nblocks;
};
#endif
