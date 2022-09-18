/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sidep.h	1.6"

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

/*
 * Machine specific constants for various Display Boards
 */

#ifndef	_SIDEP_H
#define _SIDEP_H

#include	"X.h"
#include	"Xmd.h"

/* The base definitions are from Xmd.h */

typedef	INT32		SIBool;
typedef INT32		SIAvail;
typedef INT32		SIint32;
typedef INT16		SIint16;
typedef void		SIvoid;

typedef CARD8		*SILine;
typedef INT32		*SIArray;
typedef void		*SIvoidP;	/* V2.0 */

	/* Possible states of SIBool variables */
#define	SI_TRUE		1
#define	SI_SUCCEED	1

#define SI_FALSE	0
#define SI_FAIL		0
#define SI_FATAL	-1

/*
 * This structure defines the format of a color as used between
 * the SI independent and dependant sections.  The values are
 * in the range of 0x0000 - 0xFFFF.  The value 0x0000 means that
 * The component is totally turned off.  The value 0xFFFF means
 * that the color component is as high as possible.
 */

#define VALID_RED       0x00000001
#define VALID_GREEN     0x00000002
#define VALID_BLUE      0x00000003

typedef struct SIColor	{
	SIint32		SCpindex;	/* Pixel in question */
	SIint16		SCvalid;	/* Valid color component */
	SIint16		SCred;		/* Red component */
	SIint16		SCgreen;	/* Green component */
	SIint16		SCblue;		/* Blue Component */
} SIColor, *SIColorP;

/*
 * This structure defines the visual information structure.
 * A list of these structures is passed to the SI to specifiy
 * the set of visual types that are managed by the SDD.
 */
typedef struct SIVisual	{
	SIint32		SVtype;		/* Type PSEUDOCOLOR_AVAIL... */
	SIint32		SVdepth;	/* Depth of visual: 1,2,4,8,16,24,32 */
	SIint32		SVcmapcnt;	/* Number of colormaps */
	SIint32		SVcmapsz;	/* Colormap size */
	SIint32		SVbitsrgb;	/* Valid bits per RGB */
	SIint32		SVredmask;	/* Valid Red mask (direct/true) */
	SIint32		SVgreenmask;	/* Valid Green mask (direct/true) */
	SIint32		SVbluemask;	/* Valid Blue mask (direct/true) */
	SIint32		SVredoffset;	/* offset for Red mask */
	SIint32		SVgreenoffset;	/* offset for Green mask */
	SIint32		SVblueoffset;	/* offset for Blue mask */
} SIVisual, *SIVisualP;

typedef struct siCmap {
	int	display;	/* what display the colormap is for */
	int	screen;		/* which screen */
	int	visual;		/* visual type (PSEUDOCOLOR_AVAIL ...) */
	int	sz;		/* colormap size */
	unsigned short *colors;	/* pointer to RGB values for colors */
} siCmap, *siCmapP;

/*
 * This structure defines for format of points sent between the
 * SI independent and the dependant sections.
 */

typedef struct SIPoint	{
	SIint16		x, y;
} SIPoint, *SIPointP;

/*
 * This structure defines the format of a rectangle data structure
 * that is sent between the SI independent and the dependent sections.
 */

typedef struct SIRect	{
	SIPoint	ul;
	SIPoint lr;
} SIRect, *SIRectP;

/*
 * This structure defines the format of a 1 or 8 bit deep bitmap structure.
 * This structure is always some number of INT32 items wide, and some
 * number of pixels in height.  That means that the structure is always
 * INT32 aligned, and may be padded by as many as (sizeof(INT32)-1)/depth bits.
 *
 * The Bprivate pointer may be used by memory caching routines as a
 * hook to associate private data structures with SIbitmap structures.
 */

#define	XY_BITMAP	0		/* XY format bitmap */
#define	Z_BITMAP	1		/* Z format bitmap */
#define	XY_PIXMAP	2		/* same as SCREEN format pixmap */
#define	Z_PIXMAP	3

