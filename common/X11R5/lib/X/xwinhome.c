#ident	"@(#)R5Xlib:xwinhome.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <X11/Xos.h>
#include <Xlibint.h>

extern char *getenv ();
extern char *strcat ();
extern char *strcpy ();

/* Returns the location of X 
 * If the environment variable XWINHOME is set the value returned is
 * $(XWINHOME)/name.  Else, if this module was compiled with
 * XWINHOME set then the value returned is $(XWINHOME)/name.
 * Else, "/usr/X/name" is returned.
 *
 * NOTE: memory allocation and freeing is done by this function. The memory
 * allocated in the current call, will be freed up during next call to this
 * function. So, if you
 * call this function, the returned value is guaranteed ONLY until the next
 * call to GetXWINHOME. You need to make a local copy if you need the returned
 * value  at a different time.
 */

char *
GetXWINHome (name)
char *name;
{
    static char *path = (char *)0;
    static char *env = (char *)0;

    if (name[0] == '/') {
	return (name);
    }
    if (env == (char *)0) {
	if ((env = (char *)getenv ("XWINHOME")) == (char *)0) {
#ifdef XWINHOME
            env = XWINHOME;
#else
            env = "/usr/X";
#endif /* XWINHOME */
	}
    }
    if (path != (char *)0) {
	Xfree (path);
    }
    path = Xmalloc (strlen(env) + strlen(name) + 2);
    (void)strcpy (path, env);
    (void)strcat (path, "/");
    (void)strcat (path, name);
    return (path);
}
