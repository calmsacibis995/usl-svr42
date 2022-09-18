/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/dlpi_token/ibmtok.cf/Space.c	1.7"
#ident	"$Header: $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/


/*
 *		Copyright (c) 1992 by Dell Computer Corporation
 *		All rights reserved.
 */


#include <sys/types.h>
#include <sys/stream.h>
#include <sys/ibmtokhw.h>
#include <sys/ibmtok.h>
#include <sys/socket.h>
#include <net/if.h>
#include <config.h>

uint tok_debug = 

/*
SYSCALL	+		
PUTPROC	+	
SRVPROC	+
RECV	+ 
RCVERR	+
DLPRIM	+
INFODMP	+		
DLPRIMERR	+	
DLSTATE	+	
TRACE		+		
INTR		+		
BOARD		+
LLC1		+	
SEND		+
BUFFER	+
SCHED		+	
XTRACE	+	
INIT		+  */
ERROR/*		+
SRCROUT */ ; 

struct	tokdev     tokdevs[IBMTOK_UNITS / 2];
struct	tokstat    tokstats[IBMTOK_CNTLS / 2];
struct	tokdevstat tokdevstats[IBMTOK_UNITS / 2];
int	           tok_board_cnt = IBMTOK_CNTLS / 2;
int	           tok_totminors = IBMTOK_UNITS / 2;
struct ifstats     toks_ifstats[IBMTOK_UNITS / 2];
int                tokboardfirst[IBMTOK_UNITS / 2];

struct tokparam tokparams[IBMTOK_CNTLS / 2] = {
	{
#if IBMTOK_0 && IBMTOK_1
	0,			/* board index (tok_index) */
	IBMTOK_0_VECT,		/* interrupt level (tok_int) */
	IBMTOK_0_SIOA,		/* I/O port for device (tok_ioaddr) */
	IBMTOK_0,			/* number of minor devices allowed */
	IBMTOK_CHAN,		/* DMA Channel number (tok_chan) */
	IBMTOK_CMAJOR_0,
	0,
	0,
	IBMTOK_1_SCMA,		/* Shared RAM address*/
	IBMTOK_0_SCMA,		/* The ROM BIOS address */
	ADAPTER_DISABLED
	}
#endif
#if IBMTOK_2 && IBMTOK_3
	,
	{
	1,			/* board index (tok_index) */
	IBMTOK_2_VECT,		/* interrupt level (tok_int) */
	IBMTOK_2_SIOA,		/* I/O port for device (tok_ioaddr) */
	IBMTOK_2,			/* number of minor devices allowed */
	IBMTOK_CHAN,		/* DMA Channel number (tok_chan) */
	IBMTOK_CMAJOR_2,
	0,
	0,
	IBMTOK_3_SCMA,		/* Shared RAM Address	*/
	IBMTOK_1_SCMA,		/* The bios address	*/
	ADAPTER_DISABLED
	},
#endif
};

static	char	tokname[] = "tok";

/* Space for command parameter buffers */
struct OpenParam tok_bopen[IBMTOK_CNTLS/2] = {0};

/* Be very carefull when changing the following variables. */
ushort	tok_open_options         = 0x2000;
unsigned  tok_nbr_rcv_buffers      = 20;
unsigned  tok_rcv_buff_size        = 264;
unsigned  tok_nbr_tx_buffers       = 1;
unsigned  tok_tx_buff_size         = 2048;
unsigned  tok_max_nbr_of_SAPs      = 20;
/* End of open parameters. */
