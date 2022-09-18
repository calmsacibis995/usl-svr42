/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipolytext.c	1.5"

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

/*******************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
************************************************************************/
/* $XConsortium: mipolytext.c,v 5.0 89/06/09 15:08:40 keith Exp $ */

/*
 * mipolytext.c - text routines
 *
 * Author:	haynes
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Thu Feb  5 1987
 */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"misc.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
/***** SI: start *****/
#include 	"regionstr.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"si.h"
/***** SI: end *******/

/***** SI: start *****/
/* text support routines. A charinfo array builder, and a bounding */
/* box calculator */

/* int siGCPrivateIndex = 1;*/	/* SI (for now!!) */

void
miGetGlyphs(font, count, chars, fontEncoding, glyphcount, glyphs, glist)
    FontPtr font;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding fontEncoding;
    unsigned long *glyphcount;	/* RETURN */
    CharInfoPtr glyphs[];	/* RETURN */
    SIint16 glist[];		/* RETURN */
{
  int ret;

 ret = (*font->get_glyphs) (font, count, chars, fontEncoding, glyphcount, glyphs,1,glist);

 if (ret != Successful && ret != 1) *glyphcount = 0;
 font->info.last_blittime = GetFontTime();
}



int
miPolyText(pDraw, pGC, x, y, count, chars, fontEncoding, dn, chi, gli, num, wid)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char 	*chars;
    FontEncoding fontEncoding;
    /* SI: start */
    int		dn;
    CharInfoPtr	*chi;
    SIint16	*gli;
    unsigned int num, wid;
    /* SI: end */
     
