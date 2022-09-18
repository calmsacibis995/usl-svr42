/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bitfontlib:bitmap/snfread.c	1.23"

/************************************************************************

Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
#include "libXi.h"
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



************************************************************************/

/* $XConsortium: snfread.c,v 1.10 91/07/22 22:59:00 keith Exp $ */
#ifdef DEBUG
#include	<stdio.h>
#endif

#include <ctype.h>
#include "misc.h"
#include "fontfilest.h"
#include "bitmap.h"
#include "snfstr.h"
#include "Xwincache.h"
#include "dixfontstr.h"

extern FontRendererPtr renderer;
extern RendererConfigPtr rendererInfo;
extern long GetFontTime();

extern int InitPerFontCache();
extern void FreePerFontCache();
extern char* CacheGlyphs();
extern int siGetGlyphs();
int recreate_glyphs();
int snfFreeGlyphs();
int snfGetAllGlyphs();
static int hdr_bytestoskip = 0;

static void snfUnloadFont();
static int
snfReadCharInfo(file, charInfo, scaleptr, base, font)
    FontFilePtr file;
    CharInfoPtr charInfo;
    ScaleInfoPtr scaleptr;
    char       *base;
    FontPtr    font;
{
    snfCharInfoRec snfCharInfo;
    unsigned long starttime, endtime;
    static lastoffset = 0;
   	int wid,ht,pad; 
	int gWidth, gHeight, nbyGlyphWidth;

    if (!font->info.render_time) starttime = GetFontTime();
		/* get time to render at least one glyph */
    if (FontFileRead(file, (char *) &snfCharInfo, sizeof snfCharInfo) !=
	    sizeof(snfCharInfo)) {
	return BadFontName;
    }
	
    charInfo->metrics = snfCharInfo.metrics;
    
    charInfo->bits = 0;

    if (snfCharInfo.exists) {
        scaleptr->exists = TRUE;
	charInfo->bits = base + snfCharInfo.byteOffset;
        scaleptr->byteoffset = snfCharInfo.byteOffset;
        gHeight = GLYPHHEIGHTPIXELS(charInfo);
	nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(charInfo);

	scaleptr->glyphsize = (nbyGlyphWidth * gHeight);
    	if (base != NULL) {
		scaleptr->scaled = TRUE;
		scaleptr->allocated = TRUE;
		scaleptr->cached = TRUE;
    		}
	}
    if (!font->info.render_time) {
	endtime = GetFontTime();
	font->info.render_time = endtime -starttime;
	}
return Successful;
}

static int
snfReadxCharInfo(file, charInfo)
    FontFilePtr file;
    xCharInfo  *charInfo;
{
    snfCharInfoRec snfCharInfo;
    if (FontFileRead(file, (char *) &snfCharInfo, sizeof snfCharInfo) !=
	    sizeof(snfCharInfo)) {
	return BadFontName;
    }
    *charInfo = snfCharInfo.metrics;

    return Successful;
}

static
snfCopyInfo(snfInfo, pFontInfo)
    snfFontInfoPtr snfInfo;
    FontInfoPtr pFontInfo;
{
    pFontInfo->firstCol = snfInfo->firstCol;
    pFontInfo->lastCol = snfInfo->lastCol;
    pFontInfo->firstRow = snfInfo->firstRow;
    pFontInfo->lastRow = snfInfo->lastRow;
    pFontInfo->defaultCh = snfInfo->chDefault;
    pFontInfo->noOverlap = snfInfo->noOverlap;
    pFontInfo->terminalFont = snfInfo->terminalFont;
    pFontInfo->constantMetrics = snfInfo->constantMetrics;
    pFontInfo->constantWidth = snfInfo->constantWidth;
    pFontInfo->inkInside = snfInfo->inkInside;
    pFontInfo->inkMetrics = snfInfo->inkMetrics;
    pFontInfo->allExist = snfInfo->allExist;
    pFontInfo->drawDirection = snfInfo->drawDirection;
    pFontInfo->anamorphic = FALSE;
    pFontInfo->maxOverlap = 0;
    pFontInfo->minbounds = snfInfo->minbounds.metrics;
    pFontInfo->maxbounds = snfInfo->maxbounds.metrics;
    pFontInfo->fontAscent = snfInfo->fontAscent;
    pFontInfo->fontDescent = snfInfo->fontDescent;
    pFontInfo->nprops = snfInfo->nProps;
}

