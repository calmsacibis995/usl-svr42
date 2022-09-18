/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/sad/sad.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/stream.h>
#include <sys/sad.h>

struct	saddev saddev[NUMSAD];
struct	autopush autopush[NAUTOPUSH];
struct autopush *strpcache[NSTRPHASH];	/* autopush hash list */

int	sadcnt = NUMSAD;
int	nstrphash = NSTRPHASH;
int	nautopush = NAUTOPUSH;
int	strpmask = NSTRPHASH-1;
