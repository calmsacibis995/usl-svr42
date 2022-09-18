/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/spt/spt.cf/Space.c	1.2"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */
/*
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 *
 */
#include <config.h>
#include <sys/types.h>
#include <sys/spt.h>
#include <sys/tty.h>
#include <sys/poll.h>

int 				nspttys = SPT_UNITS;

struct tty 			spt_tty[SPT_UNITS];

char 				mptflags[SPT_UNITS];

struct pollhead			*spt_php[SPT_UNITS];
struct pollhead			*mpt_php[SPT_UNITS];

/**
 ** this is really a ptr to a v86_t structure, but it is easier to define 
 ** as a caddr_t (and this is how it is defined in the proc structure).
 **/
caddr_t				spt_v86p[SPT_UNITS];

/* End Enhanced Application Compatibility Support */
