/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:folio/foliofont.c	1.9"
/*copyright	"%c%*/
/*    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/

/************************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

************************************************************************/

#define NEED_REPLIES
#include	<stdio.h>
#include	<ctype.h>
#include <os.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>

#include <f3font.h> 
#include <font.h>
#include <fontstruct.h>
#include <Xrenderer.h>
#include <fontfilest.h>
#include <dixfontstr.h>
#include <fontmisc.h>
#include <resource.h>
#include <osstruct.h>
#include <sh_fract.h>
#include <sys/param.h>
extern void CopyISOLatin1Lowered();
extern unsigned char	*matchfontname();
RendererConfigPtr f3rendererInfo;

FontScalableRec vals;
int points, pixels;
int resx, resy;
int transX, transY; 
#define ROUNDTOINT(x)   ((int) (((x) > 0.0) ? ((x) + 0.5) : ((x) - 0.5)))

int f3GetGlyphs();
int f3GetMetrics();
int f3FreeGlyphs();
void f3UnloadFont();
int f3GetAllGlyphs();
void folio_get_chars();

/*
 * load font
 */
int
FolioOpenFont(ppfont, fontname, filename, entry, format,fmask, flags)
    FontPtr *ppfont;
    char *fontname;
    char *filename;
    FontEntryPtr entry;
    fsBitmapFormat format;
    fsBitmapFormatMask fmask;
    Mask flags;

{

    FontPtr pfont;
    f3FontPtr f3font;
    int ret;

	pfont = (FontPtr) xalloc (sizeof (FontRec));
	if (!pfont) return AllocError;
	memset(pfont, 0, sizeof(FontRec));

	pfont->rendererPtr = entry->u.scalable.renderer;
	entry->u.scalable.extra->private = (pointer) pfont;


    ret = load_f3_font(fontname,filename, entry,format,fmask,pfont,flags);
#ifdef DEBUGFPOLIO
fprintf(stderr,"return: maxbounds: ascent=%d descent=%d\n",pfont->info.maxbounds.ascent,pfont->info.maxbounds.descent);
#endif

    if (ret == Successful) 
	*ppfont = pfont;
    else
	xfree(pfont);

    return ret;

}

