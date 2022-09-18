#ident	"@(#)r5fontinc:lib/include/bitmap.h	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*

 * $XConsortium: bitmap.h,v 1.1 91/05/11 09:11:56 rws Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include    <stdio.h>

/*
 * Internal format used to store bitmap fonts
 */

typedef struct _BitmapExtra {
    Atom       *glyphNames;
    int        *sWidths;
    CARD32      bitmapsSizes[GLYPHPADOPTIONS];
    FontInfoRec info;
}           BitmapExtraRec, *BitmapExtraPtr;


typedef struct _ScaleInfo {
	unsigned short exists:1;	
	unsigned short scaled:1;
	unsigned short allocated:1;		
	unsigned short cached:1;	
        short glyphsize;
        int byteoffset;
} ScaleInfoRec, *ScaleInfoPtr;

typedef struct _BitmapFont {
    unsigned    version_num;
    int         num_chars;
    int         num_tables;
    CharInfoPtr metrics;	/* font metrics, including glyph pointers */
    xCharInfo  *ink_metrics;	/* ink metrics */
    char       *bitmaps;	/* base of bitmaps, useful only to free */
    char       *filename;	/* filename, needed to recreate if glyphs are
					freed by cache */
    ScaleInfoPtr scaled;	/* scaling information */
    CharInfoPtr *encoding;	/* array of char info pointers */
    CharInfoPtr pDefault;	/* default character */
    BitmapExtraPtr bitmapExtra;	/* stuff not used by X server */
}           BitmapFontRec, *BitmapFontPtr;

extern int  bitmapReadFont(), bitmapReadFontInfo();
extern int  bitmapGetGlyphs(), bitmapGetMetrics();
extern int  bitmapGetBitmaps(), bitmapGetExtents();
extern void bitmapUnloadFont();

extern void bitmapComputeFontBounds();
extern void bitmapComputeFontInkBounds();

typedef FILE	*FontFilePtr;

#define FontFileGetc(f)	    getc(f)
#define FontFilePutc(c,f)   putc(c,f)
#define FontFileRead(f,b,n) fread((char *) b, 1, n, f)
#define FontFileWrite(f,b,n)	fwrite ((char *) b, 1, n, f)
#define FontFileSkip(f,n)   (fseek(f,n,1) != -1)
#define FontFileSeek(f,n)   (fseek(f,n,0) != -1)

#define FontFileEOF	EOF

#endif				/* _BITMAP_H_ */