typedef struct SIbitmap {
	SIint32		Btype;		/* Z_BITMAP, etc */
	SIint32		Bdepth;		/* In Bits */
	SIint32		Bwidth;		/* In pixels */
	SIint32		Bheight;	/* In pixels */
	SIint32		BorgX, BorgY;	/* Tile/Stipple Origin */
	SIArray		Bptr;		/* Bitmap location in user memory */
	SIvoidP		Bprivate;	/* For SI use only */
} SIbitmap, *SIbitmapP;

#define NullSIbitmap ((SIbitmap *)0)

/*
 * This structure contains the graphics state definition.  A Graphics
 * State is the collection of graphics modes, colors and patterns
 * used when drawing the various output facilities.
 */

#define SetSGpmask	0x00000001
#define SetSGmode	0x00000002
#define SetSGstplmode	0x00000004
#define SetSGfillmode	0x00000008
#define SetSGfillrule	0x00000010
#define SetSGarcmode	0x00000020
#define SetSGlinestyle	0x00000040
#define SetSGfg		0x00000080
#define SetSGbg		0x00000100
#define SetSGcmapidx	0x00000200
#define SetSGvisualidx	0x00000400
#define SetSGtile	0x00000800
#define SetSGstipple	0x00001000
#define SetSGline	0x00002000
#define SetSGcliplist	0x00004000

#define GetSGpmask	0x00010000
#define GetSGmode	0x00020000
#define GetSGstplmode	0x00040000
#define GetSGfillmode	0x00080000
#define GetSGfillrule	0x00100000
#define GetSGarcmode	0x00200000
#define GetSGlinestyle	0x00400000
#define GetSGfg		0x00800000
#define GetSGbg		0x01000000
#define GetSGcmapidx	0x02000000
#define GetSGvisualidx	0x04000000

#define SGFillSolidFG	0x00000001
#define SGFillSolidBG	0x00000002
#define SGFillStipple	0x00000004
#define SGFillTile	0x00000008

#define SGEvenOddRule	0x00000001
#define SGWindingRule	0x00000002

#define SGArcChord	0x00000001
#define SGArcPieSlice	0x00000002

#define SGLineSolid	0x00000001
#define SGLineDash	0x00000002
#define SGLineDblDash	0x00000004

#define SGStipple	0x00000001
#define SGOPQStipple	0x00000002

typedef struct SIGState {
	SIint32		SGpmask;	/* Current plane mask */
	SIint32		SGmode;		/* Graphics mode */
	SIint32		SGstplmode;	/* Stipple blt mode */
	SIint32		SGfillmode;	/* Graphics fill mode */
	SIint32		SGfillrule;	/* Polygon fill rule */
	SIint32		SGarcmode;	/* arc mode */
	SIint32		SGlinestyle;	/* line style */
	SIint32		SGfg;		/* Foreground pixel index */
	SIint32		SGbg;		/* Background pixel index */
	SIint32		SGcmapidx;	/* Colormap index */
	SIint32		SGvisualidx;	/* Visual index */
	SIbitmapP	SGtile;		/* Tile pattern */
	SIbitmapP	SGstipple;	/* Stipple pattern */
	SIint32		SGlineCNT;	/* Number of line on/off entries */
	SIint32		*SGline;	/* Line pattern (on/off) pairs */
	SIRectP		SGcliplist;	/* list of clipping rectangles */
	SIint32		SGclipCNT;	/* Number of clipping rectangles */
	SIRect		SGclipextent;	/* Bounding box of clipping rects */
} SIGState, *SIGStateP;

/*
 * This structure is used in defining a hardware cursor.
 */