int 
load_f3_font(fontname, filename, entry, format, fmask, font, flags)
    char *fontname;
    char *filename;
    FontEntryPtr entry;
    fsBitmapFormatMask fmask;
    FontPtr font;
    Mask flags;
{


    f3FontPtr f3font;

    int nchars, minglyph, defaultchar;
    int result, asize, acnt, bytestoalloc;
    int scale_len;
    int f3len, namelen, ret;
    char *fontspace;

    namelen = strlen(filename)+1;

    ret = open_f3_font(fontname, filename, entry,format,fmask,flags);
    if (ret != Successful) return ret; 
    folio_get_chars(&nchars, &minglyph, &defaultchar);
	/* not sure if FontRec should be part of this allocation or
		separate */
    f3len = sizeof(f3Font);
    scale_len = nchars * sizeof(f3ScaleInfo);
#ifdef DEBUG
	fprintf(stderr,"scalelen=%d nchars=%d f3ScaleInfo size=%d\n",scale_len,nchars, sizeof(f3ScaleInfo));
#endif
    bytestoalloc = namelen + f3len + scale_len;
    fontspace = (char *) xalloc(bytestoalloc);
    if (!fontspace) return AllocError;
    memset(fontspace, 0, bytestoalloc);
    f3font = (f3FontPtr) fontspace;
    f3font->folioname = (char *) (fontspace + f3len);
    memcpy(f3font->folioname,filename,namelen);
#ifdef DEBUG
fprintf(stderr,"folioname=%s\n",f3font->folioname);
#endif
     f3font->scaled = (f3ScaleInfoPtr) (fontspace + f3len + namelen);

		/* name is after f3font structure in memory and
			scaled follows filename */

    f3font->pDefault = &f3font->scaled[defaultchar - minglyph].charMetrics;
    f3font->fontPtr = (FontPtr) font;
    f3font->nglyphs = nchars;
    f3font->minglyph = minglyph;
    f3font->defaultchar = defaultchar;
    f3font->ptsize = points;
    f3font->vals.point = points;
    f3font->pixelsize = pixels;
    f3font->vals.pixel = pixels;
    f3font->transX = transX;
    f3font->transY = transY;
    font->cachestats = 0;
    f3bGetFormat(font);
    f3font->allexist = TRUE; /* override later if necessary */
    f3font->vals.x = resx;
    f3font->vals.y = resy;
    f3font->vals.width = 0;
#ifdef DEBUG
    fprintf(stderr," transX=%d transy=%d\n",transX,transY);
#endif

    folio_initialize_font(f3font);
    font->info.glyphsizeSet = TRUE;
    font->info.allExist = f3font->allexist;


    folio_fontheader(f3font,&font->info);
    folio_compute_bounds(f3font, &font->info, TRUE);
    folio_compute_props(f3font, fontname, &font->info);

    font->info.tot_glyphsize = f3font->gsize;
#ifdef XXX

    f3rendererInfo->prealloc_val = 0; 
    f3rendererInfo->prerender = FALSE;
		/* above 2 lines temporary  until bug fixed */
#endif
    
    if (f3rendererInfo->alloc_units == 0)  font->info.alloc_units = 1;
	else font->info.alloc_units = f3rendererInfo->alloc_units;
    font->info.glyphsizeSet = TRUE;
    font->fontPrivate = (pointer) f3font;
    asize = acnt = 0;
    if ((f3rendererInfo->preallocate_val != 0) || 
	(f3rendererInfo->prerender == TRUE)) 
#ifdef DEBUG
	fprintf(stderr,"check preallocate size\n");
#endif
        getsize_to_preallocate(f3font,&asize, &acnt);
#ifdef DEBUG
	fprintf(stderr,"size to allocate=%ld count=%ld\n",asize,acnt);
#endif

    if (font->rendererPtr->caller == XSERVER_TYPE) {
    result=InitPerFontCache(font, asize, acnt);
	if (result != 1) {
#ifdef DEBUG
	fprintf(stderr,"result=%d\n", result);
#endif
		xfree(f3font);
		return AllocError;
    	}
   }

    if (f3rendererInfo->prerender == TRUE) {
	result = prerender_glyphs(font);
        if (result != Successful)  {
		xfree(f3font);
		return result;
	}
    }	

	/* compute remaining accelerators */
    FolioComputeInfoAccelerators(&font->info);

    font->format = format;
    font->fpePrivate = 0;
    font->get_bitmaps = 0;
    font->get_extents = 0;
    font->get_metrics = f3GetMetrics;
    font->get_glyphs = f3GetGlyphs;
    font->unload_font = f3UnloadFont;
    font->get_all_siglyphs = f3GetAllGlyphs;
#ifdef XXX
    font->get_all_siglyphs =  0;
		/* above temp until bug fixed */
#endif
    font->free_glyphs = f3FreeGlyphs;
    font->refcnt = 0;
    font-> maxPrivate = -1;
    font->devPrivates = (pointer *) 0;
    font->maxsiPrivate = MAXSISCREENS;
    font->fontPrivate =  (pointer) f3font;
    font->svrPrivate = 0;
	/* close f3 font file ?? */
    font->rendererPtr->fonts_open++;
    return ret;

}
    


int
open_f3_font(fontname, filename, entry, format, fmask, flags)
   char *fontname;
   char *filename;
   FontEntryPtr entry;
   fsBitmapFormat format;
   fsBitmapFormatMask fmask;
   Mask flags;
{

   f3FontPtr f3font;
   FontPtr pfont;
   int result;
   int Xres, Yres;
   struct defaultVals {
        CARD16 x B16;
        CARD16 y B16;
        CARD16 point B16;
        } *defaultVals, *GetClientResolutions();
 
   char tmpname[MAXFONTNAMELEN];

	Xres = Yres = resx = resy = points = pixels = transX = transY = 0;
	/* make a master if we don't have one */

	strcpy(tmpname, fontname);
	if (!FontParseXLFDName(tmpname, &vals, FONT_XLFD_REPLACE_NONE))
		return BadFontName;

	f3rendererInfo = (RendererConfigPtr) &entry->u.scalable.renderer->config;
	result = type_SetFont(filename);
	if (!result) return result;
		/* note filename is a full path and filename */


	resx = vals.x;
	resy = vals.y;
	
	pixels = vals.pixel;
	points = vals.point;

	if ((pixels==0) && (points ==0)) {
         if (f3rendererInfo->renderer_defaults.point != 0)
		points = f3rendererInfo->renderer_defaults.point; 
	else
		points = 120;
		/* set tot default size of 12 */
#ifdef DEBUG
		fprintf(stderr,"pixels & pts both zero so set to default 12\n");
#endif
	}
	if (points != 0) {
		points = points/10;
	}
            /* default resolution to 72, so points == pixels */
	defaultVals = GetClientResolutions(&result);

	if (result !=0) {
	 Xres = defaultVals->x;
	 Yres = defaultVals->y;
	} 
	
    	if (resx == 0) {
		resx = Xres;
            	}
	if (resy == 0)  {
            	resy = Yres;
	}
	if (resx == 0) resx = 72;
	if (resy ==0) resy = 72;
		/* if still zero default to 72 */
#ifdef DEBUG
fprintf(stderr,"pixels=%d resx=%d resy=%d points=%d\n",pixels,resx,resy,points);
#endif
	if ((points ==0) && (pixels !=0)) {
		points =  ROUNDTOINT((722.7* pixels) /  resy);
		points = points / 10;
		/* calculate pointsize from pixelsize */

	} 
	
	transX = (resx * points) / 72;
	transY = (resy * points) / 72;

	return Successful;
}


