/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)siserver:ddx/mfb/maskbits.h	1.1"

/* Combined Purdue/PurduePlus patches, level 2.1, 1/24/89 */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
/* $XConsortium: maskbits.h,v 1.21 89/11/13 09:36:56 rws Exp $ */
#include "X.h"
#include "Xmd.h"
#include "servermd.h"

#ifdef ORIGINAL
extern int starttab[];
extern int endtab[];
extern int rmask[];
extern int mask[];
#else
extern unsigned int sistarttab1[];
extern unsigned int siendtab1[];
#endif
#ifdef ORIGINAL_PARTMASKS
extern unsigned partmasks[32][32];
#endif


/* the following notes use the following conventions:
SCREEN LEFT				SCREEN RIGHT
in this file and maskbits.c, left and right refer to screen coordinates,
NOT bit numbering in registers.

starttab[n]
	bits[0,n-1] = 0	bits[n,31] = 1
endtab[n] =
	bits[0,n-1] = 1	bits[n,31] = 0

startpartial[], endpartial[]
	these are used as accelerators for doing putbits and masking out
bits that are all contained between longword boudaries.  the extra
256 bytes of data seems a small price to pay -- code is smaller,
and narrow things (e.g. window borders) go faster.

the names may seem misleading; they are derived not from which end
of the word the bits are turned on, but at which end of a scanline
the table tends to be used.

look at the tables and macros to understand boundary conditions.
(careful readers will note that starttab[n] = ~endtab[n] for n != 0)

-----------------------------------------------------------------------
these two macros depend on the screen's bit ordering.
in both of them x is a screen position.  they are used to
combine bits collected from multiple longwords into a
single destination longword, and to unpack a single
source longword into multiple destinations.

SCRLEFT(dst, x)
	takes dst[x, 32] and moves them to dst[0, 32-x]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	this is a right shift on LSBFirst (forward-thinking)
	machines like the VAX, and left shift on MSBFirst
	(backwards) machines like the 680x0 and pc/rt.

SCRRIGHT(dst, x)
	takes dst[0,x] and moves them to dst[32-x, 32]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	this is a left shift on LSBFirst, right shift
	on MSBFirst.


the remaining macros are cpu-independent; all bit order dependencies
are built into the tables and the two macros above.

maskbits(x, w, startmask, endmask, nlw)
	for a span of width w starting at position x, returns
a mask for ragged bits at start, mask for ragged bits at end,
and the number of whole longwords between the ends.

maskpartialbits(x, w, mask)
	works like maskbits(), except all the bits are in the
	same longword (i.e. (x&0x1f + w) <= 32)

mask32bits(x, w, startmask, endmask, nlw)
	as maskbits, but does not calculate nlw.  it is used by
	mfbGlyphBlt to put down glyphs <= 32 bits wide.

-------------------------------------------------------------------

NOTE
	any pointers passe to the following 4 macros are
	guranteed to be 32-bit aligned.
	The only non-32-bit-aligned references ever made are
	to font glyphs, and those are made with getleftbits()
	and getshiftedleftbits (qq.v.)

getbits(psrc, x, w, dst)
	starting at position x in psrc (x < 32), collect w
	bits and put them in the screen left portion of dst.
	psrc is a longword pointer.  this may span longword boundaries.
	it special-cases fetching all w bits from one longword.

	+--------+--------+		+--------+
	|    | m |n|      |	==> 	| m |n|  |
	+--------+--------+		+--------+
	    x      x+w			0     w
	psrc     psrc+1			dst
			m = 32 - x
			n = w - m

	implementation:
	get m bits, move to screen-left of dst, zeroing rest of dst;
	get n bits from next word, move screen-right by m, zeroing
		 lower m bits of word.
	OR the two things together.

putbits(src, x, w, pdst)
	starting at position x in pdst, put down the screen-leftmost
	w bits of src.  pdst is a longword pointer.  this may
	span longword boundaries.
	it special-cases putting all w bits into the same longword.

	+--------+			+--------+--------+
	| m |n|  |		==>	|    | m |n|      |
	+--------+			+--------+--------+
	0     w				     x     x+w
	dst				pdst     pdst+1
			m = 32 - x
			n = w - m

	implementation:
	get m bits, shift screen-right by x, zero screen-leftmost x
		bits; zero rightmost m bits of *pdst and OR in stuff
		from before the semicolon.
	shift src screen-left by m, zero bits n-32;
		zero leftmost n bits of *(pdst+1) and OR in the
		stuff from before the semicolon.

putbitsrop(src, x, w, pdst, ROP)
	like putbits but calls DoRop with the rasterop ROP (see mfb.h for
	DoRop)

putbitsrrop(src, x, w, pdst, ROP)
	like putbits but calls DoRRop with the reduced rasterop ROP
	(see mfb.h for DoRRop)

-----------------------------------------------------------------------
	The two macros below are used only for getting bits from glyphs
in fonts, and glyphs in fonts are gotten only with the following two
mcros.
	You should tune these macros toyour font format and cpu
byte ordering.

NOTE
getleftbits(psrc, w, dst)
	get the leftmost w (w<=32) bits from *psrc and put them
	in dst.  this is used by the mfbGlyphBlt code for glyphs
	<=32 bits wide.
	psrc is declared (unsigned char *)

	psrc is NOT guaranteed to be 32-bit aligned.  on  many
	machines this will cause problems, so there are several
	versions of this macro.

	this macro is called ONLY for getting bits from font glyphs,
	and depends on the server-natural font padding.

	for blazing text performance, you want this macro
	to touch memory as infrequently as possible (e.g.
	fetch longwords) and as efficiently as possible
	(e.g. don't fetch misaligned longwords)

getshiftedleftbits(psrc, offset, w, dst)
	used by the font code; like getleftbits, but shifts the
	bits SCRLEFT by offset.
	this is implemented portably, calling getleftbits()
	and SCRLEFT().
	psrc is declared (unsigned char *).
*/

