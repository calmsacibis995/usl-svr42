/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386at/ktool/idtools/mdep.h	1.4"
#ident	"$Header:"

/*
 * Machine-specific definitions for ID/TP.
 */

/* System file parameter ranges */
#define CLK_IVN	0		/* clock IVN */
#define CLK_IPL	8		/* clock IPL */
#define	SIVN	0		/* start of IVN range */
#define EIVN	31		/* end of IVN range (inclusive) */
#define SIPL	1		/* start of IPL range */
#define EIPL	8		/* end of IPL range (inclusive) */
#define SITYP	0		/* start of ITYP range */
#define EITYP	4		/* end of ITYP range (inclusive) */
#define	SIOA	0x0L		/* start of I/O address range */
#define EIOA	0xFFFFL		/* end of I/O address range (inclusive) */
#define	SCMA	0x10000L	/* start of controller memory address range */
#define DMASIZ	15		/* highest dma channel number permitted */


/* Legal Master file flags */
#define ALL_MFLAGS	"abcdefhikmnorstuCDFGHMNORS-"
/* Obsolete Master file flags; ignored */
#define OLD_MFLAGS	"ainsGHMNR"

/* Specific machine-specific Master file flags */
#define IOOVLOK	'O'		/* IOA regions can overlap		*/
#define	DMASHR	'D'		/* can share DMA channel		*/

/* Macro for idcheck to determine if I/O overlap is allowed */
#define IO_OVERLAP_OK(mdev)	(INSTRING((mdev)->mflags, IOOVLOK))