int
f3FreeGlyphs(font)
FontPtr font;
{
  f3FontPtr f3font;
  CharInfoPtr cptr;
  f3ScaleInfoPtr scaleptr;

   int i,j;


#ifdef DEBUG
fprintf(stderr,"f3FreeGlyphs\n");
#endif

   f3font = (f3FontPtr) font->fontPrivate;
   scaleptr = (f3ScaleInfoPtr) f3font->scaled;

   for (i=0; i < font->info.numChars; i++, scaleptr++) {
		/* free xalloc chars */
	scaleptr->allocated = FALSE;
        scaleptr->cached = FALSE;
        scaleptr->scaled = FALSE;
        cptr= &f3font->scaled[i].charMetrics;
	cptr->bits = 0;
	}

   font->info.rerender = TRUE;
   font->info.reallocate = TRUE;
}



void
f3UnloadFont(font)
FontPtr font;
{    
  f3FontPtr f3font;
  CharInfoPtr cptr;
  f3ScaleInfoPtr scaleptr;

   f3font = (f3FontPtr) font->fontPrivate;

   FreePerFontCache(font);
   xfree(f3font);
   xfree(font);
   font->rendererPtr->fonts_open--;
}
   
#ifndef  XXX
int 
f3GetAllGlyphs(font, glyphs)
FontPtr font;
CharInfoPtr *glyphs;
{
    f3FontPtr f3font;
    f3ScaleInfoPtr scaleptr;
    CharInfoPtr cptr;

    register int i,j;
 

    f3font = (f3FontPtr) font->fontPrivate;

	/* not sure if we have to prerender them all if we
		are downloading put we must preallocate and
		not free */
		/* we can refine later if not needed but for
		now renderer all the glyphs in this routine */

    i = prerender_glyphs(font);
    if (!i) return AllocError;
		/* get all the glyphs rendererd */
    for (j=0; j < (unsigned) f3font->nglyphs;j++) {
#ifdef DEBUG
fprintf(stderr,"in f3GetAllGlyphs\n");
#endif
	cptr = &f3font->scaled[j].charMetrics; 
	*glyphs = cptr;
	glyphs++;
	}
	
return Successful;
}

#endif 

int
prerender_glyphs(font)
FontPtr font;
{
   f3FontPtr f3font;
   CharInfoPtr cptr;
   f3ScaleInfoPtr scaleptr;
   int i, j,count;
   int ret = 0;
    char *c;
#ifdef DEBUG
fprintf(stderr,"in prerender_glyphs\n");
#endif
	

    f3font = (f3FontPtr) font->fontPrivate;

#ifdef DEBUG
	fprintf(stderr,"calling InitPerFontCache size=%d size cnt=%d\n",f3font->gsize,f3font->nglyphs);
#endif
	ret=InitPerFontCache(font, f3font->gsize, f3font->nglyphs);
	if (ret < 0)  {
  		xfree (f3font);
		return AllocError;
	}
	font->info.usingGlyphCache = TRUE;
	scaleptr = f3font->scaled;
         cptr = &f3font->scaled[0].charMetrics;

	/* only allocate room for valid chars - so minglyph start in
		array position 0 etc. thru max */
    for (i=f3font->minglyph,j=0; j < (unsigned) f3font->nglyphs; i++,j++) {
	 scaleptr=f3font->scaled + j;
         cptr = &scaleptr->charMetrics;
	/* i = character code and j = array position */
	 if (scaleptr->exists == TRUE && scaleptr->allocated == FALSE &&
		scaleptr->glyphsize > 0)  {
			/* need to cache the glyph */
	/* this routine assume the font has already been opened which
		we know it has since they are sending a font ptr down */
#ifdef DEBUG
    	fprintf(stderr,"calling CacheGlyphs size=%d\n",scaleptr->glyphsize);
#endif
             cptr->bits = (char *)CacheGlyphs(font,scaleptr->glyphsize, 1,0);
             if (!cptr->bits) {
	
		FreePerFontCache(font);
  		xfree (f3font);
		return AllocError;
             	}
	     memset(cptr->bits, 0, scaleptr->glyphsize);
#ifdef DEBUG
	fprintf(stderr,"cached is true and allocated is true\n");
#endif
	     scaleptr->cached = TRUE;
             scaleptr->allocated = TRUE;
	     ret=folio_buildchar(cptr, scaleptr, &font->info, f3font, i, BC_PAINT);
             if (ret == AllocError) {
		 FreePerFontCache(font);
  		 xfree (f3font);
		return AllocError;
		} 
			/* renderer the character */
	  }
    }	

return Successful;
}

