
/*
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 */

#define STRNET

#include <util/types.h>
#include <io/stream.h>
#include <net/yhtpip/yhin.h>
#ifdef SYSV
#include <util/cmn_err.h>
#endif SYSV

#ifndef BYTE_ORDER
#include <net/yhtpip/byteorder.h>
#endif /* !BYTE_ORDER */

yhin_cksum(mp, len)
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
		printf ("yhin_cksum: message block not long enough for cksum");
		panic ("yhin_cksum");
#endif SYSV
	}
	if (fswap)
		sum = ((sum & 0xff) << 8) | sum >> 8;
	return ((ushort) ~ sum);
}
