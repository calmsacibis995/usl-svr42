/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bitfontlib:bitmap/snfstr.h	1.1"
/* $XConsortium: snfstr.h,v 1.3 91/07/17 22:12:31 keith Exp $ */

/***********************************************************
#include "libXi.h"
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef SNFSTR_H
#define SNFSTR_H 1

/*-
 * This file describes the Server Natural Font format.
 * SNF fonts are both CPU-dependent and frame buffer bit order dependent.
 * This file is used by:
 *	1)  the server, to hold font information read out of font files.
 *	2)  font converters
 *
 * Each font file contains the following
 * data structures, with no padding in-between.
 *
 *	1)  The XFONTINFO structure
 *		hand-padded to a two-short boundary.
 *		maxbounds.byteoffset is the total number of bytes in the
 *			glpyh array
 *		maxbounds.bitOffset is thetotal width of the unpadded font
 *
 *	2)  The XCHARINFO array
 *		indexed directly with character codes, both on disk
 *		and in memory.
 *
 *	3)  Character glyphs
 *		padded in the server-natural way, and
 *		ordered in the device-natural way.
 *		End of glyphs padded to 32-bit boundary.
 *
 *	4)  nProps font properties
 *
 *	5)  a sequence of null-terminated strings, for font properties
 */

#define FONT_FILE_VERSION	4

typedef struct _snfFontProp {
    CARD32      name;		/* offset of string */
    INT32       value;		/* number or offset of string */
    Bool        indirect;	/* value is a string offset */
}           snfFontPropRec;

/*
 * the following macro definitions describe a font file image in memory
 */
#define ADDRCharInfoRec( pfi)	\
	((snfCharInfoRec *) &(pfi)[1])

#define ADDRCHARGLYPHS( pfi)	\
	(((char *) &(pfi)[1]) + BYTESOFCHARINFO(pfi))

/*
 * pad out glyphs to a CARD32 boundary
 */
#define ADDRXFONTPROPS( pfi)  \
	((snfFontPropRec *) ((char *)ADDRCHARGLYPHS( pfi) + BYTESOFGLYPHINFO(pfi)))

#define ADDRSTRINGTAB( pfi)  \
	((char *)ADDRXFONTPROPS( pfi) + BYTESOFPROPINFO(pfi))

#define n2dChars(pfi)	(((pfi)->lastRow - (pfi)->firstRow + 1) * \
			 ((pfi)->lastCol - (pfi)->firstCol + 1))
#define	BYTESOFFONTINFO(pfi)	(sizeof(snfFontInfoRec))
#define BYTESOFCHARINFO(pfi)	(sizeof(snfCharInfoRec) * n2dChars(pfi))
#define	BYTESOFPROPINFO(pfi)	(sizeof(snfFontPropRec) * (pfi)->nProps)
#define	BYTESOFSTRINGINFO(pfi)	((pfi)->lenStrings)
#define	BYTESOFGLYPHINFO(pfi)	(((pfi)->maxbounds.byteOffset+3) & ~0x3)
#define BYTESOFINKINFO(pfi)	(sizeof(snfCharInfoRec) * n2dChars(pfi))

typedef struct _snfFontProp *snfFontPropPtr;
typedef struct _snfCharInfo *snfCharInfoPtr;
typedef struct _snfFontInfo *snfFontInfoPtr;

typedef struct _snfCharInfo {
    xCharInfo   metrics;	/* info preformatted for Queries */
    unsigned    byteOffset:24;	/* byte offset of the raster from pGlyphs */
    Bool        exists:1;	/* true iff glyph exists for this char */
    unsigned    pad:7;		/* must be zero for now */
}           snfCharInfoRec;

typedef struct _snfFontInfo {
    unsigned int version1;	/* version stamp */
    unsigned int allExist;
    unsigned int drawDirection;
    unsigned int noOverlap;	/* true if:
				 * max(rightSideBearing-characterWidth) <=
				 * minbounds->metrics.leftSideBearing */
    unsigned int constantMetrics;
    unsigned int terminalFont;	/* Should be deprecated!  true if: constant
				 * metrics && leftSideBearing == 0 &&
				 * rightSideBearing == characterWidth &&
				 * ascent == fontAscent && descent ==
				 * fontDescent */
    unsigned int linear:1;	/* true if firstRow == lastRow */
    unsigned int constantWidth:1;	/* true if
					 * minbounds->metrics.characterWidth
					 * ==
					 * maxbounds->metrics.characterWidth */
    unsigned int inkInside:1;	/* true if for all defined glyphs:
				 * leftSideBearing >= 0 && rightSideBearing <=
				 * characterWidth && -fontDescent <= ascent <=
				 * fontAscent && -fontAscent <= descent <=
				 * fontDescent */
    unsigned int inkMetrics:1;	/* ink metrics != bitmap metrics */
    /* used with terminalFont */
    /* see font's pInk{CI,Min,Max} */
    unsigned int padding:28;
    unsigned int firstCol;
    unsigned int lastCol;
    unsigned int firstRow;
    unsigned int lastRow;
    unsigned int nProps;
    unsigned int lenStrings;	/* length in bytes of string table */
    unsigned int chDefault;	/* default character */
    int         fontDescent;	/* minimum for quality typography */
    int         fontAscent;	/* minimum for quality typography */
    snfCharInfoRec minbounds;	/* MIN of glyph metrics over all chars */
    snfCharInfoRec maxbounds;	/* MAX of glyph metrics over all chars */
    unsigned int pixDepth;	/* intensity bits per pixel */
    unsigned int glyphSets;	/* number of sets of glyphs, for sub-pixel
				 * positioning */
    unsigned int version2;	/* version stamp double-check */
}           snfFontInfoRec;

#endif				/* SNFSTR_H */
