/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpadmin/usage.c	1.10.2.2"
#ident  "$Header: usage.c 1.2 91/06/27 $"

#include "lp.h"
#include "printers.h"
#define WHO_AM_I           I_AM_LPADMIN
#include "oam.h"

/**
 ** usage() - PRINT COMMAND USAGE
 **/

void			usage ()
{
#ifdef NETWORKING
   LP_OUTMSG(INFO, E_ADM_USAGE);
#else
   LP_OUTMSG(INFO, E_ADM_USAGENONET);
#endif
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE1);
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE2);
#if	defined(CAN_DO_MODULES)
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE3);
#endif
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE4);
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE5);
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE6);

   return;
}
