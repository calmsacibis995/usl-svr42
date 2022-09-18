/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:include/fontstruct.h	1.10"
/*copyright     "%c%"*/

/* $Header: fontstruct.h,v 1.10 91/07/22 15:37:41 keith Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



	@(#)fontstruct.h	3.2	91/04/15

******************************************************************/

#ifndef FONTSTR_H
#define FONTSTR_H

#include <X11/Xproto.h>
#include "font.h"
#include "Xrenderer.h"
#include "Xwincache.h"
#include "fontxlfd.h"
/*
 * This version of the server font data strucutre is only for describing
 * the in memory data structure. The file structure is not necessarily a
 * copy of this. That is up to the compiler and the OS layer font loading
 * machinery.
 */

#define 	DEFAULT_MAXCACHE	819200
#define		DEFAULT_MINCACHE	737280	
#define		MIN_CACHESIZE		256000
#define		LOWWATER_PERCENT	.90
#define 	FONT_DONT_FREE  	1
#define 	FONT_CAN_FREE  		2
#define 	FONT_FREE_NEXT 		3

#ifndef 	MAXSCREENS
#define  	MAXSISCREENS 3
#else
#define 	MAXSISCREENS MAXSCREENS
#endif

#define GLYPHPADOPTIONS 4	/* 1, 2, 4, or 8 */

typedef enum {
    Linear8Bit, TwoD8Bit, Linear16Bit, TwoD16Bit
}           FontEncoding;

typedef struct _FontProp {
    long        name;
    long        value;		/* assumes ATOM is not larger than INT32 */
}           FontPropRec;

typedef struct _FontResolution {
    unsigned short x_resolution;
    unsigned short y_resolution;
    unsigned short point_size;
}           FontResolutionRec;

typedef struct _ExtentInfo {
    DrawDirection drawDirection;
    int         fontAscent;
    int         fontDescent;
    int         overallAscent;
    int         overallDescent;
    int         overallWidth;
    int         overallLeft;
    int         overallRight;
}           ExtentInfoRec;

typedef struct _CharInfo {
    xCharInfo   metrics;	/* info preformatted for Queries */
    char       *bits;		/* pointer to glyph image */
}           CharInfoRec;

/*
 * Font is created at font load time. It is specific to a single encoding.
 * e.g. not all of the glyphs in a font may be part of a single encoding.
 */

typedef struct _FontInfo {
    unsigned short firstCol;
    unsigned short lastCol;
    unsigned short firstRow;
    unsigned short lastRow;
    unsigned short defaultCh;
    unsigned int noOverlap:1;
    unsigned int terminalFont:1;
    unsigned int constantMetrics:1;
    unsigned int constantWidth:1;
    unsigned int inkInside:1;
    unsigned int inkMetrics:1;
    unsigned int allExist:1;
    unsigned int drawDirection:2;
    unsigned int cachable:1;
    unsigned int anamorphic:1;
	/* below added by USL */
    unsigned int downloadable:1;
    unsigned int downloaded:1;
    unsigned int usingGlyphCache:1;
    unsigned int freeWhen:2;
    unsigned int rerender:1;
    unsigned int reallocate:1;
    unsigned int glyphsizeSet:1;
    unsigned int tot_glyphsize;
    unsigned long last_blittime;
    unsigned long render_time;
    short       preallocate;
    short       alloc_units;
    short       freed;
    short       numChars;
	/* above added by USL */
    short       maxOverlap;
    short       pad;
    xCharInfo   maxbounds;
    xCharInfo   minbounds;
    xCharInfo   ink_maxbounds;
    xCharInfo   ink_minbounds;
    short       fontAscent;
    short       fontDescent;
    int         nprops;
    FontPropPtr props;
    char       *isStringProp;
}           FontInfoRec;




typedef struct _Font {
    FontRendererPtr rendererPtr;
    PerFontCachePtr cachestats;
    int         refcnt;
    FontInfoRec info;
    char        bit;
    char        byteorder;
    char        glyph;
    char        scan;
    fsBitmapFormat format;
    int         (*get_glyphs) ( /* font, count, chars, encoding, count, glyphs */ );
    int         (*get_metrics) ( /* font, count, chars, encoding, count, glyphs */ );
    int         (*get_bitmaps) (/* client, font, flags, ranges, nranges,
				    nextents, extents */ );
    int         (*get_extents) (/* client, font, format, flags, ranges,
				    nranges, nglyphs, offsets, glyphs */ );
    void        (*unload_font) ( /* font */ );
    void        (*free_metrics) (/* font, count, glyphs)*/); 
    int         (*free_glyphs) (/* font, count, glyphs)*/); 
    int         (*get_all_siglyphs) (/* font, count, glyphs)*/); 
    FontPathElementPtr fpe;
    pointer     svrPrivate;
    pointer     fontPrivate;
    pointer     fpePrivate;
    short	maxsiPrivate;
    short	maxPrivate;
    pointer	siPrivates[MAXSISCREENS];
    pointer	*devPrivates;
}           FontRec;

