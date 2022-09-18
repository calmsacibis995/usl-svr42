/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/getfont.c	1.11"
/*
 * Author:  Keith Packard, MIT X Consortium
 */

#include  <stdio.h>
#include  "Xrenderer.h"
#include  "fontfilest.h"
#include  "confmac.h"

extern char *fontconfig_file;
extern char *renderer_options[];
extern short fntopts;
FontRenderersRec renderers; /* needs to be global for us */
#include <stdio.h>

#include "Xos.h"
#include "Xrenderer.h"
#include <sys/param.h>

#ifndef DEFAULT_ADOBELIB
#define DEFAULT_ADOBELIB 	"libatm.so"
#endif
#ifndef DEFAULT_FOLIOLIB
#define DEFAULT_FOLIOLIB 	"libfolio.so"
#endif

#ifndef DEFAULT_SPEEDOLIB
#define DEFAULT_SPEEDOLIB	"libspd.so"
#endif


#ifndef DEFAULTSNFLIB
#define DEFAULTSNFLIB 	"libbitmap.so"
#endif

extern char *cmdline_fontpath;
void setScalableDefaults();

FontRendererPtr AllocateRenderer();
int FreeUnusedRenderers();
int LoadRenderers();
int CheckForDupLibs();



XFontFileRegisterFontFileFunctions ()
{
	short caller = XSERVER_TYPE;
        snfRegisterFontFileFunctions (caller); /* this is default */
/*	adobeRegisterFontFileFunctions(caller);*/
	folioRegisterFontFileFunctions(caller);
	/*SpeedoRegisterFontFileFunctions (caller);*/
	if (!fontconfig_file) fontconfig_file = (char *)GetConfigFileName();
	if (!fontconfig_file) 
		fprintf(stderr, "Warning: missing font configuration file: %s\n",  fontconfig_file);
 	else	
	XReadConfigFile(fontconfig_file);
	if (fntopts != 0) CheckCommandLineRenderers();

	FreeUnusedRenderers(&renderers);
			/* free any set to do not use and
			already registered like defaults */

	LoadRenderers(&renderers);

}



CheckCommandLineRenderers()
{
int i,j,donotuse;
char *ptr;
     for (i=0; i < fntopts; i++) {
	if ((renderer_options[i][0] == '+') || 
	(renderer_options[i][0] == '-')) {	
	        if (renderer_options[i][0] == '-') donotuse = 1;
		   else donotuse = 0;
      		for (j =0; j < renderers.number; j++) {

		ptr = renderer_options[i];
		ptr++;
	      	if (!strcmp(renderers.renderers[j]->fileSuffix,ptr))	

		   	renderers.renderers[j]->config.donotuse = donotuse;
	      }
	   }

    }


}



int 
FreeUnusedRenderers(renderers)
FontRenderersPtr renderers;
{
    FontRenderersRec nrenderers;
    FontRendererPtr *new;

    int i, j, used;
    j = used = 0;

    for (i = 0; i < renderers->number; i++) 
	if (renderers->renderers[i]->config.donotuse!= 1) used++;

if (used == renderers->number) return 1;
		/* none of the renderers are unused */
new = (FontRendererPtr *) xalloc( sizeof (FontRendererPtr) * used);
if (!new) return -1;
for (i=0; i < renderers->number; i++) {
	if (renderers->renderers[i]->config.donotuse == 1) {
		xfree(renderers->renderers[i]->fileSuffix);
		xfree(renderers->renderers[i]->config.symbol);
		xfree(renderers->renderers[i]->config.sharedlib_filename);
		xfree(renderers->renderers[i]->config.derived_instances);
		xfree(renderers->renderers[i]->config.rendererPrivate);
		xfree(renderers->renderers[i]); 
			/* free the Font config etc. info for */
			/* the one not to be used */
	} else {
		new[j++] = renderers->renderers[i];
		}
	}
		
	/* now check thru for duplicate libraries */
xfree(renderers->renderers);	/* free original list */
renderers->number  =  used;
renderers->renderers = new;
return(1);
}


int
LoadRenderers(renderers)
FontRenderersPtr renderers;
{
    int i,ret=0;
    char *curr_lib;

    for (i=0; i < renderers->number; i++) {

	if ((renderers->renderers[i]->config.preload_renderer== 1) &&
		(renderers->renderers[i]->config.initialized == 0 ) &&
		(renderers->renderers[i]->config.loaded != 1))  {
		curr_lib = renderers->renderers[i]->config.sharedlib_filename;
		ret = CheckForDupLibs(curr_lib, renderers);
		ret= DynamicOpenFontLibrary(renderers->renderers[i],renderers);
		}
	}	
}


int
CheckForDupLibs(curr, renderers)
char *curr;
FontRenderersPtr renderers;
{

     int i, j;
     int match = 0;
     for (i =0; i < renderers->number; i++)  {
	if (!strcmp(renderers->renderers[i]->config.sharedlib_filename, curr))
			match++;
	}
     if (!match) return 0;
     for (i =0; i < renderers->number; i++)  {
	if (!strcmp(renderers->renderers[i]->config.sharedlib_filename, curr))
			renderers->renderers[i]->number = match;
		/* set each FontRenderPtr that matches number field
			to indicated the total number of suffixes
			using this library */

	}
     return(match);

}

