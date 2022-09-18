/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/site.h	1.5"

/*copyright   "%c%"*/

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
********************************************************/

#ifndef SITE_H
#define SITE_H
/*
 * The following constants are provided solely as a last line of defense.  The
 * normal build ALWAYS overrides them using a special rule given in
 * server/dix/Imakefile.  If you want to change either of these constants, 
 * you should set the DefaultFontPath or DefaultRGBDatabase configuration 
 * parameters in util/imake.includes/site.def or util/imake.includes/*.macros.
 * DO NOT CHANGE THESE VALUES OR THE DIX IMAKEFILE!
 */
#ifndef COMPILEDDEFAULTFONTPATH
#define COMPILEDDEFAULTFONTPATH	"/usr/lib/X11/fonts/misc/"
#endif
#ifndef RGB_DB
#define RGB_DB			"/usr/lib/X11/rgb"
#endif

/*
 * The following constants contain default values for all of the variables 
 * that can be initialized on the server command line or in the environment.
 */
#ifdef USE_TIMEOUT
#define DEFAULT_TIMEOUT         32767   /* seconds; increased from 60.  do better next load. */
#endif
#define COMPILEDDEFAULTFONT	"fixed"
#define COMPILEDCURSORFONT	"cursor"
#define COMPILEDDISPLAYCLASS	"MIT-unspecified"
#define DEFAULT_KEYBOARD_CLICK 	0
#define DEFAULT_BELL		50
#define DEFAULT_BELL_PITCH	400
#define DEFAULT_BELL_DURATION	100
#define DEFAULT_AUTOREPEAT	FALSE
#define DEFAULT_AUTOREPEATS	{\
	0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0 }
#define DEFAULT_LEDS		0x0        /* all off */

#define DEFAULT_PTR_NUMERATOR	2
#define DEFAULT_PTR_DENOMINATOR	1
#define DEFAULT_PTR_THRESHOLD	4

/* USL XWIN : default time 10 minutes */
#define DEFAULT_SCREEN_SAVER_TIME (10 * (60 * 1000))
#define DEFAULT_SCREEN_SAVER_INTERVAL (10 * (60 * 1000))
/* #define DEFAULT_SCREEN_SAVER_TIME (2 * (60 * 1000)) */
/* #define DEFAULT_SCREEN_SAVER_INTERVAL (2 * (60 * 1000)) */

#define DEFAULT_SCREEN_SAVER_BLANKING PreferBlanking
#define DEFAULT_SCREEN_SAVER_EXPOSURES AllowExposures
#ifndef NOLOGOHACK
#define DEFAULT_LOGO_SCREEN_SAVER 1
#endif
#ifndef DEFAULT_ACCESS_CONTROL
#define DEFAULT_ACCESS_CONTROL TRUE
#endif

#endif /* SITE_H */
