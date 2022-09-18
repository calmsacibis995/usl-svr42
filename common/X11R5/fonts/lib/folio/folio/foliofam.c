#ident	"@(#)libfolio1.2:folio/foliofam.c	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
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

/*
 * Folio font family operations.
 */

#include <cdefs.h>
#include <gdefs.h>
#include <stdio.h>
#include <f3font.h>
#include <typesclr.h>
#include <string.h>
#include <image.h>

extern long GetFontTime();
extern void BitOrderInvert();

static Unsgn16 last_transX=0;
static Unsgn16 last_transY=0;
static Unsgn16 last_ptsize=0;

int folio_initalize_font();
int folio_buildchar();


static void
set_x_char_metrics(m, origin_x, origin_y, size_x, size_y,width)
  register xCharInfo *m;
  register int origin_x, origin_y;
  register int size_x, size_y;
  register int width;
{
    m->leftSideBearing = - origin_x;
    m->rightSideBearing = size_x - origin_x;
    m->characterWidth =  width;
    m->ascent = origin_y;
    m->descent = size_y - origin_y;
    m->attributes = 0;
#ifdef DEBUGFOLIO
fprintf(stderr,"m->leftSideBearing=%d ",m->leftSideBearing);
fprintf(stderr,"m->rightSideBearing=%d ",m->rightSideBearing);
fprintf(stderr,"m->characterWidth=%d ",m->characterWidth);
fprintf(stderr,"m->ascent=%d ",m->ascent);
fprintf(stderr,"m->descent=%d\n",m->descent);
#endif

}

int
folio_initialize_font(f3font)
    f3FontPtr      f3font;
{	
    trans_dTrans trans;
    bbox_dBBox  dbbox;
    bbox_dBBox  ubbox;
    bbox_iBBox  bbox;
    float f;
    
    /*
     * Compute the f3font metrics
     */

    
    if (!type_SetFont(f3font->folioname))
	return -1;
    trans.a = 1;
    trans.b = 0;
    trans.c = 0;
    trans.d = 1;
    trans.dx = 0;
    trans.dy = 0;
    type_SetTrans(&trans);
    f3_GetFontBBox(&trans, &ubbox);
#ifdef DEBUGFOLIO
fprintf(stderr,"after f3_GetFontBBox\n"); 
#endif


    last_transX = f3font->transX;
    last_transY = f3font->transY;
    last_ptsize = f3font->ptsize;

    trans.a = f3font->transX;
    trans.b = trans.c = 0;
    trans.d = f3font->transY;
    trans.dx = 0;
    trans.dy = 0;

    type_SetTrans(&trans);

    if (!f3_GetFontBBox(&trans, &dbbox)) {
#ifdef DEBUG
	fprintf(stderr,"getFontBBox fail\n");
#endif
	return -1;
	}
    /* This is basically a guess as to what the size of characters */
    /* in this font will really be, since the folio fill code may */
    /* go outside this box.    */

    f3font->gsize = 0;
}

int
folio_buildchar(cptr, scaleptr, fontinfo, f3font, code,  op)
register CharInfoPtr cptr;
register f3ScaleInfoPtr scaleptr;
FontInfoPtr fontinfo;
f3FontPtr f3font;
Sgn32 code;
enum ff_buildchar_op op;
{
    register struct wglyph *g;
    struct wglyph saveglyph;
    bbox_iBBox  bbox;
    pair_dXY    pair,
                disp;
    type_memory mem;
    unsigned long starttime, endtime;
    int	*pglyph;
    int width,width2;
    Sgn32	amount;
    Sgn32	width_bytes;

#ifdef DEBUG1
fprintf(stderr,"foliobuildchar code=%d op=%d maxchars=%d\n",code,op,f3font->nglyphs);
#endif
g= &saveglyph;

