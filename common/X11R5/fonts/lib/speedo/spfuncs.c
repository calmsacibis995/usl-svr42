/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libspeedo:speedo/spfuncs.c	1.3"
/* $XConsortium: spfuncs.c,v 1.5 91/09/16 11:42:30 keith Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 *
 *
 * Author: Dave Lemke, Network Computing Devices, Inc
 *
 * $NCDId: @(#)spfuncs.c,v 4.8 1991/07/02 17:01:44 lemke Exp $
 *
 */

#include 	"fontstruct.h"
#include	"Xrenderer.h"
#include	<X11/Xos.h>
#include	"fontfilest.h"
#include	"spint.h"

FontRendererPtr spdrenderer;
RendererConfigPtr spdrendererInfo;

int spdFontRendererInit();

/* ARGSUSED */
SpeedoOpenScalable (fpe, pFont, flags, entry, fileName, vals, format, fmask)
    FontPathElementPtr	fpe;
    FontPtr		*pFont;
    int			flags;
    FontEntryPtr	entry;
    char		*fileName;
    FontScalablePtr	vals;
    fsBitmapFormat	format;
    fsBitmapFormatMask	fmask;
{
    char	fullName[MAXFONTNAMELEN];

    strcpy (fullName, entry->name.name);
    FontParseXLFDName (fullName, vals, FONT_XLFD_REPLACE_VALUE);
    return SpeedoFontLoad (pFont, fullName, fileName, entry,
			    format, fmask, flags);
}

/*
 * XXX
 *
 * this does a lot more then i'd like, but it has to get the bitmaps
 * in order to get accurate metrics (which it *must* have).
 *
 * a possible optimization is to avoid allocating the glyph memory
 * and to simply save the values without doing the work.
 */
static int
get_font_info(pinfo, fontname, filename, entry, spfont)
    FontInfoPtr pinfo;
    char       *fontname;
    char       *filename;
    FontEntryPtr	entry;
    SpeedoFontPtr *spfont;
{
    SpeedoFontPtr spf;
    int         err;

    err = open_sp_font(fontname, filename, entry,
	       (fsBitmapFormat) 0, (fsBitmapFormatMask) 0, (unsigned long) 0,
		       spfont);
    spf = *spfont;

    if (err != Successful)
	return err;

    cur_spf = spf;
    sp_reset_master(spf->master);

    make_sp_header(spf, pinfo);

    compute_sp_bounds(spf, pinfo, (unsigned long) 0);

    compute_sp_props(spf, fontname, pinfo);

    /* compute remaining accelerators */
    FontComputeInfoAccelerators (pinfo);

    return Successful;
}

/* ARGSUSED */
SpeedoGetInfoScaleable(fpe, pFontInfo, entry, fontName, fileName, vals)
    FontPathElementPtr	fpe;
    FontInfoPtr		pFontInfo;
    FontEntryPtr	entry;
    FontNamePtr		fontName;
    char		*fileName;
    FontScalablePtr	vals;
{
    SpeedoFontPtr spf;
    char        fullName[MAXFONTNAMELEN];
    int         err;

    fixup_vals(vals);

    strcpy(fullName, entry->name.name);
    FontParseXLFDName(fullName, vals, FONT_XLFD_REPLACE_VALUE);

    err = get_font_info(pFontInfo, fullName, fileName, entry, &spf);

    close_sp_font(spf);
    return err;
}

#ifndef USL
static FontRendererRec renderer = {
    ".spd", 4, (int (*)()) 0, SpeedoOpenScalable,
	(int (*)()) 0, SpeedoGetInfoScaleable, 0
};
    
SpeedoRegisterFontFileFunctions()
{
    make_sp_standard_props();
    sp_reset();
    FontFileRegisterRenderer(&spdrenderer);
}

#endif

int
spdFontRendererInit(p_renderer) 
FontRendererPtr p_renderer;
{
	int ret;

	p_renderer->OpenBitmap =  0;
	p_renderer->OpenScalable = SpeedoOpenScalable;
	p_renderer->GetInfoScalable = SpeedoGetInfoScaleable;
	p_renderer->GetInfoBitmap = 0;
	p_renderer->FreeRenderer = 0;
	p_renderer->fonts_open = 0;
	ret = ParseRendererPublic(p_renderer);
		/* get font configuration options set */
	spdrenderer = p_renderer;
	spdrendererInfo = &spdrenderer->config;
		/* set rendererInfo for later use */
	make_sp_standard_props();
	sp_reset();
}