#if (BITMAP_BIT_ORDER == IMAGE_BYTE_ORDER)
#define LONG2CHARS(x) (x)
#else
/*
 *  the unsigned case below is for compilers like
 *  the Danbury C and i386cc
 */
#define LONG2CHARS( x ) ( ( ( ( x ) & 0x000000FF ) << 0x18 ) \
                      | ( ( ( x ) & 0x0000FF00 ) << 0x08 ) \
                      | ( ( ( x ) & 0x00FF0000 ) >> 0x08 ) \
                      | ( ( ( x ) & (unsigned long)0xFF000000 ) >> 0x18 ) )
#endif

#ifdef STRICT_ANSI_SHIFT
#define SHL(x,y)    ((y) >= 32 ? 0 : LONG2CHARS(LONG2CHARS(x) << (y)))
#define SHR(x,y)    ((y) >= 32 ? 0 : LONG2CHARS(LONG2CHARS(x) >> (y)))
#else
#define SHL(x,y)    LONG2CHARS(LONG2CHARS(x) << (y))
#define SHR(x,y)    LONG2CHARS(LONG2CHARS(x) >> (y))
#endif

#if (BITMAP_BIT_ORDER == MSBFirst)	/* pc/rt, 680x0 */
#define SCRLEFT(lw, n)	SHL((unsigned int)(lw),(n))
#define SCRRIGHT(lw, n)	SHR((unsigned int)(lw),(n))
#else					/* vax, intel */
#define SCRLEFT(lw, n)	SHR((lw),(n))
#define SCRRIGHT(lw, n)	SHL((lw),(n))
#endif

#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = sistarttab1[(x)&0x1f]; \
    endmask = siendtab1[((x)+(w)) & 0x1f]; \
    if (startmask) \
	nlw = (((w) - (32 - ((x)&0x1f))) >> 5); \
    else \
	nlw = (w) >> 5;

#ifdef ORIGINAL_PARTMASKS
/*
 * This is the original macro from MIT, but the partmasks[32][32] table
 * takes up 4K space; by generating the mask at run time we don't need
 * define the table, thus saving 4K - (code for generating the mask)
 * Adds about 1K code to the text section but decreases the data section
 * by 4K. .....
 * To use the ORIGINAL partmasks table, just define ORIGINAL_PARTMASKS
 * at the begining of this file and add maskbits.c to the Makefile and
 * recompile libmfb.a ......  tmk 11/19/90
 */
#define maskpartialbits(x, w, mask) \
    mask = partmasks[(x)&0x1f][(w)&0x1f];
#else
#if (BITMAP_BIT_ORDER == MSBFirst)	/* pc/rt, 680x0 */
#define maskpartialbits(x, w, mask) \
  if ( !((x)&0x1f) && !((w)&0x1f) ) \
      mask = 0xFFFFFFFF; \
  else  if ( ((x)&0x1f) + ((w)&0x1f) > 32) \
	    mask = 0; \
	else mask = siendtab1[(w)&0x1f] >> ((x)&0x1f);
