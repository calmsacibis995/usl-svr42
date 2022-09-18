/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_BITMAP_H	/* wrapper symbol for kernel use */
#define _UTIL_BITMAP_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/bitmap.h	1.4"
#ident	"$Header: $"

/*
 * Operations on bitmaps of arbitrary size.
 * A bitmap is a vector of 1 or more ulongs.
 * The user of the package is responsible for range
 * checks and keeping track of sizes.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif	defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#define BT_NBIPUL	32	/* number of bits per ulong */

#define BT_ULSHIFT	5	/* log2(BT_NBIPUL), to extract word index */

#define BT_ULMASK	0x1F	/* to extract bit index */

/* 
 * A bitmap is a ulong *, a bitindex is an index_t.
 *
 * The macros BT_WIM and BT_BIW are internal; there is no need
 * for users of this package to use them.
 */

/*
 * Word in map.
 */
#define BT_WIM(bitmap, bitindex) \
	((bitmap)[(bitindex) >> BT_ULSHIFT])

/*
 * Bit in word.
 */
#define BT_BIW(bitindex) \
	(1 << ((bitindex) & BT_ULMASK))


/*
 * These are public macros.
 */

/*
 * BT_BITOUL == n bits to n ulongs.
 */
#define BT_BITOUL(nbits) \
	(((nbits) + BT_NBIPUL -1) / BT_NBIPUL)

/*
 * Return 1 if specified bit is on, 0 if specified bit is off.
 */
#define BT_TEST(bitmap, bitindex) \
	((BT_WIM((bitmap), (bitindex)) & BT_BIW(bitindex)) ? 1 : 0)

/*
 * Turn on the specified bit.
 */
#define BT_SET(bitmap, bitindex) \
	{ BT_WIM((bitmap), (bitindex)) |= BT_BIW(bitindex); }

/*
 * Turn off the specified bit.
 */
#define BT_CLEAR(bitmap, bitindex) \
	{ BT_WIM((bitmap), (bitindex)) &= ~BT_BIW(bitindex); }


#if defined(__STDC__)

/*
 * Return next available bit index from bitmap with specified number of bits.
 */
extern index_t	bt_availbit(ulong *bitmap, size_t nbits);

/*
 * Find the highest order bit that is on, and is within or below
 * the word specified by wx.
 */
extern void	bt_gethighbit(ulong *mapp, int wx, int *bitposp);

/* 
 * Search the bitmap for a consecutive pattern of 1's.
 */
extern int 	bt_range(ulong *bitmap, size_t *pos1, size_t *pos2, size_t nbits);

#else

extern index_t	bt_availbit();
extern void	bt_gethighbit();
extern int	bt_range();

#endif

#endif	/* _UTIL_BITMAP_H */
