#ident	"@(#)siserver:include/servermd.h	1.5"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: servermd.h,v 1.51 90/03/10 15:18:25 keith Exp $ */

#ifndef SERVERMD_H
#define SERVERMD_H 1

/*
 * The vendor string identifies the vendor responsible for the
 * server executable.
 */

/*
 * I know, this is bad, bad, bad.  Changing this #define to secretly be a
 * reference to a global variable.  Well, this thing's got to be dynamic,
 * and I didn't want to go and start changing DIX code (main.c).  So we'll
 * pass on defining the string here, and let DDX later fix it up right.
 *
 * The agreed-upon format for this string is: "AT&T 6386 WGS", appended with
 * " with VDC 750" or " with VDC 400" depending on the graphics card we're
 * using.  Finally, " (using vtXX)" is added so interested clients can find
 * out which VT the server's running in.  It's a kludge, but there didn't
 * seem to be any other (read cleaner) way to pass this information along.
 */
#ifndef VENDOR_STRING
extern char *   vendor_string;
#define VENDOR_STRING vendor_string
#endif  /* VENDOR_STRING */


/*
 * The vendor release number identifies, for the purpose of submitting
 * traceable bug reports, the release number of software produced
 * by the vendor.
 */
#ifndef VENDOR_RELEASE
#define VENDOR_RELEASE	4
#endif

/*
 * Machine dependent values:
 * GLYPHPADBYTES should be chosen with consideration for the space-time
 * trade-off.  Padding to 0 bytes means that there is no wasted space
 * in the font bitmaps (both on disk and in memory), but that access of
 * the bitmaps will cause odd-address memory references.  Padding to
 * 2 bytes would ensure even address memory references and would
 * be suitable for a 68010-class machine, but at the expense of wasted
 * space in the font bitmaps.  Padding to 4 bytes would be good
 * for real 32 bit machines, etc.  Be sure that you tell the font
 * compiler what kind of padding you want because its defines are
 * kept separate from this.  See server/include/font.h for how
 * GLYPHPADBYTES is used.
 *
 * Along with this, you should choose an appropriate value for
 * GETLEFTBITS_ALIGNMENT, which is used in ddx/mfb/maskbits.h.  This
 * constant choses what kind of memory references are guarenteed during
 * font access; either 1, 2 or 4, for byte, word or longword access,
 * respectively.  For instance, if you have decided to to have
 * GLYPHPADBYTES == 4, then it is pointless for you to have a
 * GETLEFTBITS_ALIGNMENT > 1, because the padding of the fonts has already
 * guarenteed you that your fonts are longword aligned.  On the other
 * hand, even if you have chosen GLYPHPADBYTES == 1 to save space, you may
 * also decide that the computing involved in aligning the pointer is more
 * costly than an odd-address access; you choose GETLEFTBITS_ALIGNMENT == 1.
 *
 * Next, choose the tuning parameters which are appropriate for your
 * hardware; these modify the behaviour of the raw frame buffer code
 * in ddx/mfb and ddx/cfb.  Defining these incorrectly will not cause
 * the server to run incorrectly, but defining these correctly will
 * cause some noticeable speed improvements:
 *
 *  AVOID_MEMORY_READ - (8-bit cfb only)
 *	When stippling pixels on the screen (polytext and pushpixels),
 *	don't read long words from the display and mask in the
 *	appropriate values.  Rather, perform multiple byte/short/long
 *	writes as appropriate.  This option uses many more instructions
 *	but runs much faster when the destination is much slower than
 *	the CPU and at least 1 level of write buffer is availible (2
 *	is much better).  Defined currently for SPARC and MIPS.
 *
 *  FAST_CONSTANT_OFFSET_MODE - (cfb and mfb)
 *	This define is used on machines which have no auto-increment
 *	addressing mode, but do have an effectively free constant-offset
 *	addressing mode.  Only MIPS uses this currently.
 *	
 *  LARGE_INSTRUCTION_CACHE -
 *	This define increases the number of times some loops are
 *	unrolled.  On 68020 machines (with 256 bytes of i-cache),
 *	this define will slow execution down as instructions miss
 *	the cache frequently.  On machines with real i-caches, this
 *	reduces loop overhead, causing a slight performance improvement.
 *	Currently defined for MIPS and SPARC
 */

#if ( (defined(SYSV) || defined(SVR4)) && #machine(i386))
#ifdef BEFORE_SI
	/* XWIN, up to load X5 */
