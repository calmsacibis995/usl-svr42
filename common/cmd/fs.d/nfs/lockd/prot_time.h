/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_time.h	1.1.4.3"
#ident	"$Header: $"

/*
 * This file consists of all timeout definition used by rpc.lockd
 */

#define MAX_LM_TIMEOUT_COUNT	1
#define LM_TIMEOUT_DEFAULT      25
#define LM_GRACE_DEFAULT       	1
int 	LM_TIMEOUT;
int 	LM_GRACE;
int	grace_period;
