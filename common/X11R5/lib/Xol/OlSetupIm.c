/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:OlSetupIm.c	1.11"
#endif

#if defined(SYSV) || defined(USL)
#include <limits.h>	/* contains PATH_MAX */
#else
#include <sys/param.h>
#define PATH_MAX MAXPATHLEN
#endif

#if defined(SVR4_0) || defined(SVR4)
#include <dlfcn.h>
#endif /* SVR4 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>

#if !defined(SLASH)
#define SLASH "/"
#endif

#if !defined(DOTESSO)
#define DOTESSO ".so"
#endif

#if !defined(MAX_IM)
#define MAX_IM 128
#endif

#if !defined(LPATH)
/*
#define LPATH "/usr/X/lib/locale/"
*/
#define LPATH "/usr/X/lib/locale"
#endif

extern OlImFunctions *
_OlSetupInputMethod OLARGLIST((dpy, im_name, lang_locale, res_name, res_class))
OLARG( Display *, dpy)
OLARG( String, im_name)
OLARG( String, lang_locale)
OLARG( String, res_name)
OLGRA( String, res_class)
{
static OlImFunctions *imf_struct = (OlImFunctions *) NULL;
static void *handle;
char *env = (char *) NULL;
char *dirpath = (char *)NULL;
static char *prev_im;

/* Dynamic linking in SVR4.0 only (not 3.2) */
#if defined(SVR4_0) || defined(SVR4)

	if (!imf_struct)
		{
		imf_struct = (OlImFunctions *) XtMalloc(sizeof(OlImFunctions));
		}

	if (im_name == NULL || *im_name == NULL) {
		if (prev_im){
				XtFree(prev_im);
				prev_im = NULL;
			}
		return((OlImFunctions *) NULL);
	}

	if (prev_im){
		if (strcmp(prev_im,im_name)){
		      /*
			    * input method has changed, close old library and open new
			    */
		   dlclose(handle);
			XtFree(prev_im);		
			prev_im = NULL;
			}
		else{
		      /*
			    * No change, return saved pointer 
			    */
			return(handle);
		}
   }
		/*
		 * Attach to input method library.  Save name and handle 
		 *	for future.
		 */
	if ( (dirpath = (char *)malloc((size_t)PATH_MAX)) == (char *) NULL)
		return((OlImFunctions *) NULL);
	*dirpath = '\0';

	if ( (env = (char *)getenv("OLIMLIBPATH")) != NULL)
		strcat(dirpath, env);
	else {
		strcat(dirpath, LPATH);
		strcat(dirpath, SLASH);
		strcat(dirpath, lang_locale);
	}
	strcat(dirpath,SLASH);
	/* Are we sure that the im_name supplied will be libxxx, and
	 * not just xxx?  Otherwise we'll have to append the "lib"
	 * to the dirpath before we append the xxx.
	 */
	strcat(dirpath,im_name);
	strcat(dirpath, DOTESSO);
	if ( (handle = dlopen(dirpath, RTLD_NOW)) != NULL) {
		/* The cast says:
		 * OlIm * (*) ()
		 * (*): This is a pointer to something (a function:())
		 * that returns a OlIm * (returns a ptr to OlIm struct)
		 */
		imf_struct->OlOpenIm = (OlIm *(*)()) dlsym(handle,"OlOpenIm");
		/* This cast says, it's a pointer to a function returning
		 * an integer.
		 */
		imf_struct->OlOpenIm =
				(OlIm *(*)()) dlsym(handle,"OlOpenIm");
		imf_struct->OlCloseIm = (void(*)()) dlsym(handle,"OlCloseIm");
		imf_struct->OlCreateIc =
				(OlIc *(*)()) dlsym(handle,"OlCreateIc");
		imf_struct->OlDestroyIc =
				(void(*)()) dlsym(handle,"OlDestroyIc");
		imf_struct->OlLookupImString =
				(int (*)()) dlsym(handle,"OlLookupImString");
		imf_struct->OlSetIcFocus =
				(void(*)()) dlsym(handle,"OlSetIcFocus");
		imf_struct->OlUnsetIcFocus =
				(void(*)()) dlsym(handle,"OlUnsetIcFocus");
		imf_struct->OlGetIcValues =
				(char *(*)()) dlsym(handle,"OlGetIcValues");
		imf_struct->OlSetIcValues =
				(char *(*)()) dlsym(handle,"OlSetIcValues");
		imf_struct->OlResetIc =
				(char *(*)()) dlsym(handle,"OlResetIc");
		imf_struct->OlImOfIc =
				(OlIm *(*)()) dlsym(handle,"OlImOfIc");
		imf_struct->OlDisplayOfIm =
				(Display *(*)()) dlsym(handle,"OlDisplayOfIm");
		imf_struct->OlLocaleOfIm =
				(char *(*)()) dlsym(handle,"OlLocaleOfIm");
		imf_struct->OlGetImValues = (void(*)()) dlsym(
					     handle,"OlGetImValues");
		   /*
			 *	Save name of im for future use
			 */
		prev_im = XtNewString(im_name);
	}
   else{
			/* 
			 *	Print warning message and clear return value.
			 * prev_im and handle have already been cleared
			 * for future calls.
			 */
      OlVaDisplayWarningMsg((Display *) dpy,
                            OleNOlInitialize,
                            OleTinputMethod,
                            OleCOlToolkitWarning,
                            OleMOlInitialize_inputMethod,
                            dlerror());
		XtFree((char *)imf_struct);
		imf_struct = (OlImFunctions *) NULL;
   }
	free(dirpath);
	dirpath = env = (char *) NULL;
	return(imf_struct);

#else /* SVR4 not defined */
	return((OlImFunctions *) NULL);
#endif

}
