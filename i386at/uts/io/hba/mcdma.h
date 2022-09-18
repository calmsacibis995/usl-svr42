/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_HBA_MCDMA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_MCDMA_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/hba/mcdma.h	1.2"
#ident	"$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1989 INTERACTIVE Systems Corporation
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


/*
 * Defines for PS/2 DMA controllers.
 */

/*
 * PS/2 DMA extended mode definitions
 *
 */
#define PS2DMA_CTL	0x18	/* function register		*/
#define PS2DMA_DAT	0x1A	/* execute function register	*/

#define PS2DMA_SMK	0x90	/* set mask register		*/
#define PS2DMA_MAR	0x20	/* memory address register	*/
#define PS2DMA_TCR	0x40	/* transfer count register	*/
#define PS2DMA_WMR	0x70	/* write mask register		*/
#define PS2DMA_CMK	0xA0	/* clear mask register		*/


#define PS2DMA_RD	0x44	/* 16-bit read mode		*/
#define PS2DMA_WR	0x4C	/* 16-bit write mode		*/

#endif /* _IO_HBA_MCDMA_H */