static CharInfoRec nonExistantChar;

int
f3GetMetrics(pfont, count, chars, charEncoding, glyphCount, glyphs )
    FontPtr     pfont;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding charEncoding;
    unsigned long *glyphCount;  /* RETURN */
    xCharInfo **glyphs;         /* RETURN */
{
    int         ret;
    xCharInfo  *ink_metrics;
    CharInfoPtr oldDefault;
    f3FontPtr   f3font;
    int         i;

    f3font = (f3FontPtr ) pfont->fontPrivate;
    oldDefault = f3font->pDefault;
    f3font->pDefault = &nonExistantChar;
    ret = f3GetGlyphs(pfont, count, chars, charEncoding, glyphCount, (CharInfoPtr *) glyphs, 0, 0);
    f3font->pDefault = oldDefault;
    return ret;
}


void
folio_get_chars(numglyphs, minglyph, defaultchar)
int *numglyphs;
int *minglyph;
int *defaultchar;
{
int count=0;
int *codes;
int code[256];
int nglyphs;
int  min;
int defaultc;
  
        codes= (int *)&code[0];
 
        nglyphs = (Unsgn16)type_GetSymbolCodes(codes,256);
        count = nglyphs;
#ifdef DEBUG
        fprintf(stderr,"code[0]=%d\n",code[0]);
        fprintf(stderr,"count=%d\n",count);
#endif
	defaultc = folio_defaultChar;
        min = code[0];
	if (min> defaultc)  {
		count += (min-defaultc);
		nglyphs = count;
		min= defaultc;
	}
#ifdef DEBUG
	fprintf(stderr,"count=%d minglyph=%d\n",count,min);
	fprintf(stderr,"nglyphs=%d\n",nglyphs);
#endif
	*defaultchar = defaultc;
	*minglyph = min;
	*numglyphs = nglyphs;
}



FolioComputeInfoAccelerators(pFontInfo)
    FontInfoPtr pFontInfo;
{
    pFontInfo->noOverlap = FALSE;
    if (pFontInfo->maxOverlap <= pFontInfo->minbounds.leftSideBearing)
	pFontInfo->noOverlap = TRUE;

    if ((pFontInfo->minbounds.ascent == pFontInfo->maxbounds.ascent) &&
	    (pFontInfo->minbounds.descent == pFontInfo->maxbounds.descent) &&
	    (pFontInfo->minbounds.leftSideBearing ==
	     pFontInfo->maxbounds.leftSideBearing) &&
	    (pFontInfo->minbounds.rightSideBearing ==
	     pFontInfo->maxbounds.rightSideBearing) &&
	    (pFontInfo->minbounds.characterWidth ==
	     pFontInfo->maxbounds.characterWidth) &&
      (pFontInfo->minbounds.attributes == pFontInfo->maxbounds.attributes)) {
	pFontInfo->constantMetrics = TRUE;
	if ((pFontInfo->maxbounds.leftSideBearing == 0) &&
		(pFontInfo->maxbounds.rightSideBearing ==
		 pFontInfo->maxbounds.characterWidth) &&
		(pFontInfo->maxbounds.ascent == pFontInfo->fontAscent) &&
		(pFontInfo->maxbounds.descent == pFontInfo->fontDescent))
	    pFontInfo->terminalFont = TRUE;
	else
	    pFontInfo->terminalFont = FALSE;
    } else {
	pFontInfo->constantMetrics = FALSE;
	pFontInfo->terminalFont = FALSE;
    }
    if (pFontInfo->minbounds.characterWidth == pFontInfo->maxbounds.characterWidth)
	pFontInfo->constantWidth = TRUE;
    else
	pFontInfo->constantWidth = FALSE;

    if ((pFontInfo->minbounds.leftSideBearing >= 0) &&
	    (pFontInfo->maxOverlap <= 0) &&
	    (pFontInfo->minbounds.ascent >= -pFontInfo->fontDescent) &&
	    (pFontInfo->maxbounds.ascent <= pFontInfo->fontAscent) &&
	    (-pFontInfo->minbounds.descent <= pFontInfo->fontAscent) &&
	    (pFontInfo->maxbounds.descent <= pFontInfo->fontDescent))
	pFontInfo->inkInside = TRUE;
    else
	pFontInfo->inkInside = FALSE;
}



