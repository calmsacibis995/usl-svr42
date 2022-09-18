#ident	"@(#)siserver:ddx/io/dyn.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include "xwin_io.h"
#include "siconfig.h"

/*
 *	The link between the core Xwin server and the Display Library is the
 *	"ScreenInterface" structure. 
 *
 */

#ifdef SHARED_DISPLIB 
#include <dlfcn.h>
/* ScreenInterface *HWroutines = NULL; */

Bool
LoadDisplayLibrary(name)
char *name;
{
        void *handle;
        ScreenInterface  **pDisplayFuncs;
	extern ScreenInterface *HWroutines;

        handle = dlopen(name, RTLD_NOW);
        if(handle == NULL)
        {
                printf("dlopen <%s> failed\nReason: %s\n", name, dlerror());
                return(-1);
        }
        pDisplayFuncs = (ScreenInterface  **) dlsym(handle, "DisplayFuncs");
        if(pDisplayFuncs == NULL)
        {
                printf("dlsym DisplayFuncs in <%s> failed\nReason: %s\n",
			 name, dlerror());
                dlclose(handle);
                return(-1);
        }
        HWroutines = *pDisplayFuncs;

        return(TRUE);
}
#else
Bool
LoadDisplayLibrary(name)
char *name;
{
	extern ScreenInterface *HWroutines, *DisplayFuncs;

	if (DisplayFuncs) {
		HWroutines = DisplayFuncs;
		return (TRUE);
	}
	else {
		return (FALSE);
	}
}
#endif
