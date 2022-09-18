#ident	"@(#)r5fontinc:include/Xrenderer.h	1.6"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef X_RENDERER

#define X_RENDERER
#define PERCENT 1/100

#define	XSERVER_TYPE		1
#define FONTSERVER_TYPE		2

#include "fontmisc.h"
#include "fontxlfd.h"

#define NullFontRenderer            ((FontRendererPtr) 0)


typedef struct _RendererConfig {
    FontScalableRec	renderer_defaults;
    char *sharedlib_filename;               /* full pathname lib to use */
    char *symbol;
    void *handle; 
    int   *derived_instances;
    short numDerived;               /* number of derived_instances  
                                        pointsize values */
    short  preallocate_val;        /* minimum preallocate percentage */
    short  rendererStatus; 
    short  initialized;
    unsigned int  donotuse:1;         /* library should be used or not */
    unsigned int  loaded:1;         /* library is loaded or not */
    unsigned int  renderPriv:1;     /* contain renderer private 
                                        info or not */
    unsigned int  download:2;
    unsigned int  type:2;          /* fonts are bitmap, scalable or both */
    unsigned int  preload_renderer:2;  /* initialize library at font open */
    unsigned int  close_when:2;    /* close library on last font close */
    unsigned int  prerender:2;     /* mimimum prerenderer glyphs  */
    unsigned int  callerType:2;  /* called by X server=1; fontserver=2  */
    short  mincache;
    short  maxcache;
    short  dload_height;            /* maximum height of downloaded fonts */
    short  dload_width;            /* maximum width of downloaded fonts */
    short  dload_maxchars;         /* maximum chars is a download font */
    short  alloc_units;
    short  maxRenderPriv;
    pointer *rendererPrivate;     /* first one has config file name/location*/
} RendererConfigRec, *RendererConfigPtr;


typedef struct _FontRenderer {
    char *fileSuffix;
    int fileSuffixLen;
    short caller;
    short future;
    int	    (*OpenBitmap)(/* fpe, pFont, flags, entry, fileName, format, fmask */);
    int	    (*OpenScalable)(/* fpe, pFont, flags, entry, fileName, vals, format, fmask */);
    int	    (*GetInfoBitmap)(/* fpe, pFontInfo, entry, fileName */);
    int	    (*GetInfoScalable)(/* fpe, pFontInfo, entry, fileName, vals */);
    int	    (*FreeRenderer) (/* fpe, pFont */);
    short   number; 
    short   fonts_open;
    RendererConfigRec config;
} FontRendererRec, *FontRendererPtr;

typedef struct _FontRenders {
     short  number;
     short  use_renderer;
    FontRendererPtr *renderers;
} FontRenderersRec, *FontRenderersPtr;

#endif /*X_RENDERER */
