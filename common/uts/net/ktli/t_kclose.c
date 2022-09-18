/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/ktli/t_kclose.c	1.3.2.1"
#ident	"$Header: $"
/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 *	Much like closef().
 *
 *	Returns:
 *		0
 */


#include <util/param.h>
#include <util/types.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <fs/file.h>
#include <net/transport/tihdr.h>
#include <net/transport/timod.h>
#include <net/transport/tiuser.h>
#include <net/ktli/t_kuser.h>

int
t_kclose(tiptr, callclosef)
	register TIUSER		*tiptr;
	register int		callclosef;
{
	register struct file	*fp;

	fp = tiptr->fp;

	kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
	if (callclosef)
		closef(fp);
	return 0;
}
