/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)truss:i386/cmd/truss/incdec.c	1.1.1.2"
#ident  "$Header: incdec.c 1.3 91/07/09 $"

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"

/*
 * Atomic operations for shared memory:
 * Increment or decrement a word in memory with one machine instruction.
 *
 * Although the window is small, encountering a forced process switch
 * in the middle of a read-alter-rewrite sequence could mess up the
 * multi-process coordination this supports.
 */

/*ARGSUSED*/
void
increment(p)
int * p;
{
	(*p)++;
}

/*ARGSUSED*/
void
decrement(p)
int * p;
{
	--(*p);
}
