/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/dynopenlib.c	1.6"
/*copyright     "%c%"*/

#include  <stdio.h>

#include <dlfcn.h>
#include <Xrenderer.h>

char *get_symbolname();
extern FontRenderersRec renderers;

int (*FontRenderInitFunc)();
static void *handle;

int
DynamicOpenFontLibrary(thisrenderer)
FontRendererPtr thisrenderer;
{
	char *symbolname;
        char *name;
	int len, result, libcnt;

	name = thisrenderer->config.sharedlib_filename;
	libcnt = thisrenderer->number;
	if ((result = DoDynamicOpen(name)) == -1) {
                fprintf(stderr,"Unable to Open %s\n",name);
                return -1;
                }
	thisrenderer->config.handle = handle;
	if ((result = LinkSymbol(thisrenderer)) == -1) {;
		fprintf(stderr,"unable to link symbol closing object\n");
		fprintf(stderr,"symbol in renderer=%s\n",thisrenderer->config.symbol);
		dlclose(thisrenderer->config.handle);
		return -1;
	}


	result = CallInit(thisrenderer);
	result = LinkMatchingLibrary(thisrenderer,  name);
	return(result);
}


int
ClearMatchingLib(thisrenderer, name)
FontRendererPtr thisrenderer;
char *name;
{
int i, result, len;
int inuse;
FontRendererPtr r;
FontRenderersPtr rptr = &renderers;
inuse=0;
for (i = 0; i < rptr->number; i++,r++) {
	r = rptr->renderers[i];
	if ((r  != thisrenderer) &&
	(r->config.loaded == 1) &&
	(r->config.sharedlib_filename) &&
	(!strcmp(r->config.sharedlib_filename, name))) {

		/* found one to clear */
		r->config.initialized = FALSE;
		r->config.loaded = FALSE;
		r->config.handle = 0;
		}
	}
}


int
CheckUseBeforeLibClose(thisrenderer, name)
FontRendererPtr thisrenderer;
char *name;
{
int i, result, len;
int inuse;
FontRendererPtr r;
FontRenderersPtr rptr = &renderers;
inuse=0;
for (i = 0; i < rptr->number; i++,r++) {
	r = rptr->renderers[i];
	if ((r  != thisrenderer) &&
	(r->config.loaded == 1) &&
	(r->config.sharedlib_filename) &&
	(!strcmp(r->config.sharedlib_filename, name))) {

			/* found a match */ 	
	if (r->fonts_open > 0) 
		/* library is in use with another suffix so don't close */
		inuse = TRUE;
	}

   }
return (inuse);
}

int
LinkMatchingLibrary(thisrenderer, name)
FontRendererPtr thisrenderer;
char *name;
{
int i, result, len;
FontRendererPtr r;
FontRenderersPtr rptr = &renderers;
	for (i = 0; i < rptr->number; i++,r++) {
		r = rptr->renderers[i];
		if ((r  != thisrenderer) &&
		(r->config.initialized == 0) &&
		(r->config.sharedlib_filename) &&
		(!strcmp(r->config.sharedlib_filename, name))) {

		if ((result= LinkSymbol(r)) != -1) {
			result = CallInit(r);
			/* found a match */ 	
	 		if (result != 1) 
				r->config.initialized = 2;
			}
		}

   	}
	return (1);
}

int
CheckOtherSuffixes(thisrenderer,name)
FontRendererPtr thisrenderer;
char *name;
{
FontRendererPtr r;
int i,match;
FontRenderersPtr rptr = &renderers;

	for (match=0,i = 0; i < rptr->number; i++,r++) {
		r = rptr->renderers[i];
		if ((r  != thisrenderer) &&
		(r->config.sharedlib_filename) &&
		(!strcmp(r->config.sharedlib_filename, name))) {
			match++;
			/* found a match */ 	
			}

   	}
	return (match);
}


DoDynamicOpen(libname)
char *libname;
{

	handle = dlopen(libname, RTLD_NOW);
#ifdef DEBUG
	fprintf(stderr,"libname=%s\n",libname);	
#endif
        if(!handle )
        {
                fprintf(stderr,"dlopen <%s> failed\nReason: %s\n", libname, dlerror());
                return(-1);
        }
return(1);
}


int
LinkSymbol(thisrenderer)
FontRendererPtr thisrenderer;
{
char *symbolname;

	symbolname=thisrenderer->config.symbol;
        FontRenderInitFunc = (int (*)())dlsym(handle, symbolname);
        if(!FontRenderInitFunc)
        {
                fprintf(stderr,"dlsym <%s> failed\nReason: %s\n", symbolname, dlerror());
	   	thisrenderer->config.initialized =  2;
			/* dysm failed */
                return(-1);
        }

return(1);
}

		
CallInit(thisrenderer)
FontRendererPtr thisrenderer;
{

(FontRenderInitFunc)(thisrenderer);
	/* call the initialization routine */


thisrenderer->config.initialized = TRUE;
thisrenderer->config.handle = handle;
thisrenderer->config.loaded = TRUE;

return(1);
}