typedef struct SICursor	{
	SIint32		SCfg;		/* Foreground pixel index */
	SIint32		SCbg;		/* Background pixel index */
	SIint32		SCwidth;	/* Cursor width */
	SIint32		SCheight;	/* Cursor height */
	SIbitmapP	SCmask;		/* Mask pattern */
	SIbitmapP	SCsrc;		/* Src cursor pattern */
	SIbitmapP	SCinvsrc;	/* Inverse Source pattern */
} SICursor, *SICursorP;

/*
 * Configuration file structure for init.
 */

typedef struct	SIConfig {
	char	*resource;	/* resource type, eg. "mouse", "display"  */
	char	*type;		/* device type, eg. "VGA", "MF-T8"        */
	SIint32	deflt;		/* default information.  For displays, this */
				/* is the visual type, eg PSEUDOCOLOR_AVAIL */
	char	*info;		/* device-dependent info, "640x480"       */
	char	*display;	/* display.screen string, eg. "0.0"       */
	SIint32	displaynum,	/* display number [note conflict w/ above] */
		screen;		/* screen number, -1 if not specified     */
	char	*device;	/* device name, "/dev/ms1"                */
} SIConfig, *SIConfigP;

/*
 * This structure contains the per glyph information in a downloaded font
 */

typedef struct	SIGlyph {
	SIint16		SFlbearing;	/* left side bearing */
	SIint16		SFrbearing;	/* right side bearing */
	SIint16		SFwidth;	/* glyph width */
	SIint16		SFascent;	/* glyph ascent */
	SIint16		SFdescent;	/* glyph descent */
	SIbitmap	SFglyph;	/* The actual glyph */
} SIGlyph, *SIGlyphP;

/*
 * This structure contains the font bounds information, and is used to
 * determine if a font is "downloadable" to the SDD.
 */

#define SFTerminalFont		0x00000001
#define SFFixedWidthFont	0x00000002
#define SFNoOverlap		0x00000004

typedef struct	SIFontInfo {
	SIint32		SFnumglyph;	/* Number of glyphs in the font */
	SIint32		SFflag;		/* Font characteristics flag */
	SIint16		SFlascent;	/* Font logical acsent */
	SIint16		SFldescent;	/* Font logical decsent */
	SIGlyph		SFmin;		/* Minimum font bounds */
	SIGlyph		SFmax;		/* Maximum font bounds */
} SIFontInfo, *SIFontInfoP;


/*
 * Screen Interface Version Numbers
 */
/*
 * OLD UNSUPPORTED SI versions
 */
#define SBI_VERSION_1p1		0x00010001	/* Version 1.1 */
#define SBI_VERSION_1p2		0x00010002	/* Version 1.2 */

/*
 * Currently supported SI version
 */
#define SI_VERSION_2p0		0x00020000	/* Version 2.0 */

/*
 * This structure is used when finding out the initial information
 * about the display in question.
 *
 * The avail_* members are 32 bit quantities with a structured layout:
 *
 *	bits (0-7)	indicates functions available
 *	bits (8-15)	indicates function specific options available
 *	bits (16-31)	indicates general options available
 */
