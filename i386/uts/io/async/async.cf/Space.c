/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/async/async.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h> /* for tunable parameters */
#include <sys/asyncsys.h>

aioreq_t	aio[NAIOSYS];
int	aio_size = NAIOSYS;
int	min_aio_servers = MINAIOS;
int	max_aio_servers = MAXAIOS;
int	aio_server_timeout = AIOTIMEOUT;
short	naioproc = NAIOPROC;