#else  /* LSB */
#define maskpartialbits(x, w, mask) \
  if ( !((x)&0x1f) && !((w)&0x1f) ) \
      mask = 0xFFFFFFFF; \
  else  if ( ((x)&0x1f) + ((w)&0x1f) > 32) \
	    mask = 0; \
         else mask = siendtab1[(w)&0x1f] << ((x)&0x1f);
#endif /* MSB / LSB */
#endif

#define mask32bits(x, w, startmask, endmask) \
    startmask = sistarttab1[(x)&0x1f]; \
    endmask = siendtab1[((x)+(w)) & 0x1f];

#ifdef __GNUC__
#ifdef vax
#define FASTGETBITS(psrc,x,w,dst) \
    asm ("extzv %1,%2,%3,%0" \
	 : "=g" (dst) \
	 : "g" (x), "g" (w), "m" (*(char *)(psrc)))
#define getbits(psrc,x,w,dst) FASTGETBITS(psrc,x,w,dst)

#define FASTPUTBITS(src, x, w, pdst) \
    asm ("insv %3,%1,%2,%0" \
	 : "=m" (*(char *)(pdst)) \
	 : "g" (x), "g" (w), "g" (src))
#define putbits(src, x, w, pdst) FASTPUTBITS(src, x, w, pdst)
#endif /* vax */
#ifdef mc68020
#define FASTGETBITS(psrc, x, w, dst) \
    asm ("bfextu %3{%1:%2},%0" \
    : "=d" (dst) : "di" (x), "di" (w), "o" (*(char *)(psrc)))

#define getbits(psrc,x,w,dst) \
{ \
    FASTGETBITS(psrc, x, w, dst);\
    dst = SHL(dst,(32-(w))); \
}

#define FASTPUTBITS(src, x, w, pdst) \
    asm ("bfins %3,%0{%1:%2}" \
	 : "=o" (*(char *)(pdst)) \
	 : "di" (x), "di" (w), "d" (src), "0" (*(char *) (pdst)))

#define putbits(src, x, w, pdst) FASTPUTBITS(SHR((src),32-(w)), x, w, pdst)

#endif /* mc68020 */
#endif /* __GNUC__ */

/*  The following flag is used to override a bugfix for sun 3/60+CG4 machines,
 */

/*  We don't need to be careful about this unless we're dealing with sun3's 
 *  We will default its usage for those who do not know anything, but will
 *  override its effect if the machine doesn't look like a sun3 
 */
#if !defined(mc68020) || !defined(sun)
#define NO_3_60_CG4
#endif

/* This is gross.  We want to #define u_putbits as something which can be used
 * in the case of the 3/60+CG4, but if we use /bin/cc or are on another
 * machine type, we want nothing to do with u_putbits.  What a hastle.  Here
 * I used slo_putbits as something which either u_putbits or putbits could be
 * defined as.
 *
 * putbits gets it iff it is not already defined with FASTPUTBITS above.
 * u_putbits gets it if we have FASTPUTBITS (putbits) from above and have not
 * 	overridden the NO_3_60_CG4 flag.
 */

#define slo_putbits(src, x, w, pdst) \
{ \
    register int n = (x)+(w)-32; \
    \
    if (n <= 0) \
    { \
	register int tmpmask; \
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | \
		(SCRRIGHT((unsigned) src, x) & tmpmask); \
    } \
    else \
    { \
	*(pdst) = (*(pdst) & siendtab1[x]) | (SCRRIGHT((unsigned) (src), x)); \
	(pdst)[1] = ((pdst)[1] & sistarttab1[n]) | \
		(SCRLEFT((unsigned) src, 32-(x)) & siendtab1[n]); \
    } \
}

#if defined(putbits) && !defined(NO_3_60_CG4)
#define u_putbits(src, x, w, pdst) slo_putbits(src, x, w, pdst)
#else
#define u_putbits(src, x, w, pdst) putbits(src, x, w, pdst)
#endif

#if !defined(putbits) 
#define putbits(src, x, w, pdst) slo_putbits(src, x, w, pdst)
#endif

/* Now if we have not gotten any really good bitfield macros, try some
 * moderately fast macros.  Alas, I don't know how to do asm instructions
 * without gcc.
 */

#ifndef getbits
#define getbits(psrc, x, w, dst) \
{ \
    dst = SCRLEFT((unsigned) *(psrc), (x)); \
    if ( ((x) + (w)) > 32) \
	dst |= (SCRRIGHT((unsigned) *((psrc)+1), 32-(x))); \
}
#endif

/*  We have to special-case putbitsrop because of 3/60+CG4 combos
 */

