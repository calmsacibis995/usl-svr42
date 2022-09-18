#ident	"@(#)r5fontlib:font/renderers.c	1.6"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



/*

 * $XConsortium: renderers.c,v 1.1 91/05/10 14:46:38 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include  <stdio.h>
#include  "Xrenderer.h"
#include  "fontfilest.h"
#include  "confmac.h"

FontRenderersRec renderers; /* needs to be global for us */


FontFileRegisterFontFileFunctions ()
{
	short caller = FONTSERVER_TYPE;
		/* X server calls special routine. */
        snfRegisterFontFileFunctions (caller); /* this is default */
	adobeRegisterFontFileFunctions(caller);
	folioRegisterFontFileFunctions(caller);

	SpeedoRegisterFontFileFunctions (caller);

	LoadRenderers(&renderers);

}



Bool
FontFileRegisterRenderer (renderer)
    FontRendererPtr renderer;
{
    int		    i;
    FontRendererPtr *new;

    for (i = 0; i < renderers.number; i++)
	if (!strcmp (renderers.renderers[i]->fileSuffix, renderer->fileSuffix))
	    return TRUE;
    i = renderers.number + 1;
    new = (FontRendererPtr *) xrealloc (renderers.renderers, sizeof *new * i);
    if (!new)
	return FALSE;
    renderer->number = i - 1;
    renderers.renderers = new;
    renderers.renderers[i - 1] = renderer;
    renderers.number = i;
    return TRUE;
}

FontRendererPtr
FontFileMatchRenderer (fileName)
    char    *fileName;
{
    int			i;
    int			fileLen;
    FontRendererPtr	r;
    char *filename2; 
    fileLen = strlen (fileName);
    filename2 = strchr(fileName, '.');
    if ((filename2 == NULL)  || (fileName == NULL))return 0;
		/* code for sun4 sparc platforms that core dump
			on strcmp of NULL strings */
    for (i = 0; i < renderers.number; i++)
    {
	r = renderers.renderers[i];
	if (fileLen >= r->fileSuffixLen &&
	    !strcmp (fileName + fileLen - r->fileSuffixLen + 1, r->fileSuffix))
	{
	    return r;
	}
	if (!strcmp(fileName, r->fileSuffix)) return r;
	if (!strcmp(++filename2, r->fileSuffix)) return r;
    }
    return 0;
}


