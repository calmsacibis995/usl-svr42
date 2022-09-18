


/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/




#ifndef _NET_YHTPIP_BYTEORDER_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_BYTEORDER_H	/* subject to change without notice */
/*
 * macros for conversion between host and (internet) network byte order
 */

#ifndef BYTE_ORDER
/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax) */
#define	BIG_ENDIAN	4321	/* most-significant byte first (IBM, net) */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long (pdp) */

#define	BYTE_ORDER	LITTLE_ENDIAN

/*
 * If we were M68K, SPARC, etc., byteorder would be BIG_ENDIAN
 * #define	BYTE_ORDER	BIG_ENDIAN
 */
#endif /* !BYTE_ORDER */


/*
 * macros for conversion between host and (internet) network byte order
 */

#if BYTE_ORDER == BIG_ENDIAN
/* big-endian */
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)

#elif defined(i386) && defined(__USLC__)

#define	ntohl(x)	xxntohl(x)
#define	ntohs(x)	xxntohs(x)
#define	htonl(x)	xxhtonl(x)
#define	htons(x)	xxhtons(x)

/*
 *	unsigned long htonl( hl )
 *	long hl;
 *	reverses the byte order of 'long hl'
 */

asm unsigned long xxhtonl( hl )
{
%mem	hl;	
	movl	hl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned long ntohl( nl )
 *	unsigned long nl;
 *	reverses the byte order of 'ulong nl'
 */

asm unsigned long xxntohl( nl )
{
%mem	nl;
	movl	nl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned short htons( hs )
 *	short hs;
 *
 *	reverses the byte order in hs.
 */

asm unsigned short xxhtons( hs )
{
%mem	hs;
	movl	hs, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned short ntohs( ns )
 *	unsigned short ns;
 *
 *	reverses the bytes in ns.
 */


asm unsigned short xxntohs( ns )
{
%mem	ns;
	movl	ns, %eax
	xchgb	%ah, %al
	clc
}

#else /*	!defined(ntohl)   little-endian, not i386 */

u_short	ntohs(), htons();
u_long	ntohl(), htonl();

#endif

/*
**	The following macro will swap bytes in a short.
**	Warning: this is very machine-specific in that it
**	expects 16-bit shorts and 8-bit chars
*/

#define bswaps(us)	(((unsigned short)((us) & 0xff) << 8) | \
			((unsigned short)((us) & ~0xff) >> 8))

#endif /* _NET_YHTPIP_BYTEORDER_H */
