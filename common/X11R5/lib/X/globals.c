/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:globals.c	1.2"
/*
 * $XConsortium: globals.c,v 1.13 91/07/12 15:54:41 gildea Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 */

/*
*/

/*
 *
 *                                 Global data
 *
 * This file should contain only those objects which must be predefined.
 */
#define NEED_EVENTS
#include <X11/Xlibint.h>


/*
 * If possible, it is useful to have the global data default to a null value.
 * Some shared library implementations are *much* happier if there isn't any
 * global initialized data.
 */
#ifdef NULL_NOT_ZERO			/* then need to initialize */
#define SetZero(t,var,z) t var = z
#else 
#define SetZero(t,var,z) t var
#endif

#ifdef USL_SHAREDLIB			/* then need extra variables */
/*
 * If we need to define extra variables for each global
 */
#if defined(__STDC__) && !defined(UNIXCPP)  /* then ANSI C concatenation */
#define ZEROINIT(t,var,val) SetZero(t,var,val); \
  SetZero (long, _libX_##var##Flag, 0); \
  SetZero (void *, _libX_##var##Ptr, NULL)
#else /* else pcc concatenation */
#define ZEROINIT(t,var,val) SetZero(t,var,val); \
  SetZero (long, _libX_/**/var/**/Flag, 0); \
  SetZero (void *, _libX_/**/var/**/Ptr, NULL)
#endif /* concat ANSI C vs. pcc */

#else /* else not USL_SHAREDLIB */
/*
 * no extra crud
 */
#define ZEROINIT(t,var,val) SetZero (t, var, val)

#endif /* USL_SHAREDLIB */


/*
 * Error handlers; used to be in XlibInt.c
 */
typedef int (*funcptr)();
ZEROINIT (funcptr, _XErrorFunction, NULL);
ZEROINIT (funcptr, _XIOErrorFunction, NULL);
ZEROINIT (_XQEvent *, _qfree, NULL);


/*
 * Debugging information and display list; used to be in XOpenDis.c
 */
ZEROINIT (int, _Xdebug, 0);
ZEROINIT (Display *, _XHeadOfDisplayList, NULL);




#ifdef STREAMSCONN


/* The following are how the Xstream connections are used:              */
/*      1)      Local connections over pseudo-tty ports.                */
/*      2)      SVR4 local connections using named streams or SVR3.2    */
/*              local connections using streams.                        */
/*      3)      SVR4 stream pipe code. This code is proprietary and     */
/*              the actual code is not included in the MIT distribution.*/
/*      4)      remote connections using tcp                            */
/*      5)      remote connections using StarLan                        */

/*
 * descriptor block for streams connections
 */

#include "Xstreams.h"

char _XsTypeOfStream[100] = { 0 };

extern int write();
extern int close();
#ifdef SVR4
extern int _XsSetupNamedStream();
/* Enhanced Application Compatibility Support */
extern int _XsSetupSp2Stream();
extern int _XsSetupSpStream();
/* End Enhanced Application Compatibility Support */
#endif 
extern int _XsSetupLocalStream();
extern int _XsConnectLocalClient();
extern int _XsCallLocalServer();
extern int _XsReadLocalStream();
extern int _XsErrorCall();
extern int _XsWriteLocalStream();
extern int _XsCloseLocalStream(); 
extern int _XsSetupTliStream();
extern int _XsConnectTliClient();
extern int _XsCallTliServer(); 
extern int _XsReadTliStream(); 
extern int _XsWriteTliStream();
extern int _XsCloseTliStream();


Xstream _XsStream[] = {

    { 
	/* local connections using pseudo-ttys */

	_XsSetupLocalStream,
	_XsConnectLocalClient,
	_XsCallLocalServer,
	_XsReadLocalStream,
	_XsErrorCall,
	write,
	close,
	NULL
    },
    { 
#ifdef SVR4
	/* local connections using named streams */

        _XsSetupNamedStream,
#else
	/* local connections using streams */
        _XsSetupLocalStream,
#endif
        _XsConnectLocalClient,
        _XsCallLocalServer,
        _XsReadLocalStream,
        _XsErrorCall,
        write,
        close,
        NULL
    },
    /* Enhanced Application Compatibility Support */
    {
#ifdef SVR4
	/* SVR4 stream pipe code */
	_XsSetupSpStream,
#else
	_XsSetupLocalStream,
#endif
	_XsConnectLocalClient,
	_XsCallLocalServer,
	_XsReadLocalStream,
	_XsErrorCall,
	write,
	close,
	NULL
    },
    /* End Enhanced Application Compatibility Support */

    {
#ifdef SVR4
	/* SVR4 stream pipe code */
	_XsSetupSp2Stream,
#else
	_XsSetupLocalStream,
#endif
	_XsConnectLocalClient,
	_XsCallLocalServer,
	_XsReadLocalStream,
	_XsErrorCall,
	write,
	close,
	NULL
    },
    {
	/* remote connections using tcp */
        _XsSetupTliStream,
        _XsConnectTliClient,
        _XsCallTliServer,
        _XsReadLocalStream,
        _XsErrorCall,
	write,
	close,
	NULL
    },
    {
	/* remote connections using StarLan */
        _XsSetupTliStream,
        _XsConnectTliClient,
        _XsCallTliServer,
        _XsReadLocalStream,
        _XsErrorCall,
        write,
        close,
        NULL
    }
};


#endif /* STREAMSCONN */


#ifdef XTEST1
/*
 * Stuff for input synthesis extension:
 */
/*
 * Holds the two event type codes for this extension.  The event type codes
 * for this extension may vary depending on how many extensions are installed
 * already, so the initial values given below will be added to the base event
 * code that is aquired when this extension is installed.
 *
 * These two variables must be available to programs that use this extension.
 */
int			XTestInputActionType = 0;
int			XTestFakeAckType   = 1;
#endif

/*
 * NOTE: any additional external definition NEED
 * to be inserted BELOW this point!!!
 */

/*
 * NOTE: any additional external definition NEED
 * to be inserted ABOVE this point!!!
 */


 _XQEvent * _qfree = NULL;
 long _qfreeFlag = 0;
 void * _qfreePtr = NULL;

