/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vga.h	1.5"

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

extern	int	vga_clip_x1;		/* clipping region */
extern	int	vga_clip_y1;
extern	int	vga_clip_x2;
extern	int	vga_clip_y2;

extern	struct	at_disp_info	vt_info;
extern	BYTE	*vga_write_map;
extern	BYTE	*vga_read_map;
extern	BYTE	*vga_color_map;
extern	BYTE	*vga_attr_map;

extern	int	vt_allplanes;
extern	int	vga_slbytes;		/* number of bytes in a scanline */
extern	int	vga_is_color;		/* true if on color monitor */
extern	BITS16	gr_mode;		/* write mode 0 setting */
#define	vga_fb	vt_info.vt_buf		/* VGA frame buffer pointer */

extern	int	vt_screen_w;	/* width of visible screen */
extern	int	vt_screen_h;	/* height of visible screen */
extern	int     vt_screen_x;    /* x position of UL corner of visible screen */
extern	int     vt_screen_y;    /* y position of UL corner of visible screen */

#define vga_byteoffset(x, y) (((y) * vga_slbytes) + ((x) >> 3))
#define vga_bitvalue(x) (1 << (7 - ((x) & 0x7)))

#define vga_swap(a, b, t)	{t = a; a = b; b = t;}

/*
 * Various defines for tiles and stipples (collectively called "patterns").
 * WARNING:  THE MAX PATTERN WIDTH IS A DEFINE HERE, BUT VARIOUS PARTS OF
 *           THE CODE WILL BREAK IF THIS VALUE IS CHANGED.
 *
 * A width of 16 works well because it is the most common X pattern size, 
 * and a majority of the other common sizes can be built up to 16.  (Other
 * common sizes are 1, 2, 4, 8).
 */
#define	VGA_PAT_W	16		/* max pattern width */
#define	VGA_PAT_H	16		/* max pattern height */
#define VGA_PATBYTES	(VGA_PAT_W / 8 * VGA_PAT_H) /* bytes in pattern */
#define VGA_BADPAT	0x80000000	/* used in fill_mode for bad pattern */
extern	BYTE 	*cur_pat;		/* current pattern */
extern	int	cur_pat_h;		/* current pattern's height */

#define	VGA_NUMDLFONTS	8		/* max number of downloadable fonts */
#define	VGA_NUMDLGLYPHS	256		/* max number of glyphs per font */
#define	VGA_DL_FONT_W	16		/* width of downloaded glyph */
#define	VGA_DL_FONT_H	32		/* height of downloaded glyph */

typedef	struct	vga_font {		/* internal font info structure */
	int	w;			/* width of glyphs */
	int	h;			/* height of glyphs */
	int	ascent;			/* distance from top to baseline */
	BYTE	*glyphs; 		/* pointer to glyph data */
} vga_font;

extern	vga_font vga_fonts[];		/* downloaded font info */

#define VGA_MAXCOLOR	16		/* maximum number of colors in a map */

typedef	struct	vga_rgb {
	BYTE	red;
	BYTE	green;
	BYTE	blue;
} vga_rgb;

extern	BYTE	ega_cmap[VGA_MAXCOLOR];	/* EGA/VGA color attribute settings */
extern  vga_rgb	vga_pallette[VGA_MAXCOLOR]; /* VGA pallette registers */

typedef struct	vga_state {
	BITS16	pmask;			/* plane mask */
	int	mode;			/* graphics mode */
	int	stp_mode;		/* stipple mode */
	int	fill_mode;		/* fill mode */
	int	fill_rule;		/* fill rule */
	int	line_count;		/* number of on-off patterns in line */
	BYTE	fg;			/* foreground color */
	BYTE	bg;			/* background color */
	int	tile_h;			/* tile height */
	int	tile_one;		/* true if tile is 1 byte wide */
	BYTE	tile[VGA_PATBYTES*4];	/* tile pattern */
	int	stpl_h;			/* stipple height */
	int	stpl_one;		/* true if stipple is 1 byte wide */
	BYTE	stpl[VGA_PATBYTES];	/* stipple pattern */
	BYTE	inv_stpl[VGA_PATBYTES];	/* inverted stipple pattern */

	BYTE	stpl_valid;		/* true if stipple has been converted */
	BYTE	tile_valid;		/* true if tile has been converted */
	SIbitmap raw_stipple;		/* stipple info downloaded */
	SIbitmap raw_tile;		/* tile info downloaded */
	BYTE	raw_stpl_data[VGA_PAT_H*4];	/* stipple pattern downloaded */
	BYTE	raw_tile_data[VGA_PAT_H*8];	/* tile pattern downloaded */
	BYTE	*big_stpl;		/* pointer to LARGE stipple data */
	BYTE	*big_tile;		/* pointer to LARGE tile data */
} vga_state;
	
#define VGA_NUMGS	4		/* number of graphic states */
extern	vga_state	vga_gstates[];	/* grapics states */
extern	vga_state	*vga_gs;	/* pointer to current state */

extern BYTE	vga_slbuf[];		/* buffer for a scanline */
extern BYTE	vga_tmpsl[];		/* temporary buffer for a scanline */

extern	BYTE	vga_src;		/* current source color */
extern	SIBool	vga_invertsrc;		/* need to invert source? */
extern	SIBool	vga_invertdest;		/* need to invert dest? */
extern	int	vga_function;		/* current VGA function */

#define VGA_NUMCUR	4		/* number of downloadable cursors */
#define	VGA_CURWIDTH	16		/* max cursor width */
#define	VGA_CURHEIGHT	16		/* max cursor height */
#define VGA_CURBW	4		/* cursor width in bytes */
#define VGA_CURBYTES	(VGA_CURBW*VGA_CURHEIGHT) /* bytes in a cursor */

/*
 *	Internally, a cursor is stored in scanlines with a byte of padding
 *	at the front of each line to shift into when needed.
 */

typedef struct vga_cursor {		/* internal cursor format */
	int	w;			/* width of cursor */
	int	h;			/* height of cursor */
	BYTE	fg;			/* foreground color */
	BYTE	bg;			/* background color */
	BYTE	*mask;			/* cursor mask */
	BYTE	*face;			/* cursor face */
} vga_cursor;

extern	vga_cursor	vga_cursors[];	/* downloaded cursor masks */
extern	int	vga_curs_on;		/* true if cursor is currently on */
extern	BYTE	*vga_curs_addr;		/* address of current cursor */

extern BYTE vga_bitflip[];		/* flips bits around */
extern BYTE vga_start_bits[];           /* Bits at start of scanline */
extern BYTE vga_end_bits[];             /* Bits at end of scanline */
extern BYTE vga_2_start_bits[];         /* 2 plane bits at start of scanline */
extern BYTE vga_2_end_bits[];           /* 2 plane bits at end of scanline */
extern BYTE vga_4_start_bits[];         /* 4 plane bits at start of scanline */
extern BYTE vga_4_end_bits[];           /* 4 plane bits at end of scanline */

extern	int (*vga_stpls[])();
extern	int (*ega_stpls[])();

/* To avoid calls to malloc and free, try to allocate temporary space on the
 * stack.  However, when a lot of space is needed, this is not feasible.  Set
 * the threshold for stack allocation.
 */
#define LCL_BUF_SIZE	8192

#define MULTIDEPTHS	1

/*
 * SVR4 : __STDC__ is defined by default in SVR4 ES
 */
#ifdef __STDC__
#define VOLATILEBYTE volatile BYTE
#else
#define VOLATILEBYTE BYTE
#endif