/*
 * cs_getxfontprops -
 * There are no properties yet, so this is a NOP in a non-DEBUG
 * server.  
 */
void
cs_getxfontprops (f3font, fontprops)
  f3FontPtr f3font;
  XFONT_PROP* fontprops;
{
    int i;

    /* If the XfontInfo request ever becomes ordered things should
     * change.
     */

    if (f3font->properties) {
	i=0;
	while (f3font->properties[i].name != 0)
	{
#ifdef DEBUG
		fprintf(stderr,"prop name=%s ",f3font->properties[i].name);
#endif
	    fontprops[i] = f3font->properties[i];
#ifdef DEBUG
		fprintf(stderr,"fprop=%s\n",fontprops[i]);
#endif
	    ++i;
	}
    }
}

static int  f3b_set;
static int  bit, byte, glyph, scan;

f3bGetFormat (font)
FontPtr font;
{
    if (!f3b_set)
	FontDefaultFormat (&bit, &byte, &glyph, &scan);
font->bit = bit;
font->byteorder = byte;
font->glyph = glyph;
font->scan = scan;
}


int
getsize_to_preallocate(f3font,asize, acnt)
f3FontPtr f3font;
int *asize;
int *acnt;
{
    *asize = 0;
    *acnt = 0;

    if (f3rendererInfo->prerender == TRUE) 
		f3rendererInfo->preallocate_val = 100;

	/* setting prerender glyphs to TRUE implies preallocate
		as well */

    if (f3rendererInfo->preallocate_val == 100)  {
		*asize = f3font->gsize;
		*acnt = f3font->nglyphs;
    } else 
    if (f3rendererInfo->preallocate_val > 0 && f3rendererInfo->preallocate_val < 100){
		*asize =  f3font->gsize * f3rendererInfo->preallocate_val;
		*asize = *asize * PERCENT;
		*acnt =   f3font->nglyphs * f3rendererInfo->preallocate_val;
		*acnt = *acnt *  PERCENT;
#ifdef DEBUG
	fprintf(stderr,"totglyphsize=%d\n",f3font->gsize);
	fprintf(stderr,"allocateval=%d\n", f3rendererInfo->preallocate_val);
#endif

	}
	
}
   
