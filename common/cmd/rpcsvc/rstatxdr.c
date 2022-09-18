/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcsvc:rstatxdr.c	1.2"
#ident	"$Header: $"

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 *
 * Previous version was: rstatxdr.c 1.1 86/09/25 Copyr 1984 Sun Micro";
 */

#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

/*** ONLY FOR TESTING
#define CPUSTATES       4
#include "rstat_a.h"
***/

rstat(host, statp)
        char *host;
        struct statstime *statp;
{
        return (rpc_call(host, RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
            xdr_void, NULL, xdr_statstime, statp, NULL));
}

havedisk(host)
        char *host;
{
        long have;
        
        if (rpc_call(host, RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_HAVEDISK,
            xdr_void, NULL, xdr_long, &have, NULL) != 0)
                return (-1);
        else
                return (have);
}

xdr_stats(xdrs, statp)
        XDR *xdrs;
        struct stats *statp;
{
        int i;
        
        for (i = 0; i < CPUSTATES; i++)
                if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
                        return (0);
        for (i = 0; i < DK_NDRIVE; i++)
                if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
                        return (0);
        if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pswpin) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pswpout) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_intr) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_ipackets) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_ierrors) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_oerrors) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_collisions) == 0)
                return (0);
        return (1);
}

xdr_statsswtch(xdrs, statp)             /* version 2 */
        XDR *xdrs;
        struct statsswtch *statp;
{
        int i;
        
        for (i = 0; i < CPUSTATES; i++)
                if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
                        return (0);
        for (i = 0; i < DK_NDRIVE; i++)
                if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
                        return (0);
        if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pswpin) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pswpout) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_intr) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_ipackets) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_ierrors) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_oerrors) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_collisions) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_swtch) == 0)
                return (0);
        for (i = 0; i < 3; i++)
                if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
                        return (0);
        if (xdr_timeval(xdrs, &statp->boottime) == 0)
                return (0);
        return (1);
}

xdr_statstime(xdrs, statp)              /* version 3 */
        XDR *xdrs;
        struct statstime *statp;
{
        int i;
        
        for (i = 0; i < CPUSTATES; i++)
                if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
                        return (0);
        for (i = 0; i < DK_NDRIVE; i++)
                if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
                        return (0);
        if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pswpin) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_pswpout) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_intr) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_ipackets) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_ierrors) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_oerrors) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->if_collisions) == 0)
                return (0);
        if (xdr_int(xdrs, &statp->v_swtch) == 0)
                return (0);
        for (i = 0; i < 3; i++)
                if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
                        return (0);
        if (xdr_timeval(xdrs, &statp->boottime) == 0)
                return (0);
        if (xdr_timeval(xdrs, &statp->curtime) == 0)
                return (0);
        return (1);
}

xdr_timeval(xdrs, tvp)
        XDR *xdrs;
        struct timeval *tvp;
{
        if (xdr_long(xdrs, &tvp->tv_sec) == 0)
                return (0);
        if (xdr_long(xdrs, &tvp->tv_usec) == 0)
                return (0);
        return (1);
}