typedef struct SIInfo	{
	SIint32		SIversion;	/* Version of the SI */
	SIint32		SIlinelen;	/* length of a scan line */
	SIint32		SIlinecnt;	/* number of scan lines */
	SIint32		SIxppin;	/* Pixels per inch (X direction) */
	SIint32		SIyppin;	/* Pixels per inch (Y direction) */
	SIint32		SIstatecnt;	/* number of graphics states */
	SIAvail		SIavail_bitblt;	/* Has bitblt routines */
	SIAvail		SIavail_stplblt;/* Has stipple blt routines */
	SIAvail		SIavail_fpoly;	/* Has polyfill routines */
	SIAvail		SIavail_point;	/* Has pointplot routine */
	SIAvail		SIavail_line;	/* Has line draw routines */
	SIAvail		SIavail_drawarc;/* Has draw arc routines */
	SIAvail		SIavail_fillarc;/* Has fill arc routines */
	SIAvail		SIavail_font;	/* Has hardware fonts */
	SIAvail		SIavail_spans;	/* Has spans routines */
	SIAvail		SIavail_memcache;/* Has memory caching routines */
	SIAvail		SIcursortype;	/* Cursor type FAKEHDWR/TRUEHDWR */
	SIint32		SIcurscnt;	/* Number of downloadable cursors */
	SIint32		SIcurswidth;	/* Best Cursor Width */
	SIint32		SIcursheight;	/* Best Cursor Height */
	SIint32		SIcursmask;	/* Mask for cursor bounds */
	SIint32		SItilewidth;	/* Best width for tile */
	SIint32		SItileheight;	/* Best height for tile */
	SIint32		SIstipplewidth;	/* Best width for stipple */
	SIint32		SIstippleheight;/* Best height for stipple */
	SIint32		SIfontcnt;	/* Number of downloadable fonts */
	SIVisualP	SIvisuals;	/* List of visuals supported by SDD */
	SIint32		SIvisualCNT;	/* Number of SDD visuals */
	SIAvail		SIavail_exten;	/* Has extensions */
} SIInfo, *SIInfoP;

/*
 * This structure contains a list of all the basic routines that 
 * must be written for the Screen Interface to work properly.
 */
typedef struct ScreenInterface {
		/* MISCELLANEOUS ROUTINES	*/
		/*	MANDATORY		*/

	SIBool	(*si_init)();			/* machine dependant init */
	SIBool	(*si_restore)();		/* machine dependant cleanup */
	SIBool	(*si_vt_save)();		/* machine dependant vt flip */
	SIBool	(*si_vt_restore)();		/* machine dependant vt flip */
	SIBool	(*si_vb_onoff)();		/* Turn on/off video blank */
	SIBool	(*si_initcache)();		/* start caching requests */
	SIBool	(*si_flushcache)();		/* write caching requests */
	SIBool	(*si_download_state)();		/* Set current state info */
	SIBool	(*si_get_state)();		/* Get current state info */
	SIBool	(*si_select_state)();		/* Select current GS entry */
	SIBool	(*si_screen)();			/* Enter/Leave a screen */

		/* SCANLINE AT A TIME ROUTINES	*/
		/*	MANDATORY		*/

	SILine	(*si_getsl)();			/* get pixels in a scanline */
	SIvoid	(*si_setsl)();			/* set pixels in a scanline */
	SIvoid	(*si_freesl)();			/* free scanline buffer */

		/* COLORMAP MANAGEMENT ROUTINES	*/
		/*	MANDATORY		*/

	SIBool	(*si_set_colormap)();		/* Set Colormap entries */
	SIBool	(*si_get_colormap)();		/* Get Colormap entries */

		/* CURSOR CONTROL ROUTINES	*/
		/*	MANDATORY		*/

	SIBool	(*si_hcurs_download)();		/* Download a cursor */
	SIBool	(*si_hcurs_turnon)();		/* Turnon the cursor */
	SIBool	(*si_hcurs_turnoff)();		/* Turnoff the cursor */
	SIBool	(*si_hcurs_move)();		/* Move the cursor position */

		/* HARDWARE SPANS CONTROL	*/
		/*	OPTIONAL		*/
	SIBool	(*si_fillspans)();		/* fill spans */

		/* HARDWARE BITBLT ROUTINES	*/
		/*	OPTIONAL		*/

	SIBool	(*si_ss_bitblt)();		/* perform scr->scr bitblt */
	SIBool	(*si_ms_bitblt)();		/* perform mem->scr bitblt */
	SIBool	(*si_sm_bitblt)();		/* perform scr->mem bitblt */

		/* HARDWARE BITBLT ROUTINES	*/
		/*	OPTIONAL		*/

	SIBool	(*si_ss_stplblt)();		/* perform scr->scr stipple */
	SIBool	(*si_ms_stplblt)();		/* perform mem->scr stipple */
	SIBool	(*si_sm_stplblt)();		/* perform scr->mem stipple */

		/* HARDWARE POLYGON FILL	*/
		/*	OPTIONAL		*/

	SIvoid	(*si_poly_clip)();		/* set polygon clip */
	SIBool	(*si_poly_fconvex)();		/* for convex polygons */
	SIBool	(*si_poly_fgeneral)();		/* for general polygons */
	SIBool	(*si_poly_fillrect)();		/* for rectangular regions */

		/* HARDWARE POINT PLOTTING	*/
		/*	OPTIONAL		*/

	SIBool	(*si_plot_points)();

		/* HARDWARE LINE DRAWING	*/
		/*	OPTIONAL		*/

	SIvoid	(*si_line_clip)();		/* set line draw clip */
	SIBool	(*si_line_onebitline)();	/* One bit line (connected) */
	SIBool	(*si_line_onebitseg)();		/* One bit line segments */
	SIBool	(*si_line_onebitrect)();	/* One bit line rectangles */

		/* HARDWARE DRAW ARC ROUTINE	*/
		/*	OPTIONAL		*/

	SIvoid	(*si_drawarc_clip)();		/* set drawarc clip */
	SIBool	(*si_drawarc)();		/* draw arc */

		/* HARDWARE FILL ARC ROUTINE	*/
		/*	OPTIONAL		*/

	SIvoid	(*si_fillarc_clip)();		/* set fill arc clip */
	SIBool	(*si_fillarc)();		/* fill arc */

		/* HARDWARE FONT CONTROL	*/
		/*	OPTIONAL		*/
	SIBool	(*si_font_check)();		/* Check Font downloadability */
	SIBool	(*si_font_download)();		/* Download a font command */
	SIBool	(*si_font_free)();		/* free a downloaded font */
	SIvoid	(*si_font_clip)();		/* set font clip */
	SIBool	(*si_font_stplblt)();		/* stipple a list of glyphs */


		/* SDD MEMORY CACHING CONTROL	*/
		/*	OPTIONAL		*/
	SIBool	(*si_cache_alloc)();		/* allocate pixmap into cache */
	SIBool	(*si_cache_free)();		/* remove pixmap from cache */
	SIBool	(*si_cache_lock)();		/* lock pixmap into cache */
	SIBool	(*si_cache_unlock)();		/* unlock pixmap from cache */


		/* SDD EXTENSION INITIALIZATION */
		/*	OPTIONAL		*/
	SIBool	(*si_exten_init)();		/* extension initialization */
} ScreenInterface;

