/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/tcpip/in_cksum.c	1.3.3.1"
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
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */


/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

#define STRNET

#include <util/types.h>
#include <io/stream.h>
#include <net/tcpip/in.h>
#ifdef SYSV
#include <util/cmn_err.h>
#endif SYSV

#ifndef BYTE_ORDER
#include <net/tcpip/byteorder.h>
#endif /* !BYTE_ORDER */

in_cksum(mp, len)
	mblk_t         *mp;
	int             len;
{
	register unsigned char *w;
	register u_long	sum;	/* Assume long > 16 bits */
	register int    mlen;
	register u_short	fswap;
	register int    nwords;

	sum = 0;
	fswap = 0;
	while (mp && len) {
		w = mp->b_rptr;
		mlen = len < mp->b_wptr - w ? len : mp->b_wptr - w;
		if ((int) w & 1 && mlen) {
			fswap = ~fswap;
#if BYTE_ORDER == LITTLE_ENDIAN
			sum += *w;
			sum = (sum & 0xffff) + (sum >> 16);
			sum = ((sum & 0xff) << 8) | sum >> 8;
#endif
#if BYTE_ORDER == BIG_ENDIAN
			sum = ((sum & 0xff) << 8) | sum >> 8;
			sum += *w;
			sum = (sum & 0xffff) + (sum >> 16);
#endif
			mlen--;
			len--;
			w++;
		}
		len -= mlen;
		nwords = mlen >> 1;
		while (nwords--) {
			sum += *(ushort *) w;
			w += 2;
		}
		while (sum & ~0xffff) {
			sum = (sum & 0xffff) + (sum >> 16);
		}
		if (mlen & 1) {
			fswap = ~fswap;
#if BYTE_ORDER == LITTLE_ENDIAN
			sum += *w;
			sum = (sum & 0xffff) + (sum >> 16);
			sum = ((sum & 0xff) << 8) | sum >> 8;
#endif
#if BYTE_ORDER == BIG_ENDIAN
			sum = ((sum & 0xff) << 8) | sum >> 8;
			sum += *w;
			sum = (sum & 0xffff) + (sum >> 16);
#endif
		}
		mp = mp->b_cont;
	}
	if (len) {
#ifdef SYSV
		cmn_err(CE_PANIC, "message block not long enough for cksum");
#else
		printf ("in_cksum: message block not long enough for cksum");
		panic ("in_cksum");
#endif SYSV
	}
	if (fswap)
		sum = ((sum & 0xff) << 8) | sum >> 8;
	return ((ushort) ~ sum);
}
