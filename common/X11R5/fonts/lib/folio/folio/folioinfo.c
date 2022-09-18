#ident	"@(#)libfolio1.2:folio/folioinfo.c	1.1"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $XConsortium: spinfo.c,v 1.7 91/09/16 11:42:32 keith Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Author: Dave Lemke, Network Computing Devices, Inc
 *
 */

#include	"fontfilest.h"
#include	"f3font.h"

/* percentage of pointsize used to specify ascent & descent */
#define	STRETCH_FACTOR	120

enum scaleType {
    atom, pixel_size, point_size, resolution_x, resolution_y, average_width,
    scaledX, scaledY, unscaled, scaledXoverY, uncomputed
};

typedef struct _fontProp {
    char       *name;
    long        atom;
    enum scaleType type;
}           fontProp;

static fontProp fontNamePropTable[] = {
    "FOUNDRY", 0, atom,
    "FAMILY_NAME", 0, atom,
    "WEIGHT_NAME", 0, atom,
    "SLANT", 0, atom,
    "SETWIDTH_NAME", 0, atom,
    "ADD_STYLE_NAME", 0, atom,
    "PIXEL_SIZE", 0, pixel_size,
    "POINT_SIZE", 0, point_size,
    "RESOLUTION_X", 0, resolution_x,
    "RESOLUTION_Y", 0, resolution_y,
    "SPACING", 0, atom,
    "AVERAGE_WIDTH", 0, average_width,
    "CHARSET_REGISTRY", 0, atom,
    "CHARSET_ENCODING", 0, atom,
};

static fontProp extraProps[] = {
    "FONT", 0, atom,
    "COPYRIGHT", 0, atom,
};

/* this is a bit kludgy */
#define	FONTPROP	0
#define	COPYRIGHTPROP	1

#define NNAMEPROPS (sizeof(fontNamePropTable) / sizeof(fontProp))
#define NEXTRAPROPS (sizeof(extraProps) / sizeof(fontProp))

#define	NPROPS	(NNAMEPROPS + NEXTRAPROPS)

void
folio_make_standard_props()
{
    int         i;
    fontProp   *t;

    i = sizeof(fontNamePropTable) / sizeof(fontProp);
    for (t = fontNamePropTable; i; i--, t++)
	t->atom = MakeAtom(t->name, (unsigned) strlen(t->name), TRUE);
    i = sizeof(extraProps) / sizeof(fontProp);
    for (t = extraProps; i; i--, t++)
	t->atom = MakeAtom(t->name, (unsigned) strlen(t->name), TRUE);
}

void
folio_compute_props(font, fontname, pinfo)
    f3FontPtr  font;
    char *fontname;
    FontInfoPtr pinfo;
{
    FontPropPtr pp;
    int         i,
                nprops;
    fontProp   *fpt;
    char       *is_str;
    char       *ptr1,
               *ptr2;

    nprops = pinfo->nprops = NPROPS;
    pinfo->isStringProp = (char *) xalloc(sizeof(char) * nprops);
    pinfo->props = (FontPropPtr) xalloc(sizeof(FontPropRec) * nprops);
    if (!pinfo->isStringProp || !pinfo->props) {
	xfree(pinfo->isStringProp);
	pinfo->isStringProp = (char *) 0;
	xfree(pinfo->props);
	pinfo->props = (FontPropPtr) 0;
	return;
    }
    memset(pinfo->isStringProp,0, (sizeof(char) * nprops));

    ptr2 = fontname;
    for (i = NNAMEPROPS, pp = pinfo->props, fpt = fontNamePropTable,
	    is_str = pinfo->isStringProp;
	    i;
	    i--, pp++, fpt++, is_str++) {
	ptr1 = ptr2 + 1;
	if (*ptr1 == '-')
	    ptr2 = ptr1;
	else {
	    if (i > 1)
		ptr2 = strchr(ptr1 + 1, '-');
	    else
		ptr2 = strchr(ptr1 + 1, '\0');
	}
	pp->name = fpt->atom;
	switch (fpt->type) {
	case atom:
	    *is_str = TRUE;
	    pp->value = MakeAtom(ptr1, ptr2 - ptr1, TRUE);
	    break;
	case pixel_size:
	    pp->value = font->vals.pixel;
	    break;
	case point_size:
	    pp->value = font->vals.point;
	    break;
	case resolution_x:
	    pp->value = font->vals.x;
	    break;
	case resolution_y:
	    pp->value = font->vals.y;
	    break;
	case average_width:
	    pp->value = font->vals.width;
	    break;
	}
    }

    for (i = 0, fpt = extraProps; i < NEXTRAPROPS; i++, is_str++, pp++, fpt++) {
	pp->name = fpt->atom;
	switch (i) {
	case FONTPROP:
	    *is_str = TRUE;
	    pp->value = MakeAtom(fontname, strlen(fontname), TRUE);
	    break;
	}
    }
}
