/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:net/tcpip/tcp.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>

#define STRNET

#include <netinet/tcp_debug.h>

#define NTCP		512
#define TCPDEBUG	4

unsigned char		tcp_dev[(NTCP+7)/8];
int			ntcp = NTCP;
struct tcp_debug	tcp_debug[TCPDEBUG];
int			tcp_ndebug = TCPDEBUG;
int			tcplinger = TCPLINGER;