    if (!type_SetFont(f3font->folioname))
	    	{
#ifdef DEBUG
		fprintf(stderr,"SetFont fail\n");
#endif
		return -1;
    	}
if (last_transX != f3font->transX ||
	last_transY != f3font->transY ||
	last_ptsize != f3font->ptsize) 
	{
	
	trans_dTrans trans;
	last_transX = f3font->transX;
	last_transY = f3font->transY;
	last_ptsize = f3font->ptsize;
	trans.a = f3font->transX;
	trans.d = f3font->transY;
	trans.b = 0;
	trans.c = 0; 
	trans.dx = 0;
	trans.dy = 0;
	type_SetTrans(&trans);
    }


    type_SetCode(code);
    if (code == ' ') type_SetCode('i');
    if (!type_InitAccess()) {
	f3font->allexist = FALSE;
		/* found a character that does not exist*/	
	scaleptr->exists = FALSE;
        if (code != ' ' ) {
		return -1;
        } else {
	/* REMIND: I think this assumes a "reasonable" encoding */
            /* space is missing: use 'i' width */
        type_SetCode('i');
	if (!type_InitAccess()) {
		/*fprintf(stderr, "set code to i and it still fail \n");*/
		return -1;
		}
	}
   }

   type_GetBBox(&bbox);
   mem.origin.x = bbox.lox;
   mem.height = bbox.hiy - bbox.loy;
   mem.origin.y = bbox.loy;
   width_bytes = bbox.hix - bbox.lox;
   mem.width = (bbox.hix - bbox.lox +31) & ~31;

#ifdef DEBUGFOLIO
   fprintf(stderr,"glyph height=%d width=%d\n",mem.height,mem.width);
   fprintf(stderr,"glyph origin x=%d y=%d\n",mem.origin.x,mem.origin.y);
#endif

   width_bytes = (width_bytes +7) >> 3;
   width_bytes = (width_bytes +3)& ~3;
   amount = width_bytes * mem.height;
   if (op != BC_PAINT) {
		/* set byteOffset to bytes of memory needed for this char*/

#ifdef DEBUGFOLIO
  fprintf(stderr,"amount=%d for code=%d mem.width=%d mem.height=%d \n",amount,code,mem.width,mem.height);
	fprintf(stderr,"f3font->gsize=%d ",f3font->gsize);
#endif

   f3font->gsize += amount;
   scaleptr->glyphsize = amount;
   scaleptr->exists = TRUE;
   scaleptr->scaled = FALSE;


#ifdef DEBUGFOLIO

	fprintf(stderr,"exists=%d scaled=%d\n",scaleptr->exists, scaleptr->scaled);
	fprintf(stderr,"after incre. scaleptr->glyphsize=%d\n",scaleptr->glyphsize);
	fprintf(stderr,"after incre. f3font->gsize=%d\n",f3font->gsize);

#endif 
	/* increment the amount by the size needed for this glyph */
	/* this stuff may not work if size of glyph changes for*/
	/* scale factors which I think it does */	

   type_GetDisplacement(&disp);

   g->origin.x =  -mem.origin.x + roundfr(f3font->matrix[2][0] -fracti(disp.x));
   g->origin.y = mem.height + mem.origin.y
   	-   roundfr(f3font->matrix[2][1] - fracti(disp.y));
   type_GetAdvance(&pair);
   width = roundfr(fracti(pair.x));
   width2 = roundfr(fracti(disp.x));

   set_x_char_metrics(&cptr->metrics,g->origin.x, g->origin.y,
	bbox.hix-bbox.lox,mem.height, (width));	

   type_GetDisplacement(&disp);
   return 1;
   }


#ifdef DEBUGFOLIO
fprintf(stderr,"building: amount=%d for code=%d\n",amount,code);
#endif
	
   if (cptr->bits == NULL && scaleptr->allocated == TRUE) return AllocError;
	