#define u_putbitsrop(src, x, w, pdst, rop) \
{\
	register int t1, t2; \
	register int n = (x)+(w)-32; \
	\
	t1 = SCRRIGHT((src), (x)); \
	DoRop(t2, rop, t1, *(pdst)); \
	\
    if (n <= 0) \
    { \
	register int tmpmask; \
	\
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
    } \
    else \
    { \
	int m = 32-(x); \
	*(pdst) = (*(pdst) & siendtab1[x]) | (t2 & sistarttab1[x]); \
	t1 = SCRLEFT((src), m); \
	DoRop(t2, rop, t1, (pdst)[1]); \
	(pdst)[1] = ((pdst)[1] & sistarttab1[n]) | (t2 & siendtab1[n]); \
    } \
}

/* If our getbits and putbits are FAST enough,
 * do this brute force, it's faster
 */

#if defined(FASTPUTBITS) && defined(FASTGETBITS) && defined(NO_3_60_CG4)
#if (BITMAP_BIT_ORDER == MSBFirst)
#define putbitsrop(src, x, w, pdst, rop) \
{ \
  register int _tmp, _tmp2; \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp2 = SCRRIGHT(src, 32-(w)); \
  DoRop(_tmp, rop, _tmp2, _tmp) \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#define putbitsrrop(src, x, w, pdst, rop) \
{ \
  register int _tmp, _tmp2; \
 \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp2 = SCRRIGHT(src, 32-(w)); \
  _tmp= DoRRop(rop, _tmp2, _tmp); \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#undef u_putbitsrop
#else
#define putbitsrop(src, x, w, pdst, rop) \
{ \
  register int _tmp; \
  FASTGETBITS(pdst, x, w, _tmp); \
  DoRop(_tmp, rop, src, _tmp) \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#define putbitsrrop(src, x, w, pdst, rop) \
{ \
  register int _tmp; \
 \
  FASTGETBITS(pdst, x, w, _tmp); \
  _tmp= DoRRop(rop, src, _tmp); \
  FASTPUTBITS(_tmp, x, w, pdst); \
}
#undef u_putbitsrop
#endif
#endif

#ifndef putbitsrop
#define putbitsrop(src, x, w, pdst, rop)  u_putbitsrop(src, x, w, pdst, rop)
#endif 

#ifndef putbitsrrop
#define putbitsrrop(src, x, w, pdst, rop) \
{\
	register int t1, t2; \
	register int n = (x)+(w)-32; \
	\
	t1 = SCRRIGHT((src), (x)); \
	t2 = DoRRop(rop, t1, *(pdst)); \
	\
    if (n <= 0) \
    { \
	register int tmpmask; \
	\
	maskpartialbits((x), (w), tmpmask); \
	*(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
    } \
    else \
    { \
	int m = 32-(x); \
	*(pdst) = (*(pdst) & siendtab1[x]) | (t2 & sistarttab1[x]); \
	t1 = SCRLEFT((src), m); \
	t2 = DoRRop(rop, t1, (pdst)[1]); \
	(pdst)[1] = ((pdst)[1] & sistarttab1[n]) | (t2 & siendtab1[n]); \
    } \
}
#endif

#if GETLEFTBITS_ALIGNMENT == 1
#define getleftbits(psrc, w, dst)	dst = *((unsigned int *) psrc)
#endif /* GETLEFTBITS_ALIGNMENT == 1 */

#if GETLEFTBITS_ALIGNMENT == 2
#define getleftbits(psrc, w, dst) \
    { \
	if ( ((int)(psrc)) & 0x01 ) \
		getbits( ((unsigned int *)(((char *)(psrc))-1)), 8, (w), (dst) ); \
	else \
		getbits(psrc, 0, w, dst);
    }
#endif /* GETLEFTBITS_ALIGNMENT == 2 */

#if GETLEFTBITS_ALIGNMENT == 4
#define getleftbits(psrc, w, dst) \
    { \
	int off, off_b; \
	off_b = (off = ( ((int)(psrc)) & 0x03)) << 3; \
	getbits( \
		(unsigned int *)( ((char *)(psrc)) - off), \
		(off_b), (w), (dst) \
	       ); \
    }
#endif /* GETLEFTBITS_ALIGNMENT == 4 */


#define getshiftedleftbits(psrc, offset, w, dst) \
	getleftbits((psrc), (w), (dst)); \
	dst = SCRLEFT((dst), (offset));

/* FASTGETBITS and FASTPUTBITS are not necessarily correct implementations of
 * getbits and putbits, but they work if used together.
 *
 * On a MSBFirst machine, a cpu bitfield extract instruction (like bfextu)
 * could normally assign its result to a long word register in the screen
 * right position.  This saves canceling register shifts by not fighting the
 * natural cpu byte order.
 *
 * Unfortunately, these fail on a 3/60+CG4 and cannot be used unmodified. Sigh.
 */
#if defined(FASTGETBITS) && defined(FASTPUTBITS)
#ifdef NO_3_60_CG4
#define u_FASTPUT(aa, bb, cc, dd)  FASTPUTBITS(aa, bb, cc, dd)
#else
#define u_FASTPUT(aa, bb, cc, dd)  u_putbits(SCRLEFT(aa, 32-(cc)), bb, cc, dd)
#endif

#define getandputbits(psrc, srcbit, dstbit, width, pdst) \
{ \
    register unsigned int _tmpbits; \
    FASTGETBITS(psrc, srcbit, width, _tmpbits); \
    u_FASTPUT(_tmpbits, dstbit, width, pdst); \
}

#define getandputrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
  register unsigned int _tmpsrc, _tmpdst; \
  FASTGETBITS(pdst, dstbit, width, _tmpdst); \
  FASTGETBITS(psrc, srcbit, width, _tmpsrc); \
  DoRop(_tmpdst, rop, _tmpsrc, _tmpdst); \
  u_FASTPUT(_tmpdst, dstbit, width, pdst); \
}

#define getandputrrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
  register unsigned int _tmpsrc, _tmpdst; \
  FASTGETBITS(pdst, dstbit, width, _tmpdst); \
  FASTGETBITS(psrc, srcbit, width, _tmpsrc); \
  _tmpdst = DoRRop(rop, _tmpsrc, _tmpdst); \
  u_FASTPUT(_tmpdst, dstbit, width, pdst); \
}

#define getandputbits0(psrc, srcbit, width, pdst) \
	getandputbits(psrc, srcbit, 0, width, pdst)

#define getandputrop0(psrc, srcbit, width, pdst, rop) \
    	getandputrop(psrc, srcbit, 0, width, pdst, rop)

#define getandputrrop0(psrc, srcbit, width, pdst, rop) \
    	getandputrrop(psrc, srcbit, 0, width, pdst, rop)


#else /* Slow poke */

/* pairs of getbits/putbits happen frequently. Some of the code can
 * be shared or avoided in a few specific instances.  It gets us a
 * small advantage, so we do it.  The getandput...0 macros are the only ones
 * which speed things here.  The others are here for compatibility w/the above
 * FAST ones
 */

#define getandputbits(psrc, srcbit, dstbit, width, pdst) \
{ \
    register unsigned int _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits); \
    putbits(_tmpbits, dstbit, width, pdst); \
}

#define getandputrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
    register unsigned int _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits) \
    putbitsrop(_tmpbits, dstbit, width, pdst, rop) \
}

