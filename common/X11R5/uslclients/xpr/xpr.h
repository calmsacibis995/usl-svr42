#ident	"@(#)xpr:xpr.h	1.5"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

typedef struct Device {
	char			*name;
	short			terminfo;
	void			(*map)();
}			Device;

#define DEFAULT_DEVICE	"ln03"

/*
 * TEXT_GAP is the distance between the header/trailer and image.
 * SCALE_LIMIT is the largest size of a printed ``dot''; the limit
 * applies both vertically and horizontally for non-symmetric dots.
 * Both values are in inches.
 */
#define TEXT_GAP	0.25
#define SCALE_LIMIT	0.25

/*
 * TEXT_PT is the desired point size of the text for the header
 * and trailer. The actual point size will depend on the device
 * and/or the internal font used.
 */
#define TEXT_PT		15

/*
 * A "Word" is the type of choice for manipulating image bits.
 * It is the longest native type that fits in a register and
 * is handled efficiently by the machine.
 */
typedef unsigned long	Word;

#define WORDSIZE	(sizeof(Word) * 8)
#define WORDMASK	(WORDSIZE - 1)


#define STREQU(A,B)	(strcmp((A),(B)) == 0)
#define STRNEQU(A,B,N)	(strncmp((A),(B),(N)) == 0)
#define PERROR(n)	(n < sys_nerr? sys_errlist[n] : "unknown error")
#define min(x,y)	((x) < (y)? (x) : (y))
#define swap(x,y)	(x ^= y, y ^= x, x ^= y)

#define F_PORTRAIT	0x01
#define F_LANDSCAPE	0x02
#define F_INVERT	0x04
#define F_APPEND	0x10
#define F_NOFF		0x20
#define F_REPORT	0x40

#define SWAP(x,y)	(x ^= y, y ^= x, x ^= y)

#define BITS_PER_LINE(I) \
	( \
		(I)->format == ZPixmap? \
			  (I)->bits_per_pixel * (I)->width \
			: (I)->width + (I)->xoffset \
	)

#define BYTES_PER_LINE(I) \
	(ROUNDUP(BITS_PER_LINE(I), (I)->bitmap_pad) / 8);

#define PLANE_AREA(I)	((I)->bytes_per_line * (I)->height)

#define IMAGE_SIZE(I) \
	(((I)->format == XYPixmap? (I)->depth : 1) * PLANE_AREA(I))

/*
 * Most of the following can be found in various Xlib routines;
 * unfortunately they aren't defined in any X header files!
 */

#define ROUNDUP(N,D)	((((N) + (D) - 1) / (D)) * (D))

#define addr(I,X,Y) \
  (I)->data + (I->format == ZPixmap? ZINDEX(X,Y,I) : XYINDEX(X,Y,I))

#define XYINDEX(X,Y,I) \
  ((Y) * (I)->bytes_per_line) \
+ (((X) + (I)->xoffset) / (I)->bitmap_unit) * ((I)->bitmap_unit >> 3)

#define ZINDEX(X,Y,I) \
  ((Y) * (I)->bytes_per_line) \
+ (((X) * (I)->bits_per_pixel) >> 3)


#define isbitmap(I) \
	( \
		(I)->format == XYBitmap \
	     || (I)->format == XYPixmap && (I)->depth == 1 \
	     || (I)->format == ZPixmap && (I)->bits_per_pixel == 1 \
	)

/*
 * "getrow()" doesn't do any copying, it just gets the
 * address of the bit-row. "putrow()" does the copying.
 */
#define paddr(I,X,Y,P)		(addr((I),(X),(Y)) + (P) * PLANE_AREA(I))
#define getrow(I,Y,P)		paddr((I),0,(Y),(P))
#define putrow(I,A,Y,N,P)	memcpy(paddr((I),0,(Y),(P)), (A), (N))


/*
 * The following is the function for converting a color represented in
 * the RGB scheme into the intensity (L) of the HSL scheme for
 * representing colors. The most intense ``color'' is white, at
 * 100 * 65536.
 */
#define INTENSITY(PC)	((unsigned)39L * (PC)->red + (unsigned)50L * (PC)->green + (unsigned)11L * (PC)->blue)

/*
 * Choose a ``bit'' from a texture. The texture is not stored as an
 * image, but as a simple array. A ``bit'' is then an element of this
 * array, properly indexed.
 *
 * The following definition REQUIRES that the texture size be a power
 * of two!
 */
#define TEXTURE_BIT(T,X,Y,N)	T[N * (Y & (N-1))  +  (X & (N-1))]

/*
 * The following function computes the degree of difference between
 * two colors.
 */
#define DISTANCE(PA,PB)	RGBdistance((PA),(PB))

/**
 ** EXTERNS
 **/

extern int		sys_nerr,
			errno;

extern char		*sys_errlist[];

extern char		*input_filename;

extern Device		device_list[];

extern int		endian(),
			color_cmp();

extern char		*Malloc(),
			*Calloc();

extern void		Read(),
#ifndef MEMUTIL
			free(),
#endif /* MEMUTIL */
			parse_args(),
			read_image(),
			enlarge(),
			bit_invert(),
			swap2(),
			swap4(),
			swap_copy();

extern char		*_4x4[],
			**color_to_bw();

extern long		RGBdistance(),
			*color_to_color();

/*
 * The following are used for an unsupported feature.
 */
struct RGBmap {
	double			red,
				green,
				blue;
};
extern struct RGBmap	rgbmap;