int
snfRegisterFontFileFunctions (caller)
short caller;
{
	
	int len;
	FontRendererPtr renderer;
	const char suffix[] = "snf";

	renderer = (FontRendererPtr) AllocateRenderer(&suffix, sizeof(suffix),caller);
	if (!renderer) return (-1);
    
			
	renderer->config.preallocate_val = 100;
	renderer->config.preload_renderer = 1;
	renderer->config.type = 2;
	renderer->config.renderer_defaults.point = 12;
	renderer->config.prerender = 1;
	renderer->config.sharedlib_filename = DEFAULTSNFLIB;
	FontFileRegisterRenderer(renderer);
#ifdef SNFARCHIVE
	snfFontRendererInit(renderer); /* do without shared lib */
	renderer->config.close_when = FALSE;
	renderer->config.loaded = TRUE;
	
#endif
       return(1);
}


int
adobeRegisterFontFileFunctions (caller)
short caller;
{
	
	int len;
	FontRendererPtr renderer;
	const char *suffix[] = { "ps", "pfb", "pfa", "PFA", "PFB", 0} ;
	int i;
	for (i=0; suffix[i] != 0; i++) {
	  len =strlen(suffix[i]);
	  renderer = (FontRendererPtr) AllocateRenderer(suffix[i], len+1, caller);
	  if (!renderer) return (-1);
	  renderer->config.sharedlib_filename = DEFAULT_ADOBELIB;
	  setScalableDefaults(renderer);
	  renderer->config.prerender = TRUE;
		/* prerendere these fonts because slow boxes barf on them */
	}
#ifdef ADOBEARCHIVE
	pfaFontRendererInit(renderer); /* do without shared lib */
	pfbFontRendererInit(renderer); /* do without shared lib */
	psFontRendererInit(renderer); /* do without shared lib */
	PFAFontRendererInit(renderer); /* do without shared lib */
	PFBFontRendererInit(renderer); /* do without shared lib */
	renderer->config.close_when = FALSE;
	renderer->config.loaded = TRUE;
#endif

}

void
setScalableDefaults(renderer)
FontRendererPtr renderer;
{
	renderer->config.preallocate_val = 0;
	renderer->config.preload_renderer = 0;
	renderer->config.type = 2;
	renderer->config.renderer_defaults.point = 12;
	FontFileRegisterRenderer(renderer);

}


int
folioRegisterFontFileFunctions (caller)
short caller;
{
	
	int len;
	FontRendererPtr renderer;
        const char suffix[] = "f3b";

	renderer = (FontRendererPtr) AllocateRenderer(&suffix, sizeof(suffix),caller);
	if (!renderer) return (-1);
	renderer->config.sharedlib_filename = DEFAULT_FOLIOLIB;
	setScalableDefaults(renderer);
	renderer->config.prerender = FALSE;
		/* folio is the only one that does not take a hit
			to prerender */
#ifdef FOLIOARCHIVE
	f3bFontRendererInit(renderer); /* do without shared lib */
	renderer->config.close_when = FALSE;
	renderer->config.loaded = TRUE;
	
#endif
       return(1);
}


FontRendererPtr
AllocateRenderer(suffix, len, caller)
char *suffix;
int len;
short caller;
{
int result;
FontRendererPtr renderer;

       renderer = (FontRendererPtr) FontFileMatchRenderer(suffix);
	/* check if already there */
       if (renderer) return(renderer);
       renderer = (FontRendererPtr) xalloc(sizeof (FontRendererRec));
       if (!renderer) return(NULL);
       memset(renderer, 0, sizeof(FontRendererRec));
       renderer->fileSuffix = (char *)xalloc(len);
       if (!renderer->fileSuffix) {
	       xfree(renderer);
	       return NULL;
	}

       strncpy(renderer->fileSuffix, suffix,len);
       renderer->fileSuffixLen = len;
       renderer->config.symbol = (char *)xalloc(len + 17);
       if (renderer->config.symbol == NULL) {
	       xfree(renderer->fileSuffix);
	       xfree(renderer);
	       return NULL;
	}
       renderer->caller= (short)caller;
       strncpy(renderer->config.symbol,renderer->fileSuffix, len-1);
       strcat(renderer->config.symbol+len-1,"FontRendererInit");
       return(renderer);
}



int
SpeedoRegisterFontFileFunctions(callerType)
short callerType;
{
	
	int len;
	FontRendererPtr renderer;
        const char suffix[] = "spd";

	renderer = (FontRendererPtr) AllocateRenderer(&suffix, sizeof(suffix), callerType);
	if (!renderer) return (-1);
	renderer->config.sharedlib_filename = DEFAULT_SPEEDOLIB;
	renderer->config.prerender = TRUE;
	setScalableDefaults(renderer);
        return(1);
}