{
   /* SI: start */
    CharInfoPtr *charinfo;
    SIint16 *glist;
    unsigned int n, w, i;	/* SI (added i) */
    siPrivFontP pPriv = (siPrivFontP)(pGC->font->siPrivates[pGC->pScreen->myNum]);

    if (dn)
    {
	charinfo = chi;
	glist = gli;
	w = wid;
	n = num;
    } else
    {
	if(!(charinfo = (CharInfoPtr *)ALLOCATE_LOCAL(count*sizeof(CharInfoPtr))))
	    return x ;
	if(!(glist = (SIint16 *)ALLOCATE_LOCAL(count*sizeof(SIint16))))
        {
	    DEALLOCATE_LOCAL(charinfo);
	    return x ;
        }
	miGetGlyphs(pGC->font, count, chars, fontEncoding, &n, charinfo, glist);
        w = 0;
        for (i=0; i < n; i++) w += charinfo[i]->metrics.characterWidth;
    }
    if (n != 0)
    {
	if (pDraw->type == DRAWABLE_WINDOW &&
	    si_havedlfonts && si_hasstipple(SIavail_font) &&
	    pPriv->fonttype == HDWR_FONT &&
	    pGC->fillStyle == FillSolid)
        {
	    register BoxPtr pbox;
	    register int nbox;
	    int ifont;

	    if (pGC->miTranslate)
            {
		x += pDraw->x;	/* SI (R4) */
		y += pDraw->y;	/* SI (R4) */
	    }
	    ifont = pPriv->hdwridx;
	    CHECKINPUT();
	    si_PrepareGS(pGC);
	    if (si_hascliplist(SIavail_font))
            {
		si_fontstplblt(ifont, x, y, n, glist, SGStipple);
	    } else
            {
		pbox = REGION_RECTS(((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);		/* SI (R4) */
		nbox = REGION_NUM_RECTS(((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);		/* SI (R4) */
		while(nbox--)
                {
		    CHECKINPUT();
		    si_fontclip(pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
		    si_fontstplblt(ifont, x, y, n, glist, SGStipple);
		    pbox++;
		}
		si_fontclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
	    }
	} else
            (*pGC->ops->PolyGlyphBlt)(		/* SI (ops in R4) */
		pDraw, pGC, x, y, n, charinfo, FONTGLYPHS(pGC->font));
    }
    if (!dn)
    {
	DEALLOCATE_LOCAL(glist);
	DEALLOCATE_LOCAL(charinfo);
    }
   /* SI: end */

    return x+w;
}

int
miPolyText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
    return miPolyText(pDraw, pGC, x, y, count, chars, Linear8Bit,
		0, (CharInfoPtr *)0,(SIint16 *)0, 0, 0);	/* SI */
}

int
miPolyText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    register CharInfoPtr *charinfo;
    unsigned long n, i;
    unsigned int w;

    /* SI: start */
    if (FONTLASTROW(pGC->font) == 0)
	return miPolyText(pDraw, pGC, x, y, count, (char *)chars, Linear16Bit,
		0, (CharInfoPtr *)0,(SIint16 *)0, 0, 0);
    else
        return miPolyText(pDraw, pGC, x, y, count, (char *)chars, TwoD16Bit,
		0, (CharInfoPtr *)0,(SIint16 *)0, 0, 0);
    /* SI: end */
}

int
miImageText(pDraw, pGC, x, y, count, chars, fontEncoding, dn, chi, gli, num)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int 	x, y;
    int 	count;
    char 	*chars;
    FontEncoding fontEncoding;
    /* SI: start */
    int		dn;
    CharInfoPtr *chi;
    SIint16	*gli;
    unsigned int num;
    /* SI: end */
{
   /* SI: start */
    CharInfoPtr *charinfo;
    SIint16 *glist;
    unsigned int n;
    siPrivFontP pPriv = (siPrivFontP)(pGC->font->siPrivates[pGC->pScreen->myNum]);

    if (dn)
    {
	charinfo = chi;
	glist = gli;
	n = num;
    } else {
	if(!(charinfo = (CharInfoPtr *)ALLOCATE_LOCAL(count*sizeof(CharInfoPtr))))
	    return;
	if(!(glist = (SIint16 *)ALLOCATE_LOCAL(count*sizeof(SIint16))))
        {
	    DEALLOCATE_LOCAL(charinfo);
	    return;
        }
	miGetGlyphs(pGC->font, count, chars, fontEncoding, &n, charinfo, glist);
    }
    if (n !=0 )
    {
	if (pDraw->type == DRAWABLE_WINDOW &&
	    si_havedlfonts && si_hasopqstipple(SIavail_font) &&
	    pPriv->fonttype == HDWR_FONT &&
	    pGC->fillStyle == FillSolid)
        {
	    register BoxPtr pbox;
	    register int nbox;
	    int ifont;

	    if (pGC->miTranslate)
            {
		x += pDraw->x;		/* SI (R4) */
		y += pDraw->y;		/* SI (R4) */
	    }
	    ifont = pPriv->hdwridx;
	    CHECKINPUT();
	    si_PrepareGS(pGC);
	    if (si_hascliplist(SIavail_font))
            {
		si_fontstplblt(ifont, x, y, n, glist, SGOPQStipple);
	    } else
            {
		pbox = REGION_RECTS(((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);	/* SI (R4) */
		nbox = REGION_NUM_RECTS(((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);	/* SI (R4) */
		while(nbox--) 
                {
		    CHECKINPUT();
		    si_fontclip(pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
		    si_fontstplblt(ifont, x, y, n, glist, SGOPQStipple);
		    pbox++;
		}
		si_fontclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
	    }
	} else
            (*pGC->ops->ImageGlyphBlt)(		/* SI (ops in R4) */
		pDraw, pGC, x, y, n, charinfo, FONTGLYPHS(pGC->font));
    }
    if (!dn)
    {
	DEALLOCATE_LOCAL(glist);
	DEALLOCATE_LOCAL(charinfo);
    }
    /* SI: end */
}

void
miImageText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{
    CharInfoPtr *charinfo;
    unsigned long n;
    FontPtr font = pGC->font;

    miImageText(pDraw, pGC, x, y, count, chars, Linear8Bit,
                        0, (CharInfoPtr *)0, (SIint16 *)0, 0);	/* SI */
}

void
miImageText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    CharInfoPtr *charinfo;
    unsigned long n;
    FontPtr font = pGC->font;

    /* SI: start */
    if (FONTLASTROW(pGC->font) == 0)
	miImageText(pDraw, pGC, x, y, count, (char *)chars, Linear16Bit,
                        0, (CharInfoPtr *)0, (SIint16 *)0, 0);
    else
        miImageText(pDraw, pGC, x, y, count, (char *)chars, TwoD16Bit,
                        0, (CharInfoPtr *)0, (SIint16 *)0, 0);
    /* SI: end */
}