static int
snfReadProps(snfInfo, pFontInfo, file)
    snfFontInfoPtr snfInfo;
    FontInfoPtr pFontInfo;
    FontFilePtr file;
{
    char       *strings;
    FontPropPtr pfp;
    snfFontPropPtr psnfp;
    char       *propspace;
    int         bytestoalloc;
    int         i;

    bytestoalloc = snfInfo->nProps * sizeof(snfFontPropRec) +
	BYTESOFSTRINGINFO(snfInfo);
    propspace = (char *) xalloc(bytestoalloc);
    if (!propspace)
	return AllocError;

    if (FontFileRead(file, propspace, bytestoalloc) != bytestoalloc) {
	xfree(propspace);
	return BadFontName;
    }
    psnfp = (snfFontPropPtr) propspace;

    strings = propspace + BYTESOFPROPINFO(snfInfo);

    for (i = 0, pfp = pFontInfo->props; i < snfInfo->nProps; i++, pfp++, psnfp++) {
	pfp->name = MakeAtom(&strings[psnfp->name],
			     (unsigned) strlen(&strings[psnfp->name]), 1);
	pFontInfo->isStringProp[i] = psnfp->indirect;
	if (psnfp->indirect)
	    pfp->value = (INT32) MakeAtom(&strings[psnfp->value],
			       (unsigned) strlen(&strings[psnfp->value]), 1);
	else
	    pfp->value = psnfp->value;
    }

    xfree(propspace);
    return Successful;
}

int
snfReadHeader(snfInfo, file)
    snfFontInfoPtr snfInfo;
    FontFilePtr file;
{
    if (FontFileRead(file, (char *) snfInfo, sizeof *snfInfo) != sizeof *snfInfo)
	return BadFontName;

    if (snfInfo->version1 != FONT_FILE_VERSION ||
	    snfInfo->version2 != FONT_FILE_VERSION)
	return BadFontName;
    return Successful;
}