	if (cptr->bits == NULL) {
#ifdef DEBUGFOLIO
		fprintf(stderr,"need to allocate memory for this glyph amount = %d glypsize=%d\n",amount,scaleptr->glyphsize);
#endif
		/* need to allocate memory for the glyph before calling 
			this rouinte  */
		cptr->bits = (char *) CacheGlyphs(f3font->fontPtr, amount, 1,1);
                if (cptr->bits == NULL) return AllocError;
                scaleptr->allocated = TRUE;
		scaleptr->cached = TRUE;
                
	}
       
	mem.bitmap = (uint32 *) (cptr->bits);
	mem.width = (bbox.hix - bbox.lox + 31) & ~31;
	memset(mem.bitmap, 0, (mem.width >> 3) * mem.height);
	if (code == ' ') {
		scaleptr->scaled = TRUE;
		return -1;  
		/* leave it set to all zeros for space */
	}
        starttime =  GetFontTime();
	type_MakeBitmap(&mem, FALSE);
	endtime = GetFontTime();
        f3font->fontPtr->info.render_time =  endtime - starttime;
	scaleptr->exists = TRUE;
	scaleptr->scaled = TRUE;
        {
        unsigned register long *p = (unsigned long *) mem.bitmap;
            register    cnt = mem.height * (mem.width >> 5);
            while (--cnt >= 0) {
                register unsigned long w = *p;
            *p = (  R[w>>24])
                    | (R[(w >> 16) & 0xFF] << 8)
                    | (R[(w >> 8) & 0xFF] << 16)
                    | (R[w & 0xFF] << 24);
                p++;
            }
        }
	
return  1;
}



int
folio_fontheader (f3font, fontinfo)

  f3FontPtr f3font;
  register FontInfoPtr fontinfo;
{
    int 		i;
    int 		nchars;

    /* REMIND: all this min/max byte stuff is wrong . . . */

    /*
     * If the font is remapped, then we can only access the first 256
     * characters.
     */
    nchars = font_get_nglyphs(f3font);
    f3font->nglyphs = nchars;
    fontinfo->numChars = nchars;
    fontinfo->firstCol = f3font->minglyph;
    fontinfo->lastCol = nchars - 1;
    fontinfo->firstRow = 0;
    fontinfo->lastRow = 0;
    fontinfo->defaultCh = f3font->defaultchar;
    fontinfo->inkMetrics = 0;
    fontinfo->allExist = 1;
    fontinfo->drawDirection = font_get_left_to_right (f3font) ?
	DrawsLeftToRight : DrawsRightToLeft;
    fontinfo->cachable = 1;
    fontinfo->anamorphic = 0;
    if (f3font->transX != f3font->transY) 
	    fontinfo->anamorphic = TRUE;


    /* Count the properties */
    i=0;
/*
    if (f3font->properties)
	while(f3font->properties[i].name != 0)
	    ++i;

    fontinfo->nprops = i;
*/

}