#define IMAGE_BYTE_ORDER        LSBFirst        /* Values for the 386/EGA */
#define BITMAP_BIT_ORDER        MSBFirst
#define GLYPHPADBYTES           4
#define GETLEFTBITS_ALIGNMENT   1
#define BITMAP_SCANLINE_UNIT    32
#define BITMAP_SCANLINE_PAD     32
#else
	/* XWIN with Screen Interface, everything after XWIN rel 4.0 */
#define IMAGE_BYTE_ORDER        LSBFirst        /* Values for the 386/EGA */
#define BITMAP_BIT_ORDER        LSBFirst
#define GLYPHPADBYTES           4
#define GETLEFTBITS_ALIGNMENT   4
#define BITMAP_SCANLINE_UNIT    32
#define BITMAP_SCANLINE_PAD     32
#endif /* BEFORE_SI */
#endif /* SYSV || SVR4 ....*/

#ifdef vax

#define IMAGE_BYTE_ORDER	LSBFirst        /* Values for the VAX only */
#define BITMAP_BIT_ORDER	LSBFirst
#define	GLYPHPADBYTES		1
#define GETLEFTBITS_ALIGNMENT	4
#define FAST_UNALIGNED_READS

#endif /* vax */

#ifdef sun

#if defined(sun386) || defined(sun5)
# define IMAGE_BYTE_ORDER	LSBFirst        /* Values for the SUN only */
# define BITMAP_BIT_ORDER	LSBFirst
#else
# define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the SUN only */
# define BITMAP_BIT_ORDER	MSBFirst
#endif

#ifdef sparc
# define AVOID_MEMORY_READ
# define LARGE_INSTRUCTION_CACHE
# define FAST_CONSTANT_OFFSET_MODE
#endif

#ifdef mc68020
#define FAST_UNALIGNED_READS
#endif

#define	GLYPHPADBYTES		4
#define GETLEFTBITS_ALIGNMENT	1

#endif /* sun */

#ifdef apollo

#define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the Apollo only*/
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		2
#define GETLEFTBITS_ALIGNMENT	4

#endif /* apollo */

#if defined(ibm032) || defined (ibm)

#if #machine(i386)
# define IMAGE_BYTE_ORDER	LSBFirst	/* Value for PS/2 only */
#else
# define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the RT only*/
#endif
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		1
#define GETLEFTBITS_ALIGNMENT	4
/* ibm pcc doesn't understand pragmas. */

#endif /* ibm */

#ifdef hpux

#define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the HP only */
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		2		/* to match product server */
#define	GETLEFTBITS_ALIGNMENT	1

#endif /* hpux */

#if defined(M4315) || defined(M4317) || defined(M4319) || defined(M4330)

#define IMAGE_BYTE_ORDER	MSBFirst        /* Values for Pegasus only */
#define BITMAP_BIT_ORDER	MSBFirst
#define GLYPHPADBYTES		4
#define GETLEFTBITS_ALIGNMENT	1

#define FAST_UNALIGNED_READS

#endif /* tektronix */

#ifdef macII

#define IMAGE_BYTE_ORDER      	MSBFirst        /* Values for the MacII only */
#define BITMAP_BIT_ORDER      	MSBFirst
#define GLYPHPADBYTES         	4
#define GETLEFTBITS_ALIGNMENT 	1

/* might want FAST_UNALIGNED_READS for frame buffers with < 1us latency */

#endif /* macII */

#ifdef mips

#ifdef MIPSEL
# define IMAGE_BYTE_ORDER	LSBFirst        /* Values for the PMAX only */
# define BITMAP_BIT_ORDER	LSBFirst
# define GLYPHPADBYTES		4
# define GETLEFTBITS_ALIGNMENT	1
#else
# define IMAGE_BYTE_ORDER	MSBFirst        /* Values for the MIPS only */
# define BITMAP_BIT_ORDER	MSBFirst
# define GLYPHPADBYTES		4
# define GETLEFTBITS_ALIGNMENT	1
#endif

#define AVOID_MEMORY_READ
#define FAST_CONSTANT_OFFSET_MODE
#define LARGE_INSTRUCTION_CACHE
#define PLENTIFUL_REGISTERS

#endif /* mips */

#ifdef stellar

#define IMAGE_BYTE_ORDER	MSBFirst       /* Values for the stellar only*/
#define BITMAP_BIT_ORDER	MSBFirst
#define	GLYPHPADBYTES		4
#define GETLEFTBITS_ALIGNMENT	4
/*
 * Use SysV random number generator.
 */
#define random rand

#endif /* stellar */