#define getandputrrop(psrc, srcbit, dstbit, width, pdst, rop) \
{ \
    register unsigned int _tmpbits; \
    getbits(psrc, srcbit, width, _tmpbits) \
    putbitsrrop(_tmpbits, dstbit, width, pdst, rop) \
}


#define getandputbits0(psrc, siindex, width, pdst) \
{			/* unroll the whole damn thing to see how it * behaves */ \
    register int          _flag = 32 - (siindex); \
    register unsigned int _src; \
 \
    _src = SCRLEFT (*(psrc), (siindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
 \
    *(pdst) = (*(pdst) & sistarttab1[(width)]) | (_src & siendtab1[(width)]); \
}


#define getandputrop0(psrc, siindex, width, pdst, rop) \
{			\
    register int          _flag = 32 - (siindex); \
    register unsigned int _src; \
 \
    _src = SCRLEFT (*(psrc), (siindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
    DoRop(_src, rop, _src, *(pdst)); \
 \
    *(pdst) = (*(pdst) & sistarttab1[(width)]) | (_src & siendtab1[(width)]); \
}

#define getandputrrop0(psrc, siindex, width, pdst, rop) \
{ \
    int             _flag = 32 - (siindex); \
    register unsigned int _src; \
 \
    _src = SCRLEFT (*(psrc), (siindex)); \
    if ((width) > _flag) \
	_src |=  SCRRIGHT (*((psrc) + 1), _flag); \
    _src = DoRRop(rop, _src, *(pdst)); \
 \
    *(pdst) = (*(pdst) & sistarttab1[(width)]) | (_src & siendtab1[(width)]); \
}

#endif  /* FASTGETBITS && FASTPUTBITS */