int
snfReadFont(pFont, file, filename,bit, byte, glyph, scan)
    FontPtr     pFont;
    FontFilePtr file;
    char        *filename;
    int         bit,
                byte,
                glyph,
                scan;
{
    snfFontInfoRec fi;
    unsigned    bytestoalloc;
    int         i,j;
    ScaleInfoPtr scaleptr;
    char       *fontspace;
    BitmapFontPtr  bitmapFont;
    int         num_chars;
    int         bitmapsSize;
    int         ret;
    int         metrics_off;
    int         bitmaps_off;
    int         encoding_off;
    int		scale_off;
    int         props_off;
    int         isStringProp_off;
    int         inksize;
    int		fname_off, fname_len;
    int		asize, acnt;
    char	*bitmaps, *repad_bitmaps;
    int		def_bit, def_byte, def_glyph, def_scan;
    int         result;
    int		nextoffset;

    ret = snfReadHeader(&fi, file);
    if (ret != Successful)
	return ret;

    SnfGetFormat (&def_bit, &def_byte, &def_glyph, &def_scan);

    /*
     * we'll allocate one chunk of memory and split it among the various parts
     * of the font:
     * 
     * BitmapFontRec CharInfoRec's Glyphs Encoding DIX Properties Ink CharInfoRec's
     *
     * If the glyphpad is not the same as the font file, then the glyphs
     * are allocated separately, to be later realloc'ed when we know
     * how big to make them.
     */

    bitmapsSize = BYTESOFGLYPHINFO(&fi);
    pFont->info.tot_glyphsize= bitmapsSize;
    pFont->info.glyphsizeSet = TRUE;
    pFont->info.render_time = 0;
    pFont->info.last_blittime = 0;

    renderer = pFont->rendererPtr;
    rendererInfo = &pFont->rendererPtr->config;
    pFont->info.preallocate = rendererInfo->preallocate_val;
    if (rendererInfo->alloc_units == 0)  pFont->info.alloc_units = 1;
	else pFont->info.alloc_units = rendererInfo->alloc_units;
    pFont->info.freeWhen = FONT_CAN_FREE;
    num_chars = n2dChars(&fi);
    bytestoalloc = sizeof(BitmapFontRec);	/* bitmapFont */
    fname_off =  bytestoalloc;
    fname_len = (strlen(filename)+1);
    bytestoalloc += fname_len;
    metrics_off = bytestoalloc;
    bytestoalloc += num_chars * sizeof(CharInfoRec);	/* metrics */
    scale_off = bytestoalloc;
    bytestoalloc += num_chars * sizeof(ScaleInfoRec);	/* scaled info */
    encoding_off = bytestoalloc;
    bytestoalloc += num_chars * sizeof(CharInfoPtr);	/* encoding */
    props_off = bytestoalloc;
    bytestoalloc += fi.nProps * sizeof(FontPropRec);	/* props */
    isStringProp_off = bytestoalloc;
    bytestoalloc += fi.nProps * sizeof(char);	/* isStringProp */
    bytestoalloc = (bytestoalloc + 3) & ~3;
#ifdef DEBUG
fprintf(stderr,"filename=%s\n",filename);
#endif
	
    fontspace = (char *) xalloc(bytestoalloc);
    if (!fontspace)
	return AllocError;
    memset(fontspace, 0, bytestoalloc); 
		/* must be zeroed out for cptr and scaleptr and filename */

    pFont->info.numChars = num_chars;
    bitmaps = 0;
    asize = acnt = 0;
    if ((rendererInfo->preallocate_val != 0) || 
	(rendererInfo->prerender == TRUE)) 
        getsize_to_preallocate(pFont,&asize, &acnt);
    result =InitPerFontCache(pFont, asize, acnt);
    if ((result != 1) && (rendererInfo->prerender == FALSE )) {
		fprintf(stderr,"Error allocating fontfile: %s\n", filename);
		xfree(fontspace);
		return AllocError;
	}
     if (result != 1) {
	if (InitPerFontCache(pFont, 0, 0) != 1) {
		fprintf(stderr,"Error allocating space for fontfile: %s\n", filename);
		xfree(fontspace);
		return AllocError;
		}
#ifdef DEBUG
	fprintf(stderr,"setting prerender to FALSE \n");
#endif
	rendererInfo->prerender = FALSE;
	rendererInfo->preallocate_val = 0;

	}
	
    if (rendererInfo->prerender == TRUE) {

	bitmaps = (char *) CacheGlyphs(pFont, asize, acnt, 0);
   	if (!bitmaps) {
	fprintf(stderr,"Error Allocating font: %s\n", filename);
	xfree (fontspace);
	FreePerFontCache(pFont); 
	return AllocError;
    	}
	
    }
    /*
     * now fix up pointers
     */

    bitmapFont = (BitmapFontPtr) fontspace;
    bitmapFont->num_chars = num_chars;
    bitmapFont->filename = (char *) (fontspace + fname_off);
    memcpy(bitmapFont->filename, filename, fname_len);
    bitmapFont->scaled = (ScaleInfoPtr) (fontspace + scale_off);
    bitmapFont->metrics = (CharInfoPtr) (fontspace + metrics_off);
    bitmapFont->encoding = (CharInfoPtr *) (fontspace + encoding_off);
    bitmapFont->bitmaps = bitmaps;
    bitmapFont->pDefault = NULL;
    bitmapFont->bitmapExtra = NULL;
    pFont->info.props = (FontPropPtr) (fontspace + props_off);
    pFont->info.isStringProp = (char *) (fontspace + isStringProp_off);

    if (fi.inkMetrics) {
#ifdef DEBUG
	fprintf(stderr,"font %s has ink metrics\n",filename);
#endif
	 inksize = num_chars * sizeof(xCharInfo);
	 bitmapFont->ink_metrics = (xCharInfo *) xalloc(inksize);
	 if (!bitmapFont->ink_metrics) {
		fprintf(stderr,"Error allocating ink metrics for font %s\n",filename);
   	 	xfree(fontspace);
		FreePerFontCache(pFont);
		return AllocError;
		}
	memset(bitmapFont->ink_metrics,0, inksize);
	}
    /*
     * read the CharInfo
     */

    ret = Successful;
    for (i = 0; ret == Successful && i < num_chars; i++) {
	ret = snfReadCharInfo(file, &bitmapFont->metrics[i], 
		&bitmapFont->scaled[i], bitmaps, pFont);
	if (bitmapFont->metrics[i].bits) {
	    bitmapFont->encoding[i] = &bitmapFont->metrics[i];
	}
	else
	    bitmapFont->encoding[i] = 0;
    }

    if (ret != Successful) {
	fprintf(stderr,"Error reading font %s charinfo\n",filename);
	xfree(fontspace);
	return ret;
    }



    /*
     * read the glyphs
     */

    if (bitmaps != NULL && rendererInfo->prerender == TRUE) {
    if (FontFileRead(file, (char *) bitmaps, bitmapsSize) != bitmapsSize) {
	fprintf(stderr,"Error reading font %s glypshs\n",filename);
        FreePerFontCache(pFont);
	xfree(fontspace);
	return BadFontName;
    }

    if (def_bit != bit)
	BitOrderInvert(bitmaps, bitmapsSize);
    if ((def_byte == def_bit) != (bit == byte)) {
	switch (bit == byte ? def_scan : scan) {
	case 1:
	    break;
	case 2:
	    TwoByteSwap(bitmaps, bitmapsSize);
	    break;
	case 4:
	    FourByteSwap(bitmaps, bitmapsSize);
	    break;
	}
    }
#ifndef USL
    if (def_glyph != glyph) {
	char	    *padbitmaps;
	int         sizepadbitmaps;
	int	    sizechar;
	CharInfoPtr metric;

	sizepadbitmaps = 0;
	metric = bitmapFont->metrics;
	for (i = 0; i < num_chars; i++)
	{
	    if (metric->bits)
		sizepadbitmaps += BYTES_FOR_GLYPH(metric,glyph);
	    metric++;
	}
	padbitmaps = (char *) xalloc(sizepadbitmaps);
	if (!padbitmaps) {
	    xfree (bitmaps);
	    xfree (fontspace);
	    return AllocError;
	}
	metric = bitmapFont->metrics;
	bitmapFont->bitmaps = padbitmaps;
	for (i = 0; i < num_chars; i++) {
	    sizechar = RepadBitmap(metric->bits, padbitmaps,
			       def_glyph, glyph,
			       metric->metrics.rightSideBearing -
			       metric->metrics.leftSideBearing,
			       metric->metrics.ascent + metric->metrics.descent);
	    metric->bits = padbitmaps;
	    padbitmaps += sizechar;
	    metric++;
	}
	xfree(bitmaps);
    }
#endif 

}  else
	{
	FontFileSkip(file, bitmapsSize);
	}
    ret = snfReadProps(&fi, &pFont->info, file);
    if (ret != Successful) {
	fprintf(stderr,"Error reading font %s properties\n",filename);
        FreePerFontCache(pFont);
	xfree(fontspace);
	return ret;
    }
    snfCopyInfo(&fi, &pFont->info);

    /* finally, read the ink metrics if the exist */

    if (fi.inkMetrics) {
	ret = Successful;
	ret = snfReadxCharInfo(file, &pFont->info.ink_minbounds);
	ret = snfReadxCharInfo(file, &pFont->info.ink_maxbounds);
	for (i = 0; ret == Successful && i < num_chars; i++) 
	    ret = snfReadxCharInfo(file, &bitmapFont->ink_metrics[i]);
	if (ret != Successful) {
	    fprintf(stderr,"Error Reading %s char ink metrics\n",filename);
            FreePerFontCache(pFont);
	    xfree(fontspace);
	    return ret;
	}
    } else {
	pFont->info.ink_minbounds = pFont->info.minbounds;
	pFont->info.ink_maxbounds = pFont->info.maxbounds;
    }

    if (pFont->info.defaultCh != (unsigned short) NO_SUCH_CHAR) {
	int         r,
	            c,
	            cols;

	int i;
	r = pFont->info.defaultCh >> 8;
	c = pFont->info.defaultCh & 0xFF;
	if (pFont->info.firstRow <= (unsigned int )r && r <= (unsigned )pFont->info.lastRow &&
		pFont->info.firstCol <= (unsigned int) c && c <= (unsigned) pFont->info.lastCol) {
	    cols = pFont->info.lastCol - pFont->info.firstCol + 1;
	    r = r - pFont->info.firstRow;
	    c = c - pFont->info.firstCol;
	    i = r*cols+ c;
	    bitmapFont->pDefault = &bitmapFont->metrics[r * cols + c];
#ifdef DEBUG
fprintf(stderr,"bitmapFont->pDefault=%d i=%d\n",r*cols+c, i);
#endif
	}
    }
    bitmapFont->bitmapExtra = (BitmapExtraPtr) 0;
    pFont->fontPrivate = (pointer) bitmapFont;
    pFont->get_bitmaps = 0;
    pFont->get_extents = 0;
#ifndef USL
    pFont->get_bitmaps = bitmapGetBitmaps;
    pFont->get_extents = bitmapGetExtents;
#endif
    pFont->svrPrivate = 0;
    pFont->free_metrics =  0;
    pFont->get_all_siglyphs = snfGetAllGlyphs;
    pFont->get_glyphs = bitmapGetGlyphs;
    pFont->free_glyphs = snfFreeGlyphs;
    pFont->get_metrics = bitmapGetMetrics;
    pFont->unload_font = snfUnloadFont;
    pFont->bit = bit;
    pFont->byteorder = byte;
    pFont->glyph = glyph;
    pFont->scan = scan;
    renderer->fonts_open++;
    return Successful;
}

