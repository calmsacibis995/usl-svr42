/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_HBA_GENTAPE_H	/* wrapper symbol for kernel use */
#define _IO_HBA_GENTAPE_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/gentape.h	1.2"
#ident	"$Header: $"

/*
 * Definitions for Generic Tape Driver
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1988 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#define TAPE_MAJOR      41

/*
 * The following definitions apply to the low-order 4 bits of the tape
 * minor device numbers.
 */

#define TDEV_FLAGMASK   0x0f    /* mask to get low-order 4 bits */
#define TDEV_REWIND     8       /* Device should be rewound on close */
				/* If not set, close on write will write a */
				/* filemark and leave the tape at that */
				/* position.  Close on read will forward- */
				/* space to the next filemark. */
#define TDEV_IMMED      4       /* Tape motion commands should return */
				/* immediately.  If not set, IOCTL calls or */
				/* closings will return AFTER any associated */
				/* tape motion has completed */
#define TDEV_DENMASK    3       /* Mask for density code, as follows: */
				/* CODE         9-TRK BPI       CART     */
#define TPDEN_DEFAULT   0       /*  0            <default>   <default>   */
#define TPDEN_LOW       1       /*  1             800       9-trk (60MB) */
#define TPDEN_MED       2       /*  2            1600     15-trk (125MB) */
#define TPDEN_HIGH      3       /*  3            6250     18-trk (150MB) */

/*
 * NOTE: All the above values selected by the device code are DEFAULTS.
 *       They can be changed by IOCTL calls, below
 */


/*
 * Tape IOCTL calls...
 */

#define TIOC            ('T'<<8)
#define TC_SEOF         (TIOC | 1)      /* Seek to EOF (count arg) */
#define TC_REWIND       (TIOC | 2)      /* Rewind tape (no arg) */
#define TC_WFM          (TIOC | 3)      /* Write File Mark (count arg) */
#define TC_ERASE        (TIOC | 4)      /* Erase entire tape (no arg) */
#define TC_RETENSION    (TIOC | 6)      /* Retension tape (no arg) */
#define TC_WAIT         (TIOC | 10)     /* Wait for long-term op (no arg) */
					/* Valid only after a long-term */
					/* operation with TDEV_IMMED set */
#define TC_GETPARMS     (TIOC | 11)     /* Get tape parameters (arg below) */


/*
 * Tape IOCTL argument block for TC_GETPARMS.
 * Address of one of these is passed as ioctl arg and is filled with
 * with values by TC_GETPARMS.
 *
 */

struct tc_parms
	{
	ushort  tcp_speed;      /* Speed of tape device */
	ushort  tcp_flags;      /* Tape Flags.  Low 4 bits are same as for */
				/* minor device codes above ('TDEV_*'). */
	ushort  tcp_spare;      /* for future expansion */
	ushort  tcp_medtype;    /* Cartridge medium type (GETPARMS only) */
	ulong   tcp_recsiz;     /* Record size if fixed-size device, max */
				/* record size if variable-size device */
	};

/* values for tcp_speed field */
#define TCPS_DEFAULT    0       /* Default (SET) or unknown (GET) speed */
#define TCPS_LOW        1       /* Low speed */
#define TCPS_MED        2       /* Medium speed */
#define TCPS_HIGH       3       /* High speed */

/* value bits for tcp_flags (from dpb_drvflags) */
#define TCPF_DENMASK    TDEV_DENMASK
#define TCPF_IMMED      TDEV_IMMED
#define TCPF_REWIND     TDEV_REWIND
#define TCPF_FIXED      0x10    /* Device uses fixed-size records (usually */
				/* a cartridge tape) */
#define TCPF_LONGOP     0x20    /* A long-term operation is in progress */
#define TCPF_ATEOT      0x40    /* Tape is at End-of-Tape point */
#define TCPF_ATEOF      0x80    /* Tape is at End-of-File point */
#define TCPF_RDONLY     0x100   /* Tape is Read Only */
#define TCPF_IOCTL      0x2000  /* Last operation on the dev was IOCTL */
				/* (As opposed to READ or WRITE) */


extern struct gdev_cfg_entry tape_cfg_tbl[];
extern ushort tape_cfg_entries;

#endif /* _IO_HBA_GENTAPE_H */