extern	ScreenInterface		*HWroutines;
extern	SIInfo			*HWinfo;
extern	char			*siSTATEerr;

/*
 * general availability flags
 */
#define TILE_AVAIL		0x10000
#define OPQSTIPPLE_AVAIL	0x20000
#define STIPPLE_AVAIL		0x40000

#define	si_hastile(avail)	((HWinfo->avail) & TILE_AVAIL)
#define	si_hasstipple(avail)	((HWinfo->avail) & STIPPLE_AVAIL)
#define	si_hasopqstipple(avail)	((HWinfo->avail) & OPQSTIPPLE_AVAIL)

#define CLIPLIST_AVAIL		0x80000

#define	si_hascliplist(avail)	((HWinfo->avail) & CLIPLIST_AVAIL)

#define DASH_AVAIL		0x100000
#define DBLDASH_AVAIL		0x200000

#define	si_hasdash(avail)	((HWinfo->avail) & DASH_AVAIL)
#define	si_hasdbldash(avail)	((HWinfo->avail) & DBLDASH_AVAIL)

#define CHORD_AVAIL		0x400000
#define PIESLICE_AVAIL		0x800000

#define	si_haschord(avail)	((HWinfo->avail) & CHORD_AVAIL)
#define	si_haspieslice(avail)	((HWinfo->avail) & PIESLICE_AVAIL)

/*
 * miscellaneous defines
 */

