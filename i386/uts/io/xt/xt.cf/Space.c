/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/xt/xt.cf/Space.c	1.4"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/tty.h>
#include <sys/nxtproto.h>
#ifdef VPIX
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/v86.h>
#endif
#include <sys/nxt.h>

#define LINKSIZE  (sizeof(struct Link) + sizeof(struct Channel) * (MAXPCHAN-1))

#include <config.h>	/* to collect other tunable parameters */

struct tty	nxt_tty[NUMXT];
struct xtctl	nxtctl[NUMXT];
int     nxt_count = NUMXT;
