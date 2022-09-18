/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:net/tcpip/arp.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>

#include <sys/types.h>

#define STRNET

#include <netinet/if_ether.h>

#define ARPTAB_BSIZ	9
#define ARPTAB_NB	19
#define ARPTAB_SIZE	(ARPTAB_BSIZ * ARPTAB_NB)

struct arptab	arptab[ARPTAB_SIZE];
int		arptab_bsiz = ARPTAB_BSIZ;
int		arptab_nb = ARPTAB_NB;
int		arptab_size = ARPTAB_SIZE;
