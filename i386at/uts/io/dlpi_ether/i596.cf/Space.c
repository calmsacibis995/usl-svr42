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

#ident	"@(#)uts-x86at:io/dlpi_ether/i596.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EL16  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 * The next three configurable parameters (CABLE_TYPE, SAAD, ZWS) will only be
 * used when there's 1 board configured on the system. Since there's only one
 * port (port 0x100) to configure the board, multiple boards can't be configured
 * through the same port. To configure multiple boards, read the EthereLink 16
 * Installation and Configuration Guide and make sure the same parameters are
 * set in the Unix systems configuration file.
 */

/*
 *  The CABLE_TYPE define determines the ethernet cable type.  A value of 1
 *  indicates the ethernet controller is to be attached to thick ethernet
 *  cable (AUI).  A value of 0 indicates a thin ethernet cable (BNC).
 */
#define	CABLE_TYPE	1

/*
 * The ZWS (Zero Wait State) define determines the wait state of the RAM.
 * A value of 0 disables 0WS. A value of 1 enable 0WS.
 */
#define ZWS 0

/*
 * The SAAD define determines the SA address decode. A value of 0 allows
 * SA address decode (normal operation). A value of 1 disables SA address
 * decode.
 */
#define SAAD 0

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
#define	IFNAME	"emd"

/********************** STOP!  DON'T TOUCH THAT DIAL ************************
 *
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere motals.
 */
int	i596cable_type = CABLE_TYPE;
int	i596zws = ZWS;
int	i596saad = SAAD;
int		i596boards = I596_CNTLS;
int		i596strlog = STREAMS_LOG;
char		*i596_ifname = IFNAME;
DL_sap_t	i596saps[ N_SAPS * I596_CNTLS ];
DL_bdconfig_t	i596config[ I596_CNTLS ] = {
#ifdef I596_0
	{
		I596_CMAJOR_0,		/* Major number			*/
		I596_0_SIOA,		/* Start of base I/O address	*/
		I596_0_EIOA,		/* End of base I/O address	*/
		I596_0_SCMA,		/* Start of base memory address	*/
		I596_0_ECMA,		/* End of base memory address	*/
		I596_0_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef I596_1
	{
		I596_CMAJOR_1,		/* Major number			*/
		I596_1_SIOA,		/* Start of base I/O address	*/
		I596_1_EIOA,		/* End of base I/O address	*/
		I596_1_SCMA,		/* Start of base memory address	*/
		I596_1_ECMA,		/* End of base memory address	*/
		I596_1_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef I596_2
	{
		I596_CMAJOR_2,		/* Major number			*/
		I596_2_SIOA,		/* Start of base I/O address	*/
		I596_2_EIOA,		/* End of base I/O address	*/
		I596_2_SCMA,		/* Start of base memory address	*/
		I596_2_ECMA,		/* End of base memory address	*/
		I596_2_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
#ifdef I596_3
	{
		I596_CMAJOR_3,		/* Major number			*/
		I596_3_SIOA,		/* Start of base I/O address	*/
		I596_3_EIOA,		/* End of base I/O address	*/
		I596_3_SCMA,		/* Start of base memory address	*/
		I596_3_ECMA,		/* End of base memory address	*/
		I596_3_VECT,		/* Interrupt vector number	*/
		N_SAPS,
		0,
	},
#endif
};

/*
 *  If there are multiple I596 boards in the same system, make sure there is
 *  a comma separated value for each one.
 */
int	i596_cable_type[ I596_CNTLS ] = {
#ifdef I596_0
		CABLE_TYPE,
#endif
#ifdef I596_1
		CABLE_TYPE,
#endif
#ifdef I596_2
		CABLE_TYPE,
#endif
#ifdef I596_3
		CABLE_TYPE,
#endif
};

#ifdef IP
int	i596inetstats = 1;
#else
int	i596inetstats = 0;
#endif

#define	N_CMD		 15
#define	N_TBD		 15
#define	N_FD		 50
#define	N_RBD		 50
#define	RCVBUFSIZ	512

unsigned short i596_n_cmd		= N_CMD;
unsigned short i596_n_tbd		= N_TBD;
unsigned short i596_n_fd		= N_FD;
unsigned short i596_n_rbd		= N_RBD;
unsigned short i596_rcvbufsiz		= RCVBUFSIZ;

#ifndef MB2
unsigned short i596_station_addr	= 0;
unsigned short i596_reset_addr		= I596_0_SIOA;
unsigned short i596_port1_addr		= I596_0_SIOA;
unsigned short i596_port2_addr		= I596_0_SIOA + 0x02;
unsigned short i596_chan_attn_addr	= I596_0_SIOA + 0x04;
unsigned short i596_clr_int_addr	= I596_0_SIOA + 0x08;
unsigned short i596_enet_in_flash	= 1;
#else
unsigned short i596_station_addr	= I596_0_SIOA + 0x10;
unsigned short i596_reset_addr		= I596_0_SIOA + 0x0F;
unsigned short i596_port1_addr		= I596_0_SIOA + 0x04;
unsigned short i596_port2_addr		= I596_0_SIOA + 0x06;
unsigned short i596_chan_attn_addr	= I596_0_SIOA + 0x02;
unsigned short i596_clr_int_addr	= 0;
unsigned short i596_enet_in_flash	= 0;
#endif

unsigned short i596_cmd_chain_enable	= 1;

