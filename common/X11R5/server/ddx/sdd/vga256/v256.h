/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256.h	1.2"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#ifdef DEBUG
extern int xdebug;
#define DBENTRY(func) if (xdebug & 0x10) printf("%s\n", func);
#define DBENTRY1(func) if (xdebug & 0x20) printf("%s\n", func);
#define DBENTRY2(func) if (xdebug & 0x40) printf("%s\n", func);
#else
#define DBENTRY(func)
#define DBENTRY1(func)
#define DBENTRY2(func)
#endif

extern	struct	at_disp_info	vt_info;


#ifndef	VGA_PAGE_SIZE
#error 	VGA_PAGE_SIZE must be defined !
#endif	/* VGA_PAGE_SIZE */

#define VIDEO_PAGE_MASK 	(VGA_PAGE_SIZE-1)

/*
 * Select a page of video memory : 
 */
#define	selectpage(j)	\
	{\
		if ( (j > v256_endpage) || (j < (v256_endpage - VIDEO_PAGE_MASK)))\
			(*vt_info.ext_page)(j);\
	}


extern	int	v256_clip_x1;		/* clipping region */
extern	int	v256_clip_y1;
extern	int	v256_clip_x2;
extern	int	v256_clip_y2;

extern	int	v256_slbytes;		/* number of bytes in a scanline */
extern	int	v256_is_color;		/* true if on a color monitor */
#define	v256_fb	(vt_info.vt_buf)	/* V256 frame buffer pointer */

#define v256_swap(a, b, t)	{t = a; a = b; b = t;}

/*
 * Define the maximum width of any supported screen in pixels (including
 * logical screens that have a panned physical window).
 */
#define MAXSCANLINE 2048

/*
 * Various defines for tiles and stipples (collectively called "patterns").
 * WARNING:  THE MAX PATTERN WIDTH IS A DEFINE HERE, BUT VARIOUS PARTS OF
 *           THE CODE WILL BREAK IF THIS VALUE IS CHANGED.
 *
 * A width of 16 works well because it is the most common X pattern size, 
 * and a majority of the other common sizes can be built up to 16.  (Other
 * common sizes are 1, 2, 4, 8).
 */
#define	V256_PAT_W	32		/* max pattern width (multiple of 8)*/
#define	V256_PAT_H	32		/* max pattern height */
#define V256_PATBYTES	(V256_PAT_W * V256_PAT_H) /* bytes in pattern */
#define V256_BADPAT	0x80000000	/* used in fill_mode for bad pattern */
extern	BYTE 	*v256_cur_pat;		/* current pattern */
extern	int		v256_cur_pat_h;		/* current pattern's height */

/*
 *	Font handling defines
 */
#define	V256_NUMDLFONTS		8	/* max number of downloadable fonts */
#define	V256_NUMDLGLYPHS	256	/* max number of glyphs per font */
#define	V256_DL_FONT_W		25	/* width of downloaded glyph */
#define	V256_DL_FONT_H		32	/* height of downloaded glyph */

/* 
 *	internal font info structure 
 */
typedef	struct	v256_font 
{	
	int		w;			/* width of glyphs */
	int		h;			/* height of glyphs */
	int		ascent;			/* distance from top to baseline */
	BYTE	*glyphs; 		/* pointer to glyph data */
} v256_font;

extern	v256_font v256_fonts[];		/* downloaded font info */

#define V256_MAXCOLOR	256		/* maximum number of colors in a map */
#define V256_PLANES		8		/* number of planes */

typedef	struct	v256_rgb 
{
	BYTE	red;
	BYTE	green;
	BYTE	blue;
}	v256_rgb;

/* 
 *	V256 pallette registers 
 */
extern  v256_rgb v256_pallette[V256_MAXCOLOR];

typedef struct	v256_state 
{
	BITS16	pmask;			/* plane mask */
	int		mode;			/* graphics mode */
	int		stp_mode;		/* stipple mode */
	int		fill_mode;		/* fill mode */
	int		fill_rule;		/* fill rule */
	BYTE	fg;				/* foreground color */
	BYTE	bg;				/* background color */
	int		stpl_h;			/* stipple height */
	BYTE	stpl[V256_PATBYTES/8];	/* stipple pattern */

	BYTE	stpl_valid;		/* true if stipple has been converted */
	BYTE	tile_valid;		/* true if tile has been converted */
	SIbitmap raw_stipple;	/* stipple info downloaded */
	SIbitmap raw_tile;		/* tile info downloaded */
	BYTE	raw_stpl_data[V256_PAT_H*4];	/* stipple pattern downloaded */
	BYTE	*raw_tile_data;	/* tile pattern downloaded */
	BYTE	*big_stpl;		/* pointer to LARGE stipple data */
}	v256_state;
	
#define	V256_NUMGS	4		/* number of graphic states */
extern	v256_state	v256_gstates[];	/* grapics states */

extern	int			v256_cur_state;	/* current state selected */
extern	v256_state	*v256_gs;	/* pointer to current state */

extern	BYTE	v256_slbuf[];		/* buffer for a scanline */
extern	BYTE	v256_tmpsl[];		/* temporary buffer for a scanline */

extern	BYTE	v256_src;		/* current source color */
extern	int		v256_invertsrc;		/* currently inverting source? */
extern	int		v256_function;		/* current V256 function */

extern	int		v256_page;		/* current page of memory in use */
extern	int		v256_endpage;		/* last valid offset from v256_fb */


/*
 * V256 defines
 */

#define V256_COPY	0x0000	/* Data unmodified */
#define V256_AND	0x0800	/* Data AND'ed with latches */
#define V256_OR		0x1000	/* Data OR'ed with latches */
#define V256_XOR	0x1800	/* Data XOR'ed with latches */

#define V256_INVERT (V256_XOR + 1)
#define V256_OR_INVERT (V256_XOR + 2)
#define V256_AND_INVERT (V256_XOR + 3)