FILE *
reopen_fontfile(pFont)
FontPtr pFont;
{
BitmapFontPtr bitmapFont;
FILE *file;
    int bytestoskip = 0;
    snfCharInfoRec snfCharInfo;
    snfFontInfoPtr snfInfo;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
	/* need to open the file, init the cache again and
		read the glyphs back in  */
    file = fopen(bitmapFont->filename, "r");
    if (!file) return file;
		/* position file at start of glyphs */
    hdr_bytestoskip = sizeof *snfInfo;
    bytestoskip = (sizeof (snfCharInfo))*bitmapFont->num_chars;
    hdr_bytestoskip += bytestoskip; 
    return(file);
}

int 
recreate_glyphs(file, pFont, chars, count, charencoding)
FILE *file;
FontPtr pFont;
register unsigned char *chars;
unsigned long count;
FontEncoding charencoding;
{
register unsigned int c;
unsigned int min;
BitmapFontPtr bitmapFont;
char *ptr;
unsigned int r, c1, numCols,numRows;
unsigned int firstRow, firstCol;
int i,j;
ScaleInfoPtr scaleptr;
CharInfoPtr cptr;
snfFontInfoPtr snfInfo;
unsigned int cdef;
int bytestoskip = 0;
    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
	/* need to open the file, init the cache again and
		read the glyphs back in  */
firstRow =  pFont->info.firstRow;
firstCol = pFont->info.firstCol;
numRows = pFont->info.lastRow - firstRow + 1;

#ifdef DEBUG1
	fprintf(stderr,"recreate_glyphs font=%s\n",bitmapFont->filename);
#endif
    min = pFont->info.firstCol;
    cdef = pFont->info.defaultCh  - min;

    numCols = pFont->info.lastCol - min + 1;

#ifdef DEBUG1
fprintf(stderr,"numCols=%d charencoding=%d cdef=%d\n",numCols,charencoding,cdef);
#endif
    for (i=0; i < count; i++) {
    if (charencoding == Linear8Bit || charencoding == TwoD8Bit) {
	c = (*chars++ - min);
	if (c >= numCols) c = cdef;
 	} else 
    if (charencoding == Linear16Bit) {
	c = *chars++ << 8;
	c = (c | *chars++) - min;
	if (c >= numCols) c = cdef;
    } else {
	r = (*chars++) - firstRow;
	c1 = (*chars++)  - firstCol;
	if ((r < numRows) && (c1 < numCols)) {
		c = r * numCols + c1;
	}
	else {
		r = (pFont->info.defaultCh >> 8) - firstRow;
		c1 = (pFont->info.defaultCh & 0xff) -firstCol;
		if ((r < numRows) && (c1 < numCols)) {
			c = r*numCols + c1;
			}
		}
	}

    if ( c >= pFont->info.numChars) continue;

    scaleptr = &bitmapFont->scaled[c];
    if (scaleptr->exists == FALSE) {
	c = cdef;
  	if ( c >= pFont->info.numChars) continue;
	scaleptr = &bitmapFont->scaled[c];
	}
    if (scaleptr->scaled == TRUE) continue;
    if (scaleptr->exists == FALSE) continue; 

    cptr = (CharInfoPtr) &bitmapFont->metrics[c]; 

		/* position file at start of glyphs */
    bytestoskip = hdr_bytestoskip + scaleptr->byteoffset;
    FontFileSeek(file, bytestoskip);
    ptr = (char *) CacheGlyphs(pFont, scaleptr->glyphsize, 1, 1);
    if (!ptr) {
		perror("snfread: cacheglyph fail ");
		return AllocError;
		}
    /*
     * read the glyphs
     */

    if (FontFileRead(file, (char *)ptr, scaleptr->glyphsize) != scaleptr->glyphsize) {
	perror("snfread: read of glyphs fail: ");
        return -1;
    }

	cptr->bits = ptr;
	bitmapFont->encoding[c] = &bitmapFont->metrics[c];
	scaleptr->scaled = TRUE;
	scaleptr->allocated = TRUE;
	scaleptr->cached= TRUE;

 
   }
return 1;
}


