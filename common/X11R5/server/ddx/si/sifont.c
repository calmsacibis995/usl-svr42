/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sifont.c	1.6"

#ifdef DEBUG
#include <stdio.h>
#endif
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
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
*/
/* $XConsortium: mfbfont.c,v 1.16 89/03/18 12:28:12 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "misc.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
/* SI: start */
#include "miscstruct.h"
#include "si.h"
#include "sidep.h"
/* SI: end */

/* SI: start */
static char *sihdwrfused = (char *)0;
static int   sihdwrfcnt = 0;
/* SI: end */

void
siinitfonts()
{
    register int i;

    if (sihdwrfused) {
	Xfree(sihdwrfused);
	sihdwrfused = (char *)0;
    }
    if ( si_havedlfonts && (sihdwrfcnt = si_GetInfoVal(SIfontcnt)) > 0) {
	sihdwrfused = (char *)Xalloc(sihdwrfcnt);
	for(i = sihdwrfcnt; --i >= 0; )
	    sihdwrfused[i] = 0;
    }
}

static Bool
sihasfreefont(pindex)
int *pindex;
{
    register int i;

    if ( si_havedlfonts && sihdwrfcnt) {
	for(i = 0; i < sihdwrfcnt; i++) {
	    if (sihdwrfused[i] == 0)
		break;
	}
	if (i < sihdwrfcnt) {
	    if (pindex)
		*pindex = i;
	    return(TRUE);
	}
    }
    return(FALSE);
}

#define siusefont(index)	sihdwrfused[(index)] = 1
#define sireleasefont(index)	sihdwrfused[(index)] = 0

/*
 * Take advantage of the per-screen private information field in the font to
 * encode the results of fairly complex tests of the font's metric fields.
 * ValidateFont need merely examine the code to select the output routines to
 * be pointed to in the GC.
 */
