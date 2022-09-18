/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:dix/globals.c	1.1"
/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



********************************************************/

/* $XConsortium: globals.c,v 1.47 89/12/07 20:33:48 keith Exp $ */

#include "X.h"
#include "Xmd.h"
#include "misc.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "input.h"
#include "dixfont.h"
#include "site.h"
#include "dixstruct.h"
#include "os.h"

ScreenInfo screenInfo;
KeybdCtrl defaultKeyboardControl = {
	DEFAULT_KEYBOARD_CLICK,
	DEFAULT_BELL,
	DEFAULT_BELL_PITCH,
	DEFAULT_BELL_DURATION,
	DEFAULT_AUTOREPEAT,
	DEFAULT_AUTOREPEATS,
	DEFAULT_LEDS};

PtrCtrl defaultPointerControl = {
	DEFAULT_PTR_NUMERATOR,
	DEFAULT_PTR_DENOMINATOR,
	DEFAULT_PTR_THRESHOLD};

ClientPtr *clients;
ClientPtr  serverClient;
int  currentMaxClients;   /* current size of clients array */

WindowPtr *WindowTable;

unsigned long globalSerialNumber = 0;
unsigned long serverGeneration = 0;

/* these next four are initialized in main.c */
long ScreenSaverTime;
long ScreenSaverInterval;
int  ScreenSaverBlanking;
int  ScreenSaverAllowExposures;

long defaultScreenSaverTime = DEFAULT_SCREEN_SAVER_TIME;
long defaultScreenSaverInterval = DEFAULT_SCREEN_SAVER_INTERVAL;
int  defaultScreenSaverBlanking = DEFAULT_SCREEN_SAVER_BLANKING;
int  defaultScreenSaverAllowExposures = DEFAULT_SCREEN_SAVER_EXPOSURES;
#ifndef NOLOGOHACK
int  logoScreenSaver = DEFAULT_LOGO_SCREEN_SAVER;
#endif

char *defaultFontPath = COMPILEDDEFAULTFONTPATH;
char *defaultTextFont = COMPILEDDEFAULTFONT;
char *defaultCursorFont = COMPILEDCURSORFONT;
char *rgbPath = RGB_DB;
char *CLrgbPath = (char *)0;	/* Command line value for rgbPath ... rjk */
char *defaultDisplayClass = COMPILEDDISPLAYCLASS;
FontPtr defaultFont;   /* not declared in dix.h to avoid including font.h in
			every compilation of dix code */
CursorPtr rootCursor;
ClientPtr requestingClient;	/* XXX this should be obsolete now, remove? */

TimeStamp currentTime;

Bool permitOldBugs = FALSE; /* turn off some error checking, to permit certain
			     * old broken clients (like R2/R3 xterms) to work
			     */

int defaultColorVisualClass = -1;
int monitorResolution = 0;

/* Sun River Work: display changed from char * to array */
char display[ 4 ];
Bool SunRiverStation = FALSE;

#ifdef USE_TIMEOUT
long TimeOutValue = DEFAULT_TIMEOUT * MILLI_PER_SECOND;
#endif
int	argcGlobal;
char	**argvGlobal;
