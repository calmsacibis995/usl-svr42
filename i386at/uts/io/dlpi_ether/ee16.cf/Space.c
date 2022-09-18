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

#ident	"@(#)uts-x86at:io/dlpi_ether/ee16.cf/Space.c	1.4"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EE16  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 *  The CABLE_TYPE define determines the ethernet cable type.  A value of 1
 *  indicates the ethernet controller is to be attached to thick ethernet
 *  cable (AUI).  A value of 0 indicates a thin ethernet cable (BNC).
 */
#define	CABLE_TYPE	1

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
#define	IFNAME	"ee16"

/********************** STOP!  DON'T TOUCH THAT DIAL ************************
 *
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere motals.
 */
int	ee16cable_type = CABLE_TYPE;
int		ee16boards = EE16_CNTLS;
int		ee16strlog = STREAMS_LOG;
char		*ee16_ifname = IFNAME;
DL_sap_t	ee16saps[ N_SAPS * EE16_CNTLS ];
DL_bdconfig_t	ee16config[ EE16_CNTLS ] = {
#ifdef EE16_0
	{
		EE16_CMAJOR_0,		/* Major number			*/
		EE16_0_SIOA,		/* Start of base I/O address	*/
		EE16_0_EIOA,		/* End of base I/O address	*/
		EE16_0_SCMA,		/* Start of base memory address	*/
		EE16_0_ECMA,		/* End of base memory address	*/
		EE16_0_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef EE16_1
	{
		EE16_CMAJOR_1,		/* Major number			*/
		EE16_1_SIOA,		/* Start of base I/O address	*/
		EE16_1_EIOA,		/* End of base I/O address	*/
		EE16_1_SCMA,		/* Start of base memory address	*/
		EE16_1_ECMA,		/* End of base memory address	*/
		EE16_1_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef EE16_2
	{
		EE16_CMAJOR_2,		/* Major number			*/
		EE16_2_SIOA,		/* Start of base I/O address	*/
		EE16_2_EIOA,		/* End of base I/O address	*/
		EE16_2_SCMA,		/* Start of base memory address	*/
		EE16_2_ECMA,		/* End of base memory address	*/
		EE16_2_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef EE16_3
	{
		EE16_CMAJOR_3,		/* Major number			*/
		EE16_3_SIOA,		/* Start of base I/O address	*/
		EE16_3_EIOA,		/* End of base I/O address	*/
		EE16_3_SCMA,		/* Start of base memory address	*/
		EE16_3_ECMA,		/* End of base memory address	*/
		EE16_3_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
};

/*
 *  If there are multiple EE16 boards in the same system, make sure there is
 *  a comma separated value for each one.
 */
int	ee16_cable_type[ EE16_CNTLS ] = {
#ifdef EE16_0
		CABLE_TYPE,
#endif
#ifdef EE16_1
		CABLE_TYPE,
#endif
#ifdef EE16_2
		CABLE_TYPE,
#endif
#ifdef EE16_3
		CABLE_TYPE,
#endif
};

#ifdef IP
int	ee16inetstats = 1;
#else
int	ee16inetstats = 0;
#endif