int
snfGetAllGlyphs(pFont, glyphs)
FontPtr pFont;
CharInfoPtr *glyphs;

{
   FILE *file;
   ScaleInfoPtr scaleptr;
   CharInfoPtr cptr;
   BitmapFontPtr   bitmapFont;
   int result;
   register int i;
   char *ptr;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
	
   if ((rendererInfo->prerender == TRUE) && (pFont->info.rerender != TRUE)) {
       for (i=0; i < bitmapFont->num_chars; i++,glyphs++) 
       	        *glyphs = bitmapFont->encoding[i];
	pFont->info.preallocate = 100;
	pFont->info.downloaded = TRUE;
	pFont->info.freeWhen = FONT_DONT_FREE;
        return Successful;
    }

    file = reopen_fontfile(pFont);
    if (!file) return -1;
	for (i=0; i < bitmapFont->num_chars; i++) {
        scaleptr = &bitmapFont->scaled[i];
        cptr= &bitmapFont->metrics[i];
         if (scaleptr->exists == TRUE && scaleptr->allocated== FALSE && scaleptr->glyphsize > 0) {
		ptr = (char *) CacheGlyphs(pFont, scaleptr->glyphsize,1,1);
		if (!ptr) {
			fclose(file);
			return AllocError;
			}
		cptr->bits = ptr;
		scaleptr->allocated = TRUE;
		scaleptr->cached = TRUE;
	}
	if (scaleptr->scaled == FALSE) {
		FontFileSeek(file, hdr_bytestoskip + scaleptr->byteoffset);
		result= FontFileRead(file, cptr->bits, scaleptr->glyphsize);
			if (result != scaleptr->glyphsize) {
			fclose(file);
			return AllocError; 
		}
		/* must allocated and scale the chars */
		scaleptr->scaled = TRUE;
	}
	bitmapFont->encoding[i] = &bitmapFont->metrics[i];
	*glyphs = bitmapFont->encoding[i];
	glyphs++;
	
    }
pFont->info.rerender = FALSE;
pFont->info.preallocate = 100;
pFont->info.downloaded = TRUE;
pFont->info.freeWhen = FONT_DONT_FREE;
fclose (file);
return(Successful);
}


