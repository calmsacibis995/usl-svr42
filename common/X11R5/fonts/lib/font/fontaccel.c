/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/fontaccel.c	1.1"
/*

 * $XConsortium: fontaccel.c,v 1.2 91/05/11 09:16:34 keith Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include    "fontmisc.h"
#include    "fontstruct.h"

FontComputeInfoAccelerators(pFontInfo)
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

FontCouldBeTerminal(pFontInfo)
    FontInfoPtr pFontInfo;
{
    if ((pFontInfo->minbounds.leftSideBearing >= 0) &&
	    (pFontInfo->maxbounds.rightSideBearing <= pFontInfo->maxbounds.characterWidth) &&
	    (pFontInfo->minbounds.characterWidth == pFontInfo->maxbounds.characterWidth) &&
	    (pFontInfo->maxbounds.ascent <= pFontInfo->fontAscent) &&
	    (pFontInfo->maxbounds.descent <= pFontInfo->fontDescent) &&
	    (pFontInfo->maxbounds.leftSideBearing != 0 ||
	     pFontInfo->minbounds.rightSideBearing != pFontInfo->minbounds.characterWidth ||
	     pFontInfo->minbounds.ascent != pFontInfo->fontAscent ||
	     pFontInfo->minbounds.descent != pFontInfo->fontDescent)) {
	return TRUE;
    }
    return FALSE;
}