extern Bool	_FontSetNewPrivate (/* pFont, n, ptr */);
extern int	AllocateFontPrivateIndex ();

#define FontGetPrivate(pFont,n) ((n) > (pFont)->maxPrivate ? (pointer) 0 : \
			     (pFont)->devPrivates[n])

#define FontSetPrivate(pFont,n,ptr) ((n) > (pFont)->maxPrivate ? \
			_FontSetNewPrivate (pFont, n, ptr) : \
			((((pFont)->devPrivates[n] = (ptr)) != 0) || TRUE))

typedef struct _FontNames {
    int         nnames;
    int         size;
    int        *length;
    char      **names;
}           FontNamesRec;

/* External view of font paths */
typedef struct _FontPathElement {
    int         name_length;
    char       *name;
    int         type;
    int         refcount;
    pointer     private;
}           FontPathElementRec;

typedef struct _FPEFunctions {
    int         (*name_check) ( /* name */ );
    int         (*init_fpe) ( /* fpe */ );
    int         (*reset_fpe) ( /* fpe */ );
    int         (*free_fpe) ( /* fpe */ );
    int         (*open_font) (	/* client, fpe, flags, name, namelen, format,
			          fid,  ppfont, alias */ );
    int         (*close_font) ( /* pfont */ );
    int         (*list_fonts) (	/* client, fpe, pattern, patlen, maxnames,
			           paths */ );
    int         (*start_list_fonts_with_info) (	/* client, fpe, name, namelen,
					           maxnames, data */ );
    int         (*list_next_font_with_info) (	/* client, fpe, name, namelen,
					         info, num, data */ );
    int         (*wakeup_fpe) ( /* fpe, mask */ );
    int		(*client_died) ( /* client, fpe */ );
}           FPEFunctionsRec, FPEFunctions;

extern int  InitFPETypes();

/*
 * Various macros for computing values based on contents of
 * the above structures
 */

#define	GLYPHWIDTHPIXELS(pci) \
	((pci)->metrics.rightSideBearing - (pci)->metrics.leftSideBearing)

#define	GLYPHHEIGHTPIXELS(pci) \
 	((pci)->metrics.ascent + (pci)->metrics.descent)

#define	GLYPHWIDTHBYTES(pci)	(((GLYPHWIDTHPIXELS(pci))+7) >> 3)

#define GLYPHWIDTHPADDED(bc)	(((bc)+7) & ~0x7)

#define BYTES_PER_ROW(bits, nbytes) \
	((nbytes) == 1 ? (((bits)+7)>>3)	/* pad to 1 byte */ \
	:(nbytes) == 2 ? ((((bits)+15)>>3)&~1)	/* pad to 2 bytes */ \
	:(nbytes) == 4 ? ((((bits)+31)>>3)&~3)	/* pad to 4 bytes */ \
	:(nbytes) == 8 ? ((((bits)+63)>>3)&~7)	/* pad to 8 bytes */ \
	: 0)

#define BYTES_FOR_GLYPH(ci,pad)	(GLYPHHEIGHTPIXELS(ci) * \
				 BYTES_PER_ROW(GLYPHWIDTHPIXELS(ci),pad))
/*
 * Macros for computing different bounding boxes for fonts; from
 * the font protocol
 */

#define FONT_MAX_ASCENT(pi)	((pi)->fontAscent > (pi)->ink_maxbounds.ascent ? \
			    (pi)->fontAscent : (pi)->ink_maxbounds.ascent)
#define FONT_MAX_DESCENT(pi)	((pi)->fontDescent > (pi)->ink_maxbounds.descent ? \
			    (pi)->fontDescent : (pi)->ink_maxbounds.descent)
#define FONT_MAX_HEIGHT(pi)	(FONT_MAX_ASCENT(pi) + FONT_MAX_DESCENT(pi))
#define FONT_MIN_LEFT(pi)	((pi)->ink_minbounds.leftSideBearing < 0 ? \
			    (pi)->ink_minbounds.leftSideBearing : 0)
#define FONT_MAX_RIGHT(pi)	((pi)->ink_maxbounds.rightSideBearing > \
				(pi)->ink_maxbounds.characterWidth ? \
			    (pi)->ink_maxbounds.rightSideBearing : \
				(pi)->ink_maxbounds.characterWidth)
#define FONT_MAX_WIDTH(pi)	(FONT_MAX_RIGHT(pi) - FONT_MIN_LEFT(pi))

#endif				/* FONTSTR_H */
