/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/rfs/rfs.cf/Space.c	1.2"
#ident	"$Header: $"

#include <config.h>

int rf_state;

int nsrmount = NSRMOUNT;
int nrcvd = NRCVD;
int nsndd = NSNDD;
int maxgdp = MAXGDP;
int minserve = MINSERVE;
int maxserve = MAXSERVE;
int nrduser = NRDUSER;
int rc_time = RCACHETIME;
int rftmo_time = RFTMO_TIME;
int rf_maxkmem = RF_MAXKMEM;