Bool
siRealizeFont( pscr, pFont)
    ScreenPtr	pscr;
    FontPtr	pFont;
{
    /*
     * pGC->font is now known to be valid
     */
    int			index = pscr->myNum;
    CharInfoPtr	        pci;
    unsigned int        cDef = FONTDEFAULTCH(pFont);
    int			ret;
    int		        i, j, size;
    siPrivFontP	        pPriv;
    unsigned char       *pglyphBase = (unsigned char *)FONTGLYPHS(pFont);
    int		      	ffont;
    unsigned long *glyphcount;
    CharInfoPtr *glyphs;
    SIGlyphP	        glist;
    pointer 		*new;

    pPriv = (siPrivFontP)Xalloc(sizeof(siPrivFont));
    if (index >= pFont->maxsiPrivate)
    		return FALSE;
    pFont->siPrivates[index] = (pointer)pPriv;
    pPriv->fonttype = UNOPT_FONT;

    /* For now all the glyphs must exist (makes glyph index calc easy) */
    if (*pFont->get_all_siglyphs ==  NULL) return(TRUE) ;
    if ( si_havedlfonts && sihasfreefont(&ffont) == TRUE) {
	/* load up font info */
	pPriv->fastidx.firstCol = i = FONTFIRSTCOL(pFont);

	pPriv->fastidx.numCols = FONTLASTCOL(pFont) - i + 1;

	pPriv->fastidx.firstRow = j = FONTFIRSTROW(pFont);

	pPriv->fastidx.numRows = FONTLASTROW(pFont) - j + 1;
	pPriv->fastidx.chDefault = j = FONTDEFAULTCH(pFont);
	pPriv->fastidx.cDef = j - i;

	size = pPriv->fastinfo.SFnumglyph =
			pPriv->fastidx.numCols * pPriv->fastidx.numRows;

	pPriv->fastinfo.SFflag = 0;
	if (TERMINALFONT(pFont))
            pPriv->fastinfo.SFflag |= SFTerminalFont;
	if (FONTCONSTWIDTH(pFont))
            pPriv->fastinfo.SFflag |= SFFixedWidthFont;
	if (FONTNOOVERLAP(pFont))
            pPriv->fastinfo.SFflag |= SFNoOverlap;
	pPriv->fastinfo.SFlascent = FONTASCENT(pFont);
	pPriv->fastinfo.SFldescent = FONTDESCENT(pFont);

	pPriv->fastinfo.SFmin.SFlbearing = FONTMINBOUNDS(pFont,leftSideBearing);
	pPriv->fastinfo.SFmin.SFrbearing = FONTMINBOUNDS(pFont,rightSideBearing);
	pPriv->fastinfo.SFmin.SFwidth = FONTMINBOUNDS(pFont,characterWidth);
	pPriv->fastinfo.SFmin.SFascent = FONTMINBOUNDS(pFont,ascent);
	pPriv->fastinfo.SFmin.SFdescent = FONTMINBOUNDS(pFont,descent);

	pPriv->fastinfo.SFmax.SFlbearing = FONTMAXBOUNDS(pFont,leftSideBearing);
	pPriv->fastinfo.SFmax.SFrbearing = FONTMAXBOUNDS(pFont,rightSideBearing);
	pPriv->fastinfo.SFmax.SFwidth = FONTMAXBOUNDS(pFont,characterWidth);
	pPriv->fastinfo.SFmax.SFascent = FONTMAXBOUNDS(pFont,ascent);
	pPriv->fastinfo.SFmax.SFdescent = FONTMAXBOUNDS(pFont,descent);
        if (*pFont->get_all_siglyphs ==  NULL) return(TRUE) ;
	if ((ParseCheck4Download(pFont) == 1) && si_checkfont(ffont, &pPriv->fastinfo) == SI_TRUE) {

#ifdef DEBUG
		fprintf(stderr,"font to download\n");
#endif
	    glyphs = (CharInfoPtr *) ALLOCATE_LOCAL(size * sizeof(CharInfoPtr));


            pFont->info.freeWhen = FONT_DONT_FREE;
            pFont->info.downloaded = TRUE;
	    ret = (*pFont->get_all_siglyphs) (pFont,  glyphs); 
	    if (ret == AllocError) {
			DEALLOCATE_LOCAL(glyphs);
			return ret;
	    }
            glist = (SIGlyphP) ALLOCATE_LOCAL(size * sizeof(SIGlyph));
            for(i = 0; i < size; i++) {
		pci = glyphs[i];

		if (!pci->bits) {
                    pci = glyphs[cDef];
		}
		glist[i].SFlbearing = pci->metrics.leftSideBearing;
		glist[i].SFrbearing = pci->metrics.rightSideBearing;
		glist[i].SFwidth = pci->metrics.characterWidth;
		glist[i].SFascent = pci->metrics.ascent;
		glist[i].SFdescent = pci->metrics.descent;
		glist[i].SFglyph.Bdepth = 1;
		glist[i].SFglyph.BorgX = 0;
		glist[i].SFglyph.BorgY = 0;
		glist[i].SFglyph.Bheight = pci->metrics.ascent +
                                           pci->metrics.descent;
		glist[i].SFglyph.Bwidth = pci->metrics.rightSideBearing -
                                          pci->metrics.leftSideBearing;
		glist[i].SFglyph.Bptr = (SIArray)FONTGLYPHBITS(pglyphBase,
                                                  pci);

            }
            if (si_fontdownload(ffont,&pPriv->fastinfo,glist) == SI_TRUE) {
		pPriv->fonttype = HDWR_FONT;
		pPriv->hdwridx = ffont;
		siusefont(ffont);
            }
            DEALLOCATE_LOCAL(glist);
            DEALLOCATE_LOCAL(glyphs);
	}
    }
/* SI: end */

    return (TRUE);
}

/*
 * no storage allocated in siRealizeFont, so there is nothing to do
 */
/*ARGSUSED*/
Bool
siUnrealizeFont( pscr, pFont)
    ScreenPtr	pscr;
    FontPtr	pFont;
{
    int			index = pscr->myNum;
    siPrivFontP		pPriv;

    pPriv = (siPrivFontP)pFont->siPrivates[index];
    if (pPriv->fonttype == HDWR_FONT) {
	si_fontfree(pPriv->hdwridx);
	sireleasefont(pPriv->hdwridx);
    } 
    Xfree(pPriv);
    return (TRUE);
}
