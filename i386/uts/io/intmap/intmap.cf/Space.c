/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/intmap/intmap.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/emap.h>	/* channel mapping include file from XENIX */
#include <sys/nmap.h>	/* channel mapping include file from XENIX */

#include "config.h" 	/* to get NEMAP tunaebale from mtune */

struct	emap	emap[NEMAP];	/* channel mapping data struct table */
struct	nxmap	nxmap[NEMAP];	/* channel mapping data struct table */


