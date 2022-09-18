/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/sxt/sxt.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/strtty.h>
#include <sys/nsxt.h>

#define LINKSIZE  (sizeof(struct Link) + sizeof(struct Channel) * (MAXPCHAN-1))

#include <config.h>	/* for overriding above parameter */

struct strtty     nsxt_tty[NUMSXT] ;
char    nsxt_buf[NUMSXT * LINKSIZE] ;
int	nsxt_cnt = NUMSXT;
