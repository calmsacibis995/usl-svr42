/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_TCPIP_BYTEORDER_H	/* wrapper symbol for kernel use */
#define _NET_TCPIP_BYTEORDER_H	/* subject to change without notice */

#ident	"@(#)uts-x86:net/tcpip/byteorder.h	1.7"
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

#endif /* _NET_TCPIP_BYTEORDER_H */
