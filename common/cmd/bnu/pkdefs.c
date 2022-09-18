/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:pkdefs.c	2.2.5.3"
#ident "$Header: pkdefs.c 1.1 91/02/28 $"

#include "uucp.h"

#define USER 1
#include "pk.h"
char next[8]	={ 1,2,3,4,5,6,7,0};	/* packet sequence numbers */
unsigned char mask[8]	={ 1,2,4,010,020,040,0100,0200 };
int	pkactive;
int pkdebug;
int pksizes[] = {
	1, 32, 64, 128, 256, 512, 1024, 2048, 4096, 1
};
struct pack *pklines[NPLINES];
