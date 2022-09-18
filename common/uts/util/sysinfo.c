/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/sysinfo.c	1.1.3.2"
#ident	"$Header: $"

#include <util/sysinfo.h>

struct sysinfo	sysinfo;	/* record system information */

struct syswait	syswait;	/* record processes waiting for disk I/O */

struct syserr	syserr;		/* record system error statistics */

struct minfo	minfo;		/* record physical memory statistics */

struct vminfo	vminfo;		/* record virtual memory statistics */