int
folio_compute_bounds (f3font, fontinfo, flag)

  f3FontPtr f3font;
  register FontInfoPtr fontinfo;
  Bool flag;
{
    /* Find the min/max bounding box, and the min/max widths. */

    /* Loop through all the characters . . . */
    int 		i;
    int 		nchars;
    CharInfoPtr   cptr;
    f3ScaleInfoPtr scaleptr;
    CharInfoRec tmpChar;
    f3ScaleInfo scaleChar;



    /* get info for default character and put that in byteOffset 0 
		then get info for rest of chars */

	/* initialize max and min values*/
    nchars = fontinfo->numChars;
    fontinfo->maxbounds.leftSideBearing = fontinfo->minbounds.leftSideBearing = 0;
    fontinfo->maxbounds.rightSideBearing = fontinfo->minbounds.rightSideBearing = 0;
    fontinfo->maxbounds.characterWidth = fontinfo->minbounds.characterWidth = 0;
    fontinfo->maxbounds.ascent = fontinfo->minbounds.ascent = 0;
    fontinfo->maxbounds.descent = fontinfo->minbounds.descent = 0;
        if (flag == TRUE)  {
	    scaleptr =  f3font->scaled;
	    cptr =  &scaleptr->charMetrics;
	    } else {
	      cptr = &tmpChar;
	      scaleptr = &scaleChar;
        }

    for (i = f3font->minglyph; i < nchars; i++) {
	unsigned short enc;
	enc = i;
	folio_buildchar(cptr, scaleptr ,fontinfo, f3font,  enc, 1);

	if (cptr->metrics.leftSideBearing > fontinfo->maxbounds.leftSideBearing)
	    fontinfo->maxbounds.leftSideBearing = cptr->metrics.leftSideBearing;
	if (cptr->metrics.leftSideBearing < fontinfo->minbounds.leftSideBearing)
	    fontinfo->minbounds.leftSideBearing = cptr->metrics.leftSideBearing;	
	
	if (cptr->metrics.rightSideBearing > fontinfo->maxbounds.rightSideBearing)
	    fontinfo->maxbounds.rightSideBearing = cptr->metrics.rightSideBearing;
	if (cptr->metrics.rightSideBearing <fontinfo-> minbounds.rightSideBearing)
	    fontinfo->minbounds.rightSideBearing = cptr->metrics.rightSideBearing;	

	if (cptr->metrics.characterWidth > fontinfo->maxbounds.characterWidth)
	    fontinfo->maxbounds.characterWidth = cptr->metrics.characterWidth;
	if (cptr->metrics.characterWidth <fontinfo->minbounds.characterWidth)
	    fontinfo->minbounds.characterWidth = cptr->metrics.characterWidth;	

	if (cptr->metrics.ascent > fontinfo->maxbounds.ascent)
	    fontinfo->maxbounds.ascent = cptr->metrics.ascent;
	if (cptr->metrics.ascent < fontinfo->minbounds.ascent)
	    fontinfo->minbounds.ascent = cptr->metrics.ascent;	

	if (cptr->metrics.descent > fontinfo->maxbounds.descent)
	    fontinfo->maxbounds.descent = cptr->metrics.descent;
	if (cptr->metrics.descent < fontinfo->minbounds.descent)
	    fontinfo->minbounds.descent = cptr->metrics.descent;	
        if (flag == TRUE)  {
	    scaleptr++;
	    cptr =  &scaleptr->charMetrics;
	    }
	}


    /* Set the bounding boxes. */

    fontinfo->fontAscent = fontinfo->maxbounds.ascent;
    fontinfo->fontDescent = fontinfo->maxbounds.descent;
    fontinfo->ink_maxbounds.leftSideBearing = fontinfo->maxbounds.leftSideBearing;
    fontinfo->ink_maxbounds.rightSideBearing = fontinfo->maxbounds.rightSideBearing;
    fontinfo->ink_maxbounds.characterWidth = fontinfo->maxbounds.characterWidth;
    fontinfo->ink_maxbounds.ascent = fontinfo->maxbounds.ascent;
    fontinfo->ink_maxbounds.descent = fontinfo->maxbounds.descent;
    fontinfo->ink_minbounds.rightSideBearing = fontinfo->minbounds.rightSideBearing;
    fontinfo->ink_minbounds.leftSideBearing = fontinfo->minbounds.leftSideBearing;
    fontinfo->ink_minbounds.characterWidth = fontinfo->minbounds.characterWidth;
    fontinfo->ink_minbounds.ascent = fontinfo->minbounds.ascent;
    fontinfo->ink_minbounds.descent = fontinfo->minbounds.descent;
#ifdef DEBUGFOLIO
fprintf(stderr,"fontinfo->fontAscent=%d fontinfo->fontDescent=%d\n",fontinfo->fontAscent,fontinfo->fontDescent);
fprintf(stderr,"MaxBounds: left=%d rt=%d wid=%d ascent=%d descent=%d\n",fontinfo->maxbounds.leftSideBearing,fontinfo->maxbounds.rightSideBearing,fontinfo->maxbounds.characterWidth,fontinfo->maxbounds.ascent,fontinfo->maxbounds.descent);
#endif


}
