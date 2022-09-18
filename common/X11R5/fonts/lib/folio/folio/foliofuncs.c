#ident	"@(#)libfolio1.2:folio/foliofuncs.c	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include    <X11/Xos.h>
#include    "fontfilest.h"
#include    "bitmap.h"
#include    "f3font.h"
#include    "Xrenderer.h"
#include    <signal.h>
#include    <ieeefp.h>
struct sigaction sigact;
struct sigaction sig_get;

extern int points;
extern int pixels;
extern int resx, resy, transX, transY;
void fixup_vals();
int FolioOpenScalable();
int FolioGetInfoScalable();
static initialized = 0;

int catch(sig)
int sig;
{
fp_rnd fpmode;
fp_except fpmask;
fp_except fpsticky;
fp_except j;


#ifdef DEBUG
fprintf(stderr,"Folio: caught signal %d\n",sig);
#endif
fpmode=fpgetround();
j = fpgetsticky() ; 
fpsticky = fpsetsticky(0);
}

f3bFontRendererInit(thisrenderer)
FontRendererPtr thisrenderer;
{
extern fp_rnd fpsetround();

        int ret;

        thisrenderer->OpenBitmap = 0;
        thisrenderer->OpenScalable = FolioOpenScalable;
        thisrenderer->GetInfoBitmap = 0;
        thisrenderer->GetInfoScalable = FolioGetInfoScalable;
        thisrenderer->FreeRenderer = 0;
        thisrenderer->number = 0;
        thisrenderer->fonts_open = 0;
        ret = ParseRendererPublic(thisrenderer);
		/* get font configuration options set */
  
if (!initialized) {
	type_BootInit();
	type_SetPixelRatio(1.0);
	type_SetSize(1.0);
	initialized = 1;
	sigact.sa_flags = 0;
	sigact.sa_handler = catch;
	(void) sigaddset(&sigact.sa_mask, SIGFPE);
	sigaction(SIGFPE, &sigact, (struct sigaction *) &sig_get);
	fpsetround(FP_RN);
        }
	folio_make_standard_props();
        return(1);
 
}



FolioOpenScalable (fpe, pFont, flags, entry, fileName, vals, format, fmask)
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
#ifdef  DEBUG
fprintf(stderr,"FolioOpenScalable: file=%s\n",fileName);
#endif
    return FolioOpenFont (pFont, fullName, fileName, entry,
			    format, fmask, flags);
}

FolioGetInfoScalable(fpe, pFontInfo, entry, fontName, fileName, vals)
    FontPathElementPtr	fpe;
    FontInfoPtr		pFontInfo;
    FontEntryPtr	entry;
    FontNamePtr		fontName;
    char		*fileName;
    FontScalablePtr	vals;
{
    f3FontPtr f3font;
    char        fullName[MAXFONTNAMELEN];
    int		namelen,f3len, bytestoalloc;
    char 	*fontspace;
    int         err;
    int		nchars, minglyph, defaultchar;

    fixup_vals(vals);

    strcpy(fullName, entry->name.name);
    FontParseXLFDName(fullName, vals, FONT_XLFD_REPLACE_VALUE);
    err = open_f3_font(fullName, fileName, entry, 0,0,0);
#ifdef DEBUG
	fprintf(stderr,"FolioGetInfoScalable: filename=%s\n",fileName);

#endif
    if (err != Successful) return err;
    f3len = sizeof(f3Font);
    bytestoalloc = namelen + f3len;
    fontspace = (char *) xalloc(bytestoalloc);
    if (!fontspace) return AllocError;
    memset(fontspace, 0, bytestoalloc);
    f3font = (f3FontPtr) fontspace;
    f3font->folioname = (char *) (fontspace + f3len);
    memcpy(f3font->folioname,fileName,namelen);
#ifdef DEBUG
fprintf(stderr,"folioname=%s\n",f3font->folioname);
#endif
    folio_get_chars(&nchars, &minglyph, &defaultchar);
    f3font->nglyphs = nchars;
    f3font->minglyph = minglyph;
    f3font->defaultchar = defaultchar;
    f3font->ptsize = points;
    f3font->vals.point = points;
    f3font->pixelsize = pixels;
    f3font->vals.pixel = pixels;
    f3font->transX = transX;
    f3font->transY = transY;
    f3font->vals.x = resx;
    f3font->vals.y = resy;
#ifdef DEBUG
    fprintf(stderr," transX=%d transy=%d\n",transX,transY);
#endif

    folio_fontheader(f3font, pFontInfo);
    folio_initialize_font(f3font);

    folio_compute_bounds(f3font, pFontInfo, FALSE);

    folio_compute_props(f3font, fullName, pFontInfo);

    /* compute remaining accelerators */
    FolioComputeInfoAccelerators (pFontInfo);

    xfree(fontspace);
    return Successful;
}



void
fixup_vals(vals)
    FontScalablePtr vals;
{
    int         x_res = 75;
    int         y_res = 75;
    int         pointsize = 120;
    int         num_res;

    if (!vals->x || !vals->y || (!vals->point && !vals->pixel)) {
	if (!vals->x)
	    vals->x = x_res;
	if (!vals->y)
	    vals->y = y_res;
	if (!vals->point)
	    vals->point = pointsize;
    }
}

