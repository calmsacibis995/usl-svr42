/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5util:patch/version.c	1.1"
/* $Header: version.c,v 2.0 86/09/17 15:40:11 lwall Exp $
 *
 * $Log:	version.c,v $
 * Revision 2.0  86/09/17  15:40:11  lwall
 * Baseline for netwide release.
 * 
 */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "INTERN.h"
#include "patchlevel.h"
#include "version.h"

/* Print out the version number and die. */

void
version()
{
    extern char rcsid[];

#ifdef lint
    rcsid[0] = rcsid[0];
#else
    fatal3("%s\nPatch level: %d\n", rcsid, PATCHLEVEL);
#endif
}
