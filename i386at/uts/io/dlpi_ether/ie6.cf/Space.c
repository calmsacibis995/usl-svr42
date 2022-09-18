/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. xxxxxx";

#ident	"@(#)uts-x86at:io/dlpi_ether/ie6.cf/Space.c	1.6"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <sys/ie6.h>
#include <config.h>

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  IE6 board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 *  The CABLE_TYPE define determines the ethernet cable type.  A value of 1
 *  indicates the ethernet controller is to be attached to thick ethernet
 *  cable (AUI).  A value of 0 indicates a thin ethernet cable (BNC).
 */
#define	CABLE_TYPE	0

/*
 *  The STREAMS_LOG define determines if STREAMS tracing will be done in the
 *  driver.  A non-zero value will allow the strace(1M) command to follow
 *  activity in the driver.  The driver ID used in the strace(1M) command is
 *  equal to the ENET_ID value (generally 2101).
 *
 *  NOTE:  STREAMS tracing can greatly reduce the performance of the driver
 *         and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0

/*
 *  The IFNAME define determines the name of the the internet statistics
 *  structure for this driver and only has meaning if the inet package is
 *  installed.  It should match the interface prefix specified in the strcf(4)
 *  file and ifconfig(1M) command used in rc.inet.  The unit number of the
 *  interface will match the board number (i.e emd0, emd1, emd2) and is not
 *  defined here.
 */
#define	IFNAME	"ie6"

/*
 *  Definition to support multicast addresses.
 */
#define	MAXMULTI	8

/********************** STOP!  DON'T TOUCH THAT DIAL ************************
 *
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere motals.
 */
int		ie6boards = IE6_CNTLS;
int		ie6strlog = STREAMS_LOG;
char		*ie6_ifname = IFNAME;
DL_sap_t	ie6saps[ N_SAPS * IE6_CNTLS ];
int		ie6_multisize = MAXMULTI;
struct ie6maddr	ie6multiaddrs[ IE6_CNTLS * MAXMULTI ];
DL_bdconfig_t	ie6config[ IE6_CNTLS ] = {
#ifdef IE6_0
	{
		IE6_CMAJOR_0,		/* Major number			*/
		IE6_0_SIOA,		/* Start of base I/O address	*/
		IE6_0_EIOA,		/* End of base I/O address	*/
		IE6_0_SCMA,		/* Start of base memory address	*/
		IE6_0_ECMA,		/* End of base memory address	*/
		IE6_0_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef IE6_1
	{
		IE6_CMAJOR_1,		/* Major number			*/
		IE6_1_SIOA,		/* Start of base I/O address	*/
		IE6_1_EIOA,		/* End of base I/O address	*/
		IE6_1_SCMA,		/* Start of base memory address	*/
		IE6_1_ECMA,		/* End of base memory address	*/
		IE6_1_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef IE6_2
	{
		IE6_CMAJOR_2,		/* Major number			*/
		IE6_2_SIOA,		/* Start of base I/O address	*/
		IE6_2_EIOA,		/* End of base I/O address	*/
		IE6_2_SCMA,		/* Start of base memory address	*/
		IE6_2_ECMA,		/* End of base memory address	*/
		IE6_2_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef IE6_3
	{
		IE6_CMAJOR_3,		/* Major number			*/
		IE6_3_SIOA,		/* Start of base I/O address	*/
		IE6_3_EIOA,		/* End of base I/O address	*/
		IE6_3_SCMA,		/* Start of base memory address	*/
		IE6_3_ECMA,		/* End of base memory address	*/
		IE6_3_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
};

/*
 *  If there are multiple IE6 boards in the same system, make sure there is
 *  a comma separated value for each one.
 */
int	ie6_cable_type[ IE6_CNTLS ] = {
#ifdef IE6_0
		CABLE_TYPE,
#endif
#ifdef IE6_1
		CABLE_TYPE,
#endif
#ifdef IE6_2
		CABLE_TYPE,
#endif
#ifdef IE6_3
		CABLE_TYPE,
#endif
};

#ifdef IP
int	ie6inetstats = 1;
#else
int	ie6inetstats = 0;
#endif