/* size of buffer to use with GetImage, measured in bytes. There's obviously
 * a trade-off between the amount of stack (or whatever ALLOCATE_LOCAL gives
 * you) used and the number of times the ddx routine has to be called.
 * 
 * for an 800 x 600 bit 16-color screen  with a 32 bit word we get
 * 16384/4 = 4096 words per buffer
 * ((800 * 4)/32) = 100 words per scanline
 * 4096 words per buffer / 100 words per scanline = 40 scanlines per buffer
 * 600 scanlines / 40 scanlines = 15 buffers to draw a full screen
 */
#if defined(stellar)
#define IMAGE_BUFSIZE		(64*1024)
#else
#define IMAGE_BUFSIZE                   16384
#endif


/* pad scanline to a longword */
#if defined(ibm) && #machine(i386)
#define BITMAP_SCANLINE_UNIT	8
#else
#define BITMAP_SCANLINE_UNIT	32
#endif
#define BITMAP_SCANLINE_PAD  32

#if (BITMAP_SCANLINE_UNIT == 32)

#define LOG2_BITMAP_UNIT                5
#define LOG2_BYTES_PER_SCANLINE_UNIT    2
#define BITMAP_MODMASK                  0x1f

#else                           /* for completeness */
# if (BITMAP_SCANLINE_UNIT == 16)
#define LOG2_BITMAP_UNIT                4
#define LOG2_BYTES_PER_SCANLINE_UNIT    1
#define BITMAP_MODMASK                  0xf
# else
#  if (BITMAP_SCANLINE_UNIT == 8)
#define LOG2_BITMAP_UNIT                3
#define LOG2_BYTES_PER_SCANLINE_UNIT    0
#define BITMAP_MODMASK                  0x7
#  endif                        /*  8 */
# endif                         /* 16 */
#endif                          /* 32 */


#if (BITMAP_SCANLINE_PAD == 32)

#define LOG2_BITMAP_PAD                 5
#define LOG2_BYTES_PER_SCANLINE_PAD     2

#else                           /* for completeness */
# if (BITMAP_SCANLINE_PAD == 16)
#define LOG2_BITMAP_PAD                 4
#define LOG2_BYTES_PER_SCANLINE_PAD     1
# else
#  if (BITMAP_SCANLINE_PAD == 8)
#define LOG2_BITMAP_PAD                 3
#define LOG2_BYTES_PER_SCANLINE_PAD     0
#  endif                        /*  8 */
# endif                         /* 16 */
#endif                          /* 32 */


/* handy conversions */
#define LOG2_BITS_PER_BYTE              3
#define BITS2BYTES(x)   ((x) >> LOG2_BITS_PER_BYTE)
#define BYTES2BITS(x)   ((x) << LOG2_BITS_PER_BYTE)
#define BITS2WORDS(x)   ((x) >> LOG2_BITMAP_UNIT)
#define WORDS2BITS(x)   ((x) << LOG2_BITMAP_UNIT)
#define BYTES2WORDS(x)  ((x) >> (LOG2_BITMAP_UNIT - LOG2_BITS_PER_BYTE))
#define WORDS2BYTES(x)  ((x) << (LOG2_BITMAP_UNIT - LOG2_BITS_PER_BYTE))

 /*
  *   This returns the number of padding units, for depth d and width w.
 * For bitmaps this can be calculated with the macros above.
 * Other depths require either grovelling over the formats field of the
 * screenInfo or hardwired constants.
 */

typedef struct _PaddingInfo {
#ifdef notdef
	int     padRoundUp;	/* pixels per pad unit - 1 */
	int	padPixelsLog2;	/* log 2 (pixels per pad unit) */
	int     padBytesLog2;	/* log 2 (bytes per pad unit) */
#else	/* AT&T hack */
        unsigned short padRoundUp;      /* pixels per pad unit - 1 */
        unsigned short padPixelsLog2;   /* log 2 (pixels per pad unit) */
        unsigned short padBytesLog2;    /* log 2 (bytes per pad unit) */
#endif
} PaddingInfo;
extern PaddingInfo PixmapWidthPaddingInfo[];

#define PixmapWidthInPadUnits(w, d) \
    (((unsigned) (w) + PixmapWidthPaddingInfo[d].padRoundUp) >> \
	PixmapWidthPaddingInfo[d].padPixelsLog2)

/*
 *	Return the number of bytes to which a scanline of the given
 * depth and width will be padded.
 */
#define PixmapBytePad(w, d) \
    (PixmapWidthInPadUnits(w, d) << PixmapWidthPaddingInfo[d].padBytesLog2)

#endif /* SERVERMD_H */