int
f3GetGlyphs(pfont, count, chars, charEncoding, glyphcount,glyphs, flag, glist)
    FontPtr pfont;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding charEncoding;
    unsigned long *glyphcount;  /* RETURN */
    CharInfoPtr glyphs[];       /* RETURN */
    int flag;
    INT16 glist[];		/* RETURN */
{
    f3FontPtr f3font;
    unsigned int firstCol = pfont->info.firstCol;
    register unsigned int numCols;
    unsigned int firstRow;
    unsigned int cDef;
    unsigned int numRows;

    register unsigned int c;
    register unsigned long i;
    unsigned long n;

    register CharInfoPtr	ci;
    register unsigned int min;
    f3ScaleInfoPtr scaleptr;
    register CharInfoPtr cptr;
    unsigned int row, col; 
    unsigned char *schars;
    unsigned int chDefault = pfont->info.defaultCh;
    int ret = 0;
    cDef = pfont->info.defaultCh - firstCol;
    schars = chars;
    f3font = (f3FontPtr) pfont->fontPrivate;
    min = f3font->minglyph;
    scaleptr = (f3ScaleInfoPtr) f3font->scaled;
    numCols = pfont->info.lastCol - firstCol + 1;

#ifdef DEBUG
fprintf(stderr,"totglyphsize=%d\n",pfont->info.tot_glyphsize);
#endif
    for (i =0; i < count; i++) {
	c = (*schars++);
#ifdef DEBUG
	fprintf(stderr, "min=%d firstCol=%d ", min,firstCol);
	fprintf(stderr,"c=%d\n ",c);
#endif
	if (c < min) continue;
	scaleptr = f3font->scaled + (c - min);
	cptr = (CharInfoPtr) &scaleptr->charMetrics;
		/* set pointer to character to check */
	if (scaleptr->exists == FALSE ) {
		continue;
		/* if character does not exist or is already built skip it */
		}
        if (scaleptr->allocated == FALSE && scaleptr->glyphsize > 0) {

#ifdef DEBUG
    	fprintf(stderr,"calling CacheGlyphs size=%d\n",scaleptr->glyphsize);
#endif
		cptr->bits = (char *) CacheGlyphs(pfont,scaleptr->glyphsize,1,1);
		if (!cptr->bits)  {
			FreePerFontCache(pfont);
		         xfree(f3font);
#ifdef DEBUG
			fprintf(stderr,"return AllocErr =%d\n", AllocError);
#endif
			return AllocError;
		}
		scaleptr->allocated = TRUE;
		scaleptr->cached = TRUE;
		memset(cptr->bits, 0, scaleptr->glyphsize);

	}

	if (scaleptr->scaled != TRUE) 
#ifdef DEBUG
	fprintf(stderr,"call buildchar for c=%d\n",c );
#endif
		ret=folio_buildchar(cptr, scaleptr, &pfont->info, f3font, c, BC_PAINT);
 		if (ret == AllocError)  {
			FreePerFontCache(pfont);
			xfree (f3font);
			return AllocError;
		}
			/* scale the character */


	}



    n = 0;
    switch (charEncoding) {

	case Linear8Bit:
	case TwoD8Bit:
	    if (pfont->info.allExist && (cDef < numCols)) {
		for (i=0; i < count; i++) {

		    c = (*chars++) - firstCol;
		    if (c >= numCols) {
			c = cDef;
		    }
		    ci = &f3font->scaled[c].charMetrics;
		    if (flag) glist[i] = c;
		    glyphs[i] = ci;
		}
		n = count;
	    } else {
		for (i=0; i < count; i++) {
    
		    c = (*chars++) - firstCol;
		    if (c < numCols) {
		    ci = &f3font->scaled[c].charMetrics;
			if (ci->bits) {
			    if (flag) glist[n] = c;
			    glyphs[n++] = ci;
			    continue;
			}
		    }
    
		    if (cDef < numCols) {
		    ci = &f3font->scaled[c].charMetrics;
			if (ci->bits) {
			    if (flag) glist[n] = cDef;
			    glyphs[n++] = ci;
			}
		    }
		}
	    }
	    break;

	case Linear16Bit:
	    if (pfont->info.allExist && (cDef < numCols)) {
		for (i=0; i < count; i++) {

		    c = *chars++ << 8;
		    c = (c | *chars++) - firstCol;
		    if (c >= numCols) {
			c = cDef;
		    }
		    ci = &f3font->scaled[c].charMetrics;
		    if (flag) glist[i] = c;
		    glyphs[i] = ci;
		}
		n = count;
	    } else {
		for (i=0; i < count; i++) {
    
		    c = *chars++ << 8;
		    c = (c | *chars++) - firstCol;
		    if (c < numCols) {
		    ci = &f3font->scaled[c].charMetrics;
			if (ci->bits) {
			    if (flag) glist[n] = c;
			    glyphs[n++] = ci;
			    continue;
			}
		    }
    
		    if (cDef < numCols) {
		    ci = &f3font->scaled[c].charMetrics;
			if (ci->bits) {
			    if (flag) glist[n] = cDef;
			    glyphs[n++] = ci;
			}
		    }
		}
	    }
	    break;

	case TwoD16Bit:
            firstRow = pfont->info.firstRow;
	    numRows = pfont->info.lastRow -firstRow + 1;
  
	    for (i=0; i < count; i++) {
		register unsigned int row;
		register unsigned int col;

		row = (*chars++) - firstRow;
		col = (*chars++) - firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    ci = &f3font->scaled[c].charMetrics;
		    if (ci->bits) {
		        if (flag) glist[n] = c;
			glyphs[n++] = ci;
			continue;
		    }
		}

		row = (chDefault>> 8)-firstRow;
		col = (chDefault & 0xff)-firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    ci = &f3font->scaled[c].charMetrics;
		    if (ci->bits) {
		        if (flag) glist[n] = c;
			glyphs[n++] = ci;
		    }
		}
	    }
	    break;
    }
    *glyphcount = n;
     return Successful;
}
/* SI: end */