int
snfFreeGlyphs(pFont)
FontPtr pFont;
{

   register int i;
   ScaleInfoPtr scaleptr;
   BitmapFontPtr   bitmapFont;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
#ifdef DEBUG
fprintf(stderr,"snfFreeGlyphs file=%s\n",bitmapFont->filename);
#endif

	/* the cache did the freeing if we are using the 
	glyph cache, so just mark all the pointers
		to null */
    for (i=0; i < pFont->info.numChars; i++) {
                scaleptr = &bitmapFont->scaled[i];
                scaleptr->allocated = FALSE;
                scaleptr->scaled = FALSE;
                scaleptr->cached = FALSE;
		bitmapFont->metrics[i].bits = 0;
	/* clear all the glyph pointers */
	}
    bitmapFont->bitmaps = 0;
    pFont->info.freeWhen = FONT_CAN_FREE;
	/* reset since we know we can free it if it gets recreated
		and flag might have been changed by cache */	
return(Successful);
}


int
snfReadFontInfo(pFontInfo, file)
    FontInfoPtr pFontInfo;
    FontFilePtr file;
{
    int         ret;
    snfFontInfoRec fi;
    int         bytestoskip;
    int         num_chars;


    ret = snfReadHeader(&fi, file);
    if (ret != Successful)
	return ret;
    snfCopyInfo(&fi, pFontInfo);

    pFontInfo->props = (FontPropPtr) xalloc(fi.nProps * sizeof(FontPropRec));
    if (!pFontInfo->props)
	return AllocError;
    pFontInfo->isStringProp = (char *) xalloc(fi.nProps * sizeof(char));
    if (!pFontInfo->isStringProp) {
	xfree(pFontInfo->props);
	return AllocError;
    }
    num_chars = n2dChars(&fi);
    bytestoskip = num_chars * sizeof(snfCharInfoRec);	/* charinfos */
    bytestoskip += BYTESOFGLYPHINFO(&fi);
    FontFileSkip(file, bytestoskip);

    ret = snfReadProps(&fi, pFontInfo, file);
    if (ret != Successful) {
	xfree(pFontInfo->props);
	xfree(pFontInfo->isStringProp);
	return ret;
    }
    if (fi.inkMetrics) {
	ret = snfReadxCharInfo(file, &pFontInfo->ink_minbounds);
	if (ret != Successful) {
	    xfree(pFontInfo->props);
	    xfree(pFontInfo->isStringProp);
	    return ret;
	}
	ret = snfReadxCharInfo(file, &pFontInfo->ink_maxbounds);
	if (ret != Successful) {
	    xfree(pFontInfo->props);
	    xfree(pFontInfo->isStringProp);
	    return ret;
	}
	} else {
	    pFontInfo->ink_minbounds = pFontInfo->minbounds;
	    pFontInfo->ink_maxbounds = pFontInfo->maxbounds;
    }
    return Successful;

}