#define	si_Init			(HWroutines->si_init)
#define	si_Restore		(HWroutines->si_restore)
#define	si_savescreen		(HWroutines->si_vt_save)
#define	si_restorescreen	(HWroutines->si_vt_restore)
#define	si_setvideo		(HWroutines->si_vb_onoff)
#define	si_Initcache		(HWroutines->si_initcache)
#define	si_Flushcache		(HWroutines->si_flushcache)
#define	si_downloadstate	(HWroutines->si_download_state)
#define	si_getstateinfo		(HWroutines->si_get_state)
#define	si_selectstate		(HWroutines->si_select_state)

/* Macro(s) for accessing parts of the info structure */

#define	si_GetInfoVal(a)	(HWinfo->a)
#define	si_SetInfoVal(a,b)	( (HWinfo->a) = (b) )

/* Commonly accessed values */

#define	si_getscanlinelen	si_GetInfoVal(SIlinelen)
#define	si_getscanlinecnt	si_GetInfoVal(SIlinecnt)

/* Macro(s) for accessing common subroutines */

/*
 * si_PrepareGS ust be called in si before any graphics drawing occurs
 * This version does not force a validate.  A validate is forced as ValidateGC
 * time.
 * R3->R4 changes: 6/26/90
 */

#define si_PrepareGS(pGC)	\
	{ SIint32 _index = ((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GStateidx; \
		sivalidatestate(_index, \
				&((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GState, \
				SI_FALSE, \
				((siPrivGC *)(pGC)->devPrivates[siGCPrivateIndex].ptr)->GSmodified); \
		if ( si_selectstate(_index) == SI_FAIL ) \
			FatalError(siSTATEerr); \
	}

/*
 * si_PrepareGS2 must be called in si before any graphics drawing occurs
 * This version always forces a change.  It is used for window operations.
 */

#define si_PrepareGS2(index,pGS)	\
	{ \
		sivalidatestate((index),(pGS), SI_TRUE, (SIint32)0); \
		if ( si_selectstate(index) == SI_FAIL ) \
			FatalError(siSTATEerr); \
	}

/*
 * si_PrepareGS3 must be called in si before any graphics drawing occurs
 * This version always forces a change.  It is used for window operations.
 * This version optinally allows cliplist/tiles/stipples to be changed.
 */

#define si_PrepareGS3(index,pGS,mod)	\
	{ \
		sivalidatestate((index),(pGS), SI_TRUE, (SIint32)mod); \
		if ( si_selectstate(index) == SI_FAIL ) \
			FatalError(siSTATEerr); \
	}

/*
 * Routine to enter/leave a screen (for multiheaded support)
 */
#define	SCREEN_ENTER	1
#define	SCREEN_LEAVE	2

#define	si_screen		(HWroutines->si_screen)


/*
 * scanline routines
 */

#define	si_getscanline		(HWroutines->si_getsl)
#define	si_setscanline		(HWroutines->si_setsl)
#define	si_freescanline		(HWroutines->si_freesl)

/*
 * colormap routines
 */

#define	si_getcolormap		(HWroutines->si_get_colormap)
#define	si_setcolormap		(HWroutines->si_set_colormap)

/*
 * bitblt routine defines
 */

#define	SSBITBLT_AVAIL	1
#define	MSBITBLT_AVAIL	2
#define	SMBITBLT_AVAIL	4

#define si_hasssbitblt		((HWinfo->SIavail_bitblt) & SSBITBLT_AVAIL)
#define si_hasmsbitblt		((HWinfo->SIavail_bitblt) & MSBITBLT_AVAIL)
#define si_hassmbitblt		((HWinfo->SIavail_bitblt) & SMBITBLT_AVAIL)

#define si_SSbitblt		(HWroutines->si_ss_bitblt)
#define si_MSbitblt		(HWroutines->si_ms_bitblt)
#define si_SMbitblt		(HWroutines->si_sm_bitblt)

/*
 * stipple blt routine defines
 */

#define	SSSTPLBLT_AVAIL	1
#define	MSSTPLBLT_AVAIL	2
#define	SMSTPLBLT_AVAIL	4

#define si_hasssstplblt		((HWinfo->SIavail_stplblt) & SSSTPLBLT_AVAIL)
#define si_hasmsstplblt		((HWinfo->SIavail_stplblt) & MSSTPLBLT_AVAIL)
#define si_hassmstplblt		((HWinfo->SIavail_stplblt) & SMSTPLBLT_AVAIL)

#define si_SSstplblt		(HWroutines->si_ss_stplblt)
#define si_MSstplblt		(HWroutines->si_ms_stplblt)
#define si_SMstplblt		(HWroutines->si_sm_stplblt)

/*
 * filled polygon routines
 */

#define CONVEXPOLY_AVAIL	0x00000001
#define GENERALPOLY_AVAIL	0x00000002
#define RECTANGLE_AVAIL		0x00000004
#define POLYEVENODD_AVAIL	0x00000100
#define POLYWINDING_AVAIL	0x00000200

#define si_canpolyfill		((HWinfo->SIavail_fpoly) & \
		(CONVEXPOLY_AVAIL | GENERALPOLY_AVAIL | RECTANGLE_AVAIL) )

#define si_hasconvexfpolygon	((HWinfo->SIavail_fpoly) & CONVEXPOLY_AVAIL)
#define si_hasgeneralfpolygon	((HWinfo->SIavail_fpoly) & GENERALPOLY_AVAIL)
#define si_hasfillrectangle	((HWinfo->SIavail_fpoly) & RECTANGLE_AVAIL)

#define si_canevenoddfill	((HWinfo->SIavail_fpoly) & POLYEVENODD_AVAIL)
#define si_canwindingfill	((HWinfo->SIavail_fpoly) & POLYWINDING_AVAIL)

#define si_setpolyclip		(HWroutines->si_poly_clip)
#define si_fillconvexpoly	(HWroutines->si_poly_fconvex)
#define si_fillgeneralpoly	(HWroutines->si_poly_fgeneral)
#define si_fillrectangle	(HWroutines->si_poly_fillrect)

/*
 * point plot routines
 */

#define PLOTPOINT_AVAIL		0x00000001

#define si_hasplotpoint		((HWinfo->SIavail_point) & PLOTPOINT_AVAIL)

#define si_plotpoints		(HWroutines->si_plot_points)

/*
 * line drawing routines
 */

#define ONEBITLINE_AVAIL	0x00000001
#define ONEBITSEG_AVAIL		0x00000002
#define ONEBITRECT_AVAIL	0x00000004



#define si_canonebit		((HWinfo->SIavail_line) & \
		(ONEBITLINE_AVAIL | ONEBITSEG_AVAIL) )

#define si_haslinedraw		((HWinfo->SIavail_line) & ONEBITLINE_AVAIL)
#define si_haslineseg		((HWinfo->SIavail_line) & ONEBITSEG_AVAIL)
#define si_haslinerect		((HWinfo->SIavail_line) & ONEBITRECT_AVAIL)

#define si_setlineclip		(HWroutines->si_line_clip)
#define si_onebitlinedraw	(HWroutines->si_line_onebitline)
#define si_onebitlineseg	(HWroutines->si_line_onebitseg)
#define si_onebitlinerect	(HWroutines->si_line_onebitrect)

/*
 * arc drawing routines
 */
#define ONEBITARC_AVAIL		0x00000001
#define FILLARC_AVAIL		0x00000001


#define si_hasdrawarc		((HWinfo->SIavail_drawarc) & ONEBITARC_AVAIL)
#define si_hasfillarc		((HWinfo->SIavail_fillarc) & FILLARC_AVAIL)

#define si_setdrawarcclip	(HWroutines->si_drawarc_clip)
#define si_Drawarc		(HWroutines->si_drawarc)

#define si_setfillarcclip	(HWroutines->si_fillarc_clip)
#define si_Fillarc		(HWroutines->si_fillarc)

/*
 * hardware cursor routines
 */
#define CURSOR_TRUEHDWR	2	/* Hardware does it all */
#define CURSOR_FAKEHDWR	4	/* must turn on/off cursor before draw ops */

#define si_havetruecursor	((HWinfo->SIcursortype) & CURSOR_TRUEHDWR)
#define si_havefakecursor	((HWinfo->SIcursortype) & CURSOR_FAKEHDWR)

#define si_bestcursorwidth	(HWinfo->SIcurswidth)
#define si_bestcursorheight	(HWinfo->SIcursheight)
#define si_cursormask		(HWinfo->SIcursmask)

#define si_downloadcursor	(HWroutines->si_hcurs_download)
#define si_turnoncursor		(HWroutines->si_hcurs_turnon)
#define si_turnoffcursor	(HWroutines->si_hcurs_turnoff)
#define si_movecursor		(HWroutines->si_hcurs_move)

/*
 * tile/stipple widths and heights
 */
#define si_maxtilewidth		(HWinfo->SItilewidth)
#define si_maxtileheight	(HWinfo->SItileheight)

#define si_maxstipplewidth	(HWinfo->SIstipplewidth)
#define si_maxstippleheight	(HWinfo->SIstippleheight)


/*
 * hardware font downloading and drawing routines
 */

#define FONT_AVAIL		0x00000001

#define si_havedlfonts		((HWinfo->SIavail_font) & FONT_AVAIL)

#define si_checkfont		(HWroutines->si_font_check)
#define si_fontdownload		(HWroutines->si_font_download)
#define si_fontfree		(HWroutines->si_font_free)
#define si_fontclip		(HWroutines->si_font_clip)
#define si_fontstplblt		(HWroutines->si_font_stplblt)

/*
 * span filling routines
 */

#define SPANS_AVAIL		0x00000001

#define si_canspansfill		((HWinfo->SIavail_spans) & SPANS_AVAIL)
#define si_Fillspans		(HWroutines->si_fillspans)

/*
 * Color Specification Flags
 */
#define STATICGRAY_AVAIL	StaticGray
#define GRAYSCALE_AVAIL		GrayScale
#define STATICCOLOR_AVAIL	StaticColor
#define PSEUDOCOLOR_AVAIL	PseudoColor
#define TRUECOLOR_AVAIL		TrueColor
#define DIRECTCOLOR_AVAIL	DirectColor

/*
 * cache memory
 */
#define SHORTTERM_MEM		0
#define LONGTERM_MEM		1

#define si_hascachememory	(HWinfo->SIavail_memcache)

#define si_CacheAlloc(a,b) \
	if (HWinfo->SIavail_memcache) \
		(HWroutines->si_cache_alloc) (a, b)

#define si_CacheFree(a,b) \
	if ((a) != NullSIbitmap) { \
	    if ((b) == SU_SDDCACHE) \
		(HWroutines->si_cache_free)(a); \
	    else \
		if ((a)->Bptr != NULL) \
			xfree ((a)->Bptr); \
	    xfree (a); \
	}

/*
 * Extensions
 */
#define	PRIVATE_EXTENSION	1

/*
 * Defines common to many things
 */
#ifdef BEF_DT
#define si_hasanycliplist    ( \
			((HWinfo->SIavail_fpoly) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_line) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_fillarc) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_spans) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_font) & CLIPLIST_AVAIL) \
			    )
#else
#define si_hasanycliplist    ( \
			((HWinfo->SIavail_fpoly) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_line) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_fillarc) & CLIPLIST_AVAIL) || \
			((HWinfo->SIavail_font) & CLIPLIST_AVAIL) \
			    )
#endif
#endif	/* _SIDEP_H */
