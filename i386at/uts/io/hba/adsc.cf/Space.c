/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/adsc.cf/Space.c	1.6"
#ident	"$Header: $"


#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/ad1542.h>
#include "config.h"

int	sc_hacnt = ADSC_CNTLS;	/* Total number of controllers	*/
int	sc_lowat = 2;		/* LU queue low water mark	*/

struct	ver_no    sdi_ver;

int	adsc_ctlr_id = SCSI_ID;	/* HBA's SCSI id number		*/

struct	hba_idata	adscidata[ADSC_CNTLS]	= {
#ifdef	ADSC_0
	{ 1, "(adsc,1st) Adaptec SCSI HBA",
	  7, ADSC_0_SIOA, ADSC_0_CHAN, ADSC_0_VECT, ADSC_0, 0 }
#endif
#ifdef	ADSC_1
	,
	{ 1, "(adsc,2nd) Adaptec SCSI HBA",
	  7, ADSC_1_SIOA, ADSC_1_CHAN, ADSC_1_VECT, ADSC_1, 0 }
#endif
#ifdef	ADSC_2
	,
	{ 1, "(adsc,3rd) Adaptec SCSI HBA",
	  7, ADSC_2_SIOA, ADSC_2_CHAN, ADSC_2_VECT, ADSC_2, 0 }
#endif
#ifdef	ADSC_3
	,
	{ 1, "(adsc,4th) Adaptec SCSI HBA",
	  7, ADSC_3_SIOA, ADSC_3_CHAN, ADSC_3_VECT, ADSC_3, 0 }
#endif
};

int	adsc_cntls	= ADSC_CNTLS;

/*
 * If this is set to 0, no local scatter/gather will be done in the driver
 * however, there will be scatter/gather done by the kernel.
 * Default = 1
 * RAN 03/31/92
 */
int	adsc_sg_enable = 1;

/*
 * This variable controls the amount of time the host adapter will spend on the
 * AT bus.  The value is in micro-seconds and should not be set higher than 8
 * to allow proper floppy operation.  It is also recommended the value not be
 * set below 4.  This variable has no effect on EISA host adapters.
 * Default = 7
 * RAN 03/31/92
 */
int	adsc_buson = 7;

/*
 * This variable controls the amount of time the host adapter will spend off
 * the AT bus after the bus on time has expired.  It should be set in
 * increments of 4, but not higher than 64.  This variable has no effect on
 * EISA host adapters.
 * Default = 4
 * RAN 03/31/92
 */
int	adsc_busoff = 4;

/*
 * This variable controls the host adapter DMA transfer rate.  By default
 * the host adapter can be set via jumpers to speeds 5, 5.7, 6.7, and 8.0
 * MBytes/sec.  The default jumper setting is 5.0MB/sec.  The following
 * settings are allowed via this variable that will override the jumper
 * settings.
 *	0 = 5.0MBytes/sec
 *	1 = 6.7MBytes/sec
 * 	2 = 8.0MBytes/sec
 *	3 = 10.0MBytes/sec
 *	4 = 5.7MBytes/sec (AHA-1540A and later)
 *
 * Optionally, the DMA rate can be adjusted to many different values by
 * setting the high bit (7) and using the following table.
 *		     Read      Write
 *		     Pulse     Pulse
 *		     Width     Width
 *	Bits	7    6 5 4  3  2 1 0	MBytes/sec
 *	------------------------------------------
 *		1    0 0 0  x  0 0 0	10.0
 *		1    0 0 1  x  0 0 1	 6.7
 *		1    0 1 0  x  0 1 0	 5.0
 *		1    0 1 1  x  0 1 1	 4.0
 *		1    1 0 0  x  1 0 0	 3.3
 *		1    1 0 1  x  1 0 1	 2.8
 *		1    1 1 0  x  1 1 0	 2.5
 *		1    1 1 1  x  1 1 1	 2.2
 *	------------------------------------------
 *	Bit 3 (x) sets the strobe pulse width.
 *			    0 = 100ns
 *			    1 = 150ns
 *	------------------------------------------
 *
 * The Read, Write, and Strobe pulse widths can be mixed and matched to get
 * various read/write rates.  For instance, say you wanted a read rate of
 * 6.7MB/sec and a write rate of 5.0MB/sec and a strobe width of 100 ns.
 * You would use 1 0 0 1 0 0 1 0 (remember the high bit must be set).
 * 10010010 = 146 (decimal) or 0x92 (hex).
 * You should know setting this to a rate you have not tested in your system
 * may result in a non-operational kernel.  The best way to determine how fast
 * your system can allow the host adapter DMA rate can be is to use a floppy,
 * set the DMA rate jumper on the adapter, run debug and execute (g=dc00:9)
 * in the BIOS of the adapter (assuming the BIOS is addressed at dc00:9).
 * If this test passes, you have some assurance the system will run at the
 * rate you jumpered the adapter to.  If it fails, do not attempt to alter
 * this value to a higher value.  This variable has no effect on EISA host
 * adapters.
 * Default = 0 (5.0MB/sec)
 * RAN 03/31/92
 */
int	adsc_dma_rate = 0;