static void
snfUnloadFont(pFont)
    FontPtr	    pFont;
{
    BitmapFontPtr   bitmapFont;

    bitmapFont = (BitmapFontPtr) pFont->fontPrivate;
#ifdef DEBUG
	fprintf(stderr,"snfUnloadFont %s\n", bitmapFont->filename);
#endif
       FreePerFontCache(pFont);
       xfree (bitmapFont);
       xfree (pFont);
       renderer->fonts_open--;
}

static int  snf_set;
static int  snf_bit, snf_byte, snf_glyph, snf_scan;

SnfSetFormat (bit, byte, glyph, scan)
    int	bit, byte, glyph, scan;
{
    snf_bit = bit;
    snf_byte = byte;
    snf_glyph = glyph;
    snf_scan = scan;
    snf_set = 1;
}

SnfGetFormat (bit, byte, glyph, scan)
    int	*bit, *byte, *glyph, *scan;
{
    if (!snf_set)
	FontDefaultFormat (&snf_bit, &snf_byte, &snf_glyph, &snf_scan);
    *bit = snf_bit;
    *byte = snf_byte;
    *glyph = snf_glyph;
    *scan = snf_scan;
}


int
getsize_to_preallocate(font,asize, acnt)
FontPtr font;
int *asize;
int *acnt;
{
    *asize = 0;
    *acnt = 0;

    if (rendererInfo->prerender == TRUE) 
		rendererInfo->preallocate_val = 100;

	/* setting prerender glyphs to TRUE implies preallocate
		as well */

    if (rendererInfo->preallocate_val == 100)  {
		*asize = font->info.tot_glyphsize;
		*acnt = font->info.numChars;

    } else 
    if (rendererInfo->preallocate_val > 0 && rendererInfo->preallocate_val < 100){
		*asize = font->info.tot_glyphsize * rendererInfo->preallocate_val;
		*acnt = font->info.numChars * rendererInfo->preallocate_val;
	}
	
}
