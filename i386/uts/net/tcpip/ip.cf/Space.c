/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:net/tcpip/ip.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/types.h>

#define STRNET

#include <netinet/ip_str.h>

#define IPCNT	8
#define IPPROVCNT	16
#define IPSENDREDIRECTS	0

/* if IPFORWARDING is set, hosts will act as gateways */
#define IPFORWARDING	0

struct	ip_pcb ip_pcb[IPCNT];
struct 	ip_provider provider[IPPROVCNT];
int     ipcnt=IPCNT;
int	ipprovcnt=IPPROVCNT;
int	ipforwarding=IPFORWARDING;
int	ipsendredirects=IPSENDREDIRECTS;

struct ifstats *ifstats;
